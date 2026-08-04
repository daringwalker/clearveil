#include "tiledimageviewmodel.h"

#include <QFutureWatcher>
#include <QtConcurrentRun>

#include <algorithm>
#include <cmath>
#include <limits>

namespace {
constexpr int kTilePixels = 512;
constexpr int kMaximumMipLevel = 12;

struct TileResult {
    QString key;
    QRect sourceRect;
    QImage image;
};
}

TiledImageViewModel::TiledImageViewModel(QObject *parent)
    : QObject(parent)
{
    // PNG random access is serialized by its temporary backing-store build.
    // One worker also gives deterministic memory use and prevents a zoom from
    // launching several decompressions of the same source.
    m_pool.setMaxThreadCount(1);
    m_pool.setExpiryTimeout(10'000);
}

TiledImageViewModel::~TiledImageViewModel()
{
    m_stopSource.request_stop();
    m_pool.clear();
    m_pool.waitForDone();
}

void TiledImageViewModel::setSource(const ImageSourcePtr &source)
{
    if (m_source == source)
        return;
    m_stopSource.request_stop();
    m_stopSource = std::stop_source();
    ++m_generation;
    m_pool.clear();
    m_source = source;
    m_cache.clear();
    m_pending.clear();
    m_needed.clear();
    m_requestQueue.clear();
    emit tilesChanged();
}

void TiledImageViewModel::updateViewport(
    const QRectF &visibleSourceRect, qreal zoom)
{
    if (!m_source || !m_source->isRegionBacked()
        || !visibleSourceRect.isValid() || zoom <= 0.0) {
        m_needed.clear();
        m_requestQueue.clear();
        return;
    }

    const QSize logicalSize = m_source->logicalSize();
    const QImage preview = m_source->preview();
    const qreal previewScale = std::min(
        static_cast<qreal>(preview.width()) / logicalSize.width(),
        static_cast<qreal>(preview.height()) / logicalSize.height());
    // The preview already contains at least one source sample per displayed
    // pixel below this scale. Avoid building the large random-access backing
    // file until the user actually zooms past preview resolution.
    if (zoom <= previewScale * 1.02) {
        m_needed.clear();
        m_requestQueue.clear();
        return;
    }

    const int level = zoom >= 1.0
        ? 0
        : std::clamp(
              static_cast<int>(std::floor(std::log2(1.0 / zoom))),
              0, kMaximumMipLevel);
    const int scale = 1 << level;
    const int sourceTilePixels = kTilePixels * scale;
    const QRect bounds(QPoint(), logicalSize);
    const QRect visible = visibleSourceRect.toAlignedRect()
        .intersected(bounds);
    if (!visible.isValid()) {
        m_needed.clear();
        m_requestQueue.clear();
        return;
    }

    const int firstColumn = visible.left() / sourceTilePixels;
    const int lastColumn = visible.right() / sourceTilePixels;
    const int firstRow = visible.top() / sourceTilePixels;
    const int lastRow = visible.bottom() / sourceTilePixels;

    QSet<QString> needed;
    QList<TileRequest> requests;
    const QPointF viewportCenter = visibleSourceRect.center();
    for (int row = firstRow; row <= lastRow; ++row) {
        for (int column = firstColumn; column <= lastColumn; ++column) {
            const QRect sourceRect(
                column * sourceTilePixels,
                row * sourceTilePixels,
                sourceTilePixels, sourceTilePixels);
            const QRect clipped = sourceRect.intersected(bounds);
            if (!clipped.isValid())
                continue;
            const QString key = tileKey(column, row, level);
            needed.insert(key);
            if (m_cache.contains(key) || m_pending.contains(key))
                continue;
            const QSize outputSize(
                std::max(1, qRound(static_cast<qreal>(clipped.width()) / scale)),
                std::max(1, qRound(static_cast<qreal>(clipped.height()) / scale)));
            const QPointF offset = clipped.center() - viewportCenter;
            requests.append({key, clipped, outputSize,
                             offset.x() * offset.x()
                                 + offset.y() * offset.y()});
        }
    }
    if (needed == m_needed)
        return;
    m_needed = std::move(needed);
    // Replace all work that has not started yet. At most one obsolete tile can
    // still be decoding, so fast panning cannot build up a long stale queue.
    std::sort(requests.begin(), requests.end(),
              [](const TileRequest &left,
                 const TileRequest &right) {
        return left.distanceSquared < right.distanceSquared;
    });
    m_requestQueue = std::move(requests);
    startNextTile();
}

QList<TiledImageViewModel::Tile>
TiledImageViewModel::visibleTiles() const
{
    QList<Tile> result;
    result.reserve(m_needed.size());
    for (const QString &key : m_needed) {
        if (const CachedTile *tile = m_cache.object(key))
            result.append({tile->sourceRect, tile->image});
    }
    return result;
}

int TiledImageViewModel::cacheLimitMiB() const
{
    return m_cache.maxCost() / 1024;
}

void TiledImageViewModel::setCacheLimitMiB(int mebibytes)
{
    m_cache.setMaxCost(std::clamp(mebibytes, 16, 256) * 1024);
}

QString TiledImageViewModel::tileKey(
    int column, int row, int level)
{
    return QStringLiteral("%1:%2:%3")
        .arg(level).arg(column).arg(row);
}

void TiledImageViewModel::startNextTile()
{
    if (m_workerBusy || !m_source)
        return;
    while (!m_requestQueue.isEmpty()) {
        const TileRequest request = m_requestQueue.takeFirst();
        if (!m_needed.contains(request.key)
            || m_cache.contains(request.key)
            || m_pending.contains(request.key)) {
            continue;
        }
        requestTile(request, m_generation);
        return;
    }
}

void TiledImageViewModel::requestTile(
    const TileRequest &request, quint64 generation)
{
    const ImageSourcePtr source = m_source;
    if (!source)
        return;
    m_workerBusy = true;
    m_pending.insert(request.key);
    const std::stop_token stopToken = m_stopSource.get_token();
    auto future = QtConcurrent::run(
        &m_pool,
        [source, request, stopToken] {
            TileResult result{
                request.key, request.sourceRect, {}};
            result.image = source->readRegion(
                request.sourceRect, request.outputSize, stopToken);
            return result;
        });
    auto *watcher = new QFutureWatcher<TileResult>(this);
    connect(watcher, &QFutureWatcher<TileResult>::finished,
            this, [this, watcher, generation] {
        const TileResult result = watcher->result();
        watcher->deleteLater();
        m_workerBusy = false;
        m_pending.remove(result.key);
        if (generation == m_generation
            && !result.image.isNull()
            && m_needed.contains(result.key)) {
            const int cost = std::max(
                1, static_cast<int>(std::min<qsizetype>(
                    result.image.sizeInBytes() / 1024,
                    std::numeric_limits<int>::max())));
            m_cache.insert(result.key,
                new CachedTile{result.sourceRect, result.image}, cost);
            emit tilesChanged();
        }
        startNextTile();
    });
    watcher->setFuture(future);
}
