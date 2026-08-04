#include "filmstripcontroller.h"

#include "filmstripview.h"
#include "thumbnailmodel.h"

#include <QAbstractItemView>
#include <QFileInfo>
#include <QItemSelectionModel>
#include <QListView>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QTimer>

#include <algorithm>

FilmstripController::FilmstripController(
    QListView *view, ThumbnailModel *openedModel,
    ThumbnailModel *directoryModel, QObject *parent)
    : QObject(parent)
    , m_view(view)
    , m_openedModel(openedModel)
    , m_directoryModel(directoryModel)
{
    if (m_view && m_openedModel)
        m_view->setModel(m_openedModel);
    connectSelection();
}

FilmstripController::Source FilmstripController::source() const
{
    return m_source;
}

void FilmstripController::setSource(Source source)
{
    if (!m_view || source == m_source)
        return;
    if (m_source == Source::OpenedImages)
        m_openedScrollValue = currentScrollValue();
    else
        m_directoryScrollValue = currentScrollValue();

    ThumbnailModel *target = source == Source::CurrentDirectory
        ? m_directoryModel.data() : m_openedModel.data();
    if (!target)
        return;
    QObject::disconnect(m_selectionConnection);
    m_source = source;
    m_view->setModel(target);
    connectSelection();
    emit sourceChanged(m_source);

    restoreScrollValue(
        source, source == Source::CurrentDirectory
            ? m_directoryScrollValue : m_openedScrollValue);
}

ThumbnailModel *FilmstripController::model() const
{
    return m_source == Source::CurrentDirectory
        ? m_directoryModel.data() : m_openedModel.data();
}

int FilmstripController::count() const
{
    ThumbnailModel *activeModel = model();
    return activeModel ? activeModel->rowCount() : 0;
}

int FilmstripController::currentRow() const
{
    return m_view ? m_view->currentIndex().row() : -1;
}

QString FilmstripController::pathAt(int row) const
{
    ThumbnailModel *activeModel = model();
    if (!activeModel || row < 0 || row >= activeModel->rowCount())
        return {};
    return activeModel->filePath(activeModel->index(row));
}

int FilmstripController::rowForPath(const QString &path) const
{
    ThumbnailModel *activeModel = model();
    if (!activeModel)
        return -1;
    const QString normalized = QFileInfo(path).absoluteFilePath();
    for (int row = 0; row < activeModel->rowCount(); ++row) {
        if (QFileInfo(activeModel->filePath(activeModel->index(row)))
                .absoluteFilePath() == normalized) {
            return row;
        }
    }
    return -1;
}

void FilmstripController::selectRow(int row, bool ensureVisible)
{
    ThumbnailModel *activeModel = model();
    if (!m_view || !activeModel
        || row < 0 || row >= activeModel->rowCount()) {
        return;
    }
    const QModelIndex index = activeModel->index(row);
    m_view->selectionModel()->setCurrentIndex(
        index, QItemSelectionModel::ClearAndSelect);
    if (ensureVisible) {
        m_view->scrollTo(index, QAbstractItemView::EnsureVisible);
    }
}

void FilmstripController::syncSelection(const QString &path)
{
    if (!m_view || !m_view->selectionModel())
        return;
    const int row = rowForPath(path);
    const QSignalBlocker blocker(m_view->selectionModel());
    if (row < 0) {
        m_view->clearSelection();
        m_view->setCurrentIndex({});
        return;
    }
    const QModelIndex index = model()->index(row);
    m_view->setCurrentIndex(index);
    m_view->scrollTo(index, QAbstractItemView::EnsureVisible);
}

void FilmstripController::connectSelection()
{
    if (!m_view || !m_view->selectionModel())
        return;
    m_selectionConnection = connect(
        m_view->selectionModel(),
        &QItemSelectionModel::currentChanged,
        this, [this](const QModelIndex &current) {
            if (current.isValid()) {
                emit activationRequested(
                    m_source, current.row(),
                    pathAt(current.row()));
            }
        });
}

int FilmstripController::currentScrollValue() const
{
    if (!m_view)
        return 0;
    QScrollBar *bar = nullptr;
    if (const auto *filmstrip =
            dynamic_cast<const FilmstripView *>(m_view.data())) {
        bar = filmstrip->activeScrollBar();
    } else {
        bar = m_view->flow() == QListView::LeftToRight
            ? m_view->horizontalScrollBar()
            : m_view->verticalScrollBar();
    }
    return bar ? bar->value() : 0;
}

void FilmstripController::restoreScrollValue(
    Source source, int value)
{
    QTimer::singleShot(0, this, [this, source, value] {
        if (!m_view || m_source != source)
            return;
        QScrollBar *bar = nullptr;
        if (const auto *filmstrip =
                dynamic_cast<const FilmstripView *>(m_view.data())) {
            bar = filmstrip->activeScrollBar();
        } else {
            bar = m_view->flow() == QListView::LeftToRight
                ? m_view->horizontalScrollBar()
                : m_view->verticalScrollBar();
        }
        if (bar) {
            bar->setValue(std::clamp(
                value, bar->minimum(), bar->maximum()));
        }
    });
}
