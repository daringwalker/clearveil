#include "vipsimagesource.h"

#include <QByteArray>
#include <QColorSpace>
#include <QFile>
#include <QFileInfo>
#include <QMutexLocker>
#include <QSaveFile>
#include <QTemporaryFile>
#include <QTimer>

#pragma push_macro("signals")
#undef signals
#include <vips/vips.h>
#pragma pop_macro("signals")

#include <algorithm>
#include <cmath>
#include <mutex>
#include <utility>

#if defined(__GLIBC__)
#include <malloc.h>
#endif

namespace {
constexpr qint64 kVipsCacheBytes = 64LL * 1024 * 1024;
constexpr qint64 kVipsDiscThresholdBytes = 64LL * 1024 * 1024;

std::once_flag s_vipsInitialization;
bool s_vipsAvailable = false;

void initializeVips()
{
    std::call_once(s_vipsInitialization, [] {
        // Formats such as PNG cannot provide random regions. Force their
        // decompressed backing store to disk before it can become a second
        // full-size anonymous allocation in this process.
        if (!qEnvironmentVariableIsSet("VIPS_DISC_THRESHOLD")) {
            qputenv("VIPS_DISC_THRESHOLD",
                    QByteArray::number(kVipsDiscThresholdBytes));
        }
        if (VIPS_INIT("clearveil") != 0) {
            vips_error_clear();
            return;
        }
        vips_concurrency_set(1);
        vips_cache_set_max_mem(kVipsCacheBytes);
        vips_cache_set_max_files(8);
        // Clearveil owns bounded preview and tile caches. Disabling the
        // process-global operation cache prevents an inactive PNG load
        // operation from retaining its large disk backing after the document
        // releases the random-access source.
        vips_cache_set_max(0);
        s_vipsAvailable = true;
    });
}

QString takeVipsError(const QString &fallback)
{
    const char *buffer = vips_error_buffer();
    const QString detail = buffer && *buffer
        ? QString::fromUtf8(buffer).trimmed() : fallback;
    vips_error_clear();
    return detail;
}

class ScopedVipsImage
{
public:
    ScopedVipsImage() = default;
    explicit ScopedVipsImage(VipsImage *image) : m_image(image) {}
    ~ScopedVipsImage()
    {
        if (m_image)
            g_object_unref(m_image);
    }
    ScopedVipsImage(const ScopedVipsImage &) = delete;
    ScopedVipsImage &operator=(const ScopedVipsImage &) = delete;
    ScopedVipsImage(ScopedVipsImage &&other) noexcept
        : m_image(std::exchange(other.m_image, nullptr)) {}

    [[nodiscard]] VipsImage *get() const { return m_image; }
    [[nodiscard]] VipsImage **out()
    {
        if (m_image)
            g_object_unref(std::exchange(m_image, nullptr));
        return &m_image;
    }
    [[nodiscard]] VipsImage *release()
    {
        return std::exchange(m_image, nullptr);
    }

private:
    VipsImage *m_image = nullptr;
};

QImage imageFromVips(VipsImage *input, QString *errorMessage)
{
    if (!input)
        return {};

    ScopedVipsImage colour;
    if (vips_colourspace(input, colour.out(), VIPS_INTERPRETATION_sRGB,
                         nullptr) != 0) {
        if (errorMessage)
            *errorMessage = takeVipsError(
                VipsImageSource::tr("Could not convert the image to sRGB."));
        return {};
    }

    ScopedVipsImage bytes;
    VipsImage *working = colour.get();
    if (vips_image_get_format(working) != VIPS_FORMAT_UCHAR) {
        if (vips_cast(working, bytes.out(), VIPS_FORMAT_UCHAR,
                      nullptr) != 0) {
            if (errorMessage)
                *errorMessage = takeVipsError(
                    VipsImageSource::tr("Could not convert the image pixels."));
            return {};
        }
        working = bytes.get();
    }

    ScopedVipsImage rgba;
    const int bands = vips_image_get_bands(working);
    if (bands == 3) {
        if (vips_addalpha(working, rgba.out(), nullptr) != 0) {
            if (errorMessage)
                *errorMessage = takeVipsError(
                    VipsImageSource::tr("Could not add the image alpha channel."));
            return {};
        }
        working = rgba.get();
    } else if (bands > 4) {
        if (vips_extract_band(working, rgba.out(), 0,
                              "n", 4, nullptr) != 0) {
            if (errorMessage)
                *errorMessage = takeVipsError(
                    VipsImageSource::tr("Could not select display color channels."));
            return {};
        }
        working = rgba.get();
    }

    if (vips_image_get_bands(working) != 4) {
        if (errorMessage) {
            *errorMessage = VipsImageSource::tr(
                "The large-image backend produced an unsupported pixel layout.");
        }
        return {};
    }

    size_t byteCount = 0;
    void *pixels = vips_image_write_to_memory(working, &byteCount);
    if (!pixels) {
        if (errorMessage)
            *errorMessage = takeVipsError(
                VipsImageSource::tr("Could not transfer the decoded image pixels."));
        return {};
    }

    const int width = vips_image_get_width(working);
    const int height = vips_image_get_height(working);
    const qsizetype expected = static_cast<qsizetype>(width)
        * height * 4;
    if (width <= 0 || height <= 0
        || byteCount < static_cast<size_t>(expected)) {
        g_free(pixels);
        if (errorMessage)
            *errorMessage = VipsImageSource::tr(
                "The large-image backend returned incomplete pixels.");
        return {};
    }

    const QImage wrapped(
        static_cast<const uchar *>(pixels), width, height,
        width * 4, QImage::Format_RGBA8888);
    QImage result = wrapped.copy();
    g_free(pixels);
    result.setColorSpace(QColorSpace(QColorSpace::SRgb));
    return result;
}
}

VipsImageSource::VipsImageSource(
    QString filePath, QSize logicalSize, QImage previewImage)
    : m_filePath(std::move(filePath))
    , m_logicalSize(logicalSize)
    , m_preview(std::move(previewImage))
{
}

VipsImageSource::~VipsImageSource()
{
    const QMutexLocker locker(&m_mutex);
    if (m_randomImage)
        g_object_unref(std::exchange(m_randomImage, nullptr));
}

bool VipsImageSource::isAvailable()
{
    initializeVips();
    return s_vipsAvailable;
}

bool VipsImageSource::supportsFile(const QString &filePath)
{
    if (!isAvailable())
        return false;
    const QByteArray encoded = QFile::encodeName(filePath);
    const char *loader = vips_foreign_find_load(encoded.constData());
    if (!loader) {
        vips_error_clear();
        return false;
    }
    return true;
}

std::shared_ptr<VipsImageSource> VipsImageSource::open(
    const QString &filePath, int previewMaximumDimension,
    std::stop_token stopToken, QString *errorMessage)
{
    if (!isAvailable()) {
        if (errorMessage)
            *errorMessage = tr("The large-image backend is unavailable.");
        return {};
    }
    if (stopToken.stop_requested())
        return {};

    const QString absolutePath = QFileInfo(filePath).absoluteFilePath();
    const QByteArray encoded = QFile::encodeName(absolutePath);
    ScopedVipsImage raw(vips_image_new_from_file(
        encoded.constData(), "access", VIPS_ACCESS_SEQUENTIAL,
        nullptr));
    if (!raw.get()) {
        if (errorMessage)
            *errorMessage = takeVipsError(tr("Could not open the large image."));
        return {};
    }

    ScopedVipsImage oriented;
    if (vips_autorot(raw.get(), oriented.out(), nullptr) != 0) {
        if (errorMessage)
            *errorMessage = takeVipsError(tr("Could not apply image orientation."));
        return {};
    }
    const QSize logicalSize(
        vips_image_get_width(oriented.get()),
        vips_image_get_height(oriented.get()));
    if (!logicalSize.isValid()) {
        if (errorMessage)
            *errorMessage = tr("The large image has invalid dimensions.");
        return {};
    }

    previewMaximumDimension = std::clamp(
        previewMaximumDimension, 512, 8192);
    ScopedVipsImage thumbnail;
    if (vips_thumbnail(encoded.constData(), thumbnail.out(),
                       previewMaximumDimension,
                       "height", previewMaximumDimension,
                       "size", VIPS_SIZE_DOWN,
                       "auto_rotate", TRUE,
                       nullptr) != 0) {
        if (errorMessage)
            *errorMessage = takeVipsError(tr("Could not create the large-image preview."));
        return {};
    }
    if (stopToken.stop_requested())
        return {};

    QImage previewImage = imageFromVips(thumbnail.get(), errorMessage);
    if (previewImage.isNull())
        return {};
    return std::shared_ptr<VipsImageSource>(
        new VipsImageSource(absolutePath, logicalSize,
                            std::move(previewImage)));
}

QSize VipsImageSource::logicalSize() const
{
    return m_logicalSize;
}

QImage VipsImageSource::preview() const
{
    return m_preview;
}

QString VipsImageSource::filePath() const
{
    return m_filePath;
}

bool VipsImageSource::isRegionBacked() const
{
    return true;
}

void VipsImageSource::releaseTransientResources()
{
    {
        const QMutexLocker locker(&m_mutex);
        if (!m_randomImage)
            return;
        // Ask the loader pipeline to close file handles and temporary backing
        // stores before releasing our final reference. A later region read
        // opens a fresh random-access pipeline without regenerating preview.
        vips_image_minimise_all(m_randomImage);
        g_object_unref(std::exchange(m_randomImage, nullptr));
    }
#if defined(__GLIBC__)
    // The document change emitted immediately after this call also clears the
    // old canvas tile cache. Trim on the next event-loop turn so both libvips
    // work buffers and those tiles have been released first.
    if (QCoreApplication *application = QCoreApplication::instance()) {
        // Give the asynchronous display-color replacement a short chance to
        // drop its previous preview as well; this stays outside the switch
        // interaction's critical path.
        QTimer::singleShot(250, application, [] { malloc_trim(0); });
    } else {
        malloc_trim(0);
    }
#endif
}

bool VipsImageSource::ensureRandomImageLocked(QString *errorMessage)
{
    if (m_randomImage)
        return true;
    const QByteArray encoded = QFile::encodeName(m_filePath);
    ScopedVipsImage raw(vips_image_new_from_file(
        encoded.constData(), "access", VIPS_ACCESS_RANDOM,
        nullptr));
    if (!raw.get()) {
        if (errorMessage)
            *errorMessage = takeVipsError(tr("Could not prepare random access to the image."));
        return false;
    }
    ScopedVipsImage oriented;
    if (vips_autorot(raw.get(), oriented.out(), nullptr) != 0) {
        if (errorMessage)
            *errorMessage = takeVipsError(tr("Could not apply image orientation."));
        return false;
    }
    m_randomImage = oriented.release();
    return true;
}

QImage VipsImageSource::readRegion(
    const QRect &sourceRect, const QSize &outputSize,
    std::stop_token stopToken, QString *errorMessage)
{
    if (stopToken.stop_requested())
        return {};
    const QRect valid = sourceRect.normalized().intersected(
        QRect(QPoint(), m_logicalSize));
    if (!valid.isValid() || !outputSize.isValid())
        return {};

    const QMutexLocker locker(&m_mutex);
    if (!ensureRandomImageLocked(errorMessage)
        || stopToken.stop_requested()) {
        return {};
    }

    ScopedVipsImage cropped;
    if (vips_crop(m_randomImage, cropped.out(),
                  valid.x(), valid.y(), valid.width(), valid.height(),
                  nullptr) != 0) {
        if (errorMessage)
            *errorMessage = takeVipsError(tr("Could not read the requested image region."));
        return {};
    }

    VipsImage *working = cropped.get();
    ScopedVipsImage resized;
    if (outputSize != valid.size()) {
        const double horizontalScale = static_cast<double>(outputSize.width())
            / valid.width();
        const double verticalScale = static_cast<double>(outputSize.height())
            / valid.height();
        if (vips_resize(working, resized.out(), horizontalScale,
                        "vscale", verticalScale,
                        "kernel", VIPS_KERNEL_LANCZOS3,
                        nullptr) != 0) {
            if (errorMessage)
                *errorMessage = takeVipsError(tr("Could not scale the requested image region."));
            return {};
        }
        working = resized.get();
    }
    if (stopToken.stop_requested())
        return {};
    return imageFromVips(working, errorMessage);
}

bool VipsImageSource::writeToFile(
    const QString &targetPath, std::stop_token stopToken,
    QString *errorMessage)
{
    if (stopToken.stop_requested())
        return false;
    const QFileInfo targetInfo(targetPath);
    if (targetPath.trimmed().isEmpty()
        || targetInfo.suffix().isEmpty()) {
        if (errorMessage)
            *errorMessage = tr("The export path has no image format extension.");
        return false;
    }

    const QString temporaryTemplate = targetInfo.absolutePath()
        + QStringLiteral("/.clearveil-export-XXXXXX.")
        + targetInfo.suffix();
    QTemporaryFile temporary(temporaryTemplate);
    temporary.setAutoRemove(true);
    if (!temporary.open()) {
        if (errorMessage)
            *errorMessage = temporary.errorString();
        return false;
    }
    const QString temporaryPath = temporary.fileName();
    temporary.close();

    {
        const QMutexLocker locker(&m_mutex);
        if (!ensureRandomImageLocked(errorMessage)
            || stopToken.stop_requested()) {
            return false;
        }
        const QByteArray encoded = QFile::encodeName(temporaryPath);
        if (vips_image_write_to_file(
                m_randomImage, encoded.constData(), nullptr) != 0) {
            if (errorMessage)
                *errorMessage = takeVipsError(
                    tr("Could not encode the large image."));
            return false;
        }
    }
    if (stopToken.stop_requested())
        return false;

    QFile encodedInput(temporaryPath);
    if (!encodedInput.open(QIODevice::ReadOnly)) {
        if (errorMessage)
            *errorMessage = encodedInput.errorString();
        return false;
    }
    QSaveFile output(targetInfo.absoluteFilePath());
    if (!output.open(QIODevice::WriteOnly)) {
        if (errorMessage)
            *errorMessage = output.errorString();
        return false;
    }
    QByteArray buffer(1024 * 1024, Qt::Uninitialized);
    while (!encodedInput.atEnd()) {
        if (stopToken.stop_requested()) {
            output.cancelWriting();
            return false;
        }
        const qint64 read = encodedInput.read(
            buffer.data(), buffer.size());
        if (read < 0 || output.write(buffer.constData(), read) != read) {
            if (errorMessage) {
                *errorMessage = read < 0
                    ? encodedInput.errorString() : output.errorString();
            }
            output.cancelWriting();
            return false;
        }
    }
    if (!output.commit()) {
        if (errorMessage)
            *errorMessage = output.errorString();
        return false;
    }
    return true;
}
