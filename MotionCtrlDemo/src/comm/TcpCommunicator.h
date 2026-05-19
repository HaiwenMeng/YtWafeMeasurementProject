#ifndef TCPCOMMUNICATOR_H
#define TCPCOMMUNICATOR_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QElapsedTimer>

class TcpCommunicator : public QObject
{
    Q_OBJECT

public:
    explicit TcpCommunicator(QObject *parent = nullptr);

    void connectToHost(const QString &host, quint16 port, int timeoutMs = 5000);
    void disconnectFromHost();
    bool sendCommand(const QString &command, const QString &endFlag = "\r\n");
    bool isConnected() const;
    void setUseEndFlag(bool useEndFlag);

signals:
    void connectionStatusChanged(bool connected);
    void dataReceived(const QByteArray &data);
    void errorOccurred(const QString &errorMessage);
    void logMessage(const QString &message);
    void connectionTimeout();

private slots:
    void checkConnectionTimeout();
    void onConnected();
    void onDisconnected();
    void onErrorOccurred(QAbstractSocket::SocketError error);
    void onReadyRead();

private:
    void emitSocketError(const QString &message);

    QTcpSocket *m_socket;
    QTimer *m_connectionTimer;
    QByteArray m_buffer;
    bool m_connected;
    bool m_useEndFlag;
    int m_connectionTimeoutMs;
    QElapsedTimer m_connectionElapsed;
};

#endif // TCPCOMMUNICATOR_H
