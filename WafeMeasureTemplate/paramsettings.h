#ifndef PARAMSETTINGS_H
#define PARAMSETTINGS_H

#include <QObject>

class ParamSettings
{
public:
    // 硬件设置
    QString axisIp;
    QString axisPort;
    QString colorFocusIpTop;
    QString colorFocusIpBottom;
    QString widIp;

    // 轴系预设位置
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

    // 轴系速度
    QString velocityMeasure;
    QString velocityNormal;

    // 共聚焦
    QString scanRate;
    QString triggerRate;

    // 测量参数
    QString trimSize;
    QString measureFileDir;

    //校准参数
    QString remindInterval;
    QString z_default_123;
    QString z_default_4;
    QString standardThickness1;
    QString standardThickness2;
    QString standardThickness3;
    QString standardThickness4;

    //以下参数界面不显示
    //上次校准时间
    QString lastCalTimeStamp_500_1500;
    QString lastCalTimeStamp_1550;

    //校准数据
    QString standardTotalVal1;
    QString standardTotalVal2;
    QString standardTotalVal3;
    QString standardTotalVal4;
    //校准方向为X
    QString CalibrationDirectionOfX;
    //算法控制
    QString IsNewWarpAlg;
    QString IsNewBowAlg;
    //保存数据控制
    QString IsSaveZM;
    QString IsSave3D;
    //读码器控制
    QString IsUseWID;
    //每条线采集的点数
    QString LineSampleNum;
    //不同版本控制
    QString IsUseStandard1550Flag;
    //Notch口宽度设置
    QString ChordLength;
};

#endif // PARAMSETTINGS_H
