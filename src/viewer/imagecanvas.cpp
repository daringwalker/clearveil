#include "imagecanvas.h"
#include "largeimagesamplecontroller.h"
#include "ocrtextselectionmodel.h"
#include "tiledimageviewmodel.h"

#include <QDragEnterEvent>
#include <QFileInfo>
#include <QKeyEvent>
#include <QMimeData>
#include <QMouseEvent>
#include <QNativeGestureEvent>
#include <QPainter>
#include <QPainterPath>
#include <QTouchEvent>
#include <QUrl>
#include <QWheelEvent>

#include <cmath>

ImageCanvas::ImageCanvas(QWidget *parent)
    : QWidget(parent)
{
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setAttribute(Qt::WA_AcceptTouchEvents);
    setMinimumSize(240, 180);
    setAccessibleName(tr("Image canvas"));
    m_tiledImageViewModel = new TiledImageViewModel(this);
    m_largeImageSampleController =
        new LargeImageSampleController(this);
    m_ocrTextSelectionModel = new OcrTextSelectionModel(this);
    connect(m_ocrTextSelectionModel,
            &OcrTextSelectionModel::selectionChanged,
            this, [this](bool hasSelection) {
        emit ocrSelectionChanged(hasSelection);
        update();
    });
    connect(m_tiledImageViewModel,
            &TiledImageViewModel::tilesChanged,
            this, qOverload<>(&ImageCanvas::update));
    connect(m_largeImageSampleController,
            &LargeImageSampleController::sampleReady,
            this, [this](const QColor &color,
                         const QPoint &position,
                         const QImage &sample,
                         bool picked, bool adjusted) {
        if (adjusted) {
            emit colorSampleAdjusted(color, position, sample);
        } else {
            emit colorHovered(color, position, sample);
        }
        if (picked)
            emit colorPicked(color, position);
        update();
    });
}

bool ImageCanvas::event(QEvent *event)
{
    if (event->type() == QEvent::NativeGesture
        && handleNativeGesture(
            static_cast<QNativeGestureEvent *>(event))) {
        return true;
    }
    if ((event->type() == QEvent::TouchBegin
         || event->type() == QEvent::TouchUpdate
         || event->type() == QEvent::TouchEnd
         || event->type() == QEvent::TouchCancel)
        && handleTouchEvent(static_cast<QTouchEvent *>(event))) {
        return true;
    }
    return QWidget::event(event);
}

void ImageCanvas::setImage(const QImage &image, bool preserveView)
{
    setColorManagedImage(
        image, image, preserveView, image.size(), {});
}

void ImageCanvas::setColorManagedImage(
    const QImage &sourceImage, const QImage &displayImage,
    bool preserveView, const QSize &logicalSize,
    const ImageSourcePtr &imageSource)
{
    m_gestureController.endTouchGesture();
    const QSize nextLogicalSize = logicalSize.isValid()
        ? logicalSize : sourceImage.size();
    const bool imageContentChanged = m_image.cacheKey()
            != sourceImage.cacheKey()
        || m_logicalImageSize != nextLogicalSize;
    m_image = sourceImage;
    m_displayImage = displayImage.size() == sourceImage.size()
        ? displayImage : sourceImage;
    m_logicalImageSize = nextLogicalSize;
    m_imageSource = imageSource;
    m_tiledImageViewModel->setSource(imageSource);
    m_largeImageSampleController->setSource(imageSource);
    m_colorSamplePosition = {-1, -1};
    if (imageContentChanged)
        clearOcrResult();
    setColorSamplePinned(false);
    if (!preserveView) {
        m_pan = {};
        if (m_zoomMode == ZoomMode::Custom && !m_zoomLocked)
            m_zoomMode = ZoomMode::Fit;
    } else {
        clampPan();
    }
    update();
    emit zoomChanged(effectiveScale());
}

const QImage &ImageCanvas::sourceImage() const
{
    return m_image;
}

const QImage &ImageCanvas::displayImage() const
{
    return m_displayImage;
}

QSize ImageCanvas::logicalImageSize() const
{
    return m_logicalImageSize.isValid()
        ? m_logicalImageSize : m_image.size();
}

qreal ImageCanvas::zoom() const
{
    return effectiveScale();
}

ImageCanvas::ZoomMode ImageCanvas::zoomMode() const
{
    return m_zoomMode;
}

bool ImageCanvas::isZoomLocked() const
{
    return m_zoomLocked;
}

bool ImageCanvas::isColorSamplePinned() const
{
    return m_colorSamplePinned;
}

QPointF ImageCanvas::viewOffset() const
{
    return m_pan;
}

ImageCanvas::CanvasAppearance ImageCanvas::canvasAppearance() const
{
    return m_canvasAppearance;
}

void ImageCanvas::setCanvasAppearance(
    const CanvasAppearance &appearance)
{
    CanvasAppearance normalized = appearance;
    normalized.checkerboardTileSize = std::clamp(
        normalized.checkerboardTileSize, 4, 64);
    if (!normalized.checkerboardLight.isValid())
        normalized.checkerboardLight = QColor(210, 213, 217);
    if (!normalized.checkerboardDark.isValid())
        normalized.checkerboardDark = QColor(164, 168, 174);

    if (m_canvasAppearance.transparencyCheckerboardVisible
            == normalized.transparencyCheckerboardVisible
        && m_canvasAppearance.checkerboardLight
            == normalized.checkerboardLight
        && m_canvasAppearance.checkerboardDark
            == normalized.checkerboardDark
        && m_canvasAppearance.checkerboardTileSize
            == normalized.checkerboardTileSize) {
        return;
    }
    m_canvasAppearance = normalized;
    update();
}

void ImageCanvas::setTransparencyCheckerboardVisible(bool visible)
{
    CanvasAppearance appearance = m_canvasAppearance;
    appearance.transparencyCheckerboardVisible = visible;
    setCanvasAppearance(appearance);
}

void ImageCanvas::setColorPickerEnabled(bool enabled)
{
    m_colorPickerEnabled = enabled;
    setColorSamplePinned(false);
    updateInteractionCursor();
}

void ImageCanvas::setOcrTextSelectionEnabled(bool enabled)
{
    if (m_ocrTextSelectionEnabled == enabled)
        return;
    m_ocrTextSelectionEnabled = enabled;
    m_selectingOcrText = false;
    if (!enabled)
        m_ocrTextSelectionModel->clearSelection();
    updateInteractionCursor();
    update();
}

void ImageCanvas::setOcrDebugOverlayEnabled(bool enabled)
{
    if (m_ocrDebugOverlayEnabled == enabled)
        return;
    m_ocrDebugOverlayEnabled = enabled;
    update();
}

void ImageCanvas::setOcrResult(const OcrResult &result)
{
    m_ocrTextSelectionModel->setResult(result);
    updateInteractionCursor();
    update();
}

void ImageCanvas::clearOcrResult()
{
    if (!m_ocrTextSelectionModel)
        return;
    m_selectingOcrText = false;
    m_ocrTextSelectionModel->clear();
    updateInteractionCursor();
    update();
}

bool ImageCanvas::ocrTextSelectionEnabled() const
{
    return m_ocrTextSelectionEnabled;
}

bool ImageCanvas::ocrDebugOverlayEnabled() const
{
    return m_ocrDebugOverlayEnabled;
}

bool ImageCanvas::hasOcrText() const
{
    return m_ocrTextSelectionModel
        && m_ocrTextSelectionModel->hasText();
}

bool ImageCanvas::hasSelectedText() const
{
    return m_ocrTextSelectionModel
        && m_ocrTextSelectionModel->hasSelection();
}

QString ImageCanvas::selectedText() const
{
    return m_ocrTextSelectionModel
        ? m_ocrTextSelectionModel->selectedText() : QString{};
}

void ImageCanvas::setMouseActions(
    const QString &wheelAction,
    const QString &ctrlWheelAction,
    const QString &doubleClickAction,
    const QString &middleButtonAction,
    const QString &backButtonAction,
    const QString &forwardButtonAction)
{
    m_wheelAction = wheelAction;
    m_ctrlWheelAction = ctrlWheelAction;
    m_doubleClickAction = doubleClickAction;
    m_middleButtonAction = middleButtonAction;
    m_backButtonAction = backButtonAction;
    m_forwardButtonAction = forwardButtonAction;
    m_navigationWheelAccumulator = 0.0;
}

void ImageCanvas::fitToWindow()
{
    if (m_zoomLocked) {
        m_customZoom = fitScale();
        m_zoomMode = ZoomMode::Custom;
    } else {
        m_zoomMode = ZoomMode::Fit;
    }
    m_pan = {};
    update();
    emit zoomChanged(effectiveScale());
}

void ImageCanvas::fitToWidth()
{
    if (m_zoomLocked) {
        m_customZoom = widthScale();
        m_zoomMode = ZoomMode::Custom;
    } else {
        m_zoomMode = ZoomMode::FitWidth;
    }
    m_pan = {};
    update();
    emit zoomChanged(effectiveScale());
}

void ImageCanvas::fitToHeight()
{
    if (m_zoomLocked) {
        m_customZoom = heightScale();
        m_zoomMode = ZoomMode::Custom;
    } else {
        m_zoomMode = ZoomMode::FitHeight;
    }
    m_pan = {};
    update();
    emit zoomChanged(effectiveScale());
}

void ImageCanvas::fillWindow()
{
    if (m_zoomLocked) {
        m_customZoom =
            std::max(widthScale(), heightScale());
        m_zoomMode = ZoomMode::Custom;
    } else {
        m_zoomMode = ZoomMode::Fill;
    }
    m_pan = {};
    update();
    emit zoomChanged(effectiveScale());
}

void ImageCanvas::actualSize()
{
    if (m_zoomLocked) {
        m_customZoom = 1.0;
        m_zoomMode = ZoomMode::Custom;
    } else {
        m_zoomMode = ZoomMode::ActualSize;
    }
    m_pan = {};
    update();
    emit zoomChanged(1.0);
}

void ImageCanvas::zoomIn()
{
    setZoom(effectiveScale() * 1.15, rect().center());
}

void ImageCanvas::zoomOut()
{
    setZoom(effectiveScale() / 1.15, rect().center());
}

void ImageCanvas::setZoom(qreal zoom, const QPointF &anchor)
{
    if (m_image.isNull())
        return;

    const qreal oldScale = effectiveScale();
    const QPointF effectiveAnchor = anchor.isNull() ? QPointF(rect().center()) : anchor;
    const QPointF oldTopLeft = imageTopLeft(oldScale);
    const QPointF imagePoint = (effectiveAnchor - oldTopLeft) / oldScale;

    m_zoomMode = ZoomMode::Custom;
    m_customZoom = std::clamp(zoom, 0.01, 64.0);

    const QSizeF scaledSize = QSizeF(logicalImageSize()) * m_customZoom;
    const QPointF centered((width() - scaledSize.width()) / 2.0,
                           (height() - scaledSize.height()) / 2.0);
    m_pan = effectiveAnchor - centered - imagePoint * m_customZoom;
    clampPan();
    update();
    emit zoomChanged(m_customZoom);
}

void ImageCanvas::setZoomLocked(bool locked)
{
    if (m_zoomLocked == locked)
        return;
    const qreal currentScale = effectiveScale();
    m_zoomLocked = locked;
    if (locked) {
        m_customZoom = currentScale;
        m_zoomMode = ZoomMode::Custom;
        clampPan();
    }
    update();
    emit zoomChanged(effectiveScale());
}

void ImageCanvas::setColorSamplePinned(bool pinned)
{
    if (m_colorSamplePinned == pinned)
        return;
    m_colorSamplePinned = pinned;
    update();
    emit colorSamplePinnedChanged(pinned);
}

void ImageCanvas::pinColorSampleAt(
    const QPoint &imagePosition)
{
    if (!imageBounds().contains(imagePosition))
        return;
    m_colorSamplePosition = imagePosition;
    setColorSamplePinned(true);
    update();
}

void ImageCanvas::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), palette().color(QPalette::Window));

    if (m_image.isNull()) {
        painter.setPen(palette().color(QPalette::Disabled, QPalette::WindowText));
        QFont titleFont = font();
        titleFont.setPointSizeF(titleFont.pointSizeF() * 1.35);
        titleFont.setWeight(QFont::DemiBold);
        painter.setFont(titleFont);
        painter.drawText(rect().adjusted(24, 0, -24, -18),
                         Qt::AlignCenter, tr("Drop an image here"));
        painter.setFont(font());
        painter.drawText(rect().adjusted(24, 46, -24, 0),
                         Qt::AlignCenter, tr("or press Ctrl+O to open"));
        return;
    }

    const qreal scale = effectiveScale();
    const QRectF target(
        imageTopLeft(scale), QSizeF(logicalImageSize()) * scale);

    const QImage &paintImage = m_displayImage.isNull()
        ? m_image : m_displayImage;
    if (paintImage.hasAlphaChannel()
        && m_canvasAppearance.transparencyCheckerboardVisible) {
        const int tile = m_canvasAppearance.checkerboardTileSize;
        painter.save();
        painter.setClipRect(target);
        const int left = static_cast<int>(std::floor(target.left() / tile)) * tile;
        const int top = static_cast<int>(std::floor(target.top() / tile)) * tile;
        for (int y = top; y < target.bottom(); y += tile) {
            for (int x = left; x < target.right(); x += tile) {
                painter.fillRect(QRect(x, y, tile, tile),
                                 ((x / tile) + (y / tile)) % 2
                                     ? m_canvasAppearance.checkerboardLight
                                     : m_canvasAppearance.checkerboardDark);
            }
        }
        painter.restore();
    }

    painter.setRenderHint(QPainter::SmoothPixmapTransform, scale != 1.0);
    painter.drawImage(target, paintImage);

    if (m_imageSource && m_tiledImageViewModel) {
        const QRectF visibleTarget = target.intersected(QRectF(rect()));
        if (visibleTarget.isValid()) {
            const QRectF visibleSource(
                (visibleTarget.left() - target.left()) / scale,
                (visibleTarget.top() - target.top()) / scale,
                visibleTarget.width() / scale,
                visibleTarget.height() / scale);
            m_tiledImageViewModel->updateViewport(
                visibleSource, scale);
            for (const TiledImageViewModel::Tile &tile
                 : m_tiledImageViewModel->visibleTiles()) {
                const QRectF tileTarget(
                    target.left() + tile.sourceRect.x() * scale,
                    target.top() + tile.sourceRect.y() * scale,
                    tile.sourceRect.width() * scale,
                    tile.sourceRect.height() * scale);
                painter.drawImage(tileTarget, tile.image);
            }
        }
    }

    painter.setPen(QColor(0, 0, 0, 70));
    painter.drawRect(target.adjusted(0, 0, -1, -1));

    if (m_ocrTextSelectionEnabled && m_ocrTextSelectionModel
        && m_ocrDebugOverlayEnabled
        && m_ocrTextSelectionModel->hasText()) {
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, false);
        const QVector<OcrSymbol> &symbols =
            m_ocrTextSelectionModel->symbols();
        const QRectF logicalImageBounds(
            QPointF(0, 0), QSizeF(logicalImageSize()));
        for (int index = 0; index < symbols.size(); ++index) {
            const OcrSymbol &symbol = symbols.at(index);
            const QRectF safeBounds = symbol.bounds.normalized().intersected(
                logicalImageBounds);
            if (safeBounds.isEmpty())
                continue;
            const QRectF symbolRect(
                target.left() + safeBounds.left() * scale,
                target.top() + safeBounds.top() * scale,
                safeBounds.width() * scale,
                safeBounds.height() * scale);
            const QRectF clipped = symbolRect.intersected(target);
            const bool selected =
                m_ocrTextSelectionModel->isSymbolSelected(index);
            const QColor background = selected
                ? QColor(32, 102, 210)
                : (symbol.supplemental
                       ? QColor(186, 242, 196)
                       : ((symbol.wordIndex & 1)
                       ? QColor(255, 232, 150)
                       : QColor(186, 232, 255)));
            const QColor foreground = selected
                ? QColor(Qt::white) : QColor(20, 24, 28);
            painter.fillRect(clipped, background);
            painter.setPen(QPen(selected ? QColor(255, 255, 255)
                                         : QColor(190, 32, 88),
                                selected ? 2.0 : 1.0));
            painter.drawRect(clipped.adjusted(0, 0, -1, -1));

            QFont symbolFont = font();
            symbolFont.setBold(true);
            symbolFont.setPixelSize(static_cast<int>(std::clamp<qreal>(
                clipped.height() * 0.55, 8.0, 24.0)));
            painter.setFont(symbolFont);
            painter.setPen(foreground);
            painter.drawText(clipped, Qt::AlignCenter, symbol.text);

            if (clipped.width() >= 54.0 && clipped.height() >= 30.0) {
                QFont indexFont = font();
                indexFont.setPixelSize(8);
                painter.setFont(indexFont);
                painter.drawText(
                    clipped.adjusted(2, 1, -2, -1),
                    Qt::AlignLeft | Qt::AlignTop,
                    QStringLiteral("%1L%2 W%3 #%4")
                        .arg(symbol.supplemental
                                 ? QStringLiteral("S ")
                                 : QString())
                        .arg(symbol.lineIndex)
                        .arg(symbol.wordIndex)
                        .arg(index));
            }
        }
        painter.restore();
    } else if (m_ocrTextSelectionEnabled && m_ocrTextSelectionModel
               && m_ocrTextSelectionModel->hasSelection()) {
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, false);
        const QColor highlight = palette().color(QPalette::Highlight);
        QColor fill = highlight;
        fill.setAlpha(92);
        QColor outline = highlight;
        outline.setAlpha(210);
        painter.setBrush(fill);
        painter.setPen(QPen(outline, 1.0));
        const QRectF logicalImageBounds(
            QPointF(0, 0), QSizeF(logicalImageSize()));
        for (const QRectF &bounds
             : m_ocrTextSelectionModel->selectedBounds()) {
            const QRectF safeBounds = bounds.normalized().intersected(
                logicalImageBounds);
            if (safeBounds.isEmpty())
                continue;
            const QRectF selectionRect(
                target.left() + safeBounds.left() * scale,
                target.top() + safeBounds.top() * scale,
                safeBounds.width() * scale,
                safeBounds.height() * scale);
            painter.drawRect(selectionRect.intersected(target));
        }
        painter.restore();
    }

    if (m_colorPickerEnabled && m_colorSamplePinned
        && imageBounds().contains(m_colorSamplePosition)) {
        const QPointF center(
            target.left()
                + (m_colorSamplePosition.x() + 0.5) * scale,
            target.top()
                + (m_colorSamplePosition.y() + 0.5) * scale);
        const qreal radius =
            std::max<qreal>(8.0, scale * 0.75);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(QColor(0, 0, 0, 190), 3));
        painter.drawEllipse(center, radius, radius);
        painter.drawLine(
            QPointF(center.x() - radius - 5, center.y()),
            QPointF(center.x() + radius + 5, center.y()));
        painter.drawLine(
            QPointF(center.x(), center.y() - radius - 5),
            QPointF(center.x(), center.y() + radius + 5));
        painter.setPen(
            QPen(QColor(255, 255, 255, 235), 1));
        painter.drawEllipse(center, radius, radius);
        painter.drawLine(
            QPointF(center.x() - radius - 5, center.y()),
            QPointF(center.x() + radius + 5, center.y()));
        painter.drawLine(
            QPointF(center.x(), center.y() - radius - 5),
            QPointF(center.x(), center.y() + radius + 5));
    }
}

void ImageCanvas::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    clampPan();
    if (m_zoomMode == ZoomMode::Fit)
        emit zoomChanged(effectiveScale());
}

void ImageCanvas::wheelEvent(QWheelEvent *event)
{
    const bool control =
        event->modifiers().testFlag(Qt::ControlModifier);
    const QString &action =
        control ? m_ctrlWheelAction : m_wheelAction;
    const QPoint angleDelta = event->angleDelta();
    const QPoint pixelDelta = event->pixelDelta();
    const qreal steps = angleDelta.y() != 0
        ? angleDelta.y() / 120.0
        : pixelDelta.y() / 60.0;
    if (action == QStringLiteral("scroll")
        && (!angleDelta.isNull()
            || !pixelDelta.isNull())) {
        m_navigationWheelAccumulator = 0.0;
        const bool horizontal = event->modifiers().testFlag(
            Qt::ShiftModifier);
        const QPoint delta = !pixelDelta.isNull()
            ? pixelDelta : angleDelta / 2;
        qreal distance = 0.0;
        if (horizontal) {
            distance = delta.y() != 0
                ? delta.y() : delta.x();
            m_pan.rx() += distance;
        } else if (std::abs(delta.x())
                   > std::abs(delta.y())) {
            distance = delta.x();
            m_pan.rx() += distance;
        } else {
            distance = delta.y();
            m_pan.ry() += distance;
        }
        clampPan();
        update();
        event->accept();
        return;
    }
    if (!qFuzzyIsNull(steps)) {
        if (action == QStringLiteral("navigate")) {
            m_navigationWheelAccumulator += steps;
            if (std::abs(m_navigationWheelAccumulator) < 1.0) {
                event->accept();
                return;
            }
            emit mouseActionRequested(
                m_navigationWheelAccumulator > 0
                    ? QStringLiteral("previous")
                    : QStringLiteral("next"));
            m_navigationWheelAccumulator = 0.0;
        } else if (action == QStringLiteral("zoom")) {
            m_navigationWheelAccumulator = 0.0;
            setZoom(effectiveScale() * std::pow(1.15, steps),
                    event->position());
        } else {
            m_navigationWheelAccumulator = 0.0;
            QWidget::wheelEvent(event);
            return;
        }
        event->accept();
        return;
    }
    QWidget::wheelEvent(event);
}

bool ImageCanvas::handleNativeGesture(QNativeGestureEvent *event)
{
    if (m_image.isNull())
        return false;

    switch (event->gestureType()) {
    case Qt::BeginNativeGesture:
        m_navigationWheelAccumulator = 0.0;
        event->accept();
        return true;
    case Qt::ZoomNativeGesture:
        setZoom(effectiveScale()
                    * CanvasGestureController::nativeZoomFactor(
                        event->value()),
                event->position());
        event->accept();
        return true;
    case Qt::PanNativeGesture:
        panBy(event->delta());
        event->accept();
        return true;
    case Qt::SmartZoomNativeGesture:
        performMouseAction(QStringLiteral("toggle_zoom"),
                           event->position());
        event->accept();
        return true;
    case Qt::EndNativeGesture:
        event->accept();
        return true;
    default:
        return false;
    }
}

bool ImageCanvas::handleTouchEvent(QTouchEvent *event)
{
    if (m_image.isNull())
        return false;

    const QList<QEventPoint> &points = event->points();
    if (event->type() == QEvent::TouchCancel
        || event->type() == QEvent::TouchEnd
        || points.size() < 2) {
        const bool wasActive =
            m_gestureController.isTouchGestureActive();
        m_gestureController.endTouchGesture();
        if (wasActive) {
            event->accept();
            return true;
        }
        return false;
    }

    const QPointF first = points.at(0).position();
    const QPointF second = points.at(1).position();
    if (!m_gestureController.isTouchGestureActive()) {
        if (!m_gestureController.beginTouchGesture(first, second))
            return false;
        m_navigationWheelAccumulator = 0.0;
        event->accept();
        return true;
    }

    const auto update =
        m_gestureController.updateTouchGesture(first, second);
    if (!update)
        return false;
    setZoom(effectiveScale() * update->zoomFactor,
            update->anchor);
    panBy(update->panDelta);
    event->accept();
    return true;
}

void ImageCanvas::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton
        && performMouseAction(m_middleButtonAction,
                              event->position())) {
        event->accept();
        return;
    }
    if (event->button() == Qt::BackButton
        && performMouseAction(m_backButtonAction,
                              event->position())) {
        event->accept();
        return;
    }
    if (event->button() == Qt::ForwardButton
        && performMouseAction(m_forwardButtonAction,
                              event->position())) {
        event->accept();
        return;
    }
    if (event->button() == Qt::LeftButton && m_colorPickerEnabled
        && !m_image.isNull()) {
        setFocus(Qt::MouseFocusReason);
        if (emitColorSample(event->position(), true))
            setColorSamplePinned(true);
        event->accept();
        return;
    }
    if (event->button() == Qt::LeftButton
        && m_ocrTextSelectionEnabled && !m_image.isNull()) {
        setFocus(Qt::MouseFocusReason);
        if (m_ocrTextSelectionModel->beginSelection(
                widgetToImagePosition(event->position()))) {
            m_selectingOcrText = true;
            setCursor(Qt::IBeamCursor);
            event->accept();
            return;
        }
        m_ocrTextSelectionModel->clearSelection();
        event->accept();
        return;
    }
    if (event->button() == Qt::LeftButton && !m_image.isNull()) {
        m_dragging = true;
        m_dragStart = event->position().toPoint();
        m_panAtDragStart = m_pan;
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void ImageCanvas::mouseMoveEvent(QMouseEvent *event)
{
    if (m_selectingOcrText) {
        m_ocrTextSelectionModel->updateSelection(
            widgetToImagePosition(event->position()));
        event->accept();
        return;
    }
    if (m_dragging) {
        m_pan = m_panAtDragStart + event->position() - QPointF(m_dragStart);
        clampPan();
        update();
        event->accept();
        return;
    }
    if (m_colorPickerEnabled && !m_colorSamplePinned
        && !m_image.isNull()) {
        emitColorSample(event->position(), false);
        event->accept();
        return;
    }
    if (!m_image.isNull() && !m_colorPickerEnabled)
        updateInteractionCursor(event->position());
    QWidget::mouseMoveEvent(event);
}

void ImageCanvas::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_selectingOcrText) {
        m_selectingOcrText = false;
        updateInteractionCursor(event->position());
        event->accept();
        return;
    }
    if (event->button() == Qt::LeftButton && m_dragging) {
        m_dragging = false;
        updateInteractionCursor(event->position());
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void ImageCanvas::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton
        && m_ocrTextSelectionEnabled && !m_image.isNull()
        && m_ocrTextSelectionModel->selectWordAt(
            widgetToImagePosition(event->position()))) {
        event->accept();
        return;
    }
    if (event->button() == Qt::LeftButton && !m_image.isNull()) {
        if (performMouseAction(m_doubleClickAction,
                               event->position())) {
            event->accept();
            return;
        }
    }
    QWidget::mouseDoubleClickEvent(event);
}

void ImageCanvas::keyPressEvent(QKeyEvent *event)
{
    if (m_ocrTextSelectionEnabled
        && event->matches(QKeySequence::SelectAll)) {
        m_ocrTextSelectionModel->selectAll();
        event->accept();
        return;
    }
    if (m_ocrTextSelectionEnabled
        && event->key() == Qt::Key_Escape
        && m_ocrTextSelectionModel->hasSelection()) {
        m_ocrTextSelectionModel->clearSelection();
        event->accept();
        return;
    }
    if (m_colorPickerEnabled && m_colorSamplePinned
        && imageBounds().contains(m_colorSamplePosition)) {
        QPoint delta;
        const int amount =
            event->modifiers().testFlag(Qt::ShiftModifier)
                ? 5 : 1;
        if (event->key() == Qt::Key_Left)
            delta.setX(-amount);
        else if (event->key() == Qt::Key_Right)
            delta.setX(amount);
        else if (event->key() == Qt::Key_Up)
            delta.setY(-amount);
        else if (event->key() == Qt::Key_Down)
            delta.setY(amount);

        if (!delta.isNull()) {
            const QPoint adjusted(
                std::clamp(
                    m_colorSamplePosition.x() + delta.x(),
                    0, logicalImageSize().width() - 1),
                std::clamp(
                    m_colorSamplePosition.y() + delta.y(),
                    0, logicalImageSize().height() - 1));
            emitColorSampleAt(adjusted, false, true);
            event->accept();
            return;
        }
    }
    QWidget::keyPressEvent(event);
}

QPointF ImageCanvas::widgetToImagePosition(
    const QPointF &widgetPosition) const
{
    const qreal scale = effectiveScale();
    if (scale <= 0.0)
        return {};
    return (widgetPosition - imageTopLeft(scale)) / scale;
}

void ImageCanvas::updateInteractionCursor(
    const QPointF &)
{
    if (m_colorPickerEnabled) {
        setCursor(Qt::CrossCursor);
        return;
    }
    if (m_ocrTextSelectionEnabled) {
        setCursor(Qt::IBeamCursor);
        return;
    }
    setCursor(Qt::OpenHandCursor);
}

bool ImageCanvas::performMouseAction(
    const QString &actionId, const QPointF &position)
{
    if (actionId == QStringLiteral("none"))
        return false;
    if (actionId == QStringLiteral("toggle_zoom")) {
        if (m_zoomMode == ZoomMode::Fit)
            setZoom(1.0, position);
        else
            fitToWindow();
        return true;
    }
    emit mouseActionRequested(actionId);
    return true;
}

bool ImageCanvas::emitColorSample(
    const QPointF &widgetPosition, bool picked)
{
    if (m_image.isNull())
        return false;

    const qreal scale = effectiveScale();
    const QRectF target(
        imageTopLeft(scale), QSizeF(logicalImageSize()) * scale);
    if (!target.contains(widgetPosition))
        return false;

    const QPoint position(
        std::clamp(
            qFloor((widgetPosition.x() - target.left()) / scale),
            0, logicalImageSize().width() - 1),
        std::clamp(
            qFloor((widgetPosition.y() - target.top()) / scale),
            0, logicalImageSize().height() - 1));
    return emitColorSampleAt(position, picked, false);
}

bool ImageCanvas::emitColorSampleAt(
    const QPoint &imagePosition, bool picked,
    bool adjusted)
{
    if (m_image.isNull()
        || !imageBounds().contains(imagePosition)) {
        return false;
    }
    m_colorSamplePosition = imagePosition;
    if (m_imageSource && m_imageSource->isRegionBacked()) {
        m_largeImageSampleController->requestSample(
            imagePosition, picked, adjusted);
        update();
        return true;
    }
    const QSize logical = logicalImageSize();
    const auto previewPoint = [this, logical](const QPoint &point) {
        return QPoint(
            std::clamp(
                static_cast<int>((static_cast<qint64>(point.x())
                    * m_image.width()) / logical.width()),
                0, m_image.width() - 1),
            std::clamp(
                static_cast<int>((static_cast<qint64>(point.y())
                    * m_image.height()) / logical.height()),
                0, m_image.height() - 1));
    };
    const QColor color = m_image.pixelColor(
        previewPoint(imagePosition));

    constexpr int radius = 5;
    constexpr int extent = radius * 2 + 1;
    QImage sample(
        extent, extent, QImage::Format_ARGB32_Premultiplied);
    sample.fill(Qt::transparent);
    for (int sampleY = 0; sampleY < extent; ++sampleY) {
        const int sourceY =
            imagePosition.y() + sampleY - radius;
        if (sourceY < 0 || sourceY >= logical.height())
            continue;
        for (int sampleX = 0; sampleX < extent; ++sampleX) {
            const int sourceX =
                imagePosition.x() + sampleX - radius;
            if (sourceX < 0 || sourceX >= logical.width())
                continue;
            sample.setPixelColor(
                sampleX, sampleY,
                m_image.pixelColor(
                    previewPoint(QPoint(sourceX, sourceY))));
        }
    }

    if (adjusted) {
        emit colorSampleAdjusted(
            color, imagePosition, sample);
    } else {
        emit colorHovered(
            color, imagePosition, sample);
    }
    if (picked)
        emit colorPicked(color, imagePosition);
    update();
    return true;
}

void ImageCanvas::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void ImageCanvas::dropEvent(QDropEvent *event)
{
    QStringList files;
    for (const QUrl &url : event->mimeData()->urls()) {
        if (url.isLocalFile())
            files.append(url.toLocalFile());
    }
    if (!files.isEmpty()) {
        emit filesDropped(files);
        event->acceptProposedAction();
    }
}

qreal ImageCanvas::fitScale() const
{
    if (m_image.isNull())
        return 1.0;
    const QSizeF available = QSizeF(size()) - QSizeF(32, 32);
    const QSize logical = logicalImageSize();
    return std::min({available.width() / logical.width(),
                     available.height() / logical.height(), 1.0});
}

qreal ImageCanvas::widthScale() const
{
    return m_image.isNull() ? 1.0
                            : std::max(0.01, (width() - 32.0)
                                / logicalImageSize().width());
}

qreal ImageCanvas::heightScale() const
{
    return m_image.isNull() ? 1.0
                            : std::max(0.01, (height() - 32.0)
                                / logicalImageSize().height());
}

qreal ImageCanvas::effectiveScale() const
{
    switch (m_zoomMode) {
    case ZoomMode::Fit:
        return fitScale();
    case ZoomMode::FitWidth:
        return widthScale();
    case ZoomMode::FitHeight:
        return heightScale();
    case ZoomMode::Fill:
        return std::max(widthScale(), heightScale());
    case ZoomMode::ActualSize:
        return 1.0;
    case ZoomMode::Custom:
        return m_customZoom;
    }
    return 1.0;
}

QPointF ImageCanvas::imageTopLeft(qreal scale) const
{
    const QSizeF scaled = QSizeF(logicalImageSize()) * scale;
    return QPointF((width() - scaled.width()) / 2.0,
                   (height() - scaled.height()) / 2.0) + m_pan;
}

QRect ImageCanvas::imageBounds() const
{
    return QRect(QPoint(), logicalImageSize());
}

void ImageCanvas::clampPan()
{
    if (m_image.isNull()) {
        m_pan = {};
        return;
    }
    if (m_zoomMode == ZoomMode::Fit) {
        m_pan = {};
        return;
    }
    const QSizeF scaled = QSizeF(logicalImageSize()) * effectiveScale();
    const qreal maxX = std::max(
        0.0, (scaled.width() - width()) / 2.0);
    const qreal maxY = std::max(
        0.0, (scaled.height() - height()) / 2.0);
    m_pan.setX(std::clamp(m_pan.x(), -maxX, maxX));
    m_pan.setY(std::clamp(m_pan.y(), -maxY, maxY));
}

void ImageCanvas::panBy(const QPointF &delta)
{
    if (m_image.isNull() || delta.isNull())
        return;
    m_pan += delta;
    clampPan();
    update();
}
