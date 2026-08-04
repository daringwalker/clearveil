#include "desktopintegration.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusUnixFileDescriptor>
#include <QFile>
#include <QPainter>
#include <QPrinter>
#include <QVariantMap>

DesktopIntegration::Result DesktopIntegration::printImage(
    QPrinter &printer, const QImage &image)
{
    if (image.isNull())
        return {Error::EmptyImage, {}};

    QPainter painter(&printer);
    if (!painter.isActive())
        return {Error::PrintFailed, {}};
    const QRect page = printer.pageLayout().paintRectPixels(
        printer.resolution());
    const QSize targetSize = image.size().scaled(
        page.size(), Qt::KeepAspectRatio);
    const QRect target(
        QPoint(page.x() + (page.width() - targetSize.width()) / 2,
               page.y() + (page.height() - targetSize.height()) / 2),
        targetSize);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.drawImage(target, image);
    if (!painter.end())
        return {Error::PrintFailed, {}};
    return {};
}

DesktopIntegration::Result
DesktopIntegration::requestWallpaper(
    const QString &imagePath)
{
    QFile imageFile(imagePath);
    if (!imageFile.open(QIODevice::ReadOnly)) {
        return {Error::ImageFileOpenFailed,
                imageFile.errorString()};
    }

    QDBusInterface portal(
        QStringLiteral("org.freedesktop.portal.Desktop"),
        QStringLiteral("/org/freedesktop/portal/desktop"),
        QStringLiteral("org.freedesktop.portal.Wallpaper"),
        QDBusConnection::sessionBus());
    if (!portal.isValid()) {
        return {Error::WallpaperServiceUnavailable,
                portal.lastError().message()};
    }

    QVariantMap options;
    options.insert(QStringLiteral("show-preview"), true);
    options.insert(QStringLiteral("set-on"),
                   QStringLiteral("background"));
    const QDBusMessage reply = portal.call(
        QStringLiteral("SetWallpaperFile"), QString(),
        QVariant::fromValue(
            QDBusUnixFileDescriptor(imageFile.handle())),
        options);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        return {Error::WallpaperRequestFailed,
                reply.errorMessage()};
    }
    return {};
}
