#include "viewernavigationcontroller.h"

#include "documentworkflowcontroller.h"
#include "filmstripcontroller.h"
#include "imageloadcontroller.h"
#include "imagesessioncontroller.h"

ViewerNavigationController::ViewerNavigationController(
    ImageSessionController *session,
    ImageLoadController *loader,
    DocumentWorkflowController *documentWorkflow,
    FilmstripController *filmstrip,
    QObject *parent)
    : QObject(parent)
    , m_session(session)
    , m_loader(loader)
    , m_documentWorkflow(documentWorkflow)
    , m_filmstrip(filmstrip)
{
    Q_ASSERT(m_session);
    Q_ASSERT(m_loader);
    Q_ASSERT(m_documentWorkflow);
    connect(m_loader, &ImageLoadController::loadFinished,
            this, [this](const ImageLoadResult &result,
                         int openedSequenceIndex, bool) {
        completeLoad(result, openedSequenceIndex);
    });
}

void ViewerNavigationController::requestPath(
    const QString &filePath,
    int openedSequenceIndex)
{
    m_session->setPendingOpenedIndex(openedSequenceIndex);
    m_loader->request(filePath, openedSequenceIndex);
    emit loadingStarted(filePath);
    emit pendingStateChanged();
}

bool ViewerNavigationController::cancel()
{
    if (!m_loader->isLoading()
        && m_session->pendingOpenedIndex() < 0) {
        return false;
    }
    m_loader->cancel();
    m_session->clearPendingOpenedIndex();
    emit pendingStateChanged();
    return true;
}

void ViewerNavigationController::prefetchAdjacent(
    int openedSequenceIndex,
    const QString &activatedFilePath)
{
    QStringList adjacentPaths;
    if (m_filmstrip
        && m_filmstrip->source()
            == FilmstripController::Source::CurrentDirectory) {
        const int pathRow = activatedFilePath.isEmpty()
            ? -1 : m_filmstrip->rowForPath(activatedFilePath);
        const int row = pathRow >= 0
            ? pathRow : m_filmstrip->currentRow();
        for (const int adjacentRow : {row + 1, row - 1}) {
            if (adjacentRow >= 0
                && adjacentRow < m_filmstrip->count()) {
                adjacentPaths.append(
                    m_filmstrip->pathAt(adjacentRow));
            }
        }
    } else {
        for (const int index : {openedSequenceIndex + 1,
                                openedSequenceIndex - 1}) {
            if (index >= 0
                && index < m_session->openedCount()) {
                adjacentPaths.append(
                    m_session->openedPathAt(index));
            }
        }
    }
    m_loader->prefetch(adjacentPaths);
}

bool ViewerNavigationController::isLoading() const
{
    return m_loader->isLoading();
}

void ViewerNavigationController::completeLoad(
    const ImageLoadResult &result,
    int openedSequenceIndex)
{
    m_session->clearPendingOpenedIndex();
    emit pendingStateChanged();
    if (!result.succeeded()) {
        emit activationFailed(result.filePath, result.error);
        return;
    }

    QString error;
    if (!m_documentWorkflow->loadDecoded(result, &error)) {
        emit activationFailed(result.filePath, error);
        return;
    }

    m_session->setLoadedPath(
        result.filePath, openedSequenceIndex);
    const int resolvedOpenedIndex =
        m_session->currentOpenedIndex();
    prefetchAdjacent(
        resolvedOpenedIndex, result.filePath);
    emit activationSucceeded(
        result.filePath, resolvedOpenedIndex);
}
