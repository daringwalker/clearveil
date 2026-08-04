#pragma once

#include "imageexportservice.h"

#include <QImage>
#include <QObject>
#include <QString>

class FrameController;
class ImageDocument;
struct ImageLoadResult;
class QFileSystemWatcher;
class QTimer;

class DocumentWorkflowController final : public QObject
{
    Q_OBJECT

public:
    explicit DocumentWorkflowController(
        ImageDocument *document,
        FrameController *frames,
        QObject *parent = nullptr);

    [[nodiscard]] bool loadPath(
        const QString &filePath,
        QString *errorMessage = nullptr);
    [[nodiscard]] bool loadDecoded(
        const ImageLoadResult &result,
        QString *errorMessage = nullptr);
    [[nodiscard]] bool loadClipboardImage(
        const QImage &image,
        QString *errorMessage = nullptr);
    [[nodiscard]] ImageExportService::Result saveAs(
        const QString &filePath);
    [[nodiscard]] bool reload(QString *errorMessage = nullptr);
    void clear();

    [[nodiscard]] const QImage &displayedImage() const;
    [[nodiscard]] QString watchedPath() const;

signals:
    void currentFileChanged(const QString &filePath);
    void externalReloadRequested();
    void externalChangeBlocked(const QString &filePath);

private:
    void openFrames(const QString &filePath);
    void refreshFileWatch();

    ImageDocument *m_document = nullptr;
    FrameController *m_frames = nullptr;
    QFileSystemWatcher *m_fileWatcher = nullptr;
    QTimer *m_reloadTimer = nullptr;
};
