#include "browserwidget.h"
#include "inputcontextpolicy.h"

#include "foldernavigationcontroller.h"
#include "thumbnailmodel.h"

#include <QActionGroup>
#include <QDir>
#include <QEvent>
#include <QHBoxLayout>
#include <QIcon>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QSlider>
#include <QSortFilterProxyModel>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

namespace {
QString directoryDisplayName(const QString &directoryPath)
{
    const QFileInfo directory(directoryPath);
    const QString name = directory.fileName();
    return name.isEmpty() ? directory.absoluteFilePath() : name;
}

bool claimFolderBrowserShortcut(QEvent *event)
{
    if (event->type() != QEvent::ShortcutOverride)
        return false;
    const auto &keyEvent = *static_cast<QKeyEvent *>(event);
    if (!InputContextPolicy::claimsShortcut(
            InputContextPolicy::Context::FolderBrowser,
            keyEvent)) {
        return false;
    }
    event->accept();
    return true;
}
}

BrowserWidget::BrowserWidget(QWidget *parent)
    : BrowserWidget(nullptr, parent)
{
}

BrowserWidget::BrowserWidget(
    DirectoryScanService *scanService, QWidget *parent)
    : QWidget(parent)
    , m_scanService(scanService
          ? scanService : new DirectoryScanService(this))
{
    m_navigation = new FolderNavigationController(this);
    setObjectName(QStringLiteral("folderBrowserPage"));
    setAutoFillBackground(true);
    setStyleSheet(QStringLiteral(
        "BrowserWidget#folderBrowserPage {"
        " background: palette(window); color: palette(window-text); }"
        "BrowserWidget#folderBrowserPage QListView {"
        " background: palette(base); color: palette(text);"
        " border: 1px solid palette(mid); border-radius: 6px; }"
        "BrowserWidget#folderBrowserPage QListView::item {"
        " border: 2px solid transparent; border-radius: 6px; padding: 4px; }"
        "BrowserWidget#folderBrowserPage QListView::item:hover {"
        " background: palette(midlight); }"
        "BrowserWidget#folderBrowserPage QListView::item:selected {"
        " background: palette(highlight); color: palette(highlighted-text);"
        " border-color: palette(highlight); }"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(8);

    auto *header = new QHBoxLayout;
    auto *backButton = new QToolButton(this);
    backButton->setIcon(QIcon::fromTheme(
        QStringLiteral("view-preview")));
    backButton->setToolTip(tr("Back to image"));
    backButton->setAccessibleName(tr("Back to image"));
    header->addWidget(backButton);

    m_historyBackButton = new QToolButton(this);
    m_historyBackButton->setObjectName(
        QStringLiteral("folderHistoryBackButton"));
    m_historyBackButton->setIcon(
        QIcon::fromTheme(QStringLiteral("go-previous")));
    m_historyBackButton->setToolTip(tr("Previous folder"));
    m_historyBackButton->setAccessibleName(tr("Previous folder"));
    m_historyBackButton->setEnabled(false);
    header->addWidget(m_historyBackButton);

    m_historyForwardButton = new QToolButton(this);
    m_historyForwardButton->setObjectName(
        QStringLiteral("folderHistoryForwardButton"));
    m_historyForwardButton->setIcon(
        QIcon::fromTheme(QStringLiteral("go-next")));
    m_historyForwardButton->setToolTip(tr("Next folder"));
    m_historyForwardButton->setAccessibleName(tr("Next folder"));
    m_historyForwardButton->setEnabled(false);
    header->addWidget(m_historyForwardButton);

    m_upButton = new QToolButton(this);
    m_upButton->setObjectName(QStringLiteral("folderParentButton"));
    m_upButton->setIcon(QIcon::fromTheme(QStringLiteral("go-up")));
    m_upButton->setToolTip(tr("Parent folder"));
    m_upButton->setAccessibleName(tr("Parent folder"));
    header->addWidget(m_upButton);

    m_locationsButton = new QToolButton(this);
    m_locationsButton->setObjectName(
        QStringLiteral("folderLocationsButton"));
    m_locationsButton->setIcon(
        QIcon::fromTheme(QStringLiteral("folder-favorites")));
    m_locationsButton->setToolTip(tr("Favorite and recent folders"));
    m_locationsButton->setAccessibleName(
        tr("Favorite and recent folders"));
    m_locationsButton->setPopupMode(QToolButton::InstantPopup);
    m_locationsMenu = new QMenu(m_locationsButton);
    m_locationsButton->setMenu(m_locationsMenu);
    header->addWidget(m_locationsButton);

    m_filter = new QLineEdit(this);
    m_filter->setObjectName(
        QStringLiteral("folderBrowserFilter"));
    m_filter->setPlaceholderText(tr("Filter images by name…"));
    m_filter->setClearButtonEnabled(true);
    m_filter->setAccessibleName(tr("Image filter"));
    header->addWidget(m_filter, 1);

    m_openSelectedButton = new QToolButton(this);
    m_openSelectedButton->setIcon(QIcon::fromTheme(QStringLiteral("document-preview")));
    m_openSelectedButton->setToolTip(tr("View selected image"));
    m_openSelectedButton->setAccessibleName(tr("View selected image"));
    m_openSelectedButton->setEnabled(false);
    header->addWidget(m_openSelectedButton);

    m_compareSelectedButton = new QToolButton(this);
    m_compareSelectedButton->setIcon(QIcon::fromTheme(QStringLiteral("view-split-left-right")));
    m_compareSelectedButton->setToolTip(tr("Compare selected images"));
    m_compareSelectedButton->setAccessibleName(tr("Compare selected images"));
    m_compareSelectedButton->setEnabled(false);
    header->addWidget(m_compareSelectedButton);

    auto *sortButton = new QToolButton(this);
    sortButton->setIcon(QIcon::fromTheme(
        QStringLiteral("view-sort-ascending")));
    sortButton->setToolTip(tr("Sort images"));
    sortButton->setAccessibleName(tr("Sort images"));
    sortButton->setPopupMode(QToolButton::InstantPopup);
    auto *sortMenu = new QMenu(sortButton);
    auto *sortGroup = new QActionGroup(sortMenu);
    const auto addSortAction =
        [sortMenu, sortGroup](const QString &text,
                              ThumbnailModel::SortKey key) {
        QAction *action = sortMenu->addAction(text);
        action->setCheckable(true);
        action->setData(static_cast<int>(key));
        sortGroup->addAction(action);
        return action;
    };
    QAction *nameSort = addSortAction(
        tr("Name"), ThumbnailModel::SortKey::Name);
    addSortAction(tr("Modified time"),
                  ThumbnailModel::SortKey::ModifiedTime);
    addSortAction(tr("File size"),
                  ThumbnailModel::SortKey::FileSize);
    addSortAction(tr("File type"),
                  ThumbnailModel::SortKey::FileType);
    nameSort->setChecked(true);
    sortMenu->addSeparator();
    QAction *ascendingAction = sortMenu->addAction(
        tr("Ascending"));
    ascendingAction->setCheckable(true);
    ascendingAction->setChecked(true);
    sortButton->setMenu(sortMenu);
    header->addWidget(sortButton);

    auto *sizeLabel = new QLabel(this);
    sizeLabel->setPixmap(QIcon::fromTheme(
        QStringLiteral("view-preview")).pixmap(18, 18));
    sizeLabel->setToolTip(tr("Thumbnail size"));
    sizeLabel->setAccessibleName(tr("Thumbnail size"));
    header->addWidget(sizeLabel);
    m_sizeSlider = new QSlider(Qt::Horizontal, this);
    m_sizeSlider->setRange(72, 220);
    m_sizeSlider->setValue(128);
    m_sizeSlider->setFixedWidth(130);
    m_sizeSlider->setAccessibleName(tr("Thumbnail size"));
    header->addWidget(m_sizeSlider);
    layout->addLayout(header);

    m_model = new ThumbnailModel(this);
    m_proxy = new QSortFilterProxyModel(this);
    m_proxy->setSourceModel(m_model);
    m_proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxy->setFilterRole(Qt::DisplayRole);

    m_view = new QListView(this);
    m_view->setObjectName(
        QStringLiteral("folderBrowserView"));
    m_view->setModel(m_proxy);
    m_view->setViewMode(QListView::IconMode);
    m_view->setResizeMode(QListView::Adjust);
    m_view->setMovement(QListView::Static);
    m_view->setWrapping(true);
    m_view->setUniformItemSizes(true);
    m_view->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_view->setDragEnabled(true);
    m_view->setDragDropMode(QAbstractItemView::DragOnly);
    m_view->setDefaultDropAction(Qt::CopyAction);
    m_view->setWordWrap(true);
    m_view->setAccessibleName(tr("Folder browser"));
    m_view->setContextMenuPolicy(Qt::CustomContextMenu);
    m_view->viewport()->setAutoFillBackground(true);
    m_view->installEventFilter(this);
    layout->addWidget(m_view, 1);
    updateGridSize(m_sizeSlider->value());

    connect(m_filter, &QLineEdit::textChanged,
            m_proxy, &QSortFilterProxyModel::setFilterFixedString);
    connect(m_sizeSlider, &QSlider::valueChanged,
            this, &BrowserWidget::updateGridSize);
    connect(sortGroup, &QActionGroup::triggered,
            this, [this, ascendingAction](QAction *action) {
        m_model->setSort(
            static_cast<ThumbnailModel::SortKey>(
                action->data().toInt()),
            ascendingAction->isChecked()
                ? Qt::AscendingOrder : Qt::DescendingOrder);
    });
    connect(ascendingAction, &QAction::toggled,
            this, [this, sortGroup, sortButton](bool ascending) {
        QAction *checked = sortGroup->checkedAction();
        if (checked) {
            m_model->setSort(
                static_cast<ThumbnailModel::SortKey>(
                    checked->data().toInt()),
                ascending ? Qt::AscendingOrder
                          : Qt::DescendingOrder);
        }
        sortButton->setIcon(QIcon::fromTheme(
            ascending
                ? QStringLiteral("view-sort-ascending")
                : QStringLiteral("view-sort-descending")));
    });
    connect(m_view, &QListView::activated,
            this, &BrowserWidget::activate);
    connect(m_openSelectedButton, &QToolButton::clicked, this, [this] {
        const QStringList files = selectedImageFiles();
        if (!files.isEmpty())
            emit imageActivated(files.constFirst());
    });
    connect(m_compareSelectedButton, &QToolButton::clicked, this, [this] {
        const QStringList files = selectedImageFiles();
        if (files.size() >= 2)
            emit compareRequested(files.mid(0, 4));
    });
    connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this] {
        const int selected = selectedImageFiles().size();
        m_openSelectedButton->setEnabled(selected > 0);
        m_compareSelectedButton->setEnabled(selected >= 2);
    });
    connect(m_upButton, &QToolButton::clicked, this, [this] {
        const QDir current(m_model->directoryPath());
        setDirectory(QFileInfo(current.absolutePath()).dir().absolutePath());
    });
    connect(m_historyBackButton, &QToolButton::clicked,
            m_navigation, &FolderNavigationController::goBack);
    connect(m_historyForwardButton, &QToolButton::clicked,
            m_navigation, &FolderNavigationController::goForward);
    connect(m_navigation,
            &FolderNavigationController::directoryRequested,
            this, &BrowserWidget::setDirectory);
    connect(m_navigation,
            &FolderNavigationController::stateChanged,
            this, &BrowserWidget::updateNavigationControls);
    connect(m_locationsMenu, &QMenu::aboutToShow,
            this, &BrowserWidget::rebuildLocationsMenu);
    connect(backButton, &QToolButton::clicked,
            this, &BrowserWidget::backRequested);
    connect(m_view, &QWidget::customContextMenuRequested,
            this, [this](const QPoint &position) {
        const QModelIndex clicked = m_view->indexAt(position);
        if (clicked.isValid()
            && !m_view->selectionModel()->isSelected(clicked)) {
            m_view->setCurrentIndex(clicked);
        }
        const QStringList files = selectedImageFiles();
        if (files.isEmpty())
            return;
        QMenu menu(this);
        QAction *viewAction = menu.addAction(
            QIcon::fromTheme(QStringLiteral("document-preview")),
            tr("View"));
        QAction *addAction = menu.addAction(
            QIcon::fromTheme(QStringLiteral("list-add")),
            tr("Add to opened images"));
        QAction *compareAction = menu.addAction(
            QIcon::fromTheme(
                QStringLiteral("view-split-left-right")),
            tr("Compare"));
        compareAction->setEnabled(files.size() >= 2);
        menu.addSeparator();
        QAction *revealAction = menu.addAction(
            QIcon::fromTheme(QStringLiteral("system-file-manager")),
            tr("Show in file manager"));
        menu.addSeparator();
        QAction *renameAction = menu.addAction(
            QIcon::fromTheme(QStringLiteral("edit-rename")),
            tr("Rename…"));
        renameAction->setEnabled(files.size() == 1);
        QAction *copyAction = menu.addAction(
            QIcon::fromTheme(QStringLiteral("edit-copy")),
            tr("Copy to…"));
        QAction *moveAction = menu.addAction(
            QIcon::fromTheme(QStringLiteral("go-jump")),
            tr("Move to…"));
        QAction *trashAction = menu.addAction(
            QIcon::fromTheme(QStringLiteral("user-trash")),
            tr("Move to Trash"));
        QAction *selected = menu.exec(
            m_view->viewport()->mapToGlobal(position));
        if (selected == viewAction)
            emit imageActivated(files.constFirst());
        else if (selected == addAction)
            emit addToOpenedRequested(files);
        else if (selected == compareAction)
            emit compareRequested(files.mid(0, 4));
        else if (selected == revealAction)
            emit revealRequested(files.constFirst());
        else if (selected == renameAction)
            emit fileOperationRequested(
                QStringLiteral("rename"), files);
        else if (selected == copyAction)
            emit fileOperationRequested(
                QStringLiteral("copy"), files);
        else if (selected == moveAction)
            emit fileOperationRequested(
                QStringLiteral("move"), files);
        else if (selected == trashAction)
            emit fileOperationRequested(
                QStringLiteral("trash"), files);
    });
    connect(m_scanService, &DirectoryScanService::scanFinished,
            this, &BrowserWidget::applyScanResult);
    updateNavigationControls();
    refreshAppearance();
}

void BrowserWidget::setDirectory(const QString &directoryPath)
{
    const QDir directory(directoryPath);
    if (!directory.exists())
        return;
    const QString normalized = directory.absolutePath();
    if (normalized == m_requestedDirectory
        && m_model->directoryPath() == normalized
        && m_model->rowCount() > 0) {
        return;
    }

    saveDirectoryState();
    if (m_scanRequestId)
        m_scanService->cancel(m_scanRequestId);
    m_requestedDirectory = normalized;
    const DirectoryState state =
        m_directoryStates.value(normalized);
    {
        const QSignalBlocker blocker(m_filter);
        m_filter->setText(state.filter);
    }
    m_proxy->setFilterFixedString(state.filter);
    m_model->setDirectoryEntries(normalized, {});
    m_scanRequestId = m_scanService->requestScan(normalized);
}

void BrowserWidget::setCurrentPath(const QString &filePath)
{
    m_preferredPath = QFileInfo(filePath).absoluteFilePath();
    if (m_preferredPath.isEmpty())
        return;
    for (int row = 0; row < m_proxy->rowCount(); ++row) {
        const QModelIndex proxyIndex = m_proxy->index(row, 0);
        if (m_model->filePath(
                m_proxy->mapToSource(proxyIndex))
            == m_preferredPath) {
            m_view->setCurrentIndex(proxyIndex);
            m_view->scrollTo(
                proxyIndex,
                QAbstractItemView::EnsureVisible);
            m_preferredPath.clear();
            return;
        }
    }
}

void BrowserWidget::refreshDirectory()
{
    if (m_requestedDirectory.isEmpty())
        return;
    saveDirectoryState();
    if (m_scanRequestId)
        m_scanService->cancel(m_scanRequestId);
    m_scanRequestId = m_scanService->requestScan(
        m_requestedDirectory, true);
}

QString BrowserWidget::directoryPath() const
{
    return m_model->directoryPath();
}

QStringList BrowserWidget::selectedImageFiles() const
{
    QStringList files;
    for (const QModelIndex &proxyIndex : m_view->selectionModel()->selectedIndexes()) {
        const QModelIndex sourceIndex = m_proxy->mapToSource(proxyIndex);
        if (!m_model->isDirectory(sourceIndex))
            files.append(m_model->filePath(sourceIndex));
    }
    return files;
}

int BrowserWidget::imageCount() const
{
    int count = 0;
    for (int row = 0; row < m_model->rowCount(); ++row) {
        if (!m_model->isDirectory(m_model->index(row)))
            ++count;
    }
    return count;
}

void BrowserWidget::setStoredLocations(
    const QStringList &recentDirectories,
    const QStringList &favoriteDirectories)
{
    m_navigation->setStoredLocations(
        recentDirectories, favoriteDirectories);
}

QStringList BrowserWidget::recentDirectories() const
{
    return m_navigation->recentDirectories();
}

QStringList BrowserWidget::favoriteDirectories() const
{
    return m_navigation->favoriteDirectories();
}

void BrowserWidget::applyScanResult(
    quint64 requestId, const DirectoryScanResult &result)
{
    if (requestId != m_scanRequestId)
        return;
    m_scanRequestId = 0;
    if (!result.succeeded()) {
        m_navigation->navigationFailed(result.directoryPath);
        emit scanFailed(result.directoryPath, result.error);
        return;
    }

    applyDirectoryResult(result);
}

void BrowserWidget::applyDirectoryResult(
    const DirectoryScanResult &result)
{
    if (!result.succeeded())
        return;
    if (!m_requestedDirectory.isEmpty()
        && result.directoryPath != m_requestedDirectory) {
        return;
    }
    if (m_scanRequestId) {
        m_scanService->cancel(m_scanRequestId);
        m_scanRequestId = 0;
    }

    m_requestedDirectory = result.directoryPath;
    m_navigation->recordVisit(result.directoryPath);
    m_model->setDirectoryEntries(
        result.directoryPath, result.entries);
    const DirectoryState state =
        m_directoryStates.value(result.directoryPath);
    const QString preferredPath =
        QFileInfo(m_preferredPath).absolutePath()
                == result.directoryPath
            ? m_preferredPath : state.selectedPath;
    m_preferredPath.clear();
    QTimer::singleShot(0, this, [this, state, preferredPath] {
        if (!preferredPath.isEmpty()) {
            for (int row = 0; row < m_proxy->rowCount(); ++row) {
                const QModelIndex proxyIndex = m_proxy->index(row, 0);
                const QModelIndex sourceIndex =
                    m_proxy->mapToSource(proxyIndex);
                if (m_model->filePath(sourceIndex)
                    == preferredPath) {
                    m_view->setCurrentIndex(proxyIndex);
                    break;
                }
            }
        }
        m_view->verticalScrollBar()->setValue(
            state.verticalScroll);
    });
    emit directoryChanged(result.directoryPath);
}

void BrowserWidget::rebuildLocationsMenu()
{
    m_locationsMenu->clear();
    const QString current = m_navigation->currentDirectory().isEmpty()
        ? m_model->directoryPath()
        : m_navigation->currentDirectory();
    QAction *favoriteAction = m_locationsMenu->addAction(
        QIcon::fromTheme(m_navigation->isFavorite(current)
            ? QStringLiteral("bookmark-remove")
            : QStringLiteral("bookmark-new")),
        m_navigation->isFavorite(current)
            ? tr("Remove current folder from favorites")
            : tr("Add current folder to favorites"));
    favoriteAction->setEnabled(!current.isEmpty());
    connect(favoriteAction, &QAction::triggered,
            this, [this, current] {
        m_navigation->toggleFavorite(current);
    });

    m_locationsMenu->addSeparator();
    m_locationsMenu->addSection(
        QIcon::fromTheme(QStringLiteral("folder-favorites")),
        tr("Favorite folders"));
    const QStringList favorites =
        m_navigation->favoriteDirectories();
    if (favorites.isEmpty()) {
        QAction *empty = m_locationsMenu->addAction(
            tr("No favorite folders"));
        empty->setEnabled(false);
    } else {
        for (const QString &path : favorites) {
            QAction *action = m_locationsMenu->addAction(
                QIcon::fromTheme(QStringLiteral("folder")),
                directoryDisplayName(path));
            action->setToolTip(path);
            connect(action, &QAction::triggered,
                    this, [this, path] {
                navigateToStoredDirectory(path);
            });
        }
    }

    m_locationsMenu->addSeparator();
    m_locationsMenu->addSection(
        QIcon::fromTheme(QStringLiteral("document-open-recent")),
        tr("Recent folders"));
    const QStringList recent = m_navigation->recentDirectories();
    if (recent.isEmpty()) {
        QAction *empty = m_locationsMenu->addAction(
            tr("No recent folders"));
        empty->setEnabled(false);
    } else {
        for (const QString &path : recent) {
            QAction *action = m_locationsMenu->addAction(
                QIcon::fromTheme(QStringLiteral("folder")),
                directoryDisplayName(path));
            action->setToolTip(path);
            connect(action, &QAction::triggered,
                    this, [this, path] {
                navigateToStoredDirectory(path);
            });
        }
        m_locationsMenu->addSeparator();
        QAction *clearAction = m_locationsMenu->addAction(
            QIcon::fromTheme(QStringLiteral("edit-clear-history")),
            tr("Clear recent folders"));
        connect(clearAction, &QAction::triggered,
                m_navigation,
                &FolderNavigationController::clearRecentDirectories);
    }
}

void BrowserWidget::updateNavigationControls()
{
    m_historyBackButton->setEnabled(m_navigation->canGoBack());
    m_historyForwardButton->setEnabled(m_navigation->canGoForward());
    const QString current = m_navigation->currentDirectory();
    const QString parent = current.isEmpty()
        ? QString()
        : QFileInfo(current).dir().absolutePath();
    m_upButton->setEnabled(
        !current.isEmpty() && parent != current);
    const bool favorite = m_navigation->isFavorite(current);
    m_locationsButton->setIcon(QIcon::fromTheme(
        favorite ? QStringLiteral("folder-favorites")
                 : QStringLiteral("folder-bookmark"),
        QIcon::fromTheme(QStringLiteral("folder"))));
}

void BrowserWidget::navigateToStoredDirectory(
    const QString &directoryPath)
{
    if (!QFileInfo(directoryPath).isDir()) {
        emit scanFailed(
            directoryPath,
            tr("The saved folder no longer exists."));
        return;
    }
    setDirectory(directoryPath);
}

void BrowserWidget::saveDirectoryState()
{
    const QString directory = m_model->directoryPath();
    if (directory.isEmpty())
        return;
    DirectoryState &state = m_directoryStates[directory];
    const QModelIndex current = m_view->currentIndex();
    if (current.isValid()) {
        state.selectedPath = m_model->filePath(
            m_proxy->mapToSource(current));
    }
    state.filter = m_filter->text();
    state.verticalScroll =
        m_view->verticalScrollBar()->value();
}

void BrowserWidget::activate(const QModelIndex &proxyIndex)
{
    const QModelIndex sourceIndex = m_proxy->mapToSource(proxyIndex);
    const QString path = m_model->filePath(sourceIndex);
    if (m_model->isDirectory(sourceIndex))
        setDirectory(path);
    else if (!path.isEmpty())
        emit imageActivated(path);
}

void BrowserWidget::updateGridSize(int thumbnailPixels)
{
    const QSize iconSize(thumbnailPixels, qRound(thumbnailPixels * 0.75));
    m_model->setThumbnailSize(iconSize);
    m_view->setIconSize(iconSize);
    m_view->setGridSize(QSize(thumbnailPixels + 34,
                              iconSize.height() + 50));
}

void BrowserWidget::refreshAppearance()
{
    if (m_refreshingAppearance || !m_view)
        return;

    m_refreshingAppearance = true;
    const QPalette currentPalette = palette();
    m_view->setPalette(currentPalette);
    m_view->viewport()->setPalette(currentPalette);
    if (m_filter)
        m_filter->setPalette(currentPalette);
    if (m_sizeSlider)
        m_sizeSlider->setPalette(currentPalette);
    if (m_openSelectedButton)
        m_openSelectedButton->setPalette(currentPalette);
    if (m_compareSelectedButton)
        m_compareSelectedButton->setPalette(currentPalette);
    if (m_model)
        m_model->refreshTheme();
    m_view->viewport()->update();
    update();
    m_refreshingAppearance = false;
}

bool BrowserWidget::event(QEvent *event)
{
    if (claimFolderBrowserShortcut(event))
        return true;
    return QWidget::event(event);
}

void BrowserWidget::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::PaletteChange
        || event->type() == QEvent::ApplicationPaletteChange
        || event->type() == QEvent::StyleChange) {
        refreshAppearance();
    }
}

void BrowserWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->modifiers() == Qt::AltModifier
        && event->key() == Qt::Key_Left) {
        m_navigation->goBack();
        event->accept();
        return;
    }
    if (event->modifiers() == Qt::AltModifier
        && event->key() == Qt::Key_Right) {
        m_navigation->goForward();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Escape) {
        emit backRequested();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Backspace) {
        const QDir current(m_model->directoryPath());
        setDirectory(
            QFileInfo(current.absolutePath()).dir().absolutePath());
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

bool BrowserWidget::eventFilter(
    QObject *watched, QEvent *event)
{
    if (watched == m_view
        && claimFolderBrowserShortcut(event)) {
        return true;
    }
    if (watched == m_view
        && event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->modifiers() == Qt::AltModifier
            && keyEvent->key() == Qt::Key_Left) {
            m_navigation->goBack();
            return true;
        }
        if (keyEvent->modifiers() == Qt::AltModifier
            && keyEvent->key() == Qt::Key_Right) {
            m_navigation->goForward();
            return true;
        }
        if (keyEvent->key() == Qt::Key_Escape) {
            emit backRequested();
            return true;
        }
        if (keyEvent->key() == Qt::Key_Backspace) {
            const QDir current(m_model->directoryPath());
            setDirectory(QFileInfo(current.absolutePath())
                .dir().absolutePath());
            return true;
        }
        if (keyEvent->key() == Qt::Key_F2) {
            const QStringList files = selectedImageFiles();
            if (files.size() == 1) {
                emit fileOperationRequested(
                    QStringLiteral("rename"), files);
            }
            return true;
        }
        if (keyEvent->key() == Qt::Key_Delete) {
            const QStringList files = selectedImageFiles();
            if (!files.isEmpty()) {
                emit fileOperationRequested(
                    QStringLiteral("trash"), files);
            }
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}
