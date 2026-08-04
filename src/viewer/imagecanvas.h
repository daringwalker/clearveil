#pragma once

#include "canvasgesturecontroller.h"
#include "imagesource.h"
#include "ocrresult.h"

#include <QImage>
#include <QColor>
#include <QPointF>
#include <QString>
#include <QWidget>

class ImageCanvas : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal zoom READ zoom NOTIFY zoomChanged)

public:
    struct CanvasAppearance {
        bool transparencyCheckerboardVisible = true;
        QColor checkerboardLight{210, 213, 217};
        QColor checkerboardDark{164, 168, 174};
        int checkerboardTileSize = 12;
    };

    enum class ZoomMode {
        Fit,
        FitWidth,
        FitHeight,
        Fill,
        ActualSize,
        Custom
    };

    explicit ImageCanvas(QWidget *parent = nullptr);

    void setImage(const QImage &image, bool preserveView = false);
    void setColorManagedImage(const QImage &sourceImage,
                              const QImage &displayImage,
                              bool preserveView = false,
                              const QSize &logicalSize = {},
                              const ImageSourcePtr &imageSource = {});
    [[nodiscard]] const QImage &sourceImage() const;
    [[nodiscard]] const QImage &displayImage() const;
    [[nodiscard]] QSize logicalImageSize() const;
    [[nodiscard]] qreal zoom() const;
    [[nodiscard]] ZoomMode zoomMode() const;
    [[nodiscard]] bool isZoomLocked() const;
    [[nodiscard]] bool isColorSamplePinned() const;
    [[nodiscard]] QPointF viewOffset() const;
    [[nodiscard]] CanvasAppearance canvasAppearance() const;
    void setCanvasAppearance(const CanvasAppearance &appearance);
    void setTransparencyCheckerboardVisible(bool visible);
    void setColorPickerEnabled(bool enabled);
    void setOcrTextSelectionEnabled(bool enabled);
    void setOcrDebugOverlayEnabled(bool enabled);
    void setOcrResult(const OcrResult &result);
    void clearOcrResult();
    [[nodiscard]] bool ocrTextSelectionEnabled() const;
    [[nodiscard]] bool ocrDebugOverlayEnabled() const;
    [[nodiscard]] bool hasOcrText() const;
    [[nodiscard]] bool hasSelectedText() const;
    [[nodiscard]] QString selectedText() const;
    void setMouseActions(const QString &wheelAction,
                         const QString &ctrlWheelAction,
                         const QString &doubleClickAction,
                         const QString &middleButtonAction,
                         const QString &backButtonAction,
                         const QString &forwardButtonAction);

public slots:
    void fitToWindow();
    void fitToWidth();
    void fitToHeight();
    void fillWindow();
    void actualSize();
    void zoomIn();
    void zoomOut();
    void setZoom(qreal zoom, const QPointF &anchor = {});
    void setZoomLocked(bool locked);
    void setColorSamplePinned(bool pinned);
    void pinColorSampleAt(const QPoint &imagePosition);

signals:
    void zoomChanged(qreal zoom);
    void filesDropped(const QStringList &files);
    void openRequested();
    void colorPicked(const QColor &color, const QPoint &imagePosition);
    void colorHovered(const QColor &color, const QPoint &imagePosition,
                      const QImage &sample);
    void colorSampleAdjusted(const QColor &color,
                             const QPoint &imagePosition,
                             const QImage &sample);
    void colorSamplePinnedChanged(bool pinned);
    void mouseActionRequested(const QString &actionId);
    void ocrSelectionChanged(bool hasSelection);

protected:
    bool event(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    [[nodiscard]] qreal fitScale() const;
    [[nodiscard]] qreal widthScale() const;
    [[nodiscard]] qreal heightScale() const;
    [[nodiscard]] qreal effectiveScale() const;
    [[nodiscard]] QPointF imageTopLeft(qreal scale) const;
    [[nodiscard]] QRect imageBounds() const;
    void clampPan();
    void panBy(const QPointF &delta);
    bool handleNativeGesture(class QNativeGestureEvent *event);
    bool handleTouchEvent(class QTouchEvent *event);
    bool performMouseAction(const QString &actionId,
                            const QPointF &position);
    bool emitColorSample(const QPointF &widgetPosition,
                         bool picked);
    bool emitColorSampleAt(const QPoint &imagePosition,
                           bool picked, bool adjusted);
    [[nodiscard]] QPointF widgetToImagePosition(
        const QPointF &widgetPosition) const;
    void updateInteractionCursor(const QPointF &widgetPosition = {});

    QImage m_image;
    QImage m_displayImage;
    QSize m_logicalImageSize;
    ImageSourcePtr m_imageSource;
    class TiledImageViewModel *m_tiledImageViewModel = nullptr;
    class LargeImageSampleController *m_largeImageSampleController = nullptr;
    class OcrTextSelectionModel *m_ocrTextSelectionModel = nullptr;
    CanvasAppearance m_canvasAppearance;
    ZoomMode m_zoomMode = ZoomMode::Fit;
    qreal m_customZoom = 1.0;
    QPointF m_pan;
    QPoint m_dragStart;
    QPointF m_panAtDragStart;
    bool m_dragging = false;
    bool m_zoomLocked = false;
    bool m_colorPickerEnabled = false;
    bool m_colorSamplePinned = false;
    bool m_ocrTextSelectionEnabled = false;
    bool m_ocrDebugOverlayEnabled = false;
    bool m_selectingOcrText = false;
    QPoint m_colorSamplePosition{-1, -1};
    QString m_wheelAction = QStringLiteral("zoom");
    QString m_ctrlWheelAction = QStringLiteral("zoom");
    QString m_doubleClickAction = QStringLiteral("toggle_zoom");
    QString m_middleButtonAction = QStringLiteral("none");
    QString m_backButtonAction = QStringLiteral("previous");
    QString m_forwardButtonAction = QStringLiteral("next");
    qreal m_navigationWheelAccumulator = 0.0;
    CanvasGestureController m_gestureController;
};
