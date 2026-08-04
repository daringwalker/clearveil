#pragma once

#include <QAbstractListModel>
#include <QCache>
#include <QDateTime>
#include <QPixmap>
#include <QSet>
#include <QSize>
#include <QThreadPool>

#include "directoryscanservice.h"

class QMimeData;
class QImage;

class ThumbnailModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum class SortKey {
        SourceOrder,
        Name,
        ModifiedTime,
        FileSize,
        FileType
    };
    Q_ENUM(SortKey)

    enum Role {
        FilePathRole = Qt::UserRole + 1,
        IsDirectoryRole
    };

    explicit ThumbnailModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QStringList mimeTypes() const override;
    QMimeData *mimeData(
        const QModelIndexList &indexes) const override;
    Qt::DropActions supportedDragActions() const override;

    void setDirectory(const QString &directoryPath);
    void setDirectoryEntries(
        const QString &directoryPath,
        const QList<DirectoryScanEntry> &entries,
        bool includeDirectories = true);
    void setFiles(const QStringList &filePaths);
    void cacheThumbnail(const QString &filePath,
                        const QImage &sourceImage);
    // Avoid decoding the same large file in parallel with the main viewer.
    // cacheThumbnail() clears this automatically once the viewer preview is
    // available.
    void setPrimaryImagePath(const QString &filePath);
    void setThumbnailSize(const QSize &size);
    void setSort(SortKey key, Qt::SortOrder order);
    void refreshTheme();
    [[nodiscard]] QString directoryPath() const;
    [[nodiscard]] QString filePath(const QModelIndex &index) const;
    [[nodiscard]] bool isDirectory(const QModelIndex &index) const;
    [[nodiscard]] QSize thumbnailSize() const;
    [[nodiscard]] SortKey sortKey() const;
    [[nodiscard]] Qt::SortOrder sortOrder() const;

private:
    struct Entry {
        QString path;
        QString name;
        bool directory = false;
        qint64 size = 0;
        QDateTime modified;
        QString suffix;
        int sourceOrder = 0;
    };

    QString cacheKey(const Entry &entry) const;
    QString latestCacheKey(const Entry &entry) const;
    void requestThumbnail(int row) const;
    void sortEntries(QList<Entry> &entries) const;
    void applyEntries(QList<Entry> entries,
                      const QString &directoryPath);
    [[nodiscard]] bool hasSameEntries(
        const QList<Entry> &entries) const;
    [[nodiscard]] static bool metadataEqual(
        const Entry &left, const Entry &right);

    QList<Entry> m_entries;
    QString m_directoryPath;
    QString m_primaryImagePath;
    QSize m_thumbnailSize{128, 96};
    mutable QCache<QString, QPixmap> m_cache;
    mutable QCache<QString, QPixmap> m_latestCache;
    mutable QSet<QString> m_pending;
    mutable QThreadPool m_decodePool;
    quint64 m_generation = 0;
    SortKey m_sortKey = SortKey::SourceOrder;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;
};
