#include "interfacelayoutpage.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <functional>
#include <utility>

namespace {
[[maybe_unused]] constexpr const char *layoutTranslationSources[] = {
    QT_TRANSLATE_NOOP("InterfaceLayoutPage", "Toolbar"),
    QT_TRANSLATE_NOOP("InterfaceLayoutPage", "Viewer"),
    QT_TRANSLATE_NOOP("InterfaceLayoutPage", "Top"),
    QT_TRANSLATE_NOOP("InterfaceLayoutPage", "Bottom"),
    QT_TRANSLATE_NOOP("InterfaceLayoutPage", "Left"),
    QT_TRANSLATE_NOOP("InterfaceLayoutPage", "Right"),
    QT_TRANSLATE_NOOP("InterfaceLayoutPage", "Attached floating"),
    QT_TRANSLATE_NOOP("InterfaceLayoutPage", "Separate window"),
    QT_TRANSLATE_NOOP("InterfaceLayoutPage", "Attached"),
    QT_TRANSLATE_NOOP("InterfaceLayoutPage", "Window")
};

QString layoutTr(const char *source)
{
    return QCoreApplication::translate(
        "InterfaceLayoutPage", source);
}

QStringList normalizedPanelOrder(const QStringList &order)
{
    const QStringList defaults{
        QStringLiteral("thumbnails"),
        QStringLiteral("information"),
        QStringLiteral("colorPicker")};
    QStringList result;
    for (const QString &panelId : order) {
        if (defaults.contains(panelId) && !result.contains(panelId))
            result.append(panelId);
    }
    for (const QString &panelId : defaults) {
        if (!result.contains(panelId))
            result.append(panelId);
    }
    return result;
}

class LayoutPreview final : public QWidget
{
public:
    using DropCallback = std::function<void(
        const QString &, const QString &, int)>;

    explicit LayoutPreview(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setObjectName(QStringLiteral("interfaceLayoutPreview"));
        setMinimumSize(320, 300);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setMouseTracking(true);
        setToolTip(QCoreApplication::translate(
            "InterfaceLayoutPage",
            "Drag the toolbar or a panel to another edge. Drop a panel into the viewer to float it."));
    }

    void setState(const InterfaceLayoutState &state)
    {
        m_state = state;
        update();
    }

    void setDropCallback(DropCallback callback)
    {
        m_dropCallback = std::move(callback);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        m_panelRegions.clear();
        m_dockRegions.clear();
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        const QPalette colors = palette();
        QRectF frame = rect().adjusted(10, 10, -10, -10);
        painter.setPen(QPen(colors.color(QPalette::Mid), 1));
        painter.setBrush(colors.color(QPalette::Base));
        painter.drawRoundedRect(frame, 7, 7);

        QRectF viewer = frame.adjusted(9, 9, -9, -9);
        const qreal bar = 20.0;

        const auto take = [&viewer](const QString &position,
                                    qreal extent) {
            QRectF result;
            if (position == QStringLiteral("top")) {
                result = QRectF(viewer.left(), viewer.top(),
                                viewer.width(), extent);
                viewer.setTop(result.bottom() + 4);
            } else if (position == QStringLiteral("bottom")) {
                result = QRectF(viewer.left(), viewer.bottom() - extent,
                                viewer.width(), extent);
                viewer.setBottom(result.top() - 4);
            } else if (position == QStringLiteral("left")) {
                result = QRectF(viewer.left(), viewer.top(), extent,
                                viewer.height());
                viewer.setLeft(result.right() + 4);
            } else if (position == QStringLiteral("right")) {
                result = QRectF(viewer.right() - extent, viewer.top(),
                                extent, viewer.height());
                viewer.setRight(result.left() - 4);
            }
            return result;
        };

        const auto drawRegion = [&painter, &colors](
                                    const QRectF &region,
                                    const QColor &accent,
                                    const QString &label) {
            if (!region.isValid() || region.isEmpty())
                return;
            QColor fill = accent;
            fill.setAlpha(72);
            painter.setPen(QPen(accent, 1));
            painter.setBrush(fill);
            painter.drawRoundedRect(region, 4, 4);
            painter.setPen(colors.color(QPalette::Text));
            painter.drawText(region.adjusted(4, 2, -4, -2),
                             Qt::AlignCenter, label);
        };

        if (m_state.showMenuBar) {
            drawRegion(take(QStringLiteral("top"), bar),
                       colors.color(QPalette::Dark),
                       layoutTr("Menu bar"));
        }
        if (m_state.showStatusBar) {
            drawRegion(take(QStringLiteral("bottom"), bar),
                       colors.color(QPalette::Dark),
                       layoutTr("Status bar"));
        }

        if (m_state.showToolbar) {
            const QRectF toolbarRegion = take(
                m_state.toolbarPosition, bar);
            drawRegion(toolbarRegion,
                       colors.color(QPalette::Highlight),
                       layoutTr("Toolbar"));
            m_panelRegions.append(
                {QStringLiteral("toolbar"),
                 m_state.toolbarPosition, toolbarRegion});
            setProperty("clearveilPreviewRect_toolbar",
                        toolbarRegion.toRect());
        }

        struct PanelPreview {
            QString id;
            bool visible;
            QString placement;
            QString label;
            QColor color;
        };
        QList<PanelPreview> panels{
            {QStringLiteral("thumbnails"),
             m_state.showThumbnails, m_state.thumbnailsPlacement,
             layoutTr("Thumbnails"), QColor(70, 138, 230)},
            {QStringLiteral("information"),
             m_state.showInformation, m_state.informationPlacement,
             layoutTr("Information"), QColor(101, 178, 113)},
            {QStringLiteral("colorPicker"),
             m_state.showColorPicker, m_state.colorPickerPlacement,
             layoutTr("Color picker"), QColor(204, 132, 73)}
        };
        const QStringList order = normalizedPanelOrder(
            m_state.panelOrder);
        std::stable_sort(
            panels.begin(), panels.end(),
            [&order](const PanelPreview &left,
                     const PanelPreview &right) {
                return order.indexOf(left.id) < order.indexOf(right.id);
            });

        struct DockGroup {
            QString placement;
            QRectF region;
            QList<PanelPreview> panels;
        };
        QList<DockGroup> dockGroups;
        const QStringList dockOrder{
            QStringLiteral("top"), QStringLiteral("bottom"),
            QStringLiteral("left"), QStringLiteral("right")};
        for (const QString &position : dockOrder) {
            DockGroup group;
            group.placement = position;
            for (const PanelPreview &item : panels) {
                if (item.visible && item.placement == position)
                    group.panels.append(item);
            }
            if (group.panels.isEmpty())
                continue;
            const bool vertical = position == QStringLiteral("left")
                || position == QStringLiteral("right");
            const qreal extent = vertical
                ? std::clamp(frame.width() * 0.11, 48.0, 72.0)
                : std::clamp(frame.height() * 0.09, 34.0, 54.0);
            group.region = take(position, extent);
            dockGroups.append(group);
        }

        painter.setPen(QPen(colors.color(QPalette::Mid), 1,
                            Qt::DashLine));
        painter.setBrush(colors.color(QPalette::Window));
        painter.drawRoundedRect(viewer, 5, 5);
        painter.setPen(colors.color(QPalette::Mid));
        painter.drawText(viewer, Qt::AlignCenter, layoutTr("Viewer"));
        m_viewerRegion = viewer;

        // QMainWindow shares one strip between panels docked on the same
        // edge. Split that strip along its long axis instead of consuming a
        // second strip for every panel; the latter made the preview show
        // several misleading, adjacent vertical slivers.
        for (const DockGroup &group : std::as_const(dockGroups)) {
            const bool vertical = group.placement
                    == QStringLiteral("left")
                || group.placement == QStringLiteral("right");
            const qreal gap = 3.0;
            const int count = group.panels.size();
            const qreal available = (vertical
                    ? group.region.height() : group.region.width())
                - gap * std::max(0, count - 1);
            const qreal itemExtent = available / count;
            for (int index = 0; index < count; ++index) {
                QRectF itemRegion = group.region;
                if (vertical) {
                    itemRegion.setTop(
                        group.region.top() + index * (itemExtent + gap));
                    itemRegion.setHeight(itemExtent);
                } else {
                    itemRegion.setLeft(
                        group.region.left() + index * (itemExtent + gap));
                    itemRegion.setWidth(itemExtent);
                }
                drawRegion(itemRegion, group.panels.at(index).color,
                           group.panels.at(index).label);
                m_panelRegions.append(
                    {group.panels.at(index).id,
                     group.placement, itemRegion});
                const QByteArray propertyName =
                    QByteArrayLiteral("clearveilPreviewRect_")
                    + group.panels.at(index).id.toUtf8();
                setProperty(propertyName.constData(),
                            itemRegion.toRect());
            }
            QStringList groupIds;
            for (const PanelPreview &item : group.panels)
                groupIds.append(item.id);
            m_dockRegions.append(
                {group.placement, group.region, groupIds});
        }

        // Attached and separate floating panels sit above the viewer. Draw
        // them last so the viewer background cannot cover them.
        int floatingIndex = 0;
        for (const PanelPreview &item : panels) {
            if (!item.visible
                || (item.placement != QStringLiteral("overlay")
                    && item.placement != QStringLiteral("floating"))) {
                continue;
            }
            const qreal width = std::min<qreal>(
                125, std::max<qreal>(92, frame.width() * 0.28));
            const QRectF floating(
                viewer.left() + 8 + floatingIndex * 14,
                viewer.top() + 8 + floatingIndex * 30,
                width, 46);
            drawRegion(
                floating, item.color,
                item.label + QStringLiteral(" · ")
                    + layoutTr(item.placement
                                       == QStringLiteral("overlay")
                                   ? "Attached" : "Window"));
            m_panelRegions.append(
                {item.id, item.placement, floating});
            const QByteArray propertyName =
                QByteArrayLiteral("clearveilPreviewRect_")
                + item.id.toUtf8();
            setProperty(propertyName.constData(), floating.toRect());
            ++floatingIndex;
        }

        if (m_dragging) {
            const DropTarget target = dropTargetAt(m_dragPosition);
            if (!target.placement.isEmpty()) {
                QColor guide = colors.color(QPalette::Highlight);
                guide.setAlpha(62);
                painter.setBrush(guide);
                painter.setPen(QPen(
                    colors.color(QPalette::Highlight), 2,
                    Qt::DashLine));
                painter.drawRoundedRect(target.highlight, 5, 5);
                if (!target.insertionLine.isNull()) {
                    painter.setPen(QPen(
                        colors.color(QPalette::HighlightedText), 4,
                        Qt::SolidLine, Qt::RoundCap));
                    painter.drawLine(target.insertionLine);
                }
            }
            const auto dragged = std::find_if(
                panels.cbegin(), panels.cend(),
                [this](const PanelPreview &item) {
                    return item.id == m_draggedPanelId;
                });
            if (dragged != panels.cend()) {
                const QRectF ghost(
                    m_dragPosition.x() - 58,
                    m_dragPosition.y() - 20, 116, 40);
                drawRegion(ghost, dragged->color, dragged->label);
            } else if (m_draggedPanelId
                       == QStringLiteral("toolbar")) {
                const QRectF ghost(
                    m_dragPosition.x() - 68,
                    m_dragPosition.y() - 16, 136, 32);
                drawRegion(ghost,
                           colors.color(QPalette::Highlight),
                           layoutTr("Toolbar"));
            }
        }
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            for (auto it = m_panelRegions.crbegin();
                 it != m_panelRegions.crend(); ++it) {
                if (!it->rect.contains(event->position()))
                    continue;
                m_draggedPanelId = it->id;
                m_dragStart = event->position();
                m_dragPosition = event->position();
                event->accept();
                return;
            }
        }
        QWidget::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (!m_draggedPanelId.isEmpty()) {
            m_dragPosition = event->position();
            if (!m_dragging
                && (m_dragPosition - m_dragStart).manhattanLength()
                    >= QApplication::startDragDistance()) {
                m_dragging = true;
                setCursor(Qt::ClosedHandCursor);
            }
            if (m_dragging)
                update();
            event->accept();
            return;
        }
        const bool overPanel = std::any_of(
            m_panelRegions.cbegin(), m_panelRegions.cend(),
            [event](const PanelRegion &region) {
                return region.rect.contains(event->position());
            });
        setCursor(overPanel ? Qt::OpenHandCursor : Qt::ArrowCursor);
        QWidget::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton
            && !m_draggedPanelId.isEmpty()) {
            if (m_dragging && m_dropCallback) {
                const DropTarget target =
                    dropTargetAt(event->position());
                if (!target.placement.isEmpty()) {
                    m_dropCallback(m_draggedPanelId,
                                   target.placement,
                                   target.index);
                }
            }
            m_draggedPanelId.clear();
            m_dragging = false;
            setCursor(Qt::ArrowCursor);
            update();
            event->accept();
            return;
        }
        QWidget::mouseReleaseEvent(event);
    }

private:
    struct PanelRegion {
        QString id;
        QString placement;
        QRectF rect;
    };

    struct DockRegion {
        QString placement;
        QRectF rect;
        QStringList panelIds;
    };

    struct DropTarget {
        QString placement;
        int index = 0;
        QRectF highlight;
        QLineF insertionLine;
    };

    DropTarget dropTargetAt(const QPointF &position) const
    {
        for (const DockRegion &dock : m_dockRegions) {
            if (!dock.rect.contains(position))
                continue;
            const bool vertical = dock.placement
                    == QStringLiteral("left")
                || dock.placement == QStringLiteral("right");
            QList<PanelRegion> siblings;
            for (const PanelRegion &panel : m_panelRegions) {
                if (panel.placement == dock.placement
                    && panel.id != m_draggedPanelId) {
                    siblings.append(panel);
                }
            }
            int index = 0;
            for (const PanelRegion &sibling : siblings) {
                const qreal coordinate = vertical
                    ? position.y() : position.x();
                const qreal center = vertical
                    ? sibling.rect.center().y()
                    : sibling.rect.center().x();
                if (coordinate >= center)
                    ++index;
            }
            QLineF insertion;
            if (vertical) {
                const qreal y = siblings.isEmpty()
                    ? dock.rect.center().y()
                    : index == 0
                        ? siblings.first().rect.top()
                        : index >= siblings.size()
                            ? siblings.last().rect.bottom()
                            : siblings.at(index).rect.top();
                insertion = QLineF(
                    dock.rect.left() + 4, y,
                    dock.rect.right() - 4, y);
            } else {
                const qreal x = siblings.isEmpty()
                    ? dock.rect.center().x()
                    : index == 0
                        ? siblings.first().rect.left()
                        : index >= siblings.size()
                            ? siblings.last().rect.right()
                            : siblings.at(index).rect.left();
                insertion = QLineF(
                    x, dock.rect.top() + 4,
                    x, dock.rect.bottom() - 4);
            }
            return {dock.placement, index, dock.rect, insertion};
        }

        if (!m_viewerRegion.contains(position))
            return {};
        const qreal edgeExtent = std::clamp(
            std::min(m_viewerRegion.width(), m_viewerRegion.height())
                * 0.2,
            36.0, 72.0);
        const qreal left = position.x() - m_viewerRegion.left();
        const qreal right = m_viewerRegion.right() - position.x();
        const qreal top = position.y() - m_viewerRegion.top();
        const qreal bottom = m_viewerRegion.bottom() - position.y();
        const qreal closest = std::min({left, right, top, bottom});
        QString placement = QStringLiteral("overlay");
        QRectF highlight = m_viewerRegion.adjusted(
            edgeExtent, edgeExtent, -edgeExtent, -edgeExtent);
        if (closest <= edgeExtent
            || m_draggedPanelId == QStringLiteral("toolbar")) {
            if (closest == left) {
                placement = QStringLiteral("left");
                highlight = QRectF(
                    m_viewerRegion.left(), m_viewerRegion.top(),
                    edgeExtent, m_viewerRegion.height());
            } else if (closest == right) {
                placement = QStringLiteral("right");
                highlight = QRectF(
                    m_viewerRegion.right() - edgeExtent,
                    m_viewerRegion.top(), edgeExtent,
                    m_viewerRegion.height());
            } else if (closest == top) {
                placement = QStringLiteral("top");
                highlight = QRectF(
                    m_viewerRegion.left(), m_viewerRegion.top(),
                    m_viewerRegion.width(), edgeExtent);
            } else {
                placement = QStringLiteral("bottom");
                highlight = QRectF(
                    m_viewerRegion.left(),
                    m_viewerRegion.bottom() - edgeExtent,
                    m_viewerRegion.width(), edgeExtent);
            }
        }
        int index = 0;
        for (const PanelRegion &panel : m_panelRegions) {
            if (panel.placement == placement
                && panel.id != m_draggedPanelId) {
                ++index;
            }
        }
        return {placement, index, highlight, {}};
    }

    InterfaceLayoutState m_state;
    DropCallback m_dropCallback;
    QList<PanelRegion> m_panelRegions;
    QList<DockRegion> m_dockRegions;
    QRectF m_viewerRegion;
    QString m_draggedPanelId;
    QPointF m_dragStart;
    QPointF m_dragPosition;
    bool m_dragging = false;
};

void addPositionItems(QComboBox *combo, bool allowFloating)
{
    combo->addItem(layoutTr("Top"), QStringLiteral("top"));
    combo->addItem(layoutTr("Bottom"), QStringLiteral("bottom"));
    combo->addItem(layoutTr("Left"), QStringLiteral("left"));
    combo->addItem(layoutTr("Right"), QStringLiteral("right"));
    if (allowFloating) {
        combo->addItem(layoutTr("Attached floating"),
                       QStringLiteral("overlay"));
        combo->addItem(layoutTr("Separate window"),
                       QStringLiteral("floating"));
    }
}

void selectData(QComboBox *combo, const QString &value,
                const QString &fallback)
{
    int index = combo->findData(value);
    if (index < 0)
        index = combo->findData(fallback);
    combo->setCurrentIndex(std::max(0, index));
}
}

InterfaceLayoutPage::InterfaceLayoutPage(
    const InterfaceLayoutState &initialState, QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("interfaceLayoutPage"));
    auto *root = new QVBoxLayout(this);

    auto *hint = new QLabel(
        tr("Drag the toolbar and panels in the preview to change their edge, floating state, or order. Use Apply to preview changes without closing Preferences."),
        this);
    hint->setWordWrap(true);
    root->addWidget(hint);

    auto *body = new QHBoxLayout;
    body->setSpacing(14);
    auto *layoutPreview = new LayoutPreview(this);
    layoutPreview->setDropCallback(
        [this](const QString &panelId,
               const QString &placement, int placementIndex) {
            applyPreviewDrop(panelId, placement, placementIndex);
        });
    m_preview = layoutPreview;
    body->addWidget(m_preview, 1);

    auto *controlsWidget = new QWidget(this);
    auto *controls = new QVBoxLayout(controlsWidget);
    controls->setContentsMargins(0, 0, 0, 0);
    controls->setSpacing(10);
    body->addWidget(controlsWidget, 1);
    root->addLayout(body, 1);

    auto *chromeGroup = new QGroupBox(tr("Window chrome"), this);
    auto *chrome = new QGridLayout(chromeGroup);
    m_showMenuBar = new QCheckBox(tr("Menu bar"), chromeGroup);
    m_showMenuBar->setObjectName(QStringLiteral("layoutShowMenuBar"));
    m_showToolbar = new QCheckBox(tr("Main toolbar"), chromeGroup);
    m_showToolbar->setObjectName(QStringLiteral("layoutShowToolbar"));
    m_showStatusBar = new QCheckBox(tr("Status bar"), chromeGroup);
    m_showStatusBar->setObjectName(QStringLiteral("layoutShowStatusBar"));
    m_showThumbnails = new QCheckBox(tr("Thumbnails"), chromeGroup);
    m_showThumbnails->setAccessibleName(tr("Show thumbnails panel"));
    m_showThumbnails->setObjectName(
        QStringLiteral("thumbnailsPanelVisibleCheckBox"));
    m_showInformation = new QCheckBox(tr("Information"), chromeGroup);
    m_showInformation->setAccessibleName(tr("Show information panel"));
    m_showInformation->setObjectName(
        QStringLiteral("informationPanelVisibleCheckBox"));
    m_showColorPicker = new QCheckBox(tr("Color picker"), chromeGroup);
    m_showColorPicker->setAccessibleName(tr("Show color picker panel"));
    m_showColorPicker->setObjectName(
        QStringLiteral("colorPickerPanelVisibleCheckBox"));
    m_layoutLocked = new QCheckBox(tr("Lock panel layout"), chromeGroup);
    m_layoutLocked->setObjectName(QStringLiteral("layoutLocked"));
    m_toolbarPosition = new QComboBox(controlsWidget);
    m_toolbarPosition->setObjectName(QStringLiteral("toolbarPosition"));
    addPositionItems(m_toolbarPosition, false);
    chrome->addWidget(m_showMenuBar, 0, 0);
    chrome->addWidget(m_showToolbar, 0, 1);
    chrome->addWidget(m_showStatusBar, 1, 0);
    chrome->addWidget(m_showThumbnails, 1, 1);
    chrome->addWidget(m_showInformation, 2, 0);
    chrome->addWidget(m_showColorPicker, 2, 1);
    chrome->addWidget(m_layoutLocked, 3, 0, 1, 2);
    controls->addWidget(chromeGroup);

    const auto makePlacement = [controlsWidget](const QString &name) {
        auto *control = new QComboBox(controlsWidget);
        control->setAccessibleName(name);
        addPositionItems(control, true);
        return control;
    };

    m_thumbnailsPlacement = makePlacement(
        tr("Thumbnails panel placement"));
    m_thumbnailsPlacement->setObjectName(
        QStringLiteral("thumbnailsPanelPlacement"));
    m_floatingThumbnailLayout = new QComboBox(controlsWidget);
    m_floatingThumbnailLayout->setObjectName(
        QStringLiteral("floatingThumbnailLayout"));
    m_floatingThumbnailLayout->addItem(
        tr("Automatic"), QStringLiteral("auto"));
    m_floatingThumbnailLayout->addItem(
        tr("Horizontal"), QStringLiteral("horizontal"));
    m_floatingThumbnailLayout->addItem(
        tr("Vertical"), QStringLiteral("vertical"));

    m_informationPlacement = makePlacement(
        tr("Information panel placement"));
    m_informationPlacement->setObjectName(
        QStringLiteral("informationPanelPlacement"));

    m_colorPickerPlacement = makePlacement(
        tr("Color picker panel placement"));
    m_colorPickerPlacement->setObjectName(
        QStringLiteral("colorPickerPanelPlacement"));

    auto *preciseToggle = new QToolButton(controlsWidget);
    preciseToggle->setObjectName(
        QStringLiteral("preciseLayoutSettingsToggle"));
    preciseToggle->setCheckable(true);
    preciseToggle->setArrowType(Qt::RightArrow);
    preciseToggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    preciseToggle->setText(tr("Precise position settings"));
    auto *preciseSettings = new QWidget(controlsWidget);
    preciseSettings->setObjectName(
        QStringLiteral("preciseLayoutSettings"));
    auto *preciseForm = new QFormLayout(preciseSettings);
    preciseForm->setContentsMargins(12, 0, 0, 0);
    m_toolbarPosition->setParent(preciseSettings);
    m_thumbnailsPlacement->setParent(preciseSettings);
    m_informationPlacement->setParent(preciseSettings);
    m_colorPickerPlacement->setParent(preciseSettings);
    m_floatingThumbnailLayout->setParent(preciseSettings);
    preciseForm->addRow(tr("Toolbar position"), m_toolbarPosition);
    preciseForm->addRow(tr("Thumbnails position"),
                        m_thumbnailsPlacement);
    preciseForm->addRow(tr("Information position"),
                        m_informationPlacement);
    preciseForm->addRow(tr("Color picker position"),
                        m_colorPickerPlacement);
    preciseForm->addRow(tr("Floating thumbnail layout"),
                        m_floatingThumbnailLayout);
    preciseSettings->hide();
    connect(preciseToggle, &QToolButton::toggled,
            this, [preciseToggle, preciseSettings](bool expanded) {
        preciseToggle->setArrowType(
            expanded ? Qt::DownArrow : Qt::RightArrow);
        preciseSettings->setVisible(expanded);
    });
    controls->addWidget(preciseToggle);
    controls->addWidget(preciseSettings);

    auto *fullscreenGroup = new QGroupBox(
        tr("Full screen"), this);
    auto *fullscreen = new QGridLayout(fullscreenGroup);
    m_showToolbarInFullscreen = new QCheckBox(
        tr("Show toolbar"), fullscreenGroup);
    m_showToolbarInFullscreen->setObjectName(
        QStringLiteral("layoutFullscreenToolbar"));
    m_showThumbnailsInFullscreen = new QCheckBox(
        tr("Show thumbnails"), fullscreenGroup);
    m_showThumbnailsInFullscreen->setObjectName(
        QStringLiteral("layoutFullscreenThumbnails"));
    m_showStatusBarInFullscreen = new QCheckBox(
        tr("Show status bar"), fullscreenGroup);
    m_showStatusBarInFullscreen->setObjectName(
        QStringLiteral("layoutFullscreenStatusBar"));
    m_showInformationInFullscreen = new QCheckBox(
        tr("Show information"), fullscreenGroup);
    m_showInformationInFullscreen->setObjectName(
        QStringLiteral("layoutFullscreenInformation"));
    fullscreen->addWidget(m_showToolbarInFullscreen, 0, 0);
    fullscreen->addWidget(m_showThumbnailsInFullscreen, 0, 1);
    fullscreen->addWidget(m_showStatusBarInFullscreen, 1, 0);
    fullscreen->addWidget(m_showInformationInFullscreen, 1, 1);
    controls->addWidget(fullscreenGroup);

    auto *reset = new QPushButton(tr("Reset interface layout"), this);
    reset->setObjectName(QStringLiteral("resetInterfaceLayoutButton"));
    connect(reset, &QPushButton::clicked, this, [this] {
        setState(InterfaceLayoutState{});
    });
    controls->addWidget(reset, 0, Qt::AlignRight);
    controls->addStretch();

    const QList<QCheckBox *> checks{
        m_showMenuBar, m_showToolbar, m_showStatusBar, m_layoutLocked,
        m_showThumbnails, m_showInformation, m_showColorPicker,
        m_showToolbarInFullscreen, m_showThumbnailsInFullscreen,
        m_showStatusBarInFullscreen, m_showInformationInFullscreen
    };
    for (QCheckBox *check : checks) {
        connect(check, &QCheckBox::toggled,
                this, &InterfaceLayoutPage::updatePreview);
    }
    const QList<QComboBox *> combos{
        m_toolbarPosition, m_thumbnailsPlacement,
        m_floatingThumbnailLayout, m_informationPlacement,
        m_colorPickerPlacement
    };
    for (QComboBox *combo : combos) {
        connect(combo, &QComboBox::currentIndexChanged,
                this, &InterfaceLayoutPage::updatePreview);
    }
    connect(m_thumbnailsPlacement, &QComboBox::currentIndexChanged,
            this, &InterfaceLayoutPage::updateControlAvailability);
    connect(m_showThumbnails, &QCheckBox::toggled,
            this, &InterfaceLayoutPage::updateControlAvailability);

    setState(initialState);
}

InterfaceLayoutState InterfaceLayoutPage::state() const
{
    InterfaceLayoutState result;
    result.showMenuBar = m_showMenuBar->isChecked();
    result.showToolbar = m_showToolbar->isChecked();
    result.showStatusBar = m_showStatusBar->isChecked();
    result.layoutLocked = m_layoutLocked->isChecked();
    result.showToolbarInFullscreen =
        m_showToolbarInFullscreen->isChecked();
    result.showThumbnailsInFullscreen =
        m_showThumbnailsInFullscreen->isChecked();
    result.showStatusBarInFullscreen =
        m_showStatusBarInFullscreen->isChecked();
    result.showInformationInFullscreen =
        m_showInformationInFullscreen->isChecked();
    result.toolbarPosition = m_toolbarPosition->currentData().toString();
    result.showThumbnails = m_showThumbnails->isChecked();
    result.thumbnailsPlacement =
        m_thumbnailsPlacement->currentData().toString();
    result.floatingThumbnailLayout =
        m_floatingThumbnailLayout->currentData().toString();
    result.showInformation = m_showInformation->isChecked();
    result.informationPlacement =
        m_informationPlacement->currentData().toString();
    result.showColorPicker = m_showColorPicker->isChecked();
    result.colorPickerPlacement =
        m_colorPickerPlacement->currentData().toString();
    result.panelOrder = normalizedPanelOrder(m_panelOrder);
    return result;
}

void InterfaceLayoutPage::setState(
    const InterfaceLayoutState &value)
{
    m_showMenuBar->setChecked(value.showMenuBar);
    m_showToolbar->setChecked(value.showToolbar);
    m_showStatusBar->setChecked(value.showStatusBar);
    m_layoutLocked->setChecked(value.layoutLocked);
    m_showToolbarInFullscreen->setChecked(
        value.showToolbarInFullscreen);
    m_showThumbnailsInFullscreen->setChecked(
        value.showThumbnailsInFullscreen);
    m_showStatusBarInFullscreen->setChecked(
        value.showStatusBarInFullscreen);
    m_showInformationInFullscreen->setChecked(
        value.showInformationInFullscreen);
    selectData(m_toolbarPosition, value.toolbarPosition,
               QStringLiteral("top"));
    m_showThumbnails->setChecked(value.showThumbnails);
    selectData(m_thumbnailsPlacement, value.thumbnailsPlacement,
               QStringLiteral("bottom"));
    selectData(m_floatingThumbnailLayout,
               value.floatingThumbnailLayout,
               QStringLiteral("auto"));
    m_showInformation->setChecked(value.showInformation);
    selectData(m_informationPlacement, value.informationPlacement,
               QStringLiteral("right"));
    m_showColorPicker->setChecked(value.showColorPicker);
    selectData(m_colorPickerPlacement, value.colorPickerPlacement,
               QStringLiteral("overlay"));
    m_panelOrder = normalizedPanelOrder(value.panelOrder);
    updateControlAvailability();
    updatePreview();
}

void InterfaceLayoutPage::applyPreviewDrop(
    const QString &panelId, const QString &placement,
    int placementIndex)
{
    if (panelId == QStringLiteral("toolbar")) {
        const int index = m_toolbarPosition->findData(placement);
        if (index >= 0) {
            m_toolbarPosition->setCurrentIndex(index);
            updatePreview();
        }
        return;
    }

    QComboBox *placementControl = nullptr;
    if (panelId == QStringLiteral("thumbnails"))
        placementControl = m_thumbnailsPlacement;
    else if (panelId == QStringLiteral("information"))
        placementControl = m_informationPlacement;
    else if (panelId == QStringLiteral("colorPicker"))
        placementControl = m_colorPickerPlacement;
    if (!placementControl)
        return;

    const int placementControlIndex =
        placementControl->findData(placement);
    if (placementControlIndex < 0)
        return;

    m_panelOrder = normalizedPanelOrder(m_panelOrder);
    const int originalIndex = m_panelOrder.indexOf(panelId);
    m_panelOrder.removeAll(panelId);
    placementControl->setCurrentIndex(placementControlIndex);

    const auto placementForPanel = [this](const QString &id) {
        if (id == QStringLiteral("thumbnails"))
            return m_thumbnailsPlacement->currentData().toString();
        if (id == QStringLiteral("information"))
            return m_informationPlacement->currentData().toString();
        if (id == QStringLiteral("colorPicker"))
            return m_colorPickerPlacement->currentData().toString();
        return QString();
    };
    QStringList siblings;
    for (const QString &id : std::as_const(m_panelOrder)) {
        if (placementForPanel(id) == placement)
            siblings.append(id);
    }

    if (siblings.isEmpty()
        || placement == QStringLiteral("overlay")
        || placement == QStringLiteral("floating")) {
        m_panelOrder.insert(
            std::clamp(originalIndex, 0,
                       static_cast<int>(m_panelOrder.size())),
            panelId);
    } else {
        placementIndex = std::clamp(
            placementIndex, 0,
            static_cast<int>(siblings.size()));
        if (placementIndex < static_cast<int>(siblings.size())) {
            m_panelOrder.insert(
                m_panelOrder.indexOf(siblings.at(placementIndex)),
                panelId);
        } else {
            m_panelOrder.insert(
                m_panelOrder.indexOf(siblings.last()) + 1,
                panelId);
        }
    }
    updatePreview();
}

void InterfaceLayoutPage::updatePreview()
{
    updateControlAvailability();
    if (auto *preview = dynamic_cast<LayoutPreview *>(m_preview))
        preview->setState(state());
}

void InterfaceLayoutPage::updateControlAvailability()
{
    m_toolbarPosition->setEnabled(m_showToolbar->isChecked());
    m_thumbnailsPlacement->setEnabled(m_showThumbnails->isChecked());
    m_informationPlacement->setEnabled(m_showInformation->isChecked());
    m_colorPickerPlacement->setEnabled(m_showColorPicker->isChecked());
    m_floatingThumbnailLayout->setEnabled(
        m_showThumbnails->isChecked()
        && (m_thumbnailsPlacement->currentData().toString()
                == QStringLiteral("overlay")
            || m_thumbnailsPlacement->currentData().toString()
                == QStringLiteral("floating")));
}
