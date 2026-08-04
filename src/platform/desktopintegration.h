#pragma once

#include <QImage>
#include <QString>

class QPrinter;

class DesktopIntegration final
{
public:
    enum class Error {
        None,
        EmptyImage,
        PrintFailed,
        ImageFileOpenFailed,
        WallpaperServiceUnavailable,
        WallpaperRequestFailed
    };

    struct Result {
        Error error = Error::None;
        QString detail;

        [[nodiscard]] bool succeeded() const
        {
            return error == Error::None;
        }
    };

    [[nodiscard]] static Result printImage(
        QPrinter &printer, const QImage &image);
    [[nodiscard]] static Result requestWallpaper(
        const QString &imagePath);
};
