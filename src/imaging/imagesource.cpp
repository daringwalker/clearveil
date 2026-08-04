#include "imagesource.h"

bool ImageSource::writeToFile(
    const QString &, std::stop_token, QString *errorMessage)
{
    if (errorMessage)
        *errorMessage = QStringLiteral(
            "This image source does not support streaming export.");
    return false;
}

QColor ImageSource::readPixel(
    const QPoint &position, std::stop_token stopToken,
    QString *errorMessage)
{
    if (!QRect(QPoint(), logicalSize()).contains(position))
        return {};
    const QImage pixel = readRegion(
        QRect(position, QSize(1, 1)), QSize(1, 1),
        stopToken, errorMessage);
    return pixel.isNull() ? QColor() : pixel.pixelColor(0, 0);
}
