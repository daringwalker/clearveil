#include "imageloadcontroller.h"

#include <QFileInfo>
#include <QFutureWatcher>
#include <QImageReader>
#include <QTimer>
#include <QtConcurrentRun>

#include <algorithm>
#include <limits>
#include <utility>

ImageLoadController::ImageLoadController(
    QObject *parent, Decoder decoder)
    : QObject(parent)
    , m_decoder(decoder ? std::move(decoder)
                        : Decoder([](const QString &path,
                                     std::stop_token stopToken) {
        ImageDecodeOptions options;
        options.normalizeToSrgb = false;
        return ImageDecoder::decode(path, options, stopToken);
    }))
{
    m_loadPool.setMaxThreadCount(1);
    m_loadPool.setExpiryTimeout(10'000);
    m_prefetchPool.setMaxThreadCount(1);
    m_prefetchPool.setExpiryTimeout(10'000);
}

ImageLoadController::ImageLoadController(
    QObject *parent, LegacyDecoder decoder)
    : ImageLoadController(
          parent,
          Decoder([decoder = std::move(decoder)](
                      const QString &path, std::stop_token) {
              return decoder(path);
          }))
{
}

ImageLoadController::~ImageLoadController()
{
    m_loadStopSource.request_stop();
    m_prefetchStopSource.request_stop();
    m_loadPool.clear();
    m_prefetchPool.clear();
}

void ImageLoadController::request(
    const QString &path, int contextIndex)
{
    m_loadStopSource.request_stop();
    m_loadStopSource = std::stop_source();
    ++m_loadGeneration;
    const quint64 generation = m_loadGeneration;
    m_loadPool.clear();
    m_prefetchStopSource.request_stop();
    ++m_prefetchGeneration;
    m_prefetchPool.clear();
    setLoading(true);
    const QString normalized = QFileInfo(path).absoluteFilePath();
    emit loadStarted(normalized, contextIndex);

    const QString key = cacheKey(normalized);
    if (const ImageLoadResult *cached = m_cache.object(key)) {
        const ImageLoadResult result = *cached;
        QTimer::singleShot(
            0, this,
            [this, result, contextIndex, generation] {
                if (generation != m_loadGeneration)
                    return;
                setLoading(false);
                emit loadFinished(result, contextIndex, true);
            });
        return;
    }

    const Decoder decoder = m_decoder;
    const std::stop_token stopToken = m_loadStopSource.get_token();
    auto future = QtConcurrent::run(
        &m_loadPool,
        [decoder, normalized, stopToken] {
            return decoder(normalized, stopToken);
        });
    auto *watcher = new QFutureWatcher<ImageLoadResult>(this);
    connect(watcher, &QFutureWatcher<ImageLoadResult>::finished,
            this, [this, watcher, generation, contextIndex] {
        const ImageLoadResult result = watcher->result();
        watcher->deleteLater();
        if (generation != m_loadGeneration)
            return;
        if (result.succeeded())
            store(result);
        setLoading(false);
        emit loadFinished(result, contextIndex, false);
    });
    watcher->setFuture(future);
}

void ImageLoadController::remember(const ImageLoadResult &result)
{
    if (result.succeeded())
        store(result);
}

void ImageLoadController::cancel()
{
    m_loadStopSource.request_stop();
    ++m_loadGeneration;
    m_loadPool.clear();
    m_prefetchStopSource.request_stop();
    ++m_prefetchGeneration;
    m_prefetchPool.clear();
    setLoading(false);
}

void ImageLoadController::prefetch(const QStringList &paths)
{
    m_prefetchStopSource.request_stop();
    m_prefetchStopSource = std::stop_source();
    ++m_prefetchGeneration;
    const quint64 generation = m_prefetchGeneration;
    m_prefetchPool.clear();
    QStringList queued;
    for (const QString &path : paths) {
        const QString normalized = QFileInfo(path).absoluteFilePath();
        if (normalized.isEmpty() || queued.contains(normalized)
            || isCached(normalized)) {
            continue;
        }
        QImageReader probe(normalized);
        const QSize sourceSize = probe.size();
        constexpr qint64 conservativeBytesPerPixel = 8;
        const qint64 estimatedBytes = sourceSize.isValid()
            ? static_cast<qint64>(sourceSize.width())
                * sourceSize.height()
                * conservativeBytesPerPixel
            : 0;
        const qint64 cacheBytes =
            static_cast<qint64>(m_cache.maxCost()) * 1024;
        if (estimatedBytes > cacheBytes)
            continue;
        queued.append(normalized);
        const Decoder decoder = m_decoder;
        const std::stop_token stopToken =
            m_prefetchStopSource.get_token();
        auto future = QtConcurrent::run(
            &m_prefetchPool,
            [decoder, normalized, stopToken] {
                return decoder(normalized, stopToken);
            });
        auto *watcher = new QFutureWatcher<ImageLoadResult>(this);
        connect(watcher, &QFutureWatcher<ImageLoadResult>::finished,
                this, [this, watcher, generation] {
            const ImageLoadResult result = watcher->result();
            watcher->deleteLater();
            if (generation == m_prefetchGeneration
                && result.succeeded()) {
                store(result);
            }
        });
        watcher->setFuture(future);
    }
}

bool ImageLoadController::isLoading() const
{
    return m_loading;
}

bool ImageLoadController::isCached(const QString &path) const
{
    return m_cache.contains(cacheKey(path));
}

int ImageLoadController::cacheLimitMiB() const
{
    return m_cache.maxCost() / 1024;
}

void ImageLoadController::setCacheLimitMiB(int mebibytes)
{
    const int bounded = std::clamp(mebibytes, 16, 4096);
    m_cache.setMaxCost(bounded * 1024);
}

void ImageLoadController::clearCache()
{
    m_cache.clear();
}

QString ImageLoadController::cacheKey(const QString &path)
{
    const QFileInfo info(path);
    return QStringLiteral("%1|%2|%3")
        .arg(info.absoluteFilePath())
        .arg(info.size())
        .arg(info.lastModified().toMSecsSinceEpoch());
}

void ImageLoadController::store(const ImageLoadResult &result)
{
    const int cost = std::max(
        1, static_cast<int>(std::min<qsizetype>(
               result.image.sizeInBytes() / 1024,
               std::numeric_limits<int>::max())));
    m_cache.insert(cacheKey(result.filePath),
                   new ImageLoadResult(result), cost);
}

void ImageLoadController::setLoading(bool loading)
{
    if (m_loading == loading)
        return;
    m_loading = loading;
    emit loadingChanged(m_loading);
}
