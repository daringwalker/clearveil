#include "canvasgesturecontroller.h"

#include <QLineF>

#include <algorithm>

bool CanvasGestureController::isTouchGestureActive() const
{
    return m_touchGestureActive;
}

bool CanvasGestureController::beginTouchGesture(
    const QPointF &first, const QPointF &second)
{
    const qreal distance = QLineF(first, second).length();
    if (distance < 1.0) {
        endTouchGesture();
        return false;
    }

    m_touchGestureActive = true;
    m_lastCenter = (first + second) / 2.0;
    m_lastDistance = distance;
    return true;
}

std::optional<CanvasGestureController::Update>
CanvasGestureController::updateTouchGesture(
    const QPointF &first, const QPointF &second)
{
    const qreal distance = QLineF(first, second).length();
    if (distance < 1.0)
        return std::nullopt;

    const QPointF center = (first + second) / 2.0;
    if (!m_touchGestureActive) {
        beginTouchGesture(first, second);
        return std::nullopt;
    }

    Update update;
    update.zoomFactor = std::clamp(
        distance / m_lastDistance, 0.25, 4.0);
    update.anchor = center;
    update.panDelta = center - m_lastCenter;
    m_lastCenter = center;
    m_lastDistance = distance;
    return update;
}

void CanvasGestureController::endTouchGesture()
{
    m_touchGestureActive = false;
    m_lastCenter = {};
    m_lastDistance = 0.0;
}

qreal CanvasGestureController::nativeZoomFactor(qreal value)
{
    return std::clamp(1.0 + value, 0.1, 10.0);
}
