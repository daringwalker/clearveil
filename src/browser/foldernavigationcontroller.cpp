#include "foldernavigationcontroller.h"

#include <QDir>
#include <QFileInfo>

FolderNavigationController::FolderNavigationController(
    QObject *parent)
    : QObject(parent)
{
}

void FolderNavigationController::recordVisit(
    const QString &directoryPath)
{
    const QString normalized = normalizedDirectory(directoryPath);
    if (normalized.isEmpty())
        return;

    if (normalized == m_pendingDirectory
        && m_pendingNavigation != PendingNavigation::None) {
        if (m_pendingNavigation == PendingNavigation::Back) {
            if (!m_currentDirectory.isEmpty()) {
                appendBounded(
                    m_forwardHistory, m_currentDirectory,
                    maximumHistoryEntries);
            }
            if (!m_backHistory.isEmpty())
                m_backHistory.removeLast();
        } else {
            if (!m_currentDirectory.isEmpty()) {
                appendBounded(
                    m_backHistory, m_currentDirectory,
                    maximumHistoryEntries);
            }
            if (!m_forwardHistory.isEmpty())
                m_forwardHistory.removeLast();
        }
        m_currentDirectory = normalized;
        m_pendingDirectory.clear();
        m_pendingNavigation = PendingNavigation::None;
    } else if (normalized != m_currentDirectory) {
        if (!m_currentDirectory.isEmpty()) {
            appendBounded(
                m_backHistory, m_currentDirectory,
                maximumHistoryEntries);
        }
        m_currentDirectory = normalized;
        m_forwardHistory.clear();
        m_pendingDirectory.clear();
        m_pendingNavigation = PendingNavigation::None;
    }
    touchRecent(normalized);
    emit stateChanged();
}

void FolderNavigationController::goBack()
{
    if (!canGoBack())
        return;
    m_pendingDirectory = m_backHistory.constLast();
    m_pendingNavigation = PendingNavigation::Back;
    emit stateChanged();
    emit directoryRequested(m_pendingDirectory);
}

void FolderNavigationController::goForward()
{
    if (!canGoForward())
        return;
    m_pendingDirectory = m_forwardHistory.constLast();
    m_pendingNavigation = PendingNavigation::Forward;
    emit stateChanged();
    emit directoryRequested(m_pendingDirectory);
}

void FolderNavigationController::navigationFailed(
    const QString &directoryPath)
{
    if (normalizedDirectory(directoryPath) != m_pendingDirectory)
        return;
    m_pendingDirectory.clear();
    m_pendingNavigation = PendingNavigation::None;
    emit stateChanged();
}

void FolderNavigationController::toggleFavorite(
    const QString &directoryPath)
{
    const QString normalized = normalizedDirectory(directoryPath);
    if (normalized.isEmpty())
        return;
    const int existing = m_favoriteDirectories.indexOf(normalized);
    if (existing >= 0) {
        m_favoriteDirectories.removeAt(existing);
    } else {
        appendBounded(
            m_favoriteDirectories, normalized,
            maximumFavoriteDirectories);
    }
    emit stateChanged();
}

void FolderNavigationController::clearRecentDirectories()
{
    if (m_recentDirectories.isEmpty())
        return;
    m_recentDirectories.clear();
    emit stateChanged();
}

void FolderNavigationController::setStoredLocations(
    const QStringList &recentDirectories,
    const QStringList &favoriteDirectories)
{
    m_recentDirectories = normalizedLocations(
        recentDirectories, maximumRecentDirectories);
    m_favoriteDirectories = normalizedLocations(
        favoriteDirectories, maximumFavoriteDirectories);
    emit stateChanged();
}

QString FolderNavigationController::currentDirectory() const
{
    return m_currentDirectory;
}

QStringList FolderNavigationController::recentDirectories() const
{
    return m_recentDirectories;
}

QStringList FolderNavigationController::favoriteDirectories() const
{
    return m_favoriteDirectories;
}

bool FolderNavigationController::canGoBack() const
{
    return m_pendingNavigation == PendingNavigation::None
        && !m_backHistory.isEmpty();
}

bool FolderNavigationController::canGoForward() const
{
    return m_pendingNavigation == PendingNavigation::None
        && !m_forwardHistory.isEmpty();
}

bool FolderNavigationController::isFavorite(
    const QString &directoryPath) const
{
    return m_favoriteDirectories.contains(
        normalizedDirectory(directoryPath));
}

QString FolderNavigationController::normalizedDirectory(
    const QString &directoryPath)
{
    if (directoryPath.trimmed().isEmpty())
        return {};
    return QDir::cleanPath(
        QFileInfo(directoryPath).absoluteFilePath());
}

QStringList FolderNavigationController::normalizedLocations(
    const QStringList &locations, int maximumCount)
{
    QStringList normalized;
    for (const QString &location : locations) {
        const QString directory = normalizedDirectory(location);
        if (directory.isEmpty() || normalized.contains(directory))
            continue;
        normalized.append(directory);
        if (normalized.size() >= maximumCount)
            break;
    }
    return normalized;
}

void FolderNavigationController::touchRecent(
    const QString &directoryPath)
{
    m_recentDirectories.removeAll(directoryPath);
    m_recentDirectories.prepend(directoryPath);
    while (m_recentDirectories.size()
           > maximumRecentDirectories) {
        m_recentDirectories.removeLast();
    }
}

void FolderNavigationController::appendBounded(
    QStringList &entries, const QString &directoryPath,
    int maximumCount)
{
    if (directoryPath.isEmpty())
        return;
    entries.removeAll(directoryPath);
    entries.append(directoryPath);
    while (entries.size() > maximumCount)
        entries.removeFirst();
}
