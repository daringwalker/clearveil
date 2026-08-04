#include "slideshowcontroller.h"

#include <QRandomGenerator>
#include <QTimer>

#include <algorithm>
#include <utility>

SlideshowController::SlideshowController(
    QObject *parent, RandomIndexPicker randomIndexPicker)
    : QObject(parent)
    , m_timer(new QTimer(this))
    , m_randomIndexPicker(
          randomIndexPicker
              ? std::move(randomIndexPicker)
              : RandomIndexPicker([](int upperExclusive) {
                    return QRandomGenerator::global()->bounded(
                        upperExclusive);
                }))
{
    setObjectName(QStringLiteral("slideshowController"));
    m_timer->setObjectName(QStringLiteral("slideshowTimer"));
    m_timer->setInterval(3'000);
    connect(m_timer, &QTimer::timeout,
            this, &SlideshowController::advance);
}

void SlideshowController::setIntervalMs(int intervalMs)
{
    m_timer->setInterval(std::max(1, intervalMs));
}

int SlideshowController::intervalMs() const
{
    return m_timer->interval();
}

void SlideshowController::setRandomOrder(bool randomOrder)
{
    m_randomOrder = randomOrder;
}

bool SlideshowController::randomOrder() const
{
    return m_randomOrder;
}

void SlideshowController::setFullscreenEnabled(bool enabled)
{
    m_fullscreenEnabled = enabled;
    if (!enabled && m_enteredFullscreen) {
        m_enteredFullscreen = false;
        emit fullscreenRequested(false);
    }
}

bool SlideshowController::fullscreenEnabled() const
{
    return m_fullscreenEnabled;
}

void SlideshowController::setNavigationState(
    int itemCount, int currentIndex)
{
    m_itemCount = std::max(0, itemCount);
    m_currentIndex = m_itemCount > 0
        ? std::clamp(currentIndex, 0, m_itemCount - 1) : -1;
    if (m_running && m_itemCount < 2)
        stop();
}

int SlideshowController::itemCount() const
{
    return m_itemCount;
}

int SlideshowController::currentIndex() const
{
    return m_currentIndex;
}

bool SlideshowController::start(bool windowIsFullscreen)
{
    if (m_itemCount < 2) {
        stop();
        return false;
    }
    if (m_running)
        return true;

    if (m_fullscreenEnabled && !windowIsFullscreen) {
        m_enteredFullscreen = true;
        emit fullscreenRequested(true);
    }
    m_timer->start();
    setRunning(true);
    return true;
}

void SlideshowController::stop()
{
    m_timer->stop();
    setRunning(false);
    if (m_enteredFullscreen) {
        m_enteredFullscreen = false;
        emit fullscreenRequested(false);
    }
}

void SlideshowController::advance()
{
    if (m_itemCount < 2) {
        stop();
        return;
    }

    const int current = std::clamp(
        m_currentIndex, 0, m_itemCount - 1);
    int target = current + 1 < m_itemCount
        ? current + 1 : 0;
    if (m_randomOrder) {
        const int candidate = std::clamp(
            m_randomIndexPicker(m_itemCount - 1),
            0, m_itemCount - 2);
        target = candidate >= current
            ? candidate + 1 : candidate;
    }
    m_currentIndex = target;
    emit activateIndexRequested(target);
}

bool SlideshowController::isRunning() const
{
    return m_running;
}

void SlideshowController::setRunning(bool running)
{
    if (m_running == running)
        return;
    m_running = running;
    emit runningChanged(m_running);
}
