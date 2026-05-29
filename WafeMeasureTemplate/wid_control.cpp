#include "wid_control.h"

WIDControl::WIDControl(QObject *parent)
    : QObject{parent}
{
    m_pTcpCommunicator.reset(new WIDTcpCommunicator);
    m_pTcpCommunicator->setUseEndFlag(false);

    connect(m_pTcpCommunicator.data(), SIGNAL(connectionTimeout), this, SIGNAL(connectionTimeout));
    connect(m_pTcpCommunicator.data(), SIGNAL(connectionStatusChanged(bool)), this, SIGNAL(connectionStatusChanged(bool)));
    // connect(m_pTcpCommunicator.data(), SIGNAL(dataReceived(QByteArray)), this, SIGNAL(dataReceived(QByteArray)));
    // connect(m_pTcpCommunicator.data(), SIGNAL(errorOccurred(QString)), this, SIGNAL(errorOccurred(QString)));

    connect(m_pTcpCommunicator.data(), &WIDTcpCommunicator::connectionTimeout, this, [this](){ m_bConnectFail = true; emit s_writeLog(u8"读码器连接失败"); });
    connect(m_pTcpCommunicator.data(), SIGNAL(connectionStatusChanged(bool)), this, SLOT(OnConnectionStatusChanged(bool)));
    connect(m_pTcpCommunicator.data(), SIGNAL(dataReceived(QByteArray)), this, SLOT(OnDataReceived(QByteArray)));
    connect(m_pTcpCommunicator.data(), SIGNAL(errorOccurred(QString)), this, SLOT(OnErrorOccurred(QString)));
    connect(m_pTcpCommunicator.data(), SIGNAL(s_writeLog(QString)), this, SIGNAL(s_writeLog(QString)));
}

void WIDControl::OnConnectionStatusChanged(bool connected)
{
    m_bConnected = connected;
}

void WIDControl::OnDataReceived(const QByteArray &data)
{
    QString received_str = QString::fromUtf8(data);

    emit s_waferID(received_str);
}

void WIDControl::OnErrorOccurred(const QString &error)
{

}


void WIDControl::connectToController(const QString &host, quint16 port)
{
    QMetaObject::invokeMethod(m_pTcpCommunicator.data(), [=]() {
        m_pTcpCommunicator->connectToHost(host, port);
    }, Qt::QueuedConnection);
}

void WIDControl::disconnectFromController()
{
    QMetaObject::invokeMethod(m_pTcpCommunicator.data(), [=]() {
        m_pTcpCommunicator->disconnectFromHost();
    }, Qt::QueuedConnection);
}

void WIDControl::readWaferID()
{
    if(!m_bConnected)
    {
        emit s_writeLog(u8"WID未连接，请停止测量后检查读码器连接.");
        emit s_sendErrorMsg(0, u8"WID未连接，请停止测量后检查读码器连接.");
        return;
    }

    QString command = "t";

    QMetaObject::invokeMethod(m_pTcpCommunicator.data(), [=]() {
        m_pTcpCommunicator->sendCommand(command, "");
    }, Qt::QueuedConnection);
}
