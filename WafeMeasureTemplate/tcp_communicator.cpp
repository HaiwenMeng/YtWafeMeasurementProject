#include "tcp_communicator.h"
#include <QDebug>

WIDTcpCommunicator::WIDTcpCommunicator(QObject *parent)
    : QObject{parent},
    m_connectedState(false),
    m_connectionTimeoutThreshold(5000)
{
    m_socket = new QTcpSocket(this);
    m_connectionMonitor = new QTimer(this);

    // 设置连接监测定时器（每秒检查一次）
    m_connectionMonitor->setInterval(1000);
    connect(m_connectionMonitor, &QTimer::timeout,
            this, &WIDTcpCommunicator::checkConnectionStatus);

    // 连接socket信号
    connect(m_socket, &QTcpSocket::connected,
            this, &WIDTcpCommunicator::onConnected);
    connect(m_socket, &QTcpSocket::disconnected,
            this, &WIDTcpCommunicator::onDisconnected);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, &WIDTcpCommunicator::onErrorOccurred);
    connect(m_socket, &QTcpSocket::readyRead,
            this, &WIDTcpCommunicator::onReadyRead);
}

void WIDTcpCommunicator::connectToHost(const QString &host, quint16 port)
{
    if(m_socket->state() != QAbstractSocket::ConnectedState) {
        m_socket->connectToHost(host, port);
        m_connectionStartTime = QDateTime::currentDateTime();
        m_connectionMonitor->start();
    }
}

void WIDTcpCommunicator::disconnectFromHost()
{
    m_socket->disconnectFromHost();
    m_connectionMonitor->stop();
}

void WIDTcpCommunicator::sendCommand(const QString &command, QString endFlag)
{
    // emit s_writeLog(QString(u8"发送：%1").arg(command));
    if(m_socket->state() == QAbstractSocket::ConnectedState) {
        QByteArray data = command.toUtf8() + endFlag.toUtf8();  // 根据实际协议添加结束符
        m_socket->write(data);
    }
}

void WIDTcpCommunicator::setUseEndFlag(bool bUseEndFlag)
{
    m_bUseEndFlag = bUseEndFlag;
}

void WIDTcpCommunicator::checkConnectionStatus()
{
    bool currentState = (m_socket->state() == QAbstractSocket::ConnectedState);
    if(currentState != m_connectedState) {
        m_connectedState = currentState;
        emit connectionStatusChanged(m_connectedState);
    }

    // 如果仍未连接，检查连接是否超时
    if (!m_connectedState) {
        qint64 elapsed = m_connectionStartTime.msecsTo(QDateTime::currentDateTime());
        if (elapsed >= m_connectionTimeoutThreshold) {
            qDebug() << "连接超时，请检查设备是否开机";
            emit connectionTimeout();
            m_connectionMonitor->stop();
            m_socket->abort();  // 中止当前连接尝试
        }
    }
}

void WIDTcpCommunicator::onConnected()
{
    m_connectedState = true;
    emit connectionStatusChanged(true);
}

void WIDTcpCommunicator::onDisconnected()
{
    m_connectedState = false;
    emit connectionStatusChanged(false);
}

void WIDTcpCommunicator::onErrorOccurred(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error)
    emit errorOccurred(m_socket->errorString());
}

void WIDTcpCommunicator::onReadyRead() {
    if(m_bUseEndFlag)
    {
        m_buffer.append(m_socket->readAll());

        // 缓冲区溢出保护
        if (m_buffer.size() > MAX_BUFFER_SIZE) {
            qWarning() << "Buffer overflow, resetting connection";
            m_socket->abort();
            m_buffer.clear();
            return;
        }

        // 协议解析循环
        while (true) {
            // 1. 查找起始符 '#'
            int startPos = m_buffer.indexOf('#');
            if (startPos == -1) {
                m_buffer.clear(); // 无有效起始符，清空缓冲区
                break;
            }

            // 2. 移除起始符前的无效数据
            if (startPos > 0) {
                qDebug() << "Discarding invalid data:" << m_buffer.left(startPos);
                m_buffer = m_buffer.mid(startPos);
            }

            // 3. 查找结束符 "\r\n"
            int endPos = m_buffer.indexOf("\r\n");
            if (endPos == -1) break; // 无完整指令

            // 4. 提取有效指令（从 # 到 \r\n 之前）
            QByteArray command = m_buffer.mid(0, endPos);
            m_buffer = m_buffer.mid(endPos + 2); // 移除已处理数据

            // 5. 验证指令格式（至少包含 # 和有效内容）
            if (command.length() > 1) {
                emit dataReceived(command);
            }
            // else {
            //     qWarning() << "Empty command received";
            // }
        }
    }
    else
    {
        QByteArray received = m_socket->readAll();
        emit dataReceived(received);
    }
}
