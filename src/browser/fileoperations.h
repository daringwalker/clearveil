#pragma once

#include <QString>

class FileOperations final
{
public:
    enum class Error {
        None,
        NoChange,
        SourceMissing,
        DestinationMissing,
        InvalidFileName,
        TargetExists,
        RenameFailed,
        CopyFailed,
        MoveFailed,
        TrashFailed,
        RevealFailed,
        InvalidApplication,
        LaunchFailed
    };

    struct Result {
        Error error = Error::None;
        QString sourcePath;
        QString targetPath;
        QString detail;

        [[nodiscard]] bool succeeded() const
        {
            return error == Error::None;
        }

        [[nodiscard]] bool isNoChange() const
        {
            return error == Error::NoChange;
        }
    };

    [[nodiscard]] static Result renameFile(
        const QString &sourcePath, const QString &newFileName);
    [[nodiscard]] static Result copyToDirectory(
        const QString &sourcePath,
        const QString &destinationDirectory);
    [[nodiscard]] static Result moveToDirectory(
        const QString &sourcePath,
        const QString &destinationDirectory);
    [[nodiscard]] static Result moveToTrash(
        const QString &sourcePath);
    [[nodiscard]] static Result revealInFileManager(
        const QString &sourcePath);
    [[nodiscard]] static Result launchApplication(
        const QString &applicationPath,
        const QString &sourcePath);

private:
    [[nodiscard]] static Result validateTransfer(
        const QString &sourcePath,
        const QString &destinationDirectory);
};
