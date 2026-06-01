#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "WaferAlgorithm.h"
#include "WaferCsvLoader.h"

#include <QCloseEvent>
#include <QDateTime>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QFileInfo>
#include <QAbstractItemView>
#include <QMessageBox>
#include <QMetaObject>
#include <QPushButton>
#include <QHeaderView>
#include <QStatusBar>
#include <QTableWidgetItem>
#include <QThread>
#include <QtConcurrent>
#include <QtMath>

#include <algorithm>
#include <numeric>

namespace
{
constexpr int kTopSensorId = 0;
constexpr int kBottomSensorId = 1;
constexpr int kSampleCount = 30;
constexpr int kSampleIntervalMs = 100;
constexpr int kWaitTimeoutMs = 120000;
constexpr double kPositionTolerance = 0.003;
constexpr double kScanOffset = 3.0;
constexpr double kScanVelocity = 2.0;
constexpr double kScanAcceleration = 40.0;
constexpr double kScanDeceleration = 40.0;
constexpr double kCalibrationThreshold = 2.0;
constexpr double kStandardCompensationValue = 0.0;

bool readyState(int state)
{
    return state == 3;
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      m_topFocus(new ColorFocusControl),
      m_bottomFocus(new ColorFocusControl),
      m_focusThread(new QThread(this)),
      m_topConnected(false),
      m_bottomConnected(false),
      m_topAcquiring(false),
      m_bottomAcquiring(false),
      m_topDistanceValid(false),
      m_bottomDistanceValid(false),
      m_topDistance(0.0),
      m_bottomDistance(0.0),
      m_cancelRequested(false),
      m_busy(false)
{
    ui->setupUi(this);
    setupUiState();
    setupColorFocusThread();
    connectSignals();
    loadConfigToUi();
}

MainWindow::~MainWindow()
{
    m_cancelRequested = true;
    if (m_motionController.isConnected())
        m_motionController.abortAxes();
    if (m_calibrationFuture.isRunning())
        m_calibrationFuture.waitForFinished();
    stopColorFocus();
    m_motionController.disconnectFromController();
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    m_cancelRequested = true;
    if (m_motionController.isConnected())
        m_motionController.abortAxes();
    QMainWindow::closeEvent(event);
}

void MainWindow::setupUiState()
{
    ui->table_result->setRowCount(4);
    ui->table_result->setColumnCount(7);
    ui->table_result->verticalHeader()->setVisible(false);
    ui->table_result->horizontalHeader()->setStretchLastSection(true);
    ui->table_result->setEditTriggers(QAbstractItemView::NoEditTriggers);
    for (int row = 0; row < 4; ++row) {
        QTableWidgetItem *idItem = new QTableWidgetItem(QString::number(row == 0 ? 500 : (row == 1 ? 725 : (row == 2 ? 800 : 1550))));
        ui->table_result->setItem(row, 0, idItem);
        for (int col = 1; col < ui->table_result->columnCount(); ++col)
            ui->table_result->setItem(row, col, new QTableWidgetItem(QStringLiteral("--")));
    }

    ui->combo_scan_direction->setCurrentIndex(0);
    ui->label_motion_status->setText(QStringLiteral("Disconnected"));
    ui->label_focus_status->setText(QStringLiteral("Disconnected"));
    ui->label_top_distance->setText(QStringLiteral("--"));
    ui->label_bottom_distance->setText(QStringLiteral("--"));
    ui->label_axis_pos->setText(QStringLiteral("--"));
    ui->label_axis_state->setText(QStringLiteral("--"));
    statusBar()->showMessage(QStringLiteral("Ready"));
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
    connect(ui->pushButton_load_config, &QPushButton::clicked, this, &MainWindow::onLoadConfigClicked);
    connect(ui->pushButton_save_config, &QPushButton::clicked, this, &MainWindow::onSaveConfigClicked);
    connect(ui->pushButton_connect_motion, &QPushButton::clicked, this, &MainWindow::onConnectMotionClicked);
    connect(ui->pushButton_disconnect_motion, &QPushButton::clicked, this, &MainWindow::onDisconnectMotionClicked);
    connect(ui->pushButton_connect_focus, &QPushButton::clicked, this, &MainWindow::onConnectColorFocusClicked);
    connect(ui->pushButton_start_acq, &QPushButton::clicked, this, &MainWindow::onStartAcquisitionClicked);
    connect(ui->pushButton_stop_acq, &QPushButton::clicked, this, &MainWindow::onStopAcquisitionClicked);
    connect(ui->pushButton_stop_motion, &QPushButton::clicked, this, &MainWindow::onStopMotionClicked);
    connect(ui->pushButton_four_calibration, &QPushButton::clicked, this, &MainWindow::onFourCalibrationClicked);
    connect(ui->pushButton_1550_calibration, &QPushButton::clicked, this, &MainWindow::onStandard1550CalibrationClicked);
    connect(ui->pushButton_browse_csv, &QPushButton::clicked, this, &MainWindow::onBrowseCsvClicked);
    connect(ui->pushButton_run_algorithm, &QPushButton::clicked, this, &MainWindow::onRunAlgorithmClicked);

    const QList<QPushButton *> moveButtons = {
        ui->pushButton_move_s1, ui->pushButton_move_s2, ui->pushButton_move_s3, ui->pushButton_move_s4
    };
    for (QPushButton *button : moveButtons)
        connect(button, &QPushButton::clicked, this, &MainWindow::onMoveStandardClicked);

    connect(&m_motionController, &MotionController::connectionStatusChanged, this, &MainWindow::onMotionConnectedChanged);
    connect(&m_motionController, &MotionController::positionUpdated, this, &MainWindow::onMotionPositionUpdated);
    connect(&m_motionController, &MotionController::stateUpdated, this, &MainWindow::onMotionStateUpdated);
    connect(&m_motionController, &MotionController::logMessage, this, &MainWindow::appendLog);
    connect(&m_motionController, &MotionController::errorMessage, this, &MainWindow::appendLog);
    connect(&m_motionController, &MotionController::stateNotReady, this, &MainWindow::appendLog);

    connect(m_topFocus, &ColorFocusControl::sampleUpdated, this, &MainWindow::onColorFocusSampleUpdated);
    connect(m_bottomFocus, &ColorFocusControl::sampleUpdated, this, &MainWindow::onColorFocusSampleUpdated);
    connect(m_topFocus, &ColorFocusControl::logMessage, this, &MainWindow::appendLog);
    connect(m_bottomFocus, &ColorFocusControl::logMessage, this, &MainWindow::appendLog);
    connect(m_topFocus, &ColorFocusControl::errorOccurred, this, [this](int, const QString &message) { appendLog(message); });
    connect(m_bottomFocus, &ColorFocusControl::errorOccurred, this, [this](int, const QString &message) { appendLog(message); });
    connect(m_topFocus, &ColorFocusControl::connectionStateChanged, this, [this](int, bool connected) {
        m_topConnected = connected;
        setColorFocusStatusText();
    });
    connect(m_bottomFocus, &ColorFocusControl::connectionStateChanged, this, [this](int, bool connected) {
        m_bottomConnected = connected;
        setColorFocusStatusText();
    });
    connect(m_topFocus, &ColorFocusControl::acquisitionStateChanged, this, [this](int, bool acquiring) {
        m_topAcquiring = acquiring;
        setColorFocusStatusText();
    });
    connect(m_bottomFocus, &ColorFocusControl::acquisitionStateChanged, this, [this](int, bool acquiring) {
        m_bottomAcquiring = acquiring;
        setColorFocusStatusText();
    });
}

void MainWindow::onLoadConfigClicked()
{
    loadConfigToUi();
}

void MainWindow::onSaveConfigClicked()
{
    saveUiToConfig();
}

bool MainWindow::loadConfigToUi()
{
    QString error;
    ParamSettings settings;
    if (!m_configManager.loadFromXml(&settings, &error)) {
        appendLog(error);
        QMessageBox::warning(this, QStringLiteral("Calibration"), error);
        return false;
    }
    m_settings = settings;
    applySettingsToUi(m_settings);
    appendLog(QStringLiteral("Loaded config: %1").arg(m_configManager.configPath()));
    return true;
}

bool MainWindow::saveUiToConfig()
{
    ParamSettings settings = readSettingsFromUi();
    QString error;
    if (!validateSettings(settings, &error)) {
        appendLog(error);
        QMessageBox::warning(this, QStringLiteral("Calibration"), error);
        return false;
    }
    if (!m_configManager.saveToXml(settings, &error)) {
        appendLog(error);
        QMessageBox::warning(this, QStringLiteral("Calibration"), error);
        return false;
    }
    m_settings = settings;
    applySettingsToUi(m_settings);
    appendLog(QStringLiteral("Saved config: %1").arg(m_configManager.configPath()));
    return true;
}

void MainWindow::applySettingsToUi(const ParamSettings &settings)
{
    ui->lineEdit_axis_ip->setText(settings.axisIp);
    ui->lineEdit_axis_port->setText(settings.axisPort);
    ui->lineEdit_cf_top->setText(settings.colorFocusIpTop);
    ui->lineEdit_cf_bottom->setText(settings.colorFocusIpBottom);
    ui->lineEdit_z_123->setText(settings.z_default_123);
    ui->lineEdit_z_4->setText(settings.z_default_4);
    ui->lineEdit_velocity_normal->setText(settings.velocityNormal);
    ui->combo_scan_direction->setCurrentIndex(settings.CalibrationDirectionOfX.toInt() == 1 ? 0 : 1);

    ui->lineEdit_s1x->setText(settings.posStandard1X);
    ui->lineEdit_s1y->setText(settings.posStandard1Y);
    ui->lineEdit_s1t->setText(settings.standardThickness1);
    ui->lineEdit_s2x->setText(settings.posStandard2X);
    ui->lineEdit_s2y->setText(settings.posStandard2Y);
    ui->lineEdit_s2t->setText(settings.standardThickness2);
    ui->lineEdit_s3x->setText(settings.posStandard3X);
    ui->lineEdit_s3y->setText(settings.posStandard3Y);
    ui->lineEdit_s3t->setText(settings.standardThickness3);
    ui->lineEdit_s4x->setText(settings.posStandard4X);
    ui->lineEdit_s4y->setText(settings.posStandard4Y);
    ui->lineEdit_s4t->setText(settings.standardThickness4);

    const QStringList histories = {
        settings.standardTotalVal1, settings.standardTotalVal2, settings.standardTotalVal3, settings.standardTotalVal4
    };
    const QStringList thickness = {
        settings.standardThickness1, settings.standardThickness2, settings.standardThickness3, settings.standardThickness4
    };
    for (int row = 0; row < 4; ++row) {
        ui->table_result->item(row, 1)->setText(thickness.at(row));
        ui->table_result->item(row, 5)->setText(histories.at(row).isEmpty() ? QStringLiteral("--") : histories.at(row));
        ui->table_result->item(row, 6)->setText(QStringLiteral("--"));
    }
}

ParamSettings MainWindow::readSettingsFromUi() const
{
    ParamSettings settings = m_settings;
    settings.axisIp = ui->lineEdit_axis_ip->text().trimmed();
    settings.axisPort = ui->lineEdit_axis_port->text().trimmed();
    settings.colorFocusIpTop = ui->lineEdit_cf_top->text().trimmed();
    settings.colorFocusIpBottom = ui->lineEdit_cf_bottom->text().trimmed();
    settings.z_default_123 = ui->lineEdit_z_123->text().trimmed();
    settings.z_default_4 = ui->lineEdit_z_4->text().trimmed();
    settings.velocityNormal = ui->lineEdit_velocity_normal->text().trimmed();
    settings.CalibrationDirectionOfX = ui->combo_scan_direction->currentIndex() == 0 ? QStringLiteral("1") : QStringLiteral("0");
    settings.posStandard1X = ui->lineEdit_s1x->text().trimmed();
    settings.posStandard1Y = ui->lineEdit_s1y->text().trimmed();
    settings.standardThickness1 = ui->lineEdit_s1t->text().trimmed();
    settings.posStandard2X = ui->lineEdit_s2x->text().trimmed();
    settings.posStandard2Y = ui->lineEdit_s2y->text().trimmed();
    settings.standardThickness2 = ui->lineEdit_s2t->text().trimmed();
    settings.posStandard3X = ui->lineEdit_s3x->text().trimmed();
    settings.posStandard3Y = ui->lineEdit_s3y->text().trimmed();
    settings.standardThickness3 = ui->lineEdit_s3t->text().trimmed();
    settings.posStandard4X = ui->lineEdit_s4x->text().trimmed();
    settings.posStandard4Y = ui->lineEdit_s4y->text().trimmed();
    settings.standardThickness4 = ui->lineEdit_s4t->text().trimmed();
    return settings;
}

bool MainWindow::validateSettings(const ParamSettings &settings, QString *errorMessage) const
{
    quint16 port = 0;
    double value = 0.0;
    if (settings.axisIp.isEmpty()) {
        *errorMessage = QStringLiteral("AxisIP is empty.");
        return false;
    }
    if (!readPortRequired(settings.axisPort, &port, errorMessage))
        return false;
    if (settings.colorFocusIpTop.isEmpty() || settings.colorFocusIpBottom.isEmpty()) {
        *errorMessage = QStringLiteral("Color focus IP is empty.");
        return false;
    }
    if (!readDoubleRequired(settings.z_default_123, QStringLiteral("ZDefault123"), &value, errorMessage))
        return false;
    if (!readDoubleRequired(settings.z_default_4, QStringLiteral("ZDefault4"), &value, errorMessage))
        return false;
    if (!readDoubleRequired(settings.velocityNormal, QStringLiteral("NormalSpeed"), &value, errorMessage) || value <= 0.0) {
        *errorMessage = QStringLiteral("NormalSpeed must be greater than 0.");
        return false;
    }
    QVector<StandardSpec> standards = buildStandards(settings, errorMessage);
    return !standards.isEmpty();
}

bool MainWindow::readDoubleRequired(const QString &text, const QString &name, double *value, QString *errorMessage) const
{
    bool ok = false;
    const double parsed = text.toDouble(&ok);
    if (!ok || !qIsFinite(parsed)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("%1 is invalid: %2").arg(name, text);
        return false;
    }
    if (value)
        *value = parsed;
    return true;
}

bool MainWindow::readPortRequired(const QString &text, quint16 *value, QString *errorMessage) const
{
    bool ok = false;
    const int parsed = text.toInt(&ok);
    if (!ok || parsed <= 0 || parsed > 65535) {
        if (errorMessage)
            *errorMessage = QStringLiteral("AxisPort is invalid: %1").arg(text);
        return false;
    }
    if (value)
        *value = static_cast<quint16>(parsed);
    return true;
}

QVector<MainWindow::StandardSpec> MainWindow::buildStandards(const ParamSettings &settings, QString *errorMessage) const
{
    QVector<StandardSpec> standards;
    standards.reserve(4);
    const QStringList xs = { settings.posStandard1X, settings.posStandard2X, settings.posStandard3X, settings.posStandard4X };
    const QStringList ys = { settings.posStandard1Y, settings.posStandard2Y, settings.posStandard3Y, settings.posStandard4Y };
    const QStringList thks = { settings.standardThickness1, settings.standardThickness2, settings.standardThickness3, settings.standardThickness4 };
    const QStringList totals = { settings.standardTotalVal1, settings.standardTotalVal2, settings.standardTotalVal3, settings.standardTotalVal4 };
    const QList<int> ids = { 500, 725, 800, 1550 };

    for (int i = 0; i < 4; ++i) {
        StandardSpec spec;
        spec.row = i;
        spec.id = ids.at(i);
        if (!readDoubleRequired(xs.at(i), QStringLiteral("PosStandard%1X").arg(i + 1), &spec.x, errorMessage) ||
            !readDoubleRequired(ys.at(i), QStringLiteral("PosStandard%1Y").arg(i + 1), &spec.y, errorMessage) ||
            !readDoubleRequired(thks.at(i), QStringLiteral("StandardThickness%1").arg(i + 1), &spec.thickness, errorMessage)) {
            return {};
        }
        if (spec.thickness <= 0.0) {
            if (errorMessage)
                *errorMessage = QStringLiteral("StandardThickness%1 must be greater than 0.").arg(i + 1);
            return {};
        }
        bool totalOk = false;
        spec.historyTotal = totals.at(i).toDouble(&totalOk);
        if (!totalOk || !qIsFinite(spec.historyTotal))
            spec.historyTotal = 0.0;
        standards.append(spec);
    }
    return standards;
}

void MainWindow::onConnectMotionClicked()
{
    ParamSettings settings = readSettingsFromUi();
    QString error;
    quint16 port = 0;
    if (settings.axisIp.isEmpty() || !readPortRequired(settings.axisPort, &port, &error)) {
        appendLog(error.isEmpty() ? QStringLiteral("AxisIP is empty.") : error);
        return;
    }
    m_motionController.connectToController(settings.axisIp, port);
}

void MainWindow::onDisconnectMotionClicked()
{
    m_motionController.disconnectFromController();
}

void MainWindow::onMotionConnectedChanged(bool connected)
{
    setMotionStatusText(connected ? QStringLiteral("Connected") : QStringLiteral("Disconnected"));
    if (connected) {
        m_motionController.readRealTimeStatus();
        m_motionController.readRealTimePos();
        m_motionController.ctrlAutoSnd(100);
    }
}

void MainWindow::onMotionPositionUpdated(const Motion::AxisPosition &position)
{
    {
        QMutexLocker locker(&m_stateMutex);
        m_axisPosition = position;
    }
    ui->label_axis_pos->setText(QStringLiteral("X=%1 Y=%2 Z=%3")
        .arg(formatNumber(position.x))
        .arg(formatNumber(position.y))
        .arg(formatNumber(position.z)));
}

void MainWindow::onMotionStateUpdated(const Motion::AxisStateSnapshot &state)
{
    {
        QMutexLocker locker(&m_stateMutex);
        m_axisState = state;
    }
    ui->label_axis_state->setText(QStringLiteral("X=%1/%2 Y=%3/%4 Z=%5/%6")
        .arg(state.stateX).arg(state.errorX)
        .arg(state.stateY).arg(state.errorY)
        .arg(state.stateZ).arg(state.errorZ));
}

void MainWindow::onConnectColorFocusClicked()
{
    ParamSettings settings = readSettingsFromUi();
    if (settings.colorFocusIpTop.isEmpty() || settings.colorFocusIpBottom.isEmpty()) {
        appendLog(QStringLiteral("Color focus IP is empty."));
        return;
    }

    const bool topOk = invokeColorFocus(m_topFocus, [this, settings]() { return m_topFocus->ConnectDevice(settings.colorFocusIpTop); }, QStringLiteral("Connect top color focus"));
    const bool bottomOk = invokeColorFocus(m_bottomFocus, [this, settings]() { return m_bottomFocus->ConnectDevice(settings.colorFocusIpBottom); }, QStringLiteral("Connect bottom color focus"));
    if (topOk && bottomOk)
        appendLog(QStringLiteral("Color focus connected."));
}

void MainWindow::onStartAcquisitionClicked()
{
    const bool topOk = invokeColorFocus(m_topFocus, [this]() { return m_topFocus->StartAcquisition(TriggerContinue, 100); }, QStringLiteral("Start top acquisition"));
    const bool bottomOk = invokeColorFocus(m_bottomFocus, [this]() { return m_bottomFocus->StartAcquisition(TriggerContinue, 100); }, QStringLiteral("Start bottom acquisition"));
    if (topOk && bottomOk)
        appendLog(QStringLiteral("Color focus acquisition started."));
}

void MainWindow::onStopAcquisitionClicked()
{
    stopColorFocusAcquisition();
}

void MainWindow::onStopMotionClicked()
{
    m_cancelRequested = true;
    if (!m_motionController.abortAxes())
        appendLog(QStringLiteral("Abort axes failed."));
}

void MainWindow::onColorFocusSampleUpdated(int sensorId, float distance, float intensity, int encoder1, int encoder2, int encoder3)
{
    Q_UNUSED(intensity)
    Q_UNUSED(encoder1)
    Q_UNUSED(encoder2)
    Q_UNUSED(encoder3)

    {
        QMutexLocker locker(&m_stateMutex);
        if (sensorId == kTopSensorId) {
            m_topDistance = distance;
            m_topDistanceValid = qIsFinite(static_cast<double>(distance));
        } else if (sensorId == kBottomSensorId) {
            m_bottomDistance = distance;
            m_bottomDistanceValid = qIsFinite(static_cast<double>(distance));
        }
    }

    if (sensorId == kTopSensorId)
        ui->label_top_distance->setText(formatNumber(distance));
    else if (sensorId == kBottomSensorId)
        ui->label_bottom_distance->setText(formatNumber(distance));
}

void MainWindow::onMoveStandardClicked()
{
    ParamSettings settings = readSettingsFromUi();
    QString error;
    QVector<StandardSpec> standards = buildStandards(settings, &error);
    if (standards.isEmpty()) {
        appendLog(error);
        return;
    }

    QPushButton *button = qobject_cast<QPushButton *>(sender());
    int row = -1;
    if (button == ui->pushButton_move_s1)
        row = 0;
    else if (button == ui->pushButton_move_s2)
        row = 1;
    else if (button == ui->pushButton_move_s3)
        row = 2;
    else if (button == ui->pushButton_move_s4)
        row = 3;
    if (row < 0)
        return;

    if (!m_motionController.moveMultiAxisLinear(standards.at(row).x, standards.at(row).y,
                                                settings.velocityNormal.toDouble(),
                                                settings.velocityNormal.toDouble() * 10.0,
                                                settings.velocityNormal.toDouble() * 10.0)) {
        appendLog(QStringLiteral("Move standard %1 failed.").arg(row + 1));
    }
}

void MainWindow::onFourCalibrationClicked()
{
    if (m_busy)
        return;
    ParamSettings settings = readSettingsFromUi();
    QString error;
    if (!validateSettings(settings, &error) || !ensureDevicesReady(&error)) {
        appendLog(error);
        QMessageBox::warning(this, QStringLiteral("Calibration"), error);
        return;
    }
    setBusy(true);
    clearRunTotals();
    m_cancelRequested = false;
    m_calibrationFuture = QtConcurrent::run([this, settings]() { runFourCalibration(settings); });
}

void MainWindow::onStandard1550CalibrationClicked()
{
    if (m_busy)
        return;
    ParamSettings settings = readSettingsFromUi();
    QString error;
    if (!validateSettings(settings, &error) || !ensureDevicesReady(&error)) {
        appendLog(error);
        QMessageBox::warning(this, QStringLiteral("Calibration"), error);
        return;
    }
    setBusy(true);
    m_cancelRequested = false;
    m_calibrationFuture = QtConcurrent::run([this, settings]() { runStandard1550Calibration(settings); });
}

bool MainWindow::ensureDevicesReady(QString *errorMessage) const
{
    if (!m_motionController.isConnected()) {
        *errorMessage = QStringLiteral("Motion controller is not connected.");
        return false;
    }
    if (!m_topConnected || !m_bottomConnected) {
        *errorMessage = QStringLiteral("Color focus is not connected.");
        return false;
    }
    if (!m_topAcquiring || !m_bottomAcquiring) {
        *errorMessage = QStringLiteral("Color focus acquisition is not started.");
        return false;
    }
    DistanceSnapshot snapshot = latestDistanceSnapshot();
    if (!snapshot.topValid || !snapshot.bottomValid) {
        *errorMessage = QStringLiteral("Color focus has no valid latest top/bottom sample. topValid=%1 bottomValid=%2 top=%3 bottom=%4")
            .arg(snapshot.topValid)
            .arg(snapshot.bottomValid)
            .arg(snapshot.top)
            .arg(snapshot.bottom);
        return false;
    }
    return true;
}

void MainWindow::runFourCalibration(ParamSettings settings)
{
    appendLog(QStringLiteral("Four standard calibration started."));
    QString error;
    QVector<StandardSpec> standards = buildStandards(settings, &error);
    if (standards.size() != 4) {
        failCalibration(error, settings);
        return;
    }

    double z = 0.0;
    double velocity = 0.0;
    if (!readDoubleRequired(settings.z_default_123, QStringLiteral("ZDefault123"), &z, &error) ||
        !readDoubleRequired(settings.velocityNormal, QStringLiteral("NormalSpeed"), &velocity, &error) ||
        !moveZBlocking(z, &error)) {
        failCalibration(error, settings);
        return;
    }

    double dx = kScanOffset;
    double dy = kScanOffset;
    if (settings.CalibrationDirectionOfX.toInt() == 1)
        dy = 0.0;
    else
        dx = 0.0;

    QVector<double> totals;
    totals.reserve(4);
    for (const StandardSpec &spec : standards) {
        if (m_cancelRequested) {
            failCalibration(QStringLiteral("Calibration canceled."), settings);
            return;
        }
        double total = 0.0;
        if (!measureStandard(spec, dx, dy, velocity, &total, &error)) {
            failCalibration(error, settings);
            return;
        }
        totals.append(total);
    }

    const auto minMax = std::minmax_element(totals.constBegin(), totals.constEnd());
    if (*minMax.second - *minMax.first > kCalibrationThreshold) {
        failCalibration(QStringLiteral("Calibration total deviation is too large, min=%1 max=%2 threshold=%3.")
                            .arg(*minMax.first).arg(*minMax.second).arg(kCalibrationThreshold),
                        settings);
        return;
    }

    settings.standardTotalVal1 = QString::number(totals.at(0), 'f', 6);
    settings.standardTotalVal2 = QString::number(totals.at(1), 'f', 6);
    settings.standardTotalVal3 = QString::number(totals.at(2), 'f', 6);
    settings.standardTotalVal4 = QString::number(totals.at(3), 'f', 6);
    settings.lastCalTimeStamp_500_1500 = QString::number(QDateTime::currentSecsSinceEpoch());

    if (!m_configManager.saveToXml(settings, &error)) {
        failCalibration(error, settings);
        return;
    }
    finishCalibration(true, QStringLiteral("Four standard calibration finished."), settings);
}

void MainWindow::runStandard1550Calibration(ParamSettings settings)
{
    appendLog(QStringLiteral("1550 calibration started."));
    QString error;
    QVector<StandardSpec> standards = buildStandards(settings, &error);
    if (standards.size() != 4) {
        failCalibration(error, settings);
        return;
    }

    double z = 0.0;
    double velocity = 0.0;
    if (!readDoubleRequired(settings.z_default_4, QStringLiteral("ZDefault4"), &z, &error) ||
        !readDoubleRequired(settings.velocityNormal, QStringLiteral("NormalSpeed"), &velocity, &error) ||
        !moveZBlocking(z, &error)) {
        failCalibration(error, settings);
        return;
    }

    double dx = kScanOffset;
    double dy = kScanOffset;
    if (settings.CalibrationDirectionOfX.toInt() == 1)
        dy = 0.0;
    else
        dx = 0.0;

    QVector<double> totals;
    totals.reserve(3);
    const StandardSpec spec = standards.at(3);
    for (int i = 0; i < 3; ++i) {
        if (m_cancelRequested) {
            failCalibration(QStringLiteral("Calibration canceled."), settings);
            return;
        }
        double total = 0.0;
        if (!measureStandard(spec, dx, dy, velocity, &total, &error)) {
            failCalibration(error, settings);
            return;
        }
        totals.append(total);
    }

    const auto minMax = std::minmax_element(totals.constBegin(), totals.constEnd());
    if (*minMax.second - *minMax.first > kCalibrationThreshold) {
        failCalibration(QStringLiteral("1550 calibration deviation is too large, min=%1 max=%2 threshold=%3.")
                            .arg(*minMax.first).arg(*minMax.second).arg(kCalibrationThreshold),
                        settings);
        return;
    }

    const double avg = std::accumulate(totals.constBegin(), totals.constEnd(), 0.0) / totals.size();
    settings.standardTotalVal4 = QString::number(avg, 'f', 6);
    settings.lastCalTimeStamp_1550 = QString::number(QDateTime::currentSecsSinceEpoch());

    if (!m_configManager.saveToXml(settings, &error)) {
        failCalibration(error, settings);
        return;
    }
    finishCalibration(true, QStringLiteral("1550 calibration finished."), settings);
}

bool MainWindow::moveZBlocking(double z, QString *errorMessage)
{
    if (!invokeMotion([this, z]() { return m_motionController.moveSingleAxisAbsolute(Motion::AxisZ, z); },
                      QStringLiteral("Move Z"), errorMessage)) {
        return false;
    }
    return waitForZ(z, errorMessage);
}

bool MainWindow::moveXyBlocking(double x, double y, double velocity, QString *errorMessage)
{
    if (!invokeMotion([this, x, y, velocity]() {
            return m_motionController.moveMultiAxisLinear(x, y, velocity, velocity * 10.0, velocity * 10.0);
        }, QStringLiteral("Move XY"), errorMessage)) {
        return false;
    }
    return waitForXy(x, y, errorMessage);
}

bool MainWindow::waitForZ(double z, QString *errorMessage)
{
    QElapsedTimer timer;
    timer.start();
    while (!m_cancelRequested) {
        Motion::AxisPosition position;
        Motion::AxisStateSnapshot state;
        {
            QMutexLocker locker(&m_stateMutex);
            position = m_axisPosition;
            state = m_axisState;
        }
        if (position.valid && state.valid &&
            qAbs(position.z - z) <= kPositionTolerance &&
            readyState(state.stateZ)) {
            return true;
        }
        if (timer.elapsed() > kWaitTimeoutMs) {
            *errorMessage = QStringLiteral("Wait Z timeout. target=%1 current=%2 state=%3 error=%4")
                .arg(z).arg(position.z).arg(state.stateZ).arg(state.errorZ);
            return false;
        }
        QThread::msleep(50);
    }
    *errorMessage = QStringLiteral("Wait Z canceled.");
    return false;
}

bool MainWindow::waitForXy(double x, double y, QString *errorMessage)
{
    QElapsedTimer timer;
    timer.start();
    while (!m_cancelRequested) {
        Motion::AxisPosition position;
        Motion::AxisStateSnapshot state;
        {
            QMutexLocker locker(&m_stateMutex);
            position = m_axisPosition;
            state = m_axisState;
        }
        if (position.valid && state.valid &&
            qAbs(position.x - x) <= kPositionTolerance &&
            qAbs(position.y - y) <= kPositionTolerance &&
            readyState(state.stateX) && readyState(state.stateY)) {
            return true;
        }
        if (timer.elapsed() > kWaitTimeoutMs) {
            *errorMessage = QStringLiteral("Wait XY timeout. target=(%1,%2) current=(%3,%4) state=(%5,%6) error=(%7,%8)")
                .arg(x).arg(y).arg(position.x).arg(position.y)
                .arg(state.stateX).arg(state.stateY).arg(state.errorX).arg(state.errorY);
            return false;
        }
        QThread::msleep(50);
    }
    *errorMessage = QStringLiteral("Wait XY canceled.");
    return false;
}

bool MainWindow::invokeMotion(const std::function<bool()> &call, const QString &action, QString *errorMessage)
{
    bool result = false;
    const bool invoked = QMetaObject::invokeMethod(&m_motionController, [&]() { result = call(); }, Qt::BlockingQueuedConnection);
    if (!invoked) {
        *errorMessage = QStringLiteral("%1 failed, invokeMethod failed.").arg(action);
        return false;
    }
    if (!result) {
        *errorMessage = QStringLiteral("%1 failed. See motion log for details.").arg(action);
        return false;
    }
    return true;
}

bool MainWindow::measureStandard(const StandardSpec &spec, double dx, double dy, double velocity, double *total, QString *errorMessage)
{
    appendLog(QStringLiteral("Measure standard %1 started.").arg(spec.id));
    if (!moveXyBlocking(spec.x + dx, spec.y + dy, velocity, errorMessage))
        return false;

    if (!invokeMotion([this, spec, dx, dy]() {
            return m_motionController.moveMultiAxisLinear(spec.x - dx, spec.y - dy,
                                                          kScanVelocity, kScanAcceleration, kScanDeceleration);
        }, QStringLiteral("Scan standard"), errorMessage)) {
        return false;
    }

    m_filterBuffer.clear();
    double sum = 0.0;
    double lastTop = 0.0;
    double lastBottom = 0.0;
    for (int i = 0; i < kSampleCount; ++i) {
        if (m_cancelRequested) {
            *errorMessage = QStringLiteral("Measure standard %1 canceled.").arg(spec.id);
            return false;
        }
        DistanceSnapshot snapshot = latestDistanceSnapshot();
        if (!snapshot.topValid || !snapshot.bottomValid ||
            !qIsFinite(snapshot.top) || !qIsFinite(snapshot.bottom) ||
            qFuzzyIsNull(snapshot.top) || qFuzzyIsNull(snapshot.bottom)) {
            Motion::AxisPosition position;
            Motion::AxisStateSnapshot state;
            {
                QMutexLocker locker(&m_stateMutex);
                position = m_axisPosition;
                state = m_axisState;
            }
            *errorMessage = QStringLiteral("Invalid color focus latest sample for standard %1 sample=%2. topValid=%3 bottomValid=%4 top=%5 bottom=%6 axis=(%7,%8,%9) state=(%10,%11,%12)")
                .arg(spec.id)
                .arg(i + 1)
                .arg(snapshot.topValid)
                .arg(snapshot.bottomValid)
                .arg(snapshot.top)
                .arg(snapshot.bottom)
                .arg(position.x)
                .arg(position.y)
                .arg(position.z)
                .arg(state.stateX)
                .arg(state.stateY)
                .arg(state.stateZ);
            return false;
        }
        lastTop = snapshot.top;
        lastBottom = snapshot.bottom;
        sum += filterValue(snapshot.top + snapshot.bottom + spec.thickness);
        QThread::msleep(kSampleIntervalMs);
    }

    if (!waitForXy(spec.x - dx, spec.y - dy, errorMessage))
        return false;

    *total = sum / kSampleCount + kStandardCompensationValue;
    QMetaObject::invokeMethod(this, [this, spec, lastTop, lastBottom, totalValue = *total]() {
        updateResultRow(spec, lastTop, lastBottom, totalValue);
    }, Qt::QueuedConnection);
    appendLog(QStringLiteral("Measure standard %1 total=%2.").arg(spec.id).arg(*total));
    return true;
}

double MainWindow::filterValue(double newValue, int windowSize)
{
    m_filterBuffer.append(newValue);
    if (m_filterBuffer.size() > windowSize)
        m_filterBuffer.removeFirst();

    QVector<double> sorted = m_filterBuffer;
    std::sort(sorted.begin(), sorted.end());
    const double median = sorted.at(sorted.size() / 2);
    const double average = std::accumulate(sorted.constBegin(), sorted.constEnd(), 0.0) / sorted.size();
    return median * 0.7 + average * 0.3;
}

MainWindow::DistanceSnapshot MainWindow::latestDistanceSnapshot() const
{
    DistanceSnapshot snapshot;
    float topDistance = 0.0f;
    float topIntensity = 0.0f;
    float bottomDistance = 0.0f;
    float bottomIntensity = 0.0f;
    if (m_topFocus) {
        snapshot.topValid = m_topFocus->GetLatestSample(&topDistance, &topIntensity);
        snapshot.top = topDistance;
    }
    if (m_bottomFocus) {
        snapshot.bottomValid = m_bottomFocus->GetLatestSample(&bottomDistance, &bottomIntensity);
        snapshot.bottom = bottomDistance;
    }
    return snapshot;
}

MainWindow::DistanceSnapshot MainWindow::distanceSnapshot() const
{
    QMutexLocker locker(&m_stateMutex);
    DistanceSnapshot snapshot;
    snapshot.topValid = m_topDistanceValid;
    snapshot.bottomValid = m_bottomDistanceValid;
    snapshot.top = m_topDistance;
    snapshot.bottom = m_bottomDistance;
    return snapshot;
}

void MainWindow::updateResultRow(const StandardSpec &spec, double top, double bottom, double total)
{
    ui->table_result->item(spec.row, 1)->setText(formatNumber(spec.thickness));
    ui->table_result->item(spec.row, 2)->setText(formatNumber(top));
    ui->table_result->item(spec.row, 3)->setText(formatNumber(bottom));
    ui->table_result->item(spec.row, 4)->setText(formatNumber(total));
    if (spec.historyTotal > 0.0) {
        ui->table_result->item(spec.row, 5)->setText(formatNumber(spec.historyTotal));
        ui->table_result->item(spec.row, 6)->setText(formatNumber(total - spec.historyTotal));
    } else {
        ui->table_result->item(spec.row, 5)->setText(QStringLiteral("--"));
        ui->table_result->item(spec.row, 6)->setText(QStringLiteral("--"));
    }
}

void MainWindow::clearRunTotals()
{
    for (int row = 0; row < ui->table_result->rowCount(); ++row) {
        ui->table_result->item(row, 2)->setText(QStringLiteral("--"));
        ui->table_result->item(row, 3)->setText(QStringLiteral("--"));
        ui->table_result->item(row, 4)->setText(QStringLiteral("--"));
        ui->table_result->item(row, 6)->setText(QStringLiteral("--"));
    }
}

void MainWindow::finishCalibration(bool success, const QString &message, const ParamSettings &settings)
{
    QMetaObject::invokeMethod(this, [this, success, message, settings]() {
        m_settings = settings;
        applySettingsToUi(m_settings);
        setBusy(false);
        appendLog(message);
        QMessageBox::information(this, QStringLiteral("Calibration"), message);
        statusBar()->showMessage(success ? QStringLiteral("Calibration finished") : QStringLiteral("Calibration failed"));
    }, Qt::QueuedConnection);
}

void MainWindow::failCalibration(const QString &message, const ParamSettings &settings)
{
    QMetaObject::invokeMethod(this, [this, message, settings]() {
        Q_UNUSED(settings)
        setBusy(false);
        appendLog(message);
        QMessageBox::warning(this, QStringLiteral("Calibration"), message);
        statusBar()->showMessage(QStringLiteral("Calibration failed"));
    }, Qt::QueuedConnection);
}

void MainWindow::setBusy(bool busy)
{
    m_busy = busy;
    ui->pushButton_four_calibration->setEnabled(!busy);
    ui->pushButton_1550_calibration->setEnabled(!busy);
    ui->pushButton_load_config->setEnabled(!busy);
    ui->pushButton_save_config->setEnabled(!busy);
    statusBar()->showMessage(busy ? QStringLiteral("Calibration running") : QStringLiteral("Ready"));
}

void MainWindow::setMotionStatusText(const QString &text)
{
    ui->label_motion_status->setText(text);
}

void MainWindow::setColorFocusStatusText()
{
    ui->label_focus_status->setText(QStringLiteral("Top:%1/%2 Bottom:%3/%4")
        .arg(m_topConnected ? QStringLiteral("Conn") : QStringLiteral("Disc"))
        .arg(m_topAcquiring ? QStringLiteral("Acq") : QStringLiteral("Stop"))
        .arg(m_bottomConnected ? QStringLiteral("Conn") : QStringLiteral("Disc"))
        .arg(m_bottomAcquiring ? QStringLiteral("Acq") : QStringLiteral("Stop")));
}

QString MainWindow::formatNumber(double value, int precision) const
{
    if (!qIsFinite(value))
        return QStringLiteral("--");
    return QString::number(value, 'f', precision);
}

void MainWindow::appendLog(const QString &message)
{
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, [this, message]() { appendLog(message); }, Qt::QueuedConnection);
        return;
    }
    const QString text = QStringLiteral("[%1] %2")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")), message);
    ui->plainTextEdit_log->appendPlainText(text);
    statusBar()->showMessage(message, 5000);
}

void MainWindow::stopColorFocus()
{
    stopColorFocusAcquisition();
    invokeColorFocus(m_topFocus, [this]() { return m_topFocus->DisconnectDevice(); }, QStringLiteral("Disconnect top color focus"));
    invokeColorFocus(m_bottomFocus, [this]() { return m_bottomFocus->DisconnectDevice(); }, QStringLiteral("Disconnect bottom color focus"));
    if (m_focusThread->isRunning()) {
        m_topFocus->deleteLater();
        m_bottomFocus->deleteLater();
        m_focusThread->quit();
        m_focusThread->wait(3000);
    }
}

void MainWindow::stopColorFocusAcquisition()
{
    invokeColorFocus(m_topFocus, [this]() { return m_topFocus->StopAcquisition(); }, QStringLiteral("Stop top acquisition"));
    invokeColorFocus(m_bottomFocus, [this]() { return m_bottomFocus->StopAcquisition(); }, QStringLiteral("Stop bottom acquisition"));
}

bool MainWindow::invokeColorFocus(ColorFocusControl *control, const std::function<bool()> &call, const QString &action)
{
    if (!control)
        return false;
    bool result = false;
    const bool invoked = QMetaObject::invokeMethod(control, [&]() { result = call(); }, Qt::BlockingQueuedConnection);
    if (!invoked) {
        appendLog(QStringLiteral("%1 failed, invokeMethod failed.").arg(action));
        return false;
    }
    if (!result) {
        appendLog(QStringLiteral("%1 failed.").arg(action));
        return false;
    }
    return true;
}

void MainWindow::onBrowseCsvClicked()
{
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("CSV"), QString(), QStringLiteral("CSV (*.csv)"));
    if (!path.isEmpty())
        ui->lineEdit_csv_path->setText(path);
}

void MainWindow::onRunAlgorithmClicked()
{
    const QString path = ui->lineEdit_csv_path->text().trimmed();
    if (path.isEmpty()) {
        appendLog(QStringLiteral("CSV path is empty."));
        return;
    }

    ParamSettings settings = readSettingsFromUi();
    Wafer::AlgorithmOptions options;
    options.lineSampleNum = settings.LineSampleNum.toInt() > 0 ? settings.LineSampleNum.toInt() : 51;
    options.useNewBowAlg = settings.IsNewBowAlg.toInt() == 1;
    options.useNewWarpAlg = settings.IsNewWarpAlg.toInt() == 1;
    options.trimSize = settings.trimSize.toDouble();
    options.chordLength = settings.ChordLength.toDouble();
    options.calibrationTotal = settings.standardTotalVal4.toDouble();

    Wafer::WaferCsvLoader loader;
    Wafer::LoadResult loadResult = QFileInfo(path).isDir() ? loader.loadDirectory(path, options) : loader.loadFile(path, options);
    if (!loadResult.success) {
        ui->plainTextEdit_algo_result->setPlainText(loadResult.errorMessage + QStringLiteral("\n") + loadResult.logs.join(QStringLiteral("\n")));
        appendLog(loadResult.errorMessage);
        return;
    }

    Wafer::WaferAlgorithm algorithm;
    Wafer::AlgorithmResult result = algorithm.runAll(loadResult.dataset, options);
    QStringList lines;
    lines << QStringLiteral("success=%1").arg(result.success)
          << QStringLiteral("errorMessage=%1").arg(result.errorMessage)
          << QStringLiteral("averageThk=%1").arg(result.hasAverageThk ? QString::number(result.averageThk, 'f', 6) : QStringLiteral("--"))
          << QStringLiteral("centerThk=%1").arg(result.hasCenterThk ? QString::number(result.centerThk, 'f', 6) : QStringLiteral("--"))
          << QStringLiteral("ttv=%1").arg(result.hasTtv ? QString::number(result.ttv, 'f', 6) : QStringLiteral("--"))
          << QStringLiteral("bow=%1").arg(result.hasBow ? QString::number(result.bow, 'f', 6) : QStringLiteral("--"))
          << QStringLiteral("warp=%1").arg(result.hasWarp ? QString::number(result.warp, 'f', 6) : QStringLiteral("--"))
          << QStringLiteral("sori=%1").arg(result.hasSori ? QString::number(result.sori, 'f', 6) : QStringLiteral("--"))
          << QStringLiteral("")
          << loadResult.logs
          << result.logs;
    ui->plainTextEdit_algo_result->setPlainText(lines.join(QStringLiteral("\n")));
    appendLog(result.success ? QStringLiteral("Algorithm finished.") : result.errorMessage);
}
