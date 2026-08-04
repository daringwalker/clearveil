#pragma once

#include "imagedecoder.h"
#include "imageexportservice.h"
#include "imagesource.h"

#include <QImage>
#include <QObject>
#include <QRect>
#include <QSize>
#include <QString>
#include <QVector>

class ImageDocument : public QObject
{
    Q_OBJECT

public:
    explicit ImageDocument(QObject *parent = nullptr);

    [[nodiscard]] static ImageLoadResult decodeFile(
        const QString &filePath);
    bool load(const QString &filePath, QString *errorMessage = nullptr);
    bool loadDecoded(const QString &filePath, const QImage &image,
                     QString *errorMessage = nullptr);
    bool loadDecoded(const ImageLoadResult &result,
                     QString *errorMessage = nullptr);
    bool loadImage(const QImage &image, QString *errorMessage = nullptr);
    bool saveAs(const QString &filePath, QString *errorMessage = nullptr);
    [[nodiscard]] ImageExportService::Result saveAsResult(
        const QString &filePath);
    void clear();

    [[nodiscard]] const QImage &image() const;
    [[nodiscard]] ImageSourcePtr imageSource() const;
    [[nodiscard]] QSize logicalSize() const;
    [[nodiscard]] bool isRegionBacked() const;
    [[nodiscard]] QString filePath() const;
    [[nodiscard]] bool isModified() const;
    [[nodiscard]] bool canUndo() const;
    [[nodiscard]] bool canRedo() const;

    bool crop(const QRect &rectangle);
    bool resizeImage(const QSize &size, Qt::TransformationMode mode = Qt::SmoothTransformation);
    bool adjustImage(int brightness, int contrast, qreal gamma);
    bool reduceRedEye(const QRect &rectangle);

public slots:
    bool rotateClockwise();
    bool rotateCounterClockwise();
    bool flipHorizontal();
    bool flipVertical();
    bool undo();
    bool redo();

signals:
    void imageChanged();
    void historyChanged();
    void modifiedChanged(bool modified);

private:
    bool apply(const QTransform &transform);
    bool applyImage(const QImage &image);
    bool setHistoryIndex(int index);

    QString m_filePath;
    ImageSourcePtr m_imageSource;
    QVector<QImage> m_history;
    int m_historyIndex = -1;
    int m_savedHistoryIndex = -1;
};
