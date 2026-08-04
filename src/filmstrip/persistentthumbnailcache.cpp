#include "persistentthumbnailcache.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QSaveFile>
#include <QStandardPaths>
#include <QThreadPool>
#include <QVector>

#include <algorithm>

namespace {
struct CacheState {
    QMutex mutex;
    bool enabled = false;
    qint64 maximumBytes =
        PersistentThumbnailCache::defaultMaximumBytes;
    QString directoryOverride;
    qint64 knownSize = -1;
};

CacheState &cacheState()
{
    static CacheState state;
    return state;
}

QString cacheDirectoryUnlocked(const CacheState &state)
{
    if (!state.directoryOverride.isEmpty())
        return state.directoryOverride;
    return QStandardPaths::writableLocation(
               QStandardPaths::CacheLocation)
        + QStringLiteral("/thumbnails/v1");
}

QString cacheFilePathUnlocked(const CacheState &state,
                              const QString &sourcePath,
                              const QSize &bucketSize)
{
    const QFileInfo source(sourcePath);
    const QByteArray fingerprint =
        source.absoluteFilePath().toUtf8()
        + '\n' + QByteArray::number(source.size())
        + '\n'
        + QByteArray::number(
            source.lastModified().toMSecsSinceEpoch())
        + '\n' + QByteArray::number(bucketSize.width())
        + 'x' + QByteArray::number(bucketSize.height());
    const QString digest = QString::fromLatin1(
        QCryptographicHash::hash(
            fingerprint, QCryptographicHash::Sha256)
            .toHex());
    return cacheDirectoryUnlocked(state)
        + QLatin1Char('/') + digest.left(2)
        + QLatin1Char('/') + digest + QStringLiteral(".png");
}

qint64 calculateSizeUnlocked(const QString &directory)
{
    qint64 total = 0;
    QDirIterator iterator(
        directory, {QStringLiteral("*.png")},
        QDir::Files, QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        iterator.next();
        total += iterator.fileInfo().size();
    }
    return total;
}

void trimUnlocked(CacheState &state)
{
    const QString directory =
        cacheDirectoryUnlocked(state);
    if (state.knownSize < 0)
        state.knownSize = calculateSizeUnlocked(directory);
    if (state.knownSize <= state.maximumBytes)
        return;

    QFileInfoList files;
    QDirIterator iterator(
        directory, {QStringLiteral("*.png")},
        QDir::Files, QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        iterator.next();
        files.append(iterator.fileInfo());
    }
    std::sort(files.begin(), files.end(),
              [](const QFileInfo &left,
                 const QFileInfo &right) {
        return left.lastModified()
            < right.lastModified();
    });
    for (const QFileInfo &file : std::as_const(files)) {
        if (state.knownSize <= state.maximumBytes)
            break;
        const qint64 bytes = file.size();
        if (QFile::remove(file.absoluteFilePath()))
            state.knownSize = std::max<qint64>(
                0, state.knownSize - bytes);
    }
}
}

void PersistentThumbnailCache::configure(
    bool enabled, qint64 maximumBytes,
    const QString &directoryOverride)
{
    CacheState &state = cacheState();
    {
        QMutexLocker locker(&state.mutex);
        const QString normalizedOverride =
            directoryOverride.isEmpty()
            ? QString() : QDir(directoryOverride).absolutePath();
        if (state.directoryOverride != normalizedOverride)
            state.knownSize = -1;
        state.directoryOverride = normalizedOverride;
        state.enabled = enabled;
        state.maximumBytes = std::max<qint64>(1, maximumBytes);
    }
    if (enabled) {
        QThreadPool::globalInstance()->start([] {
            CacheState &current = cacheState();
            QMutexLocker locker(&current.mutex);
            if (current.enabled)
                trimUnlocked(current);
        });
    }
}

bool PersistentThumbnailCache::isEnabled()
{
    CacheState &state = cacheState();
    QMutexLocker locker(&state.mutex);
    return state.enabled;
}

qint64 PersistentThumbnailCache::maximumBytes()
{
    CacheState &state = cacheState();
    QMutexLocker locker(&state.mutex);
    return state.maximumBytes;
}

QString PersistentThumbnailCache::directoryPath()
{
    CacheState &state = cacheState();
    QMutexLocker locker(&state.mutex);
    return cacheDirectoryUnlocked(state);
}

qint64 PersistentThumbnailCache::sizeBytes()
{
    CacheState &state = cacheState();
    QMutexLocker locker(&state.mutex);
    if (state.knownSize < 0) {
        state.knownSize = calculateSizeUnlocked(
            cacheDirectoryUnlocked(state));
    }
    return state.knownSize;
}

bool PersistentThumbnailCache::clear()
{
    CacheState &state = cacheState();
    QMutexLocker locker(&state.mutex);
    QDir directory(cacheDirectoryUnlocked(state));
    const bool cleared = !directory.exists()
        || directory.removeRecursively();
    if (cleared)
        state.knownSize = 0;
    return cleared;
}

QSize PersistentThumbnailCache::bucketSize(
    const QSize &requestedSize)
{
    const int extent = std::max(
        requestedSize.width(), requestedSize.height());
    for (const int bucket : {96, 160, 256, 384, 512, 768, 1024}) {
        if (extent <= bucket)
            return QSize(bucket, bucket);
    }
    return QSize(extent, extent);
}

QImage PersistentThumbnailCache::load(
    const QString &sourcePath, const QSize &bucketSize)
{
    CacheState &state = cacheState();
    QMutexLocker locker(&state.mutex);
    if (!state.enabled)
        return {};

    const QString path = cacheFilePathUnlocked(
        state, sourcePath, bucketSize);
    QImage image(path);
    if (image.isNull())
        return {};

    QFile cacheFile(path);
    if (cacheFile.open(QIODevice::ReadOnly)) {
        cacheFile.setFileTime(QDateTime::currentDateTime(),
                              QFileDevice::FileModificationTime);
    }
    return image;
}

void PersistentThumbnailCache::store(
    const QString &sourcePath, const QSize &bucketSize,
    const QImage &image)
{
    if (image.isNull())
        return;

    CacheState &state = cacheState();
    QMutexLocker locker(&state.mutex);
    if (!state.enabled)
        return;

    const QString path = cacheFilePathUnlocked(
        state, sourcePath, bucketSize);
    const QFileInfo previous(path);
    const qint64 oldBytes = previous.exists()
        ? previous.size() : 0;
    QDir().mkpath(previous.absolutePath());
    QSaveFile output(path);
    if (!output.open(QIODevice::WriteOnly))
        return;
    if (!image.save(&output, "PNG")
        || !output.commit()) {
        output.cancelWriting();
        return;
    }

    const qint64 newBytes = QFileInfo(path).size();
    if (state.knownSize < 0) {
        state.knownSize = calculateSizeUnlocked(
            cacheDirectoryUnlocked(state));
    } else {
        state.knownSize = std::max<qint64>(
            0, state.knownSize - oldBytes + newBytes);
    }
    trimUnlocked(state);
}
