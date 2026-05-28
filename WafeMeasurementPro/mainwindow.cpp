#include "mainwindow.h"
#include "parameter_settings_dialog.h"
#include "recipe_manager_dialog.h"
#include "ui_mainwindow.h"

#include <QCloseEvent>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QHeaderView>
#include <QImage>
#include <QAbstractItemView>
#include <QLinearGradient>
#include <QMessageBox>
#include <QMetaObject>
#include <QPainter>
#include <QTextStream>
#include <QThread>
#include <QVBoxLayout>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtConcurrent>
#include <QtDataVisualization/QSurface3DSeries>
#include <QtDataVisualization/QSurfaceDataProxy>
#include <QtMath>

#include <algorithm>
#include <functional>
#include <limits>
#include <numeric>
#include <utility>

using namespace QtCharts;
using namespace QtDataVisualization;

namespace
{
constexpr int kTopSensorId = 0;
constexpr int kBottomSensorId = 1;
constexpr int kWaitTimeoutMs = 120000;
constexpr int kConnectionTimeoutMs = 8000;
constexpr int kSampleIntervalMs = 40;
constexpr double kPositionTolerance = 0.01;
constexpr double kCalibrationScanOffset = 3.0;
constexpr double kCalibrationScanVelocity = 2.0;
constexpr double kCalibrationScanAcc = 40.0;

bool readyState(int state)
{
    return state == 3;
}

QString nowStamp()
{
    return QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss"));
}

bool validFinite(double value)
{
    return qIsFinite(value) && !qFuzzyIsNull(value);
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      m_topFocus(new ColorFocusControl),
      m_bottomFocus(new ColorFocusControl),
      m_focusThread(new QThread(this)),
      m_motionConnected(false),
      m_topConnected(false),
      m_bottomConnected(false),
      m_topAcquiring(false),
      m_bottomAcquiring(false),
      m_initialized(false),
      m_busy(false),
      m_stopRequested(false),
      m_topValid(false),
      m_bottomValid(false),
      m_topDistance(0.0),
      m_bottomDistance(0.0),
      m_lastEncoder(0),
      m_curveView(nullptr),
      m_heatLegendView(nullptr),
      m_surface(nullptr),
      m_surfaceContainer(nullptr)
{
    ui->setupUi(this);
    setupTables();
    setupCharts();
    setupColorFocusThread();
    connectSignals();

    QString error;
    if (!m_recipeDatabase.open(&error)) {
        showError(error);
    } else {
        appendLog(QStringLiteral("Recipe database: %1").arg(m_recipeDatabase.databasePath()));
    }
    if (!loadSettings(&error)) {
        appendLog(error);
    }
    if (!reloadRecipes(&error)) {
        appendLog(error);
    }
    updateCalibrationTable(m_settings);
    updateRecipeLabels();

    m_statusTimer.setInterval(250);
    connect(&m_statusTimer, &QTimer::timeout, this, &MainWindow::refreshStatusLabels);
    m_statusTimer.start();
    statusBar()->showMessage(QStringLiteral("Ready"));
}

MainWindow::~MainWindow()
{
    m_stopRequested = true;
    if (m_future.isRunning()) {
        m_future.waitForFinished();
    }
    if (m_motion.isConnected()) {
        m_motion.abortAxes();
        m_motion.disconnectFromController();
    }
    QString ignored;
    invokeFocus(m_topFocus, [this]() { return m_topFocus->StopAcquisition(); }, QStringLiteral("Stop top acquisition"), &ignored);
    invokeFocus(m_bottomFocus, [this]() { return m_bottomFocus->StopAcquisition(); }, QStringLiteral("Stop bottom acquisition"), &ignored);
    invokeFocus(m_topFocus, [this]() { return m_topFocus->DisconnectDevice(); }, QStringLiteral("Disconnect top focus"), &ignored);
    invokeFocus(m_bottomFocus, [this]() { return m_bottomFocus->DisconnectDevice(); }, QStringLiteral("Disconnect bottom focus"), &ignored);
    if (m_focusThread->isRunning()) {
        m_topFocus->deleteLater();
        m_bottomFocus->deleteLater();
        m_focusThread->quit();
        m_focusThread->wait(3000);
    }
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    m_stopRequested = true;
    if (m_motion.isConnected()) {
        m_motion.abortAxes();
    }
    QMainWindow::closeEvent(event);
}

void MainWindow::setupTables()
{
    ui->table_results->setColumnCount(2);
    ui->table_results->setHorizontalHeaderLabels(QStringList()
        << QString(u8"项目")
        << QString(u8"结果"));
    ui->table_results->verticalHeader()->setVisible(false);
    ui->table_results->horizontalHeader()->setStretchLastSection(true);
    ui->table_results->setEditTriggers(QAbstractItemView::NoEditTriggers);
    const QStringList names = {
        QStringLiteral("BOW"), QStringLiteral("WARP"), QStringLiteral("CENTER_THK"),
        QStringLiteral("AVERAGE_THK"), QStringLiteral("TTV"), QStringLiteral("SORI")
    };
    ui->table_results->setRowCount(names.size());
    for (int i = 0; i < names.size(); ++i) {
        ui->table_results->setItem(i, 0, new QTableWidgetItem(names.at(i)));
        ui->table_results->setItem(i, 1, new QTableWidgetItem(QStringLiteral("--")));
    }

    ui->table_calibration->setColumnCount(3);
    ui->table_calibration->setHorizontalHeaderLabels(QStringList()
        << QString(u8"标准片")
        << QString(u8"标准厚度")
        << QString(u8"校准总值"));
    ui->table_calibration->verticalHeader()->setVisible(false);
    ui->table_calibration->horizontalHeader()->setStretchLastSection(true);
    ui->table_calibration->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->table_calibration->setRowCount(4);
}

void MainWindow::setupCharts()
{
    QChart *curveChart = new QChart;
    curveChart->legend()->setVisible(false);
    curveChart->setTitle(QString(u8"当前线厚度曲线"));
    m_curveView = new QChartView(curveChart, ui->widget_curve);
    m_curveView->setRenderHint(QPainter::Antialiasing);
    QVBoxLayout *curveLayout = new QVBoxLayout(ui->widget_curve);
    curveLayout->setContentsMargins(0, 0, 0, 0);
    curveLayout->addWidget(m_curveView);

    ui->label_heatmap->setMinimumSize(360, 260);
    ui->label_heatmap->setAlignment(Qt::AlignCenter);
    ui->label_heatmap->setText(QStringLiteral("2D"));

    m_surface = new Q3DSurface;
    m_surface->axisX()->setTitle(QStringLiteral("X"));
    m_surface->axisY()->setTitle(QStringLiteral("THK"));
    m_surface->axisZ()->setTitle(QStringLiteral("Y"));
    m_surface->axisX()->setTitleVisible(true);
    m_surface->axisY()->setTitleVisible(true);
    m_surface->axisZ()->setTitleVisible(true);
    m_surfaceContainer = QWidget::createWindowContainer(m_surface, ui->widget_surface);
    QVBoxLayout *surfaceLayout = new QVBoxLayout(ui->widget_surface);
    surfaceLayout->setContentsMargins(0, 0, 0, 0);
    surfaceLayout->addWidget(m_surfaceContainer);
}

void MainWindow::setupColorFocusThread()
{
    m_topFocus->moveToThread(m_focusThread);
    m_bottomFocus->moveToThread(m_focusThread);
    m_focusThread->start();
    QMetaObject::invokeMethod(m_topFocus, [this]() { m_topFocus->SetSensorId(kTopSensorId); }, Qt::BlockingQueuedConnection);
    QMetaObject::invokeMethod(m_bottomFocus, [this]() { m_bottomFocus->SetSensorId(kBottomSensorId); }, Qt::BlockingQueuedConnection);
}

void MainWindow::connectSignals()
{
    connect(ui->button_initialize, &QPushButton::clicked, this, &MainWindow::onInitializeClicked);
    connect(ui->button_recipe_manager, &QPushButton::clicked, this, &MainWindow::onRecipeManagerClicked);
    connect(ui->button_parameter, &QPushButton::clicked, this, &MainWindow::onParameterSettingsClicked);
    connect(ui->button_estop, &QPushButton::clicked, this, &MainWindow::onEmergencyStopClicked);
    connect(ui->combo_recipe, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onRecipeChanged);
    connect(ui->button_calibration, &QPushButton::clicked, this, &MainWindow::onCalibrationClicked);
    connect(ui->button_scan_gravity, &QPushButton::clicked, this, &MainWindow::onScanGravityClicked);
    connect(ui->button_load, &QPushButton::clicked, this, &MainWindow::onLoadClicked);
    connect(ui->button_measure, &QPushButton::clicked, this, &MainWindow::onMeasureClicked);
    connect(ui->button_stop, &QPushButton::clicked, this, &MainWindow::onStopClicked);

    connect(&m_motion, &MotionController::connectionStatusChanged, this, &MainWindow::onMotionConnected);
    connect(&m_motion, &MotionController::positionUpdated, this, &MainWindow::onMotionPositionUpdated);
    connect(&m_motion, &MotionController::stateUpdated, this, &MainWindow::onMotionStateUpdated);
    connect(&m_motion, &MotionController::logMessage, this, &MainWindow::appendLog);
    connect(&m_motion, &MotionController::errorMessage, this, &MainWindow::appendLog);
    connect(&m_motion, &MotionController::stateNotReady, this, &MainWindow::appendLog);

    connect(m_topFocus, &ColorFocusControl::sampleUpdated, this, &MainWindow::onColorFocusSampleUpdated);
    connect(m_bottomFocus, &ColorFocusControl::sampleUpdated, this, &MainWindow::onColorFocusSampleUpdated);
    connect(m_topFocus, &ColorFocusControl::connectionStateChanged, this, [this](int, bool connected) {
        m_topConnected = connected;
        refreshStatusLabels();
    });
    connect(m_bottomFocus, &ColorFocusControl::connectionStateChanged, this, [this](int, bool connected) {
        m_bottomConnected = connected;
        refreshStatusLabels();
    });
    connect(m_topFocus, &ColorFocusControl::acquisitionStateChanged, this, [this](int, bool acquiring) {
        m_topAcquiring = acquiring;
        refreshStatusLabels();
    });
    connect(m_bottomFocus, &ColorFocusControl::acquisitionStateChanged, this, [this](int, bool acquiring) {
        m_bottomAcquiring = acquiring;
        refreshStatusLabels();
    });
    connect(m_topFocus, &ColorFocusControl::logMessage, this, &MainWindow::appendLog);
    connect(m_bottomFocus, &ColorFocusControl::logMessage, this, &MainWindow::appendLog);
    connect(m_topFocus, &ColorFocusControl::errorOccurred, this, [this](int, const QString &message) { appendLog(message); });
    connect(m_bottomFocus, &ColorFocusControl::errorOccurred, this, [this](int, const QString &message) { appendLog(message); });
}

bool MainWindow::loadSettings(QString *errorMessage)
{
    ParamSettings settings;
    if (!m_config.loadFromXml(&settings, errorMessage)) {
        return false;
    }
    if (!validateSettings(settings, errorMessage)) {
        return false;
    }
    m_settings = settings;
    return true;
}

bool MainWindow::validateSettings(const ParamSettings &settings, QString *errorMessage) const
{
    if (settings.axisIp.isEmpty() || settings.axisPort.isEmpty() ||
        settings.colorFocusIpTop.isEmpty() || settings.colorFocusIpBottom.isEmpty()) {
        *errorMessage = QStringLiteral("config.xml hardware address is incomplete.");
        return false;
    }
    bool ok = false;
    settings.axisPort.toUShort(&ok);
    if (!ok) {
        *errorMessage = QStringLiteral("AxisPort is invalid.");
        return false;
    }
    const QStringList requiredDoubleFields = {
        settings.posLoadX, settings.posLoadY, settings.velocityNormal, settings.velocityMeasure,
        settings.posStandard1X, settings.posStandard1Y, settings.posStandard2X, settings.posStandard2Y,
        settings.posStandard3X, settings.posStandard3Y, settings.posStandard4X, settings.posStandard4Y,
        settings.standardThickness1, settings.standardThickness2, settings.standardThickness3, settings.standardThickness4,
        settings.standardTotalVal1, settings.standardTotalVal2, settings.standardTotalVal3, settings.standardTotalVal4
    };
    for (const QString &text : requiredDoubleFields) {
        text.toDouble(&ok);
        if (!ok) {
            *errorMessage = QStringLiteral("config.xml numeric value is invalid: %1").arg(text);
            return false;
        }
    }
    return true;
}

bool MainWindow::reloadRecipes(QString *errorMessage)
{
    m_recipes = m_recipeDatabase.recipes(errorMessage);
    if (!errorMessage->isEmpty()) {
        return false;
    }
    ui->combo_recipe->blockSignals(true);
    ui->combo_recipe->clear();
    for (const ProRecipe &recipe : std::as_const(m_recipes)) {
        ui->combo_recipe->addItem(recipe.name, recipe.id);
    }
    ui->combo_recipe->blockSignals(false);
    if (!m_recipes.isEmpty()) {
        ui->combo_recipe->setCurrentIndex(0);
    }
    updateRecipeLabels();
    return true;
}

ProRecipe MainWindow::currentRecipe() const
{
    const int index = ui->combo_recipe->currentIndex();
    if (index >= 0 && index < m_recipes.size()) {
        return m_recipes.at(index);
    }
    return ProRecipe();
}

void MainWindow::setBusy(bool busy, const QString &message)
{
    m_busy = busy;
    ui->button_initialize->setEnabled(!busy);
    ui->button_recipe_manager->setEnabled(!busy);
    ui->button_parameter->setEnabled(!busy);
    ui->combo_recipe->setEnabled(!busy);
    ui->button_calibration->setEnabled(!busy);
    ui->button_scan_gravity->setEnabled(!busy);
    ui->button_load->setEnabled(!busy);
    ui->button_measure->setEnabled(!busy);
    statusBar()->showMessage(message);
}

void MainWindow::appendLog(const QString &message)
{
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, [this, message]() { appendLog(message); }, Qt::QueuedConnection);
        return;
    }
    const QString line = QStringLiteral("[%1] %2")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")), message);
    ui->plain_log->appendPlainText(line);
    statusBar()->showMessage(message, 5000);
}

void MainWindow::showError(const QString &message)
{
    appendLog(message);
    QMessageBox::warning(this, QString(u8"错误"), message);
}

void MainWindow::updateRecipeLabels()
{
    const ProRecipe recipe = currentRecipe();
    ui->label_recipe_detail->setText(QStringLiteral("%1 | path=%2 size=%3 thickness=%4 sample=%5 trim=%6")
        .arg(recipe.name)
        .arg(recipe.measurePath)
        .arg(recipe.productSize)
        .arg(recipe.productThickness)
        .arg(recipe.lineSampleNum)
        .arg(recipe.trimSize, 0, 'f', 3));
}

void MainWindow::refreshStatusLabels()
{
    Motion::AxisPosition position;
    Motion::AxisStateSnapshot state;
    double top = 0.0;
    double bottom = 0.0;
    bool topValid = false;
    bool bottomValid = false;
    {
        QMutexLocker locker(&m_stateMutex);
        position = m_position;
        state = m_axisState;
        top = m_topDistance;
        bottom = m_bottomDistance;
        topValid = m_topValid;
        bottomValid = m_bottomValid;
    }
    ui->label_axis_position->setText(position.valid
        ? QStringLiteral("X=%1  Y=%2  Z=%3").arg(position.x, 0, 'f', 3).arg(position.y, 0, 'f', 3).arg(position.z, 0, 'f', 3)
        : QStringLiteral("--"));
    ui->label_axis_state->setText(state.valid
        ? QStringLiteral("SX=%1 SY=%2 SZ=%3 EX=%4 EY=%5 EZ=%6")
              .arg(state.stateX).arg(state.stateY).arg(state.stateZ).arg(state.errorX).arg(state.errorY).arg(state.errorZ)
        : QStringLiteral("--"));
    ui->label_focus_value->setText(QStringLiteral("Top=%1  Bottom=%2")
        .arg(topValid ? QString::number(top, 'f', 4) : QStringLiteral("--"))
        .arg(bottomValid ? QString::number(bottom, 'f', 4) : QStringLiteral("--")));
    ui->label_device_state->setText(QStringLiteral("Motion:%1 Top:%2/%3 Bottom:%4/%5")
        .arg(m_motionConnected ? QStringLiteral("ON") : QStringLiteral("OFF"))
        .arg(m_topConnected ? QStringLiteral("ON") : QStringLiteral("OFF"))
        .arg(m_topAcquiring ? QStringLiteral("ACQ") : QStringLiteral("STOP"))
        .arg(m_bottomConnected ? QStringLiteral("ON") : QStringLiteral("OFF"))
        .arg(m_bottomAcquiring ? QStringLiteral("ACQ") : QStringLiteral("STOP")));
}

void MainWindow::onInitializeClicked()
{
    if (m_busy) {
        return;
    }
    QString error;
    if (!loadSettings(&error)) {
        showError(error);
        return;
    }
    setBusy(true, QString(u8"正在初始化"));
    m_stopRequested = false;
    if (!initializeDevices(&error)) {
        m_initialized = false;
        setBusy(false, QStringLiteral("Ready"));
        showError(error);
        return;
    }
    m_initialized = true;
    setBusy(false, QStringLiteral("Ready"));
    appendLog(QString(u8"初始化完成"));
}

void MainWindow::onRecipeManagerClicked()
{
    RecipeManagerDialog dialog(&m_recipeDatabase, this);
    connect(&dialog, &RecipeManagerDialog::recipesChanged, this, [this]() {
        QString error;
        if (!reloadRecipes(&error)) {
            showError(error);
        }
    });
    dialog.exec();
    QString error;
    if (!reloadRecipes(&error)) {
        showError(error);
    }
}

void MainWindow::onParameterSettingsClicked()
{
    ParameterSettingsDialog dialog(this);
    connect(&dialog, &ParameterSettingsDialog::settingsSaved, this, [this](const ParamSettings &settings) {
        m_settings = settings;
        updateCalibrationTable(m_settings);
        appendLog(QString(u8"参数已保存"));
    });
    dialog.exec();
}

void MainWindow::onEmergencyStopClicked()
{
    m_stopRequested = true;
    QString error;
    if (!invokeMotion([this]() { return m_motion.abortAxes(); }, QStringLiteral("Emergency stop"), &error)) {
        appendLog(error);
    }
    QString ignored;
    invokeFocus(m_topFocus, [this]() { return m_topFocus->StopAcquisition(); }, QStringLiteral("Stop top acquisition"), &ignored);
    invokeFocus(m_bottomFocus, [this]() { return m_bottomFocus->StopAcquisition(); }, QStringLiteral("Stop bottom acquisition"), &ignored);
    m_initialized = false;
    setBusy(false, QStringLiteral("Ready"));
    appendLog(QString(u8"急停已触发"));
}

void MainWindow::onRecipeChanged(int)
{
    updateRecipeLabels();
}

void MainWindow::onCalibrationClicked()
{
    if (!m_initialized) {
        showError(QStringLiteral("Device is not initialized."));
        return;
    }
    if (m_busy) {
        return;
    }
    ParamSettings settings = m_settings;
    setBusy(true, QString(u8"校准中"));
    m_stopRequested = false;
    m_future = QtConcurrent::run([this, settings]() mutable {
        QString error;
        const bool ok = runCalibration(settings, &error);
        QString loadError;
        if (!moveLoadPosition(&loadError)) {
            error = error.isEmpty() ? loadError : QStringLiteral("%1; %2").arg(error, loadError);
        }
        QMetaObject::invokeMethod(this, [this, ok, error, settings]() {
            if (ok) {
                m_settings = settings;
                updateCalibrationTable(m_settings);
                appendLog(QString(u8"校准完成"));
            } else {
                showError(error);
            }
            setBusy(false, QStringLiteral("Ready"));
        }, Qt::QueuedConnection);
    });
}

void MainWindow::onScanGravityClicked()
{
    if (!m_initialized) {
        showError(QStringLiteral("Device is not initialized."));
        return;
    }
    if (m_busy) {
        return;
    }
    const ProRecipe recipe = currentRecipe();
    setBusy(true, QString(u8"扫描重力补偿"));
    m_stopRequested = false;
    m_future = QtConcurrent::run([this, recipe]() {
        ProRunResult result;
        QString error;
        bool ok = scanRecipeLines(recipe, true, &result, &error);
        if (ok) {
            ok = saveGravityFiles(recipe, result, &error);
        }
        QString loadError;
        if (!moveLoadPosition(&loadError)) {
            error = error.isEmpty() ? loadError : QStringLiteral("%1; %2").arg(error, loadError);
            ok = false;
        }
        QMetaObject::invokeMethod(this, [this, ok, result, error]() {
            if (ok) {
                updateCurve(result.linePoints.isEmpty() ? QVector<ProMeasurePoint>() : result.linePoints.last());
                updateHeatMap(result);
                updateSurface(result);
                appendLog(QString(u8"重力补偿文件已生成"));
            } else {
                showError(error);
            }
            setBusy(false, QStringLiteral("Ready"));
        }, Qt::QueuedConnection);
    });
}

void MainWindow::onLoadClicked()
{
    if (!m_initialized) {
        showError(QStringLiteral("Device is not initialized."));
        return;
    }
    if (m_busy) {
        return;
    }
    setBusy(true, QString(u8"移动到上下料位"));
    m_future = QtConcurrent::run([this]() {
        QString error;
        const bool ok = moveLoadPosition(&error);
        QMetaObject::invokeMethod(this, [this, ok, error]() {
            if (!ok) {
                showError(error);
            } else {
                appendLog(QString(u8"已到达上下料位"));
            }
            setBusy(false, QStringLiteral("Ready"));
        }, Qt::QueuedConnection);
    });
}

void MainWindow::onMeasureClicked()
{
    if (!m_initialized) {
        showError(QStringLiteral("Device is not initialized."));
        return;
    }
    if (m_busy) {
        return;
    }
    const ProRecipe recipe = currentRecipe();
    QMap<int, QMap<int, double>> gravity;
    QString error;
    if (!loadGravityFiles(recipe, &gravity, &error)) {
        showError(error);
        return;
    }
    setBusy(true, QString(u8"测量中"));
    m_stopRequested = false;
    m_future = QtConcurrent::run([this, recipe]() {
        ProRunResult result;
        QString error;
        bool ok = scanRecipeLines(recipe, false, &result, &error);
        if (ok) {
            Wafer::WaferDataset dataset = buildDataset(recipe, result);
            Wafer::WaferAlgorithm algorithm;
            result.algorithm = algorithm.runAll(dataset, algorithmOptions(recipe));
            if (!result.algorithm.success) {
                ok = false;
                error = result.algorithm.errorMessage;
            }
        }
        if (ok) {
            ok = writeResultFiles(&result, &error);
        }
        QString loadError;
        if (!moveLoadPosition(&loadError)) {
            error = error.isEmpty() ? loadError : QStringLiteral("%1; %2").arg(error, loadError);
            ok = false;
        }
        result.success = ok;
        result.errorMessage = error;
        QMetaObject::invokeMethod(this, [this, result]() {
            if (result.success) {
                finishRunOnUi(result, QString(u8"测量完成"));
            } else {
                showError(result.errorMessage);
            }
            setBusy(false, QStringLiteral("Ready"));
        }, Qt::QueuedConnection);
    });
}

void MainWindow::onStopClicked()
{
    m_stopRequested = true;
    QString error;
    if (!invokeMotion([this]() { return m_motion.abortAxes(); }, QStringLiteral("Stop motion"), &error)) {
        appendLog(error);
    }
    appendLog(QString(u8"停止请求已发出"));
}

void MainWindow::onMotionConnected(bool connected)
{
    m_motionConnected = connected;
    refreshStatusLabels();
}

void MainWindow::onMotionPositionUpdated(const Motion::AxisPosition &position)
{
    QMutexLocker locker(&m_stateMutex);
    m_position = position;
}

void MainWindow::onMotionStateUpdated(const Motion::AxisStateSnapshot &state)
{
    QMutexLocker locker(&m_stateMutex);
    m_axisState = state;
}

void MainWindow::onColorFocusSampleUpdated(int sensorId, float distance, float, int encoder1, int encoder2, int encoder3)
{
    QMutexLocker locker(&m_stateMutex);
    if (sensorId == kTopSensorId) {
        m_topDistance = distance;
        m_topValid = validFinite(distance);
    } else if (sensorId == kBottomSensorId) {
        m_bottomDistance = distance;
        m_bottomValid = validFinite(distance);
    }
    m_lastEncoder = encoder1 != 0 ? encoder1 : (encoder2 != 0 ? encoder2 : encoder3);
}

bool MainWindow::waitFor(std::function<bool()> predicate, int timeoutMs, const QString &timeoutMessage, QString *errorMessage)
{
    QElapsedTimer timer;
    timer.start();
    while (!m_stopRequested) {
        if (predicate()) {
            return true;
        }
        if (timer.elapsed() > timeoutMs) {
            *errorMessage = timeoutMessage;
            return false;
        }
        if (QThread::currentThread() == thread()) {
            qApp->processEvents(QEventLoop::AllEvents, 20);
        }
        QThread::msleep(20);
    }
    *errorMessage = QStringLiteral("Operation canceled.");
    return false;
}

bool MainWindow::waitForMotionConnection(QString *errorMessage)
{
    return waitFor([this]() { return m_motionConnected; },
                   kConnectionTimeoutMs,
                   QStringLiteral("Motion controller connection timeout."),
                   errorMessage);
}

bool MainWindow::waitForXy(double x, double y, QString *errorMessage)
{
    return waitFor([this, x, y]() {
        QMutexLocker locker(&m_stateMutex);
        return m_position.valid && m_axisState.valid &&
               qAbs(m_position.x - x) <= kPositionTolerance &&
               qAbs(m_position.y - y) <= kPositionTolerance &&
               readyState(m_axisState.stateX) && readyState(m_axisState.stateY);
    }, kWaitTimeoutMs, QStringLiteral("Wait XY timeout. target=(%1,%2)").arg(x).arg(y), errorMessage);
}

bool MainWindow::waitForZ(double z, QString *errorMessage)
{
    return waitFor([this, z]() {
        QMutexLocker locker(&m_stateMutex);
        return m_position.valid && m_axisState.valid &&
               qAbs(m_position.z - z) <= kPositionTolerance &&
               readyState(m_axisState.stateZ);
    }, kWaitTimeoutMs, QStringLiteral("Wait Z timeout. target=%1").arg(z), errorMessage);
}

bool MainWindow::waitForFocusSamples(QString *errorMessage)
{
    return waitFor([this]() {
        QMutexLocker locker(&m_stateMutex);
        return m_topValid && m_bottomValid;
    }, kConnectionTimeoutMs, QStringLiteral("Color focus valid sample timeout."), errorMessage);
}

bool MainWindow::invokeMotion(const std::function<bool()> &call, const QString &action, QString *errorMessage)
{
    bool ok = false;
    if (QThread::currentThread() == m_motion.thread()) {
        ok = call();
    } else {
        const bool invoked = QMetaObject::invokeMethod(&m_motion, [&]() { ok = call(); }, Qt::BlockingQueuedConnection);
        if (!invoked) {
            *errorMessage = QStringLiteral("%1 failed, invokeMethod failed.").arg(action);
            return false;
        }
    }
    if (!ok) {
        *errorMessage = QStringLiteral("%1 failed.").arg(action);
        return false;
    }
    return true;
}

bool MainWindow::invokeFocus(ColorFocusControl *control, const std::function<bool()> &call, const QString &action, QString *errorMessage)
{
    if (!control) {
        *errorMessage = QStringLiteral("%1 failed, focus control is null.").arg(action);
        return false;
    }
    bool ok = false;
    if (QThread::currentThread() == control->thread()) {
        ok = call();
    } else {
        const bool invoked = QMetaObject::invokeMethod(control, [&]() { ok = call(); }, Qt::BlockingQueuedConnection);
        if (!invoked) {
            *errorMessage = QStringLiteral("%1 failed, invokeMethod failed.").arg(action);
            return false;
        }
    }
    if (!ok) {
        *errorMessage = QStringLiteral("%1 failed.").arg(action);
        return false;
    }
    return true;
}

bool MainWindow::initializeDevices(QString *errorMessage)
{
    if (m_motion.isConnected()) {
        m_motion.disconnectFromController();
    }
    m_motionConnected = false;
    m_motion.connectToController(m_settings.axisIp, m_settings.axisPort.toUShort());
    if (!waitForMotionConnection(errorMessage)) {
        return false;
    }

    if (!invokeMotion([this]() { return m_motion.clearError(); }, QStringLiteral("Clear motion errors"), errorMessage)) {
        return false;
    }
    if (!invokeMotion([this]() { return m_motion.enableAxis(Motion::AxisX); }, QStringLiteral("Enable X axis"), errorMessage) ||
        !invokeMotion([this]() { return m_motion.enableAxis(Motion::AxisY); }, QStringLiteral("Enable Y axis"), errorMessage) ||
        !invokeMotion([this]() { return m_motion.enableAxis(Motion::AxisZ); }, QStringLiteral("Enable Z axis"), errorMessage)) {
        return false;
    }
    if (!invokeMotion([this]() { return m_motion.ctrlAutoSnd(100); }, QStringLiteral("Start motion position upload"), errorMessage)) {
        return false;
    }
    if (!invokeMotion([this]() { return m_motion.readRealTimePos(); }, QStringLiteral("Read motion position"), errorMessage) ||
        !invokeMotion([this]() { return m_motion.readRealTimeStatus(); }, QStringLiteral("Read motion status"), errorMessage)) {
        return false;
    }
    if (!waitFor([this]() {
            QMutexLocker locker(&m_stateMutex);
            return m_axisState.valid && readyState(m_axisState.stateX) &&
                   readyState(m_axisState.stateY) && readyState(m_axisState.stateZ);
        }, kConnectionTimeoutMs, QStringLiteral("Axis state is not ready after enable."), errorMessage)) {
        return false;
    }
    const double normalVelocity = m_settings.velocityNormal.toDouble();
    if (!invokeMotion([this, normalVelocity]() { return m_motion.setVelocity(Motion::AxisX, normalVelocity, normalVelocity * 10.0, normalVelocity * 10.0); }, QStringLiteral("Set X velocity"), errorMessage) ||
        !invokeMotion([this, normalVelocity]() { return m_motion.setVelocity(Motion::AxisY, normalVelocity, normalVelocity * 10.0, normalVelocity * 10.0); }, QStringLiteral("Set Y velocity"), errorMessage) ||
        !invokeMotion([this, normalVelocity]() { return m_motion.setVelocity(Motion::AxisZ, normalVelocity, normalVelocity * 10.0, normalVelocity * 10.0); }, QStringLiteral("Set Z velocity"), errorMessage)) {
        return false;
    }

    if (!invokeFocus(m_topFocus, [this]() { return m_topFocus->ConnectDevice(m_settings.colorFocusIpTop); }, QStringLiteral("Connect top color focus"), errorMessage) ||
        !invokeFocus(m_bottomFocus, [this]() { return m_bottomFocus->ConnectDevice(m_settings.colorFocusIpBottom); }, QStringLiteral("Connect bottom color focus"), errorMessage)) {
        return false;
    }
    const int scanRate = m_settings.scanRate.toInt();
    if (!invokeFocus(m_topFocus, [this, scanRate]() { return m_topFocus->StartAcquisition(TriggerContinue, qMax(20, 1000 / qMax(1, scanRate))); }, QStringLiteral("Start top acquisition"), errorMessage) ||
        !invokeFocus(m_bottomFocus, [this, scanRate]() { return m_bottomFocus->StartAcquisition(TriggerContinue, qMax(20, 1000 / qMax(1, scanRate))); }, QStringLiteral("Start bottom acquisition"), errorMessage)) {
        return false;
    }
    if (!waitForFocusSamples(errorMessage)) {
        return false;
    }
    return true;
}

bool MainWindow::moveLoadPosition(QString *errorMessage)
{
    return moveXy(m_settings.posLoadX.toDouble(), m_settings.posLoadY.toDouble(), m_settings.velocityNormal.toDouble(), errorMessage);
}

bool MainWindow::moveXy(double x, double y, double velocity, QString *errorMessage)
{
    if (!invokeMotion([this, x, y, velocity]() {
            return m_motion.moveMultiAxisLinear(x, y, velocity, velocity * 10.0, velocity * 10.0);
        }, QStringLiteral("Move XY"), errorMessage)) {
        return false;
    }
    return waitForXy(x, y, errorMessage);
}

bool MainWindow::moveZ(double z, QString *errorMessage)
{
    if (!invokeMotion([this, z]() { return m_motion.moveSingleAxisAbsolute(Motion::AxisZ, z); }, QStringLiteral("Move Z"), errorMessage)) {
        return false;
    }
    return waitForZ(z, errorMessage);
}

bool MainWindow::runCalibration(ParamSettings settings, QString *errorMessage)
{
    if (!moveZ(settings.z_default_123.toDouble(), errorMessage)) {
        return false;
    }
    QVector<StandardSpec> standards = standardsFromSettings(settings, errorMessage);
    if (standards.isEmpty()) {
        return false;
    }
    const bool scanX = settings.CalibrationDirectionOfX.toInt() == 1;
    const double dx = scanX ? kCalibrationScanOffset : 0.0;
    const double dy = scanX ? 0.0 : kCalibrationScanOffset;
    QVector<double> totals;
    for (const StandardSpec &standard : standards) {
        double total = 0.0;
        if (!measureStandard(standard, dx, dy, &total, errorMessage)) {
            return false;
        }
        totals.append(total);
    }
    if (totals.size() != 4) {
        *errorMessage = QStringLiteral("Calibration did not produce four totals.");
        return false;
    }
    settings.standardTotalVal1 = QString::number(totals.at(0), 'f', 6);
    settings.standardTotalVal2 = QString::number(totals.at(1), 'f', 6);
    settings.standardTotalVal3 = QString::number(totals.at(2), 'f', 6);
    settings.standardTotalVal4 = QString::number(totals.at(3), 'f', 6);
    settings.lastCalTimeStamp_500_1500 = QString::number(QDateTime::currentSecsSinceEpoch());
    if (!m_config.saveToXml(settings, errorMessage)) {
        return false;
    }
    return true;
}

bool MainWindow::measureStandard(const StandardSpec &spec, double dx, double dy, double *total, QString *errorMessage)
{
    if (!moveXy(spec.x + dx, spec.y + dy, m_settings.velocityNormal.toDouble(), errorMessage)) {
        return false;
    }
    if (!invokeMotion([this, spec, dx, dy]() {
            return m_motion.moveMultiAxisLinear(spec.x - dx, spec.y - dy,
                                                kCalibrationScanVelocity, kCalibrationScanAcc, kCalibrationScanAcc);
        }, QStringLiteral("Scan standard"), errorMessage)) {
        return false;
    }

    m_calibrationFilter.clear();
    QVector<double> values;
    QElapsedTimer timer;
    timer.start();
    while (!m_stopRequested && timer.elapsed() < kWaitTimeoutMs) {
        DistanceSnapshot snapshot = distanceSnapshot();
        if (!snapshot.topValid || !snapshot.bottomValid) {
            *errorMessage = QStringLiteral("Invalid color focus sample for standard %1.").arg(spec.id);
            return false;
        }
        values.append(filteredValue(snapshot.top + snapshot.bottom + spec.thickness, &m_calibrationFilter, 5));
        if (values.size() >= 30) {
            break;
        }
        QThread::msleep(100);
    }
    if (values.size() < 30) {
        *errorMessage = QStringLiteral("Calibration sample count is insufficient for standard %1.").arg(spec.id);
        return false;
    }
    if (!waitForXy(spec.x - dx, spec.y - dy, errorMessage)) {
        return false;
    }
    const double sum = std::accumulate(values.constBegin(), values.constEnd(), 0.0);
    *total = sum / double(values.size());
    appendLog(QStringLiteral("Calibration standard %1 total=%2").arg(spec.id).arg(*total, 0, 'f', 6));
    return true;
}

QVector<MainWindow::StandardSpec> MainWindow::standardsFromSettings(const ParamSettings &settings, QString *errorMessage) const
{
    QVector<StandardSpec> standards = {
        {0, 1, settings.posStandard1X.toDouble(), settings.posStandard1Y.toDouble(), settings.standardThickness1.toDouble()},
        {1, 2, settings.posStandard2X.toDouble(), settings.posStandard2Y.toDouble(), settings.standardThickness2.toDouble()},
        {2, 3, settings.posStandard3X.toDouble(), settings.posStandard3Y.toDouble(), settings.standardThickness3.toDouble()},
        {3, 4, settings.posStandard4X.toDouble(), settings.posStandard4Y.toDouble(), settings.standardThickness4.toDouble()}
    };
    for (const StandardSpec &standard : standards) {
        if (!qIsFinite(standard.x) || !qIsFinite(standard.y) || !qIsFinite(standard.thickness)) {
            *errorMessage = QStringLiteral("Invalid standard specification.");
            return {};
        }
    }
    return standards;
}

MainWindow::DistanceSnapshot MainWindow::distanceSnapshot() const
{
    QMutexLocker locker(&m_stateMutex);
    DistanceSnapshot snapshot;
    snapshot.topValid = m_topValid;
    snapshot.bottomValid = m_bottomValid;
    snapshot.top = m_topDistance;
    snapshot.bottom = m_bottomDistance;
    snapshot.encoder = m_lastEncoder;
    return snapshot;
}

double MainWindow::filteredValue(double value, QVector<double> *buffer, int windowSize) const
{
    buffer->append(value);
    while (buffer->size() > windowSize) {
        buffer->removeFirst();
    }
    QVector<double> sorted = *buffer;
    std::sort(sorted.begin(), sorted.end());
    const double median = sorted.at(sorted.size() / 2);
    const double avg = std::accumulate(sorted.constBegin(), sorted.constEnd(), 0.0) / double(sorted.size());
    return median * 0.7 + avg * 0.3;
}

QVector<ProPathLine> MainWindow::buildPathLines(const ProRecipe &recipe, const ParamSettings &settings, QString *errorMessage) const
{
    if (!(recipe.measurePath == 1 || recipe.measurePath == 2 || recipe.measurePath == 4 ||
          recipe.measurePath == 6 || recipe.measurePath == 8)) {
        *errorMessage = QStringLiteral("Unsupported measure path: %1").arg(recipe.measurePath);
        return {};
    }
    const double centerX = recipe.productSize == 12 ? settings.posWaitX_12.toDouble() : settings.posWaitX_8.toDouble();
    const double centerY = recipe.productSize == 12 ? settings.posWaitY_12.toDouble() : settings.posWaitY_8.toDouble();
    if (!qIsFinite(centerX) || !qIsFinite(centerY)) {
        *errorMessage = QStringLiteral("Wait position is invalid for product size %1.").arg(recipe.productSize);
        return {};
    }
    double radius = 100.0;
    if (recipe.productSize == 6) {
        radius = 75.0;
    } else if (recipe.productSize == 12) {
        radius = 150.0;
    }
    radius -= recipe.trimSize;
    if (radius <= 0.0) {
        *errorMessage = QStringLiteral("Scan radius is invalid. size=%1 trim=%2").arg(recipe.productSize).arg(recipe.trimSize);
        return {};
    }

    QVector<ProPathLine> lines;
    lines.reserve(recipe.measurePath);
    const double step = recipe.measurePath == 1 ? 0.0 : 180.0 / double(recipe.measurePath);
    for (int i = 0; i < recipe.measurePath; ++i) {
        const double angle = qDegreesToRadians(i * step);
        const double dx = qCos(angle) * radius;
        const double dy = qSin(angle) * radius;
        ProPathLine line;
        line.lineIndex = i;
        line.startX = centerX - dx;
        line.startY = centerY - dy;
        line.endX = centerX + dx;
        line.endY = centerY + dy;
        lines.append(line);
    }
    return lines;
}

bool MainWindow::scanRecipeLines(const ProRecipe &recipe, bool gravityMode, ProRunResult *runResult, QString *errorMessage)
{
    runResult->points.clear();
    runResult->linePoints.clear();
    QVector<ProPathLine> lines = buildPathLines(recipe, m_settings, errorMessage);
    if (lines.isEmpty()) {
        return false;
    }
    QMap<int, QMap<int, double>> gravity;
    if (!gravityMode && !loadGravityFiles(recipe, &gravity, errorMessage)) {
        return false;
    }
    if (!moveZ(m_settings.z_default_123.toDouble(), errorMessage)) {
        return false;
    }
    for (const ProPathLine &line : lines) {
        if (m_stopRequested) {
            *errorMessage = QStringLiteral("Scan canceled.");
            return false;
        }
        QVector<ProMeasurePoint> points;
        const QMap<int, double> lineGravity = gravity.value(line.lineIndex);
        if (!scanOneLine(line, recipe, gravityMode, lineGravity, &points, errorMessage)) {
            return false;
        }
        runResult->linePoints.insert(line.lineIndex, points);
        runResult->points += points;
        QMetaObject::invokeMethod(this, [this, points]() { updateCurve(points); }, Qt::QueuedConnection);
    }
    if (runResult->points.isEmpty()) {
        *errorMessage = QStringLiteral("Scan produced zero points.");
        return false;
    }
    return true;
}

bool MainWindow::scanOneLine(const ProPathLine &line, const ProRecipe &recipe, bool gravityMode,
                             const QMap<int, double> &gravityMap, QVector<ProMeasurePoint> *points, QString *errorMessage)
{
    points->clear();
    if (!gravityMode && gravityMap.isEmpty()) {
        *errorMessage = QStringLiteral("Gravity data is empty for line %1.").arg(line.lineIndex);
        return false;
    }
    if (!moveXy(line.startX, line.startY, m_settings.velocityNormal.toDouble(), errorMessage)) {
        return false;
    }
    if (!invokeMotion([this, line]() {
            const double velocity = m_settings.velocityMeasure.toDouble();
            return m_motion.moveMultiAxisLinear(line.endX, line.endY, velocity, velocity * 10.0, velocity * 10.0);
        }, QStringLiteral("Scan line"), errorMessage)) {
        return false;
    }

    QElapsedTimer timer;
    QElapsedTimer sampleTimer;
    timer.start();
    sampleTimer.start();
    while (!m_stopRequested && timer.elapsed() < kWaitTimeoutMs) {
        Motion::AxisPosition position;
        {
            QMutexLocker locker(&m_stateMutex);
            position = m_position;
        }
        const bool reached = position.valid &&
                             qAbs(position.x - line.endX) <= kPositionTolerance &&
                             qAbs(position.y - line.endY) <= kPositionTolerance;
        if (sampleTimer.elapsed() >= kSampleIntervalMs && points->size() < recipe.lineSampleNum) {
            DistanceSnapshot snapshot = distanceSnapshot();
            if (!snapshot.topValid || !snapshot.bottomValid) {
                *errorMessage = QStringLiteral("Invalid color focus sample on line %1.").arg(line.lineIndex);
                return false;
            }
            if (!position.valid) {
                *errorMessage = QStringLiteral("Motion position is invalid on line %1.").arg(line.lineIndex);
                return false;
            }
            ProMeasurePoint point;
            point.lineIndex = line.lineIndex;
            point.sampleIndex = points->size();
            point.x = position.x;
            point.y = position.y;
            point.z = position.z;
            point.topDistance = snapshot.top;
            point.bottomDistance = snapshot.bottom;
            point.encoder = snapshot.encoder;
            point.thickness = calibrationTotalForRecipe(recipe) - point.topDistance - point.bottomDistance;
            if (gravityMode) {
                point.zGravity = (point.bottomDistance - point.topDistance) / 2.0;
                point.hasZGravity = true;
            } else {
                if (!gravityMap.contains(point.sampleIndex)) {
                    *errorMessage = QStringLiteral("Missing gravity value. line=%1 sample=%2").arg(line.lineIndex).arg(point.sampleIndex);
                    return false;
                }
                point.zGravity = gravityMap.value(point.sampleIndex);
                point.hasZGravity = true;
            }
            if (!qIsFinite(point.thickness) || !qIsFinite(point.zGravity)) {
                *errorMessage = QStringLiteral("Invalid computed value on line %1 sample %2.").arg(line.lineIndex).arg(point.sampleIndex);
                return false;
            }
            points->append(point);
            sampleTimer.restart();
        }
        if (reached) {
            break;
        }
        QThread::msleep(10);
    }
    if (m_stopRequested) {
        *errorMessage = QStringLiteral("Scan line canceled.");
        return false;
    }
    if (points->size() < recipe.lineSampleNum) {
        *errorMessage = QStringLiteral("Line %1 has insufficient points. expected=%2 actual=%3")
            .arg(line.lineIndex).arg(recipe.lineSampleNum).arg(points->size());
        return false;
    }
    return true;
}

double MainWindow::calibrationTotalForRecipe(const ProRecipe &recipe) const
{
    struct Candidate
    {
        double thickness;
        double total;
    };
    const QVector<Candidate> candidates = {
        {m_settings.standardThickness1.toDouble(), m_settings.standardTotalVal1.toDouble()},
        {m_settings.standardThickness2.toDouble(), m_settings.standardTotalVal2.toDouble()},
        {m_settings.standardThickness3.toDouble(), m_settings.standardTotalVal3.toDouble()},
        {m_settings.standardThickness4.toDouble(), m_settings.standardTotalVal4.toDouble()}
    };
    double bestDistance = std::numeric_limits<double>::max();
    double bestTotal = 0.0;
    for (const Candidate &candidate : candidates) {
        const double distance = qAbs(candidate.thickness - recipe.productThickness);
        if (distance < bestDistance && qIsFinite(candidate.total) && candidate.total > 0.0) {
            bestDistance = distance;
            bestTotal = candidate.total;
        }
    }
    return bestTotal;
}

Wafer::WaferDataset MainWindow::buildDataset(const ProRecipe &recipe, const ProRunResult &runResult) const
{
    Wafer::WaferDataset dataset;
    dataset.lineCount = recipe.measurePath;
    dataset.productSize = recipe.productSize;
    dataset.nominalThickness = recipe.productThickness;
    dataset.calibrationTotal = calibrationTotalForRecipe(recipe);
    dataset.centerX = recipe.productSize == 12 ? m_settings.posWaitX_12.toDouble() : m_settings.posWaitX_8.toDouble();
    dataset.centerY = recipe.productSize == 12 ? m_settings.posWaitY_12.toDouble() : m_settings.posWaitY_8.toDouble();
    dataset.containsZGravity = true;
    dataset.thicknessOnly = false;

    for (auto it = runResult.linePoints.constBegin(); it != runResult.linePoints.constEnd(); ++it) {
        Wafer::LineData line;
        line.lineIndex = it.key();
        for (const ProMeasurePoint &source : it.value()) {
            Wafer::DataPoint point;
            point.x = source.x;
            point.y = source.y;
            point.topDistance = source.topDistance;
            point.bottomDistance = source.bottomDistance;
            point.thickness = source.thickness;
            point.zGravity = source.zGravity;
            point.xEncoder = source.encoder;
            point.yEncoder = source.encoder;
            point.lineIndex = source.lineIndex;
            point.hasTopDistance = true;
            point.hasBottomDistance = true;
            point.hasThickness = true;
            point.hasZGravity = source.hasZGravity;
            line.points.append(point);
        }
        dataset.lines.append(line);
    }
    return dataset;
}

Wafer::AlgorithmOptions MainWindow::algorithmOptions(const ProRecipe &recipe) const
{
    Wafer::AlgorithmOptions options;
    options.lineSampleNum = recipe.lineSampleNum;
    options.lineCount = recipe.measurePath;
    options.productSize = recipe.productSize;
    options.nominalThickness = recipe.productThickness;
    options.calibrationTotal = calibrationTotalForRecipe(recipe);
    options.trimSize = recipe.trimSize;
    options.chordLength = m_settings.ChordLength.toDouble();
    options.useNewBowAlg = m_settings.IsNewBowAlg.toInt() != 0;
    options.useNewWarpAlg = m_settings.IsNewWarpAlg.toInt() != 0;
    options.centerX = recipe.productSize == 12 ? m_settings.posWaitX_12.toDouble() : m_settings.posWaitX_8.toDouble();
    options.centerY = recipe.productSize == 12 ? m_settings.posWaitY_12.toDouble() : m_settings.posWaitY_8.toDouble();
    options.radius = recipe.productSize == 12 ? 150.0 : (recipe.productSize == 6 ? 75.0 : 100.0);
    options.useCsvZGravity = true;
    options.allowDebugZeroZGravity = false;
    return options;
}

QString MainWindow::gravityDir(const ProRecipe &recipe) const
{
    const QString lineType = QStringLiteral("type%1lines").arg(recipe.measurePath);
    return QDir(QCoreApplication::applicationDirPath()).filePath(
        QStringLiteral("ZGravityFile/%1/%2/%3/ZGravity")
            .arg(lineType)
            .arg(recipe.productSize)
            .arg(recipe.productThickness));
}

bool MainWindow::saveGravityFiles(const ProRecipe &recipe, const ProRunResult &runResult, QString *errorMessage)
{
    QDir dir(gravityDir(recipe));
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        *errorMessage = QStringLiteral("Create gravity directory failed: %1").arg(dir.absolutePath());
        return false;
    }
    for (auto it = runResult.linePoints.constBegin(); it != runResult.linePoints.constEnd(); ++it) {
        const QString path = dir.filePath(QStringLiteral("z_gravity_%1.zg").arg(it.key()));
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            *errorMessage = QStringLiteral("Open gravity file failed: %1, %2").arg(path, file.errorString());
            return false;
        }
        QDataStream out(&file);
        out.setVersion(QDataStream::Qt_5_15);
        for (const ProMeasurePoint &point : it.value()) {
            if (!point.hasZGravity || !qIsFinite(point.zGravity)) {
                *errorMessage = QStringLiteral("Invalid gravity point. line=%1 sample=%2").arg(it.key()).arg(point.sampleIndex);
                return false;
            }
            QStringList fields;
            fields << QString::number(point.zGravity, 'g', 15)
                   << QString::number(point.sampleIndex);
            out << fields;
        }
    }
    return true;
}

bool MainWindow::loadGravityFiles(const ProRecipe &recipe, QMap<int, QMap<int, double>> *gravity, QString *errorMessage) const
{
    gravity->clear();
    QDir dir(gravityDir(recipe));
    if (!dir.exists()) {
        *errorMessage = QStringLiteral("Gravity directory does not exist: %1").arg(dir.absolutePath());
        return false;
    }
    for (int lineIndex = 0; lineIndex < recipe.measurePath; ++lineIndex) {
        const QString path = dir.filePath(QStringLiteral("z_gravity_%1.zg").arg(lineIndex));
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            *errorMessage = QStringLiteral("Open gravity file failed: %1, %2").arg(path, file.errorString());
            return false;
        }
        QDataStream in(&file);
        in.setVersion(QDataStream::Qt_5_15);
        QMap<int, double> lineMap;
        while (!in.atEnd()) {
            QStringList fields;
            in >> fields;
            if (fields.size() < 2) {
                *errorMessage = QStringLiteral("Invalid gravity file row: %1").arg(path);
                return false;
            }
            bool okValue = false;
            bool okIndex = false;
            const double value = fields.at(0).toDouble(&okValue);
            const int index = fields.at(1).toInt(&okIndex);
            if (!okValue || !okIndex || !qIsFinite(value)) {
                *errorMessage = QStringLiteral("Invalid gravity value: %1").arg(path);
                return false;
            }
            lineMap.insert(index, value);
        }
        if (lineMap.size() < recipe.lineSampleNum) {
            *errorMessage = QStringLiteral("Gravity file has insufficient points: %1").arg(path);
            return false;
        }
        gravity->insert(lineIndex, lineMap);
    }
    return true;
}

bool MainWindow::writeResultFiles(ProRunResult *runResult, QString *errorMessage)
{
    QString root = m_settings.measureFileDir.trimmed();
    if (root.isEmpty()) {
        root = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("MeasureData"));
    }
    QDir rootDir(root);
    if (!rootDir.exists() && !rootDir.mkpath(QStringLiteral("."))) {
        *errorMessage = QStringLiteral("Create result directory failed: %1").arg(root);
        return false;
    }
    QString recipeName;
    if (QThread::currentThread() == thread()) {
        recipeName = currentRecipe().name;
    } else {
        QMetaObject::invokeMethod(const_cast<MainWindow *>(this), [&]() { recipeName = currentRecipe().name; }, Qt::BlockingQueuedConnection);
    }
    const QString dirName = QStringLiteral("%1_%2").arg(recipeName, nowStamp());
    if (!rootDir.mkdir(dirName) && !rootDir.exists(dirName)) {
        *errorMessage = QStringLiteral("Create result subdirectory failed: %1").arg(rootDir.filePath(dirName));
        return false;
    }
    QDir outDir(rootDir.filePath(dirName));
    runResult->outputDir = outDir.absolutePath();
    runResult->rawCsvPath = outDir.filePath(QStringLiteral("raw_points.csv"));
    runResult->summaryCsvPath = outDir.filePath(QStringLiteral("summary.csv"));
    runResult->heatImagePath = outDir.filePath(QStringLiteral("heatmap_2d.png"));
    runResult->surfaceImagePath = outDir.filePath(QStringLiteral("surface_3d.png"));

    if (!writeRawCsv(runResult->rawCsvPath, *runResult, errorMessage) ||
        !writeLineCsvs(outDir.absolutePath(), *runResult, errorMessage) ||
        !writeSummaryCsv(runResult->summaryCsvPath, *runResult, errorMessage)) {
        return false;
    }
    return true;
}

bool MainWindow::writeRawCsv(const QString &path, const ProRunResult &runResult, QString *errorMessage) const
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        *errorMessage = QStringLiteral("Open raw csv failed: %1, %2").arg(path, file.errorString());
        return false;
    }
    QTextStream out(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    out.setCodec("System");
#endif
    out << "lineIndex,sampleIndex,x,y,z,topDistance,bottomDistance,thickness,zGravity,encoder\n";
    for (const ProMeasurePoint &point : runResult.points) {
        out << point.lineIndex << ',' << point.sampleIndex << ','
            << QString::number(point.x, 'g', 15) << ','
            << QString::number(point.y, 'g', 15) << ','
            << QString::number(point.z, 'g', 15) << ','
            << QString::number(point.topDistance, 'g', 15) << ','
            << QString::number(point.bottomDistance, 'g', 15) << ','
            << QString::number(point.thickness, 'g', 15) << ','
            << QString::number(point.zGravity, 'g', 15) << ','
            << point.encoder << '\n';
    }
    return true;
}

bool MainWindow::writeLineCsvs(const QString &dir, const ProRunResult &runResult, QString *errorMessage) const
{
    for (auto it = runResult.linePoints.constBegin(); it != runResult.linePoints.constEnd(); ++it) {
        const QString path = QDir(dir).filePath(QStringLiteral("line_%1_thickness.csv").arg(it.key()));
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            *errorMessage = QStringLiteral("Open line csv failed: %1, %2").arg(path, file.errorString());
            return false;
        }
        QTextStream out(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        out.setCodec("System");
#endif
        out << "sampleIndex,thickness\n";
        for (const ProMeasurePoint &point : it.value()) {
            out << point.sampleIndex << ',' << QString::number(point.thickness, 'g', 15) << '\n';
        }
    }
    return true;
}

bool MainWindow::writeSummaryCsv(const QString &path, const ProRunResult &runResult, QString *errorMessage) const
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        *errorMessage = QStringLiteral("Open summary csv failed: %1, %2").arg(path, file.errorString());
        return false;
    }
    QTextStream out(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    out.setCodec("System");
#endif
    out << "item,value\n";
    const Wafer::AlgorithmResult &r = runResult.algorithm;
    out << "success," << (r.success ? 1 : 0) << '\n';
    out << "errorMessage," << r.errorMessage << '\n';
    out << "BOW," << (r.hasBow ? QString::number(r.bow, 'g', 15) : QString()) << '\n';
    out << "WARP," << (r.hasWarp ? QString::number(r.warp, 'g', 15) : QString()) << '\n';
    out << "CENTER_THK," << (r.hasCenterThk ? QString::number(r.centerThk, 'g', 15) : QString()) << '\n';
    out << "AVERAGE_THK," << (r.hasAverageThk ? QString::number(r.averageThk, 'g', 15) : QString()) << '\n';
    out << "TTV," << (r.hasTtv ? QString::number(r.ttv, 'g', 15) : QString()) << '\n';
    out << "SORI," << (r.hasSori ? QString::number(r.sori, 'g', 15) : QString()) << '\n';
    return true;
}

void MainWindow::finishRunOnUi(const ProRunResult &runResult, const QString &message)
{
    updateCurve(runResult.linePoints.isEmpty() ? QVector<ProMeasurePoint>() : runResult.linePoints.last());
    updateHeatMap(runResult);
    updateSurface(runResult);
    updateResultTable(runResult);
    QString imageError;
    QPixmap heatPixmap = ui->label_heatmap->grab();
    if (!heatPixmap.save(runResult.heatImagePath)) {
        imageError = QStringLiteral("Save heat map image failed: %1").arg(runResult.heatImagePath);
    }
    QPixmap surfacePixmap = ui->widget_surface->grab();
    if (!surfacePixmap.save(runResult.surfaceImagePath)) {
        imageError = imageError.isEmpty()
            ? QStringLiteral("Save surface image failed: %1").arg(runResult.surfaceImagePath)
            : QStringLiteral("%1; Save surface image failed: %2").arg(imageError, runResult.surfaceImagePath);
    }
    appendLog(message);
    appendLog(QStringLiteral("Result saved: %1").arg(runResult.outputDir));
    if (!imageError.isEmpty()) {
        showError(imageError);
    }
}

void MainWindow::updateCurve(const QVector<ProMeasurePoint> &linePoints)
{
    QChart *chart = m_curveView->chart();
    chart->removeAllSeries();
    QLineSeries *series = new QLineSeries(chart);
    for (const ProMeasurePoint &point : linePoints) {
        series->append(point.sampleIndex, point.thickness);
    }
    chart->addSeries(series);
    QValueAxis *axisX = new QValueAxis;
    QValueAxis *axisY = new QValueAxis;
    axisX->setTitleText(QStringLiteral("Sample"));
    axisY->setTitleText(QStringLiteral("THK"));
    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisX);
    series->attachAxis(axisY);
    if (!linePoints.isEmpty()) {
        axisX->setRange(0, qMax(1, linePoints.last().sampleIndex));
        auto mm = std::minmax_element(linePoints.constBegin(), linePoints.constEnd(), [](const ProMeasurePoint &a, const ProMeasurePoint &b) {
            return a.thickness < b.thickness;
        });
        axisY->setRange(mm.first->thickness - 1.0, mm.second->thickness + 1.0);
    }
}

void MainWindow::updateHeatMap(const ProRunResult &runResult)
{
    if (runResult.points.isEmpty()) {
        ui->label_heatmap->setText(QStringLiteral("2D"));
        return;
    }
    const int width = qMax(320, ui->label_heatmap->width());
    const int height = qMax(240, ui->label_heatmap->height());
    QImage image(width, height, QImage::Format_RGB32);
    image.fill(Qt::white);

    auto mmX = std::minmax_element(runResult.points.constBegin(), runResult.points.constEnd(), [](const ProMeasurePoint &a, const ProMeasurePoint &b) { return a.x < b.x; });
    auto mmY = std::minmax_element(runResult.points.constBegin(), runResult.points.constEnd(), [](const ProMeasurePoint &a, const ProMeasurePoint &b) { return a.y < b.y; });
    auto mmT = std::minmax_element(runResult.points.constBegin(), runResult.points.constEnd(), [](const ProMeasurePoint &a, const ProMeasurePoint &b) { return a.thickness < b.thickness; });
    const double minX = mmX.first->x;
    const double maxX = mmX.second->x;
    const double minY = mmY.first->y;
    const double maxY = mmY.second->y;
    const double minT = mmT.first->thickness;
    const double maxT = mmT.second->thickness;

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    for (const ProMeasurePoint &point : runResult.points) {
        const double tx = qFuzzyCompare(maxX, minX) ? 0.5 : (point.x - minX) / (maxX - minX);
        const double ty = qFuzzyCompare(maxY, minY) ? 0.5 : (point.y - minY) / (maxY - minY);
        const double tt = qFuzzyCompare(maxT, minT) ? 0.5 : (point.thickness - minT) / (maxT - minT);
        QColor color;
        color.setHsvF((1.0 - tt) * 0.66, 0.95, 0.95);
        painter.setPen(Qt::NoPen);
        painter.setBrush(color);
        painter.drawEllipse(QPointF(tx * (width - 20) + 10, (1.0 - ty) * (height - 20) + 10), 3.0, 3.0);
    }
    painter.end();
    ui->label_heatmap->setPixmap(QPixmap::fromImage(image));
}

void MainWindow::updateSurface(const ProRunResult &runResult)
{
    while (!m_surface->seriesList().isEmpty()) {
        m_surface->removeSeries(m_surface->seriesList().first());
    }
    if (runResult.linePoints.isEmpty()) {
        return;
    }
    QSurfaceDataArray *data = new QSurfaceDataArray;
    data->reserve(runResult.linePoints.size());
    for (auto it = runResult.linePoints.constBegin(); it != runResult.linePoints.constEnd(); ++it) {
        QSurfaceDataRow *row = new QSurfaceDataRow;
        row->reserve(it.value().size());
        for (const ProMeasurePoint &point : it.value()) {
            row->append(QSurfaceDataItem(QVector3D(point.x, point.thickness, point.y)));
        }
        data->append(row);
    }
    QSurfaceDataProxy *proxy = new QSurfaceDataProxy;
    proxy->resetArray(data);
    QSurface3DSeries *series = new QSurface3DSeries(proxy);
    series->setDrawMode(QSurface3DSeries::DrawSurfaceAndWireframe);
    m_surface->addSeries(series);
}

void MainWindow::updateResultTable(const ProRunResult &runResult)
{
    const Wafer::AlgorithmResult &r = runResult.algorithm;
    auto setValue = [this](int row, bool hasValue, double value) {
        ui->table_results->setItem(row, 1, new QTableWidgetItem(hasValue ? QString::number(value, 'f', 6) : QStringLiteral("--")));
    };
    setValue(0, r.hasBow, r.bow);
    setValue(1, r.hasWarp, r.warp);
    setValue(2, r.hasCenterThk, r.centerThk);
    setValue(3, r.hasAverageThk, r.averageThk);
    setValue(4, r.hasTtv, r.ttv);
    setValue(5, r.hasSori, r.sori);
}

void MainWindow::updateCalibrationTable(const ParamSettings &settings)
{
    const QStringList names = {QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3"), QStringLiteral("4")};
    const QStringList thickness = {settings.standardThickness1, settings.standardThickness2, settings.standardThickness3, settings.standardThickness4};
    const QStringList totals = {settings.standardTotalVal1, settings.standardTotalVal2, settings.standardTotalVal3, settings.standardTotalVal4};
    for (int i = 0; i < 4; ++i) {
        ui->table_calibration->setItem(i, 0, new QTableWidgetItem(names.at(i)));
        ui->table_calibration->setItem(i, 1, new QTableWidgetItem(thickness.at(i)));
        ui->table_calibration->setItem(i, 2, new QTableWidgetItem(totals.at(i)));
    }
}

QMap<int, double> MainWindow::gravityMapForLine(const ProRecipe &recipe, int lineIndex, QString *errorMessage) const
{
    QMap<int, QMap<int, double>> gravity;
    if (!loadGravityFiles(recipe, &gravity, errorMessage)) {
        return {};
    }
    return gravity.value(lineIndex);
}
