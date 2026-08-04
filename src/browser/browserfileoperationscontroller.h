#pragma once

#include "fileoperations.h"

#include <QObject>
#include <QStringList>
#include <QThreadPool>

#include <functional>

class QWidget;

class BrowserFileOperationsController final : public QObject
{
    Q_OBJECT

public:
    enum class Operation {
        Rename,
        Copy,
        Move,
        Trash
    };
    Q_ENUM(Operation)

    struct BatchResult {
        QList<FileOperations::Result> successes;
        QList<FileOperations::Result> skipped;
        QList<FileOperations::Result> failures;
    };

    explicit BrowserFileOperationsController(
        QWidget *dialogParent,
        QObject *parent = nullptr);
    ~BrowserFileOperationsController() override;

    void perform(const QString &operationId,
                 const QStringList &paths);

    [[nodiscard]] static BatchResult copyFiles(
        const QStringList &paths,
        const QString &destinationDirectory);
    [[nodiscard]] static BatchResult moveFiles(
        const QStringList &paths,
        const QString &destinationDirectory);
    [[nodiscard]] static BatchResult trashFiles(
        const QStringList &paths);

signals:
    void operationCompleted(
        BrowserFileOperationsController::Operation operation,
        const QList<FileOperations::Result> &results);
    void statusMessage(const QString &message, int timeoutMs);

private:
    using FileOperation = FileOperations::Result (*)(
        const QString &, const QString &);

    [[nodiscard]] static BatchResult transferFiles(
        const QStringList &paths,
        const QString &destinationDirectory,
        FileOperation operation);
    void renameFile(const QString &path);
    void copySelected(const QStringList &paths);
    void moveSelected(const QStringList &paths);
    void trashSelected(const QStringList &paths);
    void reportBatch(Operation operation,
                     const BatchResult &result);
    void startBatch(
        Operation operation,
        std::function<BatchResult()> task);

    QWidget *m_dialogParent = nullptr;
    QThreadPool m_pool;
    bool m_busy = false;
};
