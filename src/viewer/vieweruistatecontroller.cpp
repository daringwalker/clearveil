#include "vieweruistatecontroller.h"

#include <QAction>
#include <QLabel>
#include <QWidget>

#include <utility>

ViewerUiStateController::ViewerUiStateController(
    Actions actions, Targets targets, Text text,
    QObject *parent)
    : QObject(parent)
    , m_actions(std::move(actions))
    , m_targets(targets)
    , m_text(std::move(text))
{
}

void ViewerUiStateController::applyDocumentActions(
    const DocumentActionState &state)
{
    const bool usableImage = state.hasImage && !state.loading;
    const bool hasFile = usableImage && state.fileExists;
    setEnabled(m_actions.saveAs, usableImage);
    setEnabled(m_actions.copyImage,
               usableImage && state.fullRasterTransferAvailable);
    setEnabled(m_actions.trash, hasFile);
    setEnabled(m_actions.fileActions, hasFile);
    setEnabled(m_actions.print,
               usableImage && state.fullRasterTransferAvailable);
    setEnabled(m_actions.wallpaper, hasFile);
    setEnabled(m_actions.undo, state.canUndo);
    setEnabled(m_actions.redo, state.canRedo);
    setEnabled(m_actions.editActions,
               usableImage && state.canEdit);
    setEnabled(m_actions.viewActions, usableImage);
    setEnabled(m_actions.exportFrame,
               state.framesActive && state.hasImage);
    setEnabled(m_actions.metadata, usableImage);
    setEnabled(m_actions.colorPicker, usableImage);
    setEnabled(m_actions.textSelection, usableImage);
    setEnabled(m_actions.filmstripSource,
               usableImage && state.directoryImagesAvailable);
}

void ViewerUiStateController::applyNavigationActions(
    int index, int count)
{
    setEnabled(m_actions.previous, index > 0);
    setEnabled(m_actions.next,
               index >= 0 && index + 1 < count);
    setEnabled(m_actions.slideshow, count > 1);
}

void ViewerUiStateController::showReady()
{
    setLabelText(m_targets.fileLabel, m_text.ready);
    setLabelText(m_targets.detailLabel, QString());
    setLabelText(m_targets.zoomLabel, QString());
    if (m_targets.window)
        m_targets.window->setWindowTitle(m_text.applicationTitle);
}

void ViewerUiStateController::showCollection(
    const QString &name, int imageCount)
{
    setLabelText(m_targets.fileLabel, name);
    setLabelText(m_targets.detailLabel,
                 m_text.imageCount.arg(imageCount));
    setLabelText(m_targets.zoomLabel, QString());
    if (m_targets.window) {
        m_targets.window->setWindowTitle(
            m_text.titledApplication.arg(name));
    }
}

void ViewerUiStateController::showImage(
    const ImagePresentation &presentation)
{
    const QString name = presentation.fileName.isEmpty()
        ? m_text.clipboardImage : presentation.fileName;
    const QString modifiedPrefix = presentation.modified
        ? QStringLiteral("● ") : QString();
    setLabelText(m_targets.fileLabel, modifiedPrefix + name);
    setLabelText(
        m_targets.detailLabel,
        m_text.imageDetails
            .arg(presentation.imageSize.width())
            .arg(presentation.imageSize.height())
            .arg(presentation.fileExists
                     ? humanFileSize(presentation.fileSize)
                     : m_text.notSaved)
            .arg(presentation.position)
            .arg(presentation.count));
    setLabelText(
        m_targets.zoomLabel,
        QString::number(presentation.zoom * 100.0, 'f', 0)
            + QStringLiteral("%")
            + (presentation.zoomLocked
                   ? m_text.lockedSuffix : QString()));
    if (m_targets.window) {
        m_targets.window->setWindowTitle(
            m_text.modifiedTitle.arg(modifiedPrefix, name));
    }
}

void ViewerUiStateController::setLabelText(
    QLabel *label, const QString &text)
{
    if (!label)
        return;
    label->setText(text);
    label->setAccessibleDescription(text);
}

QString ViewerUiStateController::humanFileSize(qint64 bytes)
{
    constexpr qint64 kib = 1024;
    constexpr qint64 mib = kib * 1024;
    if (bytes >= mib) {
        return QString::number(
                   bytes / static_cast<double>(mib), 'f', 1)
            + QStringLiteral(" MiB");
    }
    if (bytes >= kib) {
        return QString::number(
                   bytes / static_cast<double>(kib), 'f', 1)
            + QStringLiteral(" KiB");
    }
    return QString::number(bytes) + QStringLiteral(" B");
}

void ViewerUiStateController::setEnabled(
    QAction *action, bool enabled)
{
    if (action)
        action->setEnabled(enabled);
}

void ViewerUiStateController::setEnabled(
    const QList<QAction *> &actions, bool enabled)
{
    for (QAction *action : actions)
        setEnabled(action, enabled);
}
