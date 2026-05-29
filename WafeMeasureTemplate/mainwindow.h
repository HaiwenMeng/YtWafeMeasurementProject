#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "colorFocus_control.h"
#include "motion_control.h"
#include "plot.h"
#include "log_widget.h"
#include "serial_communicator.h"
#include "PermissionManager/usermanagementdialog.h"  //该引用一定要放在measure_control.h之前，否则编译报错，可能是宏定义冲突
#include "measure_control.h"
#include "dialog_setting.h"
#include "wid_control.h"
#include "waitingspinnerwidget.h"
#include "RecipeSettingDialog.h"
#include <dialogmeasureparams.h>
#include "dialogtemperaturecontroldevice.h"
#include "chipheatmapviewer.h"
#include "chipsurfacesampleviewer.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

enum SystemState
{
    Ready = 0, //准备就绪
    Calibration = 1, //校准中
    Load = 2, //上下料路途中
    ReadID = 3, //读取ID中
    Measure = 4 //测量中
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent* event) override;

public slots:
    // 功能控制区
    void on_pushButton_init_clicked(); //初始化
    void on_pushButton_setting_clicked(); //参数设置
   
    void on_pushButton_module_eSotp_clicked(); //急停

    //共聚焦校准区
    void on_pushButton_calibration_clicked(); //校准
    void on_pushButton_standard_part_1_clicked(); //标准件1点击
    void on_pushButton_standard_part_2_clicked(); //标准件2点击
    void on_pushButton_standard_part_3_clicked(); //标准件3点击
    void on_pushButton_standard_part_4_clicked(); //标准件4点击

    //绝对位置移动区
    void on_pushButton_moveToPos_x_clicked();
    void on_pushButton_moveToPos_y_clicked();
    void on_pushButton_moveToPos_z_clicked();
    void on_pushButton_linkage_clicked(); //XY轴联动
    void on_pushButton_zLock_clicked(); //Z轴锁定

    //XY轴单步相对运动
    void on_pushButton_xy_moveUp_clicked();
    void on_pushButton_xy_moveDown_clicked();
    void on_pushButton_xy_moveLeft_clicked();
    void on_pushButton_xy_moveRight_clicked();

    //XY轴持续运动
    // void on_pushButton_xy_moveUp_continuous_clicked();
    // void on_pushButton_xy_moveDown_continuous_clicked();
    // void on_pushButton_xy_moveLeft_continuous_clicked();
    // void on_pushButton_xy_moveRight_continuous_clicked();

    //XY轴速度调节
    void on_pushButton_xy_velocity_reduce_clicked();
    void on_pushButton_xy_velocity_increase_clicked();

    //Z轴单步相对运动
    void on_pushButton_z_moveUp_clicked();
    void on_pushButton_z_moveDown_clicked();

    //Z轴持续运动
    // void on_pushButton_z_moveUp_continuous_clicked();
    // void on_pushButton_z_moveDown_continuous_clicked();

    //Z轴速度调节
    void on_pushButton_z_velocity_reduce_clicked();
    void on_pushButton_z_velocity_increase_clicked();

    void InitAxisMoveButtonFunc();

    //测量流程区
    void on_pushButton_loading_clicked(); //上下料
    void on_pushButton_readID_clicked(); //读取ID
    void on_pushButton_measure_start_clicked(); //开始测量
    void on_pushButton_measure_stop_clicked(); //停止测量


    //彩色共聚焦相关槽函数
    void updateColorFocusValueToView(UINT16 sensor_id, float distance, float intensity); //更新彩色共聚焦实时值至界面控件
    void showErrorMsg(int sensor_id, QString lastErrorMsg); //显示彩色共聚焦错误信息
    void updateConnectionState(UINT16 sensor_id, bool isConnected); //更新彩色共聚焦连接状态

    //轴相关槽函数
    void updateAxisConnectinonStatusToView(bool isConnected);
    void updateAxis_PositionToView(AxisPos axisPos);
    void updateAxisStateToView(AxisState axisState);

    void OnMeasureOver(double BOW, double WARP, double CENTER_THK, double AVERAGE_THK, double TTV, double SORI);
    void UpdateMeasurePathFlag(bool bIsMeasurePath);

    void UpdateIsHomeState();
    void OnInitOver();

    void updateWIDConnectinonStatusToView(bool isConnected);
    void OnUpdateWaferID(QString waferID);
    void OnUpdatePlot(int pathIndex);
    void OnOpdateStandardThicknessToMainView();

    void CheckDevicePowerOff();
    void OnStateNotReadyWarning();

    void OnCheckDogResult(bool checkResult);

    void WriteWaferIdToFile(QString waferID);
    bool SavePaintImageToFile();
    QSet<QString> ReadWaferIdToSet(QFile& file);

signals:
    void s_writeLog(QString logStr);
    void s_stopMeasure();
    void s_eStop();
    // void s_calibration(int standard_id, CalibrationInfo calibrationInfo);
    void s_axis_pos(AxisPos axisPos);
    void s_axis_state(AxisState axisState);

private slots:
    void on_pushButton_clearError_clicked();

    void on_pushButton_setXY_velocity_clicked();

    void on_pushButton_setZ_velocity_clicked();

    void on_comboBox_path_currentIndexChanged(int index);

    void onLegendHover(QMouseEvent *event);

    void onPermissionsChanged();
    void showUserManagement();
    void showPermissionConfig();
    void changePassword();

    void on_pushButton_userManage_clicked();
   

    void on_comboBox_size_currentIndexChanged(int index);

    void on_pushButton_repiceSetting_clicked();



    void on_comboBox_recipe_currentIndexChanged(int index);

    void on_comboBox_thickness_currentIndexChanged(int index);





    void on_checkBox_changedRecipe_clicked(bool checked);

   

    void on_pushButton_ScanGravity_clicked();

    void on_checkBox_useRepeatEpochs_clicked(bool checked);



    void on_checkBox_changedRecipe_checkStateChanged(const Qt::CheckState &arg1);

    void on_checkBox_useRepeatEpochs_checkStateChanged(const Qt::CheckState &arg1);

   

    void on_pushButton_save_surface_clicked();

private:
    void setWidgetBackground(QWidget* widget, const QColor& color);
    void intComboBox(QString defaultSelectRecipeName);
    void InitColorFocusDisplayWidget();
    void InitLogWidget();
    void InitPlotWidget();
    void InitChipSurfaceSampleViewer();
    void ExportMeasureData();
    void JudgeAxisPresetPosition();
    // 自定义比较函数
    bool fuzzyEqual(double a, double b, double epsilon = 1e-3);
    void waitInPos(QPointF dstPosition);
    void waitInPos_z(double dstPosition);

    void UpdateResultTable(double BOW, double WARP, double CENTER_THK, double AVERAGE_THK, double TTV, double SORI);
    bool CheckValue(MeasureValue& rangeValue, double value, QTableWidgetItem* item);
    void WriteResultToExcel(WarpResult result);
    void WriteRowContent(QString fileName, WarpResult result);
    //滤波函数
    double filterValue(double newValue, int windowSize = 5);
    void StartMeasure();
    void ReadID();
    void GoLoadPostion();

    void DisconnectAndClean();
    void InitAllDevice();

    bool CheckSystemStateReady();

private:
    Ui::MainWindow *ui;
    std::shared_ptr<LogWidget> m_log_widget{nullptr};

    std::shared_ptr<CustomPlotWidget> m_colorFocus_plot_widget_top{nullptr};
    std::shared_ptr<QVBoxLayout> m_colorFocus_plot_widget_top_layout{nullptr};
    std::shared_ptr<CustomPlotWidget> m_colorFocus_plot_widget_bottom{nullptr};
    std::shared_ptr<QVBoxLayout> m_colorFocus_plot_widget_bottom_layout{nullptr};

    MeasureControl* m_pMeasureControl = nullptr;
    QThread* m_pMeasureControlThread = nullptr;

    ChipHeatMapViewer* m_ChipHeatMapViewer = nullptr;
    ChipSurfaceSampleViewer* m_ChipSurfaceSampleViewer = nullptr;

    QSharedPointer<ColorFocusControl> m_pColorFocusControl_top = nullptr;
    QSharedPointer<ColorFocusControl> m_pColorFocusControl_bottom = nullptr;
    QThread* m_pColorFocusControllThread = nullptr;

    QSharedPointer<MotionController> m_pMotionController = nullptr;

    QSharedPointer<WIDControl> m_pWIDController = nullptr;

    // QSharedPointer<QThread> m_pSerialThread = nullptr;
    QSharedPointer<SerialCommunicator> m_pSerialCommunicator = nullptr;

    QString startTriggerCommand = R"({"interval":49})";
    QString stopTriggerCommand = R"({"interval":0})";

    bool m_bSeriaCommnReturnFlag = false;

    DialogSetting m_dialogSetting;

    RecipeSettingDialog m_recipeSettingDialog;

    DialogMeasureParams m_MeasureParamsDialog;

    UserManagementDialog m_dialogUser;

    DialogTemperatureControlDevice m_DialogTemperatureControlDevice;

    QString connection_stateGreen = QString(
        "background-color: rgb(0, 255, 0);"
        "border-radius:25px;");

    QString connection_stateYellow = QString(
        "background-color: rgb(255, 219, 111);"
        "border-radius:25px;");

    QString connection_stateRed = QString(
        "background-color: rgb(255, 0, 0);"
        "border-radius:25px;");

    QString axis_stateGreen = QString(
        "background-color: rgb(0, 255, 0);"
        "border-radius:12px;");

    QString axis_stateYellow = QString(
        "background-color: rgb(255, 219, 111);"
        "border-radius:12px;");

    QString axis_stateRed = QString(
        "background-color: rgb(255, 0, 0);"
        "border-radius:12px;");

    QString preset_stateGreen = QString(
        "background-color: rgb(0, 255, 0);"
        "border-radius:5px;");

    QString preset_stateYellow = QString(
        "background-color: rgb(255, 219, 111);"
        "border-radius:5px;");

    QString preset_stateRed = QString(
        "background-color: rgb(255, 0, 0);"
        "border-radius:5px;");

    const QVector<QColor> m_plotColors = {  // 预定义颜色列表
        QColor(255, 0, 0),     // 红
        QColor(0, 150, 0),     // 绿
        QColor(0, 0, 255),     // 蓝
        QColor(255, 128, 0),   // 橙
        QColor(128, 0, 255),   // 紫
        QColor(0, 128, 128)    // 青
    };

    QPair<bool, QString> m_errorInfo_x = qMakePair<bool, QString>(false, "No error");
    QPair<bool, QString> m_errorInfo_y = qMakePair<bool, QString>(false, "No error");
    QPair<bool, QString> m_errorInfo_z = qMakePair<bool, QString>(false, "No error");

    QMap<int, QString> m_errorMap;

    bool m_bXYLinkage = false;
    bool m_bZLock = true;



    volatile bool IsXBusying=false;
    volatile bool IsYBusying = false;
    volatile bool IsZBusying = false;
    AxisPos m_axisPos;
    AxisState m_axisState;

    double m_current_distance_top = 0.0;
    double m_current_distance_bottom = 0.0;

    bool m_bStopJudgePresetPos = true;

    QMap<int, CalibrationInfo> m_mapCalibrationInfo;
    CalibrationInfo m_currentCalibrationInfo;
    double m_realtimeThickness = 0.0;

    QMap<int, int> m_mapUsedStandard;

    bool m_bMeasureOver = true;

    bool m_bIsMeasurePath = false;

    bool m_bEncoderClear = true;
    bool m_XbEncoderClear = true;
    bool m_YbEncoderClear = true;
    QString m_currentWaferID;

    bool m_bIsCalibrating = false; //正在校准标识

    //滤波函数使用
    QVector<double> m_dataBuffer;
    int m_windowSize = 5; //窗口大小
    int m_windowSize_plot = 20; //窗口大小

    //标准片校准补偿值
    double m_standardCompensationValue = 0.0;

    bool m_bIsInit_xy = false;
    bool m_bIsInit_z = false;
    bool m_bIsLoading = false;
    bool m_bReadID = false;
    bool m_bMeasuring = false;
    bool m_bIsInitFinished_xy = false;
    bool m_bIsInitFinished_z = false;
    bool m_IsLinkedFail = false;
    bool m_bIsCloseEvent = false;
    bool m_bIsColorFocusDisconnected_top = false;
    bool m_bIsColorFocusDisconnected_bottom = false;
    bool m_bIsMotionDissconnected = false;
    bool m_bIsWIDDissconnected = false;
    bool IsAxisHomeMoving = false;
    


    bool m_bIsCalAfterInit = true;

    WaitingSpinnerWidget* m_spinner = nullptr;

    SystemState m_currentSystemState = SystemState::Ready;
    MeasureMode ProgramMeasureMode = NormalMeasure;
    bool m_bCheckResult = false;

    QCPPlottableLegendItem *lastHighlightedItem = nullptr;

    double m_wait_pos_x = -1;
    double m_wait_pos_y = -1;


    void setupConnections();
    void updatePermissions();

    bool IsRecipChanged = false;
    
    void CheckRecipeState();

    void CheckZGravityState();

    QString LastCurrentLot="";
 
    void SetControlStyle();
    void SaveControlToImage(QPixmap pixmap);
    Q3DSurface* ChipSurfaceGraph;
    QWidget* ChipSurfaceContainer;
    QCustomPlot* colorBarPlot;
    QCPColorScale* colorScale;
    void PaintChipPointClouds();

    QString DefaultSelectRecipeName = "";
    int lastPathIndex = -1;
    int lastThicknessIndex = -1;
    int lastSizeIndex = -1;
};
#endif // MAINWINDOW_H
