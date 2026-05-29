#ifndef SERIAL_COMMUNICATOR_H
#define SERIAL_COMMUNICATOR_H

#include <QObject>
#include <QSerialPort>
#include <QTimer>
#define GET_TEMPER_COMMAND "01 03 00 00 00 02 C4 0B"
class SerialCommunicator : public QObject
{
    Q_OBJECT
public:
    explicit SerialCommunicator(QObject *parent = nullptr);
    ~SerialCommunicator();

    bool openPort(const QString &portName,
                  qint32 baudRate = QSerialPort::Baud115200,
                  QSerialPort::DataBits dataBits = QSerialPort::Data8,
                  QSerialPort::Parity parity = QSerialPort::NoParity,
                  QSerialPort::StopBits stopBits = QSerialPort::OneStop);

    void closePort();
    Q_INVOKABLE bool sendCommand(const QString &command, int timeoutMs = 1000);
    Q_INVOKABLE bool openAndSendCommand(const QString &portName, const QString &command);
    Q_INVOKABLE bool stopAndClose(const QString &command);
    double Temperature = 0;
    double Humidity=0;
signals:
    void resultReady(bool isMatch);
    void errorOccurred(const QString &error);
    void s_writeLog(QString logStr);

private slots:
    void handleReadyRead();
    void handleTimeout();

private:
    QSerialPort *m_serial;
   
    QString m_sentCommand;
    QByteArray m_receiveBuffer;
    QTimer* m_timer;
    int m_timeout=1000;
};

#endif // SERIAL_COMMUNICATOR_H
