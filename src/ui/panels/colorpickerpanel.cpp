#include "colorpickerpanel.h"

#include <QCoreApplication>
#include <QFormLayout>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPainter>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <utility>

namespace {
class CoordinateLegendWidget final : public QWidget
{
public:
    explicit CoordinateLegendWidget(QWidget *parent)
        : QWidget(parent)
    {
        setObjectName(QStringLiteral("coordinateAxisLegend"));
        setAccessibleName(
            QCoreApplication::translate(
                "ColorPickerPanel", "Image coordinate axes"));
        setFixedSize(32, 32);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        const QColor color = palette().color(QPalette::Text);
        painter.setPen(QPen(color, 1.5));
        const QPoint origin(8, 23);
        painter.drawLine(origin, QPoint(25, 23));
        painter.drawLine(origin, QPoint(8, 6));
        painter.drawLine(QPoint(25, 23), QPoint(21, 20));
        painter.drawLine(QPoint(25, 23), QPoint(21, 26));
        painter.drawLine(QPoint(8, 6), QPoint(5, 10));
        painter.drawLine(QPoint(8, 6), QPoint(11, 10));
        painter.setFont(QFont(painter.font().family(), 7));
        painter.drawText(QRect(22, 13, 10, 10),
                         Qt::AlignCenter, QStringLiteral("X"));
        painter.drawText(QRect(11, 0, 10, 10),
                         Qt::AlignCenter, QStringLiteral("Y"));
    }
};
}

PixelMagnifierWidget::PixelMagnifierWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("pixelMagnifier"));
    setAccessibleName(tr("Pixel magnifier"));
    setFixedSize(112, 112);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void PixelMagnifierWidget::setSample(const QImage &sample)
{
    m_sample = sample;
    update();
}

void PixelMagnifierWidget::clear()
{
    m_sample = {};
    update();
}

QSize PixelMagnifierWidget::sizeHint() const
{
    return {112, 112};
}

void PixelMagnifierWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), palette().color(QPalette::Base));

    const QRect available = rect().adjusted(8, 8, -8, -8);
    if (m_sample.isNull() || available.isEmpty()) {
        painter.setPen(palette().color(
            QPalette::Disabled, QPalette::Text));
        painter.drawText(
            available, Qt::AlignCenter | Qt::TextWordWrap,
            tr("Move the pointer over the image to inspect pixels"));
        painter.setPen(palette().color(QPalette::Mid));
        painter.drawRect(rect().adjusted(0, 0, -1, -1));
        return;
    }

    const int columns = m_sample.width();
    const int rows = m_sample.height();
    const int cell = std::max(
        1, std::min(available.width() / columns,
                    available.height() / rows));
    const QSize gridSize(columns * cell, rows * cell);
    const QRect grid(
        available.center().x() - gridSize.width() / 2,
        available.center().y() - gridSize.height() / 2,
        gridSize.width(), gridSize.height());

    const QColor missingLight(205, 208, 213);
    const QColor missingDark(162, 166, 172);
    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            const QRect pixelRect(
                grid.left() + column * cell,
                grid.top() + row * cell, cell, cell);
            const QColor color = m_sample.pixelColor(column, row);
            if (color.alpha() == 0) {
                painter.fillRect(
                    pixelRect,
                    (row + column) % 2
                        ? missingLight : missingDark);
            } else {
                painter.fillRect(pixelRect, color);
            }
        }
    }

    if (cell >= 8) {
        painter.setPen(QColor(0, 0, 0, 46));
        for (int column = 0; column <= columns; ++column) {
            const int x = grid.left() + column * cell;
            painter.drawLine(x, grid.top(), x, grid.bottom());
        }
        for (int row = 0; row <= rows; ++row) {
            const int y = grid.top() + row * cell;
            painter.drawLine(grid.left(), y, grid.right(), y);
        }
    }

    const int centerColumn = columns / 2;
    const int centerRow = rows / 2;
    const QRect centerPixel(
        grid.left() + centerColumn * cell,
        grid.top() + centerRow * cell, cell, cell);
    painter.setBrush(Qt::NoBrush);
    const QColor centerColor =
        m_sample.pixelColor(centerColumn, centerRow);
    const QColor contrast =
        qGray(centerColor.rgb()) < 128
        ? QColor(255, 255, 255, 245)
        : QColor(20, 20, 20, 235);
    painter.setPen(QPen(contrast, 2));
    painter.drawRect(
        centerPixel.adjusted(-2, -2, 1, 1));

    painter.setPen(palette().color(QPalette::Mid));
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
}

ColorPickerPanel::ColorPickerPanel(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("colorPickerPanel"));
    setAccessibleName(tr("Color picker panel"));
    setMinimumWidth(210);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(6);

    auto *previewCard = new QFrame(this);
    previewCard->setObjectName(
        QStringLiteral("colorPickerPreviewCard"));
    previewCard->setSizePolicy(
        QSizePolicy::Preferred, QSizePolicy::Maximum);
    auto *previewRow = new QHBoxLayout(previewCard);
    previewRow->setContentsMargins(0, 0, 0, 0);
    previewRow->setSpacing(8);
    m_magnifier = new PixelMagnifierWidget(this);
    previewRow->addWidget(m_magnifier, 0, Qt::AlignTop);

    auto *summary = new QVBoxLayout;
    summary->setContentsMargins(0, 2, 0, 0);
    summary->setSpacing(6);
    m_swatch = new QLabel(this);
    m_swatch->setObjectName(QStringLiteral("pickedColorSwatch"));
    m_swatch->setAccessibleName(tr("Picked color swatch"));
    m_swatch->setFixedSize(32, 32);
    auto *visuals = new QHBoxLayout;
    visuals->setContentsMargins(0, 0, 0, 0);
    visuals->setSpacing(4);
    visuals->addWidget(m_swatch);
    visuals->addWidget(new CoordinateLegendWidget(this));
    visuals->addStretch();
    summary->addLayout(visuals);
    m_positionX = new QLabel(this);
    m_positionX->setObjectName(QStringLiteral("pickedColorPositionX"));
    m_positionX->setAccessibleName(tr("Pixel X coordinate"));
    m_positionX->setTextInteractionFlags(
        Qt::TextSelectableByMouse
        | Qt::TextSelectableByKeyboard);
    m_positionX->setText(QStringLiteral("X  —"));
    summary->addWidget(m_positionX);
    m_positionY = new QLabel(this);
    m_positionY->setObjectName(QStringLiteral("pickedColorPositionY"));
    m_positionY->setAccessibleName(tr("Pixel Y coordinate"));
    m_positionY->setTextInteractionFlags(
        Qt::TextSelectableByMouse
        | Qt::TextSelectableByKeyboard);
    m_positionY->setText(QStringLiteral("Y  —"));
    summary->addWidget(m_positionY);
    summary->addStretch();
    previewRow->addLayout(summary, 1);
    layout->addWidget(previewCard);

    auto *form = new QFormLayout;
    form->setContentsMargins(0, 0, 0, 0);
    form->setHorizontalSpacing(6);
    form->setVerticalSpacing(3);
    addColorRow(QStringLiteral("HEX"), QStringLiteral("hex"), &m_hex);
    addColorRow(QStringLiteral("RGB"), QStringLiteral("rgb"), &m_rgb);
    addColorRow(QStringLiteral("RGBA"), QStringLiteral("rgba"), &m_rgba);
    addColorRow(QStringLiteral("HSL"), QStringLiteral("hsl"), &m_hsl);
    const QList<QPair<QString, QLineEdit *>> fields{
        {QStringLiteral("HEX"), m_hex},
        {QStringLiteral("RGB"), m_rgb},
        {QStringLiteral("RGBA"), m_rgba},
        {QStringLiteral("HSL"), m_hsl},
    };
    for (int index = 0; index < fields.size(); ++index) {
        auto *row = new QWidget(this);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(3);
        rowLayout->addWidget(fields.at(index).second, 1);
        rowLayout->addWidget(m_copyButtons.at(index));
        form->addRow(fields.at(index).first, row);
    }
    layout->addLayout(form);

    auto *historyHeader = new QHBoxLayout;
    auto *historyLabel = new QLabel(
        tr("Picked colors"), this);
    historyLabel->setObjectName(
        QStringLiteral("colorHistoryLabel"));
    historyHeader->addWidget(historyLabel);
    historyHeader->addStretch();
    auto *clearHistoryButton = new QToolButton(this);
    clearHistoryButton->setObjectName(
        QStringLiteral("clearColorHistoryButton"));
    clearHistoryButton->setAccessibleName(
        tr("Clear color history"));
    clearHistoryButton->setText(tr("Clear"));
    clearHistoryButton->setToolButtonStyle(
        Qt::ToolButtonTextBesideIcon);
    clearHistoryButton->setIcon(QIcon::fromTheme(
        QStringLiteral("edit-clear-history"),
        style()->standardIcon(QStyle::SP_DialogResetButton)));
    connect(clearHistoryButton, &QToolButton::clicked,
            this, &ColorPickerPanel::clearHistory);
    historyHeader->addWidget(clearHistoryButton);
    layout->addLayout(historyHeader);

    m_historyList = new QListWidget(this);
    m_historyList->setObjectName(
        QStringLiteral("colorHistoryList"));
    m_historyList->setAccessibleName(
        tr("Picked color history"));
    m_historyList->setSelectionMode(
        QAbstractItemView::SingleSelection);
    m_historyList->setSizePolicy(
        QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_historyList->setSpacing(1);
    connect(
        m_historyList,
        &QListWidget::currentItemChanged,
        this,
        [this](QListWidgetItem *current,
               QListWidgetItem *) {
            activateHistoryItem(current);
        });
    layout->addWidget(m_historyList);
    updateHistoryHeight();

    auto *bottom = new QHBoxLayout;
    m_samplingStateButton = new QToolButton(this);
    m_samplingStateButton->setObjectName(
        QStringLiteral("resumeColorSamplingButton"));
    m_samplingStateButton->setAccessibleName(
        tr("Continue picking colors"));
    m_samplingStateButton->setToolButtonStyle(
        Qt::ToolButtonTextBesideIcon);
    connect(m_samplingStateButton,
            &QToolButton::clicked,
            this, &ColorPickerPanel::resumeSamplingRequested);
    bottom->addWidget(m_samplingStateButton);

    m_feedback = new QLabel(this);
    m_feedback->setObjectName(
        QStringLiteral("colorCopyFeedback"));
    m_feedback->setAccessibleName(
        tr("Color copy status"));
    bottom->addWidget(m_feedback, 1);

    m_primaryCopyButton = new QToolButton(this);
    m_primaryCopyButton->setObjectName(
        QStringLiteral("copyPickedColorButton"));
    m_primaryCopyButton->setAccessibleName(
        tr("Copy picked color"));
    m_primaryCopyButton->setText(tr("Copy HEX"));
    m_primaryCopyButton->setToolButtonStyle(
        Qt::ToolButtonTextBesideIcon);
    m_primaryCopyButton->setIcon(QIcon::fromTheme(
        QStringLiteral("edit-copy"),
        style()->standardIcon(QStyle::SP_DialogSaveButton)));
    m_primaryCopyButton->setPopupMode(
        QToolButton::MenuButtonPopup);
    auto *menu = new QMenu(m_primaryCopyButton);
    for (const auto &[text, format] :
         QList<QPair<QString, QString>>{
             {tr("Copy HEX"), QStringLiteral("hex")},
             {tr("Copy RGB"), QStringLiteral("rgb")},
             {tr("Copy RGBA"), QStringLiteral("rgba")},
             {tr("Copy HSL"), QStringLiteral("hsl")},
             {tr("Copy all color information"),
              QStringLiteral("all")},
         }) {
        QAction *action = menu->addAction(text);
        connect(action, &QAction::triggered,
                this, [this, format] {
            emit copyRequested(format);
        });
    }
    m_primaryCopyButton->setMenu(menu);
    connect(m_primaryCopyButton, &QToolButton::clicked,
            this, [this] {
        emit copyRequested(QStringLiteral("hex"));
    });
    bottom->addWidget(m_primaryCopyButton);
    layout->addLayout(bottom);

    clear();
}

int ColorPickerPanel::preferredHeight() const
{
    return layout() ? layout()->sizeHint().height()
                    : sizeHint().height();
}

QToolButton *ColorPickerPanel::addColorRow(
    const QString &label, const QString &format,
    QLineEdit **valueField)
{
    auto *field = new QLineEdit(this);
    field->setObjectName(
        QStringLiteral("pickedColor%1").arg(label));
    field->setAccessibleName(
        tr("%1 color value").arg(label));
    field->setReadOnly(true);
    field->setClearButtonEnabled(false);
    *valueField = field;

    auto *button = new QToolButton(this);
    button->setObjectName(
        QStringLiteral("copy%1ColorButton").arg(label));
    button->setAccessibleName(
        tr("Copy %1 color value").arg(label));
    button->setToolTip(
        tr("Copy %1").arg(label));
    button->setIcon(QIcon::fromTheme(
        QStringLiteral("edit-copy"),
        style()->standardIcon(QStyle::SP_DialogSaveButton)));
    connect(button, &QToolButton::clicked,
            this, [this, format] {
        emit copyRequested(format);
    });
    m_copyButtons.append(button);
    return button;
}

void ColorPickerPanel::setSample(
    const QColor &color, const QPoint &imagePosition,
    const QImage &sample)
{
    m_currentSample = sample;
    m_magnifier->setSample(sample);
    setColor(color, imagePosition);
}

void ColorPickerPanel::setColor(
    const QColor &color, const QPoint &imagePosition)
{
    m_color = color;
    m_imagePosition = imagePosition;
    updateColorValues();
    setCopyControlsEnabled(color.isValid());
}

void ColorPickerPanel::setSamplePinned(bool pinned)
{
    m_samplingStateButton->setEnabled(pinned);
    m_samplingStateButton->setText(
        pinned ? tr("Continue picking")
               : tr("Live preview"));
    m_samplingStateButton->setIcon(QIcon::fromTheme(
        pinned ? QStringLiteral("view-refresh")
               : QStringLiteral("color-picker"),
        style()->standardIcon(
            pinned ? QStyle::SP_BrowserReload
                   : QStyle::SP_DialogHelpButton)));
    m_samplingStateButton->setToolTip(
        pinned
            ? tr("The selected pixel is fixed. Use the arrow keys to adjust it, or click to resume live preview.")
            : tr("Move the pointer over the image, then click to fix a pixel."));
}

void ColorPickerPanel::recordPickedColor(
    const QColor &color, const QPoint &imagePosition)
{
    if (!color.isValid())
        return;
    if (m_historyList->count() > 0) {
        const QListWidgetItem *first =
            m_historyList->item(0);
        if (first->data(Qt::UserRole).value<QColor>()
                == color
            && first->data(Qt::UserRole + 1).toPoint()
                == imagePosition) {
            return;
        }
    }

    auto *item = new QListWidgetItem;
    m_historyList->insertItem(0, item);
    updateHistoryItem(
        item, color, imagePosition, m_currentSample);
    m_historyList->setCurrentItem(item);
    constexpr int maximumHistoryItems = 30;
    while (m_historyList->count()
           > maximumHistoryItems) {
        delete m_historyList->takeItem(
            maximumHistoryItems);
    }
    updateHistoryHeight();
}

void ColorPickerPanel::activateHistoryItem(
    QListWidgetItem *item)
{
    if (!item)
        return;
    const QColor color =
        item->data(Qt::UserRole).value<QColor>();
    const QPoint position =
        item->data(Qt::UserRole + 1).toPoint();
    const QImage sample =
        item->data(Qt::UserRole + 2).value<QImage>();
    if (!color.isValid())
        return;
    if (sample.isNull())
        setColor(color, position);
    else
        setSample(color, position, sample);
    setSamplePinned(true);
    emit historySampleActivated(
        color, position, sample);
}

void ColorPickerPanel::updateLatestPickedColor(
    const QColor &color, const QPoint &imagePosition,
    const QImage &sample)
{
    if (!color.isValid())
        return;
    m_currentSample = sample;
    if (m_historyList->count() == 0) {
        recordPickedColor(color, imagePosition);
        return;
    }
    updateHistoryItem(
        m_historyList->item(0), color,
        imagePosition, sample);
}

void ColorPickerPanel::updateHistoryItem(
    QListWidgetItem *item, const QColor &color,
    const QPoint &imagePosition, const QImage &sample)
{
    if (!item)
        return;
    QPixmap swatch(24, 24);
    swatch.fill(Qt::transparent);
    {
        QPainter painter(&swatch);
        painter.fillRect(
            swatch.rect().adjusted(1, 1, -1, -1),
            color);
        painter.setPen(palette().color(QPalette::Text));
        painter.drawRect(
            swatch.rect().adjusted(0, 0, -1, -1));
    }
    item->setIcon(QIcon(swatch));
    item->setText(
        QStringLiteral("%1  ·  %2, %3")
            .arg(color.name(QColor::HexRgb).toUpper())
            .arg(imagePosition.x())
            .arg(imagePosition.y()));
    item->setData(Qt::UserRole, color);
    item->setData(Qt::UserRole + 1, imagePosition);
    item->setData(Qt::UserRole + 2, sample);
    item->setToolTip(
        tr("%1 at (%2, %3)")
            .arg(color.name(QColor::HexRgb).toUpper())
            .arg(imagePosition.x())
            .arg(imagePosition.y()));
}

void ColorPickerPanel::clearHistory()
{
    m_historyList->clear();
    updateHistoryHeight();
}

void ColorPickerPanel::updateHistoryHeight()
{
    if (!m_historyList)
        return;
    const int visibleRows = std::clamp(
        m_historyList->count(), 1, 3);
    const int rowHeight = m_historyList->fontMetrics().height() + 10;
    m_historyList->setFixedHeight(
        visibleRows * rowHeight
        + m_historyList->frameWidth() * 2 + 2);
    updateGeometry();
    emit preferredHeightChanged();
}

void ColorPickerPanel::clear()
{
    m_color = {};
    m_imagePosition = {};
    m_currentSample = {};
    m_magnifier->clear();
    for (QLineEdit *field :
         {m_hex, m_rgb, m_rgba, m_hsl}) {
        field->setText(QStringLiteral("—"));
    }
    m_positionX->setText(QStringLiteral("X  —"));
    m_positionY->setText(QStringLiteral("Y  —"));
    m_swatch->setPixmap({});
    m_swatch->setStyleSheet(
        QStringLiteral("border: 1px solid palette(mid);"));
    m_feedback->clear();
    setSamplePinned(false);
    setCopyControlsEnabled(false);
}

void ColorPickerPanel::showCopyConfirmation(
    const QString &format)
{
    m_feedback->setText(
        tr("%1 copied").arg(format.toUpper()));
    QTimer::singleShot(2200, m_feedback, [this] {
        m_feedback->clear();
    });
}

void ColorPickerPanel::updateColorValues()
{
    if (!m_color.isValid())
        return;

    const QString hex =
        m_color.name(QColor::HexRgb).toUpper();
    const QString rgb =
        QStringLiteral("rgb(%1, %2, %3)")
            .arg(m_color.red())
            .arg(m_color.green())
            .arg(m_color.blue());
    const QString rgba =
        QStringLiteral("rgba(%1, %2, %3, %4)")
            .arg(m_color.red())
            .arg(m_color.green())
            .arg(m_color.blue())
            .arg(QString::number(
                m_color.alphaF(), 'f', 3));
    float hue = 0.0F;
    float saturation = 0.0F;
    float lightness = 0.0F;
    float alpha = 0.0F;
    m_color.getHslF(
        &hue, &saturation, &lightness, &alpha);
    if (hue < 0.0)
        hue = 0.0;
    const QString hsl =
        QStringLiteral("hsl(%1, %2%, %3%)")
            .arg(qRound(hue * 360.0))
            .arg(qRound(saturation * 100.0))
            .arg(qRound(lightness * 100.0));

    m_hex->setText(hex);
    m_rgb->setText(rgb);
    m_rgba->setText(rgba);
    m_hsl->setText(hsl);
    m_positionX->setText(
        QStringLiteral("X  %1").arg(m_imagePosition.x()));
    m_positionY->setText(
        QStringLiteral("Y  %1").arg(m_imagePosition.y()));

    QPixmap swatch(m_swatch->size());
    swatch.fill(Qt::transparent);
    QPainter painter(&swatch);
    painter.fillRect(
        swatch.rect().adjusted(1, 1, -1, -1),
        m_color);
    painter.setPen(palette().color(QPalette::Text));
    painter.drawRect(
        swatch.rect().adjusted(0, 0, -1, -1));
    m_swatch->setStyleSheet({});
    m_swatch->setPixmap(swatch);
}

void ColorPickerPanel::setCopyControlsEnabled(bool enabled)
{
    for (QToolButton *button : std::as_const(m_copyButtons))
        button->setEnabled(enabled);
    m_primaryCopyButton->setEnabled(enabled);
}
