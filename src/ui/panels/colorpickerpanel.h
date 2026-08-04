#pragma once

#include <QColor>
#include <QImage>
#include <QPoint>
#include <QWidget>

class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QToolButton;

class PixelMagnifierWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit PixelMagnifierWidget(QWidget *parent = nullptr);

    void setSample(const QImage &sample);
    void clear();
    [[nodiscard]] QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QImage m_sample;
};

class ColorPickerPanel final : public QWidget
{
    Q_OBJECT

public:
    explicit ColorPickerPanel(QWidget *parent = nullptr);

    [[nodiscard]] int preferredHeight() const;

    void setSample(const QColor &color, const QPoint &imagePosition,
                   const QImage &sample);
    void setColor(const QColor &color, const QPoint &imagePosition);
    void setSamplePinned(bool pinned);
    void recordPickedColor(const QColor &color,
                           const QPoint &imagePosition);
    void updateLatestPickedColor(
        const QColor &color,
        const QPoint &imagePosition,
        const QImage &sample);
    void clearHistory();
    void clear();
    void showCopyConfirmation(const QString &format);

signals:
    void preferredHeightChanged();
    void copyRequested(const QString &format);
    void resumeSamplingRequested();
    void historySampleActivated(const QColor &color,
                                const QPoint &imagePosition,
                                const QImage &sample);

private:
    QToolButton *addColorRow(const QString &label, const QString &format,
                             QLineEdit **valueField);
    void activateHistoryItem(QListWidgetItem *item);
    void updateColorValues();
    void updateHistoryItem(
        QListWidgetItem *item, const QColor &color,
        const QPoint &imagePosition,
        const QImage &sample);
    void updateHistoryHeight();
    void setCopyControlsEnabled(bool enabled);

    PixelMagnifierWidget *m_magnifier = nullptr;
    QLabel *m_swatch = nullptr;
    QLabel *m_positionX = nullptr;
    QLabel *m_positionY = nullptr;
    QLineEdit *m_hex = nullptr;
    QLineEdit *m_rgb = nullptr;
    QLineEdit *m_rgba = nullptr;
    QLineEdit *m_hsl = nullptr;
    QLabel *m_feedback = nullptr;
    QListWidget *m_historyList = nullptr;
    QToolButton *m_samplingStateButton = nullptr;
    QToolButton *m_primaryCopyButton = nullptr;
    QList<QToolButton *> m_copyButtons;
    QColor m_color;
    QPoint m_imagePosition;
    QImage m_currentSample;
};
