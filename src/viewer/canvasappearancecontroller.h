#pragma once

#include <QObject>

class QAction;
class ImageCanvas;

class CanvasAppearanceController final : public QObject
{
    Q_OBJECT

public:
    explicit CanvasAppearanceController(
        ImageCanvas *canvas, QAction *checkerboardAction,
        QObject *parent = nullptr);

    [[nodiscard]] bool transparencyCheckerboardVisible() const;
    void setTransparencyCheckerboardVisible(bool visible);

signals:
    void transparencyCheckerboardVisibleChanged(bool visible);

private:
    ImageCanvas *m_canvas = nullptr;
    QAction *m_checkerboardAction = nullptr;
};
