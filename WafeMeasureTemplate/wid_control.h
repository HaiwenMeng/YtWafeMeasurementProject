#ifndef WID_CONTROL_H
#define WID_CONTROL_H

#include <QObject>
#include "tcp_communicator.h"

class WIDControl : public QObject
{
    Q_OBJECT
public:
    explicit WIDControl(QObject *parent = nullptr);

    void connectToController(const QString &host, quint16 port);
    void disconnectFromController();

    void readWaferID();
    inline bool getConnectFailState() { return m_bConnectFail; }

public slots:
    void OnConnectionStatusChanged(bool connected);
    void OnDataReceived(const QByteArray &data);
    void OnErrorOccurred(const QString &error);

signals:
    void s_writeLog(QString logStr);
    void s_sendErrorMsg(int sensor_id, QString lastErrorMsg);
    void s_waferID(QString waferID);

    // 连接状态改变信号
    void connectionStatusChanged(bool connected);
    void connectionTimeout(); //连接超时信号
    // 接收到数据信号（如果需要接收回复）
    void dataReceived(const QByteArray &data);
    // 错误信息信号
    void errorOccurred(const QString &error);

private:
    QSharedPointer<WIDTcpCommunicator> m_pTcpCommunicator = nullptr;

    bool m_bConnected = false;
    bool m_bConnectFail = false;
};

#endif // WID_CONTROL_H
