#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QDateTime>
#include <QDoubleSpinBox>
#include <QSpinBox>

namespace
{
QString displayMessage(const QString &message)
{
    if (message.startsWith("TX: ")) {
        return QString(u8"发送: %1").arg(message.mid(4));
    }
    if (message.startsWith("RX: ")) {
        return QString(u8"接收: %1").arg(message.mid(4));
    }
    if (message.startsWith("ERROR: ")) {
        return QString(u8"错误: %1").arg(message.mid(7));
    }
    return message;
}

void initDoubleSpin(QDoubleSpinBox *spin, double minValue, double maxValue, double value, int decimals = 3)
{
    spin->setRange(minValue, maxValue);
    spin->setDecimals(decimals);
    spin->setValue(value);
    spin->setSingleStep(1.0);
}

void initIntSpin(QSpinBox *spin, int minValue, int maxValue, int value)
{
    spin->setRange(minValue, maxValue);
    spin->setValue(value);
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    initUiState();
    connectUiSignals();

    connect(&m_controller, &MotionController::connectionStatusChanged,
            this, &MainWindow::onConnectionChanged);
    connect(&m_controller, &MotionController::logMessage,
            this, &MainWindow::appendLog);
    connect(&m_controller, &MotionController::errorMessage,
            this, &MainWindow::appendLog);
    connect(&m_controller, &MotionController::positionUpdated,
            this, &MainWindow::onPositionUpdated);
    connect(&m_controller, &MotionController::stateUpdated,
            this, &MainWindow::onStateUpdated);
    connect(&m_controller, &MotionController::homeStatusUpdated,
            this, &MainWindow::onHomeStatusUpdated);
    connect(&m_readTimer, &QTimer::timeout,
            this, &MainWindow::onTimerRead);

    updateUiEnabled(false);
}

MainWindow::~MainWindow()
{
    m_readTimer.stop();
    if (m_controller.isConnected()) {
        m_controller.disconnectFromController();
    }
    delete ui;
}

void MainWindow::onConnectClicked()
{
    const QString host = ui->ipEdit->text().trimmed();
    if (host.isEmpty()) {
        appendLog(QString(u8"主机地址不能为空."));
        return;
    }
    m_controller.connectToController(host, static_cast<quint16>(ui->portSpin->value()));
}

void MainWindow::onDisconnectClicked()
{
    m_readTimer.stop();
    ui->autoReadCheck->blockSignals(true);
    ui->autoReadCheck->setChecked(false);
    ui->autoReadCheck->blockSignals(false);
    ui->ctrlAutoSndCheck->blockSignals(true);
    ui->ctrlAutoSndCheck->setChecked(false);
    ui->ctrlAutoSndCheck->blockSignals(false);
    m_controller.disconnectFromController();
}

void MainWindow::onEnableClicked()
{
    m_controller.enableAxis(currentAxis());
}

void MainWindow::onDisableClicked()
{
    m_controller.disableAxis(currentAxis());
}

void MainWindow::onHomeClicked()
{
    m_controller.homeAxisAsync(currentAxis());
}

void MainWindow::onClearErrorClicked()
{
    m_controller.clearError();
}

void MainWindow::onStopAxisClicked()
{
    m_controller.abortAxis(currentAxis());
}

void MainWindow::onAbsoluteMoveClicked()
{
    m_controller.moveSingleAxisAbsolute(currentAxis(), ui->absPositionSpin->value());
}

void MainWindow::onRelativeMoveClicked()
{
    m_controller.moveSingleAxisIncremental(currentAxis(), ui->relDistanceSpin->value());
}

void MainWindow::onContinuousPositiveClicked()
{
    m_controller.moveSingleAxisContinuous(currentAxis(), 1);
}

void MainWindow::onContinuousNegativeClicked()
{
    m_controller.moveSingleAxisContinuous(currentAxis(), 0);
}

void MainWindow::onSetVelocityClicked()
{
    if (!ensurePositive(ui->velocitySpin, QString(u8"速度")) ||
        !ensurePositive(ui->accelerationSpin, QString(u8"加速度")) ||
        !ensurePositive(ui->decelerationSpin, QString(u8"减速度"))) {
        return;
    }

    m_controller.setVelocity(currentAxis(),
                             ui->velocitySpin->value(),
                             ui->accelerationSpin->value(),
                             ui->decelerationSpin->value());
}

void MainWindow::onLineMoveClicked()
{
    if (!ensurePositive(ui->lineVelocitySpin, QString(u8"XY速度")) ||
        !ensurePositive(ui->lineAccelerationSpin, QString(u8"XY加速度")) ||
        !ensurePositive(ui->lineDecelerationSpin, QString(u8"XY减速度"))) {
        return;
    }

    m_controller.moveMultiAxisLinear(ui->lineXSpin->value(),
                                     ui->lineYSpin->value(),
                                     ui->lineVelocitySpin->value(),
                                     ui->lineAccelerationSpin->value(),
                                     ui->lineDecelerationSpin->value());
}

void MainWindow::onStopLineClicked()
{
    m_controller.abortAxes();
}

void MainWindow::onSetTriggerClicked()
{
    Motion::TriggerParam param;
    param.axis = triggerAxis();
    param.startPos = ui->triggerStartSpin->value();
    param.endPos = ui->triggerEndSpin->value();
    param.interval = ui->triggerIntervalSpin->value();
    param.direction = ui->triggerDirectionSpin->value();
    param.pulseWidth = ui->triggerPulseSpin->value();
    m_controller.setTriggerParam(param);
}

void MainWindow::onTriggerOnClicked()
{
    m_controller.controlTrigger(triggerAxis(), true);
}

void MainWindow::onTriggerOffClicked()
{
    m_controller.controlTrigger(triggerAxis(), false);
}

void MainWindow::onAutoReadToggled(bool enabled)
{
    if (!enabled) {
        m_readTimer.stop();
        appendLog(QString(u8"手动轮询已停止."));
        return;
    }

    if (!m_controller.isConnected()) {
        appendLog(QString(u8"控制器未连接, 无法开启手动轮询."));
        ui->autoReadCheck->setChecked(false);
        return;
    }

    m_readTimer.start(ui->autoReadIntervalSpin->value());
    appendLog(QString(u8"手动轮询已开启, 周期=%1 ms.").arg(ui->autoReadIntervalSpin->value()));
}

void MainWindow::onCtrlAutoSndToggled(bool enabled)
{
    if (enabled) {
        if (!m_controller.ctrlAutoSnd(ui->ctrlAutoSndIntervalSpin->value())) {
            ui->ctrlAutoSndCheck->setChecked(false);
        }
    } else {
        m_controller.ctrlAutoSnd(0);
    }
}

void MainWindow::onManualReadClicked()
{
    m_controller.readRealTimePos();
    m_controller.readRealTimeStatus();
    m_controller.getHomestatus();
}

void MainWindow::onTimerRead()
{
    m_controller.readRealTimePos();
    m_controller.readRealTimeStatus();
}

void MainWindow::onConnectionChanged(bool connected)
{
    ui->connectionStatusLabel->setText(connected ? QString(u8"已连接") : QString(u8"已断开"));
    updateUiEnabled(connected);
    if (connected) {
        m_controller.readRealTimePos();
        m_controller.readRealTimeStatus();
        m_controller.getHomestatus();
    } else {
        m_readTimer.stop();
        ui->autoReadCheck->blockSignals(true);
        ui->autoReadCheck->setChecked(false);
        ui->autoReadCheck->blockSignals(false);
        ui->ctrlAutoSndCheck->blockSignals(true);
        ui->ctrlAutoSndCheck->setChecked(false);
        ui->ctrlAutoSndCheck->blockSignals(false);
    }
}

void MainWindow::appendLog(const QString &message)
{
    const QString time = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    ui->logEdit->appendPlainText(QString("[%1] %2").arg(time, displayMessage(message)));
}

void MainWindow::onPositionUpdated(const Motion::AxisPosition &position)
{
    ui->posXLabel->setText(QString::number(position.x, 'f', 3));
    ui->posYLabel->setText(QString::number(position.y, 'f', 3));
    ui->posZLabel->setText(QString::number(position.z, 'f', 3));
}

void MainWindow::onStateUpdated(const Motion::AxisStateSnapshot &state)
{
    ui->stateXLabel->setText(QString::number(state.stateX));
    ui->stateYLabel->setText(QString::number(state.stateY));
    ui->stateZLabel->setText(QString::number(state.stateZ));
    ui->errorXLabel->setText(QString::number(state.errorX));
    ui->errorYLabel->setText(QString::number(state.errorY));
    ui->errorZLabel->setText(QString::number(state.errorZ));
}

void MainWindow::onHomeStatusUpdated(bool homeX, bool homeY, bool homeZ)
{
    ui->homeLabel->setText(QString(u8"回零 X=%1, Y=%2, Z=%3")
                           .arg(homeX ? "1" : "0")
                           .arg(homeY ? "1" : "0")
                           .arg(homeZ ? "1" : "0"));
}

void MainWindow::initUiState()
{
    ui->axisCombo->addItem("X", Motion::AxisX);
    ui->axisCombo->addItem("Y", Motion::AxisY);
    ui->axisCombo->addItem("Z", Motion::AxisZ);
    ui->triggerAxisCombo->addItem("X", Motion::AxisX);
    ui->triggerAxisCombo->addItem("Y", Motion::AxisY);
    ui->triggerAxisCombo->addItem("Z", Motion::AxisZ);

    initDoubleSpin(ui->absPositionSpin, -1000000.0, 1000000.0, 0.0);
    initDoubleSpin(ui->relDistanceSpin, -1000000.0, 1000000.0, 0.0);
    initDoubleSpin(ui->velocitySpin, 0.001, 1000000.0, 10.0);
    initDoubleSpin(ui->accelerationSpin, 0.001, 1000000.0, 100.0);
    initDoubleSpin(ui->decelerationSpin, 0.001, 1000000.0, 100.0);
    initDoubleSpin(ui->lineXSpin, -1000000.0, 1000000.0, 0.0);
    initDoubleSpin(ui->lineYSpin, -1000000.0, 1000000.0, 0.0);
    initDoubleSpin(ui->lineVelocitySpin, 0.001, 1000000.0, 10.0);
    initDoubleSpin(ui->lineAccelerationSpin, 0.001, 1000000.0, 100.0);
    initDoubleSpin(ui->lineDecelerationSpin, 0.001, 1000000.0, 100.0);
    initDoubleSpin(ui->triggerStartSpin, -1000000.0, 1000000.0, 0.0);
    initDoubleSpin(ui->triggerEndSpin, -1000000.0, 1000000.0, 10.0);
    initDoubleSpin(ui->triggerIntervalSpin, 0.001, 1000000.0, 1.0);
    initIntSpin(ui->triggerDirectionSpin, 0, 1, 1);
    initIntSpin(ui->triggerPulseSpin, 1, 1000000, 500);
    initIntSpin(ui->autoReadIntervalSpin, 50, 60000, 200);
    initIntSpin(ui->ctrlAutoSndIntervalSpin, 10, 60000, 100);

    ui->logEdit->setMaximumBlockCount(2000);

    const QList<QWidget *> widgets = {
        ui->axisCombo,
        ui->enableButton,
        ui->disableButton,
        ui->homeButton,
        ui->clearErrorButton,
        ui->stopAxisButton,
        ui->absPositionSpin,
        ui->absMoveButton,
        ui->relDistanceSpin,
        ui->relMoveButton,
        ui->continuousPositiveButton,
        ui->continuousNegativeButton,
        ui->velocitySpin,
        ui->accelerationSpin,
        ui->decelerationSpin,
        ui->setVelocityButton,
        ui->lineXSpin,
        ui->lineYSpin,
        ui->lineVelocitySpin,
        ui->lineAccelerationSpin,
        ui->lineDecelerationSpin,
        ui->lineMoveButton,
        ui->stopLineButton,
        ui->triggerAxisCombo,
        ui->triggerStartSpin,
        ui->triggerEndSpin,
        ui->triggerIntervalSpin,
        ui->triggerDirectionSpin,
        ui->triggerPulseSpin,
        ui->setTriggerButton,
        ui->triggerOnButton,
        ui->triggerOffButton,
        ui->autoReadCheck,
        ui->autoReadIntervalSpin,
        ui->ctrlAutoSndCheck,
        ui->ctrlAutoSndIntervalSpin,
        ui->manualReadButton
    };
    for (QWidget *widget : widgets) {
        registerConnectedWidget(widget);
    }
}

void MainWindow::connectUiSignals()
{
    connect(ui->connectButton, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(ui->disconnectButton, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
    connect(ui->enableButton, &QPushButton::clicked, this, &MainWindow::onEnableClicked);
    connect(ui->disableButton, &QPushButton::clicked, this, &MainWindow::onDisableClicked);
    connect(ui->homeButton, &QPushButton::clicked, this, &MainWindow::onHomeClicked);
    connect(ui->clearErrorButton, &QPushButton::clicked, this, &MainWindow::onClearErrorClicked);
    connect(ui->stopAxisButton, &QPushButton::clicked, this, &MainWindow::onStopAxisClicked);
    connect(ui->absMoveButton, &QPushButton::clicked, this, &MainWindow::onAbsoluteMoveClicked);
    connect(ui->relMoveButton, &QPushButton::clicked, this, &MainWindow::onRelativeMoveClicked);
    connect(ui->continuousPositiveButton, &QPushButton::clicked, this, &MainWindow::onContinuousPositiveClicked);
    connect(ui->continuousNegativeButton, &QPushButton::clicked, this, &MainWindow::onContinuousNegativeClicked);
    connect(ui->setVelocityButton, &QPushButton::clicked, this, &MainWindow::onSetVelocityClicked);
    connect(ui->lineMoveButton, &QPushButton::clicked, this, &MainWindow::onLineMoveClicked);
    connect(ui->stopLineButton, &QPushButton::clicked, this, &MainWindow::onStopLineClicked);
    connect(ui->setTriggerButton, &QPushButton::clicked, this, &MainWindow::onSetTriggerClicked);
    connect(ui->triggerOnButton, &QPushButton::clicked, this, &MainWindow::onTriggerOnClicked);
    connect(ui->triggerOffButton, &QPushButton::clicked, this, &MainWindow::onTriggerOffClicked);
    connect(ui->autoReadCheck, &QCheckBox::toggled, this, &MainWindow::onAutoReadToggled);
    connect(ui->ctrlAutoSndCheck, &QCheckBox::toggled, this, &MainWindow::onCtrlAutoSndToggled);
    connect(ui->manualReadButton, &QPushButton::clicked, this, &MainWindow::onManualReadClicked);
}

void MainWindow::registerConnectedWidget(QWidget *widget)
{
    if (widget != nullptr && !m_connectedWidgets.contains(widget)) {
        m_connectedWidgets.append(widget);
    }
}

void MainWindow::updateUiEnabled(bool connected)
{
    ui->connectButton->setEnabled(!connected);
    ui->disconnectButton->setEnabled(connected);

    for (QWidget *widget : m_connectedWidgets) {
        if (widget != nullptr) {
            widget->setEnabled(connected);
        }
    }
}

int MainWindow::currentAxis() const
{
    return ui->axisCombo->currentData().toInt();
}

int MainWindow::triggerAxis() const
{
    return ui->triggerAxisCombo->currentData().toInt();
}

bool MainWindow::ensurePositive(QDoubleSpinBox *spin, const QString &name) const
{
    if (spin->value() <= 0.0) {
        const_cast<MainWindow *>(this)->appendLog(QString(u8"%1 必须大于 0.").arg(name));
        return false;
    }
    return true;
}
