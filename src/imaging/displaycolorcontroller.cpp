#include "displaycolorcontroller.h"

#include "imagecanvas.h"

#include <QEvent>
#include <QFutureWatcher>
#include <QGuiApplication>
#include <QScreen>
#include <QWidget>
#include <QWindow>
#include <QtConcurrentRun>

#include <utility>

DisplayColorController::DisplayColorController(
    ImageCanvas *canvas, QWidget *hostWindow, QObject *parent,
    TargetResolver resolver)
    : QObject(parent)
    , m_canvas(canvas)
    , m_hostWindow(hostWindow)
    , m_resolver(resolver ? std::move(resolver)
                          : TargetResolver(DisplayColor::resolveAutomaticTarget))
{
    Q_ASSERT(m_canvas);
    Q_ASSERT(m_hostWindow);
    m_target.colorSpace = QColorSpace(QColorSpace::SRgb);
    m_target.source = DisplayColorTargetSource::SrgbFallback;
    m_targetPool.setMaxThreadCount(1);
    m_targetPool.setExpiryTimeout(10'000);
    m_transformPool.setMaxThreadCount(1);
    m_transformPool.setExpiryTimeout(10'000);
    m_hostWindow->installEventFilter(this);
    ensureWindowBinding();
    refreshTarget();
}

DisplayColorController::~DisplayColorController()
{
    m_targetStopSource.request_stop();
    m_transformStopSource.request_stop();
    m_targetPool.clear();
    m_transformPool.clear();
    if (m_hostWindow)
        m_hostWindow->removeEventFilter(this);
}

void DisplayColorController::setImage(
    const QImage &image, bool preserveView,
    const QSize &logicalSize,
    const ImageSourcePtr &imageSource)
{
    m_sourceImage = image;
    m_logicalSize = logicalSize.isValid()
        ? logicalSize : image.size();
    m_imageSource = imageSource;
    m_preserveView = preserveView;
    ++m_transformGeneration;
    m_transformStopSource.request_stop();
    m_transformPool.clear();
    if (image.isNull()) {
        setTransforming(false);
        m_canvas->setColorManagedImage(
            {}, {}, preserveView, {}, {});
        emit imageReady();
        return;
    }
    requestTransform();
}

void DisplayColorController::refreshTarget()
{
    ensureWindowBinding();
    QScreen *screen = m_boundWindow
        ? m_boundWindow->screen() : m_hostWindow->screen();
    resolveTargetForScreen(screen);
}

DisplayColorTarget DisplayColorController::target() const
{
    return m_target;
}

bool DisplayColorController::isTransforming() const
{
    return m_transforming;
}

bool DisplayColorController::eventFilter(
    QObject *watched, QEvent *event)
{
    if (watched == m_hostWindow
        && (event->type() == QEvent::Show
            || event->type() == QEvent::WinIdChange)) {
        ensureWindowBinding();
        refreshTarget();
    }
    return QObject::eventFilter(watched, event);
}

void DisplayColorController::ensureWindowBinding()
{
    QWindow *window = m_hostWindow ? m_hostWindow->windowHandle() : nullptr;
    if (!window || m_boundWindow == window)
        return;
    if (m_boundWindow)
        disconnect(m_boundWindow, nullptr, this, nullptr);
    m_boundWindow = window;
    connect(window, &QWindow::screenChanged,
            this, [this](QScreen *screen) {
        resolveTargetForScreen(screen);
    });
}

void DisplayColorController::resolveTargetForScreen(QScreen *screen)
{
    m_targetStopSource.request_stop();
    m_targetStopSource = std::stop_source();
    ++m_targetGeneration;
    const quint64 generation = m_targetGeneration;
    m_targetPool.clear();
    const QString outputName = screen ? screen->name() : QString();
    const QString platformName = QGuiApplication::platformName();
    const TargetResolver resolver = m_resolver;
    const std::stop_token stopToken = m_targetStopSource.get_token();
    auto future = QtConcurrent::run(
        &m_targetPool,
        [resolver, outputName, platformName, stopToken] {
            if (stopToken.stop_requested())
                return DisplayColorTarget();
            return resolver(outputName, platformName);
        });
    auto *watcher = new QFutureWatcher<DisplayColorTarget>(this);
    connect(watcher, &QFutureWatcher<DisplayColorTarget>::finished,
            this, [this, watcher, generation] {
        const DisplayColorTarget resolved = watcher->result();
        watcher->deleteLater();
        if (generation != m_targetGeneration || !resolved.isValid())
            return;
        const bool changed = resolved.colorSpace != m_target.colorSpace
            || resolved.source != m_target.source
            || resolved.outputName != m_target.outputName
            || resolved.profilePath != m_target.profilePath;
        m_target = resolved;
        if (changed) {
            emit targetChanged(m_target);
            requestTransform();
        }
    });
    watcher->setFuture(future);
}

void DisplayColorController::requestTransform()
{
    if (m_sourceImage.isNull())
        return;
    m_transformStopSource.request_stop();
    m_transformStopSource = std::stop_source();
    ++m_transformGeneration;
    const quint64 generation = m_transformGeneration;
    m_transformPool.clear();
    setTransforming(true);
    const QImage source = m_sourceImage;
    const QSize logicalSize = m_logicalSize;
    const ImageSourcePtr imageSource = m_imageSource;
    const bool preserveView = m_preserveView;
    const QColorSpace targetColorSpace = m_target.colorSpace;
    const std::stop_token stopToken =
        m_transformStopSource.get_token();
    auto future = QtConcurrent::run(
        &m_transformPool,
        [source, targetColorSpace, stopToken] {
            return DisplayColor::transform(
                source, targetColorSpace, stopToken);
        });
    auto *watcher =
        new QFutureWatcher<DisplayColorTransformResult>(this);
    connect(watcher,
            &QFutureWatcher<DisplayColorTransformResult>::finished,
            this, [this, watcher, generation, source, preserveView,
                   logicalSize, imageSource] {
        const DisplayColorTransformResult result = watcher->result();
        watcher->deleteLater();
        if (generation != m_transformGeneration)
            return;
        setTransforming(false);
        if (!result.succeeded())
            return;
        m_canvas->setColorManagedImage(
            source, result.image, preserveView,
            logicalSize, imageSource);
        emit imageReady();
    });
    watcher->setFuture(future);
}

void DisplayColorController::setTransforming(bool transforming)
{
    if (m_transforming == transforming)
        return;
    m_transforming = transforming;
    emit transformingChanged(transforming);
}
