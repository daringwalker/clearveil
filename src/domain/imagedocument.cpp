#include "imagedocument.h"

#include <QFileInfo>
#include <QTransform>

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

ImageDocument::ImageDocument(QObject *parent)
    : QObject(parent)
{
}

ImageLoadResult ImageDocument::decodeFile(const QString &filePath)
{
    ImageDecodeOptions options;
    options.normalizeToSrgb = false;
    return ImageDecoder::decode(filePath, options);
}

bool ImageDocument::load(const QString &filePath, QString *errorMessage)
{
    const ImageLoadResult result = decodeFile(filePath);
    if (!result.succeeded()) {
        if (errorMessage)
            *errorMessage = result.error;
        return false;
    }
    return loadDecoded(result, errorMessage);
}

bool ImageDocument::loadDecoded(const QString &filePath, const QImage &image,
                                QString *errorMessage)
{
    ImageLoadResult result;
    result.filePath = filePath;
    result.image = image;
    return loadDecoded(result, errorMessage);
}

bool ImageDocument::loadDecoded(
    const ImageLoadResult &result, QString *errorMessage)
{
    const QImage &image = result.image;
    if (image.isNull()) {
        if (errorMessage)
            *errorMessage = tr("The image could not be decoded.");
        return false;
    }
    if (m_imageSource)
        m_imageSource->releaseTransientResources();
    m_filePath = QFileInfo(result.filePath).absoluteFilePath();
    m_imageSource = result.source;
    m_history = {image};
    m_historyIndex = 0;
    m_savedHistoryIndex = 0;
    emit imageChanged();
    emit historyChanged();
    emit modifiedChanged(false);
    return true;
}

bool ImageDocument::loadImage(const QImage &image, QString *errorMessage)
{
    if (image.isNull()) {
        if (errorMessage)
            *errorMessage = tr("The clipboard does not contain a readable image.");
        return false;
    }

    if (m_imageSource)
        m_imageSource->releaseTransientResources();
    m_filePath.clear();
    m_imageSource.reset();
    m_history = {image};
    m_historyIndex = 0;
    m_savedHistoryIndex = -1;
    emit imageChanged();
    emit historyChanged();
    emit modifiedChanged(true);
    return true;
}

bool ImageDocument::saveAs(const QString &filePath, QString *errorMessage)
{
    const ImageExportService::Result result =
        saveAsResult(filePath);
    if (!result.succeeded() && errorMessage)
        *errorMessage = result.detail;
    return result.succeeded();
}

ImageExportService::Result ImageDocument::saveAsResult(
    const QString &filePath)
{
    if (m_historyIndex < 0)
        return {ImageExportService::Error::EmptyImage,
                filePath, {}, {}};

    ImageExportService::Result result;
    if (isRegionBacked()) {
        result.filePath = filePath.isEmpty()
            ? QString() : QFileInfo(filePath).absoluteFilePath();
        result.format = ImageExportService::formatForPath(filePath);
        if (filePath.trimmed().isEmpty()) {
            result.error = ImageExportService::Error::EmptyTargetPath;
        } else if (!m_imageSource->writeToFile(
                       result.filePath, {}, &result.detail)) {
            result.error = ImageExportService::Error::EncodeFailed;
        }
    } else {
        result = ImageExportService::writeAtomically(
            image(), filePath);
    }
    if (!result.succeeded())
        return result;

    m_filePath = result.filePath;
    m_savedHistoryIndex = m_historyIndex;
    emit modifiedChanged(false);
    return result;
}

void ImageDocument::clear()
{
    if (m_imageSource)
        m_imageSource->releaseTransientResources();
    m_filePath.clear();
    m_imageSource.reset();
    m_history.clear();
    m_historyIndex = -1;
    m_savedHistoryIndex = -1;
    emit imageChanged();
    emit historyChanged();
    emit modifiedChanged(false);
}

const QImage &ImageDocument::image() const
{
    static const QImage empty;
    return m_historyIndex >= 0 ? m_history.at(m_historyIndex) : empty;
}

ImageSourcePtr ImageDocument::imageSource() const
{
    return m_imageSource;
}

QSize ImageDocument::logicalSize() const
{
    return m_imageSource ? m_imageSource->logicalSize() : image().size();
}

bool ImageDocument::isRegionBacked() const
{
    return m_imageSource && m_imageSource->isRegionBacked();
}

QString ImageDocument::filePath() const
{
    return m_filePath;
}

bool ImageDocument::isModified() const
{
    return m_historyIndex >= 0 && m_historyIndex != m_savedHistoryIndex;
}

bool ImageDocument::canUndo() const
{
    return m_historyIndex > 0;
}

bool ImageDocument::canRedo() const
{
    return m_historyIndex >= 0 && m_historyIndex + 1 < m_history.size();
}

bool ImageDocument::crop(const QRect &rectangle)
{
    if (m_historyIndex < 0)
        return false;
    const QRect valid = rectangle.normalized().intersected(image().rect());
    if (!valid.isValid() || valid == image().rect())
        return false;
    return applyImage(image().copy(valid));
}

bool ImageDocument::resizeImage(
    const QSize &size, Qt::TransformationMode mode)
{
    if (m_historyIndex < 0 || !size.isValid() || size == image().size())
        return false;
    return applyImage(
        image().scaled(size, Qt::IgnoreAspectRatio, mode));
}

bool ImageDocument::adjustImage(
    int brightness, int contrast, qreal gamma)
{
    if (m_historyIndex < 0)
        return false;
    brightness = std::clamp(brightness, -100, 100);
    contrast = std::clamp(contrast, -100, 100);
    gamma = std::clamp(gamma, 0.1, 3.0);
    if (brightness == 0 && contrast == 0 && qFuzzyCompare(gamma, 1.0))
        return false;

    std::array<uchar, 256> table{};
    const qreal brightnessOffset = brightness * 255.0 / 100.0;
    const qreal contrastValue = contrast * 2.55;
    const qreal contrastFactor =
        (259.0 * (contrastValue + 255.0))
        / (255.0 * (259.0 - contrastValue));
    for (int value = 0; value < 256; ++value) {
        qreal adjusted = contrastFactor * (value - 128.0) + 128.0;
        adjusted += brightnessOffset;
        adjusted = std::clamp(adjusted, 0.0, 255.0);
        adjusted = 255.0 * std::pow(adjusted / 255.0, 1.0 / gamma);
        table.at(value) = static_cast<uchar>(std::clamp(qRound(adjusted), 0, 255));
    }

    QImage adjusted = image().convertToFormat(QImage::Format_RGBA8888);
    for (int y = 0; y < adjusted.height(); ++y) {
        uchar *line = adjusted.scanLine(y);
        for (int x = 0; x < adjusted.width(); ++x) {
            uchar *pixel = line + x * 4;
            pixel[0] = table.at(pixel[0]);
            pixel[1] = table.at(pixel[1]);
            pixel[2] = table.at(pixel[2]);
        }
    }
    return applyImage(adjusted);
}

bool ImageDocument::reduceRedEye(const QRect &rectangle)
{
    if (m_historyIndex < 0)
        return false;
    const QRect area = rectangle.normalized().intersected(image().rect());
    if (!area.isValid())
        return false;

    QImage corrected = image().convertToFormat(QImage::Format_RGBA8888);
    const QPointF center = area.center();
    const qreal radiusX = std::max(1.0, area.width() / 2.0);
    const qreal radiusY = std::max(1.0, area.height() / 2.0);
    bool changed = false;
    for (int y = area.top(); y <= area.bottom(); ++y) {
        uchar *line = corrected.scanLine(y);
        for (int x = area.left(); x <= area.right(); ++x) {
            const qreal dx = (x - center.x()) / radiusX;
            const qreal dy = (y - center.y()) / radiusY;
            if (dx * dx + dy * dy > 1.0)
                continue;
            uchar *pixel = line + x * 4;
            const int greenBlue = (pixel[1] + pixel[2]) / 2;
            if (pixel[0] <= 90 || pixel[0] <= greenBlue * 1.35)
                continue;
            pixel[0] = static_cast<uchar>(std::clamp(
                qRound(greenBlue * 1.05), 0, 255));
            changed = true;
        }
    }
    return changed && applyImage(corrected);
}

bool ImageDocument::rotateClockwise()
{
    QTransform transform;
    return apply(transform.rotate(90));
}

bool ImageDocument::rotateCounterClockwise()
{
    QTransform transform;
    return apply(transform.rotate(-90));
}

bool ImageDocument::flipHorizontal()
{
    QTransform transform;
    return apply(transform.scale(-1, 1));
}

bool ImageDocument::flipVertical()
{
    QTransform transform;
    return apply(transform.scale(1, -1));
}

bool ImageDocument::undo()
{
    return canUndo()
        && setHistoryIndex(m_historyIndex - 1);
}

bool ImageDocument::redo()
{
    return canRedo()
        && setHistoryIndex(m_historyIndex + 1);
}

bool ImageDocument::apply(const QTransform &transform)
{
    if (m_historyIndex < 0)
        return false;
    return applyImage(
        image().transformed(transform, Qt::SmoothTransformation));
}

bool ImageDocument::applyImage(const QImage &newImage)
{
    if (m_historyIndex < 0 || newImage.isNull())
        return false;
    if (m_imageSource)
        m_imageSource->releaseTransientResources();
    m_imageSource.reset();
    m_history.resize(m_historyIndex + 1);
    m_history.append(newImage);
    return setHistoryIndex(m_history.size() - 1);
}

bool ImageDocument::setHistoryIndex(int index)
{
    if (index < 0 || index >= m_history.size()
        || index == m_historyIndex) {
        return false;
    }
    const bool wasModified = isModified();
    m_historyIndex = index;
    emit imageChanged();
    emit historyChanged();
    if (wasModified != isModified())
        emit modifiedChanged(isModified());
    return true;
}
