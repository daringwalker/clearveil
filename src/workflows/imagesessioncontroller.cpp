#include "imagesessioncontroller.h"

#include <QDir>
#include <QFileInfo>

#include <algorithm>

namespace {
QString absolutePath(const QString &path)
{
    return path.isEmpty()
        ? QString() : QFileInfo(path).absoluteFilePath();
}
}

void ImageSessionController::appendOpenedFiles(
    const QStringList &paths)
{
    m_openedFiles.appendFiles(paths);
}

void ImageSessionController::clearOpenedFiles()
{
    m_openedFiles = {};
    m_currentOpenedIndex = -1;
    m_pendingOpenedIndex = -1;
}

bool ImageSessionController::replaceOpenedFile(
    const QString &oldPath, const QString &newPath)
{
    const QString currentPath = openedPathAt(m_currentOpenedIndex);
    if (!m_openedFiles.replaceFile(oldPath, newPath))
        return false;
    m_currentOpenedIndex = absolutePath(currentPath)
            == absolutePath(oldPath)
        ? m_openedFiles.indexOf(newPath)
        : m_openedFiles.indexOf(currentPath);
    if (m_pendingOpenedIndex >= m_openedFiles.size())
        m_pendingOpenedIndex = -1;
    return true;
}

void ImageSessionController::setLoadedPath(
    const QString &path, int requestedOpenedIndex)
{
    m_pendingOpenedIndex = -1;
    if (requestedOpenedIndex >= 0
        && absolutePath(m_openedFiles.at(requestedOpenedIndex))
            == absolutePath(path)) {
        m_currentOpenedIndex = requestedOpenedIndex;
        return;
    }
    m_currentOpenedIndex = m_openedFiles.indexOf(path);
}

ImageSessionController::RemovalResult
ImageSessionController::removeOpenedAt(
    int row, const QString &displayedPath)
{
    RemovalResult result;
    const QString removedPath = m_openedFiles.at(row);
    if (removedPath.isEmpty())
        return result;

    const QString normalizedDisplayed = absolutePath(displayedPath);
    result.removedDisplayedImage =
        absolutePath(removedPath) == normalizedDisplayed;
    QStringList remaining = m_openedFiles.files();
    remaining.removeAt(row);
    m_openedFiles.loadFiles(remaining);
    m_pendingOpenedIndex = -1;
    result.removed = true;
    result.openedImagesEmpty = m_openedFiles.isEmpty();
    if (result.openedImagesEmpty) {
        m_currentOpenedIndex = -1;
        return result;
    }

    if (!result.removedDisplayedImage) {
        m_currentOpenedIndex =
            m_openedFiles.indexOf(normalizedDisplayed);
        return result;
    }

    m_currentOpenedIndex = -1;
    result.nextIndex = std::min(row, m_openedFiles.size() - 1);
    result.nextPath = m_openedFiles.at(result.nextIndex);
    return result;
}

ImageSessionController::RemovalResult
ImageSessionController::removeOpenedPath(
    const QString &path, const QString &displayedPath)
{
    const int row = m_openedFiles.indexOf(path);
    if (row >= 0)
        return removeOpenedAt(row, displayedPath);
    RemovalResult result;
    result.openedImagesEmpty = m_openedFiles.isEmpty();
    if (!result.openedImagesEmpty) {
        result.nextIndex = m_currentOpenedIndex >= 0
            ? m_currentOpenedIndex : 0;
        result.nextPath = m_openedFiles.at(result.nextIndex);
    }
    return result;
}

const QStringList &ImageSessionController::openedFiles() const
{
    return m_openedFiles.files();
}

int ImageSessionController::openedCount() const
{
    return m_openedFiles.size();
}

bool ImageSessionController::openedFilesEmpty() const
{
    return m_openedFiles.isEmpty();
}

QString ImageSessionController::openedPathAt(int index) const
{
    return m_openedFiles.at(index);
}

int ImageSessionController::openedIndexOf(const QString &path) const
{
    return m_openedFiles.indexOf(path);
}

int ImageSessionController::currentOpenedIndex() const
{
    return m_currentOpenedIndex;
}

int ImageSessionController::effectiveOpenedIndex() const
{
    return m_pendingOpenedIndex >= 0
        ? m_pendingOpenedIndex : m_currentOpenedIndex;
}

void ImageSessionController::setPendingOpenedIndex(int index)
{
    m_pendingOpenedIndex = index >= 0 && index < m_openedFiles.size()
        ? index : -1;
}

void ImageSessionController::clearPendingOpenedIndex()
{
    m_pendingOpenedIndex = -1;
}

int ImageSessionController::pendingOpenedIndex() const
{
    return m_pendingOpenedIndex;
}

bool ImageSessionController::refreshDirectoryForFile(
    const QString &filePath, bool force)
{
    const QFileInfo file(filePath);
    const QString targetDirectory = file.isFile()
        ? file.absolutePath() : QString();
    if (targetDirectory == m_directoryPath && !force)
        return false;
    m_directoryPath = targetDirectory;
    if (m_directoryPath.isEmpty())
        m_directoryFiles = {};
    else
        m_directoryFiles.loadDirectory(m_directoryPath);
    return true;
}

bool ImageSessionController::loadDirectory(
    const QString &directoryPath)
{
    const QDir directory(directoryPath);
    if (!directory.exists())
        return false;

    ImageSequence files;
    files.loadDirectory(directory.absolutePath());
    if (files.isEmpty())
        return false;

    m_directoryPath = directory.absolutePath();
    m_directoryFiles = std::move(files);
    return true;
}

void ImageSessionController::setDirectoryFiles(
    const QString &directoryPath,
    const QStringList &filePaths)
{
    m_directoryPath = QDir(directoryPath).absolutePath();
    m_directoryFiles.loadValidatedFiles(filePaths);
}

void ImageSessionController::invalidateDirectory()
{
    m_directoryPath.clear();
    m_directoryFiles = {};
}

const QStringList &ImageSessionController::directoryFiles() const
{
    return m_directoryFiles.files();
}

int ImageSessionController::directoryCount() const
{
    return m_directoryFiles.size();
}

QString ImageSessionController::directoryPathAt(int index) const
{
    return m_directoryFiles.at(index);
}

int ImageSessionController::directoryIndexOf(const QString &path) const
{
    return m_directoryFiles.indexOf(path);
}

const QString &ImageSessionController::directoryPath() const
{
    return m_directoryPath;
}
