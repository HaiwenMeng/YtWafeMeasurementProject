#include "measure_control.h"
#include <QThread>
#include <QDebug>
#include <QLineF>
#include <QtMath>
#include <QtConcurrent/QtConcurrent>
#include <qcustomplot.h>
#include <opencv2/core.hpp>
#include "WaferAlgorithm.h"

#include <limits>

namespace
{
double invalidAlgorithmValue()
{
    return std::numeric_limits<double>::quiet_NaN();
}

Wafer::AlgorithmOptions legacyAlgorithmOptions(int lineCount, int productSize, int lineSampleNum,
                                               double radius, double centerX, double centerY,
                                               double calibrationTotal, double nominalThickness,
                                               int windowSize)
{
    Wafer::AlgorithmOptions options;
    options.lineCount = lineCount;
    options.productSize = productSize;
    options.lineSampleNum = lineSampleNum;
    options.radius = radius;
    options.centerX = centerX;
    options.centerY = centerY;
    options.calibrationTotal = calibrationTotal;
    options.nominalThickness = nominalThickness;
    options.windowSize = windowSize;
    options.useCsvZGravity = true;
    options.allowDebugZeroZGravity = false;
    return options;
}

void appendLegacyPoint(Wafer::LineData *line, const DataPoint &src, double zGravity)
{
    Wafer::DataPoint point;
    point.x = src.x;
    point.y = src.y;
    point.topDistance = src.a;
    point.bottomDistance = src.b;
    point.thickness = src.t;
    point.zm = src.zm;
    point.xEncoder = src.x_en;
    point.yEncoder = src.y_en;
    point.lineIndex = line->lineIndex;
    point.hasTopDistance = true;
    point.hasBottomDistance = true;
    point.hasThickness = qIsFinite(point.thickness);
    point.hasZGravity = qIsFinite(zGravity);
    point.zGravity = zGravity;
    line->points.append(point);
}

Wafer::WaferDataset datasetFromLegacyLines(const std::vector<std::vector<DataPoint>> &lines,
                                           const std::vector<std::vector<double>> &zGravityByLine,
                                           const std::vector<DataPoint> &fallbackPoints,
                                           const std::vector<double> &fallbackZGravity,
                                           int lineCount, int productSize, double centerX,
                                           double centerY, double calibrationTotal)
{
    Wafer::WaferDataset dataset;
    dataset.lineCount = lineCount;
    dataset.productSize = productSize;
    dataset.centerX = centerX;
    dataset.centerY = centerY;
    dataset.calibrationTotal = calibrationTotal;
    dataset.nominalThickness = calibrationTotal;
    dataset.containsZGravity = true;
    dataset.containsCenterPoint = true;

    if (!lines.empty()) {
        for (int i = 0; i < static_cast<int>(lines.size()); ++i) {
            Wafer::LineData line;
            line.lineIndex = i;
            const std::vector<DataPoint> &legacyLine = lines.at(i);
            const bool hasLineGravity = i < static_cast<int>(zGravityByLine.size());
            for (int j = 0; j < static_cast<int>(legacyLine.size()); ++j) {
                const double zg = hasLineGravity && j < static_cast<int>(zGravityByLine.at(i).size())
                    ? zGravityByLine.at(i).at(j)
                    : (j < static_cast<int>(fallbackZGravity.size()) ? fallbackZGravity.at(j) : invalidAlgorithmValue());
                appendLegacyPoint(&line, legacyLine.at(j), zg);
            }
            dataset.lines.append(line);
        }
    } else {
        Wafer::LineData line;
        line.lineIndex = 0;
        for (int i = 0; i < static_cast<int>(fallbackPoints.size()); ++i) {
            const double zg = i < static_cast<int>(fallbackZGravity.size())
                ? fallbackZGravity.at(i)
                : invalidAlgorithmValue();
            appendLegacyPoint(&line, fallbackPoints.at(i), zg);
        }
        dataset.lines.append(line);
    }

    for (const Wafer::LineData &line : dataset.lines) {
        for (const Wafer::DataPoint &point : line.points) {
            if (qFuzzyCompare(point.x + 1.0, centerX + 1.0) &&
                qFuzzyCompare(point.y + 1.0, centerY + 1.0)) {
                dataset.centerPoint.valid = true;
                dataset.centerPoint.point = point;
                return dataset;
            }
        }
    }
    if (!dataset.lines.isEmpty() && !dataset.lines.first().points.isEmpty()) {
        dataset.centerPoint.valid = true;
        dataset.centerPoint.point = dataset.lines.first().points.last();
    }
    return dataset;
}

Wafer::WaferDataset datasetFromZmLine(const std::vector<double> &x,
                                      const std::vector<double> &y,
                                      const std::vector<double> &zm,
                                      const std::vector<double> &thickness)
{
    Wafer::WaferDataset dataset;
    dataset.lineCount = 1;
    dataset.containsZGravity = true;
    Wafer::LineData line;
    line.lineIndex = 0;
    const int count = qMin(qMin(static_cast<int>(x.size()), static_cast<int>(y.size())), static_cast<int>(zm.size()));
    for (int i = 0; i < count; ++i) {
        Wafer::DataPoint point;
        point.x = x.at(i);
        point.y = y.at(i);
        point.zm = zm.at(i);
        point.thickness = i < static_cast<int>(thickness.size()) ? thickness.at(i) : 0.0;
        point.hasZm = true;
        point.hasThickness = i < static_cast<int>(thickness.size()) && qIsFinite(point.thickness);
        point.lineIndex = 0;
        line.points.append(point);
    }
    dataset.lines.append(line);
    return dataset;
}
}

MeasureControl::MeasureControl(ParamSettings &paramSettings, QObject *parent)
    : QObject{parent}, m_paramSettings(paramSettings)
{
        QtConcurrent::run([=](){
        ReadAllZGravityFile();
    });

      HandelDll=nullptr;

}

MeasureControl::~MeasureControl()
{
    if(HandelDll)
    FreeLibrary(HandelDll);
}

void MeasureControl::ReadAllZGravityFile()
{
    qDebug() << u8"开始读取文件时间：" << QDateTime::currentDateTime();

    m_mapVecListZGravity.clear();
    // m_mapVecListXIndex.clear();

    QDir appDir = QCoreApplication::applicationDirPath();
    QString ZGravityFileFilePath = appDir.filePath("ZGravityFile");
    QDir dirZGravity(ZGravityFileFilePath);
    if (!dirZGravity.exists())
    {
        emit s_writeLog(u8"ZGravityFile目录不存在");
        emit s_sendErrorMsg(0, u8"ZGravityFile目录不存在");
        return;
    }
    QFileInfoList lineType_list = dirZGravity.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    foreach (const QFileInfo &lineType, lineType_list)
    {
        if (lineType.isDir())
        {
            QString str_lineType = lineType.fileName(); //线类型
            int lineCount = 0;
            if(str_lineType == "type8lines"){
                lineCount = 8;
            }else if(str_lineType == "type6lines"){
                lineCount = 6;
            }

            QDir dirLineType(lineType.absoluteFilePath());
            QFileInfoList size_list = dirLineType.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
            foreach (const QFileInfo &sizeInfo, size_list)
            {
                if (sizeInfo.isDir())
                {
                    int waferSize = sizeInfo.fileName().toInt(); //晶圆尺寸

                    QDir dirSize(sizeInfo.absoluteFilePath());
                    QFileInfoList thickness_list = dirSize.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
                    foreach (const QFileInfo &thicknessInfo, thickness_list)
                    {
                        if (thicknessInfo.isDir())
                        {
                            int waferThickness = thicknessInfo.fileName().toInt(); //晶圆厚度

                            WaferType waferType{str_lineType, waferSize, waferThickness};

                            // std::vector<std::vector<int>> vecListXIndex;
                            std::vector<QMap<int, double>> vecListZGravity;

                            for (int pathIndex = 0; pathIndex < lineCount; ++pathIndex)
                            {
                                QString zg_filePath = thicknessInfo.absoluteFilePath() + QString("/ZGravity/z_gravity_%1.csv").arg(pathIndex);
                                QString zg_binFilePath = thicknessInfo.absoluteFilePath() + QString("/ZGravity/z_gravity_%1.zg").arg(pathIndex);
                                // QString xi_filePath = thicknessInfo.absoluteFilePath() + QString("/XIndex/line_%1.csv").arg(pathIndex);
                                QFileInfo csvFile(zg_filePath);
                                QFileInfo binFile(zg_binFilePath);
                                if (csvFile.exists()&&!binFile.exists())
                                {
                                    convertCsvToBinary(zg_filePath, zg_binFilePath);
                                }
                                
                                QMap<int, double> vecZGravity = readBinaryCsv(zg_binFilePath);

                                if(vecZGravity.isEmpty()){
                                    emit s_writeLog(u8"ZGravity文件读取失败");
                                   /* emit s_sendErrorMsg(0, u8"ZGravity文件读取失败");*/
                                    continue;
                                }
                                vecListZGravity.push_back(vecZGravity);

                            }
                            if (vecListZGravity.size()==lineCount)             //必须初始化后线数据成功后才能作为重力补偿值配方
                            {
                                m_mapVecListZGravity[waferType] = vecListZGravity;
                            }
                           
                            // m_mapVecListXIndex[waferType] = vecListXIndex;
                        } else {
                            continue;
                        }
                    }
                }
                else {
                    continue;
                }
            }
        }else{
            continue;
        }
    }

    qDebug() << u8"结束读取文件时间：" << QDateTime::currentDateTime();
}

void MeasureControl::SetCenterPoints(double centerX, double centerY)
{
    CircleCenter_X = centerX;
    CircleCenter_Y = centerY;
}

void MeasureControl::cleanup()
{
    QMetaObject::invokeMethod(m_pColorFocusControl_top.data(), [&]()
        {
            if (m_pColorFocusControl_top) {
                m_pColorFocusControl_top->DisconnectDevice();
            }
        }, Qt::QueuedConnection);

    QMetaObject::invokeMethod(m_pColorFocusControl_bottom.data(), [&]()
        {
            if (m_pColorFocusControl_bottom) {
                m_pColorFocusControl_bottom->DisconnectDevice();
            }
        }, Qt::QueuedConnection);

    // 等待连接完全关闭（假设 disconnectTCP() 是异步的）
    QThread::sleep(1);  // 可替换为事件循环等待，确保 TCP 断开
    qDebug() << "ColorFocusControl TCP connection closed.";
}


bool MeasureControl::fuzzyEqual(double a, double b, double epsilon)
{
    if (std::isnan(a) || std::isnan(b)) return false;
    if (a == b) return true; // 快速路径
    if (std::isinf(a) || std::isinf(b)) return a == b;

    const double absDiff = std::abs(a - b);
    const double maxVal = std::max({1.0, std::abs(a), std::abs(b)});
    return absDiff <= epsilon * maxVal;
}

void MeasureControl::waitInPos(QPointF dstPosition)
{
    double currentX, currentY, currentZ;
    int stateX, stateY, stateZ;
  
    m_pMotionController->getAxesCurrentPos(currentX, currentY, currentZ);
   
    while (!(fuzzyEqual(dstPosition.x(), currentX) && fuzzyEqual(dstPosition.y(), currentY))) {
        if(m_bEStopFlag){
            return;
        }
        QThread::msleep(50);
        m_pMotionController->getAxesCurrentPos(currentX, currentY, currentZ);
    }
   
    m_pMotionController->getAxesCurrentState(stateX, stateY, stateZ);
    while (!(3 == stateX && 3 == stateY)) {
        if(m_bEStopFlag){
            return;
        }
        QThread::msleep(50);
        m_pMotionController->getAxesCurrentState(stateX, stateY, stateZ);
    }
   
}

bool MeasureControl::SetMeasurePara(int measurePath, int productThickness, int productSize)
{
    // m_currentPathTypeXIndexMap.clear();
    m_currentPathTypeZgravityMap.clear();
    
    m_measurePath = measurePath;
    m_productThickness = productThickness;
    m_productSize = productSize;

    QString strTypeLines = (measurePath == 6) ? "type6lines" : "type8lines";
    WaferType currentWaferType{strTypeLines, productSize, productThickness};

    if(/*!m_mapVecListXIndex.contains(currentWaferType) || */!m_mapVecListZGravity.contains(currentWaferType)){
        return false;
    }

    // std::vector<std::vector<int>>& vecListXIndex = m_mapVecListXIndex[currentWaferType];
    std::vector<QMap<int, double>>& vecListZGravity = m_mapVecListZGravity[currentWaferType];

    switch (measurePath) {
    case 8:
        // m_currentPathTypeXIndexMap[0] = vecListXIndex.at(0);
        // m_currentPathTypeXIndexMap[1] = vecListXIndex.at(1);
        // m_currentPathTypeXIndexMap[2] = vecListXIndex.at(2);
        // m_currentPathTypeXIndexMap[3] = vecListXIndex.at(3);
        // m_currentPathTypeXIndexMap[4] = vecListXIndex.at(4);
        // m_currentPathTypeXIndexMap[5] = vecListXIndex.at(5);
        // m_currentPathTypeXIndexMap[6] = vecListXIndex.at(6);
        // m_currentPathTypeXIndexMap[7] = vecListXIndex.at(7);
  
        m_currentPathTypeZgravityMap[0] = vecListZGravity.at(0);
        m_currentPathTypeZgravityMap[1] = vecListZGravity.at(1);
        m_currentPathTypeZgravityMap[2] = vecListZGravity.at(2);
        m_currentPathTypeZgravityMap[3] = vecListZGravity.at(3);
        m_currentPathTypeZgravityMap[4] = vecListZGravity.at(4);
        m_currentPathTypeZgravityMap[5] = vecListZGravity.at(5);
        m_currentPathTypeZgravityMap[6] = vecListZGravity.at(6);
        m_currentPathTypeZgravityMap[7] = vecListZGravity.at(7);
        break;
    case 6:
        // m_currentPathTypeXIndexMap[0] = vecListXIndex.at(0);
        // m_currentPathTypeXIndexMap[1] = vecListXIndex.at(1);
        // m_currentPathTypeXIndexMap[2] = vecListXIndex.at(2);
        // m_currentPathTypeXIndexMap[3] = vecListXIndex.at(3);
        // m_currentPathTypeXIndexMap[4] = vecListXIndex.at(4);
        // m_currentPathTypeXIndexMap[5] = vecListXIndex.at(5);

        m_currentPathTypeZgravityMap[0] = vecListZGravity.at(0);
        m_currentPathTypeZgravityMap[1] = vecListZGravity.at(1);
        m_currentPathTypeZgravityMap[2] = vecListZGravity.at(2);
        m_currentPathTypeZgravityMap[3] = vecListZGravity.at(3);
        m_currentPathTypeZgravityMap[4] = vecListZGravity.at(4);
        m_currentPathTypeZgravityMap[5] = vecListZGravity.at(5);
        break;
    case 4:
        // m_currentPathTypeXIndexMap[0] = vecListXIndex.at(0);
        // m_currentPathTypeXIndexMap[1] = vecListXIndex.at(2);
        // m_currentPathTypeXIndexMap[2] = vecListXIndex.at(4);
        // m_currentPathTypeXIndexMap[3] = vecListXIndex.at(6);

        m_currentPathTypeZgravityMap[0] = vecListZGravity.at(0);
        m_currentPathTypeZgravityMap[1] = vecListZGravity.at(2);
        m_currentPathTypeZgravityMap[2] = vecListZGravity.at(4);
        m_currentPathTypeZgravityMap[3] = vecListZGravity.at(6);
        break;
    case 2:
        // m_currentPathTypeXIndexMap[0] = vecListXIndex.at(0);
        // m_currentPathTypeXIndexMap[1] = vecListXIndex.at(4);

        m_currentPathTypeZgravityMap[0] = vecListZGravity.at(0);
        m_currentPathTypeZgravityMap[1] = vecListZGravity.at(4);
        break;
    case 1:
        // m_currentPathTypeXIndexMap[0] = vecListXIndex.at(0);
        m_currentPathTypeZgravityMap[0] = vecListZGravity.at(0);
        break;
    default:
        break;
    }

    return true;
}

double MeasureControl::calculateTriggerInterval(const QLineF& line, double intervalDistance, int triggerAxis)
{
    // 获取线段起点和终点
    QPointF p0 = line.p1();
    QPointF p1 = line.p2();
    double length = line.length();

    if(0 == triggerAxis)
    {
        double dx = qAbs(p1.x() - p0.x());
        double dx_trigger = intervalDistance * (dx / length);
        return dx_trigger;
    }
    else
    {
        double dy = qAbs(p1.y() - p0.y());
        double dy_trigger = intervalDistance * (dy / length);
        return dy_trigger;
    }
}

double MeasureControl::calculateTriggerStart(const QLineF& line, int triggerAxis)
{
    if(0 == triggerAxis){
        return CircleCenter_X - line.x1();
    }else if(1 == triggerAxis){
        return CircleCenter_Y - line.y2();
    }
    else{
        return CircleCenter_X - line.x1();
    }
}

double MeasureControl::calculateTriggerEnd(const QLineF& line, int triggerAxis)
{
    if(0 == triggerAxis){
        return CircleCenter_X - line.x2();
    }else if(1 == triggerAxis){
        return CircleCenter_Y - line.y1();
    }
    else{
        return CircleCenter_Y - line.x2();
    }
}


QVector<QPointF> MeasureControl::generateTriggerPoints(const QLineF& line, double intervalDistance)
{
    QVector<QPointF> points;

    // 获取线段起点和终点
    QPointF p0 = line.p1();
    QPointF p1 = line.p2();

    // 计算方向分量
    double dx = p1.x() - p0.x();
    double dy = p1.y() - p0.y();
    double length = line.length();

    if (qFuzzyIsNull(dx))
    {
        emit s_writeLog(u8"垂直线段无法基于X轴间隔触发");
        return points;
    }

    // 处理无效情况
    if (length <= 0 || intervalDistance <= 0)
    {
        return points; // 返回空列表
    }

    // 计算X轴触发间隔和参数化步长
    // double dx_trigger = intervalDistance * (dx / length);
    double t_step = intervalDistance / length;

    // 生成触发点
    double t = 0.0;
    while (t <= 1.0 + std::numeric_limits<double>::epsilon())
    {
        QPointF pt(
            p0.x() + t * dx,
            p0.y() + t * dy
            );
        points.append(pt);
        t += t_step;
    }

    // 确保终点包含在内（浮点精度补偿）
    if (!points.isEmpty() && points.last() != p1)
    {
        points.append(p1);
    }

    return points;
}

void MeasureControl::InitTimer()
{
    m_timer_1 = new QTimer(this);
    connect(m_timer_1, &QTimer::timeout, this, &MeasureControl::handleTimeout_1);
    m_timer_1->setSingleShot(true);

    m_timer_2 = new QTimer(this);
    connect(m_timer_2, &QTimer::timeout, this, &MeasureControl::handleTimeout_2);
    m_timer_2->setSingleShot(true);
}

void MeasureControl::InitConnectinon()
{
    QObject::connect(m_pSerialCommunicator.data(), &SerialCommunicator::resultReady, this,
                     [this](bool match) {
                         if(match)
                         {
                             if(ProcessType::ClearStack == m_processType)
                             {
                                 Sleep(100); //此延时为解决停止触发后从两个传感器缓存中拿到的数据个数不一致的现象
                                 m_pColorFocusControl_top->ClearDataStack();
                                 m_pColorFocusControl_bottom->ClearDataStack();

                                 StartTrigger();
                                 //emit s_writeLog(QString(u8"清空缓存完成"));
                             }
                             else if(ProcessType::ReadDistance == m_processType)
                             {
                                 Sleep(100);//此延时为解决停止触发后从两个传感器缓存中拿到的数据个数不一致的现象
                                 m_pColorFocusControl_top->CloseAcquisitionEvent(BUFFER_LENGTH);
                                 m_pColorFocusControl_bottom->CloseAcquisitionEvent(BUFFER_LENGTH);

                                 int topDataCount = m_pColorFocusControl_top->GetMeasurePointDistanceList().count();
                                 int bottomDataCount = m_pColorFocusControl_bottom->GetMeasurePointDistanceList().count();
                                 //emit s_writeLog(QString(u8"上共聚焦读到的测量点数为：%1").arg(topDataCount));
                                // emit s_writeLog(QString(u8"下共聚焦读到的测量点数为：%1").arg(bottomDataCount));
                                 if(topDataCount != bottomDataCount)
                                 {
                                     StopMeasure();
                                     StartTrigger();

                                     emit s_writeLog(QString(u8"上下共聚焦测量点数不同，请重新测量"));
                                     emit s_sendErrorMsg(0, QString(u8"上下共聚焦测量点数不同，请重新测量"));

                                     return;
                                 }
                                 m_listAllMeasurePointDistance_top.append(m_pColorFocusControl_top->GetMeasurePointDistanceList());
                                 m_listAllMeasurePointDistance_bottom.append(m_pColorFocusControl_bottom->GetMeasurePointDistanceList());

                                 StartTrigger();
                                 //emit s_writeLog(QString(u8"读取缓存完成"));
                             }
                         }

                     });
}

void MeasureControl::ResetTrimSize(QString trimSize)
{
 
    m_paramSettings.trimSize = trimSize;
}

int MeasureControl::closerToAxis(double angleDeg)
{
    // 将角度归一化到 [0, 360)
    angleDeg = fmod(fmod(angleDeg, 360.0) + 360.0, 360.0);

    // 计算与 x 轴方向最近的夹角（0° 或 180°）
    double dx = std::min(std::abs(angleDeg - 0), std::abs(angleDeg - 180));
    dx = std::min(dx, 360 - dx); // 处理跨越 0 的情况

    // 计算与 y 轴方向最近的夹角（90° 或 270°）
    double dy = std::min(std::abs(angleDeg - 90), std::abs(angleDeg - 270));
    dy = std::min(dy, 360 - dy);

    return dx <= dy ? 0 : 1;
}

//参数为弦长：传入为非负值
void MeasureControl::GeneratePathInfoList(double chordLength)
{
    m_pathInfoList_trigger.clear();
    m_pathInfoList_move.clear();

    double radius_outer = 152;
    double radius_inner = 150 - m_paramSettings.trimSize.toDouble();
    double start_angleDeg = 270;  // 起始角为负 y 轴方向
    if (!IsNewBowAlg)
        start_angleDeg = 258.75;
    double step_angleDeg = 0;

    // 设定外径、内径
    if (6 == m_productSize) {
        radius_outer = 77;
        radius_inner = 75 - m_paramSettings.trimSize.toDouble();
        Radius = 75.0;
    } else if (12 == m_productSize) {
        radius_outer = 152;
        radius_inner = 150 - m_paramSettings.trimSize.toDouble();
        Radius = 150;
    } else if (8 == m_productSize) {
        radius_outer = 102;
        radius_inner = 100 - m_paramSettings.trimSize.toDouble();
        Radius = 100.0;
    }

    // 步长角度（用于均分路径）
    switch (m_measurePath) {
    case 1: break;
    case 2: step_angleDeg = 90; break;
    case 4: step_angleDeg = 45; break;
    case 6: step_angleDeg = 30; break;
    case 8: step_angleDeg = 22.5; break;
    default: break;
    }
    bool isNeedLimit = true;
    // 根据 chordLength（弦长）计算弦所在 y 坐标（在负 y 方向）
    double chord_limit_y = 0.0;
    if (chordLength > 0 && chordLength < 2 * radius_inner) {
        chord_limit_y = sqrt(radius_inner * radius_inner - (chordLength / 2.0) * (chordLength / 2.0));
        // 取负值，因为在负 y 轴方向
        chord_limit_y = -chord_limit_y;
    } else {
        chord_limit_y = -radius_inner; // 相当于没裁切
        isNeedLimit = false;
    }

    for (int line_index = 0; line_index < m_measurePath; ++line_index)
    {
        double angleDeg = start_angleDeg - step_angleDeg * line_index;
        double rad = qDegreesToRadians(angleDeg);

        // ---- 内圆路径处理 ----
        double x_inner = radius_inner * qCos(rad);
        double y_inner = radius_inner * qSin(rad);
        QPointF start_point_inner = QPointF(x_inner, y_inner);

        // 奇数索引：对称翻转起点
        if (line_index % 2 != 0) {
            start_point_inner *= -1;
        }

        // 默认终点为对称点
        QPointF end_point_inner = -start_point_inner;

        // 根据 chord_limit_y 截断起点（负 y 轴方向的点）
        if (isNeedLimit && (start_point_inner.y() < chord_limit_y)) {
            // 根据直线斜率计算新的 x
            double ratio = start_point_inner.x() / start_point_inner.y(); // x / y
            double new_y = chord_limit_y;
            double new_x = ratio * new_y;
            start_point_inner = QPointF(new_x, new_y);
        }

        QLineF line_inner(start_point_inner, end_point_inner);
        m_pathInfoList_trigger.append(line_inner);


        // ---- 外圆路径处理 ----
        double x_outer = radius_outer * qCos(rad);
        double y_outer = radius_outer * qSin(rad);
        QPointF start_point_outer = QPointF(x_outer, y_outer);

        if (line_index % 2 != 0) {
            start_point_outer *= -1;
        }
        QPointF end_point_outer = -start_point_outer;

        // 同样处理起点
        if (isNeedLimit&&(start_point_outer.y() < chord_limit_y)) {
            double ratio = start_point_outer.x() / start_point_outer.y();
            double new_y = chord_limit_y - m_paramSettings.trimSize.toDouble();
            double new_x = ratio * new_y;
            start_point_outer = QPointF(new_x, new_y);
        }

        QLineF line_outer(start_point_outer, end_point_outer);
        m_pathInfoList_move.append(line_outer);
    }
}

double MeasureControl::getLineAngelDeg(int measurePath, int lineIndex)
{
    if (measurePath <= 0 || lineIndex < 0 || lineIndex >= measurePath * 2)
        return -1; // 错误输入，返回无效角度

    int totalLines = measurePath * 2;
    double angleStep = 360.0 / totalLines;
    double angle = 270.0 - angleStep * lineIndex;

    // 将角度归一化到 [0, 360)
    angle = fmod(fmod(angle, 360.0) + 360.0, 360.0);
    return angle;
}

int MeasureControl::getTriggerAxis(int measurePath, int lineIndex)
{
    double lineAngelDeg = getLineAngelDeg(measurePath, lineIndex);
    if (IsNewBowAlg)
        return closerToAxis(lineAngelDeg);
    else
    {
        return 0;
    }
 
}

#ifdef MOTION_TRIGGER
void MeasureControl::StartMeasure()
{
    //清空数据列表
    m_listAllMeasurePointDistance_top.clear();
    m_listAllMeasurePointDistance_bottom.clear();
    //生成所选路径list
    GeneratePathInfoList();
    emit s_writeLog(u8"生成所选路径list");

    std::vector<DataPoint> vctDataPoint;

    //切换触发模式为Burst
    m_pColorFocusControl_top->ChangeTriggerMode(TriggerMode::Burst);
    m_pColorFocusControl_bottom->ChangeTriggerMode(TriggerMode::Burst);

    for (int pathIndex = 0; pathIndex < m_pathInfoList_trigger.count(); ++pathIndex)
    {
        QLineF currentLine = m_pathInfoList_trigger.at(pathIndex);

        //移动到第pathIndex线的起点
        m_pMotionController->moveMultiAxisLinear(CircleCenter_X - currentLine.x1(), CircleCenter_Y + currentLine.y1()
                                                 , m_paramSettings.velocityNormal.toDouble()
                                                 , m_paramSettings.velocityNormal.toDouble() * 10
                                                 , m_paramSettings.velocityNormal.toDouble() * 10);
        emit s_writeLog(QString(u8"移动到第%1线的起点").arg(pathIndex + 1));
        //等待到位
        waitInPos(QPointF(CircleCenter_X - currentLine.x1(), CircleCenter_Y + currentLine.y1()));
        emit s_writeLog(QString(u8"开始测量第%1线").arg(pathIndex + 1));
        emit s_isMeasurePathFlagChanged(true);
        //设置X轴触发参数
        m_pMotionController->ControlTrigger(AxisID::X, 0);
        Sleep(500);
        double triggerInterval = calculateTriggerInterval(currentLine, m_samplingInterval);
        double triggerStart, triggerEnd;
        int direction;
        if(CircleCenter_X - currentLine.x1() < CircleCenter_X - currentLine.x2()){
            triggerStart = CircleCenter_X - currentLine.x1();
            triggerEnd = CircleCenter_X - currentLine.x2();
            direction = 1;
        }
        else{
            triggerStart = CircleCenter_X - currentLine.x2();
            triggerEnd = CircleCenter_X - currentLine.x1();
            direction = 0;
        }

        m_pMotionController->SetTriggerParam(AxisID::X, triggerStart, triggerEnd, qAbs(triggerInterval), direction);

        QtConcurrent::run([=](){
            m_pColorFocusControl_top->ClearDataStack();
            m_pColorFocusControl_bottom->ClearDataStack();
        });
        emit s_writeLog(QString(u8"清空缓存完成"));
        //清空共聚焦缓存
        Sleep(500); //此延时为解决停止触发后从两个传感器缓存中拿到的数据个数不一致的现象
        //X轴开始触发
        m_pMotionController->ControlTrigger(AxisID::X, 1);
        //移动到第pathIndex线的终点
        m_pMotionController->moveMultiAxisLinear(CircleCenter_X - currentLine.x2(), CircleCenter_Y + currentLine.y2()
                                                 , m_paramSettings.velocityMeasure.toDouble()
                                                 , m_paramSettings.velocityMeasure.toDouble() * 10
                                                 , m_paramSettings.velocityMeasure.toDouble() * 10);
        //等待到位
        waitInPos(QPointF(CircleCenter_X - currentLine.x2(), CircleCenter_Y + currentLine.y2()));
        //停止触发
        m_pMotionController->ControlTrigger(AxisID::X, 0);
        //读取共聚焦缓存
        Sleep(500);//此延时为解决停止触发后从两个传感器缓存中拿到的数据个数不一致的现象
        QtConcurrent::run([=](){
            m_pColorFocusControl_top->CloseAcquisitionEvent(BUFFER_LENGTH);
            m_pColorFocusControl_bottom->CloseAcquisitionEvent(BUFFER_LENGTH);
        });
        Sleep(100);
        int topDataCount = m_pColorFocusControl_top->GetMeasurePointDistanceList().count();
        int bottomDataCount = m_pColorFocusControl_bottom->GetMeasurePointDistanceList().count();
        emit s_writeLog(QString(u8"上共聚焦读到的测量点数为：%1").arg(topDataCount));
        emit s_writeLog(QString(u8"下共聚焦读到的测量点数为：%1").arg(bottomDataCount));
        // if(topDataCount != bottomDataCount)
        // {

        //     emit s_writeLog(QString(u8"上下共聚焦测量点数不同，请重新测量"));
        //     emit s_sendErrorMsg(0, QString(u8"上下共聚焦测量点数不同，请重新测量"));

        //     //切换触发模式为Burst
        //     m_pColorFocusControl_top->ChangeTriggerMode(TriggerMode::Continue);
        //     m_pColorFocusControl_bottom->ChangeTriggerMode(TriggerMode::Continue);
        //     emit s_writeLog(u8"测量失败");
        //     //emit s_measureOver(NULL);

        //     return;
        // }
        // m_listAllMeasurePointDistance_top.append(m_pColorFocusControl_top->GetMeasurePointDistanceList());
        // m_listAllMeasurePointDistance_bottom.append(m_pColorFocusControl_bottom->GetMeasurePointDistanceList());

        emit s_writeLog(QString(u8"读取缓存完成"));

        emit s_writeLog(QString(u8"第%1线测量完成").arg(pathIndex + 1));
        // emit s_isMeasurePathFlagChanged(false);


        //整理数据
        // double middlePos = 0.0;

        // for (int pathIndex = 0; pathIndex < m_pathInfoList2.count(); ++pathIndex)
        // {
        //     QList<_DISTANCE_VALUE> line_list_top = m_listAllMeasurePointDistance_top.at(pathIndex);
        //     QList<_DISTANCE_VALUE> line_list_bottom = m_listAllMeasurePointDistance_bottom.at(pathIndex);
        //     QLineF currentLine = m_pathInfoList2.at(pathIndex);
        //     QVector<QPointF> vecPoint = generateTriggerPoints(currentLine, m_samplingInterval);

        //     for (int pointIndex = 0; pointIndex < line_list_top.count(); ++pointIndex)
        //     {
        //         double x = vecPoint.at(pointIndex).x();
        //         double y = vecPoint.at(pointIndex).y();
        //         double topDistance = line_list_top.at(pointIndex).distance;
        //         double bottomDistance = line_list_bottom.at(pointIndex).distance;
        //         double realThickness = m_currentCalibrationInfo.total - topDistance - bottomDistance;
        //         DataPoint dataPoint{x, y, topDistance, bottomDistance, realThickness, middlePos};

        //         vctDataPoint.push_back(dataPoint);
        //     }
        // }
    }

    //切换触发模式为Continue
    m_pColorFocusControl_top->ChangeTriggerMode(TriggerMode::Continue);
    m_pColorFocusControl_bottom->ChangeTriggerMode(TriggerMode::Continue);

    // //去到圆心读取圆心为止上下共聚焦值并添加至测量数据vector中
    // m_pMotionController->moveMultiAxisLinear(CircleCenter_X, CircleCenter_Y
    //                                          , m_paramSettings.velocityMeasure.toDouble()
    //                                          , m_paramSettings.velocityMeasure.toDouble() * 10
    //                                          , m_paramSettings.velocityMeasure.toDouble() * 10);
    // //等待到位圆心
    // waitInPos(QPointF(CircleCenter_X, CircleCenter_Y));
    // double topDistance = m_pColorFocusControl_top->GetCurrentDistance();
    // double bottomDistance = m_pColorFocusControl_bottom->GetCurrentDistance();
    // double realThickness = m_currentCalibrationInfo.total - topDistance - bottomDistance;
    // DataPoint dataPoint{CircleCenter_X, CircleCenter_X, topDistance, bottomDistance, realThickness, 0.0};
    // vctDataPoint.push_back(dataPoint);

    // //调用算法
    // WarpResult warpResult = ManualWarpAlg(vctDataPoint, CircleCenter_X, CircleCenter_Y, m_currentCalibrationInfo.total, 0);

    emit s_writeLog(u8"测量完成");
    // emit s_measureOver(warpResult);

}

#endif
void MeasureControl::SetLineSampleParams(int lineSampleNum)
{
    LineSampleNum = lineSampleNum;
}
void MeasureControl::InitACQZGravity()
{
    NormalAllZGravityByLine.clear();
    OppsiteAllZGravityByLine.clear();
    XIntervals.clear();
}
#ifdef ENCODER_TRIGGER
void MeasureControl::StartMeasure(MeasureMode mode)
{
    ProgramMeasureMode = mode;
  
  
   
    m_bStopMeasureFlag = false;
    m_bEStopFlag = false;

    ///////////////////////////////////////////////
    ////    生成所选路径list
    ///////////////////////////////////////////////
    GeneratePathInfoList(ChordLength);
    emit s_writeLog(u8"生成所选路径list");

  

    // std::vector<std::vector<DataPoint>> vctAllDataPointByLine; //所有线的点vector(按线存)
    m_vecAllZGravity.clear();
    m_vctAllDataPointByLine.clear();
    m_vctAllZGravityByLine.clear();
    m_lineDistanceMatRunDir.clear();
    std::vector<DataPoint> vctAllDataPoint; //所有线的点vector
    //用于计算BOW值的数据
    std::vector<DataPoint> nor_data;
    std::vector<double> z_gravity;
  


   
    MeasureAllStep = m_pathInfoList_trigger.count();
    for (int pathIndex = 0; pathIndex < m_pathInfoList_trigger.count(); ++pathIndex)
    {
        NowStep = pathIndex;
        if(m_bStopMeasureFlag || m_bEStopFlag){
            return;
        }

        m_currentPathIndex = pathIndex;
  
        QLineF currentLine_trigger = m_pathInfoList_trigger.at(pathIndex);
        QLineF currentLine_move = m_pathInfoList_move.at(pathIndex);

        ///////////////////////////////////////////////
        ////    移动到第pathIndex线的起点
        ///////////////////////////////////////////////
        QMetaObject::invokeMethod(m_pMotionController.data(), [=]() {
        m_pMotionController->moveMultiAxisLinear(CircleCenter_X - currentLine_move.x1(), CircleCenter_Y + currentLine_move.y1()
                            , m_paramSettings.velocityNormal.toDouble()
                            , m_paramSettings.velocityNormal.toDouble() * 10
                            , m_paramSettings.velocityNormal.toDouble() * 10);
        }, Qt::QueuedConnection);

        //emit s_writeLog(QString(u8"移动到第%1线的起点").arg(pathIndex + 1));

        //等待到位
        waitInPos(QPointF(CircleCenter_X - currentLine_move.x1(), CircleCenter_Y + currentLine_move.y1()));
        if(m_bStopMeasureFlag || m_bEStopFlag){
            return;
        }
  
        emit s_writeLog(QString(u8"开始测量第%1线").arg(pathIndex + 1));
        emit s_isMeasurePathFlagChanged(true);
       
        //设置触发轴的触发参数
        BYTE ecd_sel = getTriggerAxis(m_measurePath, pathIndex);
        double triggerInterval = calculateTriggerInterval(currentLine_trigger, m_samplingInterval, ecd_sel);
        double triggerStart = calculateTriggerStart(currentLine_trigger, ecd_sel);
        double triggerEnd = calculateTriggerEnd(currentLine_trigger, ecd_sel);
       
        INT32 head = ChangeToEncoderValue(triggerStart);
        INT32 tail = ChangeToEncoderValue(triggerEnd);
        INT32 interval = ChangeToEncoderValue(triggerInterval);
        
        XIntervals.push_back(interval);
        qDebug() <<"head = " << head <<", tail = " << tail << ", interval = " << interval;

        //共聚焦开始触发采集
        QMetaObject::invokeMethod(m_pColorFocusControl_top.data(), [&]()
            {
                m_pColorFocusControl_top->StopAcquisition_CCS();
                m_pColorFocusControl_top->SetEcdTrgParam(head, tail, interval, ecd_sel);
                m_pColorFocusControl_top->StartAcquisition_CCS(TriggerMode::Encoder);

            }, Qt::BlockingQueuedConnection);
       
        QMetaObject::invokeMethod(m_pColorFocusControl_bottom.data(), [&]()
            {
                m_pColorFocusControl_bottom->StopAcquisition_CCS();
                m_pColorFocusControl_bottom->SetEcdTrgParam(head, tail, interval, ecd_sel);
                m_pColorFocusControl_bottom->StartAcquisition_CCS(TriggerMode::Encoder);
            }, Qt::BlockingQueuedConnection);
       
        Sleep(100);

        ///////////////////////////////////////////////
        ////    移动到第pathIndex线的终点
        ///////////////////////////////////////////////
        QMetaObject::invokeMethod(m_pMotionController.data(), [=]() {
        m_pMotionController->moveMultiAxisLinear(CircleCenter_X - currentLine_move.x2(), CircleCenter_Y + currentLine_move.y2()
            , m_paramSettings.velocityMeasure.toDouble()
            , m_paramSettings.velocityMeasure.toDouble() * 10
            , m_paramSettings.velocityMeasure.toDouble() * 10);
        }, Qt::QueuedConnection);
       
        //等待到位
        waitInPos(QPointF(CircleCenter_X - currentLine_move.x2(), CircleCenter_Y + currentLine_move.y2()));
       
        if(m_bStopMeasureFlag || m_bEStopFlag){
            return;
        }

        //emit s_writeLog(QString(u8"到达第%1线的终点").arg(pathIndex + 1));

        // Sleep(100);

        //读取共聚焦缓存
        int topDataCount = 0;
        int bottomDataCount = 0;

        QMetaObject::invokeMethod(m_pColorFocusControl_top.data(), [&]()
            {
                m_pColorFocusControl_top->CloseAcquisitionEvent(BUFFER_LENGTH, ecd_sel);
                QMap<INT32, _DISTANCE_VALUE> &mapDistance_top = m_pColorFocusControl_top->GetMeasurePointDistanceMap();
                topDataCount = mapDistance_top.count();
            }, Qt::BlockingQueuedConnection);
     
        QMetaObject::invokeMethod(m_pColorFocusControl_bottom.data(), [&]()
            {
                m_pColorFocusControl_bottom->CloseAcquisitionEvent(BUFFER_LENGTH, ecd_sel);
                QMap<INT32, _DISTANCE_VALUE> &mapDistance_bottom = m_pColorFocusControl_bottom->GetMeasurePointDistanceMap();
                bottomDataCount = mapDistance_bottom.count();
            }, Qt::BlockingQueuedConnection);

        if (IsDebug) {
            emit s_writeLog(QString(u8"读取缓存完成"));
            emit s_writeLog(QString(u8"上共聚焦读到的测量点数为：%1").arg(topDataCount));
            emit s_writeLog(QString(u8"下共聚焦读到的测量点数为：%1").arg(bottomDataCount));
        }
            emit s_writeLog(QString(u8"第%1线测量完成").arg(pathIndex + 1));
      
        emit s_isMeasurePathFlagChanged(false);

        ///////////////////////////////////////////////
        ////    整理数据
        ///////////////////////////////////////////////
        if ((mode == ACQNormalGravity)||(mode == ACQOppsiteGravity)) {
            //未滤波的共聚焦原始数据，用于计算重力补偿值
       
            OrganizeDataForZGravity(m_vctAllDataPointByLine, vctAllDataPoint, interval, ecd_sel==0);
           
            continue;
        }
#ifdef ORGANIZE_STANDARD
        //一一对应比对上下数据（有缺陷）
        qDebug() << u8"开始整理数据" << QDateTime::currentDateTime();
        OrganizeData(m_vctAllDataPointByLine, vctAllDataPoint, interval);
        /*if((m_vctAllDataPointByLine.size()!=0)&&(m_vctAllDataPointByLine.back().size() < 5000))
        {
            QtConcurrent::run([=](){
                qDebug() << u8"上共聚焦原始编码器值";
                for (int encoder : m_pColorFocusControl_top->GetEncoders()) {
                    qDebug() << QString::number(encoder);
                }
                qDebug() << u8"下共聚焦原始编码器值";
                for (int encoder : m_pColorFocusControl_bottom->GetEncoders()) {
                    qDebug() << QString::number(encoder);
                }
                qDebug() << u8"上共聚焦与重力补偿交集";
                qDebug() << QString(u8"offset_top = %1").arg(m_offset_top);
                for (int encoder : m_intersectionVec_topAndZG) {
                    qDebug() << QString::number(encoder);
                }
                qDebug() << u8"下共聚焦与重力补偿交集";
                qDebug() << QString(u8"offset_bottom = %1").arg(m_offset_bottom);
                for (int encoder : m_intersectionVec_bottomAndZG) {
                    qDebug() << QString::number(encoder);
                }

                int offset = 0;
                QVector<int> intersectionVec = findIntersection(m_intersectionVec_topAndZG, m_intersectionVec_bottomAndZG, m_x_interval, offset);
                qDebug() << u8"分别取交集后，上下共聚焦交集";
                qDebug() << QString(u8"offset = %1").arg(offset);
                for (int encoder : intersectionVec) {
                    qDebug() << QString::number(encoder);
                }
            });

            return;
        }*/
        //用于计算BOW值的数据
        //OrganizeDataForCalBow(nor_data, z_gravity);
        qDebug() << u8"结束整理数据" << QDateTime::currentDateTime();
#endif

#ifdef ORGANIZE_SIMPLY
        //简易切割（会错位）
        OrganizeDataSimply(m_vctAllDataPointByLine, vctAllDataPoint);
#endif

#ifdef ORGANIZE_FORZGRAVITY
        //未滤波的共聚焦原始数据，用于计算重力补偿值
        OrganizeDataForZGravity(m_vctAllDataPointByLine, vctAllDataPoint, interval);
#endif

#ifdef MIRROR_CAL
        //平面镜标定用
        OrganizeDataMirrorCalibration(m_vctAllDataPointByLine, vctAllDataPoint);
#endif

#ifdef DEBUG_ALGORITHM
        //上下共聚焦数据分别存储（调试算法用）
        OrganizeDataTopAndBottom(m_vctAllDataPointByLine, vctBottomDataPointByLine);
#endif

    }

    //切换触发模式为Continue
    QMetaObject::invokeMethod(m_pColorFocusControl_top.data(), [&]()
        {
            m_pColorFocusControl_top->StopAcquisition_CCS();
            m_pColorFocusControl_top->StartAcquisition_CCS(TriggerMode::Continue);
        }, Qt::BlockingQueuedConnection);

    QMetaObject::invokeMethod(m_pColorFocusControl_bottom.data(), [&]()
        {
            m_pColorFocusControl_bottom->StopAcquisition_CCS();
            m_pColorFocusControl_bottom->StartAcquisition_CCS(TriggerMode::Continue);
        }, Qt::BlockingQueuedConnection);

    ///////////////////////////////////////////////////////////////
    ////  去到圆心位置读取圆心上下共聚焦值并添加至测量数据vector中
    //////////////////////////////////////////////////////////////

    QMetaObject::invokeMethod(m_pMotionController.data(), [=]() {
        m_pMotionController->moveMultiAxisLinear(CircleCenter_X, CircleCenter_Y
                                                 , m_paramSettings.velocityMeasure.toDouble()
                                                 , m_paramSettings.velocityMeasure.toDouble() * 10
                                                 , m_paramSettings.velocityMeasure.toDouble() * 10);
    }, Qt::QueuedConnection);

    //等待到位圆心
    waitInPos(QPointF(CircleCenter_X, CircleCenter_Y));
    if(m_bStopMeasureFlag || m_bEStopFlag){
        return;
    }

    double topDistance = 0.0;
    double bottomDistance = 0.0;

    m_dataBuffer_top.clear();
    m_dataBuffer_bottom.clear();
    for (int n = 0; n < 100; ++n) {
        topDistance += filterValue_top(m_pColorFocusControl_top->GetCurrentDistance());
        bottomDistance += filterValue_bottom(m_pColorFocusControl_bottom->GetCurrentDistance());
        QThread::msleep(10);
    }
    topDistance = topDistance / 100;
    bottomDistance = bottomDistance / 100;

    double realThickness = m_currentCalibrationInfo.total - topDistance - bottomDistance;
    int circleCenter_x_en = ChangeToEncoderValue(CircleCenter_X);
    int circleCenter_y_en = ChangeToEncoderValue(CircleCenter_Y);
    DataPoint dataPoint{CircleCenter_X, CircleCenter_Y, topDistance, bottomDistance, realThickness, 0.0, circleCenter_x_en, circleCenter_y_en};

    //将圆心值添加到总vct末尾
    vctAllDataPoint.push_back(dataPoint);

    //将圆心值添加到每条线末尾
    for (std::vector<DataPoint>& vctDataPoint : m_vctAllDataPointByLine) {
        vctDataPoint.push_back(dataPoint);
    }

    if ((mode == ACQNormalGravity) || (mode == ACQOppsiteGravity))
    {
        for (std::vector<DataPoint>& vctDataPoints : m_vctAllDataPointByLine) 
        {
          
      
            QMap<int,double> vecZGravity;
            int N = vctDataPoints.size();    
            for (int n = 0; n < N; n++)
            {
                DataPoint& vctDataPoint = vctDataPoints[n];
                double zGravity = (vctDataPoint.b - vctDataPoint.a) / 2.0;
                vecZGravity[vctDataPoint.x_en] = zGravity;
            }
          
            if (ProgramMeasureMode == ACQOppsiteGravity)
            {
                OppsiteAllZGravityByLine.push_back(vecZGravity);
            }
            if (ProgramMeasureMode == ACQNormalGravity) {
                NormalAllZGravityByLine.push_back(vecZGravity);
            }
           
        }
        if (ProgramMeasureMode == ACQNormalGravity) {
            CalAndSaveZGravity();
        }
        emit s_measureOver(0, 0, 0, 0, 0, 0);
        emit s_writeLog(u8"扫描完成");
       //WriteDistanceDataToFile(m_vctAllDataPointByLine); //保存文件
        return;
    }



    //将圆心值添加到用于计算bow值的vector数据末尾
    nor_data.push_back(dataPoint);

#ifdef DEBUG_ALGORITHM
    //将圆心值添加到每条线末尾（测试算法用）
    for (std::vector<DataPoint>& vctDataPoint : vctTopDataPointByLine) {
        vctDataPoint.push_back(dataPoint);
    }
    for (std::vector<DataPoint>& vctDataPoint : vctBottomDataPointByLine) {
        vctDataPoint.push_back(dataPoint);
    }
#endif

#ifdef ORGANIZE_STANDARD
    //写文件
    // qDebug() << u8"开始写文件" << QDateTime::currentDateTime();
    WriteDistanceDataToFile(m_vctAllDataPointByLine);
    // qDebug() << u8"结束写文件" << QDateTime::currentDateTime();
#endif

#ifdef ORGANIZE_FORZGRAVITY
    //写文件
    WriteDistanceDataToFile(m_vctAllDataPointByLine);
#endif

#ifdef MIRROR_CAL
    //写文件（标定用）
    WriteMirrorCalibrationDataToFile(m_vctAllDataPointByLine);
#endif
  
#ifdef DEBUG_ALGORITHM
    //写文件，上下共聚焦数据分别存（调试算法用）
    WriteTopAndBottomDistanceDataToFile(vctTopDataPointByLine, vctBottomDataPointByLine);
#endif

#ifdef ORGANIZE_STANDARD

    //圆心重力补偿值
    double circleCenterZGravity = m_currentPathTypeZgravityMap.value(0).value(circleCenter_x_en);
    m_vecAllZGravity.push_back(circleCenterZGravity);
    z_gravity.push_back(circleCenterZGravity);


    //将圆心值添加到每条线末尾
    for (std::vector<double>& vctZGravity : m_vctAllZGravityByLine) {
        vctZGravity.push_back(circleCenterZGravity);
    }

    if (m_vctAllZGravityByLine.size() != m_vctAllDataPointByLine.size()) {
        QString errorMsg = QString("ZGravity line count mismatch: measure=%1, gravity=%2")
                               .arg(static_cast<int>(m_vctAllDataPointByLine.size()))
                               .arg(static_cast<int>(m_vctAllZGravityByLine.size()));
        emit s_writeLog(errorMsg);
        emit s_sendErrorMsg(0, errorMsg);
        return;
    }
    for (int i = 0; i < static_cast<int>(m_vctAllDataPointByLine.size()); ++i) {
        if (m_vctAllZGravityByLine[i].size() != m_vctAllDataPointByLine[i].size()) {
            QString errorMsg = QString("ZGravity point count mismatch: line=%1, measure=%2, gravity=%3")
                                   .arg(i + 1)
                                   .arg(static_cast<int>(m_vctAllDataPointByLine[i].size()))
                                   .arg(static_cast<int>(m_vctAllZGravityByLine[i].size()));
            emit s_writeLog(errorMsg);
            emit s_sendErrorMsg(0, errorMsg);
            return;
        }
    }
    if (m_vecAllZGravity.size() != vctAllDataPoint.size()) {
        QString errorMsg = QString("Algorithm ZGravity count mismatch: measure=%1, gravity=%2")
                               .arg(static_cast<int>(vctAllDataPoint.size()))
                               .arg(static_cast<int>(m_vecAllZGravity.size()));
        emit s_writeLog(errorMsg);
        emit s_sendErrorMsg(0, errorMsg);
        return;
    }


    //调用算法
    qDebug() << u8"开始调用算法" << QDateTime::currentDateTime();
   


   
    WarpResult warpResult = ManualWarpAlg(vctAllDataPoint, m_vecAllZGravity, CircleCenter_X, CircleCenter_Y, m_currentCalibrationInfo.total);
  
    if (!IsNewBowAlg) {
        double bow = Calculate_Bow(nor_data, z_gravity, CircleCenter_X, CircleCenter_Y);
        warpResult.BOW = bow;
    }
    int lineNum = m_vctAllDataPointByLine.size();
    QVector<double> BOWs;
    QVector<double> WARPs;
    double maxBOW=0;
    std::vector<double> all_Xs;
    std::vector<double> all_Ys;
    std::vector<double> all_ZMs;
   
    for (int i = 0; i < lineNum; i++)
    {
        std::vector<DataPoint>& vctDataPoints = m_vctAllDataPointByLine[i];
        std::vector<double> Xs;
        std::vector<double> Ys;
        std::vector<double> ZMs;
        int N = vctDataPoints.size();
       
        for (int n = 0; n < N; n++)
        {
            DataPoint& vctDataPoint = vctDataPoints[n];
            double z1 = (vctDataPoint.b - vctDataPoint.a) / 2.0;
            double zm = z1 - m_vctAllZGravityByLine[i][n];
            Xs.push_back(vctDataPoints[n].x);
            Ys.push_back(vctDataPoints[n].y);
            ZMs.push_back(zm);
            all_Xs.push_back(vctDataPoints[n].x);
            all_Ys.push_back(vctDataPoints[n].y);
            all_ZMs.push_back(zm);

        }
        if(IsSaveZM)
        SaveZMsDatas(Xs,Ys,ZMs,i);   //save datas for caculate
        double lineBOW = 0;
        double D = Radius * 2;
        if (CalcuBowLine(Xs, Ys, ZMs,D, N, lineBOW, 50) > -1)
            BOWs.push_back(lineBOW);
        if (i == 0)
            maxBOW = lineBOW;
        if (abs(maxBOW) < abs(lineBOW))
            maxBOW = lineBOW;
    }

    if (IsNewBowAlg) 
    {
        warpResult.BOW = maxBOW;
    }

    FitZM3D(all_Xs, all_Ys, all_ZMs, all_ZMs.size());
        //新算法计算WARP有问题暂时屏蔽
    if (IsNewWarpAlg) {
        std::vector<double> out_AllXs;
        std::vector<double> out_AllYs;
        std::vector<double> out_AllZMs;
        std::vector<double> out_AllTs;
      
        double maxWARP;
        double WARP=0;
        double SORI = 0;
        int sampleNum = 50;
        for (size_t i = 0; i < lineNum; i++)
        {
            std::vector<DataPoint> vctDataPoints = m_vctAllDataPointByLine[i];
            std::vector<double> Xs;
            std::vector<double> Ys;
            std::vector<double> ZMs;
            std::vector<double> Ts;
            std::vector<double> out_Xs(sampleNum + 1);
            std::vector<double> out_Ys(sampleNum + 1);
            std::vector<double> out_ZMs(sampleNum + 1);
            std::vector<double> out_Ts(sampleNum + 1);
            int N = vctDataPoints.size();
           
            for (int n = 0; n < N; n++)
            {
                DataPoint& vctDataPoint = vctDataPoints[n];
                double z1 = (vctDataPoint.b - vctDataPoint.a) / 2.0;
                double zm = z1 - m_vctAllZGravityByLine[i][n];
                Xs.push_back(vctDataPoints[n].x);
                Ys.push_back(vctDataPoints[n].y);
                ZMs.push_back(zm);
                Ts.push_back(vctDataPoint.t);
                   
            }

            double lineWARP = 0;
            if (CalculWarpSingleLine(Xs, Ys, ZMs, Ts, lineWARP, N, sampleNum, out_Xs, out_Ys, out_ZMs,out_Ts) > -1)
            {
                WARPs.push_back(lineWARP);
                out_AllXs.insert(out_AllXs.end(), out_Xs.begin(), out_Xs.end());
                out_AllYs.insert(out_AllYs.end(), out_Ys.begin(), out_Ys.end()); 
                out_AllZMs.insert(out_AllZMs.end(), out_ZMs.begin(), out_ZMs.end());
                out_AllTs.insert(out_AllTs.end(), out_Ts.begin(), out_Ts.end());
                //if(IsSaveZM)
                //SaveZMsDatas(out_Xs, out_Ys, out_ZMs, i);   //save datas for caculate
            }
            if (i == 0)
                maxWARP = lineWARP;
            if (maxWARP < lineWARP)
                maxWARP = lineWARP;
        }
        CalculWarpAllLine(out_AllXs, out_AllYs, out_AllZMs,out_AllTs, WARP, SORI, out_AllXs.size(), lineNum,LineSampleNum);
        warpResult.WARP = WARP;
        warpResult.SORI = SORI;
       
    }

    qDebug() << u8"结束调用算法" << QDateTime::currentDateTime();

    emit s_measureOver(warpResult.BOW, warpResult.WARP, warpResult.CENTER_THK, warpResult.AVERAGE_THK, warpResult.TTV, warpResult.SORI);
#endif
    QString measureMess = QString("BOW:%1,WARP:%2,CENETR_THK:%3,AVERAGE_THK:%4,TTV:%5,SORI:%6").arg(warpResult.BOW, 0, 'G', 5).arg(warpResult.WARP, 0, 'G', 5).arg(warpResult.CENTER_THK, 0, 'G', 5).arg(warpResult.AVERAGE_THK, 0, 'G', 5).arg(warpResult.TTV, 0, 'G', 5).arg(warpResult.SORI, 0, 'G', 5);
    int length = BOWs.count();
    QString bowString = "";
    QString warpString = "";
    double sumBow = 0;
    double sumWarp = 0;
    for (int i = 0; i < length; i++)
    {
        bowString += QString("BOW"+ QString::number(i+1) +":%" + QString::number(i)).arg(BOWs[i])+",";
        sumBow += BOWs[i];
        if (i<WARPs.size())
        {
            warpString += QString("WARP" + QString::number(i + 1) + ":%" + QString::number(i)).arg(WARPs[i]) + ",";
            sumWarp += WARPs[i];
        }
     
    }
    bowString += QString("Avg_BOW:%1").arg(sumBow);
    warpString += QString("Avg_WARP:%1").arg(sumWarp);
    if (IsDebug) {
        emit s_writeLog(measureMess);
        emit s_writeLog(bowString);
        emit s_writeLog(warpString);
    }
    emit s_writeLog(u8"测量完成");
}

#endif

/// <summary>
/// 通过正反两面的数据计算并且保存重力补偿值
/// </summary>
void MeasureControl::CalAndSaveZGravity()
{
    QDir appDir = QCoreApplication::applicationDirPath();
    QString dirName = QString("ZGravityFile/type%1lines/%2/%3/ZGravity").arg(m_measurePath).arg(m_productSize).arg(m_productThickness);
    QString ZGravityFileFilePath = appDir.filePath(dirName);
    QDir dirZGravity(ZGravityFileFilePath);
    if (!dirZGravity.exists())
    {
        // 目录不存在，创建它
        if (dirZGravity.mkpath(".")) // "." 表示创建当前目录
        {
            qDebug() << "成功创建目录:" << ZGravityFileFilePath;
        }
        else
        {
            qDebug() << "创建目录失败:" << ZGravityFileFilePath;
            // 这里可以添加错误处理代码，例如显示错误消息或退出程序
        }
    }


    int lineNum = NormalAllZGravityByLine.size();
    int centerIndex = 0;
    double centerGravity = 0;
    for (int i = 0; i < lineNum; i++)
    {
        QVector<int> nLineIndexs= NormalAllZGravityByLine[i].keys().toVector();
        QVector<int> oLineIndexs = OppsiteAllZGravityByLine[i].keys().toVector();
        if (i == 0)
        {
            centerIndex = ChangeToEncoderValue(CircleCenter_X);
            double z1 = NormalAllZGravityByLine[i][centerIndex];
            double z2 = OppsiteAllZGravityByLine[i][centerIndex];
            double zm = (z1 - z2) / 2.0;
            centerGravity= z1 - zm;
        }
        nLineIndexs.removeOne(centerIndex);
        oLineIndexs.removeOne(centerIndex);
        int x_interval = XIntervals[i];

        int oOffset = 0;
        QVector<int> intersectionVec = findIntersection(nLineIndexs, oLineIndexs, x_interval, oOffset);
        
        QString fileName = QString("/z_gravity_%1.zg").arg(i);
        QString saveFile= appDir.filePath(dirName+ fileName);

        //QString fileCSVName = QString("/z_gravity_%1.csv").arg(i);
        //QString saveCSVFile = appDir.filePath(dirName + fileCSVName);
        //QFile file(saveCSVFile);

        ///* 以文本方式写入*/
        //if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        //    QTextStream CSVout(&file);


            QFile bin(saveFile);
            if (!bin.open(QIODevice::WriteOnly)) {
                qWarning() << u8"无法创建二进制文件：" << saveFile;
                return;
            }


            QDataStream out(&bin);
            out.setVersion(QDataStream::Qt_5_15);  // 确保跨版本兼容
            for (int& index : intersectionVec)
            {
                double z1 = NormalAllZGravityByLine[i].value(index);
                double z2 = OppsiteAllZGravityByLine[i].value(index - oOffset);

                double zm = (z1 - z2) / 2.0;
                double zGravity = z1 - zm;

                // 逐行写入
                QStringList fields;  // 按逗号分割
                fields.append(QString("%1").arg(zGravity));
                fields.append(QString("%1").arg(index));
                out << fields;  // 直接存入二进制文件

               // CSVout << QString("%1,%2").arg(zGravity).arg(index) << "\n";
            }
          
            
            QStringList fields;  // 按逗号分割
            fields.append(QString("%1").arg(centerGravity));
            fields.append(QString("%1").arg(centerIndex));
            out << fields;  // 直接存入二进制文件

           /* CSVout << QString("%1,%2").arg(centerGravity).arg(centerIndex) << "\n";*/

            bin.close();

        //}
        //file.close();
    }
    
}

void MeasureControl::SaveZMsDatas(std::vector<double> Xs, std::vector<double> Ys, std::vector<double> ZMs, int index)
{
    QDir appDir = QCoreApplication::applicationDirPath();
    QString dirName = QString("ZGravityFile/type%1lines/%2/%3/ZGravity/ZMDatas/"+ProductName).arg(m_measurePath).arg(m_productSize).arg(m_productThickness);
    QString ZGravityFileFilePath = appDir.filePath(dirName);
    QDir dirZGravity(ZGravityFileFilePath);
    if (!dirZGravity.exists())
    {
        // 目录不存在，创建它
        if (dirZGravity.mkpath(".")) // "." 表示创建当前目录
        {
            qDebug() << "成功创建目录:" << ZGravityFileFilePath;
        }
        else
        {
            qDebug() << "创建目录失败:" << ZGravityFileFilePath;
            // 这里可以添加错误处理代码，例如显示错误消息或退出程序
        }
    }
    SaveHeatImageFile = QString("%1/%2_2D.png").arg(ZGravityFileFilePath).arg(ProductName);
    Save3DImageFile = QString("%1/%2_3D.png").arg(ZGravityFileFilePath).arg(ProductName);
    QString fileCSVName = QString("/ZM_%1.csv").arg(index);
    QString saveCSVFile = appDir.filePath(dirName + fileCSVName);
    QFile file(saveCSVFile);
    /* 以文本方式写入*/
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream CSVout(&file);
        for (size_t i = 0; i < ZMs.size(); i++)
        {
            CSVout << QString("%1,%2,%3").arg(Xs[i]).arg(Ys[i]).arg(ZMs[i]) << "\n";
        }
      

    }
    file.close();

}


QVector<int>& MeasureControl::Get_vecEncoders_top()
{
    return  m_pColorFocusControl_top->GetEncoders(); 
}
QVector<int>& MeasureControl::Get_vecEncoders_bottom()
{
    return  m_pColorFocusControl_bottom->GetEncoders();
}
void MeasureControl::OrganizeData(std::vector<std::vector<DataPoint>>& vctAllDataPointByLine, std::vector<DataPoint>& vctAllDataPoint, int x_interval)
{
    m_dataBuffer_alg_top.clear();
    m_dataBuffer_alg_bottom.clear();

    QMap<INT32, _DISTANCE_VALUE>& mapDistance_top = m_pColorFocusControl_top->GetMeasurePointDistanceMap();
    QMap<INT32, _DISTANCE_VALUE>& mapDistance_bottom = m_pColorFocusControl_bottom->GetMeasurePointDistanceMap();

    QVector<int> &vecEncoders_top = m_pColorFocusControl_top->GetEncoders();
    QVector<int> &vecEncoders_bottom = m_pColorFocusControl_bottom->GetEncoders();

    if(mapDistance_top.empty() || mapDistance_bottom.empty())
    {
        emit s_writeLog(u8"该路径数据为空");
        return;
    }

    double middlePos = 0.0;

    QMap<int, double> m_mapZGravity = m_currentPathTypeZgravityMap.value(m_currentPathIndex);
    QVector<int> xIndexVec = m_mapZGravity.keys().toVector();
    xIndexVec.removeOne(ChangeToEncoderValue(CircleCenter_X));
    //上共聚焦与重力补偿值的交集
    int offset_top = 0;
    QVector<int> intersectionVec_topAndZG = findIntersection(xIndexVec, vecEncoders_top, x_interval, offset_top);
    
    //下共聚焦与重力补偿值的交集
    int offset_bottom = 0;
    QVector<int> intersectionVec_bottomAndZG = findIntersection(xIndexVec, vecEncoders_bottom, x_interval, offset_bottom);

    int offset = 0;
    //上下共聚焦的交集
    QVector<int> intersectionVec = findIntersection(intersectionVec_topAndZG, intersectionVec_bottomAndZG, x_interval, offset);

    debugError(offset_top, offset_bottom, x_interval, intersectionVec_topAndZG, intersectionVec_bottomAndZG);

    std::vector<DataPoint> vctDataPoint;
    std::vector<double> vecZGravity;
    QVector<double> vecThickness;

    for(int& intersectionEncoder : intersectionVec)
    {
        _DISTANCE_VALUE value_top = mapDistance_top.value(intersectionEncoder - offset_top);
        _DISTANCE_VALUE value_bottom = mapDistance_bottom.value(intersectionEncoder - offset_bottom);

        if(value_top.distance < 0 || value_bottom.distance < 0 ){
            continue;
        }

        double x = ChangeToPositionValue(value_top.Encoder1);
        double y = ChangeToPositionValue(value_top.Encoder2);
        int x_en = value_top.Encoder1;
        int y_en = value_top.Encoder2;
        double topDistance = filterValue_alg_top(value_top.distance);
        double bottomDistance = filterValue_alg_bottom(value_bottom.distance);
        //double topDistance = /*filterValue_alg_top*/(value_top.distance);
        //double bottomDistance = /*filterValue_alg_bottom*/(value_bottom.distance);
        double realThickness = m_currentCalibrationInfo.total - topDistance - bottomDistance;
        if(realThickness > 1000)
        {
            qDebug() << "hhhhhhhhhhhhhhhhh";
        }

        DataPoint dataPoint{x, y, topDistance, bottomDistance, realThickness, middlePos, x_en, y_en};

        if(!m_mapZGravity.contains(intersectionEncoder))
        {
            qDebug()<< QString(u8"重力补偿值中不包含：%1").arg(intersectionEncoder);
            continue;
        }

        m_vecAllZGravity.push_back(m_mapZGravity.value(intersectionEncoder));
        vecZGravity.push_back(m_mapZGravity.value(intersectionEncoder));

        vecThickness.append(realThickness);
        vctDataPoint.push_back(dataPoint);
        vctAllDataPoint.push_back(dataPoint);
    }

    m_vctAllZGravityByLine.push_back(vecZGravity);
    vctAllDataPointByLine.push_back(vctDataPoint);
    SaveLineDistanceMats(vctDataPoint, m_currentPathIndex);

    emit s_pathDataOver(m_currentPathIndex);
}

void MeasureControl::OrganizeDataNoIntersection(std::vector<std::vector<DataPoint>>& vctAllDataPointByLine, std::vector<DataPoint>& vctAllDataPoint, int x_interval)
{
    QMap<INT32, _DISTANCE_VALUE>& mapDistance_top = m_pColorFocusControl_top->GetMeasurePointDistanceMap();

    QVector<int>& vecEncoders_top = m_pColorFocusControl_top->GetEncoders();

    if (mapDistance_top.empty())
    {
        emit s_writeLog(u8"该路径数据为空");
        return;
    }


    std::vector<DataPoint> vctDataPoint;
    std::vector<double> vecZGravity;
    QVector<double> vecThickness;

    for(auto it = mapDistance_top.constBegin(); it != mapDistance_top.constEnd(); it++)
    {
        _DISTANCE_VALUE value_top = it.value();

        if (value_top.distance < 0 ) {
            continue;
        }

        double x = ChangeToPositionValue(value_top.Encoder1);
        double y = ChangeToPositionValue(value_top.Encoder2);
        int x_en = value_top.Encoder1;
        int y_en = value_top.Encoder2;
 
        double topDistance = filterValue_alg_top(value_top.distance);
  
        //double topDistance = /*filterValue_alg_top*/(value_top.distance);
        //double bottomDistance = /*filterValue_alg_bottom*/(value_bottom.distance);
        double realThickness = 0;
        if (realThickness > 1000)
        {
            qDebug() << "hhhhhhhhhhhhhhhhh";
        }

        DataPoint dataPoint{ x, y, topDistance, 0, 0, 0, x_en, y_en };

        vctDataPoint.push_back(dataPoint);
        vctAllDataPoint.push_back(dataPoint);
    }

    vctAllDataPointByLine.push_back(vctDataPoint);
    SaveLineDistanceMats(vctDataPoint, m_currentPathIndex);

    emit s_pathDataOver(m_currentPathIndex);
}

void MeasureControl::debugError(int offset_top, int offset_bottom, int x_interval, QVector<int> intersectionVec_topAndZG, QVector<int> intersectionVec_bottomAndZG)
{
    m_offset_top = offset_top;
    m_offset_bottom = offset_bottom;
    m_x_interval = x_interval;
    m_intersectionVec_topAndZG = intersectionVec_topAndZG;
    m_intersectionVec_bottomAndZG = intersectionVec_bottomAndZG;
}

void MeasureControl::OrganizeDataForCalBow(std::vector<DataPoint>& nor_data, std::vector<double>& z_gravity)
{
    if (m_vctAllDataPointByLine.empty()) {
        return;
    }
    std::vector<DataPoint>& vctDataPoint  = m_vctAllDataPointByLine.back();
    int linePointCount = vctDataPoint.size();

    nor_data.push_back(vctDataPoint.at(0));
    nor_data.push_back(vctDataPoint.at(1));
    nor_data.push_back(vctDataPoint.at(2));
    nor_data.push_back(vctDataPoint.at(3));
    nor_data.push_back(vctDataPoint.at(4));
    nor_data.push_back(vctDataPoint.at(linePointCount - 5));
    nor_data.push_back(vctDataPoint.at(linePointCount - 4));
    nor_data.push_back(vctDataPoint.at(linePointCount - 3));
    nor_data.push_back(vctDataPoint.at(linePointCount - 2));
    nor_data.push_back(vctDataPoint.at(linePointCount - 1));

    std::vector<double>& vctZGravity  = m_vctAllZGravityByLine.back();
    z_gravity.push_back(vctZGravity.at(0));
    z_gravity.push_back(vctZGravity.at(1));
    z_gravity.push_back(vctZGravity.at(2));
    z_gravity.push_back(vctZGravity.at(3));
    z_gravity.push_back(vctZGravity.at(4));
    z_gravity.push_back(vctZGravity.at(linePointCount - 5));
    z_gravity.push_back(vctZGravity.at(linePointCount - 5));
    z_gravity.push_back(vctZGravity.at(linePointCount - 5));
    z_gravity.push_back(vctZGravity.at(linePointCount - 5));
    z_gravity.push_back(vctZGravity.at(linePointCount - 5));

}

void MeasureControl::OrganizeDataSimply(std::vector<std::vector<DataPoint>>& vctAllDataPointByLine, std::vector<DataPoint>& vctAllDataPoint)
{
    QMap<INT32, _DISTANCE_VALUE> &mapDistance_top = m_pColorFocusControl_top->GetMeasurePointDistanceMap();
    QMap<INT32, _DISTANCE_VALUE> &mapDistance_bottom = m_pColorFocusControl_bottom->GetMeasurePointDistanceMap();

    int topDataCount = mapDistance_top.count();
    int bottomDataCount = mapDistance_bottom.count();

    std::vector<DataPoint> vctDataPoint;

    double middlePos = 0.0;

    if(topDataCount <= bottomDataCount)
    {
        for (INT32 key : mapDistance_top.keys())
        {
            if(!mapDistance_bottom.contains(key)){
                continue;
            }
            _DISTANCE_VALUE value_top = mapDistance_top.value(key);
            _DISTANCE_VALUE value_bottom = mapDistance_bottom.value(key);

            double x = ChangeToPositionValue(value_top.Encoder1);
            double y = ChangeToPositionValue(value_top.Encoder2);
            int x_en = value_top.Encoder1;
            int y_en = value_top.Encoder2;
            if(!(value_top.distance > 0 && value_bottom.distance > 0)){
                continue;
            }
            double topDistance = value_top.distance;
            double bottomDistance = value_bottom.distance;
            double realThickness = m_currentCalibrationInfo.total - topDistance - bottomDistance;

            DataPoint dataPoint{x, y, topDistance, bottomDistance, realThickness, middlePos, x_en, y_en};
            vctDataPoint.push_back(dataPoint);
            vctAllDataPoint.push_back(dataPoint);
        }
    }
    else
    {
        for (INT32 key : mapDistance_bottom.keys())
        {
            if(!mapDistance_top.contains(key)){
                continue;
            }
            _DISTANCE_VALUE value_top = mapDistance_top.value(key);
            _DISTANCE_VALUE value_bottom = mapDistance_bottom.value(key);

            double x = ChangeToPositionValue(value_bottom.Encoder1);
            double y = ChangeToPositionValue(value_bottom.Encoder2);
            int x_en = value_bottom.Encoder1;
            int y_en = value_bottom.Encoder2;
            if(!(value_top.distance > 0 && value_bottom.distance > 0)){
                continue;
            }
            double topDistance = value_top.distance;
            double bottomDistance = value_bottom.distance;
            double realThickness = m_currentCalibrationInfo.total - topDistance - bottomDistance;

            DataPoint dataPoint{x, y, topDistance, bottomDistance, realThickness, middlePos, x_en, y_en};
            vctDataPoint.push_back(dataPoint);
            vctAllDataPoint.push_back(dataPoint);
        }
    }

    vctAllDataPointByLine.push_back(vctDataPoint);
}

void MeasureControl::OrganizeDataForZGravity(std::vector<std::vector<DataPoint>>& vctAllDataPointByLine, std::vector<DataPoint>& vctAllDataPoint, int x_interval,bool x_trigerMode)
{
    m_dataBuffer_alg_top.clear();
    m_dataBuffer_alg_bottom.clear();
   
    QMap<INT32, _DISTANCE_VALUE> &mapDistance_top = m_pColorFocusControl_top->GetMeasurePointDistanceMap();
    QMap<INT32, _DISTANCE_VALUE> &mapDistance_bottom = m_pColorFocusControl_bottom->GetMeasurePointDistanceMap();

 
    if (IsSaveZM)
    {
        QDir appDir = QCoreApplication::applicationDirPath();
        QString tag = ProgramMeasureMode == ACQNormalGravity ? "Normal" : "Oppsite";
        QString dirName = QString("ZGravityFile/type%1lines/%2/%3/ZGravity/%4").arg(m_measurePath).arg(m_productSize).arg(m_productThickness).arg(tag);
        QString ZGravityFileFilePath = appDir.filePath(dirName);
        QDir dirZGravity(ZGravityFileFilePath);
        if (!dirZGravity.exists())
        {
            // 目录不存在，创建它
            if (dirZGravity.mkpath(".")) // "." 表示创建当前目录
            {
                qDebug() << "成功创建目录:" << ZGravityFileFilePath;
            }
            else
            {
                qDebug() << "创建目录失败:" << ZGravityFileFilePath;
                // 这里可以添加错误处理代码，例如显示错误消息或退出程序
            }
        }

        static int lineIndex = 0;
        lineIndex += 1;
        QString currentDir = QCoreApplication::applicationDirPath();
        QString currentfileName = QString("%1/line_%2.csv").arg(ZGravityFileFilePath).arg(lineIndex);
        SaveHeatImageFile = QString("%1/%2_%3_%4_%5_2D.png").arg(ZGravityFileFilePath).arg(ProductName).arg(m_measurePath).arg(m_productSize).arg(m_productThickness);
        Save3DImageFile = QString("%1/%2_%3_%4_%5_3D.png").arg(ZGravityFileFilePath).arg(ProductName).arg(m_measurePath).arg(m_productSize).arg(m_productThickness);
        QFile file(currentfileName);

        // 以文本方式写入
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            for (int& index : mapDistance_top.keys())
            {
                _DISTANCE_VALUE value = mapDistance_top.value(index);
                double x = ChangeToPositionValue(value.Encoder1);
                double y = ChangeToPositionValue(value.Encoder2);
                // 逐行写入
                out << QString("%1,%2,%3").arg(x).arg(y).arg(value.distance) << "\n";
            }


        }
        else {
            qDebug() << "无法打开文件：" << file.errorString();
        }

        file.close();

    }
   
    if(mapDistance_top.empty() || mapDistance_bottom.empty())
    {
        emit s_writeLog(u8"该路径数据为空");
        return;
    }

    double middlePos = 0.0;

    int offset = 0;
    QVector<int> intersectionVec = findIntersection(m_pColorFocusControl_top->GetEncoders(), m_pColorFocusControl_bottom->GetEncoders(), x_interval, offset);
    
    std::vector<DataPoint> vctDataPoint;
    QVector<double> vecThickness;
    
    for(int& intersectionEncoder : intersectionVec)
    {
        _DISTANCE_VALUE value_top = mapDistance_top.value(intersectionEncoder);
        _DISTANCE_VALUE value_bottom = mapDistance_bottom.value(intersectionEncoder - offset);

        if(value_top.distance < 0 || value_bottom.distance < 0 ){
            continue;
        }
      
        double x = ChangeToPositionValue(value_top.Encoder1);
        double y = ChangeToPositionValue(value_top.Encoder2);
        int x_en = value_top.Encoder1;
        int y_en = value_top.Encoder2;
        double topDistance = /*filterValue_alg_top*/(value_top.distance);
        double bottomDistance = /*filterValue_alg_bottom*/(value_bottom.distance);
        double realThickness = m_currentCalibrationInfo.total - topDistance - bottomDistance;
        
        if (!x_trigerMode)
        {
            x_en = y_en;
        }

        DataPoint dataPoint{x, y, topDistance, bottomDistance, realThickness, middlePos, x_en, y_en};
       
        vecThickness.append(realThickness);
        vctDataPoint.push_back(dataPoint);
        vctAllDataPoint.push_back(dataPoint);
       
    }

    vctAllDataPointByLine.push_back(vctDataPoint);
   
    emit s_pathDataOver(m_currentPathIndex);
}

void MeasureControl::OrganizeDataMirrorCalibration(std::vector<std::vector<DataPoint>>& vctAllDataPointByLine, std::vector<DataPoint>& vctAllDataPoint)
{
    QMap<INT32, _DISTANCE_VALUE> &mapDistance_top = m_pColorFocusControl_top->GetMeasurePointDistanceMap();

    std::vector<DataPoint> vctDataPoint;

    for (INT32 key : mapDistance_top.keys())
    {
        _DISTANCE_VALUE value_top = mapDistance_top.value(key);

        double x = ChangeToPositionValue(value_top.Encoder1);
        double y = ChangeToPositionValue(value_top.Encoder2);
        int x_en = value_top.Encoder1;
        int y_en = value_top.Encoder2;
        double topDistance = value_top.distance;
        if(topDistance < 0){
            continue;
        }

        DataPoint dataPoint{x, y, topDistance, 0, 0, 0, x_en, y_en};
        vctDataPoint.push_back(dataPoint);
        vctAllDataPoint.push_back(dataPoint);
    }

    vctAllDataPointByLine.push_back(vctDataPoint);
}

void MeasureControl::OrganizeDataTopAndBottom(std::vector<std::vector<DataPoint>>& vctTopDataPointByLine, std::vector<std::vector<DataPoint>>& vctBottomDataPointByLine)
{
    QMap<INT32, _DISTANCE_VALUE> &mapDistance_top = m_pColorFocusControl_top->GetMeasurePointDistanceMap();
    QMap<INT32, _DISTANCE_VALUE> &mapDistance_bottom = m_pColorFocusControl_bottom->GetMeasurePointDistanceMap();

    std::vector<DataPoint> vctDataPoint_top;
    std::vector<DataPoint> vctDataPoint_bottom;

    for (auto it = mapDistance_top.constBegin(); it != mapDistance_top.constEnd(); ++it)
    {
        _DISTANCE_VALUE value_top = mapDistance_top.value(it.key());

        double x = ChangeToPositionValue(value_top.Encoder1);
        double y = ChangeToPositionValue(value_top.Encoder2);
        int x_en = value_top.Encoder1;
        int y_en = value_top.Encoder2;
        double topDistance = value_top.distance;
        if(value_top.distance < 0){
            continue;
        }

        DataPoint dataPoint{x, y, topDistance, 0, 0, 0, x_en, y_en};
        vctDataPoint_top.push_back(dataPoint);
    }
    for (auto it = mapDistance_bottom.constBegin(); it != mapDistance_bottom.constEnd(); ++it)
    {
        _DISTANCE_VALUE value_bottom = mapDistance_bottom.value(it.key());

        double x = ChangeToPositionValue(value_bottom.Encoder1);
        double y = ChangeToPositionValue(value_bottom.Encoder2);
        int x_en = value_bottom.Encoder1;
        int y_en = value_bottom.Encoder2;
        double bottomDistance = value_bottom.distance;
        if(value_bottom.distance < 0){
            continue;
        }

        DataPoint dataPoint{x, y, 0, bottomDistance, 0, 0, x_en, y_en};
        vctDataPoint_bottom.push_back(dataPoint);
    }
    vctTopDataPointByLine.push_back(vctDataPoint_top);
    vctBottomDataPointByLine.push_back(vctDataPoint_bottom);
}

void MeasureControl::SaveLineDistanceMats(const std::vector<DataPoint>& lineData, int lineIndex)
{
    if (lineData.empty()) {
        emit s_writeLog(QString("Skip line distance Mat save, empty line: %1").arg(lineIndex + 1));
        return;
    }

    if (m_lineDistanceMatRunDir.isEmpty()) {
        QString modeTag = "Measure";
        if (ProgramMeasureMode == ACQNormalGravity) {
            modeTag = "GravityNormal";
        } else if (ProgramMeasureMode == ACQOppsiteGravity) {
            modeTag = "GravityOppsite";
        }
        const QString safeProductName = ProductName.isEmpty() ? QString("Unknown") : ProductName;
        const QString runName = QString("%1_%2_%3")
                                    .arg(modeTag)
                                    .arg(safeProductName)
                                    .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz"));
        QDir appDir = QCoreApplication::applicationDirPath();
        m_lineDistanceMatRunDir = appDir.filePath(QString("LineDistanceMats/type%1lines/%2/%3/%4")
                                                      .arg(m_measurePath)
                                                      .arg(m_productSize)
                                                      .arg(m_productThickness)
                                                      .arg(runName));
    }

    QDir matDir(m_lineDistanceMatRunDir);
    if (!matDir.exists() && !matDir.mkpath(".")) {
        const QString errorMsg = QString("Create line distance Mat directory failed: %1").arg(m_lineDistanceMatRunDir);
        emit s_writeLog(errorMsg);
        emit s_sendErrorMsg(0, errorMsg);
        return;
    }

    cv::Mat topMat(static_cast<int>(lineData.size()), 3, CV_32FC1);
    cv::Mat bottomMat(static_cast<int>(lineData.size()), 3, CV_32FC1);
    for (int i = 0; i < static_cast<int>(lineData.size()); ++i) {
        const DataPoint& point = lineData.at(i);
        topMat.at<float>(i, 0) = static_cast<float>(point.x);
        topMat.at<float>(i, 1) = static_cast<float>(point.y);
        topMat.at<float>(i, 2) = static_cast<float>(point.a);
        bottomMat.at<float>(i, 0) = static_cast<float>(point.x);
        bottomMat.at<float>(i, 1) = static_cast<float>(point.y);
        bottomMat.at<float>(i, 2) = static_cast<float>(point.b);
    }

    const QString topPath = matDir.filePath(QString("top_line_%1.xml").arg(lineIndex));
    const QString bottomPath = matDir.filePath(QString("bottom_line_%1.xml").arg(lineIndex));
    try {
        cv::FileStorage topStorage(topPath.toLocal8Bit().constData(), cv::FileStorage::WRITE);
        if (!topStorage.isOpened()) {
            const QString errorMsg = QString("Open top line Mat file failed: %1").arg(topPath);
            emit s_writeLog(errorMsg);
            emit s_sendErrorMsg(0, errorMsg);
            return;
        }
        topStorage << "columns" << "x,y,top";
        topStorage << "mat" << topMat;
        topStorage.release();

        cv::FileStorage bottomStorage(bottomPath.toLocal8Bit().constData(), cv::FileStorage::WRITE);
        if (!bottomStorage.isOpened()) {
            const QString errorMsg = QString("Open bottom line Mat file failed: %1").arg(bottomPath);
            emit s_writeLog(errorMsg);
            emit s_sendErrorMsg(0, errorMsg);
            return;
        }
        bottomStorage << "columns" << "x,y,bottom";
        bottomStorage << "mat" << bottomMat;
        bottomStorage.release();
    } catch (const cv::Exception& e) {
        const QString errorMsg = QString("Save line distance Mat failed: line=%1, error=%2")
                                     .arg(lineIndex + 1)
                                     .arg(QString::fromLocal8Bit(e.what()));
        emit s_writeLog(errorMsg);
        emit s_sendErrorMsg(0, errorMsg);
        return;
    }

    emit s_writeLog(QString("Saved line distance Mat: line=%1, points=%2, dir=%3")
                        .arg(lineIndex + 1)
                        .arg(static_cast<int>(lineData.size()))
                        .arg(m_lineDistanceMatRunDir));
}

void MeasureControl::WriteDistanceDataToFile(std::vector<std::vector<DataPoint>>& vctAllDataPointByLine)
{
    QDir appDir = QCoreApplication::applicationDirPath();
    QString dirName;
    if (ProgramMeasureMode == NormalMeasure) {
        const QString safeProductName = ProductName.isEmpty() ? QString("Unknown") : ProductName;
        const QString runName = QString("%1_%2")
                                    .arg(safeProductName)
                                    .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz"));
        dirName = QString("MeasureLineData/type%1lines/%2/%3/%4")
                      .arg(m_measurePath)
                      .arg(m_productSize)
                      .arg(m_productThickness)
                      .arg(runName);
    } else {
        QString tag = ProgramMeasureMode == ACQNormalGravity ? "Normal" : "Oppsite";
        dirName = QString("ZGravityFile/type%1lines/%2/%3/ZGravity/%4")
                      .arg(m_measurePath)
                      .arg(m_productSize)
                      .arg(m_productThickness)
                      .arg(tag);
    }

    const QString lineDataDirPath = appDir.filePath(dirName);
    QDir lineDataDir(lineDataDirPath);
    if (!lineDataDir.exists() && !lineDataDir.mkpath(".")) {
        QString errorMsg = QString("Create line data directory failed: %1").arg(lineDataDirPath);
        emit s_writeLog(errorMsg);
        emit s_sendErrorMsg(0, errorMsg);
        return;
    }

    emit s_writeLog(QString("Save line data csv directory: %1").arg(lineDataDirPath));
    for (int lineIndex = 0; lineIndex < static_cast<int>(vctAllDataPointByLine.size()); ++lineIndex)
    {
        const QString currentfileName = QString("%1/line_%2.csv").arg(lineDataDirPath).arg(lineIndex);
        QFile file(currentfileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QString errorMsg = QString("Open line data csv failed: %1, error=%2")
                                   .arg(currentfileName)
                                   .arg(file.errorString());
            emit s_writeLog(errorMsg);
            emit s_sendErrorMsg(0, errorMsg);
            continue;
        }

        QTextStream out(&file);
        out << "x,y,x_en,y_en,top,bottom,thickness,zm\n";
        const std::vector<DataPoint>& vctDataPoint = vctAllDataPointByLine.at(lineIndex);
        for (int pointIndex = 0; pointIndex < static_cast<int>(vctDataPoint.size()); ++pointIndex) {
            const DataPoint& point = vctDataPoint.at(pointIndex);
            out << QString("%1,%2,%3,%4,%5,%6,%7,%8")
                       .arg(point.x)
                       .arg(point.y)
                       .arg(point.x_en)
                       .arg(point.y_en)
                       .arg(point.a)
                       .arg(point.b)
                       .arg(point.t)
                       .arg(point.zm) << "\n";
        }
        file.close();
        emit s_writeLog(QString("Saved line data csv: %1, points=%2")
                            .arg(currentfileName)
                            .arg(static_cast<int>(vctDataPoint.size())));
    }
}

void MeasureControl::WriteMirrorCalibrationDataToFile(std::vector<std::vector<DataPoint>>& vctAllDataPointByLine)
{
    for (int lineIndex = 0; lineIndex < vctAllDataPointByLine.size(); ++lineIndex)
    {
        QString currentDir = QCoreApplication::applicationDirPath();
        QString currentfileName = QString("%1/line_%2.csv").arg(currentDir).arg(lineIndex);
        QFile file(currentfileName);

        // 以文本方式写入
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);

            std::vector<DataPoint> vctDataPoint = vctAllDataPointByLine.at(lineIndex);
            for (int pointIndex = 0; pointIndex < vctDataPoint.size(); ++pointIndex) {

                out << QString("%1,%2,%3,%4,%5").arg(vctDataPoint.at(pointIndex).x).arg(vctDataPoint.at(pointIndex).y)
                           .arg(vctDataPoint.at(pointIndex).x_en).arg(vctDataPoint.at(pointIndex).y_en)
                           .arg(vctDataPoint.at(pointIndex).a) << "\n";
            }

        } else {
            qDebug() << "无法打开文件：" << file.errorString();
        }

        file.close();
    }
}

void MeasureControl::WriteTopAndBottomDistanceDataToFile(std::vector<std::vector<DataPoint>>& vctTopDataPointByLine, std::vector<std::vector<DataPoint>>& vctBottomDataPointByLine)
{
    for (int lineIndex = 0; lineIndex < vctTopDataPointByLine.size(); ++lineIndex)
    {
        QString currentDir = QCoreApplication::applicationDirPath();
        QString currentfileName = QString("%1/top_line_%2.csv").arg(currentDir).arg(lineIndex);
        QFile file(currentfileName);

        // 以文本方式写入
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);

            std::vector<DataPoint> vctDataPoint = vctTopDataPointByLine.at(lineIndex);
            for (int pointIndex = 0; pointIndex < vctDataPoint.size(); ++pointIndex) {
                // 逐行写入
                out << QString("%1,%2,%3,%4,%5").arg(vctDataPoint.at(pointIndex).x).arg(vctDataPoint.at(pointIndex).y)
                           .arg(vctDataPoint.at(pointIndex).x_en).arg(vctDataPoint.at(pointIndex).y_en)
                           .arg(vctDataPoint.at(pointIndex).a) << "\n";
            }

        } else {
            qDebug() << "无法打开文件：" << file.errorString();
        }

        file.close();
    }
    for (int lineIndex = 0; lineIndex < vctBottomDataPointByLine.size(); ++lineIndex)
    {
        QString currentDir = QCoreApplication::applicationDirPath();
        QString currentfileName = QString("%1/bottom_line_%2.csv").arg(currentDir).arg(lineIndex);
        QFile file(currentfileName);

        // 以文本方式写入
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);

            std::vector<DataPoint> vctDataPoint = vctBottomDataPointByLine.at(lineIndex);
            for (int pointIndex = 0; pointIndex < vctDataPoint.size(); ++pointIndex) {
                // 逐行写入
                out << QString("%1,%2,%3,%4,%5").arg(vctDataPoint.at(pointIndex).x).arg(vctDataPoint.at(pointIndex).y)
                           .arg(vctDataPoint.at(pointIndex).x_en).arg(vctDataPoint.at(pointIndex).y_en)
                           .arg(vctDataPoint.at(pointIndex).b) << "\n";
            }

        } else {
            qDebug() << "无法打开文件：" << file.errorString();
        }

        file.close();
    }
}

void MeasureControl::StopMeasure()
{
    // if(m_timer_1)
    // {
    //     m_timer_1->stop();
    // }
    // if(m_timer_2)
    // {
    //     m_timer_2->stop();
    // }
}


void MeasureControl::handleTimeout_1()
{
    if(m_bStopMeasureFlag)
    {
        // QThread::currentThread()->quit();
        return;
    }
    GetOneLineDataFromColorFocus();
    
    m_timer_2->start(3000);

}

void MeasureControl::handleTimeout_2()
{
    if(m_bStopMeasureFlag)
    {
        // QThread::currentThread()->quit();
        return;
    }
    ClearColorFocusStack();

    m_timer_1->start(10000);
}

void MeasureControl::ClearColorFocusStack()
{
    m_processType = ProcessType::ClearStack;
    m_pSerialCommunicator->sendCommand(stopTriggerCommand);
    // QMetaObject::invokeMethod(m_pSerialCommunicator.data(), "sendCommand", Qt::QueuedConnection, Q_ARG(QString, stopTriggerCommand));
}

void MeasureControl::GetOneLineDataFromColorFocus()
{
    m_processType = ProcessType::ReadDistance;
    m_pSerialCommunicator->sendCommand(stopTriggerCommand);
    // QMetaObject::invokeMethod(m_pSerialCommunicator.data(), "sendCommand", Qt::QueuedConnection, Q_ARG(QString, stopTriggerCommand));
}

void MeasureControl::StartTrigger()
{
    m_pSerialCommunicator->sendCommand(startTriggerCommand);
    // QMetaObject::invokeMethod(m_pSerialCommunicator.data(), "sendCommand", Qt::BlockingQueuedConnection, Q_ARG(QString, startTriggerCommand));
    m_processType = ProcessType::None;
}

void MeasureControl::OnUpdateStopFlag()
{
    m_bStopMeasureFlag = true;

    //切换触发模式为Continue
    m_pColorFocusControl_top->StopAcquisition_CCS();
    m_pColorFocusControl_top->StartAcquisition_CCS(TriggerMode::Continue);
    m_pColorFocusControl_bottom->StopAcquisition_CCS();
    m_pColorFocusControl_bottom->StartAcquisition_CCS(TriggerMode::Continue);
}

void MeasureControl::OnupdateEStopFlag()
{
    m_bEStopFlag = true;
}

double MeasureControl::filterValue_top(double newValue)
{
    m_dataBuffer_top.append(newValue);

    // 保持窗口大小
    if (m_dataBuffer_top.size() > m_windowSize) {
        m_dataBuffer_top.removeFirst();
    }

    // ----------- 中值滤波 -----------
    QVector<double> sortedData = m_dataBuffer_top;
    std::sort(sortedData.begin(), sortedData.end());
    double medianValue = sortedData[sortedData.size() / 2];

    // ----------- 滑动平均滤波 -----------
    double avgValue = std::accumulate(sortedData.begin(), sortedData.end(), 0.0) / sortedData.size();

    // ----------- 数据融合 -----------
    return (medianValue * 0.7) + (avgValue * 0.3);  // 组合滤波，按权重调整
}

double MeasureControl::filterValue_bottom(double newValue)
{
    m_dataBuffer_bottom.append(newValue);

    // 保持窗口大小
    if (m_dataBuffer_bottom.size() > m_windowSize) {
        m_dataBuffer_bottom.removeFirst();
    }

    // ----------- 中值滤波 -----------
    QVector<double> sortedData = m_dataBuffer_bottom;
    std::sort(sortedData.begin(), sortedData.end());
    double medianValue = sortedData[sortedData.size() / 2];

    // ----------- 滑动平均滤波 -----------
    double avgValue = std::accumulate(sortedData.begin(), sortedData.end(), 0.0) / sortedData.size();

    // ----------- 数据融合 -----------
    return (medianValue * 0.7) + (avgValue * 0.3);  // 组合滤波，按权重调整
}

double MeasureControl::filterValue_alg_top(double newValue)
{
    m_dataBuffer_alg_top.append(newValue);

    // 保持窗口大小
    if (m_dataBuffer_alg_top.size() > m_windowSize_alg) {
        m_dataBuffer_alg_top.removeFirst();
    }

    // ----------- 中值滤波 -----------
    QVector<double> sortedData = m_dataBuffer_alg_top;
    std::sort(sortedData.begin(), sortedData.end());
    double medianValue = sortedData[sortedData.size() / 2];

    // ----------- 滑动平均滤波 -----------
    double avgValue = std::accumulate(sortedData.begin(), sortedData.end(), 0.0) / sortedData.size();

    // ----------- 数据融合 -----------
    return (medianValue * 0.7) + (avgValue * 0.3);  // 组合滤波，按权重调整
}

double MeasureControl::filterValue_alg_bottom(double newValue)
{
    m_dataBuffer_alg_bottom.append(newValue);

    // 保持窗口大小
    if (m_dataBuffer_alg_bottom.size() > m_windowSize_alg) {
        m_dataBuffer_alg_bottom.removeFirst();
    }

    // ----------- 中值滤波 -----------
    QVector<double> sortedData = m_dataBuffer_alg_bottom;
    std::sort(sortedData.begin(), sortedData.end());
    double medianValue = sortedData[sortedData.size() / 2];

    // ----------- 滑动平均滤波 -----------
    double avgValue = std::accumulate(sortedData.begin(), sortedData.end(), 0.0) / sortedData.size();

    // ----------- 数据融合 -----------
    return (medianValue * 0.7) + (avgValue * 0.3);  // 组合滤波，按权重调整
}

QVector<int> MeasureControl::findIntersection(const QVector<int>& A, const QVector<int>& B, int step, int& offset)
{
    // 1. 直接求交集
    QSet<int> setA;
    for (int a : A)
        setA.insert(a);

    QSet<int> intersection;
    for (int b : B) {
        if (setA.contains(b))
            intersection.insert(b);
    }
   
    if (!intersection.isEmpty()) {
        // 交集不为空，直接返回排序后的结果
        QVector<int> result = intersection.values().toVector();
        std::sort(result.begin(), result.end());

        offset = 0;
        return result;
    }

    // 2. 如果直接交集为空，认为存在“错位”，则尝试对齐
    // 计算 A 和 B 的首元素对 step 的余数（确保为正值）
    int modA = (A.first() % step + step) % step;
    int modB = (B.first() % step + step) % step;
    offset = modA - modB;  // 要把 B 数组调整 offset 个单位后与 A 对齐

    // 调整 B 数组（新建一个偏移后的副本）
    QVector<int> B_shifted;
    B_shifted.reserve(B.size());
    for (int b : B)
        B_shifted.append(b + offset);

    // 再次求交集：A 与偏移后的 B
    QSet<int> setB_shifted;
    for (int b : B_shifted)
        setB_shifted.insert(b);

    QSet<int> intersection2;
    for (int a : A) {
        if (setB_shifted.contains(a))
            intersection2.insert(a);
    }

    QVector<int> result2 = intersection2.values().toVector();
    std::sort(result2.begin(), result2.end());
    return result2;

    // 得到交集后，还原 B 的偏移，即交集中每个元素都减去 offset
    // QVector<int> finalResult;
    // for (int x : intersection2)
    //     finalResult.append(x - offset);

    // std::sort(finalResult.begin(), finalResult.end());
    // return finalResult;
}

INT32 MeasureControl::ChangeToEncoderValue(float position)
{
    return position * 2000;
}

float MeasureControl::ChangeToPositionValue(INT32 encoder)
{
    return qRound(encoder / 2000.0 * 100) / 100.0f;
}


//寻找x索引
std::vector<int> MeasureControl::get_Xindex(const std::string& filePath) {
    std::vector<int> thirdColumn;
    std::ifstream file(filePath);

    if (!file.is_open()) {
        std::cerr << "错误: 无法打开文件 " << filePath << std::endl;
        return thirdColumn;
    }

    std::string line;
    size_t lineNum = 0; // 添加行号计数器
    while (std::getline(file, line)) {
        lineNum++;
        std::stringstream ss(line);
        std::string cell;
        int columnCount = 0;

        // 查找第三列
        while (std::getline(ss, cell, ',')) {
            if (++columnCount == 3) {
                try {
                    // 执行安全数值转换
                    int value = std::stod(cell);
                    thirdColumn.push_back(value);
                }
                catch (const std::invalid_argument& e) {
                    std::cerr << "行 " << lineNum << ": 无效数值 '" << cell << "'" << std::endl;
                }
                catch (const std::out_of_range& e) {
                    std::cerr << "行 " << lineNum << ": 数值超出范围 '" << cell << "'" << std::endl;
                }
                break;
            }
        }

        // 列数不足处理
        if (columnCount < 3) {
            std::cerr << "行 " << lineNum << ": 缺少第三列数据" << std::endl;
        }
    }

    file.close();
    return thirdColumn;
}


//读取重力补偿值
QMap<int, double> MeasureControl::get_zg(const std::string& filePath) {
    QMap<int, double> zg_map;
    std::ifstream file(filePath);

    if (!file.is_open()) {
        std::cerr << "错误: 无法打开文件 " << filePath << std::endl;
        return zg_map;
    }

    std::string line;
    size_t lineNum = 0;

    int key;
    double value;

    while (std::getline(file, line)) {
        lineNum++;
        std::stringstream ss(line);
        std::string cell;
        int columnCount = 0;

        // 查找第一列（列号从1开始计数）
        while (std::getline(ss, cell, ',')) {
            if (columnCount == 0) {  // 修改判断条件为第一列
                try {
                    value = std::stod(cell);
                }
                catch (const std::invalid_argument& e) {
                    std::cerr << "行 " << lineNum << ": 无效数值 '" << cell << "'" << std::endl;
                }
                catch (const std::out_of_range& e) {
                    std::cerr << "行 " << lineNum << ": 数值超出范围 '" << cell << "'" << std::endl;
                }
            }
            else if (columnCount == 1) {  // 修改判断条件为第一列
                try {
                    key = std::stoi(cell);
                }
                catch (const std::invalid_argument& e) {
                    std::cerr << "行 " << lineNum << ": 无效数值 '" << cell << "'" << std::endl;
                }
                catch (const std::out_of_range& e) {
                    std::cerr << "行 " << lineNum << ": 数值超出范围 '" << cell << "'" << std::endl;
                }
            }
            
            columnCount++;
        }

        zg_map[key] = value;

        // 检查列数是否足够
        if (columnCount < 2) {
            std::cerr << "行 " << lineNum << ": 缺少第一列数据" << std::endl;
        }
    }

    file.close();
    return zg_map;
}

void MeasureControl::convertCsvToBinary(const QString &csvFile, const QString &binFile)
{
    QFile file(csvFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << u8"无法打开 CSV 文件：" << csvFile;
        return;
    }

    QFile bin(binFile);
    if (!bin.open(QIODevice::WriteOnly)) {
        qWarning() << u8"无法创建二进制文件：" << binFile;
        return;
    }

    QTextStream in(&file);
    QDataStream out(&bin);
    out.setVersion(QDataStream::Qt_5_15);  // 确保跨版本兼容

    while (!in.atEnd()) {
        QString line = in.readLine();  // 读取一行 CSV
        QStringList fields = line.split(",");  // 按逗号分割
        out << fields;  // 直接存入二进制文件
    }

    file.close();
    bin.close();
    qDebug() << u8"CSV 转换为二进制成功：" << binFile;
}

QMap<int, double> MeasureControl::readBinaryCsv(const QString &binFile)
{
    QMap<int, double> zg_map;

    QFile file(binFile);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << u8"无法打开二进制文件：" << binFile;
        return zg_map;
    }

    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_5_15);

    while (!file.atEnd()) {
        QStringList fields;
        in >> fields;  // 直接读取
        zg_map[fields.last().toInt()] = fields.first().toDouble();
        // qDebug() << fields;  // 输出数据
    }

    file.close();

    return zg_map;
}


WarpResult MeasureControl::ManualWarpAlg(const std::vector<DataPoint>& nor_data, std::vector<double>& z_gravity,double center_x, double center_y, double D, int window_size)
{
    WarpResult warpResult;
    warpResult.BOW = invalidAlgorithmValue();
    warpResult.WARP = invalidAlgorithmValue();
    warpResult.CENTER_THK = invalidAlgorithmValue();
    warpResult.AVERAGE_THK = invalidAlgorithmValue();
    warpResult.TTV = invalidAlgorithmValue();
    warpResult.SORI = invalidAlgorithmValue();

    Wafer::WaferDataset dataset = datasetFromLegacyLines(
        m_vctAllDataPointByLine, m_vctAllZGravityByLine, nor_data, z_gravity,
        m_measurePath, m_productSize, center_x, center_y, D);
    Wafer::AlgorithmOptions options = legacyAlgorithmOptions(
        m_measurePath, m_productSize, LineSampleNum, Radius, center_x, center_y,
        D, m_productThickness, window_size);

    Wafer::WaferAlgorithm algorithm;
    const Wafer::AlgorithmResult result = algorithm.runAll(dataset, options);
    for (const QString &log : result.logs) {
        emit s_writeLog(log);
    }
    if (!result.success) {
        emit s_sendErrorMsg(0, result.errorMessage);
        emit s_writeLog(result.errorMessage);
        return warpResult;
    }

    warpResult.BOW = result.hasBow ? result.bow : invalidAlgorithmValue();
    warpResult.WARP = result.hasWarp ? result.warp : invalidAlgorithmValue();
    warpResult.CENTER_THK = result.hasCenterThk ? result.centerThk : invalidAlgorithmValue();
    warpResult.AVERAGE_THK = result.hasAverageThk ? result.averageThk : invalidAlgorithmValue();
    warpResult.TTV = result.hasTtv ? result.ttv : invalidAlgorithmValue();
    warpResult.SORI = result.hasSori ? result.sori : invalidAlgorithmValue();
    return warpResult;
}

double MeasureControl::Calculate_Bow(const std::vector<DataPoint>& nor_data, std::vector<double>& z_gravity,double center_x, double center_y)
{
    Wafer::WaferDataset dataset = datasetFromLegacyLines(
        m_vctAllDataPointByLine, m_vctAllZGravityByLine, nor_data, z_gravity,
        m_measurePath, m_productSize, center_x, center_y, m_currentCalibrationInfo.total);
    Wafer::AlgorithmOptions options = legacyAlgorithmOptions(
        m_measurePath, m_productSize, LineSampleNum, Radius, center_x, center_y,
        m_currentCalibrationInfo.total, m_productThickness, 40000);

    Wafer::WaferAlgorithm algorithm;
    const Wafer::AlgorithmResult result = algorithm.runAll(dataset, options);
    if (!result.success || !result.hasBow) {
        const QString error = result.errorMessage.isEmpty() ? QStringLiteral("Calculate_Bow failed.") : result.errorMessage;
        emit s_sendErrorMsg(0, error);
        emit s_writeLog(error);
        return invalidAlgorithmValue();
    }
    return result.bow;
}



//计算八线 每一根线的bow
double MeasureControl::CalcuBowLine(std::vector<double> x_local, std::vector<double> y_local, std::vector<double> zm_local, float D, int PointNum, double& Bow, int SampleNum)
{
    Q_UNUSED(PointNum);
    std::vector<double> thickness(zm_local.size(), 1.0);
    Wafer::WaferDataset dataset = datasetFromZmLine(x_local, y_local, zm_local, thickness);
    Wafer::AlgorithmOptions options = legacyAlgorithmOptions(
        1, m_productSize, SampleNum, D / 2.0, CircleCenter_X, CircleCenter_Y,
        m_currentCalibrationInfo.total, m_productThickness, 40000);

    Wafer::WaferAlgorithm algorithm;
    const Wafer::AlgorithmResult result = algorithm.runAll(dataset, options);
    if (!result.success || !result.hasBow) {
        const QString error = result.errorMessage.isEmpty() ? QStringLiteral("CalcuBowLine failed.") : result.errorMessage;
        emit s_sendErrorMsg(0, error);
        emit s_writeLog(error);
        return -1.0;
    }
    Bow = result.bow;
    return Bow;
}


//计算八线 单线的warp
int  MeasureControl::CalculWarpSingleLine(std::vector<double> x, std::vector<double> y, std::vector<double> ZM, std::vector<double> Thk, double& Warp, int PointNum, int SampleNum, std::vector<double>& out_x, std::vector<double>& out_y, std::vector<double>& out_ZmFit,std::vector<double>& out_Thk)
{
    Q_UNUSED(PointNum);
    Wafer::WaferDataset dataset = datasetFromZmLine(x, y, ZM, Thk);
    Wafer::AlgorithmOptions options = legacyAlgorithmOptions(
        1, m_productSize, SampleNum, Radius, CircleCenter_X, CircleCenter_Y,
        m_currentCalibrationInfo.total, m_productThickness, 40000);

    Wafer::WaferAlgorithm algorithm;
    const Wafer::AlgorithmResult result = algorithm.runAll(dataset, options);
    if (!result.success || !result.hasWarp) {
        const QString error = result.errorMessage.isEmpty() ? QStringLiteral("CalculWarpSingleLine failed.") : result.errorMessage;
        emit s_sendErrorMsg(0, error);
        emit s_writeLog(error);
        return -1;
    }
    Warp = result.warp;
    const int copyCount = qMin(qMin(qMin(static_cast<int>(out_x.size()), static_cast<int>(out_y.size())),
                                    static_cast<int>(out_ZmFit.size())),
                               qMin(static_cast<int>(out_Thk.size()), static_cast<int>(x.size())));
    for (int i = 0; i < copyCount; ++i) {
        out_x[i] = x[i];
        out_y[i] = y[i];
        out_ZmFit[i] = ZM[i];
        out_Thk[i] = i < static_cast<int>(Thk.size()) ? Thk[i] : 0.0;
    }
    return 0;
}



//计算八线总的Warp
int  MeasureControl::CalculWarpAllLine(std::vector<double> x, std::vector<double> y, std::vector<double> ZM, std::vector<double> Thk, double& Warp, double& Sori,int DataCount, int NumLine, int SampleNum)
{
    Q_UNUSED(DataCount);
    Wafer::WaferDataset dataset;
    dataset.lineCount = NumLine;
    dataset.productSize = m_productSize;
    dataset.centerX = CircleCenter_X;
    dataset.centerY = CircleCenter_Y;
    dataset.calibrationTotal = m_currentCalibrationInfo.total;
    dataset.nominalThickness = m_productThickness;
    dataset.containsZGravity = true;

    const int safeNumLine = qMax(1, NumLine);
    const int pointsPerLine = qMax(1, static_cast<int>(x.size()) / safeNumLine);
    int index = 0;
    for (int lineIndex = 0; lineIndex < safeNumLine && index < static_cast<int>(x.size()); ++lineIndex) {
        Wafer::LineData line;
        line.lineIndex = lineIndex;
        for (int j = 0; j < pointsPerLine && index < static_cast<int>(x.size()); ++j, ++index) {
            Wafer::DataPoint point;
            point.x = x.at(index);
            point.y = index < static_cast<int>(y.size()) ? y.at(index) : 0.0;
            point.zm = index < static_cast<int>(ZM.size()) ? ZM.at(index) : invalidAlgorithmValue();
            point.thickness = index < static_cast<int>(Thk.size()) ? Thk.at(index) : invalidAlgorithmValue();
            point.hasZm = qIsFinite(point.zm);
            point.hasThickness = qIsFinite(point.thickness);
            point.lineIndex = lineIndex;
            line.points.append(point);
        }
        dataset.lines.append(line);
    }

    Wafer::AlgorithmOptions options = legacyAlgorithmOptions(
        safeNumLine, m_productSize, SampleNum, Radius, CircleCenter_X, CircleCenter_Y,
        m_currentCalibrationInfo.total, m_productThickness, 40000);

    Wafer::WaferAlgorithm algorithm;
    const Wafer::AlgorithmResult result = algorithm.runAll(dataset, options);
    if (!result.success || !result.hasWarp) {
        const QString error = result.errorMessage.isEmpty() ? QStringLiteral("CalculWarpAllLine failed.") : result.errorMessage;
        emit s_sendErrorMsg(0, error);
        emit s_writeLog(error);
        return -1;
    }
    Warp = result.warp;
    Sori = result.hasSori ? result.sori : invalidAlgorithmValue();
    return 0;
}
void MeasureControl::FitZM3D(std::vector<double> x, std::vector<double> y, std::vector<double> ZM, int point_num)
{
    if (false) 
    {
         QString saveCSVFile = "SurfacePointCloud.csv";
      QFile file(saveCSVFile);


    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream CSVout(&file);
        for (int i = 0; i < point_num; i++)
        {

              CSVout << QString("%1,%2,%3").arg(x[i]).arg(y[i]).arg(ZM[i]) << "\n";
        }

          }
          file.close();
    }
    SurfacePointCloud_Xs.clear();
    SurfacePointCloud_Ys.clear();
    SurfacePointCloud_ZMs.clear();

    if (point_num <= 0 ||
        point_num > static_cast<int>(x.size()) ||
        point_num > static_cast<int>(y.size()) ||
        point_num > static_cast<int>(ZM.size())) {
        const QString error = QStringLiteral("FitZM3D failed, invalid point count.");
        emit s_sendErrorMsg(0, error);
        emit s_writeLog(error);
        return;
    }

    if (!HandelDll) {
        QDir appDir = QCoreApplication::applicationDirPath();
        const QString algDllPath = QDir::toNativeSeparators(appDir.filePath("AlgApi.dll"));
        HandelDll = LoadLibraryW(reinterpret_cast<LPCWSTR>(algDllPath.utf16()));
        if (!HandelDll) {
            const QString error = QString("FitZM3DWithContour failed, load AlgApi.dll failed, winError=%1").arg(GetLastError());
            emit s_sendErrorMsg(0, error);
            emit s_writeLog(error);
            return;
        }
    }

    FitZM3DWithContour dllfunc_new = (FitZM3DWithContour)GetProcAddress(HandelDll, "FitZM3DWithContour");
    if (!dllfunc_new) {
        const QString error = QString("FitZM3DWithContour failed, function not found, winError=%1").arg(GetLastError());
        emit s_sendErrorMsg(0, error);
        emit s_writeLog(error);
        return;
    }

    int maxSampleNum = theta_Points* r_Points;
    std::vector<double> out_Xs(maxSampleNum + 1);
    std::vector<double> out_Ys(maxSampleNum + 1);
    std::vector<double> out_ZMs(maxSampleNum + 1);
    std::vector<double> contour_X(200), contour_XZ(200), contour_Y(200), contour_YZ(200);

    if (dllfunc_new(x.data(), y.data(), ZM.data(), out_Xs.data(), out_Ys.data(), out_ZMs.data(), contour_X.data(), contour_XZ.data(), contour_Y.data(), contour_YZ.data(), point_num, Radius* 1000.0, radiusCut, 200, theta_Points, r_Points) != 0)
    {
        const QString error = QStringLiteral("FitZM3DWithContour failed.");
        emit s_sendErrorMsg(0, error);
        emit s_writeLog(error);
        return;
    }

    for (int i = 0; i < maxSampleNum; i++)
    {
        SurfacePointCloud_Xs.push_back(out_Xs[i]);
        SurfacePointCloud_Ys.push_back(out_Ys[i]);
        SurfacePointCloud_ZMs.push_back(out_ZMs[i]);
    }

        //写文件
        QDir appDir = QCoreApplication::applicationDirPath();
        QString dirName = QString("ZGravityFile/type%1lines/%2/%3/ZGravity/ZMDatas/"+ProductName).arg(m_measurePath).arg(m_productSize).arg(m_productThickness);
        QString ZGravityFileFilePath = appDir.filePath(dirName);
        QDir dirZGravity(ZGravityFileFilePath);
        if (!dirZGravity.exists())
        {
            // 目录不存在，创建它
            if (dirZGravity.mkpath(".")) // "." 表示创建当前目录
            {
                qDebug() << "成功创建目录:" << ZGravityFileFilePath;
            }
            else
            {
                qDebug() << "创建目录失败:" << ZGravityFileFilePath;
                const QString error = QString("FitZM3DWithContour failed, create directory failed: %1").arg(ZGravityFileFilePath);
                emit s_sendErrorMsg(0, error);
                emit s_writeLog(error);
                return;
            }
        }

        {
            QString fileCSVName = QString("/ZM_X.csv");
            QString saveCSVFile = appDir.filePath(dirName + fileCSVName);
            QFile file(saveCSVFile);
            /* 以文本方式写入*/
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream CSVout(&file);
                for (size_t i = 0; i < contour_X.size(); i++)
                {
                    CSVout << QString("%1,%2").arg(contour_X[i]).arg(contour_XZ[i]) << "\n";
                }
            }
            file.close();
        }

        {
            QString fileCSVName = QString("/ZM_Y.csv");
            QString saveCSVFile = appDir.filePath(dirName + fileCSVName);
            QFile file(saveCSVFile);
            /* 以文本方式写入*/
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream CSVout(&file);
                for (size_t i = 0; i < contour_X.size(); i++)
                {
                    CSVout << QString("%1,%2").arg(contour_Y[i]).arg(contour_YZ[i]) << "\n";
                }
            }
            file.close();
        }

     
}



