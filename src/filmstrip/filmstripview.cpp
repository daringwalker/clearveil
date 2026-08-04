#include "filmstripview.h"

#include <QIcon>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <utility>

namespace {
class FilmstripDelegate final : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    [[nodiscard]] QRect closeButtonRect(
        const QStyleOptionViewItem &option) const
    {
        const QRect icon = iconRect(option);
        return QRect(icon.right() - 17, icon.top() + 3, 18, 18);
    }

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setClipRect(option.rect);

        const bool selected =
            option.state & QStyle::State_Selected;
        const bool hovered =
            option.state & QStyle::State_MouseOver;
        const QRect background =
            contentRect(option).adjusted(3, 3, -3, -3);
        if (selected) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(option.palette.highlight());
            painter->drawRoundedRect(background, 5, 5);
        } else if (hovered) {
            QColor hover = option.palette.color(
                QPalette::Highlight);
            hover.setAlpha(48);
            painter->setPen(Qt::NoPen);
            painter->setBrush(hover);
            painter->drawRoundedRect(background, 5, 5);
        }

        const QRect iconArea = iconRect(option);
        const QVariant decoration =
            index.data(Qt::DecorationRole);
        QPixmap pixmap;
        if (decoration.canConvert<QPixmap>())
            pixmap = qvariant_cast<QPixmap>(decoration);
        else if (decoration.canConvert<QIcon>())
            pixmap = qvariant_cast<QIcon>(decoration).pixmap(
                iconArea.size());
        if (!pixmap.isNull()) {
            const QPixmap scaled = pixmap.scaled(
                iconArea.size(), Qt::KeepAspectRatio,
                Qt::SmoothTransformation);
            const QPoint topLeft(
                iconArea.center().x() - scaled.width() / 2,
                iconArea.center().y() - scaled.height() / 2);
            painter->drawPixmap(topLeft, scaled);
        }

        const auto *view = dynamic_cast<const FilmstripView *>(
            option.widget);
        if (!view || view->fileNamesVisible()) {
            const QRect textArea = textRect(option);
            const QColor textColor = selected
                ? option.palette.color(QPalette::HighlightedText)
                : option.palette.color(QPalette::Text);
            painter->setPen(textColor);
            const QString text = option.fontMetrics.elidedText(
                index.data(Qt::DisplayRole).toString(),
                Qt::ElideMiddle, textArea.width());
            painter->drawText(textArea, Qt::AlignCenter, text);
        }

        if (view && view->closeButtonsVisible()
            && (selected || hovered)) {
            const QRect closeRect = closeButtonRect(option);
            QColor closeBackground = option.palette.color(
                selected ? QPalette::HighlightedText
                         : QPalette::Window);
            closeBackground.setAlpha(210);
            painter->setPen(Qt::NoPen);
            painter->setBrush(closeBackground);
            painter->drawEllipse(closeRect);
            painter->setPen(QPen(
                option.palette.color(
                    selected ? QPalette::Highlight
                             : QPalette::WindowText),
                1.5, Qt::SolidLine, Qt::RoundCap));
            const QPoint center = closeRect.center();
            painter->drawLine(center + QPoint(-3, -3),
                              center + QPoint(3, 3));
            painter->drawLine(center + QPoint(3, -3),
                              center + QPoint(-3, 3));
        }
        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override
    {
        if (const auto *view =
                qobject_cast<const QListView *>(option.widget)) {
            const QSize grid = view->gridSize();
            if (grid.isValid())
                return grid;
        }
        return QStyledItemDelegate::sizeHint(option, index);
    }

private:
    [[nodiscard]] QRect iconRect(
        const QStyleOptionViewItem &option) const
    {
        constexpr int spacing = 4;
        constexpr int outerMargin = 7;
        const QRect content = contentRect(option);
        const auto *view = dynamic_cast<const FilmstripView *>(
            option.widget);
        const bool showName = !view || view->fileNamesVisible();
        const int labelHeight = showName
            ? option.fontMetrics.height() + 4 : 0;
        const int maximumIconHeight = std::max(
            20, content.height() - labelHeight
                    - (showName ? spacing : 0)
                    - outerMargin * 2);
        const QSize iconSize = option.decorationSize.boundedTo(
            QSize(std::max(20,
                           content.width() - outerMargin * 2),
                  maximumIconHeight));
        const int contentHeight = iconSize.height()
            + (showName ? spacing + labelHeight : 0);
        const int top = content.top()
            + std::max(outerMargin,
                       (content.height() - contentHeight) / 2);
        return QRect(
            content.center().x() - iconSize.width() / 2,
            top, iconSize.width(), iconSize.height());
    }

    [[nodiscard]] QRect textRect(
        const QStyleOptionViewItem &option) const
    {
        constexpr int spacing = 4;
        const QRect icon = iconRect(option);
        const QRect content = contentRect(option);
        const int labelHeight = option.fontMetrics.height() + 4;
        return QRect(content.left() + 5,
                     icon.bottom() + 1 + spacing,
                     content.width() - 10, labelHeight);
    }

    [[nodiscard]] QRect contentRect(
        const QStyleOptionViewItem &option) const
    {
        QRect content = option.rect;
        if (const auto *view =
                dynamic_cast<const FilmstripView *>(option.widget)) {
            if (view->isVerticalLayout())
                content.adjust(0, 0, -FilmstripView::overlayExtent(), 0);
            else
                content.adjust(0, 0, 0, -FilmstripView::overlayExtent());
        }
        return content;
    }
};
}

FilmstripView::FilmstripView(QWidget *parent)
    : QListView(parent)
{
    setProperty("closeButtonsVisible", true);
    setItemDelegate(new FilmstripDelegate(this));
    m_horizontalOverlay = makeOverlayScrollBar(
        Qt::Horizontal,
        QStringLiteral("filmstripHorizontalScrollBar"));
    m_verticalOverlay = makeOverlayScrollBar(
        Qt::Vertical,
        QStringLiteral("filmstripVerticalScrollBar"));

    connect(horizontalScrollBar(), &QScrollBar::rangeChanged,
            this, [this] { syncOverlayScrollBars(); });
    connect(horizontalScrollBar(), &QScrollBar::valueChanged,
            m_horizontalOverlay, &QScrollBar::setValue);
    connect(m_horizontalOverlay, &QScrollBar::valueChanged,
            horizontalScrollBar(), &QScrollBar::setValue);
    connect(verticalScrollBar(), &QScrollBar::rangeChanged,
            this, [this] { syncOverlayScrollBars(); });
    connect(verticalScrollBar(), &QScrollBar::valueChanged,
            m_verticalOverlay, &QScrollBar::setValue);
    connect(m_verticalOverlay, &QScrollBar::valueChanged,
            verticalScrollBar(), &QScrollBar::setValue);
}

void FilmstripView::setResizeHandler(
    std::function<void()> handler)
{
    m_resizeHandler = std::move(handler);
}

void FilmstripView::setCloseHandler(
    std::function<void(int)> handler)
{
    m_closeHandler = std::move(handler);
}

void FilmstripView::setCloseButtonsVisible(bool visible)
{
    if (property("closeButtonsVisible").toBool() == visible)
        return;
    setProperty("closeButtonsVisible", visible);
    viewport()->update();
}

bool FilmstripView::closeButtonsVisible() const
{
    return property("closeButtonsVisible").toBool();
}

void FilmstripView::setFileNamesVisible(bool visible)
{
    if (property("fileNamesVisible").isValid()
        && property("fileNamesVisible").toBool() == visible) {
        return;
    }
    setProperty("fileNamesVisible", visible);
    doItemsLayout();
    viewport()->update();
    if (m_resizeHandler)
        m_resizeHandler();
}

bool FilmstripView::fileNamesVisible() const
{
    const QVariant value = property("fileNamesVisible");
    return !value.isValid() || value.toBool();
}

void FilmstripView::setVerticalLayout(bool vertical)
{
    if (m_verticalLayout == vertical)
        return;
    m_verticalLayout = vertical;
    setProperty("filmstripVerticalLayout", vertical);
    setFlow(QListView::LeftToRight);
    setWrapping(vertical);
    doItemsLayout();
    positionOverlayScrollBars();
    syncOverlayScrollBars();
}

bool FilmstripView::isVerticalLayout() const
{
    return m_verticalLayout;
}

QScrollBar *FilmstripView::activeScrollBar() const
{
    return m_verticalLayout ? verticalScrollBar()
                            : horizontalScrollBar();
}

void FilmstripView::updateOverlayScrollBars()
{
    syncOverlayScrollBars();
}

void FilmstripView::resizeEvent(QResizeEvent *event)
{
    QListView::resizeEvent(event);
    positionOverlayScrollBars();
    syncOverlayScrollBars();
    if (m_resizeHandler)
        m_resizeHandler();
}

void FilmstripView::wheelEvent(QWheelEvent *event)
{
    QScrollBar *bar = activeScrollBar();
    if (bar && bar->maximum() > bar->minimum()) {
        const QPoint pixelDelta = event->pixelDelta();
        int delta = m_verticalLayout
            ? pixelDelta.y()
            : (std::abs(pixelDelta.x()) > std::abs(pixelDelta.y())
                   ? pixelDelta.x() : pixelDelta.y());
        if (delta == 0) {
            const QPoint angleDelta = event->angleDelta();
            delta = m_verticalLayout
                ? angleDelta.y()
                : (std::abs(angleDelta.x()) > std::abs(angleDelta.y())
                       ? angleDelta.x() : angleDelta.y());
            const int itemStep = std::max(
                24, m_verticalLayout
                    ? gridSize().height() / 3
                    : gridSize().width() / 3);
            delta = qRound(delta / 120.0 * itemStep);
        }
        if (delta != 0) {
            bar->setValue(bar->value() - delta);
            event->accept();
            return;
        }
    }
    QListView::wheelEvent(event);
}

void FilmstripView::mousePressEvent(QMouseEvent *event)
{
    m_closePressedRow = closeButtonRowAt(
        event->position().toPoint());
    if (event->button() == Qt::LeftButton
        && m_closePressedRow >= 0) {
        event->accept();
        return;
    }
    QListView::mousePressEvent(event);
}

void FilmstripView::mouseReleaseEvent(QMouseEvent *event)
{
    const int releasedRow = closeButtonRowAt(
        event->position().toPoint());
    if (event->button() == Qt::LeftButton
        && m_closePressedRow >= 0
        && releasedRow == m_closePressedRow) {
        const int row = m_closePressedRow;
        m_closePressedRow = -1;
        if (m_closeHandler)
            m_closeHandler(row);
        event->accept();
        return;
    }
    m_closePressedRow = -1;
    QListView::mouseReleaseEvent(event);
}

QScrollBar *FilmstripView::makeOverlayScrollBar(
    Qt::Orientation orientation, const QString &objectName)
{
    auto *bar = new QScrollBar(orientation, this);
    bar->setObjectName(objectName);
    bar->setFocusPolicy(Qt::NoFocus);
    bar->setContextMenuPolicy(Qt::NoContextMenu);
    bar->setStyleSheet(QStringLiteral(
        "QScrollBar { background: transparent; border: none; }"
        "QScrollBar:horizontal { height: 14px; margin: 0; }"
        "QScrollBar:vertical { width: 14px; margin: 0; }"
        "QScrollBar::handle { background: palette(mid);"
        " border-radius: 3px; min-width: 28px; min-height: 28px; }"
        "QScrollBar::handle:horizontal { margin: 4px 0; }"
        "QScrollBar::handle:vertical { margin: 0 4px; }"
        "QScrollBar::handle:hover { background: palette(highlight); }"
        "QScrollBar::add-line, QScrollBar::sub-line {"
        " width: 0; height: 0; border: none; }"
        "QScrollBar::add-page, QScrollBar::sub-page {"
        " background: transparent; }"));
    bar->hide();
    return bar;
}

void FilmstripView::positionOverlayScrollBars()
{
    constexpr int inset = 4;
    constexpr int thickness = 14;
    const QRect viewportRect = viewport()->geometry();
    m_horizontalOverlay->setGeometry(
        viewportRect.left() + inset,
        viewportRect.bottom() - thickness - 1,
        std::max(0, viewportRect.width() - inset * 2),
        thickness);
    m_verticalOverlay->setGeometry(
        viewportRect.right() - thickness - 1,
        viewportRect.top() + inset,
        thickness,
        std::max(0, viewportRect.height() - inset * 2));
    m_horizontalOverlay->raise();
    m_verticalOverlay->raise();
}

void FilmstripView::syncOverlayScrollBars()
{
    QScrollBar *horizontal = horizontalScrollBar();
    QScrollBar *vertical = verticalScrollBar();
    {
        const QSignalBlocker blocker(m_horizontalOverlay);
        m_horizontalOverlay->setRange(
            horizontal->minimum(), horizontal->maximum());
        m_horizontalOverlay->setPageStep(horizontal->pageStep());
        m_horizontalOverlay->setValue(horizontal->value());
    }
    {
        const QSignalBlocker blocker(m_verticalOverlay);
        m_verticalOverlay->setRange(
            vertical->minimum(), vertical->maximum());
        m_verticalOverlay->setPageStep(vertical->pageStep());
        m_verticalOverlay->setValue(vertical->value());
    }
    m_horizontalOverlay->setVisible(
        !m_verticalLayout
        && horizontal->maximum() > horizontal->minimum());
    m_verticalOverlay->setVisible(
        m_verticalLayout
        && vertical->maximum() > vertical->minimum());
    m_horizontalOverlay->raise();
    m_verticalOverlay->raise();
}

int FilmstripView::closeButtonRowAt(
    const QPoint &position) const
{
    const QModelIndex index = indexAt(position);
    const auto *delegate = dynamic_cast<const FilmstripDelegate *>(
        itemDelegate());
    if (!index.isValid() || !delegate
        || !closeButtonsVisible()) {
        return -1;
    }
    QStyleOptionViewItem option;
    initViewItemOption(&option);
    option.rect = visualRect(index);
    option.decorationSize = iconSize();
    return delegate->closeButtonRect(option).contains(position)
        ? index.row() : -1;
}
