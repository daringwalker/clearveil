#pragma once

#include <QObject>
#include <QStringList>

#include <memory>

class QLocalServer;
class QLockFile;

class SingleInstance : public QObject
{
    Q_OBJECT

public:
    explicit SingleInstance(QObject *parent = nullptr);
    SingleInstance(const QString &instanceId,
                   const QString &runtimeDirectory,
                   QObject *parent = nullptr);
    ~SingleInstance() override;

    bool startOrForward(const QStringList &paths);

signals:
    void pathsReceived(const QStringList &paths);

private:
    bool forward(const QStringList &paths);
    QString instanceKey() const;
    QString serverName() const;
    QString lockPath() const;

    std::unique_ptr<QLockFile> m_lock;
    std::unique_ptr<QLocalServer> m_server;
    QString m_instanceId;
    QString m_runtimeDirectory;
};
