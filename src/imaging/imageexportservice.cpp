#include "imageexportservice.h"

#include <QFileInfo>
#include <QImageWriter>
#include <QSaveFile>

#include <algorithm>

QByteArray ImageExportService::formatForPath(
    const QString &filePath)
{
    QByteArray format =
        QFileInfo(filePath).suffix().toLatin1().toLower();
    if (format == "jpg" || format == "jpe")
        return QByteArrayLiteral("jpeg");
    if (format == "tif")
        return QByteArrayLiteral("tiff");
    return format;
}

ImageExportService::Result
ImageExportService::writeAtomically(
    const QImage &image, const QString &filePath,
    int quality)
{
    Result result;
    result.filePath = filePath.isEmpty()
        ? QString() : QFileInfo(filePath).absoluteFilePath();
    result.format = formatForPath(filePath);
    if (image.isNull()) {
        result.error = Error::EmptyImage;
        return result;
    }
    if (filePath.trimmed().isEmpty()) {
        result.error = Error::EmptyTargetPath;
        return result;
    }

    QSaveFile output(result.filePath);
    if (!output.open(QIODevice::WriteOnly)) {
        result.error = Error::OpenFailed;
        result.detail = output.errorString();
        return result;
    }

    QImageWriter writer(&output, result.format);
    writer.setQuality(std::clamp(quality, 0, 100));
    if (!writer.write(image)) {
        result.error = Error::EncodeFailed;
        result.detail = writer.errorString();
        output.cancelWriting();
        return result;
    }
    if (!output.commit()) {
        result.error = Error::CommitFailed;
        result.detail = output.errorString();
        return result;
    }
    return result;
}
