#pragma once

#include <QObject>

#include <functional>

class QTimer;

class SlideshowController final : public QObject
{
    Q_OBJECT

public:
    using RandomIndexPicker = std::function<int(int upperExclusive)>;

    explicit SlideshowController(
        QObject *parent = nullptr,
        RandomIndexPicker randomIndexPicker = {});

    void setIntervalMs(int intervalMs);
    [[nodiscard]] int intervalMs() const;

    void setRandomOrder(bool randomOrder);
    [[nodiscard]] bool randomOrder() const;

    void setFullscreenEnabled(bool enabled);
    [[nodiscard]] bool fullscreenEnabled() const;

    void setNavigationState(int itemCount, int currentIndex);
    [[nodiscard]] int itemCount() const;
    [[nodiscard]] int currentIndex() const;

    bool start(bool windowIsFullscreen);
    void stop();
    void advance();
    [[nodiscard]] bool isRunning() const;

signals:
    void activateIndexRequested(int index);
    void runningChanged(bool running);
    void fullscreenRequested(bool fullscreen);

private:
    void setRunning(bool running);

    QTimer *m_timer = nullptr;
    RandomIndexPicker m_randomIndexPicker;
    int m_itemCount = 0;
    int m_currentIndex = -1;
    bool m_randomOrder = false;
    bool m_fullscreenEnabled = false;
    bool m_enteredFullscreen = false;
    bool m_running = false;
};
