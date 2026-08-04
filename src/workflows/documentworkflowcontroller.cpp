#include "documentworkflowcontroller.h"

#include "framecontroller.h"
#include "imagedecoder.h"
#include "imagedocument.h"

#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QTimer>

#include <algorithm>

DocumentWorkflowController::DocumentWorkflowController(
    ImageDocument *document,
    FrameController *frames,
    QObject *parent)
    : QObject(parent)
    , m_document(document)
    , m_frames(frames)
    , m_fileWatcher(new QFileSystemWatcher(this))
    , m_reloadTimer(new QTimer(this))
{
    Q_ASSERT(m_document);
    Q_ASSERT(m_frames);
    m_reloadTimer->setSingleShot(true);
    m_reloadTimer->setInterval(180);
    connect(m_reloadTimer, &QTimer::timeout,
            this, &DocumentWorkflowController::externalReloadRequested);
    connect(m_fileWatcher, &QFileSystemWatcher::fileChanged,
            this, [this](const QString &path) {
        if (!m_document
            || QFileInfo(path).absoluteFilePath()
                != QFileInfo(m_document->filePath()).absoluteFilePath()) {
            return;
        }
        if (m_document->isModified()) {
            emit externalChangeBlocked(path);
            return;
        }
        m_reloadTimer->start();
    });
}

bool DocumentWorkflowController::loadPath(
    const QString &filePath,
    QString *errorMessage)
{
    m_frames->close();
    if (!m_document->load(filePath, errorMessage))
        return false;
    openFrames(m_document->filePath());
    refreshFileWatch();
    emit currentFileChanged(m_document->filePath());
    return true;
}

bool DocumentWorkflowController::loadDecoded(
    const ImageLoadResult &result,
    QString *errorMessage)
{
    if (!result.succeeded()) {
        if (errorMessage)
            *errorMessage = result.error;
        return false;
    }
    m_frames->close();
    if (!m_document->loadDecoded(result, errorMessage)) {
        return false;
    }
    openFrames(m_document->filePath());
    refreshFileWatch();
    emit currentFileChanged(m_document->filePath());
    return true;
}

bool DocumentWorkflowController::loadClipboardImage(
    const QImage &image,
    QString *errorMessage)
{
    m_frames->close();
    if (!m_document->loadImage(image, errorMessage))
        return false;
    refreshFileWatch();
    emit currentFileChanged({});
    return true;
}

ImageExportService::Result
DocumentWorkflowController::saveAs(
    const QString &filePath)
{
    if (m_frames->isActive()) {
        return ImageExportService::writeAtomically(
            displayedImage(), filePath);
    }
    ImageExportService::Result result =
        m_document->saveAsResult(filePath);
    if (result.succeeded()) {
        refreshFileWatch();
        emit currentFileChanged(result.filePath);
    }
    return result;
}

bool DocumentWorkflowController::reload(
    QString *errorMessage)
{
    const QString path = m_document->filePath();
    if (path.isEmpty())
        return false;

    const bool wasPlaying = m_frames->isPlaying();
    const int oldFrame = m_frames->currentFrame();
    if (!loadPath(path, errorMessage)) {
        refreshFileWatch();
        return false;
    }
    if (m_frames->isActive() && oldFrame >= 0) {
        m_frames->setCurrentFrame(
            std::min(oldFrame, m_frames->frameCount() - 1));
    }
    if (m_frames->isAnimated())
        m_frames->setPlaying(wasPlaying);
    return true;
}

void DocumentWorkflowController::clear()
{
    m_reloadTimer->stop();
    m_frames->close();
    m_document->clear();
    refreshFileWatch();
    emit currentFileChanged({});
}

const QImage &DocumentWorkflowController::displayedImage() const
{
    if (m_frames->isActive()
        && !m_frames->currentImage().isNull()) {
        return m_frames->currentImage();
    }
    return m_document->image();
}

QString DocumentWorkflowController::watchedPath() const
{
    const QStringList files = m_fileWatcher->files();
    return files.isEmpty() ? QString() : files.constFirst();
}

void DocumentWorkflowController::openFrames(
    const QString &filePath)
{
    // A false return is normal for a single-frame image. The document remains
    // fully usable, so frame probing must never turn a successful load into an
    // error.
    QString frameError;
    m_frames->open(filePath, &frameError);
}

void DocumentWorkflowController::refreshFileWatch()
{
    m_reloadTimer->stop();
    const QStringList watched = m_fileWatcher->files();
    if (!watched.isEmpty())
        m_fileWatcher->removePaths(watched);
    const QString path = m_document->filePath();
    if (!path.isEmpty() && QFileInfo::exists(path))
        m_fileWatcher->addPath(path);
}
