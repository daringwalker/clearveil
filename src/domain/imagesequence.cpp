#include "imagesequence.h"

#include "formatcapabilities.h"

#include <QDir>
#include <QFileInfo>
#include <QSet>

namespace {
QSet<QString> supportedSuffixes()
{
    return FormatCapabilities::readableExtensions();
}

int naturalCompare(QStringView left, QStringView right)
{
    qsizetype leftIndex = 0;
    qsizetype rightIndex = 0;
    while (leftIndex < left.size() && rightIndex < right.size()) {
        const bool leftDigit = left.at(leftIndex).isDigit();
        const bool rightDigit = right.at(rightIndex).isDigit();
        if (leftDigit && rightDigit) {
            const qsizetype leftRunStart = leftIndex;
            const qsizetype rightRunStart = rightIndex;
            while (leftIndex < left.size() && left.at(leftIndex).isDigit())
                ++leftIndex;
            while (rightIndex < right.size() && right.at(rightIndex).isDigit())
                ++rightIndex;

            qsizetype leftSignificant = leftRunStart;
            qsizetype rightSignificant = rightRunStart;
            while (leftSignificant + 1 < leftIndex
                   && left.at(leftSignificant) == u'0')
                ++leftSignificant;
            while (rightSignificant + 1 < rightIndex
                   && right.at(rightSignificant) == u'0')
                ++rightSignificant;

            const qsizetype leftLength = leftIndex - leftSignificant;
            const qsizetype rightLength = rightIndex - rightSignificant;
            if (leftLength != rightLength)
                return leftLength < rightLength ? -1 : 1;
            const int numberOrder = QStringView(left).mid(leftSignificant, leftLength)
                .compare(QStringView(right).mid(rightSignificant, rightLength));
            if (numberOrder != 0)
                return numberOrder;
            continue;
        }

        const QString leftCharacter = QString(left.at(leftIndex)).toCaseFolded();
        const QString rightCharacter = QString(right.at(rightIndex)).toCaseFolded();
        const int characterOrder = QStringView(leftCharacter).compare(rightCharacter);
        if (characterOrder != 0)
            return characterOrder;
        ++leftIndex;
        ++rightIndex;
    }
    if (leftIndex == left.size() && rightIndex == right.size())
        return 0;
    return leftIndex == left.size() ? -1 : 1;
}
}

void ImageSequence::loadAround(const QString &filePath)
{
    const QFileInfo fileInfo(filePath);
    loadDirectory(fileInfo.absolutePath());
    const QString absoluteFile = fileInfo.absoluteFilePath();
    if (!m_files.contains(absoluteFile) && fileInfo.isFile())
        m_files.append(absoluteFile);
}

void ImageSequence::loadDirectory(const QString &directoryPath)
{
    const QDir directory(directoryPath);
    m_files = sortedImageFiles(
        directory.entryInfoList(QDir::Files | QDir::Readable | QDir::NoDotAndDotDot));
}

void ImageSequence::loadFiles(const QStringList &filePaths)
{
    m_files.clear();
    appendFiles(filePaths);
}

void ImageSequence::loadValidatedFiles(
    const QStringList &filePaths)
{
    m_files.clear();
    m_files.reserve(filePaths.size());
    QSet<QString> seen;
    for (const QString &path : filePaths) {
        const QString absolutePath =
            QFileInfo(path).absoluteFilePath();
        if (absolutePath.isEmpty()
            || seen.contains(absolutePath)) {
            continue;
        }
        m_files.append(absolutePath);
        seen.insert(absolutePath);
    }
}

void ImageSequence::appendFiles(const QStringList &filePaths)
{
    QSet<QString> seen;
    for (const QString &path : std::as_const(m_files))
        seen.insert(QFileInfo(path).absoluteFilePath());
    for (const QString &path : filePaths) {
        const QFileInfo info(path);
        const QString absolutePath = info.absoluteFilePath();
        if (isSupportedImage(info) && !seen.contains(absolutePath)) {
            m_files.append(absolutePath);
            seen.insert(absolutePath);
        }
    }
}

bool ImageSequence::replaceFile(const QString &oldPath, const QString &newPath)
{
    const QString absoluteOldPath = QFileInfo(oldPath).absoluteFilePath();
    const QFileInfo newInfo(newPath);
    const QString absoluteNewPath = newInfo.absoluteFilePath();
    const int oldIndex = m_files.indexOf(absoluteOldPath);
    if (oldIndex < 0 || !isSupportedImage(newInfo))
        return false;

    const int duplicateIndex = m_files.indexOf(absoluteNewPath);
    if (duplicateIndex >= 0 && duplicateIndex != oldIndex) {
        m_files.removeAt(oldIndex);
        return true;
    }
    m_files[oldIndex] = absoluteNewPath;
    return true;
}

const QStringList &ImageSequence::files() const
{
    return m_files;
}

int ImageSequence::indexOf(const QString &filePath) const
{
    return m_files.indexOf(QFileInfo(filePath).absoluteFilePath());
}

QString ImageSequence::at(int index) const
{
    return index >= 0 && index < m_files.size() ? m_files.at(index) : QString();
}

int ImageSequence::size() const
{
    return m_files.size();
}

bool ImageSequence::isEmpty() const
{
    return m_files.isEmpty();
}

bool ImageSequence::isSupportedImage(const QFileInfo &info)
{
    static const QSet<QString> suffixes = supportedSuffixes();
    return info.isFile() && info.isReadable()
        && suffixes.contains(info.suffix().toLower());
}

bool ImageSequence::naturalLess(const QString &left, const QString &right)
{
    return naturalCompare(left, right) < 0;
}

QStringList ImageSequence::sortedImageFiles(const QFileInfoList &entries)
{
    QFileInfoList filtered;
    for (const QFileInfo &entry : entries) {
        if (isSupportedImage(entry))
            filtered.append(entry);
    }

    std::sort(filtered.begin(), filtered.end(), [](const QFileInfo &left,
                                                   const QFileInfo &right) {
        const int nameOrder = naturalCompare(left.fileName(), right.fileName());
        if (nameOrder != 0)
            return naturalLess(left.fileName(), right.fileName());
        return left.absoluteFilePath() < right.absoluteFilePath();
    });

    QStringList result;
    result.reserve(filtered.size());
    for (const QFileInfo &entry : std::as_const(filtered))
        result.append(entry.absoluteFilePath());
    return result;
}
