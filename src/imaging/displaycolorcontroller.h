#pragma once

#include "displaycolor.h"
#include "imagesource.h"

#include <QObject>
#include <QPointer>
#include <QThreadPool>

#include <functional>
#include <stop_token>

class ImageCanvas;
class QScreen;
class QWidget;
class QWindow;

class DisplayColorController final : public QObject
{
    Q_OBJECT

public:
    using TargetResolver = std::function<DisplayColorTarget(
        const QString &, const QString &)>;

    explicit DisplayColorController(
        ImageCanvas *canvas, QWidget *hostWindow,
        QObject *parent = nullptr, TargetResolver resolver = {});
    ~DisplayColorController() override;

    void setImage(const QImage &image, bool preserveView = false,
                  const QSize &logicalSize = {},
                  const ImageSourcePtr &imageSource = {});
    void refreshTarget();

    [[nodiscard]] DisplayColorTarget target() const;
    [[nodiscard]] bool isTransforming() const;

signals:
    void targetChanged(const DisplayColorTarget &target);
    void transformingChanged(bool transforming);
    void imageReady();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void ensureWindowBinding();
    void resolveTargetForScreen(QScreen *screen);
    void requestTransform();
    void setTransforming(bool transforming);

    ImageCanvas *m_canvas = nullptr;
    QWidget *m_hostWindow = nullptr;
    QPointer<QWindow> m_boundWindow;
    TargetResolver m_resolver;
    DisplayColorTarget m_target;
    QImage m_sourceImage;
    QSize m_logicalSize;
    ImageSourcePtr m_imageSource;
    bool m_preserveView = false;
    QThreadPool m_targetPool;
    QThreadPool m_transformPool;
    std::stop_source m_targetStopSource;
    std::stop_source m_transformStopSource;
    quint64 m_targetGeneration = 0;
    quint64 m_transformGeneration = 0;
    bool m_transforming = false;
};

Q_DECLARE_METATYPE(DisplayColorTarget)
