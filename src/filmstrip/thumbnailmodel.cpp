#include "thumbnailmodel.h"

#include "imagesequence.h"
#include "persistentthumbnailcache.h"
#include "vipsimagesource.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QIcon>
#include <QImageReader>
#include <QLocale>
#include <QMimeData>
#include <QUrl>
#include <QtConcurrentRun>

#include <algorithm>
#include <utility>

namespace {
QString humanFileSize(qint64 bytes)
{
    const QLocale locale;
    if (bytes < 1024)
        return QObject::tr("%1 B").arg(locale.toString(bytes));
    const qreal kib = static_cast<qreal>(bytes) / 1024.0;
    if (kib < 1024.0)
        return QObject::tr("%1 KiB").arg(locale.toString(kib, 'f', 1));
    return QObject::tr("%1 MiB").arg(
        locale.toString(kib / 1024.0, 'f', 1));
}
}

ThumbnailModel::ThumbnailModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_cache(96 * 1024)
    , m_latestCache(32 * 1024)
{
    m_decodePool.setMaxThreadCount(2);
    m_decodePool.setExpiryTimeout(10'000);
}

int ThumbnailModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_entries.size();
}

QVariant ThumbnailModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return {};
    const Entry &entry = m_entries.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
        return entry.name;
    case Qt::ToolTipRole:
        if (entry.directory)
            return entry.path;
        {
            QImageReader reader(entry.path);
            const QSize dimensions = reader.size();
            QStringList details{entry.name};
            if (dimensions.isValid()) {
                details.append(tr("%1 × %2 pixels")
                    .arg(dimensions.width())
                    .arg(dimensions.height()));
            }
            details.append(humanFileSize(entry.size));
            if (entry.modified.isValid()) {
                details.append(QLocale().toString(
                    entry.modified, QLocale::ShortFormat));
            }
            details.append(entry.path);
            return details.join(QLatin1Char('\n'));
        }
    case FilePathRole:
        return entry.path;
    case IsDirectoryRole:
        return entry.directory;
    case Qt::DecorationRole:
        if (entry.directory) {
            return QIcon::fromTheme(QStringLiteral("folder"))
                .pixmap(m_thumbnailSize, QIcon::Normal, QIcon::Off);
        }
        if (QPixmap *cached = m_cache.object(cacheKey(entry)))
            return *cached;
        requestThumbnail(index.row());
        if (QPixmap *latest = m_latestCache.object(latestCacheKey(entry)))
            return *latest;
        return QIcon::fromTheme(QStringLiteral("image-x-generic"))
            .pixmap(m_thumbnailSize, QIcon::Disabled, QIcon::Off);
    default:
        return {};
    }
}

Qt::ItemFlags ThumbnailModel::flags(
    const QModelIndex &index) const
{
    Qt::ItemFlags itemFlags =
        QAbstractListModel::flags(index);
    if (index.isValid())
        itemFlags |= Qt::ItemIsDragEnabled;
    return itemFlags;
}

QStringList ThumbnailModel::mimeTypes() const
{
    return {QStringLiteral("text/uri-list")};
}

QMimeData *ThumbnailModel::mimeData(
    const QModelIndexList &indexes) const
{
    QList<QUrl> urls;
    QSet<QString> seen;
    for (const QModelIndex &index : indexes) {
        const QString path = filePath(index);
        if (path.isEmpty() || seen.contains(path))
            continue;
        urls.append(QUrl::fromLocalFile(path));
        seen.insert(path);
    }
    auto *data = new QMimeData;
    data->setUrls(urls);
    return data;
}

Qt::DropActions ThumbnailModel::supportedDragActions() const
{
    return Qt::CopyAction;
}

QHash<int, QByteArray> ThumbnailModel::roleNames() const
{
    auto roles = QAbstractListModel::roleNames();
    roles.insert(FilePathRole, "filePath");
    roles.insert(IsDirectoryRole, "isDirectory");
    return roles;
}

void ThumbnailModel::setDirectory(const QString &directoryPath)
{
    const QDir directory(directoryPath);
    if (!directory.exists())
        return;

    QList<Entry> nextEntries;
    const QFileInfoList entries = directory.entryInfoList(
        QDir::Dirs | QDir::Files | QDir::Readable | QDir::NoDotAndDotDot,
        QDir::DirsFirst | QDir::Name | QDir::IgnoreCase);
    for (const QFileInfo &info : entries) {
        if (!info.isDir() && !ImageSequence::isSupportedImage(info))
            continue;
        nextEntries.append({
            info.absoluteFilePath(), info.fileName(), info.isDir(),
            info.size(), info.lastModified(), info.suffix().toLower(),
            static_cast<int>(nextEntries.size())});
    }
    std::sort(nextEntries.begin(), nextEntries.end(),
              [](const Entry &left, const Entry &right) {
        if (left.directory != right.directory)
            return left.directory;
        return ImageSequence::naturalLess(
            left.name, right.name);
    });
    for (int row = 0; row < nextEntries.size(); ++row)
        nextEntries[row].sourceOrder = row;
    sortEntries(nextEntries);

    const QString absolutePath = directory.absolutePath();
    applyEntries(std::move(nextEntries), absolutePath);
}

void ThumbnailModel::setDirectoryEntries(
    const QString &directoryPath,
    const QList<DirectoryScanEntry> &entries,
    bool includeDirectories)
{
    QList<Entry> nextEntries;
    nextEntries.reserve(entries.size());
    int sourceOrder = 0;
    for (const DirectoryScanEntry &entry : entries) {
        if (entry.directory && !includeDirectories)
            continue;
        nextEntries.append({
            entry.path,
            includeDirectories || entry.directory
                ? entry.name
                : QFileInfo(entry.name).completeBaseName(),
            entry.directory, entry.size, entry.modified,
            entry.suffix, sourceOrder++});
    }
    sortEntries(nextEntries);
    applyEntries(std::move(nextEntries),
                 QDir(directoryPath).absolutePath());
}

void ThumbnailModel::setFiles(const QStringList &filePaths)
{
    QList<Entry> nextEntries;
    nextEntries.reserve(filePaths.size());
    QSet<QString> seen;
    for (const QString &filePath : filePaths) {
        const QFileInfo info(filePath);
        const QString absolutePath = info.absoluteFilePath();
        if (!ImageSequence::isSupportedImage(info)
            || seen.contains(absolutePath)) {
            continue;
        }
        nextEntries.append({
            absolutePath, info.completeBaseName(), false,
            info.size(), info.lastModified(), info.suffix().toLower(),
            static_cast<int>(nextEntries.size())});
        seen.insert(absolutePath);
    }
    sortEntries(nextEntries);

    applyEntries(std::move(nextEntries), {});
}

void ThumbnailModel::cacheThumbnail(
    const QString &filePath, const QImage &sourceImage)
{
    if (filePath.isEmpty() || sourceImage.isNull())
        return;

    const QFileInfo info(filePath);
    const QString absolutePath = info.absoluteFilePath();
    if (m_primaryImagePath == absolutePath)
        m_primaryImagePath.clear();
    Entry entry{
        absolutePath, info.completeBaseName(), false,
        info.size(), info.lastModified(), info.suffix().toLower(), 0};
    int row = -1;
    for (int index = 0; index < m_entries.size(); ++index) {
        if (m_entries.at(index).path == absolutePath) {
            entry = m_entries.at(index);
            row = index;
            break;
        }
    }

    const QImage thumbnail = sourceImage.scaled(
        m_thumbnailSize, Qt::KeepAspectRatio,
        Qt::SmoothTransformation);
    if (thumbnail.isNull())
        return;
    const QPixmap pixmap = QPixmap::fromImage(thumbnail);
    const int cost = std::max(
        1, static_cast<int>(thumbnail.sizeInBytes() / 1024));
    m_cache.insert(cacheKey(entry), new QPixmap(pixmap), cost);
    m_latestCache.insert(
        latestCacheKey(entry), new QPixmap(pixmap), cost);
    if (row >= 0) {
        m_pending.remove(cacheKey(entry));
        emit dataChanged(index(row), index(row),
                         {Qt::DecorationRole});
    }
}

void ThumbnailModel::setPrimaryImagePath(const QString &filePath)
{
    const QString normalized = filePath.isEmpty()
        ? QString() : QFileInfo(filePath).absoluteFilePath();
    if (m_primaryImagePath == normalized)
        return;
    const QString previous = std::exchange(
        m_primaryImagePath, normalized);
    for (int row = 0; row < m_entries.size(); ++row) {
        const QString &path = m_entries.at(row).path;
        if (path == previous || path == normalized) {
            emit dataChanged(index(row), index(row),
                             {Qt::DecorationRole});
        }
    }
}

void ThumbnailModel::setThumbnailSize(const QSize &size)
{
    if (!size.isValid() || size == m_thumbnailSize)
        return;
    m_thumbnailSize = size;
    ++m_generation;
    m_pending.clear();
    if (!m_entries.isEmpty())
        emit dataChanged(index(0), index(m_entries.size() - 1), {Qt::DecorationRole});
}

void ThumbnailModel::setSort(SortKey key, Qt::SortOrder order)
{
    if (m_sortKey == key && m_sortOrder == order)
        return;
    m_sortKey = key;
    m_sortOrder = order;
    QList<Entry> sorted = m_entries;
    sortEntries(sorted);
    if (hasSameEntries(sorted))
        return;
    beginResetModel();
    m_entries = std::move(sorted);
    ++m_generation;
    m_pending.clear();
    endResetModel();
}

void ThumbnailModel::refreshTheme()
{
    if (!m_entries.isEmpty())
        emit dataChanged(index(0), index(m_entries.size() - 1),
                         {Qt::ForegroundRole, Qt::BackgroundRole,
                          Qt::DecorationRole});
}

QString ThumbnailModel::directoryPath() const
{
    return m_directoryPath;
}

QString ThumbnailModel::filePath(const QModelIndex &index) const
{
    return index.isValid() && index.row() < m_entries.size()
        ? m_entries.at(index.row()).path : QString();
}

bool ThumbnailModel::isDirectory(const QModelIndex &index) const
{
    return index.isValid() && index.row() < m_entries.size()
        && m_entries.at(index.row()).directory;
}

QSize ThumbnailModel::thumbnailSize() const
{
    return m_thumbnailSize;
}

ThumbnailModel::SortKey ThumbnailModel::sortKey() const
{
    return m_sortKey;
}

Qt::SortOrder ThumbnailModel::sortOrder() const
{
    return m_sortOrder;
}

QString ThumbnailModel::cacheKey(const Entry &entry) const
{
    return QStringLiteral("%1|%2|%3|%4x%5")
        .arg(entry.path)
        .arg(entry.size)
        .arg(entry.modified.toMSecsSinceEpoch())
        .arg(m_thumbnailSize.width())
        .arg(m_thumbnailSize.height());
}

QString ThumbnailModel::latestCacheKey(const Entry &entry) const
{
    return QStringLiteral("%1|%2|%3")
        .arg(entry.path)
        .arg(entry.size)
        .arg(entry.modified.toMSecsSinceEpoch());
}

void ThumbnailModel::sortEntries(QList<Entry> &entries) const
{
    const auto naturalCompare = [](const QString &left,
                                   const QString &right) {
        if (ImageSequence::naturalLess(left, right))
            return -1;
        if (ImageSequence::naturalLess(right, left))
            return 1;
        return 0;
    };
    const SortKey key = m_sortKey;
    const Qt::SortOrder order = m_sortOrder;
    std::stable_sort(
        entries.begin(), entries.end(),
        [key, order, naturalCompare](const Entry &left,
                                     const Entry &right) {
            if (left.directory != right.directory)
                return left.directory;
            int comparison = 0;
            switch (key) {
            case SortKey::SourceOrder:
                comparison = left.sourceOrder < right.sourceOrder
                    ? -1 : left.sourceOrder > right.sourceOrder ? 1 : 0;
                break;
            case SortKey::Name:
                comparison = naturalCompare(left.name, right.name);
                break;
            case SortKey::ModifiedTime:
                comparison = left.modified < right.modified
                    ? -1 : left.modified > right.modified ? 1 : 0;
                break;
            case SortKey::FileSize:
                comparison = left.size < right.size
                    ? -1 : left.size > right.size ? 1 : 0;
                break;
            case SortKey::FileType:
                comparison = QString::compare(
                    left.suffix, right.suffix,
                    Qt::CaseInsensitive);
                break;
            }
            if (comparison == 0)
                comparison = naturalCompare(left.name, right.name);
            if (comparison == 0) {
                comparison = QString::compare(
                    left.path, right.path, Qt::CaseSensitive);
            }
            return order == Qt::AscendingOrder
                ? comparison < 0 : comparison > 0;
        });
}

void ThumbnailModel::applyEntries(
    QList<Entry> entries, const QString &directoryPath)
{
    if (m_directoryPath == directoryPath
        && hasSameEntries(entries)) {
        return;
    }

    const QSet<QString> nextPaths = [&entries] {
        QSet<QString> paths;
        paths.reserve(entries.size());
        for (const Entry &entry : std::as_const(entries))
            paths.insert(entry.path);
        return paths;
    }();
    bool metadataChanged = false;
    for (int row = m_entries.size() - 1; row >= 0; --row) {
        if (nextPaths.contains(m_entries.at(row).path))
            continue;
        beginRemoveRows({}, row, row);
        m_entries.removeAt(row);
        endRemoveRows();
        metadataChanged = true;
    }

    for (int targetRow = 0; targetRow < entries.size(); ++targetRow) {
        const Entry &target = entries.at(targetRow);
        int currentRow = -1;
        for (int row = targetRow; row < m_entries.size(); ++row) {
            if (m_entries.at(row).path == target.path) {
                currentRow = row;
                break;
            }
        }
        if (currentRow < 0) {
            beginInsertRows({}, targetRow, targetRow);
            m_entries.insert(targetRow, target);
            endInsertRows();
            metadataChanged = true;
            continue;
        }
        if (currentRow != targetRow) {
            beginMoveRows({}, currentRow, currentRow, {}, targetRow);
            m_entries.move(currentRow, targetRow);
            endMoveRows();
        }
        if (!metadataEqual(m_entries.at(targetRow), target)) {
            m_entries[targetRow] = target;
            emit dataChanged(index(targetRow), index(targetRow));
            metadataChanged = true;
        }
    }
    while (m_entries.size() > entries.size()) {
        const int row = m_entries.size() - 1;
        beginRemoveRows({}, row, row);
        m_entries.removeAt(row);
        endRemoveRows();
        metadataChanged = true;
    }
    m_directoryPath = directoryPath;
    if (metadataChanged) {
        ++m_generation;
        m_pending.clear();
    }
}

bool ThumbnailModel::hasSameEntries(
    const QList<Entry> &entries) const
{
    if (entries.size() != m_entries.size())
        return false;
    for (int row = 0; row < entries.size(); ++row) {
        if (!metadataEqual(entries.at(row), m_entries.at(row))) {
            return false;
        }
    }
    return true;
}

bool ThumbnailModel::metadataEqual(
    const Entry &left, const Entry &right)
{
    return left.path == right.path
        && left.name == right.name
        && left.directory == right.directory
        && left.size == right.size
        && left.modified == right.modified
        && left.suffix == right.suffix
        && left.sourceOrder == right.sourceOrder;
}

void ThumbnailModel::requestThumbnail(int row) const
{
    if (row < 0 || row >= m_entries.size())
        return;
    const Entry entry = m_entries.at(row);
    if (entry.path == m_primaryImagePath)
        return;
    const QString key = cacheKey(entry);
    if (m_pending.contains(key))
        return;
    m_pending.insert(key);
    const QSize requestedSize = m_thumbnailSize;
    const quint64 generation = m_generation;

    auto future = QtConcurrent::run(&m_decodePool,
                                    [path = entry.path, requestedSize] {
        const bool usePersistentCache =
            PersistentThumbnailCache::isEnabled();
        const QSize decodeSize = usePersistentCache
            ? PersistentThumbnailCache::bucketSize(requestedSize)
            : requestedSize;
        if (usePersistentCache) {
            QImage cached =
                PersistentThumbnailCache::load(path, decodeSize);
            if (!cached.isNull()) {
                return cached.scaled(
                    requestedSize, Qt::KeepAspectRatio,
                    Qt::SmoothTransformation);
            }
        }
        QImageReader reader(path);
        reader.setAutoTransform(true);
        const QSize sourceSize = reader.size();
        const qint64 estimatedDecodedBytes = sourceSize.isValid()
            ? static_cast<qint64>(sourceSize.width())
                * sourceSize.height() * 4
            : 0;
        QImage image;
        bool decodedWithVips = false;
        // PNG and several other Qt readers ignore ScaledSize and briefly
        // allocate the full source just to produce a filmstrip thumbnail.
        // Reuse the bounded large-image backend for those inputs as well.
        if (estimatedDecodedBytes >= 128LL * 1024 * 1024
            && VipsImageSource::supportsFile(path)) {
            const auto source = VipsImageSource::open(
                path, std::max(decodeSize.width(), decodeSize.height()));
            if (source) {
                image = source->preview();
                decodedWithVips = true;
            }
        }
        if (!decodedWithVips) {
            if (sourceSize.isValid()) {
                reader.setScaledSize(
                    sourceSize.scaled(
                        decodeSize, Qt::KeepAspectRatio));
            }
            image = reader.read();
        }
        if (!image.isNull()
            && (image.width() > decodeSize.width()
                || image.height() > decodeSize.height())) {
            image = image.scaled(decodeSize, Qt::KeepAspectRatio,
                                 Qt::SmoothTransformation);
        }
        if (usePersistentCache && !image.isNull()) {
            PersistentThumbnailCache::store(
                path, decodeSize, image);
        }
        return image.scaled(
            requestedSize, Qt::KeepAspectRatio,
            Qt::SmoothTransformation);
    });
    auto *self = const_cast<ThumbnailModel *>(this);
    auto *watcher = new QFutureWatcher<QImage>(self);
    QObject::connect(watcher, &QFutureWatcher<QImage>::finished,
                     self,
                     [self, watcher, key, entry, generation] {
        const QImage image = watcher->result();
        watcher->deleteLater();
        self->m_pending.remove(key);
        if (generation != self->m_generation || image.isNull())
            return;

        auto *pixmap = new QPixmap(QPixmap::fromImage(image));
        const int cost = std::max(1, static_cast<int>(image.sizeInBytes() / 1024));
        self->m_cache.insert(key, pixmap, cost);
        self->m_latestCache.insert(
            self->latestCacheKey(entry),
            new QPixmap(QPixmap::fromImage(image)), cost);
        for (int row = 0; row < self->m_entries.size(); ++row) {
            if (self->m_entries.at(row).path == entry.path) {
                emit self->dataChanged(self->index(row), self->index(row),
                                       {Qt::DecorationRole});
                break;
            }
        }
    });
    watcher->setFuture(future);
}
