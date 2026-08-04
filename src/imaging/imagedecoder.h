#pragma once

#include <QByteArray>
#include <QCoreApplication>
#include <QImage>
#include <QSize>
#include <QString>

#include <memory>
#include <stop_token>

class ImageSource;

enum class ImageDecodeError
{
    None,
    Cancelled,
    TooLarge,
    ReadFailed,
    ColorConversionFailed,
};

struct ImageDecodeMetrics
{
    QSize sourceSize;
    QSize decodedSize;
    QByteArray format;
    qsizetype decodedBytes = 0;
    qint64 elapsedNanoseconds = 0;
    bool sourceColorSpacePresent = false;
    bool convertedToSrgb = false;
};

struct ImageLoadResult
{
    QString filePath;
    QImage image;
    QString error;
    ImageDecodeError errorCode = ImageDecodeError::None;
    ImageDecodeMetrics metrics;
    std::shared_ptr<ImageSource> source;

    [[nodiscard]] bool succeeded() const
    {
        return !image.isNull()
            && errorCode == ImageDecodeError::None;
    }
};

Q_DECLARE_METATYPE(ImageLoadResult)

struct ImageDecodeOptions
{
    qint64 maximumPixels = 400'000'000;
    bool normalizeToSrgb = true;
    bool preferRegionBackedLargeImages = true;
    qint64 regionBackedThresholdBytes = 128LL * 1024 * 1024;
    int largeImagePreviewMaximumDimension = 2048;
};

class ImageDecoder final
{
    Q_DECLARE_TR_FUNCTIONS(ImageDecoder)

public:
    static void configureProcessImageIo();

    [[nodiscard]] static ImageLoadResult decode(
        const QString &filePath,
        const ImageDecodeOptions &options = {},
        std::stop_token stopToken = {});
};
