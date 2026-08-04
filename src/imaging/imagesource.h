#pragma once

#include <QColor>
#include <QImage>
#include <QRect>
#include <QSize>
#include <QString>

#include <memory>
#include <stop_token>

// A decoded image is not necessarily one contiguous QImage. Large still
// images are represented by a small preview plus regions decoded on demand.
// Keeping this interface independent from widgets also lets export and editing
// consume the same bounded-memory pixel source later.
class ImageSource
{
public:
    virtual ~ImageSource() = default;

    [[nodiscard]] virtual QSize logicalSize() const = 0;
    [[nodiscard]] virtual QImage preview() const = 0;
    [[nodiscard]] virtual QString filePath() const = 0;
    [[nodiscard]] virtual bool isRegionBacked() const = 0;

    // Drop resources that are expensive to keep while this source is not the
    // active document. The lightweight preview and source metadata remain
    // available, and implementations recreate random access lazily.
    virtual void releaseTransientResources() {}

    [[nodiscard]] virtual QImage readRegion(
        const QRect &sourceRect, const QSize &outputSize,
        std::stop_token stopToken = {},
        QString *errorMessage = nullptr) = 0;

    // Export without first materializing the complete image as a QImage.
    // Region-backed sources override this with their streaming encoder.
    [[nodiscard]] virtual bool writeToFile(
        const QString &targetPath,
        std::stop_token stopToken = {},
        QString *errorMessage = nullptr);

    [[nodiscard]] virtual QColor readPixel(
        const QPoint &position,
        std::stop_token stopToken = {},
        QString *errorMessage = nullptr);
};

using ImageSourcePtr = std::shared_ptr<ImageSource>;
