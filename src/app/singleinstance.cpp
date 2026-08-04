#include "singleinstance.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLockFile>
#include <QStandardPaths>

#include <unistd.h>

namespace {
constexpr qsizetype kMaximumIpcBytes = 16 * 1024 * 1024;
constexpr qsizetype kMaximumForwardedPaths = 16 * 1024;
constexpr qsizetype kMaximumPathCharacters = 32 * 1024;
}

SingleInstance::SingleInstance(QObject *parent)
    : QObject(parent)
{
}

SingleInstance::SingleInstance(
    const QString &instanceId,
    const QString &runtimeDirectory,
    QObject *parent)
    : QObject(parent)
    , m_instanceId(instanceId.trimmed())
    , m_runtimeDirectory(
          QDir(runtimeDirectory).absolutePath())
{
}

SingleInstance::~SingleInstance() = default;

bool SingleInstance::startOrForward(const QStringList &paths)
{
    m_lock = std::make_unique<QLockFile>(lockPath());
    m_lock->setStaleLockTime(0);
    if (!m_lock->tryLock()) {
        // The primary owns the lock before it starts listening. Retry briefly
        // so rapid file-manager launches cannot race into another process.
        for (int attempt = 0; attempt < 20; ++attempt) {
            if (forward(paths))
                return false;
        }
        // A terminated or crashed primary may leave the lock behind. QLockFile
        // validates the recorded PID and host before removing it, so only
        // recover after forwarding proved that no primary is listening.
        if (!m_lock->removeStaleLockFile()
            || !m_lock->tryLock()) {
            return false;
        }
    }

    QLocalServer::removeServer(serverName());
    m_server = std::make_unique<QLocalServer>();
    m_server->setSocketOptions(QLocalServer::UserAccessOption);
    if (!m_server->listen(serverName()))
        return false;

    connect(m_server.get(), &QLocalServer::newConnection, this, [this] {
        while (QLocalSocket *socket = m_server->nextPendingConnection()) {
            const auto consumeMessage = [this, socket] {
                if (socket->property("clearveilProcessed").toBool())
                    return;
                QByteArray buffer = socket->property("clearveilBuffer").toByteArray();
                buffer += socket->readAll();
                if (buffer.size() > kMaximumIpcBytes) {
                    socket->disconnectFromServer();
                    return;
                }
                const qsizetype newline = buffer.indexOf('\n');
                if (newline < 0) {
                    socket->setProperty("clearveilBuffer", buffer);
                    return;
                }

                QJsonParseError error;
                const QJsonDocument document =
                    QJsonDocument::fromJson(buffer.first(newline), &error);
                if (error.error == QJsonParseError::NoError
                    && document.isArray()
                    && document.array().size() <= kMaximumForwardedPaths) {
                    socket->setProperty("clearveilProcessed", true);
                    QStringList received;
                    for (const QJsonValue &value : document.array()) {
                        if (!value.isString())
                            continue;
                        const QString path = value.toString();
                        if (path.size() <= kMaximumPathCharacters)
                            received.append(path);
                    }
                    emit pathsReceived(received);
                }
                socket->disconnectFromServer();
            };
            connect(socket, &QLocalSocket::readyRead, socket, consumeMessage);
            connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
            if (socket->bytesAvailable() > 0)
                consumeMessage();
        }
    });
    return true;
}

bool SingleInstance::forward(const QStringList &paths)
{
    if (paths.size() > kMaximumForwardedPaths)
        return false;

    QJsonArray array;
    for (const QString &path : paths) {
        const QString absolutePath =
            QFileInfo(path).absoluteFilePath();
        if (absolutePath.size() > kMaximumPathCharacters)
            return false;
        array.append(absolutePath);
    }
    QByteArray message =
        QJsonDocument(array).toJson(QJsonDocument::Compact);
    message.append('\n');
    if (message.size() > kMaximumIpcBytes)
        return false;

    QLocalSocket socket;
    socket.connectToServer(serverName(), QIODevice::WriteOnly);
    if (!socket.waitForConnected(100))
        return false;

    if (socket.write(message) != message.size())
        return false;
    socket.flush();
    if (socket.bytesToWrite() > 0)
        socket.waitForBytesWritten(1000);
    socket.disconnectFromServer();
    if (socket.state() != QLocalSocket::UnconnectedState)
        socket.waitForDisconnected(1000);

    // Once a connected local socket accepts the complete message, retrying can
    // only create duplicates. QLocalSocket flushes pending bytes while
    // disconnecting, and the primary also guards each socket against a second
    // readyRead delivery.
    return true;
}

QString SingleInstance::serverName() const
{
    if (!m_runtimeDirectory.isEmpty()) {
        return QDir(m_runtimeDirectory).filePath(
            instanceKey() + QStringLiteral(".socket"));
    }
    return instanceKey();
}

QString SingleInstance::instanceKey() const
{
    const QString applicationName =
        !m_instanceId.isEmpty()
        ? m_instanceId
        : QCoreApplication::applicationName().isEmpty()
        ? QStringLiteral("clearveil")
        : QCoreApplication::applicationName();
    return QStringLiteral("%1-%2")
        .arg(applicationName, QString::number(getuid()));
}

QString SingleInstance::lockPath() const
{
    QString runtime = m_runtimeDirectory;
    if (runtime.isEmpty()) {
        runtime = QStandardPaths::writableLocation(
            QStandardPaths::RuntimeLocation);
    }
    if (runtime.isEmpty())
        runtime = QDir::tempPath();
    return QDir(runtime).filePath(
        instanceKey() + QStringLiteral(".lock"));
}
