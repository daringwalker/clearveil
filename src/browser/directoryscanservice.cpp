#include "directoryscanservice.h"

#include "formatcapabilities.h"
#include "imagesequence.h"

#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QTimer>
#include <QtConcurrentRun>

#include <algorithm>

namespace {
DirectoryScanResult scanDirectory(
    const QString &directoryPath,
    const QSet<QString> &supportedSuffixes,
    const std::shared_ptr<std::atomic_bool> &cancelled)
{
    DirectoryScanResult result;
    QElapsedTimer timer;
    timer.start();

    const QDir directory(directoryPath);
    result.directoryPath = directory.absolutePath();
    if (!directory.exists()) {
        result.error = QObject::tr("The folder does not exist.");
        return result;
    }
    result.directoryModified =
        QFileInfo(result.directoryPath).lastModified();

    QDirIterator iterator(
        result.directoryPath,
        QDir::Dirs | QDir::Files | QDir::Readable
            | QDir::NoDotAndDotDot,
        QDirIterator::NoIteratorFlags);
    while (iterator.hasNext()) {
        if (cancelled->load(std::memory_order_relaxed)) {
            result.cancelled = true;
            result.entries.clear();
            return result;
        }
        iterator.next();
        const QFileInfo info = iterator.fileInfo();
        if (!info.isDir()
            && (!info.isFile()
                || !supportedSuffixes.contains(
                    info.suffix().toLower()))) {
            continue;
        }
        result.entries.append({
            info.absoluteFilePath(), info.fileName(), info.isDir(),
            info.size(), info.lastModified(),
            info.suffix().toLower()});
    }

    std::stable_sort(
        result.entries.begin(), result.entries.end(),
        [](const DirectoryScanEntry &left,
           const DirectoryScanEntry &right) {
            if (left.directory != right.directory)
                return left.directory;
            if (ImageSequence::naturalLess(left.name, right.name))
                return true;
            if (ImageSequence::naturalLess(right.name, left.name))
                return false;
            return left.path < right.path;
        });
    result.elapsedMs = timer.elapsed();
    return result;
}
}

bool DirectoryScanResult::succeeded() const
{
    return !cancelled && error.isEmpty()
        && !directoryPath.isEmpty();
}

QStringList DirectoryScanResult::imageFiles() const
{
    QStringList files;
    files.reserve(entries.size());
    for (const DirectoryScanEntry &entry : entries) {
        if (!entry.directory)
            files.append(entry.path);
    }
    return files;
}

int DirectoryScanResult::imageCount() const
{
    int count = 0;
    for (const DirectoryScanEntry &entry : entries) {
        if (!entry.directory)
            ++count;
    }
    return count;
}

DirectoryScanService::DirectoryScanService(QObject *parent)
    : QObject(parent)
    , m_supportedSuffixes(
          FormatCapabilities::readableExtensions())
{
    m_pool.setMaxThreadCount(2);
    m_pool.setExpiryTimeout(10'000);
}

DirectoryScanService::~DirectoryScanService()
{
    for (auto iterator = m_requests.begin();
         iterator != m_requests.end(); ++iterator) {
        iterator->cancelled->store(
            true, std::memory_order_relaxed);
    }
    m_pool.waitForDone();
}

quint64 DirectoryScanService::requestScan(
    const QString &directoryPath, bool forceRefresh)
{
    const QString normalized =
        QDir(directoryPath).absolutePath();
    const quint64 requestId = m_nextRequestId++;

    if (!forceRefresh) {
        DirectoryScanResult cached;
        if (cachedResult(normalized, &cached)) {
            QTimer::singleShot(0, this,
                [this, requestId, cached] {
                    emit scanFinished(requestId, cached);
                });
            return requestId;
        }
    }

    const auto cancelled =
        std::make_shared<std::atomic_bool>(false);
    m_requests.insert(requestId, {cancelled});
    auto future = QtConcurrent::run(
        &m_pool, scanDirectory, normalized,
        m_supportedSuffixes, cancelled);
    auto *watcher =
        new QFutureWatcher<DirectoryScanResult>(this);
    connect(watcher,
            &QFutureWatcher<DirectoryScanResult>::finished,
            this, [this, watcher, requestId] {
        const DirectoryScanResult result = watcher->result();
        watcher->deleteLater();
        m_requests.remove(requestId);
        if (result.succeeded())
            storeCache(result);
        if (!result.cancelled)
            emit scanFinished(requestId, result);
    });
    watcher->setFuture(future);
    return requestId;
}

void DirectoryScanService::cancel(quint64 requestId)
{
    const auto iterator = m_requests.find(requestId);
    if (iterator == m_requests.end())
        return;
    iterator->cancelled->store(true, std::memory_order_relaxed);
    m_requests.erase(iterator);
}

void DirectoryScanService::invalidate(
    const QString &directoryPath)
{
    const QString normalized =
        QDir(directoryPath).absolutePath();
    m_cache.remove(normalized);
    m_cacheOrder.removeAll(normalized);
}

bool DirectoryScanService::cachedResult(
    const QString &directoryPath,
    DirectoryScanResult *result) const
{
    const QString normalized =
        QDir(directoryPath).absolutePath();
    const auto iterator = m_cache.constFind(normalized);
    if (iterator == m_cache.cend()
        || !cacheIsFresh(iterator.value())) {
        return false;
    }
    if (result)
        *result = iterator.value();
    return true;
}

void DirectoryScanService::storeCache(
    const DirectoryScanResult &result)
{
    constexpr int maximumCachedDirectories = 16;
    m_cache.insert(result.directoryPath, result);
    m_cacheOrder.removeAll(result.directoryPath);
    m_cacheOrder.append(result.directoryPath);
    while (m_cacheOrder.size() > maximumCachedDirectories) {
        const QString oldest = m_cacheOrder.takeFirst();
        m_cache.remove(oldest);
    }
}

bool DirectoryScanService::cacheIsFresh(
    const DirectoryScanResult &result) const
{
    const QFileInfo directory(result.directoryPath);
    return directory.isDir()
        && directory.lastModified() == result.directoryModified;
}
