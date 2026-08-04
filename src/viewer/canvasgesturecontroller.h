#pragma once

#include <QPointF>

#include <optional>

class CanvasGestureController
{
public:
    struct Update {
        qreal zoomFactor = 1.0;
        QPointF anchor;
        QPointF panDelta;
    };

    [[nodiscard]] bool isTouchGestureActive() const;
    bool beginTouchGesture(const QPointF &first,
                           const QPointF &second);
    [[nodiscard]] std::optional<Update> updateTouchGesture(
        const QPointF &first, const QPointF &second);
    void endTouchGesture();

    [[nodiscard]] static qreal nativeZoomFactor(qreal value);

private:
    bool m_touchGestureActive = false;
    QPointF m_lastCenter;
    qreal m_lastDistance = 0.0;
};
