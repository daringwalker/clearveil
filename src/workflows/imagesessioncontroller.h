#pragma once

#include "imagesequence.h"

#include <QString>
#include <QStringList>

class ImageSessionController
{
public:
    struct RemovalResult {
        bool removed = false;
        bool removedDisplayedImage = false;
        bool openedImagesEmpty = false;
        int nextIndex = -1;
        QString nextPath;
    };

    void appendOpenedFiles(const QStringList &paths);
    void clearOpenedFiles();
    bool replaceOpenedFile(const QString &oldPath,
                           const QString &newPath);
    void setLoadedPath(const QString &path,
                       int requestedOpenedIndex = -1);
    [[nodiscard]] RemovalResult removeOpenedAt(
        int row, const QString &displayedPath);
    [[nodiscard]] RemovalResult removeOpenedPath(
        const QString &path, const QString &displayedPath);

    [[nodiscard]] const QStringList &openedFiles() const;
    [[nodiscard]] int openedCount() const;
    [[nodiscard]] bool openedFilesEmpty() const;
    [[nodiscard]] QString openedPathAt(int index) const;
    [[nodiscard]] int openedIndexOf(const QString &path) const;
    [[nodiscard]] int currentOpenedIndex() const;
    [[nodiscard]] int effectiveOpenedIndex() const;

    void setPendingOpenedIndex(int index);
    void clearPendingOpenedIndex();
    [[nodiscard]] int pendingOpenedIndex() const;

    bool refreshDirectoryForFile(const QString &filePath,
                                 bool force = false);
    bool loadDirectory(const QString &directoryPath);
    void setDirectoryFiles(const QString &directoryPath,
                           const QStringList &filePaths);
    void invalidateDirectory();
    [[nodiscard]] const QStringList &directoryFiles() const;
    [[nodiscard]] int directoryCount() const;
    [[nodiscard]] QString directoryPathAt(int index) const;
    [[nodiscard]] int directoryIndexOf(const QString &path) const;
    [[nodiscard]] const QString &directoryPath() const;

private:
    ImageSequence m_openedFiles;
    ImageSequence m_directoryFiles;
    QString m_directoryPath;
    int m_currentOpenedIndex = -1;
    int m_pendingOpenedIndex = -1;
};
