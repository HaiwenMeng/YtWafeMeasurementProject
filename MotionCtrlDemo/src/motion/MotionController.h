#ifndef MOTIONCONTROLLER_H
#define MOTIONCONTROLLER_H

#include "MotionTypes.h"
#include "../comm/TcpCommunicator.h"

#include <QObject>
#include <QTimer>

struct AxisStatus
{
    double position = 0.0;
    bool isEnabled = false;
    bool isHomed = false;
    bool calibrationEnabled1D = false;
    bool calibrationEnabled2D = false;
};

struct AxisPos
{
    double pos_x = 0.0;
    double pos_y = 0.0;
    double pos_z = 0.0;
};

struct AxisState
{
    int state_x = -1;
    int state_y = -1;
    int state_z = -1;
    int error_x = -1;
    int error_y = -1;
    int error_z = -1;
};

class MotionController : public QObject
{
    Q_OBJECT

public:
    explicit MotionController(QObject *parent = nullptr);

    void connectToController(const QString &host, quint16 port);
    void disconnectFromController();
    bool enableAxis(int axis);
    bool disableAxis(int axis);
    bool homeAxisAsync(int axis);
    bool abortAxis(int axis);
    bool abortAxes();
    bool clearError();
    bool getErrors(int axis);
    bool moveSingleAxisAbsolute(int axis, double position);
    bool moveSingleAxisIncremental(int axis, double distance);
    bool moveSingleAxisContinuous(int axis, int direction);
    bool moveMultiAxisLinear(double posX, double posY, double velocity, double acceleration, double deceleration);
    bool setVelocity(int axis, double velocity, double acceleration, double deceleration);
    bool readRealTimePos();
    bool readRealTimeStatus();
    bool ctrlAutoSnd(int interval);
    bool getHomestatus();
    bool setTriggerParam(const Motion::TriggerParam &param);
    bool controlTrigger(int axis, bool enabled);
    bool SetTriggerParam(int axis, double startPos, double endPos, double interval, int direction, int pulseWidth = 500);
    bool ControlTrigger(int axis, int state);

    Motion::AxisPosition getAxesCurrentPos() const;
    Motion::AxisStateSnapshot getAxesCurrentState() const;
    void getAxesCurrentPos(double &posX, double &posY, double &posZ) const;
    void getAxesCurrentState(int &stateX, int &stateY, int &stateZ) const;
    bool IsAxisBusying(int axis) const;
    bool isConnected() const;
    bool getConnectState() const;
    bool getConnectFailState() const;
    void setInitFlag(bool isInit);

    int m_state_x = -1;
    int m_state_y = -1;
    int m_state_z = -1;

signals:
    void connectionStatusChanged(bool connected);
    void connectionTimeout();
    void logMessage(const QString &message);
    void errorMessage(const QString &message);
    void s_writeLog(QString message);
    void s_sendErrorMsg(int sensorId, QString message);
    void positionUpdated(const Motion::AxisPosition &position);
    void stateUpdated(const Motion::AxisStateSnapshot &state);
    void s_axis_pos(AxisPos position);
    void s_axis_state(AxisState state);
    void s_bIsHomeIng();
    void s_initOver();
    void homeStatusUpdated(bool homeX, bool homeY, bool homeZ);
    void rawDataReceived(const QString &data);
    void stateNotReady(const QString &message);
    void s_stateNotReady();

private slots:
    void onDataReceived(const QByteArray &data);
    void onConnectionStatusChanged(bool connected);
    void onErrorOccurred(const QString &message);

private:
    bool sendCommand(const QString &command);
    bool sendCommands(const QStringList &commands);
    bool ensureConnected(const QString &actionName) const;
    bool ensureKnownAxis(int axis, const QString &actionName) const;
    bool ensureAxisReady(int axis, const QString &actionName) const;
    bool ensureAxesReadyForLine() const;
    bool validatePositive(const QString &name, double value) const;
    QStringList servoCommandsForAxis(int axis, int enabled) const;
    void parseFrame(const QString &frame);
    bool parseAutoSnd(const QStringList &parts, const QString &frame);
    bool parseCommandReply(const QStringList &parts, const QString &frame);
    bool parsePayload(const QString &payload, QList<double> *values, QString *error) const;
    void updatePosition(int axis, double value);
    void updateState(int axis, int value);
    void updateError(int axis, int value);
    void updateHome(int axis, bool value);
    int currentStateForAxis(int axis) const;
    int currentErrorForAxis(int axis) const;
    bool currentStateKnownForAxis(int axis) const;
    bool parseLastInt(const QList<double> &values, int *value) const;
    void handleTimeoutBeforeHomeCommand();
    void handleTimeoutAfterHomeCommand();

    TcpCommunicator m_comm;
    Motion::AxisPosition m_position;
    Motion::AxisStateSnapshot m_state;
    bool m_homeX;
    bool m_homeY;
    bool m_homeZ;
    bool m_homeValidX;
    bool m_homeValidY;
    bool m_homeValidZ;
    bool m_connectFailed = false;
    bool m_isInit = false;
    bool m_initSequenceActive = false;
    bool m_initHomeIssued = false;
    QTimer *m_timerBeforeHomeCommand = nullptr;
    QTimer *m_timerAfterHomeCommand = nullptr;
};

Q_DECLARE_METATYPE(AxisPos)
Q_DECLARE_METATYPE(AxisState)

#endif // MOTIONCONTROLLER_H
