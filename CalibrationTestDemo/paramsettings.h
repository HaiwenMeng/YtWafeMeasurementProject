#ifndef PARAMSETTINGS_H
#define PARAMSETTINGS_H

#include <QString>

class ParamSettings
{
public:
    QString axisIp;
    QString axisPort;
    QString colorFocusIpTop;
    QString colorFocusIpBottom;
    QString widIp;

    QString posLoadX;
    QString posLoadY;
    QString posWaitX_12;
    QString posWaitY_12;
    QString posWaitX_8;
    QString posWaitY_8;
    QString posWaitX_6;
    QString posWaitY_6;
    QString posStandard1X;
    QString posStandard1Y;
    QString posStandard2X;
    QString posStandard2Y;
    QString posStandard3X;
    QString posStandard3Y;
    QString posStandard4X;
    QString posStandard4Y;

    QString velocityMeasure;
    QString velocityNormal;

    QString scanRate;
    QString triggerRate;

    QString trimSize;
    QString measureFileDir;

    QString remindInterval;
    QString z_default_123;
    QString z_default_4;
    QString standardThickness1;
    QString standardThickness2;
    QString standardThickness3;
    QString standardThickness4;

    QString lastCalTimeStamp_500_1500;
    QString lastCalTimeStamp_1550;

    QString standardTotalVal1;
    QString standardTotalVal2;
    QString standardTotalVal3;
    QString standardTotalVal4;
    QString CalibrationDirectionOfX;
    QString IsNewWarpAlg;
    QString IsNewBowAlg;
    QString IsSaveZM;
    QString IsSave3D;
    QString IsUseWID;
    QString LineSampleNum;
    QString IsUseStandard1550Flag;
    QString ChordLength;
};

#endif // PARAMSETTINGS_H
