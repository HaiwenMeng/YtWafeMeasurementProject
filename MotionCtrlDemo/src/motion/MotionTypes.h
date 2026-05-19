#ifndef MOTIONTYPES_H
#define MOTIONTYPES_H

#include <QString>
#include <QtGlobal>
#include <QMetaType>

namespace Motion
{

enum AxisId
{
    AxisZ = 0,
    AxisY = 2,
    AxisX = 4
};

enum CommandId
{
    CmdSvon = 6001,
    CmdHome,
    CmdSmcHome,
    CmdAbs,
    CmdRel,
    CmdVel,
    CmdLineXy,
    CmdHalt,
    CmdHaltLineXy,
    CmdClear,
    CmdSetSpeed,
    CmdSetAcceleration,
    CmdSetDeceleration,
    CmdOutp,
    CmdGetInp,
    CmdHomeStatusX,
    CmdHomeStatusY,
    CmdHomeStatusZ,
    CmdGetOutp,
    CmdReadCurrentPosX,
    CmdReadCurrentPosY,
    CmdReadCurrentPosZ,
    CmdReadCommandPos,
    CmdReadSpeed,
    CmdReadAcceleration,
    CmdReadDeceleration,
    CmdStatusX,
    CmdStatusY,
    CmdStatusZ,
    CmdErrorStatusX,
    CmdErrorStatusY,
    CmdErrorStatusZ,
    CmdSetPcom1Param,
    CmdCtrlPcom1,
    CmdCtrlAutoSndStart,
    CmdCtrlAutoSndStop
};

struct AxisPosition
{
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    bool valid = false;
};

struct AxisStateSnapshot
{
    int stateX = -1;
    int stateY = -1;
    int stateZ = -1;
    int errorX = -1;
    int errorY = -1;
    int errorZ = -1;
    bool valid = false;
};

struct TriggerParam
{
    int axis = AxisX;
    double startPos = 0.0;
    double endPos = 0.0;
    double interval = 1.0;
    int direction = 1;
    int pulseWidth = 500;
};

struct MotionStatus
{
    AxisPosition position;
    AxisStateSnapshot state;
    bool homeX = false;
    bool homeY = false;
    bool homeZ = false;
    bool homeValidX = false;
    bool homeValidY = false;
    bool homeValidZ = false;
};

inline bool isKnownAxis(int axis)
{
    return axis == AxisX || axis == AxisY || axis == AxisZ;
}

inline QString axisName(int axis)
{
    switch (axis) {
    case AxisX:
        return "X";
    case AxisY:
        return "Y";
    case AxisZ:
        return "Z";
    default:
        return QString("Axis%1").arg(axis);
    }
}

inline int homeStatusCommandId(int axis)
{
    switch (axis) {
    case AxisX:
        return CmdHomeStatusX;
    case AxisY:
        return CmdHomeStatusY;
    case AxisZ:
        return CmdHomeStatusZ;
    default:
        return -1;
    }
}

inline int readPosCommandId(int axis)
{
    switch (axis) {
    case AxisX:
        return CmdReadCurrentPosX;
    case AxisY:
        return CmdReadCurrentPosY;
    case AxisZ:
        return CmdReadCurrentPosZ;
    default:
        return -1;
    }
}

inline int statusCommandId(int axis)
{
    switch (axis) {
    case AxisX:
        return CmdStatusX;
    case AxisY:
        return CmdStatusY;
    case AxisZ:
        return CmdStatusZ;
    default:
        return -1;
    }
}

inline int errorStatusCommandId(int axis)
{
    switch (axis) {
    case AxisX:
        return CmdErrorStatusX;
    case AxisY:
        return CmdErrorStatusY;
    case AxisZ:
        return CmdErrorStatusZ;
    default:
        return -1;
    }
}

inline QString commandSvon(int commandId, int axis, int enabled)
{
    return QString("#svon %1,%2,%3").arg(commandId).arg(axis).arg(enabled);
}

inline QString commandHome(int commandId, int axis)
{
    return QString("#home %1,%2").arg(commandId).arg(axis);
}

inline QString commandAbs(int commandId, int axis, double position)
{
    return QString("#abs %1,%2,%3").arg(commandId).arg(axis).arg(position, 0, 'g', 15);
}

inline QString commandRel(int commandId, int axis, double distance)
{
    return QString("#rel %1,%2,%3").arg(commandId).arg(axis).arg(distance, 0, 'g', 15);
}

inline QString commandVel(int commandId, int axis, int direction)
{
    return QString("#vel %1,%2,%3").arg(commandId).arg(axis).arg(direction);
}

inline QString commandLineXy(int commandId, double x, double y, double velocity, double acceleration, double deceleration)
{
    return QString("#linexy %1,%2,%3,%4,%5,%6")
        .arg(commandId)
        .arg(x, 0, 'g', 15)
        .arg(y, 0, 'g', 15)
        .arg(velocity, 0, 'g', 15)
        .arg(acceleration, 0, 'g', 15)
        .arg(deceleration, 0, 'g', 15);
}

inline QString commandHalt(int commandId, int axis)
{
    return QString("#halt %1,%2").arg(commandId).arg(axis);
}

inline QString commandHaltLineXy(int commandId)
{
    return QString("#haltlinexy %1").arg(commandId);
}

inline QString commandClear(int commandId, int axis)
{
    return QString("#clr %1,%2").arg(commandId).arg(axis);
}

inline QString commandSetSpeed(int commandId, int axis, double value)
{
    return QString("#ssp %1,%2,%3").arg(commandId).arg(axis).arg(value, 0, 'g', 15);
}

inline QString commandSetAcceleration(int commandId, int axis, double value)
{
    return QString("#sad %1,%2,%3").arg(commandId).arg(axis).arg(value, 0, 'g', 15);
}

inline QString commandSetDeceleration(int commandId, int axis, double value)
{
    return QString("#sdl %1,%2,%3").arg(commandId).arg(axis).arg(value, 0, 'g', 15);
}

inline QString commandReadCurrentPos(int commandId, int axis)
{
    return QString("#rcp %1,%2").arg(commandId).arg(axis);
}

inline QString commandStatus(int commandId, int axis)
{
    return QString("#status %1,%2").arg(commandId).arg(axis);
}

inline QString commandErrorStatus(int commandId, int axis)
{
    return QString("#errstatus %1,%2").arg(commandId).arg(axis);
}

inline QString commandHomeStatus(int commandId, int axis)
{
    return QString("#homestatus %1,%2").arg(commandId).arg(axis);
}

inline QString commandSetPcom1Param(const TriggerParam &param)
{
    return QString("#setpcom1param %1,%2,%3,%4,%5,%6,%7")
        .arg(CmdSetPcom1Param)
        .arg(param.axis)
        .arg(param.direction)
        .arg(param.pulseWidth)
        .arg(param.interval, 0, 'g', 15)
        .arg(param.endPos, 0, 'g', 15)
        .arg(param.startPos, 0, 'g', 15);
}

inline QString commandCtrlPcom1(int axis, int state)
{
    return QString("#ctrlpcom1 %1,%2,%3").arg(CmdCtrlPcom1).arg(axis).arg(state);
}

inline QString commandCtrlAutoSnd(int commandId, int interval)
{
    return QString("#ctrlAutoSnd %1,%2").arg(commandId).arg(interval);
}

} // namespace Motion

Q_DECLARE_METATYPE(Motion::AxisPosition)
Q_DECLARE_METATYPE(Motion::AxisStateSnapshot)
Q_DECLARE_METATYPE(Motion::MotionStatus)

#endif // MOTIONTYPES_H
