#include "serial_communicator.h"
#include <QDebug>

SerialCommunicator::SerialCommunicator(QObject *parent): QObject(parent), m_serial(new QSerialPort(this)),
    m_timer(new QTimer(this)), m_timeout(1000)
{
   
    connect(m_timer, &QTimer::timeout, this, &SerialCommunicator::handleTimeout);
  /*  m_timer->setSingleShot(true);*/
  
    m_timer->start(2000);
}

SerialCommunicator::~SerialCommunicator()
{
    closePort();
}

bool SerialCommunicator::openPort(const QString &portName, qint32 baudRate,
                                  QSerialPort::DataBits dataBits,
                                  QSerialPort::Parity parity,
                                  QSerialPort::StopBits stopBits)
{
    if(m_serial->isOpen()) {
        emit errorOccurred("Port already open");
        return false;
    }

    m_serial->setPortName(portName);
    m_serial->setBaudRate(baudRate);
    m_serial->setDataBits(dataBits);
    m_serial->setParity(parity);
    m_serial->setStopBits(stopBits);

    if(!m_serial->open(QIODevice::ReadWrite)) {
        emit errorOccurred("Open failed: " + m_serial->errorString());
        return false;
    }
    else
    {
        connect(m_serial, &QSerialPort::readyRead, this, &SerialCommunicator::handleReadyRead);
    }
    return true;
}

void SerialCommunicator::closePort()
{
    if(m_serial->isOpen()) {
        m_serial->close();
    }
}

bool SerialCommunicator::sendCommand(const QString &command, int timeoutMs)
{
    if(!m_serial->isOpen()) {
        emit errorOccurred("Port not open");
        return false;
    }

    m_timeout = timeoutMs;
    m_sentCommand = command;
    m_receiveBuffer.clear();

   // QByteArray data = command.toUtf8() + "\r\n";
    QByteArray data = QByteArray::fromHex(command.toUtf8());
    qint64 written = m_serial->write(data);

    if(written == -1) {
        emit errorOccurred("Write error: " + m_serial->errorString());
        return false;
    }

    if(!m_serial->waitForBytesWritten(m_timeout)) {
        emit errorOccurred("Write timeout");
        return false;
    }

    // m_timer->start(m_timeout);
    return true;
}

void SerialCommunicator::handleReadyRead()
{
    m_receiveBuffer += m_serial->readAll();
    // m_timer->start(m_timeout); // 收到数据时重置超时计时器
    
    auto hex = m_receiveBuffer.toHex();
    Temperature = hex.mid(6, 4).toInt(NULL, 16) /10.0;
    Humidity = hex.mid(10, 4).toInt(NULL, 16) / 10.0;
    QString received = QString::fromUtf8(hex).trimmed();
   
    bool match = (received == m_sentCommand);

    emit resultReady(match);
    m_receiveBuffer.clear();
}

void SerialCommunicator::handleTimeout()
{
    sendCommand(GET_TEMPER_COMMAND);
    QString received = QString::fromUtf8(m_receiveBuffer).trimmed();
    bool match = (received == m_sentCommand);
    if (!match)
    {

    }
    else
    {

    }
    emit resultReady(match);
    m_receiveBuffer.clear();
}

bool SerialCommunicator::openAndSendCommand(const QString &portName, const QString &command)
{
    if(openPort(portName))
    {
        emit s_writeLog(QString(u8"触发器串口%1打开成功").arg(portName));

        if(sendCommand(command))
        {
            emit s_writeLog(u8"发送触发信号成功");
            return true;
        }
        else
        {
            emit s_writeLog(u8"发送触发信号失败");
            // QMessageBox::warning(this, u8"错误", u8"发送触发信号失败");
            return false;
        }
    }
    else
    {
        QString errMsg = QString(u8"触发器串口%1打开失败").arg(portName);
        emit s_writeLog(errMsg);
        // QMessageBox::warning(this, u8"错误", errMsg);
        return false;
    }
}

bool SerialCommunicator::stopAndClose(const QString &command)
{
    if(sendCommand(command))
    {
        closePort();
        return true;
    }

    return false;
}
