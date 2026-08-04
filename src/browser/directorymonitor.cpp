#include "directorymonitor.h"

#include <QDir>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QTimer>

DirectoryMonitor::DirectoryMonitor(QObject *parent)
    : QObject(parent)
    , m_watcher(new QFileSystemWatcher(this))
    , m_debounceTimer(new QTimer(this))
{
    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(180);
    connect(m_watcher, &QFileSystemWatcher::directoryChanged,
            this, [this](const QString &path) {
        if (QFileInfo(path).absoluteFilePath() == m_directory)
            m_debounceTimer->start();
    });
    connect(m_debounceTimer, &QTimer::timeout,
            this, [this] {
        restoreWatch();
        if (!m_directory.isEmpty())
            emit refreshRequested(m_directory);
    });
}

void DirectoryMonitor::setDirectory(
    const QString &directoryPath)
{
    const QFileInfo info(directoryPath);
    const QString normalized = info.isDir()
        ? info.absoluteFilePath() : QString();
    if (normalized == m_directory)
        return;
    m_debounceTimer->stop();
    const QStringList watched = m_watcher->directories();
    if (!watched.isEmpty())
        m_watcher->removePaths(watched);
    m_directory = normalized;
    restoreWatch();
}

QString DirectoryMonitor::directory() const
{
    return m_directory;
}

void DirectoryMonitor::restoreWatch()
{
    if (m_directory.isEmpty()
        || !QFileInfo(m_directory).isDir()
        || m_watcher->directories().contains(m_directory)) {
        return;
    }
    m_watcher->addPath(m_directory);
}
