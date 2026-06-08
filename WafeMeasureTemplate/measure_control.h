#ifndef MEASURE_CONTROL_H
#define MEASURE_CONTROL_H

#include <QObject>
#include "colorFocus_control.h"
#include "serial_communicator.h"
#include <QSharedPointer>
#include "paramsettings.h"
#include "motion_control.h"
#include "ManualWarpAlg.h"
#include <algorithm>  // std::max_element
#include <cmath>
#include <stdexcept>


enum ProcessType
{
    None = 0, //不处理
    ClearStack = 1, //清传感器数据缓存
    ReadDistance =2 //读传感器数据
};
struct  PointZM
{
    double x;
    double y;
    double z;
    PointZM(double x_, double y_, double z_)
    {
        x = x_;
        y = y_;
        z = z_;
    }
};
struct PathInfo
{
    QPointF start;
    QPointF end;
    int triggerAxis;
    double triggerInterval;
};
Q_DECLARE_METATYPE(PathInfo);

struct CalibrationInfo
{
    double topSurface;
    double bottomSurface;
    double standard;
    double total;
};
Q_DECLARE_METATYPE(CalibrationInfo);

struct WaferType
{
    QString typeLines; //6线/8线
    int size; //尺寸（8寸/12寸）
    int thickness; //厚度

    // 重载 operator< 以便 QMap 能够对键进行排序
    bool operator<(const WaferType &other) const {
        if (typeLines != other.typeLines) return typeLines < other.typeLines;  // 先比较 key1
        if (size != other.size) return size < other.size;
        return thickness < other.thickness;  // 若 key1 相同，再比较 key2
    }
};
Q_DECLARE_METATYPE(WaferType);

class MeasureControl : public QObject
{
    Q_OBJECT
public:
    explicit MeasureControl( ParamSettings &paramSettings, QObject *parent = nullptr);
    ~MeasureControl();

    inline void SetColorFocusPointer(QSharedPointer<ColorFocusControl> colorFocusPtr_1, QSharedPointer<ColorFocusControl> colorFocusPtr_2) {
        m_pColorFocusControl_top = colorFocusPtr_1;
        m_pColorFocusControl_bottom = colorFocusPtr_2;
    };

    inline QSharedPointer<ColorFocusControl> GetTopColorFocus() {return m_pColorFocusControl_top;}
    inline QSharedPointer<ColorFocusControl> GetBottomColorFocus() {return m_pColorFocusControl_bottom;}

    inline void SetSerialCommPointer(QSharedPointer<SerialCommunicator> serialCommunicator) {
        m_pSerialCommunicator = serialCommunicator;
    }

    inline void SetMotionControlPointer(QSharedPointer<MotionController> motionController) {
        m_pMotionController = motionController;
    }

    bool SetMeasurePara(int measurePath, int productThickness, int productSize);

    inline void SetCalibrationInfo(CalibrationInfo calibrationInfo){
        m_currentCalibrationInfo = calibrationInfo;
    }

    inline std::vector<std::vector<DataPoint>>& GetLineDataVec(){ return m_vctAllDataPointByLine; }

    inline QString GetMeasurePath() { return m_measurePath; }
    inline QString GetProductSize() { return m_productSize; }
    inline QString GetProductThickness() { return m_productThickness; }

    void InitTimer();
    void InitConnectinon();
    MeasureMode ProgramMeasureMode = NormalMeasure;
    
    int MeasureAllStep = 0;   //测量总进度
    int NowStep = 0;           //当前测量进度

    Q_INVOKABLE void StartMeasure(MeasureMode mode= NormalMeasure);
    void StopMeasure();
    void handleTimeout_1();
    void handleTimeout_2();

    void StartTrigger();

    void ReadAllZGravityFile();

    void ResetTrimSize(QString trimSize);

    void InitACQZGravity();
    void CalAndSaveZGravity();
    void SaveZMsDatas(std::vector<double> Xs, std::vector<double> Ys, std::vector<double> ZMs, int index);
    int RepeatEpochs = 1;
    int NowEpoch = 0;
    bool IsNewWarpAlg = true;
    bool IsNewBowAlg = true;
    bool IsSaveZM = false;
    bool IsSaveImage3D = false;
    QString ProductName = "Unknow";
    void SetLineSampleParams(int lineSampleNum);
    
    std::vector<double> SurfacePointCloud_Xs;
    std::vector<double> SurfacePointCloud_Ys;
    std::vector<double> SurfacePointCloud_ZMs;
    double Radius = 150.0; 
    double radiusCut = 100.0; 
    int theta_Points = 720; 
    int r_Points = 400;
    bool IsDebug = true;
    double ChordLength = 0.0;
    std::vector<std::vector<DataPoint>> vctTopDataPointByLine; //所有线的点vector(按线存)(上表面)(调试算法用)
    std::vector<std::vector<DataPoint>> vctBottomDataPointByLine; //所有线的点vector(按线存)(下表面)(调试算法用)
    QVector<int>& Get_vecEncoders_top();
    QVector<int>& Get_vecEncoders_bottom();
    void SetCenterPoints(double centerX, double centerY);
public slots:
    void ClearColorFocusStack();
    void GetOneLineDataFromColorFocus();
    void OnUpdateStopFlag();
    void OnupdateEStopFlag();
    void cleanup();



private:
    //angleDeg:极坐标系下的角度；
    //return:轴号（0 为 Encoder1，1 为 Encoder2，2 为Encoder3）
    int closerToAxis(double angleDeg);
    void GeneratePathInfoList(double chordLength = 0.0);
    double getLineAngelDeg(int measurePath, int lineIndex);
    int getTriggerAxis(int measurePath, int lineIndex);
    // 自定义比较函数
    bool fuzzyEqual(double a, double b, double epsilon = 1e-3);
    void waitInPos(QPointF dstPosition);
    double calculateTriggerInterval(const QLineF& line, double intervalDistance, int triggerAxis = 0);
    double calculateTriggerStart(const QLineF& line, int triggerAxis = 0);
    double calculateTriggerEnd(const QLineF& line, int triggerAxis = 0);
    QVector<QPointF> generateTriggerPoints(const QLineF& line, double intervalDistance);
    double filterValue_top(double newValue);
    double filterValue_bottom(double newValue);
    double filterValue_alg_top(double newValue);
    double filterValue_alg_bottom(double newValue);
    QVector<int> findIntersection(const QVector<int>& A, const QVector<int>& B, int step, int& offset);

    void OrganizeData(std::vector<std::vector<DataPoint>>& vctAllDataPointByLine, std::vector<DataPoint>& vctAllDataPoint, int x_interval);
    void OrganizeDataNoIntersection(std::vector<std::vector<DataPoint>>& vctAllDataPointByLine, std::vector<DataPoint>& vctAllDataPoint, int x_interval);
    void OrganizeDataForCalBow(std::vector<DataPoint>& nor_data, std::vector<double>& z_gravity);
    void OrganizeDataSimply(std::vector<std::vector<DataPoint>>& vctAllDataPointByLine, std::vector<DataPoint>& vctAllDataPoint);
    void OrganizeDataForZGravity(std::vector<std::vector<DataPoint>>& vctAllDataPointByLine, std::vector<DataPoint>& vctAllDataPoint, int x_interval,bool x_trigerMode);
    void OrganizeDataMirrorCalibration(std::vector<std::vector<DataPoint>>& vctAllDataPointByLine, std::vector<DataPoint>& vctAllDataPoint);
    void OrganizeDataTopAndBottom(std::vector<std::vector<DataPoint>>& vctTopDataPointByLine, std::vector<std::vector<DataPoint>>& vctBottomDataPointByLine);

    void WriteDistanceDataToFile(std::vector<std::vector<DataPoint>>& vctAllDataPointByLine);
    void WriteMirrorCalibrationDataToFile(std::vector<std::vector<DataPoint>>& vctAllDataPointByLine);
    void WriteTopAndBottomDistanceDataToFile(std::vector<std::vector<DataPoint>>& vctTopDataPointByLine, std::vector<std::vector<DataPoint>>& vctBottomDataPointByLine);
    void SaveLineDistanceMats(const std::vector<DataPoint>& lineData, int lineIndex);

    INT32 ChangeToEncoderValue(float position);
    float ChangeToPositionValue(INT32 encoder);

    void debugError(int offset_top, int offset_bottom, int x_interval, QVector<int> intersectionVec_topAndZG, QVector<int> intersectionVec_bottomAndZG);

    //寻找x索引
    std::vector<int> get_Xindex(const std::string& filePath);
    //读取重力补偿值
    QMap<int, double> get_zg(const std::string& filePath);
    void convertCsvToBinary(const QString &csvFile, const QString &binFile);
    QMap<int, double> readBinaryCsv(const QString &binFile);
    WarpResult ManualWarpAlg(const std::vector<DataPoint>& nor_data, std::vector<double>& z_gravity,double center_x, double center_y, double D, int window_size = 40000);
    double Calculate_Bow(const std::vector<DataPoint>& nor_data, std::vector<double>& z_gravity,double center_x, double center_y);


	//计算八线 每一根线的bow
	double  CalcuBowLine(std::vector<double> x_local, std::vector<double> y_local, std::vector<double> zm_local, float D, int PointNum, double& Bow, int SampleNum);
	
	//计算八线 单线的warp
    int  CalculWarpSingleLine(std::vector<double> x, std::vector<double> y, std::vector<double> ZM, std::vector<double> Thk, double& Warp, int PointNum, int SampleNum, std::vector<double>& out_x, std::vector<double>& out_y, std::vector<double>& out_ZmFit, std::vector<double>& out_Thk);


	//计算八线总的Warp
	int  CalculWarpAllLine(std::vector<double> x, std::vector<double> y, std::vector<double> zm, std::vector<double> Thk, double& Warp, double& Sori, int DataCount, int NumLine, int SampleNum);


    

signals:
    void s_writeLog(QString logStr);
    void s_sendErrorMsg(int sensor_id, QString lastErrorMsg);
    void s_measureOver(double BOW, double WARP, double CENTER_THK, double AVERAGE_THK, double TTV, double SORI);
    void s_isMeasurePathFlagChanged(bool isMeasurePath);
    void s_pathDataOver(int pathIndex);
public:
    QString SaveHeatImageFile;
    QString Save3DImageFile;
private:
    QSharedPointer<ColorFocusControl> m_pColorFocusControl_top = nullptr;
    QSharedPointer<ColorFocusControl> m_pColorFocusControl_bottom = nullptr;
    QThread* m_pColorFocusControllThread = nullptr;

    QSharedPointer<SerialCommunicator> m_pSerialCommunicator = nullptr;
    QSharedPointer<MotionController> m_pMotionController = nullptr;

    ParamSettings &m_paramSettings;

    ProcessType m_processType = ProcessType::None;

    QString startTriggerCommand = R"({"interval":49})";
    QString stopTriggerCommand = R"({"interval":0})";

    QTimer *m_timer_1;
    QTimer *m_timer_2;

    bool m_bStopMeasureFlag = false; //停止测量流程标识
    bool m_bEStopFlag = false; //急停标识

    int m_measurePath = 0; //测量路径
    int m_productThickness = 0.0; //产品厚度
    int m_productSize = 0; //产品尺寸
    int LineSampleNum = 51;


    CalibrationInfo m_currentCalibrationInfo;

    double standard_real_thinkness_1 = 750.0;
    double standard_real_thinkness_2 = 775.0;
    double standard_real_thinkness_3 = 800.0;
    double standard_real_thinkness_4 = 825.0;

    double CircleCenter_X= 142.322;
    double CircleCenter_Y= 151.896;

    QList<PathInfo> m_pathInfoList; //所选测量路径的所有路线信息
    QList<QLineF> m_pathInfoList_trigger; //所选测量路径的所有（触发信息）
    QList<QLineF> m_pathInfoList_move; //所选测量路径的所有（移动信息）

    double m_samplingInterval = 0.02;

    QList< QList<_DISTANCE_VALUE> > m_listAllMeasurePointDistance_top;
    QList< QList<_DISTANCE_VALUE> > m_listAllMeasurePointDistance_bottom;

    int m_currentPathIndex;
    std::vector<std::vector<DataPoint>> m_vctAllDataPointByLine; //所有线的点vector(按线存)
    std::vector<std::vector<double>> m_vctAllZGravityByLine; //所有线的重力补偿值vector(按线存)

    std::vector<QMap<int,double>> NormalAllZGravityByLine; //正面所有线的重力补偿值vector(按线存)
    std::vector<QMap<int,double>> OppsiteAllZGravityByLine; //反面所有线的重力补偿值vector(按线存)
    std::vector<int> XIntervals;

    QMap<WaferType, std::vector<std::vector<int>>> m_mapVecListXIndex;
    QMap<WaferType, std::vector<QMap<int, double>>> m_mapVecListZGravity;

    QMap<int, std::vector<int>> m_currentPathTypeXIndexMap;
    QMap<int, QMap<int, double>> m_currentPathTypeZgravityMap;
    std::vector<double> m_vecAllZGravity;
    QString m_lineDistanceMatRunDir;

    //滤波函数使用
    QVector<double> m_dataBuffer_top;
    QVector<double> m_dataBuffer_bottom;
    int m_windowSize = 5; //窗口大小

    QVector<double> m_dataBuffer_alg_top;
    QVector<double> m_dataBuffer_alg_bottom;
    int m_windowSize_alg = 20; //窗口大小

    int m_offset_top;
    int m_offset_bottom;
    int m_x_interval;
    QVector<int> m_intersectionVec_topAndZG;
    QVector<int> m_intersectionVec_bottomAndZG;

    HINSTANCE HandelDll;
    void FitZM3D(std::vector<double> x, std::vector<double> y, std::vector<double> ZM, int point_num);

};
typedef WarpResult(*fc)(const DataPoint* nor_data,  int n);
typedef WarpResult(*ManualWarpAlgFuction)(const DataPoint* nor_data, double* z_gravity,int n, double center_x, double center_y, double D, int window_size);
typedef double(*Calculate_BowFunction)(const DataPoint* nor_data, double* z_gravity, int n, double center_x, double center_y);

typedef double(*CalcuBowLineFunction)(double*x_local, double*y_local, double*zm_local, float D, int PointNum, double& Bow, int SampleNum);
typedef int(*CalculWarpSingleLineFunction)(double*x, double*y, double*ZM, double*Thk, double& Warp, int PointNum, int SampleNum, double* out_x, double* out_y, double* out_ZmFit, double* out_Thk);
typedef int(*CalculWarpAllLineFunction)(double*x, double*y, double*ZM, double*Thk, double& Warp, double& Sori, int DataCount, int NumLine, int SampleNum);
typedef int(*FitZM3DFunction)(double* x, double* y, double* ZM, int point_num, double* out_x,  double* out_y, double* out_ZM, double Radius, double radiusCut, int theta_Points, int r_Points);
typedef int (*FitZM3DWithContour)(double* x, double* y, double* ZM,
                               double* out_x, double* out_y, double* out_ZM,
                               double* contour_X, double* contour_XZ,    // X方向轮廓线
                               double* contour_Y, double* contour_YZ,     // Y方向轮廓线
                               int point_num, double Radius , double radiusCut , int points_per_line , int theta_Points_Sample, int r_Points_Sample );
#endif // MEASURE_CONTROL_H
