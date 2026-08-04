#pragma once

#include <QFileInfoList>
#include <QStringList>

class ImageSequence
{
public:
    void loadAround(const QString &filePath);
    void loadDirectory(const QString &directoryPath);
    void loadFiles(const QStringList &filePaths);
    void loadValidatedFiles(const QStringList &filePaths);
    void appendFiles(const QStringList &filePaths);
    bool replaceFile(const QString &oldPath, const QString &newPath);

    [[nodiscard]] const QStringList &files() const;
    [[nodiscard]] int indexOf(const QString &filePath) const;
    [[nodiscard]] QString at(int index) const;
    [[nodiscard]] int size() const;
    [[nodiscard]] bool isEmpty() const;

    static bool isSupportedImage(const QFileInfo &info);
    static bool naturalLess(const QString &left, const QString &right);
    static QStringList sortedImageFiles(const QFileInfoList &entries);

private:
    QStringList m_files;
};
