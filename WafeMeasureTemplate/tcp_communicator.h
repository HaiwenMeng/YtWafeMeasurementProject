#ifndef TCP_COMMUNICATOR_H
#define TCP_COMMUNICATOR_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QRegularExpression>
#include <QDateTime>

const long long int MAX_BUFFER_SIZE = 1024 * 1024;

class WIDTcpCommunicator : public QObject
{
    Q_OBJECT
public:
    explicit WIDTcpCommunicator(QObject *parent = nullptr);

    // 连接服务器
    Q_INVOKABLE void connectToHost(const QString &host, quint16 port);
    // 断开连接
    Q_INVOKABLE void disconnectFromHost();
    // 发送指令
    Q_INVOKABLE void sendCommand(const QString &command, QString endFlag = "\r\n");
    //设置是否使用缓存判断结束符标识
    Q_INVOKABLE void setUseEndFlag(bool bUseEndFlag = true);

signals:
    // 连接状态改变信号
    void connectionStatusChanged(bool connected);
    // 接收到数据信号（如果需要接收回复）
    void dataReceived(const QByteArray &data);
    // 错误信息信号
    void errorOccurred(const QString &error);
    void s_writeLog(QString logStr);
    void connectionTimeout(); //连接超时信号

public slots:
    // 连接状态监测
    void checkConnectionStatus();

private slots:
    void onConnected();
    void onDisconnected();
    void onErrorOccurred(QAbstractSocket::SocketError error);
    void onReadyRead();

private:
    QTcpSocket *m_socket;
    QByteArray m_buffer;  // 新增数据缓冲区
    QTimer *m_connectionMonitor;
    bool m_connectedState;
    int m_connectionTimeoutThreshold; // 超时时间，单位毫秒
    QDateTime m_connectionStartTime;

    QRegularExpression regExp = QRegularExpression("^#[a-z]+\\s+index,\\d+$");

    bool m_bUseEndFlag = true;
};

#endif // TCP_COMMUNICATOR_H

