#pragma once

#include <QObject>
#include <QString>

class QFileSystemWatcher;
class QTimer;

class DirectoryMonitor final : public QObject
{
    Q_OBJECT

public:
    explicit DirectoryMonitor(QObject *parent = nullptr);

    void setDirectory(const QString &directoryPath);
    [[nodiscard]] QString directory() const;

signals:
    void refreshRequested(const QString &directoryPath);

private:
    void restoreWatch();

    QFileSystemWatcher *m_watcher = nullptr;
    QTimer *m_debounceTimer = nullptr;
    QString m_directory;
};
