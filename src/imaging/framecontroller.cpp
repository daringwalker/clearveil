#include "framecontroller.h"

#include <QImageReader>
#include <QMovie>

#include <algorithm>

FrameController::FrameController(QObject *parent)
    : QObject(parent)
{
}

FrameController::~FrameController() = default;

bool FrameController::open(const QString &filePath, QString *errorMessage)
{
    close();

    QImageReader probe(filePath);
    probe.setAutoTransform(true);
    const int imageCount = probe.imageCount();

    if (probe.supportsAnimation()) {
        auto movie = std::make_unique<QMovie>(filePath);
        movie->setCacheMode(QMovie::CacheAll);
        if (!movie->isValid()) {
            if (errorMessage)
                *errorMessage = tr("The animated image could not be decoded.");
            return false;
        }

        m_kind = Kind::Animated;
        m_filePath = filePath;
        m_movie = std::move(movie);
        m_frameCount = std::max(0, m_movie->frameCount());

        connect(m_movie.get(), &QMovie::frameChanged,
                this, &FrameController::updateMovieFrame);
        connect(m_movie.get(), &QMovie::stateChanged, this, [this] {
            emit stateChanged();
        });
        m_movie->start();
        emit stateChanged();
        return true;
    }

    if (imageCount > 1) {
        m_kind = Kind::MultiPage;
        m_filePath = filePath;
        m_frameCount = imageCount;
        if (!readPage(0, errorMessage)) {
            close();
            return false;
        }
        emit stateChanged();
        return true;
    }

    return false;
}

void FrameController::close()
{
    if (m_movie)
        m_movie->stop();
    m_movie.reset();
    m_kind = Kind::None;
    m_filePath.clear();
    m_currentImage = {};
    m_currentFrame = -1;
    m_frameCount = 0;
    emit stateChanged();
}

FrameController::Kind FrameController::kind() const
{
    return m_kind;
}

bool FrameController::isActive() const
{
    return m_kind != Kind::None;
}

bool FrameController::isAnimated() const
{
    return m_kind == Kind::Animated;
}

bool FrameController::isPlaying() const
{
    return m_movie && m_movie->state() == QMovie::Running;
}

int FrameController::currentFrame() const
{
    return m_currentFrame;
}

int FrameController::frameCount() const
{
    return m_frameCount;
}

const QImage &FrameController::currentImage() const
{
    return m_currentImage;
}

void FrameController::setPlaying(bool playing)
{
    if (!m_movie)
        return;
    if (m_movie->state() == QMovie::NotRunning) {
        if (playing)
            m_movie->start();
    } else {
        m_movie->setPaused(!playing);
    }
    emit stateChanged();
}

void FrameController::first()
{
    setCurrentFrame(0);
}

void FrameController::previous()
{
    setCurrentFrame(m_currentFrame - 1);
}

void FrameController::next()
{
    setCurrentFrame(m_currentFrame + 1);
}

void FrameController::last()
{
    if (m_frameCount > 0)
        setCurrentFrame(m_frameCount - 1);
}

void FrameController::setCurrentFrame(int index)
{
    if (!isActive() || m_frameCount <= 0)
        return;
    index = std::clamp(index, 0, m_frameCount - 1);
    if (index == m_currentFrame)
        return;

    if (m_movie) {
        m_movie->setPaused(true);
        if (!m_movie->jumpToFrame(index))
            return;
        emit stateChanged();
        return;
    }
    readPage(index);
}

bool FrameController::readPage(int index, QString *errorMessage)
{
    QImageReader reader(m_filePath);
    reader.setAutoTransform(true);
    if (!reader.jumpToImage(index)) {
        if (errorMessage)
            *errorMessage = reader.errorString();
        return false;
    }
    const QImage image = reader.read();
    if (image.isNull()) {
        if (errorMessage)
            *errorMessage = reader.errorString();
        return false;
    }

    m_currentFrame = index;
    m_currentImage = image;
    emit frameChanged(m_currentImage, m_currentFrame, m_frameCount);
    emit stateChanged();
    return true;
}

void FrameController::updateMovieFrame(int index)
{
    if (!m_movie)
        return;
    m_currentFrame = index;
    m_currentImage = m_movie->currentImage();
    const int discoveredCount = m_movie->frameCount();
    if (discoveredCount > 0)
        m_frameCount = discoveredCount;
    emit frameChanged(m_currentImage, m_currentFrame, m_frameCount);
}
