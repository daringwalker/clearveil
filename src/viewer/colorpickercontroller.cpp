#include "colorpickercontroller.h"

#include "colorpickerpanel.h"
#include "imagecanvas.h"

#include <QtMath>

ColorPickerController::ColorPickerController(
    ImageCanvas *canvas, ColorPickerPanel *panel,
    QObject *parent)
    : QObject(parent)
    , m_canvas(canvas)
    , m_panel(panel)
{
    setObjectName(QStringLiteral("colorPickerController"));
    Q_ASSERT(m_canvas);
    Q_ASSERT(m_panel);

    connect(m_canvas, &ImageCanvas::colorHovered,
            this, &ColorPickerController::acceptHoveredSample);
    connect(m_canvas, &ImageCanvas::colorPicked,
            this, &ColorPickerController::acceptPickedSample);
    connect(m_canvas, &ImageCanvas::colorSampleAdjusted,
            this, &ColorPickerController::acceptAdjustedSample);
    connect(m_canvas,
            &ImageCanvas::colorSamplePinnedChanged,
            m_panel, &ColorPickerPanel::setSamplePinned);
    connect(m_panel, &ColorPickerPanel::resumeSamplingRequested,
            this, &ColorPickerController::resumeSampling);
    connect(m_panel, &ColorPickerPanel::historySampleActivated,
            this, &ColorPickerController::acceptHistorySample);
    connect(m_panel, &ColorPickerPanel::copyRequested,
            this, [this](const QString &format) {
        const QString text = formattedColor(format);
        if (!text.isEmpty())
            emit copyTextRequested(text, format);
    });
}

void ColorPickerController::setEnabled(bool enabled)
{
    m_enabled = enabled;
    m_canvas->setColorPickerEnabled(enabled);
    if (!enabled) {
        clearCurrent();
        m_panel->clear();
    }
}

bool ColorPickerController::isEnabled() const
{
    return m_enabled;
}

bool ColorPickerController::isSamplePinned() const
{
    return m_canvas->isColorSamplePinned();
}

void ColorPickerController::resumeSampling()
{
    m_canvas->setColorSamplePinned(false);
}

void ColorPickerController::resetForImage()
{
    clearCurrent();
    m_panel->clear();
    m_panel->clearHistory();
}

bool ColorPickerController::hasSample() const
{
    return m_hasSample;
}

QColor ColorPickerController::color() const
{
    return m_color;
}

QPoint ColorPickerController::imagePosition() const
{
    return m_imagePosition;
}

QString ColorPickerController::formattedColor(
    const QString &format) const
{
    if (!m_hasSample || !m_color.isValid())
        return {};

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
    if (hue < 0.0F)
        hue = 0.0F;
    const QString hsl =
        QStringLiteral("hsl(%1, %2%, %3%)")
            .arg(qRound(hue * 360.0F))
            .arg(qRound(saturation * 100.0F))
            .arg(qRound(lightness * 100.0F));

    if (format == QStringLiteral("rgb"))
        return rgb;
    if (format == QStringLiteral("rgba"))
        return rgba;
    if (format == QStringLiteral("hsl"))
        return hsl;
    if (format == QStringLiteral("all")) {
        return tr("HEX: %1\nRGB: %2\nRGBA: %3\nHSL: %4\nPosition: (%5, %6)")
            .arg(hex, rgb, rgba, hsl)
            .arg(m_imagePosition.x())
            .arg(m_imagePosition.y());
    }
    return hex;
}

void ColorPickerController::acceptHoveredSample(
    const QColor &color, const QPoint &position,
    const QImage &sample)
{
    setCurrent(color, position);
    m_panel->setSample(color, position, sample);
}

void ColorPickerController::acceptPickedSample(
    const QColor &color, const QPoint &position)
{
    setCurrent(color, position);
    m_panel->setColor(color, position);
    m_panel->recordPickedColor(color, position);
}

void ColorPickerController::acceptAdjustedSample(
    const QColor &color, const QPoint &position,
    const QImage &sample)
{
    setCurrent(color, position);
    m_panel->setSample(color, position, sample);
    m_panel->updateLatestPickedColor(
        color, position, sample);
}

void ColorPickerController::acceptHistorySample(
    const QColor &color, const QPoint &position,
    const QImage &)
{
    setCurrent(color, position);
    m_canvas->pinColorSampleAt(position);
}

void ColorPickerController::setCurrent(
    const QColor &color, const QPoint &position)
{
    m_color = color;
    m_imagePosition = position;
    m_hasSample = color.isValid();
}

void ColorPickerController::clearCurrent()
{
    m_color = {};
    m_imagePosition = {};
    m_hasSample = false;
}
