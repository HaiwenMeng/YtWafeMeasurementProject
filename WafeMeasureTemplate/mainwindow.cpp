#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtConcurrent/QtConcurrent>
#include "xlsxdocument.h"
#include "xlsxcell.h"
#include "dialogmessagebox.h"
#include "PermissionManager/currentuser.h"
// #include "PermissionManager/permissionmanager.h"

using namespace QXlsx;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    qRegisterMetaType<UINT16>("UINT16");

    // 设置各区域背景颜色
    setWidgetBackground(ui->verticalWidget_control, QColor(199, 199, 199));
    setWidgetBackground(ui->widget_colorFocus, Qt::white);
    setWidgetBackground(ui->widget_axis, Qt::white);
    setWidgetBackground(ui->widget_measure, Qt::white);
  
    
    setWidgetBackground(ui->verticalWidget_measureResult, QColor(199, 199, 199));
    setWidgetBackground(ui->widget_measureInfo, QColor(235, 235, 235));
    setWidgetBackground(ui->widget_result, Qt::white);
    setWidgetBackground(ui->verticalWidget_log, QColor(199, 199, 199));

    ui->label_27->setVisible(false);
    ui->label_GravityState->setVisible(false);
    /*ui->tab_2->setVisible(false);
    ui->tab_2->hide();
    ui->tabWidget->removeTab(1);*/
    // QPixmap logo(":/image/main/image/logo.png");
    // // 将图片按 QLabel 的尺寸进行缩放，并保持宽高比，同时采用平滑转换提高缩放质量
    // ui->label_logo->setPixmap(logo.scaled(ui->label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));

    QVBoxLayout *layout = new QVBoxLayout(ui->pushButton_measure_start);
    layout->setSpacing(5);  // 图标和文字之间的间距
    layout->setAlignment(Qt::AlignCenter);  // 让整体居中
  
    // 创建 QLabel 作为图标
    QLabel *iconLabel = new QLabel();
    iconLabel->setPixmap(QPixmap(":/image/main/image/kaishirenwu.png").scaled(48, 48, Qt::KeepAspectRatio));
    iconLabel->setAlignment(Qt::AlignCenter);

    // 创建 QLabel 作为文字
    QLabel *textLabel = new QLabel(u8"开始测量");
    textLabel->setAlignment(Qt::AlignCenter);
    textLabel->setStyleSheet(u8"font-family: '微软雅黑'; font-size: 12pt;");

    // 添加到布局
    layout->addStretch();   // 上方留空白
    layout->addWidget(iconLabel);
    layout->addWidget(textLabel);
    layout->addStretch();   // 下方留空白
    ui->pushButton_measure_start->setLayout(layout);
    // ui->pushButton_measure_start->setStyleSheet("QPushButton { border-radius: 10px; }");

    // QString originalStyle = ui->pushButton_loading->styleSheet();  // 获取原样式
    // ui->pushButton_loading->setStyleSheet(originalStyle + "QPushButton { border-radius: 10px; }");
     // 手动连接信号与槽

    ui->checkBox_changedRecipe->setVisible(false);

    if (!m_dialogSetting.initSettings())
    {
        QTimer::singleShot(0, this, [](){
            QMessageBox::critical(nullptr, u8"错误", u8"加载配置文件config.xml失败，请检查！");
            QCoreApplication::exit(-1);
        });
        return;
    }
   
   
    ui->spinBox_repeatEpochs->setEnabled(false);
    
    //初始化下拉框
    intComboBox("");
    //初始化彩色共聚焦曲线控件
    InitColorFocusDisplayWidget();
    //初始化Log控件
    InitLogWidget();
    //初始化曲线控件
    InitPlotWidget();

    setWindowState(Qt::WindowState::WindowMaximized);
   
    //运动控制
    m_pMotionController.reset(new MotionController);
   // connect(m_pMotionController.data(), SIGNAL(s_writeLog(QString)), m_log_widget.get(), SLOT(wirteLog(QString)));
    connect(m_pMotionController.data(), SIGNAL(s_sendErrorMsg(int,QString)), this, SLOT(showErrorMsg(int,QString)));
    connect(m_pMotionController.data(), SIGNAL(connectionStatusChanged(bool)), this, SLOT(updateAxisConnectinonStatusToView(bool)));
    connect(m_pMotionController.data(), &MotionController::connectionTimeout, this, &MainWindow::CheckDevicePowerOff);
    connect(m_pMotionController.data(), &MotionController::s_stateNotReady, this, &MainWindow::OnStateNotReadyWarning);
    connect(m_pMotionController.data(), SIGNAL(s_bIsHomeIng()), this, SLOT(UpdateIsHomeState()));
    connect(m_pMotionController.data(), SIGNAL(s_initOver()), this, SLOT(OnInitOver()));

    connect(m_pMotionController.data(), SIGNAL(s_axis_pos(AxisPos)), this, SLOT(updateAxis_PositionToView(AxisPos)));
    connect(m_pMotionController.data(), SIGNAL(s_axis_state(AxisState)), this, SLOT(updateAxisStateToView(AxisState)));

    InitAxisMoveButtonFunc();

    if (m_dialogSetting.getSettings().IsUseWID.toInt() == 1) {
        //WID控制
        m_pWIDController.reset(new WIDControl);
        connect(m_pWIDController.data(), SIGNAL(s_writeLog(QString)), m_log_widget.get(), SLOT(wirteLog(QString)));
        connect(m_pWIDController.data(), SIGNAL(s_sendErrorMsg(int, QString)), this, SLOT(showErrorMsg(int, QString)));
        connect(m_pWIDController.data(), SIGNAL(connectionStatusChanged(bool)), this, SLOT(updateWIDConnectinonStatusToView(bool)));
        connect(m_pWIDController.data(), SIGNAL(s_waferID(QString)), this, SLOT(OnUpdateWaferID(QString)));
        connect(m_pWIDController.data(), &WIDControl::connectionTimeout, this, &MainWindow::CheckDevicePowerOff);
    }

    connect(&m_dialogSetting, SIGNAL(s_updateStandardThicknessToMainView()), this, SLOT(OnOpdateStandardThicknessToMainView()));

    //测量控制
    m_pMeasureControlThread = new QThread();
    m_pMeasureControl = new MeasureControl(m_dialogSetting.getSettings());

    if (m_dialogSetting.getSettings().IsUseStandard1550Flag.toInt() == 1)
    {
        ui->label_Temp->setVisible(false);
        ui->label_humidity->setVisible(false);
        ui->label_31->setVisible(false);
        ui->label_34->setVisible(false);
        m_pMeasureControl->SetCenterPoints(TW_CircleCenter_X, TW_CircleCenter_Y);
    }
    else
    {
        m_pMeasureControl->SetCenterPoints(NJ_CircleCenter_X, NJ_CircleCenter_Y);
    }

    m_pMeasureControl->moveToThread(m_pMeasureControlThread);
    m_pMeasureControl->SetMotionControlPointer(m_pMotionController);

    connect(m_pMeasureControl, &MeasureControl::s_checkDogResult, this, &MainWindow::OnCheckDogResult);
    connect(m_pMeasureControl, SIGNAL(s_writeLog(QString)), m_log_widget.get(), SLOT(wirteLog(QString)));
    connect(m_pMeasureControl, SIGNAL(s_sendErrorMsg(int,QString)), this, SLOT(showErrorMsg(int,QString)));
    connect(m_pMeasureControl, SIGNAL(s_measureOver(double,double,double,double,double,double)), this, SLOT(OnMeasureOver(double,double,double,double,double,double)));
    connect(this, SIGNAL(s_stopMeasure()), m_pMeasureControl, SLOT(OnUpdateStopFlag()), Qt::DirectConnection);
    connect(this, SIGNAL(s_eStop()), m_pMeasureControl, SLOT(OnupdateEStopFlag()), Qt::DirectConnection);
    connect(m_pMeasureControl, SIGNAL(s_pathDataOver(int)), this, SLOT(OnUpdatePlot(int)));
    connect(m_pMeasureControl, &MeasureControl::s_isMeasurePathFlagChanged, this, &MainWindow::UpdateMeasurePathFlag);
    // 连接线程启动后的初始化操作
    connect(m_pMeasureControlThread, &QThread::started, m_pMeasureControl, &MeasureControl::InitMeasureControl);

    // QObject::connect(m_pMeasureControl, &MeasureControl::s_pathData, this,
    //                  [this](QVector<double> vecThickness) {
    //                      OnUpdatePlot(vecThickness);
                     // });
    // connect(this, SIGNAL(s_calibration(int,CalibrationInfo)), m_pMeasureControl, SLOT(UpdateCalibrationInfo(int,CalibrationInfo)));
    // connect(m_pMeasureControlThread, &QThread::finished, m_pMeasureControl, &QObject::deleteLater);
    // 释放 MeasureControl 避免内存泄漏
    // connect(m_pMeasureControlThread, &QThread::finished, this, [=]() {
    //     if (m_pMeasureControl) {
    //         m_pMeasureControl->deleteLater();
    //     }
    // });
    // connect(m_pMeasureControlThread, &QThread::finished, m_pMeasureControlThread, &QThread::deleteLater);

    // m_pMeasureControlThread->start();

 
    //共聚焦信号连接
    m_pColorFocusControllThread = new QThread();

    m_pColorFocusControl_top.reset(new ColorFocusControl);
    m_pColorFocusControl_bottom.reset(new ColorFocusControl);
    m_pColorFocusControl_top->SetSensoId(SENSOR_ID1);
    m_pColorFocusControl_bottom->SetSensoId(SENSOR_ID2);

    m_pColorFocusControl_top->moveToThread(m_pColorFocusControllThread);
    m_pColorFocusControl_bottom->moveToThread(m_pColorFocusControllThread);
    m_pMeasureControl->SetColorFocusPointer(m_pColorFocusControl_top, m_pColorFocusControl_bottom);

    connect(m_pColorFocusControl_top.data(), SIGNAL(s_writeLog(QString)), m_log_widget.get(), SLOT(wirteLog(QString)));
    connect(m_pColorFocusControl_bottom.data(), SIGNAL(s_writeLog(QString)), m_log_widget.get(), SLOT(wirteLog(QString)));

    connect(m_pColorFocusControl_top.data(), SIGNAL(s_sendErrorMsg(int,QString)), this, SLOT(showErrorMsg(int,QString)));
    connect(m_pColorFocusControl_top.data(), SIGNAL(s_colorFocusUpdated(UINT16,float,float)), this, SLOT(updateColorFocusValueToView(UINT16,float,float)));
    connect(m_pColorFocusControl_top.data(), &ColorFocusControl::s_connectionStateChanged, this, &MainWindow::updateConnectionState);
    connect(m_pColorFocusControl_top.data(), &ColorFocusControl::connectionTimeout, this, &MainWindow::CheckDevicePowerOff);

    connect(m_pColorFocusControl_bottom.data(), SIGNAL(s_sendErrorMsg(int,QString)), this, SLOT(showErrorMsg(int,QString)));
    connect(m_pColorFocusControl_bottom.data(), SIGNAL(s_colorFocusUpdated(UINT16,float,float)), this, SLOT(updateColorFocusValueToView(UINT16,float,float)));
    connect(m_pColorFocusControl_bottom.data(), &ColorFocusControl::s_connectionStateChanged, this, &MainWindow::updateConnectionState);
    connect(m_pColorFocusControl_bottom.data(), &ColorFocusControl::connectionTimeout, this, &MainWindow::CheckDevicePowerOff);
 
    // m_pSerialCommunicator.reset(new SerialCommunicator);
    // connect(m_pSerialCommunicator.data(), SIGNAL(s_writeLog(QString)), m_log_widget.get(), SLOT(wirteLog(QString)));
    // QObject::connect(m_pSerialCommunicator.data(), &SerialCommunicator::resultReady, this,
    //                  [this](bool match) {
    //                      // qDebug() << "Result match:" << match;
    //                      m_bSeriaCommnReturnFlag = match;
    //                      emit s_writeLog(QString(u8"串口返回结果：%1").arg(match));
    //                  });
    // QObject::connect(m_pSerialCommunicator.data(), &SerialCommunicator::errorOccurred, this,
    //                  [this](const QString &err) {
    //                      // qDebug() << "Error:" << err;
    //                      emit s_writeLog(err);
    //                  });


    // OnOpdateStandardThicknessToMainView();

    //初始化测量厚度所使用的标准片的map
    m_mapUsedStandard[660] = 725;
    m_mapUsedStandard[700] = 725;
    m_mapUsedStandard[750] = 725;
    m_mapUsedStandard[775] = 800;
    m_mapUsedStandard[800] = 800;

    double totalVal1 = m_dialogSetting.getSettings().standardTotalVal1.toDouble();
    double totalVal2 = m_dialogSetting.getSettings().standardTotalVal2.toDouble();
    double totalVal3 = m_dialogSetting.getSettings().standardTotalVal3.toDouble();
    double totalVal4 = m_dialogSetting.getSettings().standardTotalVal4.toDouble();
    m_mapCalibrationInfo[500] = CalibrationInfo{0, 0, 0, totalVal1};
    m_mapCalibrationInfo[725] = CalibrationInfo{0, 0, 0, totalVal2};
    m_mapCalibrationInfo[800] = CalibrationInfo{0, 0, 0, totalVal3};
    m_mapCalibrationInfo[1550] = CalibrationInfo{0, 0, 0, totalVal4};
    ui->label_total_1->setText(m_dialogSetting.getSettings().standardTotalVal1);
    ui->label_total_2->setText(m_dialogSetting.getSettings().standardTotalVal2);
    ui->label_total_3->setText(m_dialogSetting.getSettings().standardTotalVal3);
    ui->label_total_4->setText(m_dialogSetting.getSettings().standardTotalVal4);

    m_currentCalibrationInfo = m_mapCalibrationInfo.value(500);
    
    //等待控件
    m_spinner = new WaitingSpinnerWidget(this);

    m_spinner->setRoundness(70.0);
    m_spinner->setMinimumTrailOpacity(15.0);
    m_spinner->setTrailFadePercentage(70.0);
    m_spinner->setNumberOfLines(12);
    m_spinner->setLineLength(30);
    m_spinner->setLineWidth(8);
    m_spinner->setInnerRadius(30);
    m_spinner->setRevolutionsPerSecond(1);
    m_spinner->setColor(QColor(81, 4, 71));


    // 创建状态栏右侧的 QLabel
    QString currentUser = CurrentUser::instance().username();
    QString currentRole;
    switch (CurrentUser::instance().role()) {
    case 0: currentRole = u8"管理员"; break;
    case 1: currentRole = u8"工程师"; break;
    case 2: currentRole = u8"操作工"; break;
    default:
        break;
    }
    QLabel *rightLabel = new QLabel(QString(u8"当前登录用户：%1  角色：%2").arg(currentUser).arg(currentRole));
    // 添加到状态栏的永久区域（右侧）
    ui->statusbar->addPermanentWidget(rightLabel);

    if(2 == CurrentUser::instance().role()) {
        ui->widget_axis->setEnabled(false);
        ui->pushButton_userManage->setEnabled(false);
        ui->pushButton_repiceSetting->setEnabled(false);
        ui->checkBox_changedRecipe->setEnabled(false);
        ui->pushButton_ScanGravity->setEnabled(false);
        m_pMeasureControl->IsDebug = false;
    }
    else if(1 == CurrentUser::instance().role()){
        ui->pushButton_userManage->setEnabled(false);
        m_pMeasureControl->IsDebug = false;
    }
    ui->verticalWidget_control->setEnabled(false);

    // 定义 X 宏，遍历 ERROR_LIST 插入数据
#define X(code, message) m_errorMap.insert(code, message);
    ERROR_LIST
#undef X

}

MainWindow::~MainWindow()
{
    

    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    m_bIsCloseEvent = true;

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, u8"退出", u8"确定要退出吗？", QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes)
    {
        DisconnectAndClean();

        // 软件彻底关闭，安全释放对象
        delete m_pMeasureControl;
        delete m_pMeasureControlThread;
        delete m_ChipHeatMapViewer;
      
        delete m_ChipSurfaceSampleViewer;

        m_ChipHeatMapViewer = nullptr;

        m_ChipSurfaceSampleViewer = nullptr;
        m_pMeasureControl = nullptr;
        m_pMeasureControlThread = nullptr;

        if(!m_bIsInitFinished_xy || !m_bIsInitFinished_z){
            m_spinner->stop();
        }

        qApp->quit();
    }
    else
    {
        event->ignore();
    }
    
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event); // 保留基类的处理逻辑
    on_comboBox_path_currentIndexChanged(ui->comboBox_path->currentIndex());
}

void MainWindow::InitAllDevice()
{

       
    m_spinner->start();
   
    m_bIsInit_xy = true;
    m_bIsInit_z = true;

    m_bIsInitFinished_xy = false;
    m_bIsInitFinished_z = false;

    m_bIsCloseEvent = false;

    m_bIsColorFocusDisconnected_top = false;
    m_bIsColorFocusDisconnected_bottom = false;
    m_bIsMotionDissconnected = false;
    m_bIsWIDDissconnected = false;



    m_pMeasureControlThread->start();  // 启动线程
    m_pColorFocusControllThread->start();

    //连接共聚焦并开始采集
    QString ipStr_top = m_dialogSetting.getSettings().colorFocusIpTop;
    QString ipStr_bottom = m_dialogSetting.getSettings().colorFocusIpBottom;

    QMetaObject::invokeMethod(m_pColorFocusControl_top.data(), [=]()
    {
        if(m_pColorFocusControl_top->ConnectDevice(ipStr_top))
        {
            m_pColorFocusControl_top->StartAcquisition(TriggerMode::Continue);
        }
    }, Qt::QueuedConnection);
    QMetaObject::invokeMethod(m_pColorFocusControl_bottom.data(), [=]()
    {
        if(m_pColorFocusControl_bottom->ConnectDevice(ipStr_bottom))
        {
            m_pColorFocusControl_bottom->StartAcquisition(TriggerMode::Continue);
        }
    }, Qt::QueuedConnection);
   
   
    if (m_dialogSetting.getSettings().IsUseWID.toInt() == 1) {
        //连接WID
        QString ipStr_wid = m_dialogSetting.getSettings().widIp;
        m_pWIDController->connectToController(ipStr_wid, 2005);
    }
   

    //连接轴系并开始读取实时位置
    QString ipStr_motion = m_dialogSetting.getSettings().axisIp;
    quint16 port = m_dialogSetting.getSettings().axisPort.toInt();
    m_pMotionController->setInitFlag(true);
    m_pMotionController->connectToController(ipStr_motion, port);
    IsAxisHomeMoving = true;

   JudgeAxisPresetPosition();
}

void MainWindow::DisconnectAndClean()
{
    m_bStopJudgePresetPos = true;

    if (m_dialogSetting.getSettings().IsUseWID.toInt() == 1) {
        m_pWIDController->disconnectFromController();
    }

    m_pMotionController->disconnectFromController();


    if (m_pMeasureControl) {
        m_pMeasureControl->TerminateU1000();
        m_pMeasureControl->cleanup();
    }
  
    // 确保线程退出
    if (m_pMeasureControlThread) {
        m_pMeasureControlThread->quit();  // 让线程安全退出
        if (!m_pMeasureControlThread->wait(3000)) { // 最多等待 3 秒
            qWarning() << "线程退出超时，强制终止";
            m_pMeasureControlThread->terminate();
        }
    }
    if (m_pColorFocusControllThread) {
        m_pColorFocusControllThread->quit();  // 让线程安全退出
        if (!m_pColorFocusControllThread->wait(3000)) { // 最多等待 3 秒
            qWarning() << "线程退出超时，强制终止";
            m_pColorFocusControllThread->terminate();
        }
    }
}

void MainWindow::setWidgetBackground(QWidget* widget, const QColor& color)
{
    QPalette pal = widget->palette();
    pal.setColor(QPalette::Window, color);
    widget->setPalette(pal);
    widget->setAutoFillBackground(true);
}



void MainWindow::intComboBox(QString defaultSelectRecipeName="")
{
    ui->comboBox_recipe->clear();
    ui->comboBox_path->clear();
    ui->comboBox_thickness->clear();
    ui->comboBox_size->clear();
    int defaultTrimSize = m_dialogSetting.getSettings().trimSize.toInt();
    auto names=m_recipeSettingDialog.InitRecipes(defaultTrimSize); 
    int defaultRecipeIndex = -1;
    for (int i = 0; i < names.count(); i++)
    {
        ui->comboBox_recipe->addItem(names[i],i);
        if (defaultSelectRecipeName == names[i])
            defaultRecipeIndex = i;
    }
   
    ui->comboBox_recipe->setEditable(true);
    QCompleter* pCompleter = new QCompleter(ui->comboBox_recipe->model());
   
    pCompleter->setFilterMode(Qt::MatchContains);
    ui->comboBox_recipe->setCompleter(pCompleter);

    QLineEdit* lineEdit = ui->comboBox_recipe->lineEdit();
    QFont font("Microsoft YaHei", 12); // 微软雅黑，12号字
    lineEdit->setFont(font);
    // 通过样式表设置字体大小（关键代码）
    pCompleter->popup()->setStyleSheet(
        "font - size: 12pt;"
        "font-family: Microsoft YaHei;"
    );

    //测量路径
    ui->comboBox_path->addItem(u8"1线", 1);
    ui->comboBox_path->addItem(u8"2线", 2);
    ui->comboBox_path->addItem(u8"4线", 4);
    ui->comboBox_path->addItem(u8"6线", 6);
    ui->comboBox_path->addItem(u8"8线", 8);
    ui->comboBox_path->setCurrentIndex(-1);
   
    //产品厚度

 
    QList<int> thicknessNames = m_recipeSettingDialog.FindThicknessName();
    for (int i = 0; i < thicknessNames.count(); i++)
    {
        ui->comboBox_thickness->addItem(QString::number(thicknessNames[i]) +u8"μm", thicknessNames[i]);
    }
    ui->comboBox_thickness->setCurrentIndex(-1);

    //产品尺寸
     ui->comboBox_size->addItem(u8"6英寸", 6);
    ui->comboBox_size->addItem(u8"8英寸", 8);
    ui->comboBox_size->addItem(u8"12英寸", 12);
    ui->comboBox_size->setCurrentIndex(-1);
    ui->comboBox_recipe->setCurrentIndex(defaultRecipeIndex);
    on_comboBox_recipe_currentIndexChanged(defaultRecipeIndex);
    if (m_dialogSetting.getSettings().IsUseWID.toInt() !=1) 
    {
        ui->label_status_wid->setVisible(false);
        ui->label_status_wid_tip->setVisible(false);
        ui->pushButton_readID->setVisible(false);
        ui->label_waferID->setVisible(false);
        ui->label_waferID_tip->setVisible(false);
       
    }
    ui->pushButton_loading->setEnabled(false);
    ui->pushButton_readID->setEnabled(false);
}

//初始化彩色共聚焦曲线控件
void MainWindow::InitColorFocusDisplayWidget()
{
    m_colorFocus_plot_widget_top = std::make_shared<CustomPlotWidget>(ui->widget_plot_top);
    m_colorFocus_plot_widget_top_layout = std::make_shared<QVBoxLayout>(ui->widget_plot_top);
    m_colorFocus_plot_widget_top_layout->addWidget(m_colorFocus_plot_widget_top.get());
    m_colorFocus_plot_widget_top_layout->setMargin(0);
    ui->widget_plot_top->setLayout(m_colorFocus_plot_widget_top_layout.get());

    m_colorFocus_plot_widget_bottom = std::make_shared<CustomPlotWidget>(ui->widget_plot_bottom);
    m_colorFocus_plot_widget_bottom_layout = std::make_shared<QVBoxLayout>(ui->widget_plot_bottom);
    m_colorFocus_plot_widget_bottom_layout->addWidget(m_colorFocus_plot_widget_bottom.get());
    m_colorFocus_plot_widget_bottom_layout->setMargin(0);
    ui->widget_plot_bottom->setLayout(m_colorFocus_plot_widget_bottom_layout.get());
}

void MainWindow::InitLogWidget()
{
    m_log_widget = std::make_shared<LogWidget>(ui->widget_log);
    ui->widget_log->layout()->addWidget(m_log_widget.get());

    connect(this, SIGNAL(s_writeLog(QString)), m_log_widget.get(), SLOT(wirteLog(QString)));
}

void MainWindow::CheckDevicePowerOff()
{

    bool bWidPowerOff = false;
    if (m_dialogSetting.getSettings().IsUseWID.toInt() == 1)
    {
        bWidPowerOff = m_pWIDController->getConnectFailState();
    }

    bool bMotionPowerOff = m_pMotionController->getConnectFailState();
    bool bColorFocusTopPowerOff = m_pColorFocusControl_top->getConnectFailState();
    bool bColorFocusBottonPowerOff = m_pColorFocusControl_bottom->getConnectFailState();

    if(bWidPowerOff && bMotionPowerOff && bColorFocusTopPowerOff && bColorFocusBottonPowerOff)
    {
        m_spinner->stop();
        QMessageBox::warning(this, u8"提示", QString(u8"初始化设备连接失败，请检查设备是否上电！"));
        return;
    }
   
}

void MainWindow::OnCheckDogResult(bool checkResult)
{
    m_bCheckResult = checkResult;
}

void MainWindow::OnStateNotReadyWarning()
{
    QMessageBox::warning(this, u8"提示", QString(u8"轴系运动中，或未处于准备就绪状态，请确认！"));
    m_spinner->stop();
    return;
}

bool MainWindow::CheckSystemStateReady()
{
    if(m_currentSystemState != SystemState::Ready)
    {
        QString str_currentState;

        switch (m_currentSystemState) {
        case SystemState::Calibration: str_currentState = u8"校准中"; break;
        case SystemState::Load: str_currentState = u8"上/下料"; break;
        case SystemState::ReadID: str_currentState = u8"读取ID中"; break;
        case SystemState::Measure: str_currentState = u8"测量中"; break;
        default:
            break;
        }

        QMessageBox::warning(this, u8"提示", QString(u8"系统正处于%1状态，请等待结束后再操作！").arg(str_currentState));
        return false;
    }

    return true;
}

// 功能控制区
//初始化
void MainWindow::on_pushButton_init_clicked()
{
    
    if((m_bIsInitFinished_xy && m_bIsInitFinished_z)||m_IsLinkedFail)
    {
        if (m_IsLinkedFail)
        {
            DisconnectAndClean();
        }
        else
        {

            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(this, u8"初始化", u8"已经初始化过，是否重新初始化？", QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes)
            {
                DisconnectAndClean();
          
            }
            else
            {
                return;
            }

        }
    }
  
    if(!CheckSystemStateReady()){
        return;
    }
    InitAllDevice();
  
}

//参数设置
void MainWindow::on_pushButton_setting_clicked()
{
    m_dialogSetting.exec();
    
  
}

//急停
void MainWindow::on_pushButton_module_eSotp_clicked()
{
    QMetaObject::invokeMethod(m_pMotionController.data(), [&]()
                              {
        if(!m_pMotionController->getConnectState()){
            return;
        }
        m_pMotionController->abortAxes();
                              }, Qt::QueuedConnection);

    emit s_eStop();

    m_currentSystemState = SystemState::Ready;
}

//共聚焦校准区
//校准
void MainWindow::on_pushButton_calibration_clicked()
{
    if(!m_bIsInitFinished_xy)
    {
        QMessageBox::warning(this, u8"提示", QString(u8"请先点击初始化按钮进行初始化！"));
        return;
    }

    if(!m_bCheckResult)
    {
        QMessageBox::warning(this, u8"提示", QString(u8"加密模块验证失败！"));
        return;
    }

    if(!CheckSystemStateReady()){
        return;
    }

   
    m_currentSystemState = SystemState::Calibration;
    if (m_dialogSetting.getSettings().IsUseStandard1550Flag.toInt() != 1)
    {
        if (QMessageBox::question(this, u8"校准提示",
            u8"即将重新校准，请确定是否继续校准?") != QMessageBox::Yes)
            return;
        QtConcurrent::run([=]()
            {
                ParamSettings& settings = m_dialogSetting.getSettings();    
                double dstZPos = settings.z_default_123.toDouble();

                if (m_axisPos.pos_z != dstZPos)
                {
                    QMetaObject::invokeMethod(m_pMotionController.data(), [&]()
                        {
                            m_pMotionController->moveSingleAxisAbsolute(AxisID::Z, dstZPos);
                        }, Qt::BlockingQueuedConnection);
                }
                //等待到位
                waitInPos_z(dstZPos);

                double standard_1_x = settings.posStandard1X.toDouble();
                double standard_1_y = settings.posStandard1Y.toDouble();

                double standard_2_x = settings.posStandard2X.toDouble();
                double standard_2_y = settings.posStandard2Y.toDouble();

                double standard_3_x = settings.posStandard3X.toDouble();
                double standard_3_y = settings.posStandard3Y.toDouble();

                double standard_4_x = settings.posStandard4X.toDouble();
                double standard_4_y = settings.posStandard4Y.toDouble();

                double standard_thickness_1 = settings.standardThickness1.toDouble();
                double standard_thickness_2 = settings.standardThickness2.toDouble();
                double standard_thickness_3 = settings.standardThickness3.toDouble();
                double standard_thickness_4 = settings.standardThickness4.toDouble();

                m_bIsCalibrating = true;

                double dx = 3;
                double dy = 3;
                if (settings.CalibrationDirectionOfX.toInt() == 1) // 暂时保留配置文件设置扫描的方向控制
                {

                    dy = 0;
                }
                else
                {
                    dx = 0;
                }

                ///运动到第一个校准位置
                QMetaObject::invokeMethod(m_pMotionController.data(), [&]()
                    {
                        m_pMotionController->moveMultiAxisLinear(standard_1_x + dx, standard_1_y + dy
                            , settings.velocityNormal.toDouble()
                            , settings.velocityNormal.toDouble() * 10
                            , settings.velocityNormal.toDouble() * 10);
                    }, Qt::BlockingQueuedConnection);
                //等待到位
                waitInPos(QPointF(standard_1_x + dx, standard_1_y + dy));
                //测量一小段位置
                QMetaObject::invokeMethod(m_pMotionController.data(), [&]()
                    {
                        m_pMotionController->moveMultiAxisLinear(standard_1_x - dx, standard_1_y - dy, 2.0, 40.0, 40.0);
                    }, Qt::BlockingQueuedConnection);
                double realTotal_1 = 0.0;
                m_dataBuffer.clear();
                for (int n = 0; n < 30; ++n) {
                    realTotal_1 += filterValue(m_current_distance_top + m_current_distance_bottom + standard_thickness_1);
                    QThread::msleep(100);
                }
                realTotal_1 = realTotal_1 / 30 + m_standardCompensationValue;
                //更新配置数据及显示
                m_mapCalibrationInfo[500] = CalibrationInfo{ m_current_distance_top, m_current_distance_bottom, standard_thickness_1, realTotal_1 };
                QMetaObject::invokeMethod(this, [&]()
                    {
                        ui->label_standard_1->setText(settings.standardThickness1);
                        ui->label_total_1->setText(QString::number(realTotal_1));
                        // settings.standardTotalVal1 = QString::number(realTotal_1);
                    }, Qt::BlockingQueuedConnection);


                ///运动到第二个校准位置
                QMetaObject::invokeMethod(m_pMotionController.data(), [&]()
                    {
                        m_pMotionController->moveMultiAxisLinear(standard_2_x + dx, standard_2_y + dy
                            , settings.velocityNormal.toDouble()
                            , settings.velocityNormal.toDouble() * 10
                            , settings.velocityNormal.toDouble() * 10);
                    }, Qt::BlockingQueuedConnection);
                //等待到位
                waitInPos(QPointF(standard_2_x + dx, standard_2_y + dy));
                //测量一小段位置
                QMetaObject::invokeMethod(m_pMotionController.data(), [&]()
                    {
                        m_pMotionController->moveMultiAxisLinear(standard_2_x - dx, standard_2_y - dy, 2.0, 40.0, 40.0);
                    }, Qt::BlockingQueuedConnection);
                double realTotal_2 = 0.0;
                m_dataBuffer.clear();
                for (int n = 0; n < 30; ++n) {
                    realTotal_2 += filterValue(m_current_distance_top + m_current_distance_bottom + standard_thickness_2);
                    QThread::msleep(100);
                }
                realTotal_2 = realTotal_2 / 30 + m_standardCompensationValue;
                //更新配置数据及显示
                m_mapCalibrationInfo[725] = CalibrationInfo{ m_current_distance_top, m_current_distance_bottom, standard_thickness_2, realTotal_2 };
                QMetaObject::invokeMethod(this, [&]()
                    {
                        ui->label_standard_2->setText(settings.standardThickness2);
                        ui->label_total_2->setText(QString::number(realTotal_2));
                        // settings.standardTotalVal2 = QString::number(realTotal_2);
                    }, Qt::BlockingQueuedConnection);

                ///运动到第三个校准位置
                QMetaObject::invokeMethod(m_pMotionController.data(), [&]()
                    {
                        m_pMotionController->moveMultiAxisLinear(standard_3_x + dx, standard_3_y + dy
                            , settings.velocityNormal.toDouble()
                            , settings.velocityNormal.toDouble() * 10
                            , settings.velocityNormal.toDouble() * 10);
                    }, Qt::BlockingQueuedConnection);
                //等待到位
                waitInPos(QPointF(standard_3_x + dx, standard_3_y + dy));
                //测量一小段位置
                QMetaObject::invokeMethod(m_pMotionController.data(), [&]()
                    {
                        m_pMotionController->moveMultiAxisLinear(standard_3_x - dx, standard_3_y - dy, 2.0, 40.0, 40.0);
                    }, Qt::BlockingQueuedConnection);
                double realTotal_3 = 0.0;
                m_dataBuffer.clear();
                for (int n = 0; n < 30; ++n) {
                    realTotal_3 += filterValue(m_current_distance_top + m_current_distance_bottom + standard_thickness_3);
                    QThread::msleep(100);
                }
                realTotal_3 = realTotal_3 / 30 + m_standardCompensationValue;
                //更新配置数据及显示
                m_mapCalibrationInfo[800] = CalibrationInfo{ m_current_distance_top, m_current_distance_bottom, standard_thickness_3, realTotal_3 };
                QMetaObject::invokeMethod(this, [&]()
                    {
                        ui->label_standard_3->setText(settings.standardThickness3);
                        ui->label_total_3->setText(QString::number(realTotal_3));
                        // settings.standardTotalVal3 = QString::number(realTotal_3);
                    }, Qt::BlockingQueuedConnection);


                ///运动到第四个校准位置
                QMetaObject::invokeMethod(m_pMotionController.data(), [&]()
                    {
                        m_pMotionController->moveMultiAxisLinear(standard_4_x + dx, standard_4_y + dy
                            , settings.velocityNormal.toDouble()
                            , settings.velocityNormal.toDouble() * 10
                            , settings.velocityNormal.toDouble() * 10);
                    }, Qt::BlockingQueuedConnection);
                //等待到位
                waitInPos(QPointF(standard_4_x + dx, standard_4_y + dy));
                //测量一小段位置
                QMetaObject::invokeMethod(m_pMotionController.data(), [&]()
                    {
                        m_pMotionController->moveMultiAxisLinear(standard_4_x - dx, standard_4_y - dy, 2.0, 40.0, 40.0);
                    }, Qt::BlockingQueuedConnection);
                double realTotal_4 = 0.0;
                m_dataBuffer.clear();
                for (int n = 0; n < 30; ++n) {
                    realTotal_4 += filterValue(m_current_distance_top + m_current_distance_bottom + standard_thickness_4);
                    QThread::msleep(100);
                }
                realTotal_4 = realTotal_4 / 30 + m_standardCompensationValue;
                //更新配置数据及显示
                m_mapCalibrationInfo[1550] = CalibrationInfo{ m_current_distance_top, m_current_distance_bottom, standard_thickness_4, realTotal_4 };
                QMetaObject::invokeMethod(this, [&]()
                    {
                        ui->label_standard_4->setText(settings.standardThickness4);
                        ui->label_total_4->setText(QString::number(realTotal_4));
                        // settings.standardTotalVal4 = QString::number(realTotal_4);
                    }, Qt::BlockingQueuedConnection);


                //判断三个标准件数据是否一致
                std::vector<double> values = { realTotal_1, realTotal_2, realTotal_3,realTotal_4 };
                // 计算最小值和最大值
                double minVal = *std::min_element(values.begin(), values.end());
                double maxVal = *std::max_element(values.begin(), values.end());



                double threshold = 2.0;

                // 判断最大值和最小值的差是否超过阈值
                if (maxVal - minVal > threshold) {
                    emit s_writeLog(u8"各校准件测得数据偏差过大，请重新校准");
                    QMetaObject::invokeMethod(this, [&]()
                        {
                            QMessageBox::warning(this, u8"校准", QString(u8"各校准片测得数据偏差过大，请重新校准，或检查校准片是否脏污！"));
                            m_spinner->stop();
                            m_currentSystemState = SystemState::Ready;
                            SetControlStyle();
                        }, Qt::BlockingQueuedConnection);
                    
                   
                    return;
                }

                //将校准数据保存至配置文件
                settings.standardTotalVal1 = QString::number(realTotal_1);
                settings.standardTotalVal2 = QString::number(realTotal_2);
                settings.standardTotalVal3 = QString::number(realTotal_3);
                settings.standardTotalVal4 = QString::number(realTotal_4);
                settings.lastCalTimeStamp_500_1500 = QString::number(QDateTime::currentSecsSinceEpoch());
                m_dialogSetting.saveToFileOnly();

                m_bIsCalibrating = false;

                m_bIsCalAfterInit = true;

                emit s_writeLog(u8"校准完成");

                QMetaObject::invokeMethod(this, [&]() {
                    m_spinner->stop();
                    QMessageBox::information(this, u8"校准", QString(u8"校准完成！"));
                    }, Qt::BlockingQueuedConnection);

                m_currentSystemState = SystemState::Ready;

                return;
            });
    }
    else
    {
        DialogMessageBox digCal;
        if (!digCal.exec()) {
            return;
        }
        if (!digCal.GetSelStandard1550Flag())
        {
            QtConcurrent::run([=]()
                {
                    ParamSettings& settings = m_dialogSetting.getSettings();
                    double dstZPos = settings.z_default_123.toDouble();

                    if (m_axisPos.pos_z != dstZPos)
                    {
                        QMetaObject::invokeMethod(m_pMotionController.data(), [&]()
                            {
                                m_pMotionController->moveSingleAxisAbsolute(AxisID::Z, dstZPos);
                            }, Qt::BlockingQueuedConnection);
                    }
                    //等待到位
                    waitInPos_z(dstZPos);

                    double standard_1_x = settings.posStandard1X.toDouble();
                    double standard_1_y = settings.posStandard1Y.toDouble();

                    double standard_2_x = settings.posStandard2X.toDouble();
                    double standard_2_y = settings.posStandard2Y.toDouble();

                    double standard_3_x = settings.posStandard3X.toDouble();
                    double standard_3_y = settings.posStandard3Y.toDouble();


                    double standard_thickness_1 = settings.standardThickness1.toDouble();
                    double standard_thickness_2 = settings.standardThickness2.toDouble();
                    double standard_thickness_3 = settings.standardThickness3.toDouble();

                    m_bIsCalibrating = true;

                    double dx = 3;
                    double dy = 3;
                    if (settings.CalibrationDirectionOfX.toInt() == 1)
                    {

                        dy = 0;
                    }
                    else
                    {
                        dx = 0;
                    }

                    ///运动到第一个校准位置
                    QMetaObject::invokeMethod(m_pMotionController.data(), [&]()
                        {
                            m_pMotionController->moveMultiAxisLinear(standard_1_x + dx, standard_1_y + dy
                                , settings.velocityNormal.toDouble()
                                , settings.velocityNormal.toDouble() * 10
                                , settings.velocityNormal.toDouble() * 10);
                        }, Qt::BlockingQueuedConnection);
                    //等待到位
                    waitInPos(QPointF(standard_1_x + dx, standard_1_y + dy));
                    //测量一小段位置
                    QMetaObject::invokeMethod(m_pMotionController.data(), [&]()
                        {
                            m_pMotionController->moveMultiAxisLinear(standard_1_x - dx, standard_1_y - dy, 2.0, 40.0, 40.0);
                        }, Qt::BlockingQueuedConnection);
                    double realTotal_1 = 0.0;
                    m_dataBuffer.clear();
                    for (int n = 0; n < 30; ++n) {
                        realTotal_1 += filterValue(m_current_distance_top + m_current_distance_bottom + standard_thickness_1);
                        QThread::msleep(100);
                    }
                    realTotal_1 = realTotal_1 / 30 + m_standardCompensationValue;
                    //更新配置数据及显示
                    m_mapCalibrationInfo[500] = CalibrationInfo{ m_current_distance_top, m_current_distance_bottom, standard_thickness_1, realTotal_1 };
                    QMetaObject::invokeMethod(this, [&]()
                        {
                            ui->label_standard_1->setText(settings.standardThickness1);
                            ui->label_total_1->setText(QString::number(realTotal_1));
                            // settings.standardTotalVal1 = QString::number(realTotal_1);
                        }, Qt::BlockingQueuedConnection);


                    ///运动到第二个校准位置
                    QMetaObject::invokeMethod(m_pMotionController.data(), [&]()
                        {
                            m_pMotionController->moveMultiAxisLinear(standard_2_x + dx, standard_2_y + dy
                                , settings.velocityNormal.toDouble()
                                , settings.velocityNormal.toDouble() * 10
                                , settings.velocityNormal.toDouble() * 10);
                        }, Qt::BlockingQueuedConnection);
                    //等待到位
                    waitInPos(QPointF(standard_2_x + dx, standard_2_y + dy));
                    //测量一小段位置
                    QMetaObject::invokeMethod(m_pMotionController.data(), [&]()
                        {
                            m_pMotionController->moveMultiAxisLinear(standard_2_x - dx, standard_2_y - dy, 2.0, 40.0, 40.0);
                        }, Qt::BlockingQueuedConnection);
                    double realTotal_2 = 0.0;
                    m_dataBuffer.clear();
                    for (int n = 0; n < 30; ++n) {
                        realTotal_2 += filterValue(m_current_distance_top + m_current_distance_bottom + standard_thickness_2);
                        QThread::msleep(100);
                    }
                    realTotal_2 = realTotal_2 / 30 + m_standardCompensationValue;
                    //更新配置数据及显示
                    m_mapCalibrationInfo[725] = CalibrationInfo{ m_current_distance_top, m_current_distance_bottom, standard_thickness_2, realTotal_2 };
                    QMetaObject::invokeMethod(this, [&]()
                        {
                            ui->label_standard_2->setText(settings.standardThickness2);
                            ui->label_total_2->setText(QString::number(realTotal_2));
                            // settings.standardTotalVal2 = QString::number(realTotal_2);
                        }, Qt::BlockingQueuedConnection);

                    ///运动到第三个校准位置
                    QMetaObject::invokeMethod(m_pMotionController.data(), [&]()
                        {
                            m_pMotionController->moveMultiAxisLinear(standard_3_x + dx, standard_3_y + dy
                                , settings.velocityNormal.toDouble()
                                , settings.velocityNormal.toDouble() * 10
                                , settings.velocityNormal.toDouble() * 10);
                        }, Qt::BlockingQueuedConnection);
                    //等待到位
                    waitInPos(QPointF(standard_3_x + dx, standard_3_y + dy));
                    //测量一小段位置
                    QMetaObject::invokeMethod(m_pMotionController.data(), [&]()
                        {
                            m_pMotionController->moveMultiAxisLinear(standard_3_x - dx, standard_3_y - dy, 2.0, 40.0, 40.0);
                        }, Qt::BlockingQueuedConnection);
                    double realTotal_3 = 0.0;
                    m_dataBuffer.clear();
                    for (int n = 0; n < 30; ++n) {
                        realTotal_3 += filterValue(m_current_distance_top + m_current_distance_bottom + standard_thickness_3);
                        QThread::msleep(100);
                    }
                    realTotal_3 = realTotal_3 / 30 + m_standardCompensationValue;
                    //更新配置数据及显示
                    m_mapCalibrationInfo[800] = CalibrationInfo{ m_current_distance_top, m_current_distance_bottom, standard_thickness_3, realTotal_3 };
                    QMetaObject::invokeMethod(this, [&]()
                        {
                            ui->label_standard_3->setText(settings.standardThickness3);
                            ui->label_total_3->setText(QString::number(realTotal_3));
                            // settings.standardTotalVal3 = QString::number(realTotal_3);
                        }, Qt::BlockingQueuedConnection);

                    //判断三个标准件数据是否一致
                    std::vector<double> values = { realTotal_1, realTotal_2, realTotal_3 };
                    // 计算最小值和最大值
                    double minVal = *std::min_element(values.begin(), values.end());
                    double maxVal = *std::max_element(values.begin(), values.end());

                    double threshold = 2.0;

                    // 判断最大值和最小值的差是否超过阈值
                    if (maxVal - minVal > threshold) {
                        emit s_writeLog(u8"各校准件测得数据偏差过大，请重新校准");
                        QMetaObject::invokeMethod(this, [&]()
                            {
                                QMessageBox::warning(this, u8"校准", QString(u8"各校准片测得数据偏差过大，请重新校准，或检查校准片是否脏污！"));
                                m_spinner->stop();
                                m_currentSystemState = SystemState::Ready;
                                SetControlStyle();
                            }, Qt::BlockingQueuedConnection);
                        return;
                    }

                    //将校准数据保存至配置文件
                    settings.standardTotalVal1 = QString::number(realTotal_1);
                    settings.standardTotalVal2 = QString::number(realTotal_2);
                    settings.standardTotalVal3 = QString::number(realTotal_3);

                    settings.lastCalTimeStamp_500_1500 = QString::number(QDateTime::currentSecsSinceEpoch());
                    m_dialogSetting.saveToFileOnly();

                    m_bIsCalibrating = false;

                    m_bIsCalAfterInit = true;

                    emit s_writeLog(u8"校准完成");

                    QMetaObject::invokeMethod(this, [&]() {
                        m_spinner->stop();
                        QMessageBox::information(this, u8"校准", QString(u8"校准完成！"));
                        }, Qt::BlockingQueuedConnection);

                    m_currentSystemState = SystemState::Ready;

                    return;
                });
        }
        else
        {

            QtConcurrent::run([=]()
                {
                    ParamSettings& settings = m_dialogSetting.getSettings();

                    double dstZPos = settings.z_default_4.toDouble();

                    if (m_axisPos.pos_z != dstZPos)
                    {
                        QMetaObject::invokeMethod(m_pMotionController.data(), [&]()
                            {
                                m_pMotionController->moveSingleAxisAbsolute(AxisID::Z, dstZPos);
                            }, Qt::BlockingQueuedConnection);
                    }
                    //等待到位
                    waitInPos_z(dstZPos);

                    double standard_4_x = settings.posStandard4X.toDouble();
                    double standard_4_y = settings.posStandard4Y.toDouble();

                    double standard_thickness_4 = settings.standardThickness4.toDouble();

                    m_bIsCalibrating = true;


                    ///运动到第四个校准位置(三次)
                    double realTotalFirstTime, relTotalSecondTime, realTotalThirdTime;
                    double dx = 3;
                    double dy = 3;
                    if (settings.CalibrationDirectionOfX.toInt() == 1) // 暂时保留配置文件设置扫描的方向控制
                    {

                        dy = 0;
                    }
                    else
                    {
                        dx = 0;
                    }
                    //第一次
                    QMetaObject::invokeMethod(m_pMotionController.data(), [&]()
                        {
                            m_pMotionController->moveMultiAxisLinear(standard_4_x+dx, standard_4_y + dy
                                , settings.velocityNormal.toDouble()
                                , settings.velocityNormal.toDouble() * 10
                                , settings.velocityNormal.toDouble() * 10);
                        }, Qt::BlockingQueuedConnection);
                    //等待到位
                    waitInPos(QPointF(standard_4_x, standard_4_y + 3));
                    //测量一小段位置
                    QMetaObject::invokeMethod(m_pMotionController.data(), [&]()
                        {
                            m_pMotionController->moveMultiAxisLinear(standard_4_x-dx, standard_4_y - dy, 2.0, 40.0, 40.0);
                        }, Qt::BlockingQueuedConnection);
                    realTotalFirstTime = 0.0;
                    m_dataBuffer.clear();
                    for (int n = 0; n < 30; ++n) {
                        realTotalFirstTime += filterValue(m_current_distance_top + m_current_distance_bottom + standard_thickness_4);
                        QThread::msleep(100);
                    }
                    realTotalFirstTime = realTotalFirstTime / 30 + m_standardCompensationValue;
                    //更新配置数据及显示
                    m_mapCalibrationInfo[1550] = CalibrationInfo{ m_current_distance_top, m_current_distance_bottom, standard_thickness_4, realTotalFirstTime };
                    QMetaObject::invokeMethod(this, [&]()
                        {
                            ui->label_standard_4->setText(settings.standardThickness4);
                            ui->label_total_4->setText(QString::number(realTotalFirstTime));
                            // settings.standardTotalVal4 = QString::number(realTotalFirstTime);
                        }, Qt::BlockingQueuedConnection);

                    //第二次
                    QMetaObject::invokeMethod(m_pMotionController.data(), [&]()
                        {
                            m_pMotionController->moveMultiAxisLinear(standard_4_x+dx, standard_4_y + dy
                                , settings.velocityNormal.toDouble()
                                , settings.velocityNormal.toDouble() * 10
                                , settings.velocityNormal.toDouble() * 10);
                        }, Qt::BlockingQueuedConnection);
                    //等待到位
                    waitInPos(QPointF(standard_4_x, standard_4_y + 3));
                    //测量一小段位置
                    QMetaObject::invokeMethod(m_pMotionController.data(), [&]()
                        {
                            m_pMotionController->moveMultiAxisLinear(standard_4_x-dx, standard_4_y - dy, 2.0, 40.0, 40.0);
                        }, Qt::BlockingQueuedConnection);
                    relTotalSecondTime = 0.0;
                    m_dataBuffer.clear();
                    for (int n = 0; n < 30; ++n) {
                        relTotalSecondTime += filterValue(m_current_distance_top + m_current_distance_bottom + standard_thickness_4);
                        QThread::msleep(100);
                    }
                    relTotalSecondTime = relTotalSecondTime / 30 + m_standardCompensationValue;
                    //更新配置数据及显示
                    m_mapCalibrationInfo[1550] = CalibrationInfo{ m_current_distance_top, m_current_distance_bottom, standard_thickness_4, relTotalSecondTime };
                    QMetaObject::invokeMethod(this, [&]()
                        {
                            ui->label_standard_4->setText(settings.standardThickness4);
                            ui->label_total_4->setText(QString::number(relTotalSecondTime));
                            // settings.standardTotalVal4 = QString::number(relTotalSecondTime);
                        }, Qt::BlockingQueuedConnection);

                    //第三次
                    QMetaObject::invokeMethod(m_pMotionController.data(), [&]()
                        {
                            m_pMotionController->moveMultiAxisLinear(standard_4_x+dx, standard_4_y + dy
                                , settings.velocityNormal.toDouble()
                                , settings.velocityNormal.toDouble() * 10
                                , settings.velocityNormal.toDouble() * 10);
                        }, Qt::BlockingQueuedConnection);
                    //等待到位
                    waitInPos(QPointF(standard_4_x, standard_4_y + 3));
                    //测量一小段位置
                    QMetaObject::invokeMethod(m_pMotionController.data(), [&]()
                        {
                            m_pMotionController->moveMultiAxisLinear(standard_4_x-dx, standard_4_y - dy, 2.0, 40.0, 40.0);
                        }, Qt::BlockingQueuedConnection);
                    realTotalThirdTime = 0.0;
                    m_dataBuffer.clear();
                    for (int n = 0; n < 30; ++n) {
                        realTotalThirdTime += filterValue(m_current_distance_top + m_current_distance_bottom + standard_thickness_4);
                        QThread::msleep(100);
                    }
                    realTotalThirdTime = realTotalThirdTime / 30 + m_standardCompensationValue;
                    //更新配置数据及显示
                    m_mapCalibrationInfo[1550] = CalibrationInfo{ m_current_distance_top, m_current_distance_bottom, standard_thickness_4, realTotalThirdTime };
                    QMetaObject::invokeMethod(this, [&]()
                        {
                            ui->label_standard_4->setText(settings.standardThickness4);
                            ui->label_total_4->setText(QString::number(realTotalThirdTime));
                            // settings.standardTotalVal4 = QString::number(realTotalThirdTime);
                        }, Qt::BlockingQueuedConnection);

                    //判断三次标准件数据是否一致
                    std::vector<double> values = { realTotalFirstTime, relTotalSecondTime, realTotalThirdTime };
                    // 计算最小值和最大值
                    double minVal = *std::min_element(values.begin(), values.end());
                    double maxVal = *std::max_element(values.begin(), values.end());

                    double threshold = 2.0;
                   
                    // 判断最大值和最小值的差是否超过阈值
                    if (maxVal - minVal > threshold) {
                        emit s_writeLog(u8"各校准件测得数据偏差过大，请重新校准");
                        QMetaObject::invokeMethod(this, [&]()
                            {
                                QMessageBox::warning(this, u8"校准", QString(u8"各校准片测得数据偏差过大，请重新校准，或检查校准片是否脏污！"));
                                m_spinner->stop();
                                m_currentSystemState = SystemState::Ready;
                                SetControlStyle();
                            }, Qt::BlockingQueuedConnection);


                        return;
                    }

                    //将校准数据保存至配置文件
                    double avgVal = (realTotalFirstTime + relTotalSecondTime + realTotalThirdTime) / 3;
                    settings.standardTotalVal4 = QString::number(avgVal);
                    settings.lastCalTimeStamp_1550 = QString::number(QDateTime::currentSecsSinceEpoch());

                    m_dialogSetting.saveToFileOnly();

                    m_bIsCalibrating = false;

                    m_bIsCalAfterInit = true;

                    emit s_writeLog(u8"校准完成");

                    QMetaObject::invokeMethod(this, [&]() {
                        m_spinner->stop();
                        QMessageBox::information(this, u8"校准", QString(u8"校准完成！"));
                        }, Qt::BlockingQueuedConnection);

                    m_currentSystemState = SystemState::Ready;

                    return;
                });
        }
    }
    m_spinner->start();

}

//标准件1点击
void MainWindow::on_pushButton_standard_part_1_clicked()
{
    if(!CheckSystemStateReady()){
        return;
    }

    m_pMotionController->moveMultiAxisLinear(
        m_dialogSetting.getSettings().posStandard1X.toDouble(), m_dialogSetting.getSettings().posStandard1Y.toDouble()
        , m_dialogSetting.getSettings().velocityNormal.toDouble()
        , m_dialogSetting.getSettings().velocityNormal.toDouble() * 10
        , m_dialogSetting.getSettings().velocityNormal.toDouble() * 10);

    emit s_writeLog(u8"移动到校准位1");
}

//标准件2点击
void MainWindow::on_pushButton_standard_part_2_clicked()
{
    if(!CheckSystemStateReady()){
        return;
    }

    m_pMotionController->moveMultiAxisLinear(
        m_dialogSetting.getSettings().posStandard2X.toDouble(), m_dialogSetting.getSettings().posStandard2Y.toDouble()
        , m_dialogSetting.getSettings().velocityNormal.toDouble()
        , m_dialogSetting.getSettings().velocityNormal.toDouble() * 10
        , m_dialogSetting.getSettings().velocityNormal.toDouble() * 10);

    emit s_writeLog(u8"移动到校准位2");
}

//标准件3点击
void MainWindow::on_pushButton_standard_part_3_clicked()
{
    if(!CheckSystemStateReady()){
        return;
    }

    m_pMotionController->moveMultiAxisLinear(
        m_dialogSetting.getSettings().posStandard3X.toDouble(), m_dialogSetting.getSettings().posStandard3Y.toDouble()
        , m_dialogSetting.getSettings().velocityNormal.toDouble()
        , m_dialogSetting.getSettings().velocityNormal.toDouble() * 10
        , m_dialogSetting.getSettings().velocityNormal.toDouble() * 10);

    emit s_writeLog(u8"移动到校准位3");
}

//标准件4点击
void MainWindow::on_pushButton_standard_part_4_clicked()
{
    if(!CheckSystemStateReady()){
        return;
    }

    m_pMotionController->moveMultiAxisLinear(
        m_dialogSetting.getSettings().posStandard4X.toDouble(), m_dialogSetting.getSettings().posStandard4Y.toDouble()
        , m_dialogSetting.getSettings().velocityNormal.toDouble()
        , m_dialogSetting.getSettings().velocityNormal.toDouble() * 10
        , m_dialogSetting.getSettings().velocityNormal.toDouble() * 10);

    emit s_writeLog(u8"移动到校准位4");
}


//绝对位置移动区
void MainWindow::on_pushButton_moveToPos_x_clicked()
{
    if(!m_pMotionController->getConnectState()){
        return;
    }

    if(!CheckSystemStateReady()){
        return;
    }

    QString strXPos = ui->lineEdit_targetPos_x->text();
    if (strXPos.isEmpty()) {
        return;
    }

    if(!m_bXYLinkage){
        m_pMotionController->moveSingleAxisAbsolute(AxisID::X, strXPos.toDouble());
    }
    else{
        QString strYPos = ui->lineEdit_targetPos_y->text();
        if (strYPos.isEmpty()) {
            return;
        }

        double velocity = ui->lineEdit_xy_velocity->text().toDouble();

        m_pMotionController->moveMultiAxisLinear(
            strXPos.toDouble(), strYPos.toDouble()
            , velocity
            , velocity * 10
            , velocity * 10);
    }
}

void MainWindow::on_pushButton_moveToPos_y_clicked()
{
    if(!m_pMotionController->getConnectState()){
        return;
    }

    if(!CheckSystemStateReady()){
        return;
    }

    QString strYPos = ui->lineEdit_targetPos_y->text();
    if (strYPos.isEmpty()) {
        return;
    }

    if(!m_bXYLinkage){
        m_pMotionController->moveSingleAxisAbsolute(AxisID::Y, strYPos.toDouble());
    }
    else{
        QString strXPos = ui->lineEdit_targetPos_x->text();
        if (strXPos.isEmpty()) {
            return;
        }

        double velocity = ui->lineEdit_xy_velocity->text().toDouble();

        m_pMotionController->moveMultiAxisLinear(
            strXPos.toDouble(), strYPos.toDouble()
            , velocity
            , velocity * 10
            , velocity * 10);
    }
}

void MainWindow::on_pushButton_moveToPos_z_clicked()
{
    if(!m_pMotionController->getConnectState()){
        return;
    }

    if(!CheckSystemStateReady()){
        return;
    }

    if(m_bZLock)
    {
        QMessageBox::information(this, u8"提示", u8"Z轴锁定中！");
        return;
    }

    QString strZPos = ui->lineEdit_targetPos_z->text();
    if (strZPos.isEmpty()) {
        return;
    }

    m_pMotionController->moveSingleAxisAbsolute(AxisID::Z, strZPos.toDouble());
}

//XY轴联动
void MainWindow::on_pushButton_linkage_clicked()
{
    if(m_bXYLinkage)
    {
        m_bXYLinkage = false;
        QIcon icon(":/image/main/image/unlock.png");
        ui->pushButton_linkage->setIcon(icon);
    }
    else
    {
        m_bXYLinkage = true;
        QIcon icon(":/image/main/image/lock.png");
        ui->pushButton_linkage->setIcon(icon);
    }
}

//Z轴锁定
void MainWindow::on_pushButton_zLock_clicked()
{
    if(m_bZLock)
    {
        m_bZLock = false;
        QIcon icon(":/image/main/image/jiesuo.png");
        ui->pushButton_zLock->setIcon(icon);
    }
    else
    {
        m_bZLock = true;
        QIcon icon(":/image/main/image/suoding.png");
        ui->pushButton_zLock->setIcon(icon);
    }
}


//XY轴单步相对运动
void MainWindow::on_pushButton_xy_moveUp_clicked()
{
    if(!m_pMotionController->getConnectState()){
        return;
    }

    if(!CheckSystemStateReady()){
        return;
    }

    QString strYDistance = ui->lineEdit_targetDistance_xy->text();
    if (strYDistance.isEmpty()) {
        return;
    }


    double dYDistance = strYDistance.toDouble();
    if (dYDistance > 0) dYDistance = dYDistance * (-1);

    m_pMotionController->moveSingleAxisIncremental(AxisID::Y, dYDistance);
}

void MainWindow::on_pushButton_xy_moveDown_clicked()
{
    if(!m_pMotionController->getConnectState()){
        return;
    }

    if(!CheckSystemStateReady()){
        return;
    }

    QString strYDistance = ui->lineEdit_targetDistance_xy->text();
    if (strYDistance.isEmpty()) {
        return;
    }

    double dYDistance = strYDistance.toDouble();
    if (dYDistance < 0) dYDistance = dYDistance * (-1);

    m_pMotionController->moveSingleAxisIncremental(AxisID::Y, dYDistance);
}

void MainWindow::on_pushButton_xy_moveLeft_clicked()
{
    if(!m_pMotionController->getConnectState()){
        return;
    }
    
    if(!CheckSystemStateReady()){
        return;
    }

    QString strXDistance = ui->lineEdit_targetDistance_xy->text();
    if (strXDistance.isEmpty()) {
        return;
    }

    double dXDistance = strXDistance.toDouble();
    if (dXDistance > 0) dXDistance = dXDistance * (-1);

    m_pMotionController->moveSingleAxisIncremental(AxisID::X, dXDistance);
}

void MainWindow::on_pushButton_xy_moveRight_clicked()
{
    if(!m_pMotionController->getConnectState()){
        return;
    }

    if(!CheckSystemStateReady()){
        return;
    }

    QString strXDistance = ui->lineEdit_targetDistance_xy->text();
    if (strXDistance.isEmpty()) {
        return;
    }

    double dXDistance = strXDistance.toDouble();
    if (dXDistance < 0) dXDistance = dXDistance * (-1);

    m_pMotionController->moveSingleAxisIncremental(AxisID::X, dXDistance);
}

//XY轴速度调节
void MainWindow::on_pushButton_xy_velocity_reduce_clicked()
{
    //速度-
    double velocity = ui->lineEdit_xy_velocity->text().toDouble();
    if(velocity >= 50.0) { velocity -= 10.0;}
    else if(velocity >= 10.0) { velocity -= 5.0;}
    else if(velocity >= 1.0) { velocity -= 1.0;}
    else{
        velocity -= 0.1;
        if (velocity <= 0.1) {
            velocity = 0.1;
        }
    }

    ui->lineEdit_xy_velocity->setText(QString::number(velocity));
}

void MainWindow::on_pushButton_xy_velocity_increase_clicked()
{
    //速度+
    double velocity = ui->lineEdit_xy_velocity->text().toDouble();
    if(velocity >= 50.0) {
        velocity += 10.0;
        if (velocity >= 100.0) {
            velocity = 100;
        }
    }
    else if(velocity >= 10.0) { velocity += 5.0;}
    else if(velocity >= 1.0) { velocity += 1.0;}
    else{ velocity += 0.1;}

    ui->lineEdit_xy_velocity->setText(QString::number(velocity));
}

//Z轴单步相对运动
void MainWindow::on_pushButton_z_moveUp_clicked()
{
    if(!m_pMotionController->getConnectState()){
        return;
    }

    if(!CheckSystemStateReady()){
        return;
    }

    if(m_bZLock)
    {
        QMessageBox::information(this, u8"提示", u8"Z轴锁定中！");
        return;
    }

    QString strZDistance = ui->lineEdit_targetDistance_z->text();
    if (strZDistance.isEmpty()) {
        return;
    }

    double dZDistance = strZDistance.toDouble();
    if (dZDistance > 0) dZDistance = dZDistance * (-1);

    m_pMotionController->moveSingleAxisIncremental(AxisID::Z, dZDistance);
}

void MainWindow::on_pushButton_z_moveDown_clicked()
{
    if(!m_pMotionController->getConnectState()){
        return;
    }

    if(!CheckSystemStateReady()){
        return;
    }

    if(m_bZLock)
    {
        QMessageBox::information(this, u8"提示", u8"Z轴锁定中！");
        return;
    }

    QString strZDistance = ui->lineEdit_targetDistance_z->text();
    if (strZDistance.isEmpty()) {
        return;
    }

    double dZDistance = strZDistance.toDouble();
    if (dZDistance < 0) dZDistance = dZDistance * (-1);

    m_pMotionController->moveSingleAxisIncremental(AxisID::Z, dZDistance);
}

//Z轴速度调节
void MainWindow::on_pushButton_z_velocity_reduce_clicked()
{
    //速度-
    double velocity = ui->lineEdit_z_velocity->text().toDouble();
    if(velocity >= 50.0) { velocity -= 10.0;}
    else if(velocity >= 10.0) { velocity -= 5.0;}
    else if(velocity >= 1.0) { velocity -= 1.0;}
    else{
        velocity -= 0.1;
        if (velocity <= 0.1) {
            velocity = 0.1;
        }
    }

    ui->lineEdit_z_velocity->setText(QString::number(velocity));
}

void MainWindow::on_pushButton_z_velocity_increase_clicked()
{
    //速度+
    double velocity = ui->lineEdit_z_velocity->text().toDouble();
    if(velocity >= 50.0) {
        velocity += 10.0;
        if (velocity >= 100.0) {
            velocity = 100;
        }
    }
    else if(velocity >= 10.0) { velocity += 5.0;}
    else if(velocity >= 1.0) { velocity += 1.0;}
    else{ velocity += 0.1;}

    ui->lineEdit_z_velocity->setText(QString::number(velocity));
}

void MainWindow::InitAxisMoveButtonFunc()
{
    //XY轴
    //持续左
    connect(ui->pushButton_xy_moveLeft_continuous, &QPushButton::pressed, this, [=](){
        if(!m_pMotionController->getConnectState()){
            return;
        }

        if(!CheckSystemStateReady()){
            return;
        }

        m_pMotionController->moveSingleAxisContinuous(AxisID::X, 0);
    });

    connect(ui->pushButton_xy_moveLeft_continuous, &QPushButton::released, this, [=](){
        m_pMotionController->abortAxis(AxisID::X);
    });

    //持续右
    connect(ui->pushButton_xy_moveRight_continuous, &QPushButton::pressed, this, [=](){
        if(!m_pMotionController->getConnectState()){
            return;
        }

        if(!CheckSystemStateReady()){
            return;
        }

        m_pMotionController->moveSingleAxisContinuous(AxisID::X, 1);
    });

    connect(ui->pushButton_xy_moveRight_continuous, &QPushButton::released, this, [=](){
        m_pMotionController->abortAxis(AxisID::X);
    });

    //持续里
    connect(ui->pushButton_xy_moveUp_continuous, &QPushButton::pressed, this, [=](){
        if(!m_pMotionController->getConnectState()){
            return;
        }

        if(!CheckSystemStateReady()){
            return;
        }

        m_pMotionController->moveSingleAxisContinuous(AxisID::Y, 0);
    });

    connect(ui->pushButton_xy_moveUp_continuous, &QPushButton::released, this, [=](){
        m_pMotionController->abortAxis(AxisID::Y);
    });

    //持续外
    connect(ui->pushButton_xy_moveDown_continuous, &QPushButton::pressed, this, [=](){
        if(!m_pMotionController->getConnectState()){
            return;
        }

        if(!CheckSystemStateReady()){
            return;
        }

        m_pMotionController->moveSingleAxisContinuous(AxisID::Y, 1);
    });

    connect(ui->pushButton_xy_moveDown_continuous, &QPushButton::released, this, [=](){
        m_pMotionController->abortAxis(AxisID::Y);
    });



    //Z轴
    //持续上
    connect(ui->pushButton_z_moveUp_continuous, &QPushButton::pressed, this, [=](){
        if(!m_pMotionController->getConnectState()){
            return;
        }

        if(!CheckSystemStateReady()){
            return;
        }

        if(m_bZLock)
        {
            QMessageBox::information(this, u8"提示", u8"Z轴锁定中！");
            return;
        }
        m_pMotionController->moveSingleAxisContinuous(AxisID::Z, 0);
    });

    connect(ui->pushButton_z_moveUp_continuous, &QPushButton::released, this, [=](){
        m_pMotionController->abortAxis(AxisID::Z);
    });

    //持续下
    connect(ui->pushButton_z_moveDown_continuous, &QPushButton::pressed, this, [=](){
        if(!m_pMotionController->getConnectState()){
            return;
        }

        if(!CheckSystemStateReady()){
            return;
        }

        if(m_bZLock)
        {
            QMessageBox::information(this, u8"提示", u8"Z轴锁定中！");
            return;
        }
        m_pMotionController->moveSingleAxisContinuous(AxisID::Z, 1);
    });

    connect(ui->pushButton_z_moveDown_continuous, &QPushButton::released, this, [=](){
        m_pMotionController->abortAxis(AxisID::Z);
    });
}


//测量流程区
//上下料
void MainWindow::on_pushButton_loading_clicked()
{
    if(!m_bIsInitFinished_xy)
    {
        QMessageBox::warning(this, u8"提示", QString(u8"请先点击初始化按钮进行初始化！"));
        return;
    }

    if(!m_bCheckResult)
    {
        QMessageBox::warning(this, u8"提示", QString(u8"加密模块验证失败！"));
        return;
    }

    if(!CheckSystemStateReady()){
        return;
    }

    GoLoadPostion();
}

void MainWindow::GoLoadPostion()
{
    m_currentSystemState = SystemState::Load;

    m_bIsLoading = true;

    bool ret =m_pMotionController->moveMultiAxisLinear(
        m_dialogSetting.getSettings().posLoadX.toDouble(), m_dialogSetting.getSettings().posLoadY.toDouble()
        , m_dialogSetting.getSettings().velocityNormal.toDouble()
        , m_dialogSetting.getSettings().velocityNormal.toDouble() * 10
        , m_dialogSetting.getSettings().velocityNormal.toDouble() * 10);

    if (!ret)
    {
        m_currentSystemState = SystemState::Ready;
        m_bIsLoading = false;

        return;
    }

    emit s_writeLog(u8"移动到上下料位");
}

//读取ID
void MainWindow::on_pushButton_readID_clicked()
{
    if(!m_bIsInitFinished_xy)
    {
        QMessageBox::warning(this, u8"提示", QString(u8"请先点击初始化按钮进行初始化！"));
        return;
    }
  
    if(!m_bCheckResult)
    {
        QMessageBox::warning(this, u8"提示", QString(u8"加密模块验证失败！"));
        return;
    }

    if(!CheckSystemStateReady()){
        return;
    }

    m_bMeasuring = false;

    ReadID();
}

void MainWindow::ReadID()
{
    m_currentSystemState = SystemState::ReadID;
    SetControlStyle();
    ParamSettings& settings = m_dialogSetting.getSettings();

    bool ret = m_pMotionController->moveMultiAxisLinear(
        m_wait_pos_x, m_wait_pos_y
        , settings.velocityNormal.toDouble()
        , settings.velocityNormal.toDouble() * 10
        , settings.velocityNormal.toDouble() * 10);

    if (!ret)
    {
        m_currentSystemState = SystemState::Ready;
        SetControlStyle();

        return;
    }

    QtConcurrent::run([=](){
        waitInPos(QPointF(m_wait_pos_x, m_wait_pos_y));

        QMetaObject::invokeMethod(m_pWIDController.data(), [=]() {
            m_pWIDController->readWaferID();
        }, Qt::BlockingQueuedConnection);

    });

    emit s_writeLog(u8"移动到扫码位");
}

//开始测量
void MainWindow::on_pushButton_measure_start_clicked()
{
    if ((ProgramMeasureMode == NormalMeasure)&&IsRecipChanged)
    {
        if (QMessageBox::question(this, u8"程式提示",
            u8"程式 " + ui->comboBox_recipe->currentText() + u8" 已经修改是否添加为新的程式? ") == QMessageBox::Yes) 
        {
            m_recipeSettingDialog.newRecipeName = ui->comboBox_recipe->currentText();
            m_recipeSettingDialog.DefaultPath=ui->comboBox_path->currentText();
            m_recipeSettingDialog.DefaultThickness = ui->comboBox_thickness->currentText();
            m_recipeSettingDialog.DefaultSize = ui->comboBox_size->currentText();
            if (m_recipeSettingDialog.ResetRecipe())
            {
                QString selectRecpieName = m_recipeSettingDialog.newRecipeName;
                intComboBox(selectRecpieName);
            }
         
           
        }
        IsRecipChanged = false;
        ui->checkBox_changedRecipe->setEnabled(false);
        ui->comboBox_path->setEnabled(false);
        ui->comboBox_thickness->setEnabled(false);
        ui->comboBox_size->setEnabled(false);
    }

    if(!m_bIsInitFinished_xy)
    {
        QMessageBox::warning(this, u8"提示", QString(u8"请先点击初始化按钮进行初始化！"));
        m_currentSystemState = SystemState::Ready;
        SetControlStyle();
        return;
    }

    if(!m_bCheckResult)
    {
        QMessageBox::warning(this, u8"提示", QString(u8"加密模块验证失败！"));
        m_currentSystemState = SystemState::Ready;
        SetControlStyle();
        return;
    }

    if(!CheckSystemStateReady()){
        /*m_currentSystemState = SystemState::Ready;
        SetControlStyle();*/
        return;
    }

    if (IsXBusying || IsYBusying || IsZBusying)
    {
        QMessageBox::warning(this, u8"提示", QString(u8"轴系运动中，或未处于准备就绪状态，不能进行测量！"));
        return;
    }

    if(!m_bEncoderClear)
    {
        QMessageBox::warning(this, u8"提示", QString(u8"编码器未清零，请手动控制轴运动到零点！"));
        m_currentSystemState = SystemState::Ready;
        SetControlStyle();
        return;
    }

    if(m_bIsLoading)
    {
        QMessageBox::warning(this, u8"提示", QString(u8"运动平台未在上/下料位置！"));
        m_currentSystemState = SystemState::Ready;
        SetControlStyle();
        return;
    }

    if(!m_bIsCalAfterInit)
    {
        QMessageBox::warning(this, u8"错误", QString(u8"重新上电初始化后未校准，请先校准！"));
        m_currentSystemState = SystemState::Ready;
        SetControlStyle();
        return;
    }

    int measurePath = ui->comboBox_path->currentData().toInt();
    int productThickness = ui->comboBox_thickness->currentData().toInt();
    int productSize = ui->comboBox_size->currentData().toInt();
    bool bParaCorrect = m_pMeasureControl->SetMeasurePara(measurePath, productThickness, productSize);

    if ((ProgramMeasureMode == NormalMeasure) && !bParaCorrect) {
        QMessageBox::warning(this, u8"错误", QString(u8"所选测量参数没有对应的重力补偿文件，请检查！"));
        m_currentSystemState = SystemState::Ready;
        SetControlStyle();
        return;
    }
    m_pMeasureControl->RepeatEpochs = ui->spinBox_repeatEpochs->value();
    //求最近的标准
    auto it = m_mapCalibrationInfo.constBegin();
    int closestKey = it.key(); 
    int minDiff = std::abs(closestKey - productThickness); 

    while (it != m_mapCalibrationInfo.constEnd()) {
        int diff = std::abs(it.key() - productThickness); 
        if (diff < minDiff) {
            minDiff = diff; 
            closestKey = it.key(); 
        }
        ++it; 
    }
    productThickness = closestKey;

    m_currentCalibrationInfo = m_mapCalibrationInfo.value(productThickness);
    m_pMeasureControl->SetCalibrationInfo(m_currentCalibrationInfo);
    m_pMeasureControl->ProductName = ui->lineEdit_LotID->text();

    double remindInterval = m_dialogSetting.getSettings().remindInterval.toDouble();
    m_pMeasureControl-> IsNewWarpAlg = m_dialogSetting.getSettings().IsNewWarpAlg.toInt()==1;
    m_pMeasureControl-> IsNewBowAlg = m_dialogSetting.getSettings().IsNewBowAlg.toInt() == 1;
    m_pMeasureControl->IsSaveZM = m_dialogSetting.getSettings().IsSaveZM.toInt() == 1;
    m_pMeasureControl->IsSaveImage3D = m_dialogSetting.getSettings().IsSave3D.toInt() == 1;
    m_pMeasureControl->SetLineSampleParams(m_dialogSetting.getSettings().LineSampleNum.toInt());
    qint64 lastCalTimeStamp_500_1500 = m_dialogSetting.getSettings().lastCalTimeStamp_500_1500.toULongLong();
    qint64 lastCalTimeStamp_1550 = m_dialogSetting.getSettings().lastCalTimeStamp_1550.toULongLong();
    m_pMeasureControl->ChordLength = m_dialogSetting.getSettings().ChordLength.toDouble();
  
    // int productThickness = ui->comboBox_thickness->currentData().toInt();
    qint64 lastCalTimeStamp = productThickness <= 1150 ? lastCalTimeStamp_500_1500 : lastCalTimeStamp_1550;

    if(QDateTime::currentSecsSinceEpoch() - lastCalTimeStamp > remindInterval * 3600)
    {
        emit s_writeLog(QString(u8"距上次校准已超过%1小时，请重新校准！").arg(remindInterval));
        QMessageBox::information(this, u8"提示", QString(u8"距上次校准已超过%1小时，请重新校准！").arg(remindInterval));
        m_currentSystemState = SystemState::Ready;
        SetControlStyle();
        return;
    }
   
    if((ProgramMeasureMode == NormalMeasure) && (ui->lineEdit_LotID->text().isEmpty()|| ui->lineEdit_LotID->text().length()==0))
    {
        QMessageBox::warning(this, u8"错误", QString(u8"产品批号为空！"));
        m_currentSystemState = SystemState::Ready;
        SetControlStyle();
        return;
    }
    QMetaObject::invokeMethod(this,[=](){
        ui->label_ProductState->setText("--");
        ui->label_ProductState->setStyleSheet(
            "color:black ;"          // 文字颜色
            "background-color: white;"  // 背景颜色
            );
      if (LastCurrentLot != ui->lineEdit_LotID->text().trimmed())
        {
         
          while (ui->tableWidget_result->rowCount() > 0)
          {
              ui->tableWidget_result->removeRow(0);
          }

        }
    });

  

    // if(m_currentWaferID == "" || m_currentWaferID == "no_read")
    // {
    //     QMessageBox::warning(this, u8"错误", QString(u8"WaferID未识别！"));
    //     return;
    // }

    m_bMeasuring = true;

    if (m_dialogSetting.getSettings().IsUseWID.toInt() == 1 && ProgramMeasureMode == NormalMeasure)
    {
        ReadID();
    }
    else
    {
        OnUpdateWaferID("--");
    }
}

void MainWindow::StartMeasure()
{
   
    m_currentSystemState = SystemState::Measure;
    SetControlStyle();
    ui->widget_plot->clearGraphs();
    ui->widget_plot->replot();
    ui->pushButton_measure_start->setEnabled(false);
    QMetaObject::invokeMethod(m_pMeasureControl, [&]() {
        m_pMeasureControl->StartMeasure(ProgramMeasureMode);
    }, Qt::QueuedConnection);

    ui->label_detectTime->setText(QDateTime::currentDateTime().toString());

    ui->label_measure_state->setStyleSheet(preset_stateGreen);
    emit s_writeLog(u8"开始测量");

    m_bMeasureOver = false;
}


void MainWindow::SetControlStyle()
{
    bool isReady = m_currentSystemState == SystemState::Ready;

    ui->pushButton_init->setEnabled(isReady);
    ui->pushButton_setting->setEnabled(isReady);
    ui->pushButton_repiceSetting->setEnabled(isReady);
   
    ui->pushButton_userManage->setEnabled(isReady);
    ui->pushButton_ScanGravity->setEnabled(isReady);
    ui->groupBox->setEnabled(isReady);
    ui->widget_axis -> setEnabled(isReady);
    ui->lineEdit_LotID->setEnabled(isReady);
    ui->checkBox_changedRecipe->setEnabled(isReady);
    if (isReady) 
    {
        ui->comboBox_path->setEnabled(ui->checkBox_changedRecipe->isChecked());
        ui->comboBox_thickness->setEnabled(ui->checkBox_changedRecipe->isChecked());
        ui->comboBox_size->setEnabled(ui->checkBox_changedRecipe->isChecked());
    }
    else
    {
        ui->comboBox_path->setEnabled(isReady);
        ui->comboBox_thickness->setEnabled(isReady);
        ui->comboBox_size->setEnabled(isReady);
    }
    ui->checkBox_useRepeatEpochs->setEnabled(isReady);
    if (ui->checkBox_useRepeatEpochs->isChecked())
    {
        ui->spinBox_repeatEpochs->setEnabled(isReady);
    }
    else
    {
        ui->spinBox_repeatEpochs->setEnabled(false);
    }
    ui->comboBox_recipe->setEnabled(isReady);
    ui->pushButton_loading->setEnabled(isReady);
    ui->pushButton_readID->setEnabled(isReady);
    ui->pushButton_measure_start->setEnabled(isReady);
    ui->pushButton_measure_stop->setEnabled(!isReady);
    ui->lineEdit_LotID->setText(ui->lineEdit_LotID->text().trimmed());
}

void MainWindow::PaintChipPointClouds()
{
   m_ChipHeatMapViewer->PaintZMGeatPointClouds(m_pMeasureControl->SurfacePointCloud_Xs, m_pMeasureControl->SurfacePointCloud_Ys, m_pMeasureControl->SurfacePointCloud_ZMs, m_pMeasureControl->theta_Points, m_pMeasureControl->r_Points);
   m_ChipSurfaceSampleViewer->FillZM3DSurfacePointClouds(m_pMeasureControl->SurfacePointCloud_Xs, m_pMeasureControl->SurfacePointCloud_Ys, m_pMeasureControl->SurfacePointCloud_ZMs, m_pMeasureControl->theta_Points, m_pMeasureControl->r_Points);
   colorBarPlot->yAxis2->setRange(QCPRange(m_ChipSurfaceSampleViewer->ZMin, m_ChipSurfaceSampleViewer->ZMax));
   colorBarPlot->rescaleAxes();
   colorBarPlot->replot();
   //colorScale->axis()->setRange(m_ChipSurfaceSampleViewer->ZMin, m_ChipSurfaceSampleViewer->ZMax);
   //QMetaObject::invokeMethod(this, [&]()
   //    {
   //      
   //       
   //    }, Qt::BlockingQueuedConnection);

   this->SavePaintImageToFile();

}
void MainWindow::OnMeasureOver(double BOW, double WARP, double CENTER_THK, double AVERAGE_THK, double TTV, double SORI)
{
    m_bMeasureOver = true;

    ui->label_measure_state->setStyleSheet(preset_stateYellow);
    if (ProgramMeasureMode == NormalMeasure)
    {
        UpdateResultTable(BOW, WARP, CENTER_THK, AVERAGE_THK, TTV, SORI);
        WriteResultToExcel(WarpResult{ BOW, WARP, CENTER_THK, AVERAGE_THK, TTV, SORI });

       
       /* QMetaObject::invokeMethod(this, [&]()
            {*/
                PaintChipPointClouds();
         /*   }, Qt::BlockingQueuedConnection);*/


        
    }
    m_currentWaferID = "";
    ui->label_waferID->setText("");
    ui->label_detectTime->setText("");

    //正常测试模式
    GoLoadPostion();


    ui->pushButton_measure_start->setEnabled(true);
    m_currentSystemState = SystemState::Ready;
    SetControlStyle();
    if (ProgramMeasureMode == ACQNormalGravity)
    {
        ui->pushButton_measure_start->setEnabled(true);
        m_pMeasureControl->ReadAllZGravityFile();
        int defaultRecipeIndex = ui->comboBox_recipe->currentIndex();
        intComboBox(DefaultSelectRecipeName);
        ui->comboBox_recipe->setCurrentIndex(defaultRecipeIndex);
        QMessageBox::warning(this, u8"提示", QString(u8"所选测量参数典型片扫描完成，请进行正常测量！"));
        ProgramMeasureMode = NormalMeasure;
    }
    if (ProgramMeasureMode == ACQOppsiteGravity)
    {
       
 
        QtConcurrent::run([=]() {
            QThread::msleep(5000);
            QMetaObject::invokeMethod(this, [&]()
                {
                    QMessageBox::warning(this, u8"扫描典型片提示", QString(u8"第二步：请沿Y轴左右转翻转晶圆，确保无WaferID面朝上，且箭头依然指向Notch口！"));
                    ProgramMeasureMode = ACQNormalGravity;
                    on_pushButton_measure_start_clicked();
                }, Qt::BlockingQueuedConnection);

            });

    }
    if (ProgramMeasureMode == NormalMeasure) {
        //批量测试模式
        if (ui->checkBox_useRepeatEpochs->isChecked())
        {
            if (++m_pMeasureControl->NowEpoch < m_pMeasureControl->RepeatEpochs)
            {
                QtConcurrent::run([=]() {
                    QThread::msleep(10000);
                    QMetaObject::invokeMethod(this, [&]()
                        {
                            if (ui->checkBox_useRepeatEpochs->isChecked())
                            {
                                on_pushButton_measure_start_clicked();
                            }
                        }, Qt::BlockingQueuedConnection);

                    });
            }
            else
            {
                m_pMeasureControl->NowEpoch = 0;
            }
        }
    }
}

void MainWindow::UpdateMeasurePathFlag(bool bIsMeasurePath)
{
    m_bIsMeasurePath = bIsMeasurePath;
}

void MainWindow::UpdateIsHomeState()
{
    IsAxisHomeMoving = false;
    m_bEncoderClear = false;
    m_XbEncoderClear = false;
    m_YbEncoderClear = false;
}

void MainWindow::OnInitOver()
{
    GoLoadPostion();
}

void MainWindow::updateWIDConnectinonStatusToView(bool isConnected)
{
    ui->label_status_wid->setStyleSheet(isConnected ? connection_stateGreen : connection_stateYellow);
    m_bIsWIDDissconnected = !isConnected;

    if(m_bIsMotionDissconnected && m_bIsWIDDissconnected
        && m_bIsColorFocusDisconnected_top && m_bIsColorFocusDisconnected_bottom)
    {
        if(m_bIsCloseEvent){
            qApp->quit();
        }
        else if(m_bIsInitFinished_xy && m_bIsInitFinished_z){
            InitAllDevice();
        }
    }
}

void MainWindow::OnUpdateWaferID(QString waferID)
{
    ui->label_waferID->setText(waferID);

    // if(waferID == "no_read")
    // {
    //     m_currentWaferID = "";
    //     emit s_writeLog(QString(u8"未识别到ID：%1").arg(waferID));
    //     showErrorMsg(0, u8"未识别到WaferID");
    //      m_currentSystemState = SystemState::Ready;
    //     return;
    // }

    m_currentWaferID = waferID;

    m_currentSystemState = SystemState::Ready;

    if(m_bMeasuring)
    {
        QMetaObject::invokeMethod(this, [&]() { StartMeasure(); }, Qt::QueuedConnection);
    }
    else
    {
        //只读wafer ID并存储文件模式
        if(waferID == "no_read")
        {
            emit s_writeLog(QString(u8"未识别到ID：%1").arg(waferID));
            showErrorMsg(0, u8"未识别到WaferID");
        }
        else
        {
            WriteWaferIdToFile(waferID);
        }

        //回到上料位
        GoLoadPostion();
        //恢复按钮状态
        m_currentSystemState = SystemState::Ready;
        SetControlStyle();
    }
  
    ui->label_Temp->setText(QString::number(m_DialogTemperatureControlDevice.GetTemperature(), 'f', 1));
    ui->label_humidity->setText(QString::number(m_DialogTemperatureControlDevice.GetHumidity(), 'f', 1));
}
bool MainWindow::CheckValue(MeasureValue& rangeValue,double value,QTableWidgetItem* item)
{
    bool state = true;
    if (!rangeValue.IsInvalid)
    {
        if ((value < rangeValue.MinValue) || (value > rangeValue.MaxValue)) 
        {
            state = false;
            item->setBackground(QColor::fromRgb(255, 0, 0));
            //item->setBackgroundColor();
        }
    }
    return state;
}
void MainWindow::UpdateResultTable(double BOW, double WARP, double CENTER_THK, double AVERAGE_THK, double TTV, double SORI)
{
    // static int waferIndex = 1;
    bool isPass = true;
    int rowCount = ui->tableWidget_result->rowCount();
    ui->tableWidget_result->insertRow(rowCount);
    int currentRow = rowCount;
    // ui->tableWidget_result->setItem(currentRow, 0, new QTableWidgetItem(QString::number(waferIndex)));
  
    ui->tableWidget_result->setItem(currentRow, 0,new  QTableWidgetItem(ui->lineEdit_LotID->text()));
    ui->tableWidget_result->setItem(currentRow, 1, new QTableWidgetItem(m_currentWaferID));
    auto item2 = new QTableWidgetItem(QString::number(BOW));
    if (!CheckValue(m_MeasureParamsDialog.BOWValue, BOW, item2))
        isPass = false;
    ui->tableWidget_result->setItem(currentRow, 2, item2);

    auto item3 = new QTableWidgetItem(QString::number(WARP));
    if (!CheckValue(m_MeasureParamsDialog.WARPValue, WARP, item3))
        isPass = false;
    ui->tableWidget_result->setItem(currentRow, 3, item3);

    auto item4 = new QTableWidgetItem(QString::number(CENTER_THK));
    if (!CheckValue(m_MeasureParamsDialog.CTHKValue, CENTER_THK, item4))
        isPass = false;
    ui->tableWidget_result->setItem(currentRow, 4, item4);

    auto item5 = new QTableWidgetItem(QString::number(AVERAGE_THK));
    if (!CheckValue(m_MeasureParamsDialog.ATHKValue, AVERAGE_THK, item5))
        isPass = false;
    ui->tableWidget_result->setItem(currentRow, 5, item5);

    auto item6 = new QTableWidgetItem(QString::number(TTV));
    if (!CheckValue(m_MeasureParamsDialog.TTVValue, TTV, item6))
        isPass = false;
    ui->tableWidget_result->setItem(currentRow, 6, item6);

    auto item7 = new QTableWidgetItem(QString::number(SORI));
    if (!CheckValue(m_MeasureParamsDialog.SORIValue, SORI, item7))
        isPass = false;
    ui->tableWidget_result->setItem(currentRow, 7, item7);

    ui->tableWidget_result->setItem(currentRow, 8, new QTableWidgetItem(QString::number(m_DialogTemperatureControlDevice.GetTemperature(), 'f', 1)));
    ui->tableWidget_result->setItem(currentRow, 9, new QTableWidgetItem(QString::number(m_DialogTemperatureControlDevice.GetHumidity(), 'f', 1)));
    ui->tableWidget_result->setItem(currentRow, 10, new QTableWidgetItem(QDateTime::currentDateTime().toString()));
   

   
    if (isPass)
    {
        ui->label_ProductState->setText("PASS");
        ui->label_ProductState->setStyleSheet(
            "color: white;"          // 文字颜色
            "background-color: lime;"  // 背景颜色
        );
    }
    else
    {
        ui->label_ProductState->setText("FAIL");
        ui->label_ProductState->setStyleSheet(
            "color: yellow;"          // 文字颜色
            "background-color: red;"  // 背景颜色
        );
    }
    // 滚动到最新行
    ui->tableWidget_result->scrollToBottom();
    ui->tableWidget_result->scrollToItem(ui->tableWidget_result->item(currentRow, 0), QAbstractItemView::PositionAtBottom);
   
}

void MainWindow::WriteResultToExcel(WarpResult result)
{
    QString filePath;
    QDir appDir = QCoreApplication::applicationDirPath();

    if(m_dialogSetting.getSettings().measureFileDir.isEmpty()){
        filePath = appDir.filePath("MeasureData");
    }else{
        filePath = m_dialogSetting.getSettings().measureFileDir;
    }

    QDir dir(filePath);
    if (!dir.exists()) {
        dir.mkpath(filePath); // 创建文件夹，包括任何必要的父目录
    }

    QString templateFile = QDir::toNativeSeparators(appDir.filePath(QString("COA_Template.xlsx")));
    if (!QFile::exists(templateFile))
    {
        emit s_writeLog(u8"模板文件不存在！");

        emit s_stopMeasure();

        m_bMeasureOver = true;
        m_currentSystemState = SystemState::Ready;

        ui->label_measure_state->setStyleSheet(preset_stateYellow);
        emit s_writeLog(u8"停止测量");

        QMessageBox::warning(this, u8"保存文件", u8"模板文件不存在！");
        return;
    }

    LastCurrentLot = ui->lineEdit_LotID->text();
    QString currentfileName = QString("%1/%2.xlsx").arg(filePath).arg(LastCurrentLot);
    if (QFile::exists(currentfileName))
    {
        WriteRowContent(currentfileName, result);
    }
    else
    {
        if (!QFile::copy(templateFile, currentfileName)) {
            emit s_stopMeasure();

            m_bMeasureOver = true;
            m_currentSystemState = SystemState::Ready;

            ui->label_measure_state->setStyleSheet(preset_stateYellow);
            emit s_writeLog(u8"停止测量");
            return;;
        }
        else{
       
            WriteRowContent(currentfileName, result);
        }
    }

    //emit s_writeLog(u8"写文件结束！");
    // QMessageBox::information(this, u8"提示", u8"测量完成！");
}

void MainWindow::WriteRowContent(QString fileName, WarpResult result)
{
    Document xlsx(fileName);
    if (!xlsx.load()) {
        QMessageBox::warning(this, u8"打开文件", u8"更新Excel文件失败！");
        return;
    }
  
   
    LastCurrentLot = ui->lineEdit_LotID->text();

    bool state= xlsx.selectSheet(0);
    if (!state)
    {
        emit s_writeLog(u8"选择文件内容失败！");
        QMessageBox::warning(this, u8"保存文件", u8"选择文件内容失败！");
        return;
    }
    Worksheet* sheet = xlsx.currentWorksheet();

    // **从第一行开始检查，找到最后一行有数据的行号**
    int lastDataRow = Template_File_Head_Row_Count; // 记录最后一个有数据的行号
    int maxCols = 13; // 最多检查前 13 列
    int dimensionRow = sheet->dimension().lastRow();

    for (int row = dimensionRow; row >= Template_File_Head_Row_Count + 1; --row) {
        for (int col = 1; col <= maxCols; ++col) {
            QXlsx::Cell *cell = sheet->cellAt(row, col);
            if (cell && !cell->value().isNull()) {  // **判断单元格是否真的有数据**
                lastDataRow = row;
                goto found; // **找到后立即跳出循环**
            }
        }
    }

found:
    int pieceCount = lastDataRow - Template_File_Head_Row_Count;
    int nowDataRow = lastDataRow + 1;
    state = xlsx.write(nowDataRow, 1, LastCurrentLot)&&state;
    state = xlsx.write(nowDataRow, 2, m_currentWaferID) && state;
    state = xlsx.write(nowDataRow, 3, pieceCount + 1) && state;
    state = xlsx.write(nowDataRow, 4,  result.AVERAGE_THK) && state;
    state = xlsx.write(nowDataRow, 5,  result.CENTER_THK) && state;
    state = xlsx.write(nowDataRow, 6,  result.TTV) && state;
    state = xlsx.write(nowDataRow, 7,  result.BOW) && state;
    state = xlsx.write(nowDataRow, 8,  result.WARP) && state; 
    state = xlsx.write(nowDataRow, 9,  result.SORI) && state;
    if (m_dialogSetting.getSettings().IsUseStandard1550Flag.toInt() != 1)
    {
        double temperature =m_DialogTemperatureControlDevice.GetTemperature();
        double humidity = m_DialogTemperatureControlDevice.GetHumidity();
        state = xlsx.write(nowDataRow, 10, temperature)&& state;
        state = xlsx.write(nowDataRow, 11, humidity) && state;

    }
    // xlsx.write(lastDataRow + 1, 9,  QDateTime::currentDateTime().toString());
    if (!state)
    {
        emit s_writeLog(u8"写入文件内容失败！");
        QMessageBox::warning(this, u8"保存文件", u8"写入文件内容失败！");
        return;
    }
    if(!xlsx.save())
    {
        emit s_writeLog(u8"保存文件失败！");
        QMessageBox::warning(this, u8"保存文件", u8"保存文件失败！");

        return;
    }
}

double MainWindow::filterValue(double newValue, int windowSize)
{
    m_dataBuffer.append(newValue);

    // 保持窗口大小
    if (m_dataBuffer.size() > windowSize) {
        m_dataBuffer.removeFirst();
    }

    // ----------- 中值滤波 -----------
    QVector<double> sortedData = m_dataBuffer;
    std::sort(sortedData.begin(), sortedData.end());
    double medianValue = sortedData[sortedData.size() / 2];

    // ----------- 滑动平均滤波 -----------
    double avgValue = std::accumulate(sortedData.begin(), sortedData.end(), 0.0) / sortedData.size();

    // ----------- 数据融合 -----------
    return (medianValue * 0.7) + (avgValue * 0.3);  // 组合滤波，按权重调整
}

void MainWindow::InitPlotWidget()
{
    // 初始化绘图控件
    QCustomPlot *customPlot = ui->widget_plot;
    // customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    // 获取绘图区域的轴矩形（默认轴）
    QCPAxisRect *axisRect = customPlot->axisRect();

    // 设置缩放仅作用于 X 轴
    axisRect->setRangeZoomAxes(customPlot->xAxis, nullptr);
    // 第二个参数为 Y 轴指针，设为 nullptr 表示禁止 Y 轴缩放
    // 配置拖动：仅允许 X 轴拖动
    axisRect->setRangeDragAxes(customPlot->xAxis, nullptr);  // Y 轴设为 nullptr

    // 启用缩放交互（默认是水平+垂直，此处已通过 setRangeZoomAxes 限制）
    customPlot->setInteraction(QCP::iRangeZoom, true);
    customPlot->setInteraction(QCP::iRangeDrag, true);
    // 显示图例
    customPlot->legend->setVisible(true);
    // 允许图例项可选中（显示复选框）
    customPlot->legend->setSelectableParts(QCPLegend::spItems);
    // 监听鼠标移动事件
    // connect(customPlot, &QCustomPlot::mouseMove, this, &MainWindow::onLegendHover);
    // 设置 X 轴标签
    customPlot->xAxis->setLabel(u8"测量点（ 左<---->右 ）");
    // 设置 Y 轴标签
    customPlot->yAxis->setLabel(u8"厚度 (μm)");

    // 连接图例点击信号到槽函数
    connect(customPlot, &QCustomPlot::legendClick, [=](QCPLegend *legend, QCPAbstractLegendItem *item) {
        Q_UNUSED(legend)
        if (item) {
            // 切换对应曲线的可见性
            QCPPlottableLegendItem *plItem = qobject_cast<QCPPlottableLegendItem*>(item);
            if (plItem) {
                plItem->plottable()->setVisible(!plItem->plottable()->visible());
                customPlot->replot(); // 重绘
            }
        }
    });
    InitChipSurfaceSampleViewer();
}

void MainWindow::InitChipSurfaceSampleViewer()
{
   
   m_ChipHeatMapViewer = new ChipHeatMapViewer(ui->widget_plot_chipheatmap);

   ChipSurfaceGraph = new Q3DSurface();
 
   ChipSurfaceContainer = QWidget::createWindowContainer(ChipSurfaceGraph);
   m_ChipSurfaceSampleViewer = new ChipSurfaceSampleViewer(ChipSurfaceGraph);

   m_ChipSurfaceSampleViewer = new ChipSurfaceSampleViewer(ChipSurfaceGraph);

   ChipSurfaceContainer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
   ChipSurfaceContainer->setFocusPolicy(Qt::StrongFocus);
   ui->horizontalLayout_Viewer->addWidget(ChipSurfaceContainer,9);


  

   // 创建左侧QCustomPlot颜色条
   colorBarPlot = new QCustomPlot(this);
   colorBarPlot->setMinimumWidth(100);
   colorBarPlot->setMaximumWidth(100);
   colorBarPlot->setFixedWidth(100);
   // 创建颜色映射
   QCPColorMap* colorMap = new QCPColorMap(colorBarPlot->xAxis, colorBarPlot->yAxis);
   colorMap->data()->setSize(1, 100);  // 1列，100行
   colorMap->data()->setRange(QCPRange(0, 1), QCPRange(0, 6));
 


   // 填充颜色数据
   for (int yIndex = 0; yIndex < 100; ++yIndex) {
       double value = 10.0 * yIndex / 150.0;  // 值范围0-10
       colorMap->data()->setCell(0, yIndex, value);
   }

   // 设置颜色渐变
   QCPColorGradient gradient;
   gradient.setColorStopAt(0.0, Qt::blue);
   gradient.setColorStopAt(0.2, Qt::cyan);
   gradient.setColorStopAt(0.5, Qt::green);
   gradient.setColorStopAt(0.8, Qt::yellow);
   gradient.setColorStopAt(1.0, Qt::red);
   colorMap->setGradient(gradient);

   // 创建颜色条
   colorScale = new QCPColorScale(colorBarPlot);
   colorBarPlot->plotLayout()->addElement(0, 0, colorScale);
   colorMap->setColorScale(colorScale);
   colorMap->rescaleDataRange(true);
  /* colorScale->setLabel(u8"温度");*/
   colorScale->setBarWidth(100);
   colorScale->setGradient(QCPColorGradient::gpJet);
  // colorScale->axis()->setRange();
   // 设置坐标轴范围和可见性
 /*  colorBarPlot->xAxis->setRange(0, 1);*/
  /* colorBarPlot->yAxis->setRange(0, 100);*/
   colorBarPlot->xAxis->setVisible(false);
   colorBarPlot->xAxis2->setVisible(false);
   colorBarPlot->yAxis->setVisible(false);
   colorBarPlot->yAxis2->setVisible(true);
   colorBarPlot->yAxis2->setRange(QCPRange(0, 100));
   // 刷新绘图
   colorBarPlot->rescaleAxes();
   colorBarPlot->replot();
   colorBarPlot->setMinimumWidth(65);
   ui->horizontalLayout_Viewer->addWidget(colorBarPlot,1);


   QLinearGradient gr;

   QMap<double, QColor> stops = colorMap->gradient().colorStops();
   for (auto it = stops.begin(); it != stops.end(); ++it) {
       gr.setColorAt(it.key(), it.value());
   }
   m_ChipSurfaceSampleViewer->m_sqrtSinSeries->setBaseGradient(gr);
  
}

void  MainWindow::SaveControlToImage(QPixmap pixmap)
{
    
    // 保存对话框
    QString filePath = QFileDialog::getSaveFileName(
        this, u8"保存图片", QDir::homePath(), u8"PNG图片 (*.png);;JPEG图片 (*.jpg);;所有文件 (*)"
    );

    if (!filePath.isEmpty()) {
        // 保存图片
        if (pixmap.save(filePath)) {
            QMessageBox::information(this, u8"成功", u8"图片保存成功！");
        }
        else {
            QMessageBox::warning(this, u8"失败", u8"图片保存失败！");
        }
    }
}
// 鼠标移动事件处理函数
void MainWindow::onLegendHover(QMouseEvent *event)
{
    QCustomPlot *customPlot = ui->widget_plot;

    // 获取鼠标在绘图控件中的位置
    double mouseX = event->pos().x();
    double mouseY = event->pos().y();

    // 遍历图例项，找出鼠标悬停的项
    QCPPlottableLegendItem *hoveredItem = nullptr;
    for (int i = 0; i < customPlot->legend->itemCount(); ++i)
    {
        QCPAbstractLegendItem *item = customPlot->legend->item(i);
        if (item && item->selectTest(QPointF(mouseX, mouseY), false) >= 0)
        {
            hoveredItem = qobject_cast<QCPPlottableLegendItem *>(item);
            break;
        }
    }

    // 处理高亮效果
    if (hoveredItem != lastHighlightedItem)
    {
        if (lastHighlightedItem)
        {
            // 恢复上一个高亮项的默认样式
            QFont font = lastHighlightedItem->font();
            font.setPointSize(10);  // 恢复默认字号
            lastHighlightedItem->setFont(font);
        }

        if (hoveredItem)
        {
            // 使当前项高亮
            QFont font = hoveredItem->font();
            font.setPointSize(12);  // 变大字号
            hoveredItem->setFont(font);
        }

        // 记录当前高亮的项
        lastHighlightedItem = hoveredItem;

        // 重新绘制
        customPlot->replot();
    }
}

void MainWindow::OnUpdatePlot(int pathIndex)
{
    m_dataBuffer.clear();
    QCustomPlot *customPlot = ui->widget_plot;

    // 隐藏所有已有曲线
    for (int i = 0; i < customPlot->graphCount(); ++i) {
        customPlot->graph(i)->setVisible(false);
    }

    // 添加曲线
    std::vector<DataPoint>& vecLineData = m_pMeasureControl->GetLineDataVec().at(pathIndex);
    int dataCount = static_cast<int>(vecLineData.size());
    if (m_pMeasureControl->IsDebug) {
        emit s_writeLog(QString(u8"路径%1的测量点数为：%2").arg(pathIndex + 1).arg(dataCount));
    }
    QVector<double> x1(dataCount), y1(dataCount);
    // ... 填充数据（示例用随机数）
    for (int i = 0, j = dataCount -1; i < dataCount; ++i, --j) {
        x1[i] = i;
        y1[i] = filterValue(vecLineData.at(j).t, m_windowSize_plot);
        // qDebug()<< QString(u8"路径%1，第%2点X = %3").arg(pathIndex + 1).arg(i + 1).arg(vecLineData.at(j).x_en);
    }
    QCPGraph *graph1 = customPlot->addGraph();
    graph1->setData(x1, y1);
    graph1->setName(QString(u8"路径%1").arg(pathIndex + 1)); // 设置图例名称
    const int colorIndex = pathIndex % m_plotColors.size();
    graph1->setPen(QPen(m_plotColors[colorIndex], 1));  // 设置颜色
    graph1->setVisible(true);  // 新添加的曲线默认可见
    // 确保图例颜色与曲线同步
    // graph1->setSelectedPen(QPen(m_plotColors[colorIndex].darker(150), 1.5));
    // graph1->setSelectedBrush(QBrush(curveColor.darker(150))); // 图例背景色
    // graph1->setSelectedIconBorderPen(QPen(curveColor.darker(150))); // 图例边框
    // graph1->setLineStyle(QCPGraph::lsNone); // 关闭连线
    // graph1->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, 1)); // 圆形点，大小5像素

    // 自动缩放轴
    customPlot->rescaleAxes();
    customPlot->replot();
  
    // if(dataCount < 5000)
    // {
    //     if (CurrentUser::instance().role() != 2)
    //     {
    //         QDir appDir = QCoreApplication::applicationDirPath() + "/Log/Errors";
    //         QString dirName = QDate::currentDate().toString("yyyy-MM-dd");
    //         QString fileFilePath = appDir.filePath(dirName);
    //         QDir dirPath(fileFilePath);
    //         bool isError = false;
    //         if (!dirPath.exists())
    //         {

    //             if (dirPath.mkpath(".")) //
    //             {
    //                 qDebug() << "成功创建目录:" << dirPath;
    //             }
    //             else
    //             {
    //                 isError = true;
    //                 qDebug() << "创建目录失败:" << dirPath;

    //             }
    //         }
    //         if (!isError) {
    //             std::vector<DataPoint>& vecLineData = m_pMeasureControl->GetLineDataVec().at(pathIndex);
    //             QString fileTopCSVName = QString("Top_LineError_%1_%2.csv").arg(pathIndex + 1).arg(QDate::currentDate().toString("yyyy_MM_dd"));
    //             QString fileBottomCSVName = QString("Bottom_LineError_%1_%2.csv").arg(pathIndex + 1).arg(QDate::currentDate().toString("yyyy_MM_dd"));
    //             QString saveCSVFile1 = fileFilePath + "/" + fileTopCSVName;
    //             QString saveCSVFile2 = fileFilePath + "/" + fileBottomCSVName;
    //             QFile file1(saveCSVFile1);
    //             if (file1.exists())
    //                 file1.remove();
    //             /* 以文本方式写入*/
    //             if (file1.open(QIODevice::WriteOnly | QIODevice::Text)) {
    //                 QTextStream CSVout(&file1);
    //                 auto encoderDatas = m_pMeasureControl->Get_vecEncoders_top();
    //                 int total = encoderDatas.size();
    //                 for (int i = 0; i < total; ++i) {
    //                     auto value = encoderDatas.at(i);
    //                     CSVout << QString("%1").arg(value) << "\n";
    //                 }
    //             }
    //             file1.close();

    //             QFile file2(saveCSVFile2);
    //             if (file2.exists())
    //                 file2.remove();
    //             /* 以文本方式写入*/
    //             if (file2.open(QIODevice::WriteOnly | QIODevice::Text)) {
    //                 QTextStream CSVout(&file2);
    //                 auto encoderDatas = m_pMeasureControl->Get_vecEncoders_bottom();
    //                 int total = encoderDatas.size();
    //                 for (int i = 0; i < total; ++i) {
    //                     auto value = encoderDatas.at(i);
    //                     CSVout << QString("%1").arg(value) << "\n";
    //                 }
    //             }
    //             file2.close();
    //         }
    //     }

    //     emit s_stopMeasure();
    //     emit s_writeLog(QString(u8"路径%1的测量点数不足5000，请检查硅片放置情况或上共聚焦位置是否在测量量程内。").arg(pathIndex + 1));

    //     emit s_stopMeasure();

    //     m_bMeasureOver = true;
    //     m_currentSystemState = SystemState::Ready;

    //     ui->label_measure_state->setStyleSheet(preset_stateYellow);
    //     emit s_writeLog(u8"停止测量");

    //     QMessageBox::warning(this, u8"错误", QString(u8"路径%1的测量点数不足10000，请检查硅片放置情况或上共聚焦位置是否在测量量程内。").arg(pathIndex + 1));
    //     SetControlStyle();
    //     return;
    // }
}

void MainWindow::OnOpdateStandardThicknessToMainView()
{
    ui->pushButton_standard_part_1->setText(m_dialogSetting.getSettings().standardThickness1);
    ui->pushButton_standard_part_2->setText(m_dialogSetting.getSettings().standardThickness2);
    ui->pushButton_standard_part_3->setText(m_dialogSetting.getSettings().standardThickness3);
    ui->pushButton_standard_part_4->setText(m_dialogSetting.getSettings().standardThickness4);
}

//停止测量
void MainWindow::on_pushButton_measure_stop_clicked()
{
    if(!m_bIsInitFinished_xy)
    {
        QMessageBox::warning(this, u8"提示", QString(u8"请先点击初始化按钮进行初始化！"));
        return;
    }

    if(!m_bCheckResult)
    {
        QMessageBox::warning(this, u8"提示", QString(u8"加密模块验证失败！"));
        return;
    }

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, u8"测量", u8"确认停止测量？", QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) {
        return;
    }

    emit s_stopMeasure();

    m_bMeasureOver = true;
    m_currentSystemState = SystemState::Ready;
  
    ui->label_measure_state->setStyleSheet(preset_stateYellow);
    emit s_writeLog(u8"停止测量");

   
    SetControlStyle();
}


//更新彩色共聚焦实时值至界面控件
void MainWindow::updateColorFocusValueToView(UINT16 sensor_id, float distance, float intensity)
{
    distance > 0 ? distance : distance = 0;
    if(SENSOR_ID1 == sensor_id)
    {
        m_current_distance_top = distance;

        m_colorFocus_plot_widget_top->UpdatePlot(distance, 0.2);
        ui->label_current_distance_top->setText(QString::number(distance, 'f', 3));
        ui->label_current_intensity_top->setText(QString::number(intensity, 'f', 3));
    }
    else if(SENSOR_ID2 == sensor_id)
    {
        m_current_distance_bottom = distance;

        m_colorFocus_plot_widget_bottom->UpdatePlot(distance, 0.2);
        ui->label_current_distance_bottom->setText(QString::number(distance, 'f', 3));
        ui->label_current_intensity_bottom->setText(QString::number(intensity, 'f', 3));
    }

    // if(m_bIsMeasurePath)
    {
        if(fuzzyEqual(m_current_distance_top, 0) || fuzzyEqual(m_current_distance_bottom, 0)){
            return;
        }
        m_realtimeThickness = m_currentCalibrationInfo.total - m_current_distance_top - m_current_distance_bottom;

        // m_realtimeThickness > 0 ? m_realtimeThickness : m_realtimeThickness = 0;
        ui->label_realtime_thickness->setText(QString::number(m_realtimeThickness, 'f', 3));
    }
    // if(!fuzzyEqual(m_realtimeThickness, 0))
    // {
    //     m_realtimeThickness = 0;
    //     ui->label_realtime_thickness->setText(QString::number(m_realtimeThickness, 'f', 3));
    // }

    ui->label_Temp->setText(QString::number(m_DialogTemperatureControlDevice.GetTemperature(), 'f', 1));
    ui->label_humidity->setText(QString::number(m_DialogTemperatureControlDevice.GetHumidity(), 'f', 1));
}

//显示错误信息
void MainWindow::showErrorMsg(int sensor_id, QString lastErrorMsg)
{
    Q_UNUSED(sensor_id);

    QMessageBox::warning(this, u8"错误", lastErrorMsg);
}

//更新彩色共聚焦连接状态
void MainWindow::updateConnectionState(UINT16 sensor_id, bool isConnected)
{
    if(SENSOR_ID1 == sensor_id)
    {
        ui->label_status_colorFocus_top->setStyleSheet(isConnected ? connection_stateGreen : connection_stateYellow);
        m_bIsColorFocusDisconnected_top = !isConnected;
    }
    else if(SENSOR_ID2 == sensor_id)
    {
        ui->label_status_colorFocus_bottom->setStyleSheet(isConnected ? connection_stateGreen : connection_stateYellow);
        m_bIsColorFocusDisconnected_bottom = !isConnected;
    }

    if(m_bIsMotionDissconnected && m_bIsWIDDissconnected
        && m_bIsColorFocusDisconnected_top && m_bIsColorFocusDisconnected_bottom)
    {
        if(m_bIsCloseEvent){
            qApp->quit();
        }
        else if(m_bIsInitFinished_xy && m_bIsInitFinished_z){
            InitAllDevice();
        }
    }
}

void MainWindow::ExportMeasureData()
{
    //创建文件夹
    QDir appDir = QCoreApplication::applicationDirPath();
    QString currentTime = QString::number(QDateTime::currentSecsSinceEpoch());
    QString filePath = appDir.filePath("MeasureData/");
    QDir dir(filePath);
    if (!dir.exists())
    {
        dir.mkpath(filePath); // 创建文件夹，包括任何必要的父目录
    }

    QFile file_top(QString("%1/%2_top.csv").arg(filePath, currentTime));
    if (file_top.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream out(&file_top);
        QList<_DISTANCE_VALUE>& distanceList = m_pColorFocusControl_top->GetMeasurePointDistanceList();
        // for (const _DISTANCE_VALUE& distanceInfo : distanceList)
        // {
        //     out << distanceInfo.Encoder1 << "," << distanceInfo.Encoder2 << "," << distanceInfo.Encoder3 << ","
        //         << m_pColorFocusControl_top->ChangeToPositionValue(distanceInfo.Encoder1) << ","
        //         << m_pColorFocusControl_top->ChangeToPositionValue(distanceInfo.Encoder2) << ","
        //         << m_pColorFocusControl_top->ChangeToPositionValue(distanceInfo.Encoder3) << ","
        //         << distanceInfo.distance << "\n";
        // }
        for (const _DISTANCE_VALUE& distanceInfo : distanceList)
        {
            static uint x = 1; uint y = 1;

            out << x++ << "," << y << "," << distanceInfo.distance << "\n";
        }
        file_top.close();
    } else
    {
        QMessageBox::warning(this, u8"错误", QString(u8"无法打开文件进行写操作"));
        return;
    }

    QFile file_bottom(QString("%1/%2_bottom.csv").arg(filePath, currentTime));
    if (file_bottom.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream out(&file_bottom);
        QList<_DISTANCE_VALUE>& distanceList = m_pColorFocusControl_bottom->GetMeasurePointDistanceList();
        // for (const _DISTANCE_VALUE& distanceInfo : distanceList)
        // {
        //     out << distanceInfo.Encoder1 << "," << distanceInfo.Encoder2 << "," << distanceInfo.Encoder3 << ","
        //         << m_pColorFocusControl_bottom->ChangeToPositionValue(distanceInfo.Encoder1) << ","
        //         << m_pColorFocusControl_bottom->ChangeToPositionValue(distanceInfo.Encoder2) << ","
        //         << m_pColorFocusControl_bottom->ChangeToPositionValue(distanceInfo.Encoder3) << ","
        //         << distanceInfo.distance << "\n";
        // }
        for (const _DISTANCE_VALUE& distanceInfo : distanceList)
        {
            static uint x = 1; uint y = 1;

            out << x++ << "," << y << "," << distanceInfo.distance << "\n";
        }
        file_bottom.close();
    } else
    {
        QMessageBox::warning(this, u8"错误", QString(u8"无法打开文件进行写操作"));
        return;
    }
}

void MainWindow::updateAxisConnectinonStatusToView(bool isConnected)
{
    ui->label_status_axis->setStyleSheet(isConnected ? connection_stateGreen : connection_stateYellow);
    m_bIsMotionDissconnected = !isConnected;

    if(m_bIsMotionDissconnected && m_bIsWIDDissconnected
        && m_bIsColorFocusDisconnected_top && m_bIsColorFocusDisconnected_bottom)
    {
        if(m_bIsCloseEvent){
            qApp->quit();
        }
        else if(m_bIsInitFinished_xy && m_bIsInitFinished_z){
            InitAllDevice();
        }
    }
}

void MainWindow::updateAxisStateToView(AxisState axisState)
{
    m_axisState = axisState;

    //x轴
    if(0 == axisState.state_x){
        if(!m_errorInfo_x.first){
            m_errorInfo_x.first = true;
            ui->label_status_x->setStyleSheet(axis_stateRed);

            m_errorInfo_x.second = m_errorMap.value(axisState.error_x, "Unknown error");
        }
    }
    else if(1 == axisState.state_x){
        if(!m_errorInfo_x.first){
            m_errorInfo_x.first = true;
            ui->label_status_x->setStyleSheet(axis_stateYellow);

            m_errorInfo_x.second = m_errorMap.value(axisState.error_x, "Unknown error");
        }
    }
    else if(axisState.state_x > 1){
        if(m_errorInfo_x.first){
            m_errorInfo_x.first = false;
            ui->label_status_x->setStyleSheet(axis_stateGreen);
        }
    }

    //y轴
    if(0 == axisState.state_y){
        if(!m_errorInfo_y.first){
            m_errorInfo_y.first = true;
            ui->label_status_y->setStyleSheet(axis_stateRed);

            m_errorInfo_y.second = m_errorMap.value(axisState.error_y, "Unknown error");
        }
    }
    else if(1 == axisState.state_y){
        if(!m_errorInfo_y.first){
            m_errorInfo_y.first = true;
            ui->label_status_y->setStyleSheet(axis_stateYellow);

            m_errorInfo_y.second = m_errorMap.value(axisState.error_y, "Unknown error");
        }
    }
    else if(axisState.state_y > 1){
        if(m_errorInfo_y.first){
            m_errorInfo_y.first = false;
            ui->label_status_y->setStyleSheet(axis_stateGreen);
        }
    }

    //z轴
    if(0 == axisState.state_z){
        if(!m_errorInfo_z.first){
            m_errorInfo_z.first = true;
            ui->label_status_z->setStyleSheet(axis_stateRed);

            m_errorInfo_z.second = m_errorMap.value(axisState.error_z, "Unknown error");
        }
    }
    else if(1 == axisState.state_z){
        if(!m_errorInfo_z.first){
            m_errorInfo_z.first = true;
            ui->label_status_z->setStyleSheet(axis_stateYellow);

            m_errorInfo_z.second = m_errorMap.value(axisState.error_z, "Unknown error");
        }
    }
    else if(axisState.state_z > 1){
        if(m_errorInfo_z.first){
            m_errorInfo_z.first = false;
            ui->label_status_z->setStyleSheet(axis_stateGreen);
        }
    }

    //更新状态栏错误信息显示
    QString errorInfo = "";
    if(m_errorInfo_x.first) {
        errorInfo.append(QString(u8"X轴错误%1：%2; ").arg(axisState.error_x).arg(m_errorInfo_x.second));
    }
    if(m_errorInfo_y.first) {
        errorInfo.append(QString(u8"Y轴错误%1：%2; ").arg(axisState.error_y).arg(m_errorInfo_y.second));
    }
    if(m_errorInfo_z.first) {
        errorInfo.append(QString(u8"Z轴错误%1：%2; ").arg(axisState.error_z).arg(m_errorInfo_z.second));
    }

    ui->statusbar->showMessage(errorInfo);
}

void MainWindow::updateAxis_PositionToView(AxisPos axisPos)
{
    IsXBusying = m_pMotionController->IsAxisBusying(AxisID::X);
    IsYBusying = m_pMotionController->IsAxisBusying(AxisID::Y);
    IsZBusying = m_pMotionController->IsAxisBusying(AxisID::Z);
   
    m_axisPos = axisPos;

    ui->label_positionFeedback_x->setText(QString::number(axisPos.pos_x, 'f', 3));
    ui->label_positionFeedback_y->setText(QString::number(axisPos.pos_y, 'f', 3));
    ui->label_positionFeedback_z->setText(QString::number(axisPos.pos_z, 'f', 3));
}

void MainWindow::on_pushButton_clearError_clicked()
{
    m_pMotionController->clearError();
}


void MainWindow::on_pushButton_setXY_velocity_clicked()
{
    QString strVelocity = ui->lineEdit_xy_velocity->text();
    if (strVelocity.isEmpty()) {
        return;
    }
    double dVelocity = strVelocity.toDouble();
    m_pMotionController->setVelocity(AxisID::X, dVelocity, dVelocity * 10, dVelocity * 10);
    m_pMotionController->setVelocity(AxisID::Y, dVelocity, dVelocity * 10, dVelocity * 10);
}


void MainWindow::on_pushButton_setZ_velocity_clicked()
{
    QString strVelocity = ui->lineEdit_z_velocity->text();
    if (strVelocity.isEmpty()) {
        return;
    }
    double dVelocity = strVelocity.toDouble();
    m_pMotionController->setVelocity(AxisID::Z, dVelocity, dVelocity * 10, dVelocity * 10);

}

bool MainWindow::fuzzyEqual(double a, double b, double epsilon)
{
    if (std::isnan(a) || std::isnan(b)) return false;
    if (a == b) return true; // 快速路径
    if (std::isinf(a) || std::isinf(b)) return a == b;

    const double absDiff = std::abs(a - b);
    const double maxVal = std::max({1.0, std::abs(a), std::abs(b)});
    return absDiff <= epsilon * maxVal;
}

void MainWindow::waitInPos(QPointF dstPosition)
{
    while (!(fuzzyEqual(dstPosition.x(), m_axisPos.pos_x) && fuzzyEqual(dstPosition.y(), m_axisPos.pos_y))) {
        QThread::msleep(100);
    }
    while (!(3 == m_axisState.state_x && 3 == m_axisState.state_y)) {
        QThread::msleep(100);
    }
}

void MainWindow::waitInPos_z(double dstPosition)
{
    while (!(fuzzyEqual(dstPosition, m_axisPos.pos_z))) {
        QThread::msleep(100);
    }
    while (3 != m_axisState.state_z) {
        QThread::msleep(100);
    }
}

void MainWindow::JudgeAxisPresetPosition()
{
    m_bStopJudgePresetPos = false;

    double standard_1_x_min = m_dialogSetting.getSettings().posStandard1X.toDouble() - 6;
    double standard_1_x_max = m_dialogSetting.getSettings().posStandard1X.toDouble() + 6;
    double standard_1_y_min = m_dialogSetting.getSettings().posStandard1Y.toDouble() - 3;
    double standard_1_y_max = m_dialogSetting.getSettings().posStandard1Y.toDouble() + 3;

    double standard_2_x_min = m_dialogSetting.getSettings().posStandard2X.toDouble() - 6;
    double standard_2_x_max = m_dialogSetting.getSettings().posStandard2X.toDouble() + 6;
    double standard_2_y_min = m_dialogSetting.getSettings().posStandard2Y.toDouble() - 3;
    double standard_2_y_max = m_dialogSetting.getSettings().posStandard2Y.toDouble() + 3;

    double standard_3_x_min = m_dialogSetting.getSettings().posStandard3X.toDouble() - 6;
    double standard_3_x_max = m_dialogSetting.getSettings().posStandard3X.toDouble() + 6;
    double standard_3_y_min = m_dialogSetting.getSettings().posStandard3Y.toDouble() - 3;
    double standard_3_y_max = m_dialogSetting.getSettings().posStandard3Y.toDouble() + 3;

    double standard_4_x_min = m_dialogSetting.getSettings().posStandard4X.toDouble() - 6;
    double standard_4_x_max = m_dialogSetting.getSettings().posStandard4X.toDouble() + 6;
    double standard_4_y_min = m_dialogSetting.getSettings().posStandard4Y.toDouble() - 3;
    double standard_4_y_max = m_dialogSetting.getSettings().posStandard4Y.toDouble() + 3;

    double load_x = m_dialogSetting.getSettings().posLoadX.toDouble();
    double load_y = m_dialogSetting.getSettings().posLoadY.toDouble();


    qint64 lastCalTimeStamp_500_1500 = m_dialogSetting.getSettings().lastCalTimeStamp_500_1500.toULongLong();
    qint64 lastCalTimeStamp_1550 = m_dialogSetting.getSettings().lastCalTimeStamp_1550.toULongLong();
    double lastZDefault_123 = m_dialogSetting.getSettings().z_default_123.toDouble();
    double lastZDdfault_4 = m_dialogSetting.getSettings().z_default_4.toDouble();
    double lastZValue = lastCalTimeStamp_500_1500 >= lastCalTimeStamp_1550 ? lastZDefault_123 : lastZDdfault_4;

    QtConcurrent::run([=]()
    {
        double totalVal = 0.0;
        double currentStandardVal = 0.0;

        QThread::msleep(2000);
        m_IsLinkedFail = false;
        while(!m_bStopJudgePresetPos)
        {
            QThread::msleep(200);
            
            if (!m_bIsInitFinished_xy)
            {
                bool isError = m_pMotionController->getConnectFailState()|| m_pColorFocusControl_top->getConnectFailState() || m_pColorFocusControl_bottom->getConnectFailState();
                if (isError) {
                    QMetaObject::invokeMethod(this, [=]()
                        {
                        
                            m_spinner->stop();
                          
                            QMessageBox::warning(this, u8"提示:", u8"运动平台或共聚焦未连接请检查线路或稍后再尝试！");
                            m_IsLinkedFail =  true;
                        }, Qt::QueuedConnection);
                    m_bStopJudgePresetPos = true;
                    continue;
                }
            }
            //判断XY轴位置
            if(m_axisPos.pos_x > standard_1_x_min && m_axisPos.pos_x < standard_1_x_max
                && m_axisPos.pos_y > standard_1_y_min && m_axisPos.pos_y < standard_1_y_max)
            {
                QMetaObject::invokeMethod(this, [&]()
                    {
                        totalVal = m_dialogSetting.getSettings().standardTotalVal1.toDouble();
                        currentStandardVal = totalVal - m_current_distance_top - m_current_distance_bottom;
                        ui->label_topSurface_1->setText(QString::number(m_current_distance_top, 'f', 3));
                        ui->label_bottomSurface_1->setText(QString::number(m_current_distance_bottom, 'f', 3));
                        ui->label_standard_1->setText(QString::number(currentStandardVal, 'f', 1));

                        ui->label_standardPart_position_1->setStyleSheet(preset_stateGreen);
                    }, Qt::QueuedConnection);
            }
            else if(m_axisPos.pos_x > standard_2_x_min && m_axisPos.pos_x < standard_2_x_max
                && m_axisPos.pos_y > standard_2_y_min && m_axisPos.pos_y < standard_2_y_max)
            {
                QMetaObject::invokeMethod(this, [&]()
                    {
                        totalVal = m_dialogSetting.getSettings().standardTotalVal2.toDouble();
                        currentStandardVal = totalVal - m_current_distance_top - m_current_distance_bottom;
                        ui->label_topSurface_2->setText(QString::number(m_current_distance_top, 'f', 3));
                        ui->label_bottomSurface_2->setText(QString::number(m_current_distance_bottom, 'f', 3));
                        ui->label_standard_2->setText(QString::number(currentStandardVal, 'f', 1));

                        ui->label_standardPart_position_2->setStyleSheet(preset_stateGreen);
                    }, Qt::QueuedConnection);
            }
            else if(m_axisPos.pos_x > standard_3_x_min && m_axisPos.pos_x < standard_3_x_max
                     && m_axisPos.pos_y > standard_3_y_min && m_axisPos.pos_y < standard_3_y_max)
            {
                QMetaObject::invokeMethod(this, [&]()
                    {
                        totalVal = m_dialogSetting.getSettings().standardTotalVal3.toDouble();
                        currentStandardVal = totalVal - m_current_distance_top - m_current_distance_bottom;
                        ui->label_topSurface_3->setText(QString::number(m_current_distance_top, 'f', 3));
                        ui->label_bottomSurface_3->setText(QString::number(m_current_distance_bottom, 'f', 3));
                        ui->label_standard_3->setText(QString::number(currentStandardVal, 'f', 1));

                        ui->label_standardPart_position_3->setStyleSheet(preset_stateGreen);
                    }, Qt::QueuedConnection);
            }
            else if(m_axisPos.pos_x > standard_4_x_min && m_axisPos.pos_x < standard_4_x_max
                     && m_axisPos.pos_y > standard_4_y_min && m_axisPos.pos_y < standard_4_y_max)
            {
                QMetaObject::invokeMethod(this, [&]()
                    {
                        totalVal = m_dialogSetting.getSettings().standardTotalVal4.toDouble();
                        currentStandardVal = totalVal - m_current_distance_top - m_current_distance_bottom;
                        ui->label_topSurface_4->setText(QString::number(m_current_distance_top, 'f', 3));
                        ui->label_bottomSurface_4->setText(QString::number(m_current_distance_bottom, 'f', 3));
                        ui->label_standard_4->setText(QString::number(currentStandardVal, 'f', 1));

                        ui->label_standardPart_position_4->setStyleSheet(preset_stateGreen);
                    }, Qt::QueuedConnection);
            }
            else if(m_bEncoderClear&&fuzzyEqual(m_axisPos.pos_x, load_x) && fuzzyEqual(m_axisPos.pos_y, load_y))
            {
                QMetaObject::invokeMethod(this, [=]()
                    {
                        ui->label_loading_position->setStyleSheet(preset_stateGreen);
                    }, Qt::QueuedConnection);

                m_bIsLoading = false;
                if(!m_bIsInitFinished_xy && m_bIsInitFinished_z)
                {
                    QMetaObject::invokeMethod(this, [=]()
                      {
                            ui->verticalWidget_control->setEnabled(true);
                          m_spinner->stop();
                      }, Qt::QueuedConnection);
                    m_bIsInitFinished_xy = true;
                }

                m_currentSystemState = SystemState::Ready;
            }
            else if(fuzzyEqual(m_axisPos.pos_x, m_wait_pos_x)&&(m_wait_pos_x!=-1) && fuzzyEqual(m_axisPos.pos_y, m_wait_pos_y) && (m_wait_pos_y != -1))
            {
                QMetaObject::invokeMethod(this, [=]()
                    {
                        ui->label_waitting_position->setStyleSheet(preset_stateGreen);
                    }, Qt::QueuedConnection);
            }
            else if(fuzzyEqual(m_axisPos.pos_x, 0.0, 1e-2) || fuzzyEqual(m_axisPos.pos_y, 0.0, 1e-2))     
            {
                if (!m_XbEncoderClear)
                {
                    if (fuzzyEqual(m_axisPos.pos_x, 0.0))
                    {
                        QMetaObject::invokeMethod(m_pColorFocusControl_top.data(), [&]()
                            {
                                m_pColorFocusControl_top->XRecenterEncoder();
                            }, Qt::QueuedConnection);
                        QMetaObject::invokeMethod(m_pColorFocusControl_bottom.data(), [&]()
                            {
                                m_pColorFocusControl_bottom->XRecenterEncoder();
                            }, Qt::QueuedConnection);
                        m_XbEncoderClear = true;
                        emit s_writeLog(u8"共聚焦X轴编码器已清零");
                    }
                    else if (!IsXBusying)
                    {
                        QMetaObject::invokeMethod(m_pMotionController.data(), [&]()
                            {
                                m_pMotionController->moveSingleAxisAbsolute(AxisID::X, 0.0);
                                emit s_writeLog(QString(u8"move x to 0.0, current x = %1").arg(m_axisPos.pos_x));
                            }, Qt::QueuedConnection);
                    }
                }
                
                if (!m_YbEncoderClear)
                {
                    if (fuzzyEqual(m_axisPos.pos_y, 0.0))
                    {
                        QMetaObject::invokeMethod(m_pColorFocusControl_top.data(), [&]()
                            {
                                m_pColorFocusControl_top->YRecenterEncoder();
                            }, Qt::QueuedConnection);
                        QMetaObject::invokeMethod(m_pColorFocusControl_bottom.data(), [&]()
                            {
                                m_pColorFocusControl_bottom->YRecenterEncoder();
                            }, Qt::QueuedConnection);
                        m_YbEncoderClear = true;
                        emit s_writeLog(u8"共聚焦Y轴编码器已清零");
                    }
                    else if (!IsYBusying)
                    {
                        QMetaObject::invokeMethod(m_pMotionController.data(), [&]()
                            {
                                m_pMotionController->moveSingleAxisAbsolute(AxisID::Y, 0.0);
                                emit s_writeLog(QString(u8"move y to 0.0, current y = %1").arg(m_axisPos.pos_y));
                            }, Qt::QueuedConnection);
                    }
                }
                
                if (!m_bEncoderClear && m_XbEncoderClear && m_YbEncoderClear&& (!IsXBusying) && (!IsYBusying) && (!IsZBusying))
                {
                    m_bEncoderClear = true;
                    emit s_writeLog(u8"运动平台已回零,共聚焦全部编码器已清零");

                    //设置默认速度
                    double velocity = m_dialogSetting.getSettings().velocityNormal.toDouble();
                    m_pMotionController->setVelocity(AxisID::X, velocity, velocity * 10, velocity * 10);
                    m_pMotionController->setVelocity(AxisID::Y, velocity, velocity * 10, velocity * 10);

                    //如果是初始化中，则回零后移动到上料位
                    if (m_bIsInit_xy)
                    {
                        QMetaObject::invokeMethod(this, [&]() { GoLoadPostion(); }, Qt::QueuedConnection);
                        m_bIsInit_xy = false;
                        m_bIsLoading = true;
                    }
                }

                //waitInPos(QPointF(0.0, 0.0));

                //if(!m_bEncoderClear&&!IsXBusying && !IsYBusying)
                //{
                //    while(!(fuzzyEqual(m_axisPos.pos_x, 0.0, 1e-3) && fuzzyEqual(m_axisPos.pos_y, 0.0, 1e-3)))
                //    {
                //        m_pMotionController->moveSingleAxisAbsolute(AxisID::X, 0.0);
                //        m_pMotionController->moveSingleAxisAbsolute(AxisID::Y, 0.0);

                //        QThread::msleep(500);
                //    }

                //    QMetaObject::invokeMethod(m_pColorFocusControl_top.data(), [&]()
                //                              {
                //                                  m_pColorFocusControl_top->RecenterEncoder();
                //                              }, Qt::QueuedConnection);
                //    QMetaObject::invokeMethod(m_pColorFocusControl_bottom.data(), [&]()
                //                              {
                //                                  m_pColorFocusControl_bottom->RecenterEncoder();
                //                              }, Qt::QueuedConnection);


                //    m_bEncoderClear = true;
                //    emit s_writeLog(u8"运动平台已回零,编码器清零");

                //    //设置默认速度
                //    double velocity = m_dialogSetting.getSettings().velocityNormal.toDouble();
                //    m_pMotionController->setVelocity(AxisID::X, velocity, velocity * 10, velocity * 10);
                //    m_pMotionController->setVelocity(AxisID::Y, velocity, velocity * 10, velocity * 10);

                //    //如果是初始化中，则回零后移动到上料位
                //    if(m_bIsInit_xy)
                //    {
                //        QMetaObject::invokeMethod(this, [&]() { GoLoadPostion(); }, Qt::QueuedConnection);
                //        m_bIsInit_xy = false;
                //        m_bIsLoading = true;
                //    }
                //}
            }
            else
            {
                QMetaObject::invokeMethod(this, [=]()
                    {
                        ui->label_topSurface_1->setText("0.000");
                        ui->label_bottomSurface_1->setText("0.000");

                        ui->label_topSurface_2->setText("0.000");
                        ui->label_bottomSurface_2->setText("0.000");

                        ui->label_topSurface_3->setText("0.000");
                        ui->label_bottomSurface_3->setText("0.000");

                        ui->label_topSurface_4->setText("0.000");
                        ui->label_bottomSurface_4->setText("0.000");

                        ui->label_loading_position->setStyleSheet(preset_stateYellow);
                        ui->label_waitting_position->setStyleSheet(preset_stateYellow);
                        ui->label_standardPart_position_1->setStyleSheet(preset_stateYellow);
                        ui->label_standardPart_position_2->setStyleSheet(preset_stateYellow);
                        ui->label_standardPart_position_3->setStyleSheet(preset_stateYellow);
                        ui->label_standardPart_position_4->setStyleSheet(preset_stateYellow);
                    }, Qt::QueuedConnection);
            }

            int zState = m_pMotionController->m_state_z;
            //判断Z轴位置
            if(fuzzyEqual(m_axisPos.pos_z, 0.0, 1e-3) && (zState==3))     //轴运动中不能操作轴必须轴停稳定后再进行下步操作
            {
                //如果是初始化中，则回零后移动到上次的校准位置
                if(m_bIsInit_z)
                {
                    m_pMotionController->moveSingleAxisAbsolute(AxisID::Z, lastZValue);
                    m_bIsInit_z = false;
                    m_bIsCalAfterInit = false;
                }
            }
            /*else if(fuzzyEqual(m_axisPos.pos_z, lastZValue, 1e-6))
            {
                if(!m_bIsInitFinished_z)
                {
                    m_bIsInitFinished_z = true;
                }
            }*/
            else
            {
                m_bIsInitFinished_z = true;
            }
           
        }

    });

}
/// <summary>
/// 检查测量程式的配方是否准备就绪
/// </summary>
void MainWindow::CheckRecipeState()
{
  
    if (!ui->checkBox_changedRecipe->isChecked())
        return;
    int measurePath = ui->comboBox_path->currentIndex();
    int productThickness = ui->comboBox_thickness->currentIndex();
    int productSize = ui->comboBox_size->currentIndex();
    IsRecipChanged = (measurePath!=lastPathIndex)||(productThickness!=lastThicknessIndex)||(lastSizeIndex!=productSize);

    CheckZGravityState();
}

void MainWindow::CheckZGravityState() 
{
   
    if ((ui->comboBox_path->count() == 0) || (ui->comboBox_thickness->count() == 0) || (ui->comboBox_size->count() == 0))
    {
        return;
    }
    if (!this->isVisible() || !this->isActiveWindow())
        return;
    int measurePath = ui->comboBox_path->currentData().toInt();
    int productThickness = ui->comboBox_thickness->currentData().toInt();
    int productSize = ui->comboBox_size->currentData().toInt();
    bool bParaCorrect = m_pMeasureControl->SetMeasurePara(measurePath, productThickness, productSize);
    ui->pushButton_measure_start->setEnabled(bParaCorrect);
    if (bParaCorrect)
    {
        ui->label_GravityState->setStyleSheet(
            "color: lime;"          // 文字颜色
            "background-color: white;"  // 背景颜色
            );
        ui->label_GravityState->setText(u8"正常");
    }
    else
    {
        ui->label_GravityState->setStyleSheet(
            "color: red;"          // 文字颜色
            "background-color: yellow;"  // 背景颜色
            );
        ui->label_GravityState->setText(u8"缺失");
    }
}

void MainWindow::on_comboBox_path_currentIndexChanged(int index)
{
    Q_UNUSED(index);

    CheckRecipeState();
    int lineCount = ui->comboBox_path->currentData().toInt();
    QString imageFile = QString(u8":/image/main/image/line-%1.jpg").arg(lineCount);
    if (m_dialogSetting.getSettings().IsUseStandard1550Flag.toInt() != 1)
    {
        imageFile = QString(u8":/image/main/image/line-%1.png").arg(lineCount);
    }
    QPixmap pixmap(imageFile);

    ui->label_line_picture->setPixmap(pixmap.scaled(ui->label_line_picture->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void MainWindow::setupConnections()
{
    // connect(&PermissionManager::instance(), &PermissionManager::permissionsChanged,
    //         this, &MainWindow::onPermissionsChanged);

    // connect(ui->actionUserManagement, &QAction::triggered,
    //         this, &MainWindow::showUserManagement);
    // connect(ui->actionPermissionConfig, &QAction::triggered,
    //         this, &MainWindow::showPermissionConfig);
    // connect(ui->actionChangePassword, &QAction::triggered,
    //         this, &MainWindow::changePassword);
}

void MainWindow::updatePermissions()
{
    // bool isAdmin = CurrentUser::instance().role() == 0;

    // ui->actionUserManagement->setVisible(isAdmin);
    // ui->actionPermissionConfig->setVisible(isAdmin);

    // // 根据权限设置模块可用性
    // ui->module1Button->setEnabled(PermissionManager::instance().hasPermission("Module1"));
    // ui->module2Button->setEnabled(PermissionManager::instance().hasPermission("Module2"));
}

void MainWindow::onPermissionsChanged()
{
    updatePermissions();
}

void MainWindow::showUserManagement()
{
    // UserManagementDialog dlg(this);
    // dlg.exec();
}

void MainWindow::showPermissionConfig()
{
    // PermissionConfigDialog dlg(this);
    // dlg.exec();
}

void MainWindow::changePassword()
{
    // PasswordChangeDialog dlg(this);
    // dlg.exec();
}

void MainWindow::on_pushButton_userManage_clicked()
{
    m_dialogUser.exec();
}

void MainWindow::on_comboBox_size_currentIndexChanged(int index)
{
    Q_UNUSED(index);

    if (!m_bIsInitFinished_xy)
    {
        return;
    }
    int productSize = ui->comboBox_size->currentData().toInt();
    CheckRecipeState();
    ParamSettings& settings = m_dialogSetting.getSettings();
    if(12 == productSize){
        m_wait_pos_x = settings.posWaitX_12.toDouble();
        m_wait_pos_y = settings.posWaitY_12.toDouble();
    }else if(8 == productSize){
        m_wait_pos_x = settings.posWaitX_8.toDouble();
        m_wait_pos_y = settings.posWaitY_8.toDouble();
    }
    else if (6 == productSize) {
        m_wait_pos_x = settings.posWaitX_6.toDouble();
        m_wait_pos_y = settings.posWaitY_6.toDouble();
    }
}

void MainWindow::on_pushButton_repiceSetting_clicked()
{
   // m_DialogTemperatureControlDevice.exec();
    m_recipeSettingDialog.exec();
    intComboBox(DefaultSelectRecipeName);
}

void MainWindow::on_comboBox_recipe_currentIndexChanged(int index)
{
    Q_UNUSED(index);

    if (ui->comboBox_path->count()==0)
    {
        return;
    }
  
    QSqlRecord record =m_recipeSettingDialog.FindRecipeByName(ui->comboBox_recipe->currentText());
    bool isReady = !record.isEmpty();
    if (isReady)
    {
        ui->pushButton_loading->setEnabled(true);
        ui->pushButton_readID->setEnabled(true);
        DefaultSelectRecipeName = ui->comboBox_recipe->currentText();
        m_MeasureParamsDialog.InitMeasureParams(DefaultSelectRecipeName);
       QString targetPath= record.value("motion_path").toString();
       lastPathIndex = ui->comboBox_path->findText(targetPath);
       if(lastPathIndex !=-1)
           ui->comboBox_path->setCurrentIndex(lastPathIndex);
       else
       {
           isReady = false;
           ui->comboBox_path->setCurrentIndex(-1);
           QMessageBox::warning(this, u8"错误", QString(u8"所选测量参数没有对应的测量路径，请检查！"));
       }
       QString targetThickness = record.value("product_thickness").toString();
       lastThicknessIndex = ui->comboBox_thickness->findText(targetThickness);
       if (lastThicknessIndex != -1)
           ui->comboBox_thickness->setCurrentIndex(lastThicknessIndex);
       else
       {
           isReady = false;
           ui->comboBox_thickness->setCurrentIndex(-1);
           QMessageBox::warning(this, u8"错误", QString(u8"所选测量参数没有对应的产品标准，请检查！"));
       }
       QString targetSize = record.value("product_size").toString();
       lastSizeIndex = ui->comboBox_size->findText(targetSize);
       if (lastSizeIndex != -1)
           ui->comboBox_size->setCurrentIndex(lastSizeIndex);
       else
       {
           isReady = false;
           ui->comboBox_size->setCurrentIndex(-1);
           QMessageBox::warning(this, u8"错误", QString(u8"所选测量参数没有对应的产品尺寸，请检查！"));
       }
       IsRecipChanged = false;
       m_pMeasureControl->ResetTrimSize(record.value("trim_size").toString());
       m_MeasureParamsDialog.InitMeasureParams(ui->comboBox_recipe->currentText());
       CheckZGravityState();
     
   }
   
   ui->checkBox_changedRecipe->setEnabled(isReady);
   ui->pushButton_ScanGravity->setEnabled(isReady);
}

void MainWindow::on_comboBox_thickness_currentIndexChanged(int index)
{
    Q_UNUSED(index);

    CheckRecipeState();
}

void MainWindow::on_checkBox_changedRecipe_clicked(bool checked)
{
    ui->comboBox_recipe->setEnabled(!checked);
    ui->comboBox_path->setEnabled(checked);
    ui->comboBox_thickness->setEnabled(checked);
    ui->comboBox_size->setEnabled(checked);
}

void MainWindow::on_pushButton_ScanGravity_clicked()
{
    if (QMessageBox::question(this, u8"扫描典型片提示",
        u8"第一步:即将扫描典型片反面数据，请确保有WaferID面朝上，且箭头指向Notch口.") != QMessageBox::Yes)
        return;
    ui->label_GravityState->setStyleSheet(
        "color: black;"          // 文字颜色
        "background-color: yellow;"  // 背景颜色
    );
    ui->label_GravityState->setText(u8"正在生成...");
    ui->pushButton_measure_start->setEnabled(false);
    ui->pushButton_ScanGravity->setEnabled(false);
    m_currentSystemState = SystemState::Ready;

    m_pMeasureControl->InitACQZGravity();
    ProgramMeasureMode = ACQOppsiteGravity;

  
    on_pushButton_measure_start_clicked();
}

void MainWindow::on_checkBox_useRepeatEpochs_clicked(bool checked)
{
  
    ui->spinBox_repeatEpochs->setEnabled(checked);
}

void MainWindow::on_checkBox_changedRecipe_checkStateChanged(const Qt::CheckState &arg1)
{
    Q_UNUSED(arg1);

    bool checked=arg1==Qt::Checked;
    ui->comboBox_recipe->setEnabled(!checked);
    ui->comboBox_path->setEnabled(checked);
    ui->comboBox_thickness->setEnabled(checked);
    ui->comboBox_size->setEnabled(checked);
}

void MainWindow::on_checkBox_useRepeatEpochs_checkStateChanged(const Qt::CheckState &arg1)
{
    Q_UNUSED(arg1);
}

void MainWindow::on_pushButton_save_surface_clicked()
{
    auto pixmap=ChipSurfaceGraph->renderToImage(16);
    QString completeDefaultPath = QString("%1/%2").arg(QDir::homePath()).arg(ui->lineEdit_LotID->text());
    // 保存对话框
    QString filePath = QFileDialog::getSaveFileName(
        this, u8"保存图片", completeDefaultPath, u8"PNG图片 (*.png);;JPEG图片 (*.jpg);;所有文件 (*)"
    );
    // 捕获控件内容为QPixmap
    auto image = ui->widget_plot_chipheatmap->grab();
    QString filePath1 = filePath;
    QString filePath2 = filePath;
    filePath1 = filePath1.replace(".", "_2D.");
    filePath2 = filePath2.replace(".", "_3D.");
    if (!filePath.isEmpty()) {
        // 保存图片
        if (pixmap.save(filePath2)&& image.save(filePath1)) {
            QMessageBox::information(this, u8"成功", u8"图片保存成功！");
        }
        else {
            QMessageBox::warning(this, u8"失败", u8"图片保存失败！");
        }
    }
}

bool MainWindow::SavePaintImageToFile() 
{
    
    if (!m_pMeasureControl->IsSaveImage3D)
        return false;
    QString filePath;
    QDir appDir = QCoreApplication::applicationDirPath();

    if (m_dialogSetting.getSettings().measureFileDir.isEmpty()) {
        filePath = appDir.filePath("MeasureData");
    }
    else {
        filePath = m_dialogSetting.getSettings().measureFileDir;
    }
    ui->tabWidget->setCurrentIndex(1);
    QString ProductName = m_pMeasureControl->ProductName;
    QString m_measurePath = m_pMeasureControl->GetMeasurePath();
    QString m_productSize = m_pMeasureControl->GetProductSize();
    QString m_productThickness = m_pMeasureControl->GetProductThickness();
    QString heatImageFile = QString("%1/%2_%3_%4_%5_2D.png").arg(filePath).arg(ProductName).arg(m_measurePath).arg(m_productSize).arg(m_productThickness);
    QString _3dImageFile = QString("%1/%2_%3_%4_%5_3D.png").arg(filePath).arg(ProductName).arg(m_measurePath).arg(m_productSize).arg(m_productThickness);
    if (!m_pMeasureControl->SaveHeatImageFile.isEmpty() && !m_pMeasureControl->Save3DImageFile.isEmpty()&&((m_pMeasureControl->SaveHeatImageFile.count()>0)&& (m_pMeasureControl->Save3DImageFile.count()>0)))
    {
        heatImageFile = m_pMeasureControl->SaveHeatImageFile;
        _3dImageFile= m_pMeasureControl->Save3DImageFile;
    }
    bool state = false;
    try
    {
        auto pixmap = ChipSurfaceGraph->renderToImage(16);

        auto image = ui->widget_plot_chipheatmap->grab();
        bool state = pixmap.save(heatImageFile) && image.save(_3dImageFile);
    }
    catch (const std::exception&)
    {
        state = false;
    }
   
   return state;
}


void MainWindow::WriteWaferIdToFile(QString waferID)
{
    QString filePath;
    QDir appDir = QCoreApplication::applicationDirPath();

    if (m_dialogSetting.getSettings().measureFileDir.isEmpty()) {
        filePath = appDir.filePath("MeasureData");
    }
    else {
        filePath = m_dialogSetting.getSettings().measureFileDir;
    }

    QDir dir(filePath);
    if (!dir.exists()) {
        dir.mkpath(filePath);
    }

    LastCurrentLot = ui->lineEdit_LotID->text();
    QString currentfileName = QString("%1/WaverID_%2.csv").arg(filePath).arg(LastCurrentLot);

    // 第一次读取判断
    QSet<QString> existingIds;
    {
        QFile checkFile(currentfileName);
        if (checkFile.exists() && checkFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            existingIds = ReadWaferIdToSet(checkFile);
            checkFile.close();
        }
    }

    if (existingIds.contains(waferID)) {
        QMessageBox::information(this, u8"写入文件", u8"该片ID已记录过");
        return;
    }

    // 追加写入
    QFile file(currentfileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QMessageBox::warning(this, u8"写入文件", u8"无法打开文件进行写入");
        return;
    }

    QTextStream out(&file);
    out << waferID << "\n";
    file.close();
}


QSet<QString> MainWindow::ReadWaferIdToSet(QFile& file)
{
    QSet<QString> existingLines;

    QTextStream in(&file);

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (!line.isEmpty()) {
            existingLines.insert(line);
        }
    }

    return existingLines;
}

