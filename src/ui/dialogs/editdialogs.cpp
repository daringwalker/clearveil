#include "editdialogs.h"

#include "clearveilicon.h"
#include "imagedocument.h"
#include "persistentthumbnailcache.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHash>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QKeySequenceEdit>
#include <QKeyEvent>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QSet>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>

#include <algorithm>
#include <utility>

class CropPreviewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CropPreviewWidget(const QImage &image, QWidget *parent = nullptr)
        : QWidget(parent)
        , m_image(image)
        , m_selection(image.rect())
    {
        setMinimumSize(420, 280);
        setObjectName(QStringLiteral("cropPreview"));
        setAccessibleName(tr("Crop selection preview"));
        setAccessibleDescription(
            tr("Drag outside the selection to create it, drag inside to move it, or drag a handle to resize it."));
        setFocusPolicy(Qt::StrongFocus);
        setMouseTracking(true);
    }

    QRect selection() const { return m_selection; }

    void setSelection(const QRect &selection)
    {
        const QRect valid = selection.normalized().intersected(m_image.rect());
        if (!valid.isValid() || valid == m_selection)
            return;
        m_selection = valid;
        m_userAdjusted = true;
        update();
        emit selectionChanged(m_selection);
    }

    void setAspectRatio(qreal ratio)
    {
        m_aspectRatio = ratio;
        if (ratio <= 0.0 || m_selection.isEmpty())
            return;

        int width = m_selection.width();
        int height = std::max(1, qRound(width / ratio));
        if (height > m_image.height()) {
            height = m_image.height();
            width = std::max(1, qRound(height * ratio));
        }
        width = std::min(width, m_image.width());
        QRect adjusted(QPoint(), QSize(width, height));
        adjusted.moveCenter(m_selection.center());
        moveInsideImage(adjusted);
        setSelection(adjusted);
    }

signals:
    void selectionChanged(const QRect &selection);

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.fillRect(rect(), palette().color(QPalette::Window));
        const QRectF target = imageTarget();
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.drawImage(target, m_image);

        const QRectF selected = imageToWidget(m_selection);
        QPainterPath outside;
        outside.addRect(target);
        QPainterPath inside;
        inside.addRect(selected);
        painter.fillPath(outside.subtracted(inside), QColor(0, 0, 0, 145));
        painter.setPen(QPen(palette().color(QPalette::Highlight), 2));
        painter.drawRect(selected);

        painter.setPen(QPen(QColor(255, 255, 255, 180), 1, Qt::DashLine));
        painter.drawLine(QPointF(selected.left() + selected.width() / 3.0, selected.top()),
                         QPointF(selected.left() + selected.width() / 3.0, selected.bottom()));
        painter.drawLine(QPointF(selected.left() + selected.width() * 2.0 / 3.0, selected.top()),
                         QPointF(selected.left() + selected.width() * 2.0 / 3.0, selected.bottom()));
        painter.drawLine(QPointF(selected.left(), selected.top() + selected.height() / 3.0),
                         QPointF(selected.right(), selected.top() + selected.height() / 3.0));
        painter.drawLine(QPointF(selected.left(), selected.top() + selected.height() * 2.0 / 3.0),
                         QPointF(selected.right(), selected.top() + selected.height() * 2.0 / 3.0));

        painter.setPen(QPen(palette().color(QPalette::Highlight), 2));
        painter.setBrush(palette().color(QPalette::Base));
        for (const QPointF &point : handleCenters(selected))
            painter.drawRect(handleRectangle(point));
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() != Qt::LeftButton)
            return;
        setFocus(Qt::MouseFocusReason);

        m_dragMode = hitTest(event->position());
        if (m_dragMode == DragMode::None)
            return;

        const QPoint imagePoint = boundedImagePoint(event->position());
        if (!m_image.rect().contains(imagePoint))
            return;

        if (m_dragMode == DragMode::Move
            && m_selection == m_image.rect()
            && !m_userAdjusted) {
            m_dragMode = DragMode::NewSelection;
        }
        m_dragStart = imagePoint;
        m_selectionAtDragStart = m_selection;
        m_dragging = true;
        updateCursor(event->position());
        event->accept();
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (!m_dragging) {
            updateCursor(event->position());
            event->accept();
            return;
        }

        const QPoint current = boundedImagePoint(event->position());
        QRect selection = m_selectionAtDragStart;
        switch (m_dragMode) {
        case DragMode::NewSelection:
            selection = aspectRectangle(
                m_dragStart, current, m_dragMode);
            break;
        case DragMode::Move:
            selection.translate(current - m_dragStart);
            moveInsideImage(selection);
            break;
        case DragMode::Left:
            selection.setLeft(
                std::min(current.x(), selection.right()));
            selection = aspectRectangle(
                selection.bottomRight(), selection.topLeft(),
                m_dragMode);
            break;
        case DragMode::Top:
            selection.setTop(
                std::min(current.y(), selection.bottom()));
            selection = aspectRectangle(
                selection.bottomRight(), selection.topLeft(),
                m_dragMode);
            break;
        case DragMode::Right:
            selection.setRight(
                std::max(current.x(), selection.left()));
            selection = aspectRectangle(
                selection.topLeft(), selection.bottomRight(),
                m_dragMode);
            break;
        case DragMode::Bottom:
            selection.setBottom(
                std::max(current.y(), selection.top()));
            selection = aspectRectangle(
                selection.topLeft(), selection.bottomRight(),
                m_dragMode);
            break;
        case DragMode::TopLeft:
            selection = aspectRectangle(
                m_selectionAtDragStart.bottomRight(), current,
                m_dragMode);
            break;
        case DragMode::TopRight:
            selection = aspectRectangle(
                m_selectionAtDragStart.bottomLeft(), current,
                m_dragMode);
            break;
        case DragMode::BottomRight:
            selection = aspectRectangle(
                m_selectionAtDragStart.topLeft(), current,
                m_dragMode);
            break;
        case DragMode::BottomLeft:
            selection = aspectRectangle(
                m_selectionAtDragStart.topRight(), current,
                m_dragMode);
            break;
        case DragMode::None:
            return;
        }
        setSelection(selection.intersected(m_image.rect()));
        m_userAdjusted = true;
        event->accept();
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            m_dragging = false;
            m_dragMode = DragMode::None;
            updateCursor(event->position());
            event->accept();
        }
    }

    void leaveEvent(QEvent *event) override
    {
        if (!m_dragging)
            unsetCursor();
        QWidget::leaveEvent(event);
    }

    void keyPressEvent(QKeyEvent *event) override
    {
        QPoint delta;
        const int amount =
            event->modifiers().testFlag(Qt::ShiftModifier) ? 10 : 1;
        if (event->key() == Qt::Key_Left)
            delta.setX(-amount);
        else if (event->key() == Qt::Key_Right)
            delta.setX(amount);
        else if (event->key() == Qt::Key_Up)
            delta.setY(-amount);
        else if (event->key() == Qt::Key_Down)
            delta.setY(amount);
        else {
            QWidget::keyPressEvent(event);
            return;
        }
        QRect moved = m_selection.translated(delta);
        moveInsideImage(moved);
        setSelection(moved);
        m_userAdjusted = true;
        event->accept();
    }

private:
    enum class DragMode {
        None,
        NewSelection,
        Move,
        Left,
        Top,
        Right,
        Bottom,
        TopLeft,
        TopRight,
        BottomRight,
        BottomLeft,
    };

    QRectF imageTarget() const
    {
        constexpr qreal controlMargin = 7.0;
        const QRectF available =
            QRectF(rect()).adjusted(controlMargin, controlMargin,
                                    -controlMargin, -controlMargin);
        const QSizeF size = QSizeF(m_image.size()).scaled(
            available.size(), Qt::KeepAspectRatio);
        return QRectF(
            QPointF(available.center().x() - size.width() / 2.0,
                    available.center().y() - size.height() / 2.0),
            size);
    }

    QRectF imageToWidget(const QRect &rectangle) const
    {
        const QRectF target = imageTarget();
        const qreal scale = target.width() / m_image.width();
        return QRectF(target.topLeft() + QPointF(rectangle.x() * scale,
                                                 rectangle.y() * scale),
                      QSizeF(rectangle.width() * scale, rectangle.height() * scale));
    }

    QPoint widgetToImage(const QPointF &point) const
    {
        const QRectF target = imageTarget();
        const qreal scale = target.width() / m_image.width();
        return QPoint(qFloor((point.x() - target.left()) / scale),
                      qFloor((point.y() - target.top()) / scale));
    }

    QPoint boundedImagePoint(const QPointF &point) const
    {
        QPoint result = widgetToImage(point);
        result.setX(std::clamp(
            result.x(), 0, std::max(0, m_image.width() - 1)));
        result.setY(std::clamp(
            result.y(), 0, std::max(0, m_image.height() - 1)));
        return result;
    }

    static QList<QPointF> handleCenters(const QRectF &rectangle)
    {
        const qreal centerX = rectangle.center().x();
        const qreal centerY = rectangle.center().y();
        return {
            rectangle.topLeft(),
            QPointF(centerX, rectangle.top()),
            rectangle.topRight(),
            QPointF(rectangle.right(), centerY),
            rectangle.bottomRight(),
            QPointF(centerX, rectangle.bottom()),
            rectangle.bottomLeft(),
            QPointF(rectangle.left(), centerY),
        };
    }

    static QRectF handleRectangle(const QPointF &center)
    {
        constexpr qreal handleSize = 10.0;
        return QRectF(
            center.x() - handleSize / 2.0,
            center.y() - handleSize / 2.0,
            handleSize, handleSize);
    }

    DragMode hitTest(const QPointF &position) const
    {
        const QRectF selected = imageToWidget(m_selection);
        const QList<QPointF> centers = handleCenters(selected);
        const QList<DragMode> modes{
            DragMode::TopLeft, DragMode::Top,
            DragMode::TopRight, DragMode::Right,
            DragMode::BottomRight, DragMode::Bottom,
            DragMode::BottomLeft, DragMode::Left,
        };
        for (qsizetype index = 0;
             index < centers.size(); ++index) {
            if (handleRectangle(centers.at(index))
                    .adjusted(-3, -3, 3, 3)
                    .contains(position)) {
                return modes.at(index);
            }
        }
        if (selected.contains(position))
            return DragMode::Move;
        if (imageTarget().contains(position))
            return DragMode::NewSelection;
        return DragMode::None;
    }

    void updateCursor(const QPointF &position)
    {
        if (m_dragging && m_dragMode == DragMode::Move) {
            setCursor(Qt::ClosedHandCursor);
            return;
        }
        const DragMode mode =
            m_dragging ? m_dragMode : hitTest(position);
        switch (mode) {
        case DragMode::TopLeft:
        case DragMode::BottomRight:
            setCursor(Qt::SizeFDiagCursor);
            break;
        case DragMode::TopRight:
        case DragMode::BottomLeft:
            setCursor(Qt::SizeBDiagCursor);
            break;
        case DragMode::Left:
        case DragMode::Right:
            setCursor(Qt::SizeHorCursor);
            break;
        case DragMode::Top:
        case DragMode::Bottom:
            setCursor(Qt::SizeVerCursor);
            break;
        case DragMode::Move:
            setCursor(Qt::OpenHandCursor);
            break;
        case DragMode::NewSelection:
            setCursor(Qt::CrossCursor);
            break;
        case DragMode::None:
            unsetCursor();
            break;
        }
    }

    QRect aspectRectangle(
        const QPoint &anchor, const QPoint &moving,
        DragMode mode) const
    {
        QRect result(anchor, moving);
        result = result.normalized();
        if (m_aspectRatio <= 0.0)
            return result;

        int width = std::max(1, result.width());
        int height = std::max(1, result.height());
        const bool verticalPrimary =
            mode == DragMode::Top || mode == DragMode::Bottom;
        if (verticalPrimary)
            width = std::max(1, qRound(height * m_aspectRatio));
        else
            height = std::max(1, qRound(width / m_aspectRatio));

        if (width > m_image.width()) {
            width = m_image.width();
            height = std::max(1, qRound(width / m_aspectRatio));
        }
        if (height > m_image.height()) {
            height = m_image.height();
            width = std::max(1, qRound(height * m_aspectRatio));
        }

        QRect constrained(QPoint(), QSize(width, height));
        const bool movingLeft = moving.x() < anchor.x();
        const bool movingUp = moving.y() < anchor.y();
        if (mode == DragMode::Left || mode == DragMode::Right) {
            constrained.moveCenter(
                QPoint(result.center().x(),
                       m_selectionAtDragStart.center().y()));
        } else if (mode == DragMode::Top
                   || mode == DragMode::Bottom) {
            constrained.moveCenter(
                QPoint(m_selectionAtDragStart.center().x(),
                       result.center().y()));
        } else {
            constrained.moveLeft(
                movingLeft ? anchor.x() - width + 1 : anchor.x());
            constrained.moveTop(
                movingUp ? anchor.y() - height + 1 : anchor.y());
        }
        moveInsideImage(constrained);
        return constrained;
    }

    void moveInsideImage(QRect &rectangle) const
    {
        if (rectangle.left() < 0)
            rectangle.moveLeft(0);
        if (rectangle.top() < 0)
            rectangle.moveTop(0);
        if (rectangle.right() >= m_image.width())
            rectangle.moveRight(m_image.width() - 1);
        if (rectangle.bottom() >= m_image.height())
            rectangle.moveBottom(m_image.height() - 1);
    }

    QImage m_image;
    QRect m_selection;
    QPoint m_dragStart;
    QRect m_selectionAtDragStart;
    qreal m_aspectRatio = 0.0;
    DragMode m_dragMode = DragMode::None;
    bool m_dragging = false;
    bool m_userAdjusted = false;
};

CropDialog::CropDialog(const QImage &image, QWidget *parent)
    : QDialog(parent)
    , m_image(image)
{
    setWindowTitle(tr("Crop image"));
    resize(660, 520);
    auto *layout = new QVBoxLayout(this);
    m_preview = new CropPreviewWidget(image, this);
    layout->addWidget(m_preview, 1);
    auto *instructions = new QLabel(
        tr("Drag to create a selection. Drag inside it to move; drag the eight handles to resize."),
        this);
    instructions->setWordWrap(true);
    instructions->setObjectName(
        QStringLiteral("cropInstructions"));
    layout->addWidget(instructions);

    auto *editors = new QHBoxLayout;
    m_x = new QSpinBox(this);
    m_y = new QSpinBox(this);
    m_width = new QSpinBox(this);
    m_height = new QSpinBox(this);
    m_x->setObjectName(QStringLiteral("cropX"));
    m_y->setObjectName(QStringLiteral("cropY"));
    m_width->setObjectName(QStringLiteral("cropWidth"));
    m_height->setObjectName(QStringLiteral("cropHeight"));
    m_x->setRange(0, std::max(0, image.width() - 1));
    m_y->setRange(0, std::max(0, image.height() - 1));
    m_width->setRange(1, image.width());
    m_height->setRange(1, image.height());
    for (auto [label, editor] : {
             std::pair{tr("X"), m_x}, std::pair{tr("Y"), m_y},
             std::pair{tr("Width"), m_width}, std::pair{tr("Height"), m_height}}) {
        editors->addWidget(new QLabel(label, this));
        editors->addWidget(editor);
    }
    auto *ratio = new QComboBox(this);
    ratio->setObjectName(QStringLiteral("cropAspectRatio"));
    ratio->setAccessibleName(tr("Crop aspect ratio"));
    ratio->addItem(tr("Free"), 0.0);
    ratio->addItem(QStringLiteral("1:1"), 1.0);
    ratio->addItem(QStringLiteral("4:3"), 4.0 / 3.0);
    ratio->addItem(QStringLiteral("16:9"), 16.0 / 9.0);
    ratio->addItem(tr("Original"), image.width() / static_cast<qreal>(image.height()));
    editors->addWidget(ratio);
    layout->addLayout(editors);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_actionButton = buttons->button(QDialogButtonBox::Ok);
    m_actionButton->setText(tr("Crop"));
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    syncEditors(image.rect());
    connect(m_preview, &CropPreviewWidget::selectionChanged,
            this, &CropDialog::syncEditors);
    for (QSpinBox *editor : {m_x, m_y, m_width, m_height})
        connect(editor, &QSpinBox::valueChanged, this, &CropDialog::syncPreview);
    connect(ratio, &QComboBox::currentIndexChanged, this, [this, ratio] {
        const qreal value = ratio->currentData().toDouble();
        m_preview->setAspectRatio(value);
        if (value > 0.0) {
            m_height->setValue(std::max(1, qRound(m_width->value() / value)));
            syncPreview();
        }
    });
}

QRect CropDialog::cropRectangle() const
{
    return m_preview->selection();
}

void CropDialog::setOperationText(const QString &windowTitle,
                                  const QString &actionText)
{
    setWindowTitle(windowTitle);
    m_actionButton->setText(actionText);
}

void CropDialog::syncEditors(const QRect &rectangle)
{
    const QSignalBlocker bx(m_x), by(m_y), bw(m_width), bh(m_height);
    m_x->setValue(rectangle.x());
    m_y->setValue(rectangle.y());
    m_width->setValue(rectangle.width());
    m_height->setValue(rectangle.height());
}

void CropDialog::syncPreview()
{
    const int width = std::min(m_width->value(), m_image.width() - m_x->value());
    const int height = std::min(m_height->value(), m_image.height() - m_y->value());
    m_preview->setSelection(QRect(m_x->value(), m_y->value(), width, height));
}

ResizeDialog::ResizeDialog(const QSize &currentSize, QWidget *parent)
    : QDialog(parent)
    , m_originalSize(currentSize)
{
    setWindowTitle(tr("Resize image"));
    auto *layout = new QVBoxLayout(this);
    auto *form = new QFormLayout;
    m_width = new QSpinBox(this);
    m_height = new QSpinBox(this);
    for (QSpinBox *editor : {m_width, m_height})
        editor->setRange(1, 100000);
    m_width->setValue(currentSize.width());
    m_height->setValue(currentSize.height());
    form->addRow(tr("Width (px)"), m_width);
    form->addRow(tr("Height (px)"), m_height);
    m_keepAspect = new QCheckBox(tr("Keep aspect ratio"), this);
    m_keepAspect->setChecked(true);
    form->addRow(QString(), m_keepAspect);
    layout->addLayout(form);

    connect(m_width, &QSpinBox::valueChanged, this, [this](int width) {
        if (m_updating || !m_keepAspect->isChecked())
            return;
        m_updating = true;
        m_height->setValue(qRound(width * m_originalSize.height()
                                  / static_cast<qreal>(m_originalSize.width())));
        m_updating = false;
    });
    connect(m_height, &QSpinBox::valueChanged, this, [this](int height) {
        if (m_updating || !m_keepAspect->isChecked())
            return;
        m_updating = true;
        m_width->setValue(qRound(height * m_originalSize.width()
                                 / static_cast<qreal>(m_originalSize.height())));
        m_updating = false;
    });

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

QSize ResizeDialog::targetSize() const
{
    return QSize(m_width->value(), m_height->value());
}

AdjustDialog::AdjustDialog(const QImage &image, QWidget *parent)
    : QDialog(parent)
    , m_previewSource(image.scaled(420, 260, Qt::KeepAspectRatio,
                                   Qt::SmoothTransformation))
{
    setWindowTitle(tr("Adjust colors"));
    resize(560, 500);
    auto *layout = new QVBoxLayout(this);
    m_preview = new QLabel(this);
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setMinimumHeight(270);
    layout->addWidget(m_preview, 1);

    auto *form = new QFormLayout;
    m_brightness = new QSlider(Qt::Horizontal, this);
    m_contrast = new QSlider(Qt::Horizontal, this);
    for (QSlider *slider : {m_brightness, m_contrast})
        slider->setRange(-100, 100);
    m_gamma = new QDoubleSpinBox(this);
    m_gamma->setRange(0.1, 3.0);
    m_gamma->setSingleStep(0.1);
    m_gamma->setValue(1.0);
    form->addRow(tr("Brightness"), m_brightness);
    form->addRow(tr("Contrast"), m_contrast);
    form->addRow(tr("Gamma"), m_gamma);
    layout->addLayout(form);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Reset,
        this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons->button(QDialogButtonBox::Reset), &QPushButton::clicked,
            this, [this] {
        m_brightness->setValue(0);
        m_contrast->setValue(0);
        m_gamma->setValue(1.0);
    });
    layout->addWidget(buttons);
    connect(m_brightness, &QSlider::valueChanged, this, &AdjustDialog::updatePreview);
    connect(m_contrast, &QSlider::valueChanged, this, &AdjustDialog::updatePreview);
    connect(m_gamma, &QDoubleSpinBox::valueChanged, this, &AdjustDialog::updatePreview);
    updatePreview();
}

int AdjustDialog::brightness() const { return m_brightness->value(); }
int AdjustDialog::contrast() const { return m_contrast->value(); }
qreal AdjustDialog::gamma() const { return m_gamma->value(); }

void AdjustDialog::updatePreview()
{
    ImageDocument previewDocument;
    previewDocument.loadImage(m_previewSource);
    previewDocument.adjustImage(brightness(), contrast(), gamma());
    m_preview->setPixmap(QPixmap::fromImage(previewDocument.image()));
}

SettingsDialog::SettingsDialog(const QString &theme, const QString &language,
                               const QString &toolbarPosition,
                               const QString &filmstripPosition, int slideshowSeconds,
                               bool showFilmstrip,
                               bool showFilmstripFileNames,
                               int filmstripThumbnailExtent,
                               int filmstripVerticalColumns,
                               const QString &directoryThumbnailSortKey,
                               bool directoryThumbnailSortAscending,
                               bool randomSlideshow,
                               bool fullscreenSlideshow,
                               int imageMemoryCacheMiB,
                               bool persistentThumbnailCacheEnabled,
                               int persistentThumbnailCacheMiB,
                               qint64 persistentThumbnailCacheUsageBytes,
                               const QList<ActionRegistry::ToolbarItemDefinition> &toolbarItems,
                               const QStringList &toolbarLayout,
                               const QStringList &defaultToolbarLayout,
                               const QList<QPair<QString, QString>> &shortcutItems,
                               const QStringList &shortcutLayout,
                               const QStringList &defaultShortcutLayout,
                               const QString &wheelAction,
                               const QString &ctrlWheelAction,
                               const QString &doubleClickAction,
                               const QString &middleButtonAction,
                               const QString &backButtonAction,
                               const QString &forwardButtonAction,
                               const InterfaceLayoutState &interfaceLayout,
                               QWidget *parent)
    : QDialog(parent)
    , m_toolbarItemDefinitions(toolbarItems)
    , m_defaultToolbarLayout(defaultToolbarLayout)
    , m_shortcutItemDefinitions(shortcutItems)
    , m_defaultShortcutLayout(defaultShortcutLayout)
{
    setWindowTitle(tr("Preferences"));
    setMinimumSize(900, 640);
    auto *layout = new QVBoxLayout(this);
    auto *tabs = new QTabWidget(this);
    m_tabs = tabs;
    tabs->setObjectName(QStringLiteral("settingsTabs"));

    auto *generalPage = new QWidget(tabs);
    auto *generalLayout = new QVBoxLayout(generalPage);
    auto *form = new QFormLayout;

    m_theme = new QComboBox(this);
    m_theme->setObjectName(QStringLiteral("settingsTheme"));
    m_theme->setAccessibleName(tr("Application theme"));
    m_theme->addItem(tr("Follow system"), QStringLiteral("system"));
    m_theme->addItem(tr("Light"), QStringLiteral("light"));
    m_theme->addItem(tr("Dark"), QStringLiteral("dark"));
    const int themeIndex = m_theme->findData(theme);
    m_theme->setCurrentIndex(themeIndex >= 0 ? themeIndex : 0);
    form->addRow(tr("Theme"), m_theme);

    m_language = new QComboBox(this);
    m_language->setAccessibleName(tr("Interface language"));
    m_language->addItem(tr("Follow system language"), QStringLiteral("system"));
    m_language->addItem(QStringLiteral("简体中文"), QStringLiteral("zh_CN"));
    m_language->addItem(QStringLiteral("English"), QStringLiteral("en"));
    const int languageIndex = m_language->findData(language);
    m_language->setCurrentIndex(languageIndex >= 0 ? languageIndex : 0);
    form->addRow(tr("Language"), m_language);

    m_slideshowSeconds = new QSpinBox(this);
    m_slideshowSeconds->setAccessibleName(tr("Slideshow interval"));
    m_slideshowSeconds->setRange(1, 60);
    m_slideshowSeconds->setSuffix(tr(" s"));
    m_slideshowSeconds->setValue(std::clamp(slideshowSeconds, 1, 60));
    form->addRow(tr("Slideshow interval"), m_slideshowSeconds);

    m_randomSlideshow = new QCheckBox(tr("Random slideshow order"), this);
    m_randomSlideshow->setChecked(randomSlideshow);
    form->addRow(QString(), m_randomSlideshow);
    m_fullscreenSlideshow = new QCheckBox(tr("Enter full screen for slideshow"), this);
    m_fullscreenSlideshow->setChecked(fullscreenSlideshow);
    form->addRow(QString(), m_fullscreenSlideshow);
    generalLayout->addLayout(form);

    auto *hint = new QLabel(
        tr("Use Apply to preview changes without closing this window. Language changes take effect after restarting Clearveil."),
        generalPage);
    hint->setWordWrap(true);
    hint->setStyleSheet(QStringLiteral("color: palette(mid);"));
    generalLayout->addWidget(hint);
    generalLayout->addStretch();
    tabs->addTab(generalPage, tr("General"));

    InterfaceLayoutState resolvedLayout = interfaceLayout;
    resolvedLayout.toolbarPosition = toolbarPosition;
    resolvedLayout.thumbnailsPlacement = filmstripPosition;
    resolvedLayout.showThumbnails = showFilmstrip;
    m_interfaceLayoutPage = new InterfaceLayoutPage(
        resolvedLayout, tabs);
    tabs->addTab(m_interfaceLayoutPage,
                 tr("Interface and layout"));

    auto *galleryPage = new QWidget(tabs);
    auto *galleryLayout = new QVBoxLayout(galleryPage);
    auto *filmstripForm = new QFormLayout;
    m_showFilmstripFileNames = new QCheckBox(
        tr("Show file names"), galleryPage);
    m_showFilmstripFileNames->setObjectName(
        QStringLiteral("filmstripShowFileNames"));
    m_showFilmstripFileNames->setChecked(
        showFilmstripFileNames);
    filmstripForm->addRow(QString(), m_showFilmstripFileNames);

    m_filmstripThumbnailExtent = new QSpinBox(galleryPage);
    m_filmstripThumbnailExtent->setObjectName(
        QStringLiteral("filmstripThumbnailExtent"));
    m_filmstripThumbnailExtent->setRange(48, 256);
    m_filmstripThumbnailExtent->setSuffix(tr(" px"));
    m_filmstripThumbnailExtent->setValue(
        std::clamp(filmstripThumbnailExtent, 48, 256));
    filmstripForm->addRow(tr("Maximum thumbnail size"),
                          m_filmstripThumbnailExtent);

    m_filmstripVerticalColumns = new QSpinBox(galleryPage);
    m_filmstripVerticalColumns->setObjectName(
        QStringLiteral("filmstripVerticalColumns"));
    m_filmstripVerticalColumns->setRange(1, 4);
    m_filmstripVerticalColumns->setValue(
        std::clamp(filmstripVerticalColumns, 1, 4));
    filmstripForm->addRow(tr("Columns in vertical layout"),
                          m_filmstripVerticalColumns);

    m_directoryThumbnailSortKey = new QComboBox(galleryPage);
    m_directoryThumbnailSortKey->setObjectName(
        QStringLiteral("directoryThumbnailSortKey"));
    m_directoryThumbnailSortKey->addItem(
        tr("Name"), QStringLiteral("name"));
    m_directoryThumbnailSortKey->addItem(
        tr("Modified time"), QStringLiteral("modified"));
    m_directoryThumbnailSortKey->addItem(
        tr("File size"), QStringLiteral("size"));
    m_directoryThumbnailSortKey->addItem(
        tr("File type"), QStringLiteral("type"));
    const int sortIndex = m_directoryThumbnailSortKey->findData(
        directoryThumbnailSortKey);
    m_directoryThumbnailSortKey->setCurrentIndex(
        sortIndex >= 0 ? sortIndex : 0);
    filmstripForm->addRow(tr("Current folder sorting"),
                          m_directoryThumbnailSortKey);

    m_directoryThumbnailSortDirection =
        new QComboBox(galleryPage);
    m_directoryThumbnailSortDirection->setObjectName(
        QStringLiteral("directoryThumbnailSortDirection"));
    m_directoryThumbnailSortDirection->addItem(
        tr("Ascending"), true);
    m_directoryThumbnailSortDirection->addItem(
        tr("Descending"), false);
    m_directoryThumbnailSortDirection->setCurrentIndex(
        directoryThumbnailSortAscending ? 0 : 1);
    filmstripForm->addRow(tr("Sort direction"),
                          m_directoryThumbnailSortDirection);
    galleryLayout->addLayout(filmstripForm);

    auto *sortingHint = new QLabel(
        tr("Sorting applies only to Current folder. Opened images always keep the order in which you opened them."),
        galleryPage);
    sortingHint->setWordWrap(true);
    sortingHint->setStyleSheet(
        QStringLiteral("color: palette(mid);"));
    galleryLayout->addWidget(sortingHint);

    auto *memoryHint = new QLabel(
        tr("Main images are cached in memory for faster navigation. This cache is cleared when Clearveil exits."),
        galleryPage);
    memoryHint->setWordWrap(true);
    galleryLayout->addWidget(memoryHint);

    auto *cacheForm = new QFormLayout;
    m_imageMemoryCacheMiB = new QSpinBox(galleryPage);
    m_imageMemoryCacheMiB->setObjectName(
        QStringLiteral("imageMemoryCacheLimitMiB"));
    m_imageMemoryCacheMiB->setAccessibleName(
        tr("Maximum main image memory cache"));
    m_imageMemoryCacheMiB->setRange(16, 4'096);
    m_imageMemoryCacheMiB->setSuffix(tr(" MiB"));
    m_imageMemoryCacheMiB->setValue(
        std::clamp(imageMemoryCacheMiB, 16, 4'096));
    cacheForm->addRow(tr("Main image memory cache"),
                      m_imageMemoryCacheMiB);

    auto *galleryHint = new QLabel(
        tr("Clearveil normally keeps thumbnails only for the current session. Enable disk caching only if you frequently revisit the same images."),
        galleryPage);
    galleryHint->setWordWrap(true);
    galleryLayout->addWidget(galleryHint);

    m_persistentThumbnailCacheEnabled =
        new QCheckBox(tr("Enable persistent thumbnail cache"),
                      galleryPage);
    m_persistentThumbnailCacheEnabled->setObjectName(
        QStringLiteral("persistentThumbnailCacheEnabled"));
    m_persistentThumbnailCacheEnabled->setChecked(
        persistentThumbnailCacheEnabled);
    cacheForm->addRow(
        QString(), m_persistentThumbnailCacheEnabled);

    m_persistentThumbnailCacheMiB =
        new QSpinBox(galleryPage);
    m_persistentThumbnailCacheMiB->setObjectName(
        QStringLiteral("persistentThumbnailCacheLimitMiB"));
    m_persistentThumbnailCacheMiB->setAccessibleName(
        tr("Maximum persistent thumbnail cache"));
    m_persistentThumbnailCacheMiB->setRange(64, 65'536);
    m_persistentThumbnailCacheMiB->setSuffix(tr(" MiB"));
    m_persistentThumbnailCacheMiB->setValue(
        std::clamp(persistentThumbnailCacheMiB,
                   64, 65'536));
    m_persistentThumbnailCacheMiB->setEnabled(
        persistentThumbnailCacheEnabled);
    cacheForm->addRow(tr("Maximum cache size"),
                      m_persistentThumbnailCacheMiB);

    m_persistentThumbnailCacheUsage =
        new QLabel(galleryPage);
    m_persistentThumbnailCacheUsage->setObjectName(
        QStringLiteral("persistentThumbnailCacheUsage"));
    const auto updateUsage =
        [this](qint64 bytes) {
        const qreal mebibytes =
            static_cast<qreal>(bytes)
            / (1024.0 * 1024.0);
        m_persistentThumbnailCacheUsage->setText(
            tr("%1 MiB").arg(
                QString::number(mebibytes, 'f',
                                mebibytes < 10.0 ? 2 : 1)));
    };
    updateUsage(persistentThumbnailCacheUsageBytes);
    cacheForm->addRow(tr("Current disk usage"),
                      m_persistentThumbnailCacheUsage);
    galleryLayout->addLayout(cacheForm);

    connect(m_persistentThumbnailCacheEnabled,
            &QCheckBox::toggled,
            m_persistentThumbnailCacheMiB,
            &QWidget::setEnabled);

    auto *cacheButtons = new QHBoxLayout;
    cacheButtons->addStretch();
    auto *clearCacheButton =
        new QPushButton(tr("Clear thumbnail cache"),
                        galleryPage);
    clearCacheButton->setObjectName(
        QStringLiteral("clearPersistentThumbnailCacheButton"));
    connect(clearCacheButton, &QPushButton::clicked,
            this, [this, updateUsage] {
        if (PersistentThumbnailCache::clear()) {
            updateUsage(0);
        } else {
            QMessageBox::warning(
                this, tr("Could not clear cache"),
                tr("Some thumbnail cache files could not be removed."));
        }
    });
    cacheButtons->addWidget(clearCacheButton);
    galleryLayout->addLayout(cacheButtons);
    galleryLayout->addStretch();
    tabs->addTab(galleryPage, tr("Thumbnail strip"));

    auto *toolbarPage = new QWidget(tabs);
    auto *toolbarLayoutBox = new QVBoxLayout(toolbarPage);
    auto *toolbarHint = new QLabel(
        tr("Choose which commands appear on the main toolbar and drag them into a useful order."),
        toolbarPage);
    toolbarHint->setWordWrap(true);
    toolbarLayoutBox->addWidget(toolbarHint);

    m_toolbarItems = new QListWidget(toolbarPage);
    m_toolbarItems->setObjectName(QStringLiteral("toolbarItems"));
    m_toolbarItems->setAccessibleName(tr("Toolbar items"));
    m_toolbarItems->setSelectionMode(
        QAbstractItemView::SingleSelection);
    m_toolbarItems->setDragDropMode(
        QAbstractItemView::InternalMove);
    m_toolbarItems->setDefaultDropAction(Qt::MoveAction);
    m_toolbarItems->setAlternatingRowColors(true);
    populateToolbarItems(toolbarLayout);
    toolbarLayoutBox->addWidget(m_toolbarItems, 1);

    auto *toolbarButtons = new QHBoxLayout;
    auto *moveUpButton = new QPushButton(tr("Move up"), toolbarPage);
    moveUpButton->setObjectName(QStringLiteral("toolbarMoveUpButton"));
    auto *moveDownButton = new QPushButton(tr("Move down"), toolbarPage);
    moveDownButton->setObjectName(QStringLiteral("toolbarMoveDownButton"));
    auto *resetToolbarButton =
        new QPushButton(tr("Restore defaults"), toolbarPage);
    resetToolbarButton->setObjectName(
        QStringLiteral("toolbarResetButton"));
    connect(moveUpButton, &QPushButton::clicked,
            this, [this] { moveToolbarItem(-1); });
    connect(moveDownButton, &QPushButton::clicked,
            this, [this] { moveToolbarItem(1); });
    connect(resetToolbarButton, &QPushButton::clicked,
            this, [this] {
                populateToolbarItems(m_defaultToolbarLayout);
            });
    toolbarButtons->addWidget(moveUpButton);
    toolbarButtons->addWidget(moveDownButton);
    toolbarButtons->addStretch();
    toolbarButtons->addWidget(resetToolbarButton);
    toolbarLayoutBox->addLayout(toolbarButtons);
    tabs->addTab(toolbarPage, tr("Toolbar"));

    auto *shortcutPage = new QWidget(tabs);
    auto *shortcutPageLayout = new QVBoxLayout(shortcutPage);
    auto *shortcutHint = new QLabel(
        tr("Select a shortcut field and press the desired key combination. Clear the field to disable a shortcut."),
        shortcutPage);
    shortcutHint->setWordWrap(true);
    shortcutPageLayout->addWidget(shortcutHint);

    m_shortcutItems = new QTableWidget(shortcutPage);
    m_shortcutItems->setObjectName(QStringLiteral("shortcutItems"));
    m_shortcutItems->setAccessibleName(tr("Keyboard shortcuts"));
    m_shortcutItems->setColumnCount(2);
    m_shortcutItems->setHorizontalHeaderLabels(
        {tr("Command"), tr("Shortcut")});
    m_shortcutItems->verticalHeader()->hide();
    m_shortcutItems->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::Stretch);
    m_shortcutItems->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::ResizeToContents);
    m_shortcutItems->setSelectionBehavior(
        QAbstractItemView::SelectRows);
    m_shortcutItems->setAlternatingRowColors(true);
    populateShortcutItems(shortcutLayout);
    shortcutPageLayout->addWidget(m_shortcutItems, 1);

    auto *shortcutButtons = new QHBoxLayout;
    shortcutButtons->addStretch();
    auto *resetShortcutsButton =
        new QPushButton(tr("Restore defaults"), shortcutPage);
    resetShortcutsButton->setObjectName(
        QStringLiteral("shortcutResetButton"));
    connect(resetShortcutsButton, &QPushButton::clicked,
            this, [this] {
                populateShortcutItems(m_defaultShortcutLayout);
            });
    shortcutButtons->addWidget(resetShortcutsButton);
    shortcutPageLayout->addLayout(shortcutButtons);
    tabs->addTab(shortcutPage, tr("Shortcuts"));

    auto *mousePage = new QWidget(tabs);
    auto *mousePageLayout = new QVBoxLayout(mousePage);
    auto *mouseHint = new QLabel(
        tr("Choose how mouse gestures behave on the image canvas."),
        mousePage);
    mouseHint->setWordWrap(true);
    mousePageLayout->addWidget(mouseHint);
    auto *mouseForm = new QFormLayout;

    m_wheelAction = new QComboBox(mousePage);
    m_wheelAction->setObjectName(
        QStringLiteral("mouseWheelAction"));
    m_wheelAction->setAccessibleName(tr("Mouse wheel action"));
    m_wheelAction->addItem(
        tr("Scroll image"), QStringLiteral("scroll"));
    m_wheelAction->addItem(tr("Zoom"), QStringLiteral("zoom"));
    m_wheelAction->addItem(tr("Previous / next image"),
                           QStringLiteral("navigate"));
    m_wheelAction->addItem(tr("Do nothing"),
                           QStringLiteral("none"));
    int mouseActionIndex =
        m_wheelAction->findData(wheelAction);
    m_wheelAction->setCurrentIndex(
        mouseActionIndex >= 0 ? mouseActionIndex : 0);
    mouseForm->addRow(tr("Mouse wheel"), m_wheelAction);

    const auto populateWheelActions =
        [](QComboBox *combo,
           const QString &selected) {
            combo->addItem(
                tr("Scroll image"),
                QStringLiteral("scroll"));
            combo->addItem(
                tr("Zoom"),
                QStringLiteral("zoom"));
            combo->addItem(
                tr("Previous / next image"),
                QStringLiteral("navigate"));
            combo->addItem(
                tr("Do nothing"),
                QStringLiteral("none"));
            const int index =
                combo->findData(selected);
            combo->setCurrentIndex(
                index >= 0 ? index : 0);
        };
    m_ctrlWheelAction = new QComboBox(mousePage);
    m_ctrlWheelAction->setObjectName(
        QStringLiteral("mouseCtrlWheelAction"));
    m_ctrlWheelAction->setAccessibleName(
        tr("Control plus mouse wheel action"));
    populateWheelActions(
        m_ctrlWheelAction, ctrlWheelAction);
    mouseForm->addRow(
        tr("Ctrl + mouse wheel"),
        m_ctrlWheelAction);
    auto *scrollHint = new QLabel(
        tr("When mouse wheel is set to scroll, "
           "Shift + mouse wheel scrolls horizontally."),
        mousePage);
    scrollHint->setWordWrap(true);
    mouseForm->addRow(QString(), scrollHint);

    const auto populatePointerActions =
        [](QComboBox *combo, const QString &selected) {
            combo->addItem(tr("Do nothing"),
                           QStringLiteral("none"));
            combo->addItem(tr("Toggle fit / actual size"),
                           QStringLiteral("toggle_zoom"));
            combo->addItem(tr("Full screen"),
                           QStringLiteral("fullscreen"));
            combo->addItem(tr("Previous image"),
                           QStringLiteral("previous"));
            combo->addItem(tr("Next image"),
                           QStringLiteral("next"));
            combo->addItem(tr("Fit to window"),
                           QStringLiteral("fit"));
            combo->addItem(tr("Actual size"),
                           QStringLiteral("actual_size"));
            combo->addItem(tr("Slideshow"),
                           QStringLiteral("slideshow"));
            const int index = combo->findData(selected);
            combo->setCurrentIndex(index >= 0 ? index : 0);
        };

    m_doubleClickAction = new QComboBox(mousePage);
    m_doubleClickAction->setObjectName(
        QStringLiteral("mouseDoubleClickAction"));
    m_doubleClickAction->setAccessibleName(
        tr("Double-click action"));
    populatePointerActions(m_doubleClickAction,
                           doubleClickAction);
    mouseForm->addRow(tr("Double-click"),
                      m_doubleClickAction);

    m_middleButtonAction = new QComboBox(mousePage);
    m_middleButtonAction->setObjectName(
        QStringLiteral("mouseMiddleButtonAction"));
    m_middleButtonAction->setAccessibleName(
        tr("Middle button action"));
    populatePointerActions(m_middleButtonAction,
                           middleButtonAction);
    mouseForm->addRow(tr("Middle button"),
                      m_middleButtonAction);

    m_backButtonAction = new QComboBox(mousePage);
    m_backButtonAction->setObjectName(
        QStringLiteral("mouseBackButtonAction"));
    m_backButtonAction->setAccessibleName(
        tr("Back button action"));
    populatePointerActions(m_backButtonAction,
                           backButtonAction);
    mouseForm->addRow(tr("Back button"),
                      m_backButtonAction);

    m_forwardButtonAction = new QComboBox(mousePage);
    m_forwardButtonAction->setObjectName(
        QStringLiteral("mouseForwardButtonAction"));
    m_forwardButtonAction->setAccessibleName(
        tr("Forward button action"));
    populatePointerActions(m_forwardButtonAction,
                           forwardButtonAction);
    mouseForm->addRow(tr("Forward button"),
                      m_forwardButtonAction);
    mousePageLayout->addLayout(mouseForm);
    mousePageLayout->addStretch();
    auto *mouseButtons = new QHBoxLayout;
    mouseButtons->addStretch();
    auto *resetMouseButton =
        new QPushButton(tr("Restore defaults"), mousePage);
    resetMouseButton->setObjectName(
        QStringLiteral("mouseResetButton"));
    connect(resetMouseButton, &QPushButton::clicked,
            this, [this] {
                const auto select =
                    [](QComboBox *combo,
                       const QString &value) {
                        const int index =
                            combo->findData(value);
                        if (index >= 0)
                            combo->setCurrentIndex(index);
                    };
                select(m_wheelAction,
                       QStringLiteral("scroll"));
                select(m_ctrlWheelAction,
                       QStringLiteral("zoom"));
                select(m_doubleClickAction,
                       QStringLiteral("toggle_zoom"));
                select(m_middleButtonAction,
                       QStringLiteral("none"));
                select(m_backButtonAction,
                       QStringLiteral("previous"));
                select(m_forwardButtonAction,
                       QStringLiteral("next"));
            });
    mouseButtons->addWidget(resetMouseButton);
    mousePageLayout->addLayout(mouseButtons);
    tabs->addTab(mousePage, tr("Mouse"));
    layout->addWidget(tabs, 1);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel
            | QDialogButtonBox::Apply,
        this);
    buttons->setObjectName(QStringLiteral("settingsButtonBox"));
    auto *applyButton = buttons->button(QDialogButtonBox::Apply);
    applyButton->setObjectName(QStringLiteral("settingsApplyButton"));
    connect(applyButton, &QPushButton::clicked,
            this, [this] {
                if (validateShortcutConflicts())
                    emit applyRequested();
            });
    connect(buttons, &QDialogButtonBox::accepted,
            this, [this] {
                if (validateShortcutConflicts())
                    accept();
            });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

void SettingsDialog::populateToolbarItems(const QStringList &layout)
{
    if (!m_toolbarItems)
        return;

    QHash<QString, QString> labels;
    QHash<QString, QIcon> icons;
    for (const auto &definition : std::as_const(
             m_toolbarItemDefinitions)) {
        labels.insert(definition.id, definition.label);
        icons.insert(definition.id, definition.icon);
    }

    m_toolbarItems->clear();
    QSet<QString> seen;
    const auto addItem = [this, &labels, &icons, &seen](
                             const QString &encoded) {
        const bool enabled = !encoded.startsWith(QLatin1Char('!'));
        const QString id = enabled ? encoded : encoded.mid(1);
        if (id.isEmpty() || seen.contains(id)
            || !labels.contains(id)) {
            return;
        }
        auto *item = new QListWidgetItem(labels.value(id),
                                         m_toolbarItems);
        QIcon itemIcon = icons.value(id);
        if (itemIcon.isNull())
            itemIcon = ClearveilIcon::fromName(
                id.startsWith(QStringLiteral("separator"))
                    ? QStringLiteral("separator")
                    : id == QStringLiteral("spacer")
                        ? QStringLiteral("spacer")
                        : QStringLiteral("unknown"));
        item->setIcon(itemIcon);
        item->setData(Qt::UserRole, id);
        item->setFlags(item->flags()
                       | Qt::ItemIsUserCheckable
                       | Qt::ItemIsDragEnabled);
        item->setCheckState(enabled ? Qt::Checked
                                    : Qt::Unchecked);
        seen.insert(id);
    };

    for (const QString &encoded : layout)
        addItem(encoded);
    for (const auto &definition : std::as_const(
             m_toolbarItemDefinitions)) {
        if (!seen.contains(definition.id))
            addItem(QStringLiteral("!") + definition.id);
    }
    if (m_toolbarItems->count() > 0)
        m_toolbarItems->setCurrentRow(0);
}

void SettingsDialog::moveToolbarItem(int offset)
{
    if (!m_toolbarItems)
        return;
    const int row = m_toolbarItems->currentRow();
    const int target = row + offset;
    if (row < 0 || target < 0
        || target >= m_toolbarItems->count()) {
        return;
    }
    QListWidgetItem *item = m_toolbarItems->takeItem(row);
    m_toolbarItems->insertItem(target, item);
    m_toolbarItems->setCurrentRow(target);
}

void SettingsDialog::populateShortcutItems(
    const QStringList &layout)
{
    if (!m_shortcutItems)
        return;

    QHash<QString, QString> shortcuts;
    for (const QString &encoded : layout) {
        const int separator = encoded.indexOf(QLatin1Char('\t'));
        if (separator <= 0)
            continue;
        shortcuts.insert(encoded.left(separator),
                         encoded.mid(separator + 1));
    }

    m_shortcutItems->clearContents();
    m_shortcutItems->setRowCount(
        m_shortcutItemDefinitions.size());
    for (int row = 0; row < m_shortcutItemDefinitions.size();
         ++row) {
        const auto &[id, label] =
            m_shortcutItemDefinitions.at(row);
        auto *commandItem = new QTableWidgetItem(label);
        commandItem->setData(Qt::UserRole, id);
        commandItem->setFlags(
            commandItem->flags() & ~Qt::ItemIsEditable);
        m_shortcutItems->setItem(row, 0, commandItem);

        auto *editor = new QKeySequenceEdit(
            QKeySequence::fromString(shortcuts.value(id),
                                     QKeySequence::PortableText),
            m_shortcutItems);
        editor->setObjectName(
            QStringLiteral("shortcutEditor_") + id);
        editor->setAccessibleName(
            tr("Shortcut for %1").arg(label));
        editor->setProperty("shortcutId", id);
        editor->setClearButtonEnabled(true);
        m_shortcutItems->setCellWidget(row, 1, editor);
    }
    m_shortcutItems->resizeColumnToContents(1);
}

bool SettingsDialog::validateShortcutConflicts()
{
    QHash<QString, QString> owners;
    for (int row = 0; row < m_shortcutItems->rowCount();
         ++row) {
        auto *editor = qobject_cast<QKeySequenceEdit *>(
            m_shortcutItems->cellWidget(row, 1));
        const QTableWidgetItem *command =
            m_shortcutItems->item(row, 0);
        if (!editor || !command)
            continue;
        const QString shortcut = editor->keySequence().toString(
            QKeySequence::PortableText);
        if (shortcut.isEmpty())
            continue;
        if (owners.contains(shortcut)) {
            QMessageBox::warning(
                this, tr("Shortcut conflict"),
                tr("“%1” is already assigned to both “%2” and “%3”.")
                    .arg(editor->keySequence().toString(
                             QKeySequence::NativeText),
                         owners.value(shortcut),
                         command->text()));
            return false;
        }
        owners.insert(shortcut, command->text());
    }
    return true;
}

QString SettingsDialog::theme() const
{
    return m_theme->currentData().toString();
}

QString SettingsDialog::language() const
{
    return m_language->currentData().toString();
}

InterfaceLayoutState SettingsDialog::interfaceLayout() const
{
    return m_interfaceLayoutPage
        ? m_interfaceLayoutPage->state()
        : InterfaceLayoutState{};
}

void SettingsDialog::showInterfaceLayoutPage()
{
    if (m_tabs && m_interfaceLayoutPage)
        m_tabs->setCurrentWidget(m_interfaceLayoutPage);
}

QString SettingsDialog::toolbarPosition() const
{
    return interfaceLayout().toolbarPosition;
}

QString SettingsDialog::filmstripPosition() const
{
    const QString placement = interfaceLayout().thumbnailsPlacement;
    return placement == QStringLiteral("floating")
        ? QStringLiteral("bottom") : placement;
}

int SettingsDialog::slideshowSeconds() const
{
    return m_slideshowSeconds->value();
}

bool SettingsDialog::showFilmstrip() const
{
    return interfaceLayout().showThumbnails;
}

bool SettingsDialog::showFilmstripFileNames() const
{
    return m_showFilmstripFileNames
        && m_showFilmstripFileNames->isChecked();
}

int SettingsDialog::filmstripThumbnailExtent() const
{
    return m_filmstripThumbnailExtent
        ? m_filmstripThumbnailExtent->value() : 256;
}

int SettingsDialog::filmstripVerticalColumns() const
{
    return m_filmstripVerticalColumns
        ? m_filmstripVerticalColumns->value() : 1;
}

QString SettingsDialog::directoryThumbnailSortKey() const
{
    return m_directoryThumbnailSortKey
        ? m_directoryThumbnailSortKey->currentData().toString()
        : QStringLiteral("name");
}

bool SettingsDialog::directoryThumbnailSortAscending() const
{
    return !m_directoryThumbnailSortDirection
        || m_directoryThumbnailSortDirection
               ->currentData().toBool();
}

bool SettingsDialog::randomSlideshow() const
{
    return m_randomSlideshow->isChecked();
}

bool SettingsDialog::fullscreenSlideshow() const
{
    return m_fullscreenSlideshow->isChecked();
}

int SettingsDialog::imageMemoryCacheMiB() const
{
    return m_imageMemoryCacheMiB
        ? m_imageMemoryCacheMiB->value() : 256;
}

bool SettingsDialog::persistentThumbnailCacheEnabled() const
{
    return m_persistentThumbnailCacheEnabled
        && m_persistentThumbnailCacheEnabled->isChecked();
}

int SettingsDialog::persistentThumbnailCacheMiB() const
{
    return m_persistentThumbnailCacheMiB
        ? m_persistentThumbnailCacheMiB->value() : 512;
}

QStringList SettingsDialog::toolbarLayout() const
{
    QStringList layout;
    if (!m_toolbarItems)
        return layout;
    layout.reserve(m_toolbarItems->count());
    for (int row = 0; row < m_toolbarItems->count(); ++row) {
        const QListWidgetItem *item = m_toolbarItems->item(row);
        const QString id = item->data(Qt::UserRole).toString();
        layout.append(item->checkState() == Qt::Checked
            ? id : QStringLiteral("!") + id);
    }
    return layout;
}

QStringList SettingsDialog::shortcutLayout() const
{
    QStringList layout;
    if (!m_shortcutItems)
        return layout;
    layout.reserve(m_shortcutItems->rowCount());
    for (int row = 0; row < m_shortcutItems->rowCount();
         ++row) {
        const QTableWidgetItem *command =
            m_shortcutItems->item(row, 0);
        auto *editor = qobject_cast<QKeySequenceEdit *>(
            m_shortcutItems->cellWidget(row, 1));
        if (!command || !editor)
            continue;
        layout.append(
            command->data(Qt::UserRole).toString()
            + QLatin1Char('\t')
            + editor->keySequence().toString(
                QKeySequence::PortableText));
    }
    return layout;
}

QString SettingsDialog::wheelAction() const
{
    return m_wheelAction->currentData().toString();
}

QString SettingsDialog::ctrlWheelAction() const
{
    return m_ctrlWheelAction->currentData().toString();
}

QString SettingsDialog::doubleClickAction() const
{
    return m_doubleClickAction->currentData().toString();
}

QString SettingsDialog::middleButtonAction() const
{
    return m_middleButtonAction->currentData().toString();
}

QString SettingsDialog::backButtonAction() const
{
    return m_backButtonAction->currentData().toString();
}

QString SettingsDialog::forwardButtonAction() const
{
    return m_forwardButtonAction->currentData().toString();
}

#include "editdialogs.moc"
