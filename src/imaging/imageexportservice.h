#pragma once

#include <QByteArray>
#include <QImage>
#include <QString>

class ImageExportService final
{
public:
    enum class Error {
        None,
        EmptyImage,
        EmptyTargetPath,
        OpenFailed,
        EncodeFailed,
        CommitFailed
    };

    struct Result {
        Error error = Error::None;
        QString filePath;
        QByteArray format;
        QString detail;

        [[nodiscard]] bool succeeded() const
        {
            return error == Error::None;
        }
    };

    [[nodiscard]] static QByteArray formatForPath(
        const QString &filePath);
    [[nodiscard]] static Result writeAtomically(
        const QImage &image, const QString &filePath,
        int quality = 95);
};
