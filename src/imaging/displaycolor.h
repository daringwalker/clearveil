#pragma once

#include <QColorSpace>
#include <QImage>
#include <QString>

#include <stop_token>

enum class DisplayColorTargetSource
{
    CompositorSrgb,
    ColordProfile,
    SrgbFallback,
};

struct DisplayColorTarget
{
    QColorSpace colorSpace{QColorSpace::SRgb};
    DisplayColorTargetSource source =
        DisplayColorTargetSource::SrgbFallback;
    QString outputName;
    QString profilePath;

    [[nodiscard]] bool isValid() const
    {
        return colorSpace.isValid();
    }
};

enum class DisplayColorTransformError
{
    None,
    EmptyImage,
    Cancelled,
    ConversionFailed,
};

struct DisplayColorTransformResult
{
    QImage image;
    DisplayColorTransformError error =
        DisplayColorTransformError::None;
    qint64 elapsedNanoseconds = 0;
    bool assumedSrgb = false;
    bool converted = false;

    [[nodiscard]] bool succeeded() const
    {
        return !image.isNull()
            && error == DisplayColorTransformError::None;
    }
};

class DisplayColor final
{
public:
    [[nodiscard]] static DisplayColorTarget resolveAutomaticTarget(
        const QString &outputName, const QString &platformName);

    [[nodiscard]] static DisplayColorTransformResult transform(
        const QImage &source, const QColorSpace &targetColorSpace,
        std::stop_token stopToken = {});
};
