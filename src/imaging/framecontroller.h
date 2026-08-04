#pragma once

#include <QImage>
#include <QObject>
#include <QString>

#include <memory>

class QMovie;

class FrameController : public QObject
{
    Q_OBJECT

public:
    enum class Kind {
        None,
        Animated,
        MultiPage
    };
    Q_ENUM(Kind)

    explicit FrameController(QObject *parent = nullptr);
    ~FrameController() override;

    bool open(const QString &filePath, QString *errorMessage = nullptr);
    void close();

    [[nodiscard]] Kind kind() const;
    [[nodiscard]] bool isActive() const;
    [[nodiscard]] bool isAnimated() const;
    [[nodiscard]] bool isPlaying() const;
    [[nodiscard]] int currentFrame() const;
    [[nodiscard]] int frameCount() const;
    [[nodiscard]] const QImage &currentImage() const;

public slots:
    void setPlaying(bool playing);
    void first();
    void previous();
    void next();
    void last();
    void setCurrentFrame(int index);

signals:
    void frameChanged(const QImage &image, int currentFrame, int frameCount);
    void stateChanged();

private:
    bool readPage(int index, QString *errorMessage = nullptr);
    void updateMovieFrame(int index);

    Kind m_kind = Kind::None;
    QString m_filePath;
    std::unique_ptr<QMovie> m_movie;
    QImage m_currentImage;
    int m_currentFrame = -1;
    int m_frameCount = 0;
};
