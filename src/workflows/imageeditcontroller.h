#pragma once

#include <QObject>
#include <QRect>
#include <QSize>

class FrameController;
class ImageDocument;

class ImageEditController final : public QObject
{
    Q_OBJECT

public:
    enum class Command {
        RotateClockwise,
        RotateCounterClockwise,
        FlipHorizontal,
        FlipVertical,
        Crop,
        Resize,
        AdjustColors,
        ReduceRedEye,
        Undo,
        Redo
    };
    Q_ENUM(Command)

    enum class Error {
        None,
        NoImage,
        FrameSequenceActive,
        RegionBackedImage,
        InvalidArgument,
        NoChange,
        NothingToUndo,
        NothingToRedo
    };
    Q_ENUM(Error)

    struct Result {
        Command command = Command::RotateClockwise;
        Error error = Error::None;
        QSize sizeBefore;
        QSize sizeAfter;
        bool changed = false;

        [[nodiscard]] bool succeeded() const
        {
            return error == Error::None;
        }
    };

    explicit ImageEditController(
        ImageDocument *document,
        FrameController *frames,
        QObject *parent = nullptr);

    [[nodiscard]] bool canEdit() const;
    [[nodiscard]] bool canUndo() const;
    [[nodiscard]] bool canRedo() const;

    Result rotateClockwise();
    Result rotateCounterClockwise();
    Result flipHorizontal();
    Result flipVertical();
    Result crop(const QRect &rectangle);
    Result resize(const QSize &size);
    Result adjustColors(int brightness, int contrast,
                        qreal gamma);
    Result reduceRedEye(const QRect &rectangle);
    Result undo();
    Result redo();

signals:
    void commandFinished(
        const ImageEditController::Result &result);

private:
    [[nodiscard]] Error editAvailabilityError() const;
    Result finish(Command command, Error error,
                  const QSize &sizeBefore, bool changed);

    ImageDocument *m_document = nullptr;
    FrameController *m_frames = nullptr;
};

Q_DECLARE_METATYPE(ImageEditController::Result)
