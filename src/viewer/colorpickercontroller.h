#pragma once

#include <QColor>
#include <QImage>
#include <QObject>
#include <QPoint>
#include <QString>

class ColorPickerPanel;
class ImageCanvas;

class ColorPickerController final : public QObject
{
    Q_OBJECT

public:
    explicit ColorPickerController(
        ImageCanvas *canvas, ColorPickerPanel *panel,
        QObject *parent = nullptr);

    void setEnabled(bool enabled);
    [[nodiscard]] bool isEnabled() const;
    [[nodiscard]] bool isSamplePinned() const;
    void resumeSampling();
    void resetForImage();

    [[nodiscard]] bool hasSample() const;
    [[nodiscard]] QColor color() const;
    [[nodiscard]] QPoint imagePosition() const;
    [[nodiscard]] QString formattedColor(
        const QString &format) const;

signals:
    void copyTextRequested(const QString &text,
                           const QString &format);

private:
    void acceptHoveredSample(
        const QColor &color, const QPoint &position,
        const QImage &sample);
    void acceptPickedSample(
        const QColor &color, const QPoint &position);
    void acceptAdjustedSample(
        const QColor &color, const QPoint &position,
        const QImage &sample);
    void acceptHistorySample(
        const QColor &color, const QPoint &position,
        const QImage &sample);
    void setCurrent(const QColor &color,
                    const QPoint &position);
    void clearCurrent();

    ImageCanvas *m_canvas = nullptr;
    ColorPickerPanel *m_panel = nullptr;
    QColor m_color;
    QPoint m_imagePosition;
    bool m_enabled = false;
    bool m_hasSample = false;
};
