#pragma once

#include <QListView>

#include <functional>

class QScrollBar;

class FilmstripView final : public QListView
{
public:
    explicit FilmstripView(QWidget *parent = nullptr);

    static constexpr int overlayExtent() { return 18; }

    void setResizeHandler(std::function<void()> handler);
    void setCloseHandler(std::function<void(int)> handler);
    void setCloseButtonsVisible(bool visible);
    [[nodiscard]] bool closeButtonsVisible() const;
    void setFileNamesVisible(bool visible);
    [[nodiscard]] bool fileNamesVisible() const;
    void setVerticalLayout(bool vertical);
    [[nodiscard]] bool isVerticalLayout() const;
    [[nodiscard]] QScrollBar *activeScrollBar() const;
    void updateOverlayScrollBars();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QScrollBar *makeOverlayScrollBar(
        Qt::Orientation orientation, const QString &objectName);
    void positionOverlayScrollBars();
    void syncOverlayScrollBars();
    [[nodiscard]] int closeButtonRowAt(const QPoint &position) const;

    std::function<void()> m_resizeHandler;
    std::function<void(int)> m_closeHandler;
    QScrollBar *m_horizontalOverlay = nullptr;
    QScrollBar *m_verticalOverlay = nullptr;
    bool m_verticalLayout = false;
    int m_closePressedRow = -1;
};
