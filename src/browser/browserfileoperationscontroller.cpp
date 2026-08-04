#include "browserfileoperationscontroller.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QtConcurrentRun>

#include <utility>

namespace {
bool isSkipped(const FileOperations::Result &result)
{
    return result.error == FileOperations::Error::NoChange
        || result.error == FileOperations::Error::TargetExists;
}
}

BrowserFileOperationsController::BrowserFileOperationsController(
    QWidget *dialogParent, QObject *parent)
    : QObject(parent)
    , m_dialogParent(dialogParent)
{
    m_pool.setMaxThreadCount(1);
    m_pool.setExpiryTimeout(10'000);
}

BrowserFileOperationsController::~BrowserFileOperationsController()
{
    m_pool.waitForDone();
}

void BrowserFileOperationsController::perform(
    const QString &operationId, const QStringList &paths)
{
    if (paths.isEmpty())
        return;
    if (m_busy) {
        emit statusMessage(
            tr("Another file operation is still running."),
            3000);
        return;
    }
    if (operationId == QStringLiteral("rename"))
        renameFile(paths.constFirst());
    else if (operationId == QStringLiteral("copy"))
        copySelected(paths);
    else if (operationId == QStringLiteral("move"))
        moveSelected(paths);
    else if (operationId == QStringLiteral("trash"))
        trashSelected(paths);
}

BrowserFileOperationsController::BatchResult
BrowserFileOperationsController::copyFiles(
    const QStringList &paths,
    const QString &destinationDirectory)
{
    return transferFiles(
        paths, destinationDirectory,
        &FileOperations::copyToDirectory);
}

BrowserFileOperationsController::BatchResult
BrowserFileOperationsController::moveFiles(
    const QStringList &paths,
    const QString &destinationDirectory)
{
    return transferFiles(
        paths, destinationDirectory,
        &FileOperations::moveToDirectory);
}

BrowserFileOperationsController::BatchResult
BrowserFileOperationsController::trashFiles(
    const QStringList &paths)
{
    BatchResult batch;
    for (const QString &path : paths) {
        FileOperations::Result result =
            FileOperations::moveToTrash(path);
        if (result.succeeded())
            batch.successes.append(result);
        else if (isSkipped(result))
            batch.skipped.append(result);
        else
            batch.failures.append(result);
    }
    return batch;
}

BrowserFileOperationsController::BatchResult
BrowserFileOperationsController::transferFiles(
    const QStringList &paths,
    const QString &destinationDirectory,
    FileOperation operation)
{
    BatchResult batch;
    for (const QString &path : paths) {
        FileOperations::Result result =
            operation(path, destinationDirectory);
        if (result.succeeded())
            batch.successes.append(result);
        else if (isSkipped(result))
            batch.skipped.append(result);
        else
            batch.failures.append(result);
    }
    return batch;
}

void BrowserFileOperationsController::renameFile(
    const QString &path)
{
    const QFileInfo source(path);
    bool accepted = false;
    const QString newName = QInputDialog::getText(
        m_dialogParent, tr("Rename image"),
        tr("New file name"), QLineEdit::Normal,
        source.fileName(), &accepted).trimmed();
    if (!accepted || newName.isEmpty())
        return;

    const FileOperations::Result result =
        FileOperations::renameFile(path, newName);
    if (!result.succeeded()) {
        QMessageBox::warning(
            m_dialogParent, tr("Cannot rename image"),
            result.error == FileOperations::Error::TargetExists
                ? tr("A file named “%1” already exists.")
                      .arg(newName)
                : tr("The image could not be renamed."));
        return;
    }
    emit operationCompleted(Operation::Rename, {result});
    emit statusMessage(tr("Image renamed."), 3000);
}

void BrowserFileOperationsController::copySelected(
    const QStringList &paths)
{
    const QString destination =
        QFileDialog::getExistingDirectory(
            m_dialogParent, tr("Copy images to"),
            QFileInfo(paths.constFirst()).absolutePath());
    if (destination.isEmpty())
        return;
    startBatch(Operation::Copy,
        [paths, destination] {
            return copyFiles(paths, destination);
        });
}

void BrowserFileOperationsController::moveSelected(
    const QStringList &paths)
{
    const QString destination =
        QFileDialog::getExistingDirectory(
            m_dialogParent, tr("Move images to"),
            QFileInfo(paths.constFirst()).absolutePath());
    if (destination.isEmpty())
        return;
    startBatch(Operation::Move,
        [paths, destination] {
            return moveFiles(paths, destination);
        });
}

void BrowserFileOperationsController::trashSelected(
    const QStringList &paths)
{
    const QMessageBox::StandardButton answer =
        QMessageBox::question(
            m_dialogParent, tr("Move images to Trash"),
            tr("Move %1 selected image(s) to the Trash?")
                .arg(paths.size()),
            QMessageBox::Cancel | QMessageBox::Yes,
            QMessageBox::Cancel);
    if (answer != QMessageBox::Yes)
        return;
    startBatch(Operation::Trash,
        [paths] { return trashFiles(paths); });
}

void BrowserFileOperationsController::startBatch(
    Operation operation,
    std::function<BatchResult()> task)
{
    m_busy = true;
    emit statusMessage(tr("Processing files…"), 0);
    auto future = QtConcurrent::run(
        &m_pool, std::move(task));
    auto *watcher = new QFutureWatcher<BatchResult>(this);
    connect(watcher, &QFutureWatcher<BatchResult>::finished,
            this, [this, watcher, operation] {
        const BatchResult result = watcher->result();
        watcher->deleteLater();
        m_busy = false;
        reportBatch(operation, result);
    });
    watcher->setFuture(future);
}

void BrowserFileOperationsController::reportBatch(
    Operation operation, const BatchResult &result)
{
    if (!result.failures.isEmpty()
        || !result.skipped.isEmpty()) {
        QMessageBox::warning(
            m_dialogParent, tr("Some files were not processed"),
            tr("Completed: %1\nSkipped: %2\nFailed: %3")
                .arg(result.successes.size())
                .arg(result.skipped.size())
                .arg(result.failures.size()));
    }
    if (!result.successes.isEmpty()) {
        emit operationCompleted(operation, result.successes);
        emit statusMessage(
            tr("Processed %1 image(s).")
                .arg(result.successes.size()),
            3000);
    } else {
        emit statusMessage(QString(), 0);
    }
}
