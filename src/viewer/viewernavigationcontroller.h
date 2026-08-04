#pragma once

#include <QObject>
#include <QString>

class DocumentWorkflowController;
class FilmstripController;
class ImageLoadController;
class ImageSessionController;
struct ImageLoadResult;

class ViewerNavigationController final : public QObject
{
    Q_OBJECT

public:
    ViewerNavigationController(
        ImageSessionController *session,
        ImageLoadController *loader,
        DocumentWorkflowController *documentWorkflow,
        FilmstripController *filmstrip,
        QObject *parent = nullptr);

    void requestPath(const QString &filePath,
                     int openedSequenceIndex = -1);
    [[nodiscard]] bool cancel();
    void prefetchAdjacent(
        int openedSequenceIndex,
        const QString &activatedFilePath = {});
    [[nodiscard]] bool isLoading() const;

signals:
    void loadingStarted(const QString &filePath);
    void activationSucceeded(const QString &filePath,
                             int resolvedOpenedIndex);
    void activationFailed(const QString &filePath,
                          const QString &error);
    void pendingStateChanged();

private:
    void completeLoad(const ImageLoadResult &result,
                      int openedSequenceIndex);

    ImageSessionController *m_session = nullptr;
    ImageLoadController *m_loader = nullptr;
    DocumentWorkflowController *m_documentWorkflow = nullptr;
    FilmstripController *m_filmstrip = nullptr;
};
