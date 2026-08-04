#pragma once

#include <QImage>
#include <QSize>
#include <QString>

class PersistentThumbnailCache
{
public:
    static constexpr qint64 defaultMaximumBytes =
        512LL * 1024LL * 1024LL;

    static void configure(bool enabled, qint64 maximumBytes,
                          const QString &directoryOverride = {});
    [[nodiscard]] static bool isEnabled();
    [[nodiscard]] static qint64 maximumBytes();
    [[nodiscard]] static QString directoryPath();
    [[nodiscard]] static qint64 sizeBytes();
    static bool clear();

    [[nodiscard]] static QSize bucketSize(
        const QSize &requestedSize);
    [[nodiscard]] static QImage load(
        const QString &sourcePath, const QSize &bucketSize);
    static void store(const QString &sourcePath,
                      const QSize &bucketSize,
                      const QImage &image);
};
