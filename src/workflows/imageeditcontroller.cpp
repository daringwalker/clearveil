#include "imageeditcontroller.h"

#include "framecontroller.h"
#include "imagedocument.h"

#include <cmath>

ImageEditController::ImageEditController(
    ImageDocument *document, FrameController *frames,
    QObject *parent)
    : QObject(parent)
    , m_document(document)
    , m_frames(frames)
{
    setObjectName(QStringLiteral("imageEditController"));
    Q_ASSERT(m_document);
    Q_ASSERT(m_frames);
}

bool ImageEditController::canEdit() const
{
    return editAvailabilityError() == Error::None;
}

bool ImageEditController::canUndo() const
{
    return m_document && m_document->canUndo();
}

bool ImageEditController::canRedo() const
{
    return m_document && m_document->canRedo();
}

ImageEditController::Result
ImageEditController::rotateClockwise()
{
    const QSize before = m_document->image().size();
    const Error error = editAvailabilityError();
    if (error != Error::None)
        return finish(Command::RotateClockwise, error, before, false);
    const bool changed = m_document->rotateClockwise();
    return finish(Command::RotateClockwise,
                  changed ? Error::None : Error::NoChange,
                  before, changed);
}

ImageEditController::Result
ImageEditController::rotateCounterClockwise()
{
    const QSize before = m_document->image().size();
    const Error error = editAvailabilityError();
    if (error != Error::None) {
        return finish(Command::RotateCounterClockwise,
                      error, before, false);
    }
    const bool changed = m_document->rotateCounterClockwise();
    return finish(Command::RotateCounterClockwise,
                  changed ? Error::None : Error::NoChange,
                  before, changed);
}

ImageEditController::Result ImageEditController::flipHorizontal()
{
    const QSize before = m_document->image().size();
    const Error error = editAvailabilityError();
    if (error != Error::None)
        return finish(Command::FlipHorizontal, error, before, false);
    const bool changed = m_document->flipHorizontal();
    return finish(Command::FlipHorizontal,
                  changed ? Error::None : Error::NoChange,
                  before, changed);
}

ImageEditController::Result ImageEditController::flipVertical()
{
    const QSize before = m_document->image().size();
    const Error error = editAvailabilityError();
    if (error != Error::None)
        return finish(Command::FlipVertical, error, before, false);
    const bool changed = m_document->flipVertical();
    return finish(Command::FlipVertical,
                  changed ? Error::None : Error::NoChange,
                  before, changed);
}

ImageEditController::Result ImageEditController::crop(
    const QRect &rectangle)
{
    const QSize before = m_document->image().size();
    const Error availability = editAvailabilityError();
    if (availability != Error::None)
        return finish(Command::Crop, availability, before, false);
    const QRect valid = rectangle.normalized().intersected(
        m_document->image().rect());
    if (!valid.isValid()) {
        return finish(Command::Crop, Error::InvalidArgument,
                      before, false);
    }
    if (valid == m_document->image().rect())
        return finish(Command::Crop, Error::NoChange, before, false);
    const bool changed = m_document->crop(valid);
    return finish(Command::Crop,
                  changed ? Error::None : Error::NoChange,
                  before, changed);
}

ImageEditController::Result ImageEditController::resize(
    const QSize &size)
{
    const QSize before = m_document->image().size();
    const Error availability = editAvailabilityError();
    if (availability != Error::None)
        return finish(Command::Resize, availability, before, false);
    if (!size.isValid()) {
        return finish(Command::Resize, Error::InvalidArgument,
                      before, false);
    }
    if (size == before)
        return finish(Command::Resize, Error::NoChange, before, false);
    const bool changed = m_document->resizeImage(size);
    return finish(Command::Resize,
                  changed ? Error::None : Error::NoChange,
                  before, changed);
}

ImageEditController::Result ImageEditController::adjustColors(
    int brightness, int contrast, qreal gamma)
{
    const QSize before = m_document->image().size();
    const Error availability = editAvailabilityError();
    if (availability != Error::None) {
        return finish(Command::AdjustColors, availability,
                      before, false);
    }
    if (brightness < -100 || brightness > 100
        || contrast < -100 || contrast > 100
        || !std::isfinite(gamma)
        || gamma < 0.1 || gamma > 3.0) {
        return finish(Command::AdjustColors,
                      Error::InvalidArgument, before, false);
    }
    if (brightness == 0 && contrast == 0
        && qFuzzyCompare(gamma, 1.0)) {
        return finish(Command::AdjustColors,
                      Error::NoChange, before, false);
    }
    const bool changed = m_document->adjustImage(
        brightness, contrast, gamma);
    return finish(Command::AdjustColors,
                  changed ? Error::None : Error::NoChange,
                  before, changed);
}

ImageEditController::Result ImageEditController::reduceRedEye(
    const QRect &rectangle)
{
    const QSize before = m_document->image().size();
    const Error availability = editAvailabilityError();
    if (availability != Error::None) {
        return finish(Command::ReduceRedEye, availability,
                      before, false);
    }
    const QRect valid = rectangle.normalized().intersected(
        m_document->image().rect());
    if (!valid.isValid()) {
        return finish(Command::ReduceRedEye,
                      Error::InvalidArgument, before, false);
    }
    const bool changed = m_document->reduceRedEye(valid);
    return finish(Command::ReduceRedEye,
                  changed ? Error::None : Error::NoChange,
                  before, changed);
}

ImageEditController::Result ImageEditController::undo()
{
    const QSize before = m_document->image().size();
    if (m_document->image().isNull())
        return finish(Command::Undo, Error::NoImage, before, false);
    if (!m_document->canUndo()) {
        return finish(Command::Undo, Error::NothingToUndo,
                      before, false);
    }
    const bool changed = m_document->undo();
    return finish(Command::Undo,
                  changed ? Error::None : Error::NothingToUndo,
                  before, changed);
}

ImageEditController::Result ImageEditController::redo()
{
    const QSize before = m_document->image().size();
    if (m_document->image().isNull())
        return finish(Command::Redo, Error::NoImage, before, false);
    if (!m_document->canRedo()) {
        return finish(Command::Redo, Error::NothingToRedo,
                      before, false);
    }
    const bool changed = m_document->redo();
    return finish(Command::Redo,
                  changed ? Error::None : Error::NothingToRedo,
                  before, changed);
}

ImageEditController::Error
ImageEditController::editAvailabilityError() const
{
    if (!m_document || m_document->image().isNull())
        return Error::NoImage;
    if (m_frames && m_frames->isActive())
        return Error::FrameSequenceActive;
    if (m_document->isRegionBacked())
        return Error::RegionBackedImage;
    return Error::None;
}

ImageEditController::Result ImageEditController::finish(
    Command command, Error error, const QSize &sizeBefore,
    bool changed)
{
    const Result result{
        command, error, sizeBefore,
        m_document ? m_document->image().size() : QSize(),
        changed};
    emit commandFinished(result);
    return result;
}
