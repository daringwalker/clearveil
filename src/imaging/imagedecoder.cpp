#include "imagedecoder.h"

#include "formatcapabilities.h"
#include "vipsimagesource.h"

#include <QColorSpace>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QImageReader>

#include <utility>

namespace {
constexpr int kImageReaderAllocationLimitMiB = 512;
}

void ImageDecoder::configureProcessImageIo()
{
    const int currentLimit = QImageReader::allocationLimit();
    if (currentLimit > 0
        && currentLimit < kImageReaderAllocationLimitMiB) {
        QImageReader::setAllocationLimit(
            kImageReaderAllocationLimitMiB);
    }
}

ImageLoadResult ImageDecoder::decode(
    const QString &filePath, const ImageDecodeOptions &options,
    std::stop_token stopToken)
{
    QElapsedTimer elapsed;
    elapsed.start();

    ImageLoadResult result;
    result.filePath = QFileInfo(filePath).absoluteFilePath();
    const auto finish = [&result, &elapsed](ImageDecodeError errorCode,
                                            const QString &error = {}) {
        result.errorCode = errorCode;
        result.error = error;
        result.metrics.elapsedNanoseconds = elapsed.nsecsElapsed();
        return result;
    };
    if (stopToken.stop_requested())
        return finish(ImageDecodeError::Cancelled);

    configureProcessImageIo();
    QImageReader reader(result.filePath);
    reader.setAutoTransform(true);
    result.metrics.sourceSize = reader.size();
    result.metrics.format = reader.format().toLower();
    if (stopToken.stop_requested())
        return finish(ImageDecodeError::Cancelled);

    const QSize sourceSize = result.metrics.sourceSize;
    if (sourceSize.isValid()
        && options.maximumPixels > 0
        && static_cast<qint64>(sourceSize.width())
               * sourceSize.height()
            > options.maximumPixels) {
        return finish(
            ImageDecodeError::TooLarge,
            tr("The image is too large to decode safely (%1 × %2 pixels).")
                .arg(sourceSize.width()).arg(sourceSize.height()));
    }

    const qint64 estimatedDecodedBytes = sourceSize.isValid()
        ? static_cast<qint64>(sourceSize.width())
            * sourceSize.height() * 4
        : 0;
    if (options.preferRegionBackedLargeImages
        && estimatedDecodedBytes >= options.regionBackedThresholdBytes
        && VipsImageSource::supportsFile(result.filePath)) {
        QString sourceError;
        auto source = VipsImageSource::open(
            result.filePath,
            options.largeImagePreviewMaximumDimension,
            stopToken, &sourceError);
        if (stopToken.stop_requested())
            return finish(ImageDecodeError::Cancelled);
        if (!source) {
            return finish(
                ImageDecodeError::ReadFailed,
                sourceError.isEmpty()
                    ? tr("The large image could not be decoded.")
                    : sourceError);
        }
        result.image = source->preview();
        result.source = std::move(source);
        result.metrics.sourceSize = result.source->logicalSize();
        result.metrics.decodedSize = result.image.size();
        result.metrics.decodedBytes = result.image.sizeInBytes();
        result.metrics.sourceColorSpacePresent =
            result.image.colorSpace().isValid();
        return finish(ImageDecodeError::None);
    }

    QImage decoded = reader.read();
    if (stopToken.stop_requested())
        return finish(ImageDecodeError::Cancelled);
    if (decoded.isNull()) {
        return finish(
            ImageDecodeError::ReadFailed,
            FormatCapabilities::friendlyDecodeError(
                result.filePath, reader.error(), reader.errorString()));
    }

    result.metrics.sourceColorSpacePresent =
        decoded.colorSpace().isValid();
    const QColorSpace srgb(QColorSpace::SRgb);
    if (options.normalizeToSrgb
        && decoded.colorSpace().isValid()
        && decoded.colorSpace() != srgb) {
        if (stopToken.stop_requested())
            return finish(ImageDecodeError::Cancelled);
        QImage converted = decoded.convertedToColorSpace(srgb);
        if (stopToken.stop_requested())
            return finish(ImageDecodeError::Cancelled);
        if (converted.isNull()) {
            return finish(
                ImageDecodeError::ColorConversionFailed,
                tr("The embedded color profile could not be converted to sRGB."));
        }
        decoded = std::move(converted);
        result.metrics.convertedToSrgb = true;
    }

    result.metrics.decodedSize = decoded.size();
    result.metrics.decodedBytes = decoded.sizeInBytes();
    result.image = std::move(decoded);
    return finish(ImageDecodeError::None);
}
