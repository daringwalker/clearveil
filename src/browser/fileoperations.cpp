#include "fileoperations.h"

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QUrl>

namespace {
FileOperations::Result failure(
    FileOperations::Error error,
    const QString &sourcePath = {},
    const QString &targetPath = {},
    const QString &detail = {})
{
    return {error, sourcePath, targetPath, detail};
}

QString absoluteFilePath(const QString &path)
{
    return path.isEmpty()
        ? QString() : QFileInfo(path).absoluteFilePath();
}
}

FileOperations::Result FileOperations::renameFile(
    const QString &sourcePath, const QString &newFileName)
{
    const QFileInfo source(sourcePath);
    const QString normalizedSource = source.absoluteFilePath();
    if (!source.exists() || !source.isFile())
        return failure(Error::SourceMissing, normalizedSource);

    const QString name = newFileName.trimmed();
    if (name.isEmpty() || name == QStringLiteral(".")
        || name == QStringLiteral("..")
        || name.contains(QLatin1Char('/'))) {
        return failure(Error::InvalidFileName, normalizedSource);
    }

    const QString target = source.dir().absoluteFilePath(name);
    if (target == normalizedSource)
        return failure(Error::NoChange, normalizedSource, target);
    if (QFileInfo::exists(target))
        return failure(Error::TargetExists, normalizedSource, target);

    QFile file(normalizedSource);
    if (!file.rename(target)) {
        return failure(Error::RenameFailed, normalizedSource,
                       target, file.errorString());
    }
    return {Error::None, normalizedSource,
            absoluteFilePath(target), {}};
}

FileOperations::Result FileOperations::validateTransfer(
    const QString &sourcePath,
    const QString &destinationDirectory)
{
    const QFileInfo source(sourcePath);
    const QString normalizedSource = source.absoluteFilePath();
    if (!source.exists() || !source.isFile())
        return failure(Error::SourceMissing, normalizedSource);

    const QFileInfo destination(destinationDirectory);
    if (!destination.exists() || !destination.isDir()) {
        return failure(Error::DestinationMissing,
                       normalizedSource,
                       destination.absoluteFilePath());
    }
    const QString target = QDir(destination.absoluteFilePath())
                               .absoluteFilePath(source.fileName());
    if (target == normalizedSource)
        return failure(Error::NoChange, normalizedSource, target);
    if (QFileInfo::exists(target))
        return failure(Error::TargetExists, normalizedSource, target);
    return {Error::None, normalizedSource, target, {}};
}

FileOperations::Result FileOperations::copyToDirectory(
    const QString &sourcePath,
    const QString &destinationDirectory)
{
    Result result = validateTransfer(
        sourcePath, destinationDirectory);
    if (!result.succeeded())
        return result;

    QFile source(result.sourcePath);
    if (!source.copy(result.targetPath)) {
        result.error = Error::CopyFailed;
        result.detail = source.errorString();
    }
    return result;
}

FileOperations::Result FileOperations::moveToDirectory(
    const QString &sourcePath,
    const QString &destinationDirectory)
{
    Result result = validateTransfer(
        sourcePath, destinationDirectory);
    if (!result.succeeded())
        return result;

    QFile source(result.sourcePath);
    if (source.rename(result.targetPath))
        return result;

    source.setFileName(result.sourcePath);
    if (!source.copy(result.targetPath)) {
        result.error = Error::MoveFailed;
        result.detail = source.errorString();
        return result;
    }
    source.setFileName(result.sourcePath);
    if (!source.remove()) {
        const QString detail = source.errorString();
        QFile::remove(result.targetPath);
        result.error = Error::MoveFailed;
        result.detail = detail;
    }
    return result;
}

FileOperations::Result FileOperations::moveToTrash(
    const QString &sourcePath)
{
    const QString normalizedSource = absoluteFilePath(sourcePath);
    const QFileInfo source(normalizedSource);
    if (!source.exists() || !source.isFile())
        return failure(Error::SourceMissing, normalizedSource);

    QString pathInTrash;
    if (!QFile::moveToTrash(normalizedSource, &pathInTrash))
        return failure(Error::TrashFailed, normalizedSource);
    return {Error::None, normalizedSource, pathInTrash, {}};
}

FileOperations::Result FileOperations::revealInFileManager(
    const QString &sourcePath)
{
    const QFileInfo source(sourcePath);
    const QString normalizedSource = source.absoluteFilePath();
    if (!source.exists())
        return failure(Error::SourceMissing, normalizedSource);
    const QString directory = source.isDir()
        ? normalizedSource : source.absolutePath();
    if (!QDesktopServices::openUrl(
            QUrl::fromLocalFile(directory))) {
        return failure(Error::RevealFailed,
                       normalizedSource, directory);
    }
    return {Error::None, normalizedSource, directory, {}};
}

FileOperations::Result FileOperations::launchApplication(
    const QString &applicationPath,
    const QString &sourcePath)
{
    const QFileInfo source(sourcePath);
    const QString normalizedSource = source.absoluteFilePath();
    if (!source.exists() || !source.isFile())
        return failure(Error::SourceMissing, normalizedSource);

    const QFileInfo application(applicationPath);
    if (!application.exists() || !application.isFile()
        || !application.isExecutable()) {
        return failure(Error::InvalidApplication,
                       normalizedSource,
                       application.absoluteFilePath());
    }
    if (!QProcess::startDetached(
            application.absoluteFilePath(),
            {normalizedSource})) {
        return failure(Error::LaunchFailed,
                       normalizedSource,
                       application.absoluteFilePath());
    }
    return {Error::None, normalizedSource,
            application.absoluteFilePath(), {}};
}
