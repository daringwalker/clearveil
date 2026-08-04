#include "filmstriplayoutcontroller.h"

#include "clearveilicon.h"
#include "filmstripview.h"
#include "paneltitlebar.h"

#include <QAction>
#include <QActionGroup>
#include <QDockWidget>
#include <QMainWindow>
#include <QMenu>
#include <QSignalBlocker>
#include <QToolButton>

FilmstripLayoutController::FilmstripLayoutController(
    QDockWidget *dock, FilmstripView *view,
    PanelTitleBar *titleBar, QObject *parent)
    : QObject(parent), m_dock(dock), m_view(view)
{
    if (!dock || !view || !titleBar)
        return;

    m_button = new QToolButton(titleBar);
    m_button->setObjectName(
        QStringLiteral("floatingThumbnailLayoutButton"));
    m_button->setAutoRaise(true);
    m_button->setFixedSize(28, 24);
    m_button->setIconSize(QSize(17, 17));
    m_button->setPopupMode(QToolButton::InstantPopup);

    auto *menu = new QMenu(m_button);
    auto *group = new QActionGroup(menu);
    group->setExclusive(true);
    m_autoAction = menu->addAction(tr("Automatic layout"));
    m_horizontalAction = menu->addAction(tr("Horizontal layout"));
    m_verticalAction = menu->addAction(tr("Vertical layout"));
    for (QAction *action : {m_autoAction, m_horizontalAction,
                            m_verticalAction}) {
        action->setCheckable(true);
        group->addAction(action);
    }
    m_autoAction->setObjectName(
        QStringLiteral("floatingThumbnailLayoutAuto"));
    m_horizontalAction->setObjectName(
        QStringLiteral("floatingThumbnailLayoutHorizontal"));
    m_verticalAction->setObjectName(
        QStringLiteral("floatingThumbnailLayoutVertical"));
    m_autoAction->setIcon(
        ClearveilIcon::fromName(QStringLiteral("layout_auto")));
    m_horizontalAction->setIcon(
        ClearveilIcon::fromName(QStringLiteral("layout_horizontal")));
    m_verticalAction->setIcon(
        ClearveilIcon::fromName(QStringLiteral("layout_vertical")));
    m_button->setMenu(menu);
    titleBar->addControl(m_button);

    connect(m_autoAction, &QAction::triggered,
            this, [this] { setMode(Mode::Automatic, true); });
    connect(m_horizontalAction, &QAction::triggered,
            this, [this] { setMode(Mode::Horizontal, true); });
    connect(m_verticalAction, &QAction::triggered,
            this, [this] { setMode(Mode::Vertical, true); });
    connect(dock, &QDockWidget::topLevelChanged,
            this, [this] { updateLayout(); });
    connect(dock, &QDockWidget::dockLocationChanged,
            this, [this] { updateLayout(); });

    setMode(Mode::Automatic, false);
}

void FilmstripLayoutController::setModeName(const QString &mode)
{
    if (mode == QStringLiteral("horizontal"))
        setMode(Mode::Horizontal, false);
    else if (mode == QStringLiteral("vertical"))
        setMode(Mode::Vertical, false);
    else
        setMode(Mode::Automatic, false);
}

QString FilmstripLayoutController::modeName() const
{
    switch (m_mode) {
    case Mode::Horizontal:
        return QStringLiteral("horizontal");
    case Mode::Vertical:
        return QStringLiteral("vertical");
    case Mode::Automatic:
        return QStringLiteral("auto");
    }
    return QStringLiteral("auto");
}

bool FilmstripLayoutController::isVertical() const
{
    return m_vertical;
}

void FilmstripLayoutController::updateLayout()
{
    if (!m_dock || !m_view)
        return;

    const bool freeform = m_dock->isFloating()
        || m_dock->property("clearveilOverlay").toBool();
    bool vertical = false;
    if (freeform) {
        if (m_mode == Mode::Vertical) {
            vertical = true;
        } else if (m_mode == Mode::Horizontal) {
            vertical = false;
        } else {
            const QSize available = m_view->viewport()
                ? m_view->viewport()->size() : m_view->size();
            vertical = available.height() > available.width();
        }
    } else {
        const Qt::DockWidgetArea area = qobject_cast<QMainWindow *>(
            m_dock->parentWidget())
            ? qobject_cast<QMainWindow *>(m_dock->parentWidget())
                  ->dockWidgetArea(m_dock)
            : Qt::BottomDockWidgetArea;
        vertical = area == Qt::LeftDockWidgetArea
            || area == Qt::RightDockWidgetArea;
    }

    if (m_button)
        m_button->setVisible(freeform);
    if (m_vertical == vertical
        && m_view->isVerticalLayout() == vertical) {
        updateButton();
        return;
    }
    m_vertical = vertical;
    m_view->setVerticalLayout(vertical);
    updateButton();
    emit orientationChanged(vertical);
}

void FilmstripLayoutController::setMode(Mode mode, bool notify)
{
    const bool changed = m_mode != mode;
    m_mode = mode;
    {
        const QSignalBlocker autoBlocker(m_autoAction);
        const QSignalBlocker horizontalBlocker(m_horizontalAction);
        const QSignalBlocker verticalBlocker(m_verticalAction);
        if (m_autoAction)
            m_autoAction->setChecked(mode == Mode::Automatic);
        if (m_horizontalAction)
            m_horizontalAction->setChecked(mode == Mode::Horizontal);
        if (m_verticalAction)
            m_verticalAction->setChecked(mode == Mode::Vertical);
    }
    updateLayout();
    if (changed && notify)
        emit modeChanged(modeName());
}

void FilmstripLayoutController::updateButton()
{
    if (!m_button)
        return;
    QString iconName;
    QString text;
    switch (m_mode) {
    case Mode::Automatic:
        iconName = QStringLiteral("layout_auto");
        text = tr("Thumbnail layout: Automatic");
        break;
    case Mode::Horizontal:
        iconName = QStringLiteral("layout_horizontal");
        text = tr("Thumbnail layout: Horizontal");
        break;
    case Mode::Vertical:
        iconName = QStringLiteral("layout_vertical");
        text = tr("Thumbnail layout: Vertical");
        break;
    }
    m_button->setIcon(ClearveilIcon::fromName(iconName));
    m_button->setToolTip(text);
    m_button->setAccessibleName(text);
}
