#include "MotionController.h"

#include <QMetaType>
#include <QtMath>

using namespace Motion;

namespace
{
bool isReadyState(int state)
{
    return state == 3;
}
}

MotionController::MotionController(QObject *parent)
    : QObject(parent),
      m_homeX(false),
      m_homeY(false),
      m_homeZ(false),
      m_homeValidX(false),
      m_homeValidY(false),
      m_homeValidZ(false)
{
    qRegisterMetaType<Motion::AxisPosition>("Motion::AxisPosition");
    qRegisterMetaType<Motion::AxisStateSnapshot>("Motion::AxisStateSnapshot");
    qRegisterMetaType<AxisPos>("AxisPos");
    qRegisterMetaType<AxisState>("AxisState");

    connect(&m_comm, &TcpCommunicator::connectionStatusChanged,
            this, &MotionController::onConnectionStatusChanged);
    connect(&m_comm, &TcpCommunicator::dataReceived,
            this, &MotionController::onDataReceived);
    connect(&m_comm, &TcpCommunicator::errorOccurred,
            this, &MotionController::onErrorOccurred);
    connect(&m_comm, &TcpCommunicator::logMessage,
            this, &MotionController::logMessage);
}

void MotionController::connectToController(const QString &host, quint16 port)
{
    m_comm.connectToHost(host, port);
}

void MotionController::disconnectFromController()
{
    if (m_comm.isConnected()) {
        ctrlAutoSnd(0);
    }
    m_comm.disconnectFromHost();
}

bool MotionController::enableAxis(int axis)
{
    if (!ensureKnownAxis(axis, "enableAxis")) {
        return false;
    }
    if (!ensureConnected("enableAxis")) {
        return false;
    }

    return sendCommands(servoCommandsForAxis(axis, 1));
}

bool MotionController::disableAxis(int axis)
{
    if (!ensureKnownAxis(axis, "disableAxis")) {
        return false;
    }
    if (!ensureConnected("disableAxis")) {
        return false;
    }

    return sendCommands(servoCommandsForAxis(axis, 0));
}

bool MotionController::homeAxisAsync(int axis)
{
    if (!ensureKnownAxis(axis, "homeAxisAsync")) {
        return false;
    }
    if (!ensureAxisReady(axis, "homeAxisAsync")) {
        return false;
    }

    return sendCommand(commandHome(CmdHome, axis));
}

bool MotionController::abortAxis(int axis)
{
    if (!ensureKnownAxis(axis, "abortAxis")) {
        return false;
    }
    if (!ensureConnected("abortAxis")) {
        return false;
    }

    return sendCommand(commandHalt(CmdHalt, axis));
}

bool MotionController::abortAxes()
{
    if (!ensureConnected("abortAxes")) {
        return false;
    }

    QStringList commands;
    commands << commandHaltLineXy(CmdHaltLineXy)
             << commandHalt(CmdHalt, AxisX)
             << commandHalt(CmdHalt, AxisY)
             << commandHalt(CmdHalt, AxisZ);
    return sendCommands(commands);
}

bool MotionController::clearError()
{
    if (!ensureConnected("clearError")) {
        return false;
    }

    QStringList commands;
    commands << commandClear(CmdClear, AxisX)
             << commandClear(CmdClear, AxisY)
             << commandClear(CmdClear, AxisZ);
    return sendCommands(commands);
}

bool MotionController::getErrors(int axis)
{
    if (!ensureKnownAxis(axis, "getErrors")) {
        return false;
    }
    if (!ensureConnected("getErrors")) {
        return false;
    }

    return sendCommand(commandErrorStatus(errorStatusCommandId(axis), axis));
}

bool MotionController::moveSingleAxisAbsolute(int axis, double position)
{
    if (!qIsFinite(position)) {
        emit errorMessage(tr("Absolute position is not a finite number."));
        return false;
    }
    if (!ensureAxisReady(axis, "moveSingleAxisAbsolute")) {
        return false;
    }

    return sendCommand(commandAbs(CmdAbs, axis, position));
}

bool MotionController::moveSingleAxisIncremental(int axis, double distance)
{
    if (!qIsFinite(distance)) {
        emit errorMessage(tr("Incremental distance is not a finite number."));
        return false;
    }
    if (!ensureAxisReady(axis, "moveSingleAxisIncremental")) {
        return false;
    }

    return sendCommand(commandRel(CmdRel, axis, distance));
}

bool MotionController::moveSingleAxisContinuous(int axis, int direction)
{
    if (direction != 0 && direction != 1) {
        emit errorMessage(tr("Continuous direction must be 0 or 1."));
        return false;
    }
    if (!ensureAxisReady(axis, "moveSingleAxisContinuous")) {
        return false;
    }

    return sendCommand(commandVel(CmdVel, axis, direction));
}

bool MotionController::moveMultiAxisLinear(double posX, double posY, double velocity, double acceleration, double deceleration)
{
    if (!qIsFinite(posX) || !qIsFinite(posY)) {
        emit errorMessage(tr("XY target position is not a finite number."));
        return false;
    }
    if (!validatePositive("XY velocity", velocity) ||
        !validatePositive("XY acceleration", acceleration) ||
        !validatePositive("XY deceleration", deceleration)) {
        return false;
    }
    if (!ensureAxesReadyForLine()) {
        return false;
    }

    return sendCommand(commandLineXy(CmdLineXy, posX, posY, velocity, acceleration, deceleration));
}

bool MotionController::setVelocity(int axis, double velocity, double acceleration, double deceleration)
{
    if (!ensureKnownAxis(axis, "setVelocity")) {
        return false;
    }
    if (!validatePositive("Velocity", velocity) ||
        !validatePositive("Acceleration", acceleration) ||
        !validatePositive("Deceleration", deceleration)) {
        return false;
    }
    if (!ensureAxisReady(axis, "setVelocity")) {
        return false;
    }

    QStringList commands;
    commands << commandSetSpeed(CmdSetSpeed, axis, velocity)
             << commandSetAcceleration(CmdSetAcceleration, axis, acceleration)
             << commandSetDeceleration(CmdSetDeceleration, axis, deceleration);
    return sendCommands(commands);
}

bool MotionController::readRealTimePos()
{
    if (!ensureConnected("readRealTimePos")) {
        return false;
    }

    QStringList commands;
    commands << commandReadCurrentPos(CmdReadCurrentPosX, AxisX)
             << commandReadCurrentPos(CmdReadCurrentPosY, AxisY)
             << commandReadCurrentPos(CmdReadCurrentPosZ, AxisZ);
    return sendCommands(commands);
}

bool MotionController::readRealTimeStatus()
{
    if (!ensureConnected("readRealTimeStatus")) {
        return false;
    }

    QStringList commands;
    commands << commandStatus(CmdStatusX, AxisX)
             << commandStatus(CmdStatusY, AxisY)
             << commandStatus(CmdStatusZ, AxisZ)
             << commandErrorStatus(CmdErrorStatusX, AxisX)
             << commandErrorStatus(CmdErrorStatusY, AxisY)
             << commandErrorStatus(CmdErrorStatusZ, AxisZ);
    return sendCommands(commands);
}

bool MotionController::ctrlAutoSnd(int interval)
{
    if (interval < 0) {
        emit errorMessage(tr("Auto send interval must be greater than or equal to 0."));
        return false;
    }
    if (!ensureConnected("ctrlAutoSnd")) {
        return false;
    }

    const int commandId = interval > 0 ? CmdCtrlAutoSndStart : CmdCtrlAutoSndStop;
    return sendCommand(commandCtrlAutoSnd(commandId, interval));
}

bool MotionController::getHomestatus()
{
    if (!ensureConnected("getHomestatus")) {
        return false;
    }

    QStringList commands;
    commands << commandHomeStatus(CmdHomeStatusX, AxisX)
             << commandHomeStatus(CmdHomeStatusY, AxisY)
             << commandHomeStatus(CmdHomeStatusZ, AxisZ);
    return sendCommands(commands);
}

bool MotionController::setTriggerParam(const TriggerParam &param)
{
    if (!ensureKnownAxis(param.axis, "setTriggerParam")) {
        return false;
    }
    if (!qIsFinite(param.startPos) || !qIsFinite(param.endPos)) {
        emit errorMessage(tr("Trigger start or end position is not a finite number."));
        return false;
    }
    if (!validatePositive("Trigger interval", param.interval)) {
        return false;
    }
    if (param.direction != 0 && param.direction != 1) {
        emit errorMessage(tr("Trigger direction must be 0 or 1."));
        return false;
    }
    if (param.pulseWidth <= 0) {
        emit errorMessage(tr("Trigger pulse width must be greater than 0."));
        return false;
    }
    if (!ensureConnected("setTriggerParam")) {
        return false;
    }

    return sendCommand(commandSetPcom1Param(param));
}

bool MotionController::controlTrigger(int axis, bool enabled)
{
    if (!ensureKnownAxis(axis, "controlTrigger")) {
        return false;
    }
    if (!ensureConnected("controlTrigger")) {
        return false;
    }

    return sendCommand(commandCtrlPcom1(axis, enabled ? 1 : 0));
}

bool MotionController::SetTriggerParam(int axis, double startPos, double endPos, double interval, int direction, int pulseWidth)
{
    TriggerParam param;
    param.axis = axis;
    param.startPos = startPos;
    param.endPos = endPos;
    param.interval = interval;
    param.direction = direction;
    param.pulseWidth = pulseWidth;
    return setTriggerParam(param);
}

bool MotionController::ControlTrigger(int axis, int state)
{
    return controlTrigger(axis, state != 0);
}

AxisPosition MotionController::getAxesCurrentPos() const
{
    return m_position;
}

AxisStateSnapshot MotionController::getAxesCurrentState() const
{
    return m_state;
}

void MotionController::getAxesCurrentPos(double &posX, double &posY, double &posZ) const
{
    posX = m_position.x;
    posY = m_position.y;
    posZ = m_position.z;
}

void MotionController::getAxesCurrentState(int &stateX, int &stateY, int &stateZ) const
{
    stateX = m_state.stateX;
    stateY = m_state.stateY;
    stateZ = m_state.stateZ;
}

bool MotionController::IsAxisBusying(int axis) const
{
    return !currentStateKnownForAxis(axis) || !isReadyState(currentStateForAxis(axis));
}

bool MotionController::isConnected() const
{
    return m_comm.isConnected();
}

bool MotionController::getConnectState() const
{
    return isConnected();
}

bool MotionController::getConnectFailState() const
{
    return m_connectFailed;
}

void MotionController::setInitFlag(bool isInit)
{
    m_isInit = isInit;
}

void MotionController::onDataReceived(const QByteArray &data)
{
    const QString frame = QString::fromUtf8(data).trimmed();
    if (frame.isEmpty()) {
        emit errorMessage(tr("Received empty frame."));
        return;
    }

    emit rawDataReceived(frame);
    emit logMessage(QString("RX: %1").arg(frame));
    parseFrame(frame);
}

void MotionController::onConnectionStatusChanged(bool connected)
{
    m_connectFailed = !connected;
    emit connectionStatusChanged(connected);
    if (connected && m_isInit) {
        emit s_initOver();
    }
}

void MotionController::onErrorOccurred(const QString &message)
{
    m_connectFailed = true;
    emit errorMessage(message);
    emit s_sendErrorMsg(0, message);
    emit s_writeLog(message);
    emit connectionTimeout();
}

bool MotionController::sendCommand(const QString &command)
{
    return m_comm.sendCommand(command);
}

bool MotionController::sendCommands(const QStringList &commands)
{
    if (commands.isEmpty()) {
        emit errorMessage(tr("No command to send."));
        return false;
    }

    bool ok = true;
    for (const QString &command : commands) {
        ok = m_comm.sendCommand(command) && ok;
    }
    return ok;
}

bool MotionController::ensureConnected(const QString &actionName) const
{
    if (!m_comm.isConnected()) {
        emit const_cast<MotionController *>(this)->errorMessage(
            tr("%1 failed, controller is not connected.").arg(actionName));
        return false;
    }
    return true;
}

bool MotionController::ensureKnownAxis(int axis, const QString &actionName) const
{
    if (!isKnownAxis(axis)) {
        emit const_cast<MotionController *>(this)->errorMessage(
            tr("%1 failed, unknown axis: %2").arg(actionName).arg(axis));
        return false;
    }
    return true;
}

bool MotionController::ensureAxisReady(int axis, const QString &actionName) const
{
    if (!ensureKnownAxis(axis, actionName)) {
        return false;
    }
    if (!ensureConnected(actionName)) {
        return false;
    }
    if (!currentStateKnownForAxis(axis)) {
        const QString message = tr("%1 failed, axis %2 state is unknown.")
            .arg(actionName, axisName(axis));
        emit const_cast<MotionController *>(this)->stateNotReady(message);
        emit const_cast<MotionController *>(this)->s_stateNotReady();
        emit const_cast<MotionController *>(this)->errorMessage(message);
        emit const_cast<MotionController *>(this)->s_sendErrorMsg(0, message);
        return false;
    }
    if (!isReadyState(currentStateForAxis(axis))) {
        const QString message = tr("%1 failed, axis %2 is not ready, state=%3, error=%4.")
            .arg(actionName, axisName(axis))
            .arg(currentStateForAxis(axis))
            .arg(currentErrorForAxis(axis));
        emit const_cast<MotionController *>(this)->stateNotReady(message);
        emit const_cast<MotionController *>(this)->s_stateNotReady();
        emit const_cast<MotionController *>(this)->errorMessage(message);
        emit const_cast<MotionController *>(this)->s_sendErrorMsg(0, message);
        return false;
    }
    return true;
}

bool MotionController::ensureAxesReadyForLine() const
{
    if (!ensureConnected("moveMultiAxisLinear")) {
        return false;
    }
    return ensureAxisReady(AxisX, "moveMultiAxisLinear") &&
           ensureAxisReady(AxisY, "moveMultiAxisLinear");
}

bool MotionController::validatePositive(const QString &name, double value) const
{
    if (!qIsFinite(value) || value <= 0.0) {
        emit const_cast<MotionController *>(this)->errorMessage(
            tr("%1 must be greater than 0.").arg(name));
        return false;
    }
    return true;
}

QStringList MotionController::servoCommandsForAxis(int axis, int enabled) const
{
    QStringList commands;
    if (axis == AxisX) {
        // X may be a dual-drive or gantry axis, so keep the legacy auxiliary axis command.
        commands << commandSvon(CmdSvon, AxisX, enabled)
                 << commandSvon(CmdSvon, AxisX - 1, enabled);
    } else if (axis == AxisY) {
        // Y may be a dual-drive or gantry axis, so keep the legacy auxiliary axis command.
        commands << commandSvon(CmdSvon, AxisY, enabled)
                 << commandSvon(CmdSvon, AxisY - 1, enabled);
    } else if (axis == AxisZ) {
        commands << commandSvon(CmdSvon, AxisZ, enabled);
    }
    return commands;
}

void MotionController::parseFrame(const QString &frame)
{
    if (frame == "#errcmd errindex") {
        emit errorMessage(tr("Controller rejected command: %1").arg(frame));
        return;
    }

    const QStringList parts = frame.split(' ', Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        emit errorMessage(tr("Invalid frame: %1").arg(frame));
        return;
    }

    if (parts.first() == "#autoSnd") {
        parseAutoSnd(parts, frame);
        return;
    }

    parseCommandReply(parts, frame);
}

bool MotionController::parseAutoSnd(const QStringList &parts, const QString &frame)
{
    if (parts.size() < 2) {
        emit errorMessage(tr("Invalid autoSnd frame: %1").arg(frame));
        return false;
    }

    QList<double> values;
    QString error;
    if (!parsePayload(parts.last(), &values, &error)) {
        emit errorMessage(tr("Invalid autoSnd payload: %1, frame=%2").arg(error, frame));
        return false;
    }

    if (values.size() <= 15) {
        emit errorMessage(tr("Invalid autoSnd field count %1, frame=%2").arg(values.size()).arg(frame));
        return false;
    }

    m_position.z = values.at(1);
    m_position.y = values.at(7);
    m_position.x = values.at(13);
    m_position.valid = true;

    m_state.stateZ = qRound(values.at(2));
    m_state.stateY = qRound(values.at(8));
    m_state.stateX = qRound(values.at(14));
    m_state.errorZ = qRound(values.at(3));
    m_state.errorY = qRound(values.at(9));
    m_state.errorX = qRound(values.at(15));
    m_state.valid = true;

    emit positionUpdated(m_position);
    emit stateUpdated(m_state);
    return true;
}

bool MotionController::parseCommandReply(const QStringList &parts, const QString &frame)
{
    if (parts.size() < 2) {
        emit logMessage(tr("Unrecognized reply: %1").arg(frame));
        return false;
    }

    const QString payloadText = parts.last().trimmed();
    const QStringList rawItems = payloadText.split(',', Qt::KeepEmptyParts);
    if (rawItems.isEmpty()) {
        emit errorMessage(tr("Reply has empty payload: %1").arg(frame));
        return false;
    }

    bool commandIdOk = false;
    const int commandId = rawItems.first().trimmed().toInt(&commandIdOk);
    if (!commandIdOk) {
        emit errorMessage(tr("Invalid reply command id: %1").arg(frame));
        return false;
    }

    if (rawItems.size() >= 2 && rawItems.last().trimmed() == ">") {
        emit logMessage(tr("Reply accepted without local state update: %1").arg(frame));
        return true;
    }
    if (rawItems.size() >= 2 && rawItems.last().trimmed() == "?") {
        emit errorMessage(tr("Controller rejected command: %1").arg(frame));
        return false;
    }

    QList<double> values;
    QString error;
    if (!parsePayload(payloadText, &values, &error)) {
        emit errorMessage(tr("Invalid reply payload: %1, frame=%2").arg(error, frame));
        return false;
    }
    if (values.isEmpty()) {
        emit errorMessage(tr("Reply has empty payload: %1").arg(frame));
        return false;
    }

    int lastInt = 0;
    switch (commandId) {
    case CmdReadCurrentPosX:
        if (values.size() < 2) {
            emit errorMessage(tr("RCP X reply missing position: %1").arg(frame));
            return false;
        }
        updatePosition(AxisX, values.last());
        return true;
    case CmdReadCurrentPosY:
        if (values.size() < 2) {
            emit errorMessage(tr("RCP Y reply missing position: %1").arg(frame));
            return false;
        }
        updatePosition(AxisY, values.last());
        return true;
    case CmdReadCurrentPosZ:
        if (values.size() < 2) {
            emit errorMessage(tr("RCP Z reply missing position: %1").arg(frame));
            return false;
        }
        updatePosition(AxisZ, values.last());
        return true;
    case CmdStatusX:
        if (!parseLastInt(values, &lastInt)) {
            emit errorMessage(tr("Status X reply missing state: %1").arg(frame));
            return false;
        }
        updateState(AxisX, lastInt);
        return true;
    case CmdStatusY:
        if (!parseLastInt(values, &lastInt)) {
            emit errorMessage(tr("Status Y reply missing state: %1").arg(frame));
            return false;
        }
        updateState(AxisY, lastInt);
        return true;
    case CmdStatusZ:
        if (!parseLastInt(values, &lastInt)) {
            emit errorMessage(tr("Status Z reply missing state: %1").arg(frame));
            return false;
        }
        updateState(AxisZ, lastInt);
        return true;
    case CmdErrorStatusX:
        if (!parseLastInt(values, &lastInt)) {
            emit errorMessage(tr("Error X reply missing code: %1").arg(frame));
            return false;
        }
        updateError(AxisX, lastInt);
        return true;
    case CmdErrorStatusY:
        if (!parseLastInt(values, &lastInt)) {
            emit errorMessage(tr("Error Y reply missing code: %1").arg(frame));
            return false;
        }
        updateError(AxisY, lastInt);
        return true;
    case CmdErrorStatusZ:
        if (!parseLastInt(values, &lastInt)) {
            emit errorMessage(tr("Error Z reply missing code: %1").arg(frame));
            return false;
        }
        updateError(AxisZ, lastInt);
        return true;
    case CmdHomeStatusX:
        if (!parseLastInt(values, &lastInt)) {
            emit errorMessage(tr("Home X reply missing value: %1").arg(frame));
            return false;
        }
        updateHome(AxisX, lastInt != 0);
        return true;
    case CmdHomeStatusY:
        if (!parseLastInt(values, &lastInt)) {
            emit errorMessage(tr("Home Y reply missing value: %1").arg(frame));
            return false;
        }
        updateHome(AxisY, lastInt != 0);
        return true;
    case CmdHomeStatusZ:
        if (!parseLastInt(values, &lastInt)) {
            emit errorMessage(tr("Home Z reply missing value: %1").arg(frame));
            return false;
        }
        updateHome(AxisZ, lastInt != 0);
        return true;
    default:
        emit logMessage(tr("Reply accepted without local state update: %1").arg(frame));
        return true;
    }
}

bool MotionController::parsePayload(const QString &payload, QList<double> *values, QString *error) const
{
    values->clear();
    const QStringList items = payload.split(',', Qt::KeepEmptyParts);
    for (const QString &item : items) {
        bool ok = false;
        const double value = item.trimmed().toDouble(&ok);
        if (!ok || !qIsFinite(value)) {
            if (error) {
                *error = tr("bad value '%1'").arg(item);
            }
            return false;
        }
        values->append(value);
    }
    return true;
}

void MotionController::updatePosition(int axis, double value)
{
    if (axis == AxisX) {
        m_position.x = value;
    } else if (axis == AxisY) {
        m_position.y = value;
    } else if (axis == AxisZ) {
        m_position.z = value;
    }
    m_position.valid = true;
    emit positionUpdated(m_position);
    AxisPos pos;
    pos.pos_x = m_position.x;
    pos.pos_y = m_position.y;
    pos.pos_z = m_position.z;
    emit s_axis_pos(pos);
}

void MotionController::updateState(int axis, int value)
{
    if (axis == AxisX) {
        m_state.stateX = value;
    } else if (axis == AxisY) {
        m_state.stateY = value;
    } else if (axis == AxisZ) {
        m_state.stateZ = value;
    }
    m_state_x = m_state.stateX;
    m_state_y = m_state.stateY;
    m_state_z = m_state.stateZ;
    m_state.valid = true;
    emit stateUpdated(m_state);
    AxisState state;
    state.state_x = m_state.stateX;
    state.state_y = m_state.stateY;
    state.state_z = m_state.stateZ;
    state.error_x = m_state.errorX;
    state.error_y = m_state.errorY;
    state.error_z = m_state.errorZ;
    emit s_axis_state(state);
}

void MotionController::updateError(int axis, int value)
{
    if (axis == AxisX) {
        m_state.errorX = value;
    } else if (axis == AxisY) {
        m_state.errorY = value;
    } else if (axis == AxisZ) {
        m_state.errorZ = value;
    }
    m_state.valid = true;
    emit stateUpdated(m_state);
    AxisState state;
    state.state_x = m_state.stateX;
    state.state_y = m_state.stateY;
    state.state_z = m_state.stateZ;
    state.error_x = m_state.errorX;
    state.error_y = m_state.errorY;
    state.error_z = m_state.errorZ;
    emit s_axis_state(state);
}

void MotionController::updateHome(int axis, bool value)
{
    if (axis == AxisX) {
        m_homeX = value;
        m_homeValidX = true;
    } else if (axis == AxisY) {
        m_homeY = value;
        m_homeValidY = true;
    } else if (axis == AxisZ) {
        m_homeZ = value;
        m_homeValidZ = true;
    }

    if (m_homeValidX && m_homeValidY && m_homeValidZ) {
        emit homeStatusUpdated(m_homeX, m_homeY, m_homeZ);
    }
}

int MotionController::currentStateForAxis(int axis) const
{
    if (axis == AxisX) {
        return m_state.stateX;
    }
    if (axis == AxisY) {
        return m_state.stateY;
    }
    if (axis == AxisZ) {
        return m_state.stateZ;
    }
    return -1;
}

int MotionController::currentErrorForAxis(int axis) const
{
    if (axis == AxisX) {
        return m_state.errorX;
    }
    if (axis == AxisY) {
        return m_state.errorY;
    }
    if (axis == AxisZ) {
        return m_state.errorZ;
    }
    return -1;
}

bool MotionController::currentStateKnownForAxis(int axis) const
{
    return currentStateForAxis(axis) >= 0;
}

bool MotionController::parseLastInt(const QList<double> &values, int *value) const
{
    if (values.size() < 2 || value == nullptr) {
        return false;
    }
    *value = qRound(values.last());
    return true;
}
