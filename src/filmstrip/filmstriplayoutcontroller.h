#pragma once

#include <QObject>
#include <QPointer>
#include <QString>

class QAction;
class FilmstripView;
class PanelTitleBar;
class QDockWidget;
class QToolButton;

class FilmstripLayoutController final : public QObject
{
    Q_OBJECT

public:
    enum class Mode {
        Automatic,
        Horizontal,
        Vertical
    };

    FilmstripLayoutController(
        QDockWidget *dock, FilmstripView *view,
        PanelTitleBar *titleBar,
        QObject *parent = nullptr);

    void setModeName(const QString &mode);
    [[nodiscard]] QString modeName() const;
    [[nodiscard]] bool isVertical() const;
    void updateLayout();

signals:
    void modeChanged(const QString &mode);
    void orientationChanged(bool vertical);

private:
    void setMode(Mode mode, bool notify);
    void updateButton();

    QPointer<QDockWidget> m_dock;
    QPointer<FilmstripView> m_view;
    QToolButton *m_button = nullptr;
    QAction *m_autoAction = nullptr;
    QAction *m_horizontalAction = nullptr;
    QAction *m_verticalAction = nullptr;
    Mode m_mode = Mode::Automatic;
    bool m_vertical = false;
};
