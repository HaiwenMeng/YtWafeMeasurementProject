#include "TcpCommunicator.h"

namespace
{
const int kConnectionCheckIntervalMs = 100;
const int kMaxBufferSize = 1024 * 1024;
}

TcpCommunicator::TcpCommunicator(QObject *parent)
    : QObject(parent),
      m_socket(new QTcpSocket(this)),
      m_connectionTimer(new QTimer(this)),
      m_connected(false),
      m_useEndFlag(true),
      m_connectionTimeoutMs(5000)
{
    m_connectionTimer->setInterval(kConnectionCheckIntervalMs);

    connect(m_connectionTimer, &QTimer::timeout,
            this, &TcpCommunicator::checkConnectionTimeout);
    connect(m_socket, &QTcpSocket::connected,
            this, &TcpCommunicator::onConnected);
    connect(m_socket, &QTcpSocket::disconnected,
            this, &TcpCommunicator::onDisconnected);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, &TcpCommunicator::onErrorOccurred);
    connect(m_socket, &QTcpSocket::readyRead,
            this, &TcpCommunicator::onReadyRead);
}

void TcpCommunicator::connectToHost(const QString &host, quint16 port, int timeoutMs)
{
    if (host.trimmed().isEmpty()) {
        emitSocketError(tr("Host is empty."));
        return;
    }

    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        emit logMessage(tr("Already connected to %1:%2").arg(host).arg(port));
        return;
    }

    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->abort();
    }

    m_connectionTimeoutMs = timeoutMs > 0 ? timeoutMs : 5000;
    emit logMessage(tr("Connecting to %1:%2").arg(host).arg(port));
    m_socket->connectToHost(host, port);
    m_connectionElapsed.restart();
    m_connectionTimer->start();
}

void TcpCommunicator::disconnectFromHost()
{
    m_connectionTimer->stop();

    if (m_socket->state() == QAbstractSocket::UnconnectedState) {
        if (m_connected) {
            m_connected = false;
            emit connectionStatusChanged(false);
        }
        return;
    }

    emit logMessage(tr("Disconnecting from controller."));
    m_socket->disconnectFromHost();
}

bool TcpCommunicator::sendCommand(const QString &command, const QString &endFlag)
{
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        emitSocketError(tr("Send failed, socket is not connected: %1").arg(command));
        return false;
    }

    if (command.trimmed().isEmpty()) {
        emitSocketError(tr("Send failed, command is empty."));
        return false;
    }

    const QByteArray data = command.toUtf8() + endFlag.toUtf8();
    const qint64 written = m_socket->write(data);
    if (written != data.size()) {
        emitSocketError(tr("Send failed, wrote %1 of %2 bytes: %3")
                        .arg(written).arg(data.size()).arg(command));
        return false;
    }

    if (!m_socket->flush()) {
        emitSocketError(tr("Socket flush failed: %1").arg(command));
        return false;
    }

    emit logMessage(tr("TX: %1").arg(command));
    return true;
}

bool TcpCommunicator::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

void TcpCommunicator::setUseEndFlag(bool useEndFlag)
{
    m_useEndFlag = useEndFlag;
}

void TcpCommunicator::checkConnectionTimeout()
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_connectionTimer->stop();
        return;
    }

    if (m_socket->state() == QAbstractSocket::ConnectingState) {
        if (!m_connectionElapsed.isValid()) {
            m_connectionElapsed.restart();
        }
        if (m_connectionElapsed.elapsed() < m_connectionTimeoutMs) {
            return;
        }
        m_connectionTimer->stop();
        m_connectionElapsed.invalidate();
        m_socket->abort();
        emitSocketError(tr("Connection timeout."));
        emit connectionTimeout();
        return;
    }

    m_connectionTimer->stop();
    m_connectionElapsed.invalidate();
}

void TcpCommunicator::onConnected()
{
    m_connectionTimer->stop();
    m_connectionElapsed.invalidate();
    m_connected = true;
    emit logMessage(tr("Controller connected."));
    emit connectionStatusChanged(true);
}

void TcpCommunicator::onDisconnected()
{
    m_connectionTimer->stop();
    m_connectionElapsed.invalidate();
    m_connected = false;
    m_buffer.clear();
    emit logMessage(tr("Controller disconnected."));
    emit connectionStatusChanged(false);
}

void TcpCommunicator::onErrorOccurred(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error)
    emitSocketError(m_socket->errorString());
}

void TcpCommunicator::onReadyRead()
{
    if (!m_useEndFlag) {
        const QByteArray data = m_socket->readAll();
        if (!data.isEmpty()) {
            emit dataReceived(data);
        }
        return;
    }

    m_buffer.append(m_socket->readAll());
    if (m_buffer.size() > kMaxBufferSize) {
        m_buffer.clear();
        emitSocketError(tr("Receive buffer overflow."));
        m_socket->abort();
        return;
    }

    while (true) {
        const int startPos = m_buffer.indexOf('#');
        if (startPos < 0) {
            if (!m_buffer.isEmpty()) {
                emit logMessage(tr("Discard RX data without start flag."));
                m_buffer.clear();
            }
            return;
        }

        if (startPos > 0) {
            m_buffer.remove(0, startPos);
        }

        const int endPos = m_buffer.indexOf("\r\n");
        if (endPos < 0) {
            return;
        }

        const QByteArray frame = m_buffer.left(endPos);
        m_buffer.remove(0, endPos + 2);

        if (frame.size() <= 1) {
            emitSocketError(tr("Received empty frame."));
            continue;
        }

        emit dataReceived(frame);
    }
}

void TcpCommunicator::emitSocketError(const QString &message)
{
    const QString text = message.trimmed().isEmpty() ? tr("Unknown socket error.") : message;
    emit logMessage(tr("ERROR: %1").arg(text));
    emit errorOccurred(text);
}
