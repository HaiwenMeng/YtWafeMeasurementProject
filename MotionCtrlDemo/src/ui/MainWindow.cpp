#include "MainWindow.h"

#include <QDateTime>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

namespace
{
QString zh(const char *text)
{
    return QString::fromUtf8(text);
}

QString displayMessage(const QString &message)
{
    if (message.startsWith("TX: ")) {
        return zh("\xe5\x8f\x91\xe9\x80\x81\x3a\x20\x25\x31").arg(message.mid(4));
    }
    if (message.startsWith("RX: ")) {
        return zh("\xe6\x8e\xa5\xe6\x94\xb6\x3a\x20\x25\x31").arg(message.mid(4));
    }
    if (message.startsWith("ERROR: ")) {
        return zh("\xe9\x94\x99\xe8\xaf\xaf\x3a\x20\x25\x31").arg(message.mid(7));
    }
    return message;
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_ipEdit(nullptr),
      m_portSpin(nullptr),
      m_connectButton(nullptr),
      m_disconnectButton(nullptr),
      m_connectionStatusLabel(nullptr),
      m_logEdit(nullptr),
      m_axisCombo(nullptr),
      m_posXLabel(nullptr),
      m_posYLabel(nullptr),
      m_posZLabel(nullptr),
      m_stateXLabel(nullptr),
      m_stateYLabel(nullptr),
      m_stateZLabel(nullptr),
      m_errorXLabel(nullptr),
      m_errorYLabel(nullptr),
      m_errorZLabel(nullptr),
      m_homeLabel(nullptr),
      m_enableButton(nullptr),
      m_disableButton(nullptr),
      m_homeButton(nullptr),
      m_clearErrorButton(nullptr),
      m_stopAxisButton(nullptr),
      m_absPositionSpin(nullptr),
      m_relDistanceSpin(nullptr),
      m_velocitySpin(nullptr),
      m_accelerationSpin(nullptr),
      m_decelerationSpin(nullptr),
      m_lineXSpin(nullptr),
      m_lineYSpin(nullptr),
      m_lineVelocitySpin(nullptr),
      m_lineAccelerationSpin(nullptr),
      m_lineDecelerationSpin(nullptr),
      m_triggerAxisCombo(nullptr),
      m_triggerStartSpin(nullptr),
      m_triggerEndSpin(nullptr),
      m_triggerIntervalSpin(nullptr),
      m_triggerDirectionSpin(nullptr),
      m_triggerPulseSpin(nullptr),
      m_autoReadCheck(nullptr),
      m_autoReadIntervalSpin(nullptr),
      m_ctrlAutoSndCheck(nullptr),
      m_ctrlAutoSndIntervalSpin(nullptr),
      m_manualReadButton(nullptr)
{
    setWindowTitle(zh("\xe8\xbf\x90\xe5\x8a\xa8\xe6\x8e\xa7\xe5\x88\xb6\xe6\xb5\x8b\xe8\xaf\x95"));
    resize(1180, 760);
    setCentralWidget(buildCentralWidget());

    connect(&m_controller, &MotionController::connectionStatusChanged,
            this, &MainWindow::onConnectionChanged);
    connect(&m_controller, &MotionController::logMessage,
            this, &MainWindow::appendLog);
    connect(&m_controller, &MotionController::errorMessage,
            this, &MainWindow::appendLog);
    connect(&m_controller, &MotionController::stateNotReady,
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
}

void MainWindow::onConnectClicked()
{
    const QString host = m_ipEdit->text().trimmed();
    if (host.isEmpty()) {
        appendLog(zh("\xe4\xb8\xbb\xe6\x9c\xba\xe5\x9c\xb0\xe5\x9d\x80\xe4\xb8\x8d\xe8\x83\xbd\xe4\xb8\xba\xe7\xa9\xba\xe3\x80\x82"));
        return;
    }
    m_controller.connectToController(host, static_cast<quint16>(m_portSpin->value()));
}

void MainWindow::onDisconnectClicked()
{
    m_readTimer.stop();
    m_autoReadCheck->blockSignals(true);
    m_autoReadCheck->setChecked(false);
    m_autoReadCheck->blockSignals(false);
    m_ctrlAutoSndCheck->blockSignals(true);
    m_ctrlAutoSndCheck->setChecked(false);
    m_ctrlAutoSndCheck->blockSignals(false);
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
    m_controller.moveSingleAxisAbsolute(currentAxis(), m_absPositionSpin->value());
}

void MainWindow::onRelativeMoveClicked()
{
    m_controller.moveSingleAxisIncremental(currentAxis(), m_relDistanceSpin->value());
}

void MainWindow::onContinuousPositiveClicked()
{
    m_controller.moveSingleAxisContinuous(currentAxis(), 1);
}

void MainWindow::onContinuousNegativeClicked()
{
    m_controller.moveSingleAxisContinuous(currentAxis(), -1);
}

void MainWindow::onSetVelocityClicked()
{
    if (!ensurePositive(m_velocitySpin, zh("\xe9\x80\x9f\xe5\xba\xa6")) ||
        !ensurePositive(m_accelerationSpin, zh("\xe5\x8a\xa0\xe9\x80\x9f\xe5\xba\xa6")) ||
        !ensurePositive(m_decelerationSpin, zh("\xe5\x87\x8f\xe9\x80\x9f\xe5\xba\xa6"))) {
        return;
    }
    m_controller.setVelocity(currentAxis(),
                             m_velocitySpin->value(),
                             m_accelerationSpin->value(),
                             m_decelerationSpin->value());
}

void MainWindow::onLineMoveClicked()
{
    if (!ensurePositive(m_lineVelocitySpin, zh("\x58\x59\x20\xe9\x80\x9f\xe5\xba\xa6")) ||
        !ensurePositive(m_lineAccelerationSpin, zh("\x58\x59\x20\xe5\x8a\xa0\xe9\x80\x9f\xe5\xba\xa6")) ||
        !ensurePositive(m_lineDecelerationSpin, zh("\x58\x59\x20\xe5\x87\x8f\xe9\x80\x9f\xe5\xba\xa6"))) {
        return;
    }
    m_controller.moveMultiAxisLinear(m_lineXSpin->value(),
                                     m_lineYSpin->value(),
                                     m_lineVelocitySpin->value(),
                                     m_lineAccelerationSpin->value(),
                                     m_lineDecelerationSpin->value());
}

void MainWindow::onStopLineClicked()
{
    m_controller.abortAxes();
}

void MainWindow::onSetTriggerClicked()
{
    Motion::TriggerParam param;
    param.axis = triggerAxis();
    param.startPos = m_triggerStartSpin->value();
    param.endPos = m_triggerEndSpin->value();
    param.interval = m_triggerIntervalSpin->value();
    param.direction = m_triggerDirectionSpin->value();
    param.pulseWidth = m_triggerPulseSpin->value();
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
        appendLog(zh("\xe6\x89\x8b\xe5\x8a\xa8\xe8\xbd\xae\xe8\xaf\xa2\xe5\xb7\xb2\xe5\x81\x9c\xe6\xad\xa2\xe3\x80\x82"));
        return;
    }

    if (!m_controller.isConnected()) {
        appendLog(zh("\xe6\x8e\xa7\xe5\x88\xb6\xe5\x99\xa8\xe6\x9c\xaa\xe8\xbf\x9e\xe6\x8e\xa5\xef\xbc\x8c\xe6\x97\xa0\xe6\xb3\x95\xe5\xbc\x80\xe5\x90\xaf\xe6\x89\x8b\xe5\x8a\xa8\xe8\xbd\xae\xe8\xaf\xa2\xe3\x80\x82"));
        m_autoReadCheck->setChecked(false);
        return;
    }

    m_readTimer.start(m_autoReadIntervalSpin->value());
    appendLog(zh("\xe6\x89\x8b\xe5\x8a\xa8\xe8\xbd\xae\xe8\xaf\xa2\xe5\xb7\xb2\xe5\xbc\x80\xe5\x90\xaf\xef\xbc\x8c\xe5\x91\xa8\xe6\x9c\x9f\x3d\x25\x31\x20\x6d\x73\xe3\x80\x82").arg(m_autoReadIntervalSpin->value()));
}

void MainWindow::onCtrlAutoSndToggled(bool enabled)
{
    if (enabled) {
        if (!m_controller.ctrlAutoSnd(m_ctrlAutoSndIntervalSpin->value())) {
            m_ctrlAutoSndCheck->setChecked(false);
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
    m_connectionStatusLabel->setText(connected ? zh("\xe5\xb7\xb2\xe8\xbf\x9e\xe6\x8e\xa5") : zh("\xe5\xb7\xb2\xe6\x96\xad\xe5\xbc\x80"));
    updateUiEnabled(connected);
    if (!connected) {
        m_readTimer.stop();
        m_autoReadCheck->blockSignals(true);
        m_autoReadCheck->setChecked(false);
        m_autoReadCheck->blockSignals(false);
        m_ctrlAutoSndCheck->blockSignals(true);
        m_ctrlAutoSndCheck->setChecked(false);
        m_ctrlAutoSndCheck->blockSignals(false);
    }
}

void MainWindow::appendLog(const QString &message)
{
    const QString time = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    m_logEdit->appendPlainText(QString("[%1] %2").arg(time, displayMessage(message)));
}

void MainWindow::onPositionUpdated(const Motion::AxisPosition &position)
{
    m_posXLabel->setText(QString::number(position.x, 'f', 3));
    m_posYLabel->setText(QString::number(position.y, 'f', 3));
    m_posZLabel->setText(QString::number(position.z, 'f', 3));
}

void MainWindow::onStateUpdated(const Motion::AxisStateSnapshot &state)
{
    m_stateXLabel->setText(QString::number(state.stateX));
    m_stateYLabel->setText(QString::number(state.stateY));
    m_stateZLabel->setText(QString::number(state.stateZ));
    m_errorXLabel->setText(QString::number(state.errorX));
    m_errorYLabel->setText(QString::number(state.errorY));
    m_errorZLabel->setText(QString::number(state.errorZ));
}

void MainWindow::onHomeStatusUpdated(bool homeX, bool homeY, bool homeZ)
{
    m_homeLabel->setText(zh("\xe5\x9b\x9e\xe9\x9b\xb6\x20\x58\x3d\x25\x31\xef\xbc\x8c\x59\x3d\x25\x32\xef\xbc\x8c\x5a\x3d\x25\x33")
                         .arg(homeX ? "1" : "0")
                         .arg(homeY ? "1" : "0")
                         .arg(homeZ ? "1" : "0"));
}

QWidget *MainWindow::buildCentralWidget()
{
    QWidget *page = new QWidget(this);
    QVBoxLayout *root = new QVBoxLayout(page);
    QHBoxLayout *top = new QHBoxLayout;
    QVBoxLayout *left = new QVBoxLayout;
    QVBoxLayout *right = new QVBoxLayout;

    left->addWidget(buildConnectionGroup());
    left->addWidget(buildAxisGroup());
    left->addWidget(buildRefreshGroup());
    left->addStretch();

    right->addWidget(buildSingleAxisGroup());
    right->addWidget(buildLineGroup());
    right->addWidget(buildTriggerGroup());
    right->addStretch();

    top->addLayout(left, 1);
    top->addLayout(right, 2);
    root->addLayout(top, 3);

    m_logEdit = new QPlainTextEdit(page);
    m_logEdit->setReadOnly(true);
    m_logEdit->setMaximumBlockCount(2000);
    QGroupBox *logGroup = new QGroupBox(zh("\xe6\x97\xa5\xe5\xbf\x97"), page);
    QVBoxLayout *logLayout = new QVBoxLayout(logGroup);
    logLayout->addWidget(m_logEdit);
    root->addWidget(logGroup, 2);

    return page;
}

QGroupBox *MainWindow::buildConnectionGroup()
{
    QGroupBox *group = new QGroupBox(zh("\xe8\xbf\x9e\xe6\x8e\xa5\xe5\x8c\xba"), this);
    QGridLayout *layout = new QGridLayout(group);

    m_ipEdit = new QLineEdit("90.0.0.1", group);
    m_portSpin = createIntSpin(1, 65535, 5000);
    m_connectButton = new QPushButton(zh("\xe8\xbf\x9e\xe6\x8e\xa5"), group);
    m_disconnectButton = new QPushButton(zh("\xe6\x96\xad\xe5\xbc\x80"), group);
    m_connectionStatusLabel = new QLabel(zh("\xe5\xb7\xb2\xe6\x96\xad\xe5\xbc\x80"), group);

    layout->addWidget(new QLabel(zh("\x49\x50\xe5\x9c\xb0\xe5\x9d\x80"), group), 0, 0);
    layout->addWidget(m_ipEdit, 0, 1);
    layout->addWidget(new QLabel(zh("\xe7\xab\xaf\xe5\x8f\xa3"), group), 1, 0);
    layout->addWidget(m_portSpin, 1, 1);
    layout->addWidget(m_connectButton, 2, 0);
    layout->addWidget(m_disconnectButton, 2, 1);
    layout->addWidget(new QLabel(zh("\xe7\x8a\xb6\xe6\x80\x81"), group), 3, 0);
    layout->addWidget(m_connectionStatusLabel, 3, 1);

    connect(m_connectButton, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(m_disconnectButton, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
    return group;
}

QGroupBox *MainWindow::buildAxisGroup()
{
    QGroupBox *group = new QGroupBox(zh("\xe8\xbd\xb4\xe7\x9b\x91\xe6\x8e\xa7"), this);
    QGridLayout *layout = new QGridLayout(group);

    m_axisCombo = new QComboBox(group);
    m_axisCombo->addItem("X", Motion::AxisX);
    m_axisCombo->addItem("Y", Motion::AxisY);
    m_axisCombo->addItem("Z", Motion::AxisZ);
    registerConnectedWidget(m_axisCombo);

    m_posXLabel = new QLabel("0.000", group);
    m_posYLabel = new QLabel("0.000", group);
    m_posZLabel = new QLabel("0.000", group);
    m_stateXLabel = new QLabel(zh("\xe6\x9c\xaa\xe7\x9f\xa5"), group);
    m_stateYLabel = new QLabel(zh("\xe6\x9c\xaa\xe7\x9f\xa5"), group);
    m_stateZLabel = new QLabel(zh("\xe6\x9c\xaa\xe7\x9f\xa5"), group);
    m_errorXLabel = new QLabel(zh("\xe6\x9c\xaa\xe7\x9f\xa5"), group);
    m_errorYLabel = new QLabel(zh("\xe6\x9c\xaa\xe7\x9f\xa5"), group);
    m_errorZLabel = new QLabel(zh("\xe6\x9c\xaa\xe7\x9f\xa5"), group);
    m_homeLabel = new QLabel(zh("\xe5\x9b\x9e\xe9\x9b\xb6\x20\x58\x3d\x3f\xef\xbc\x8c\x59\x3d\x3f\xef\xbc\x8c\x5a\x3d\x3f"), group);

    layout->addWidget(new QLabel(zh("\xe5\xbd\x93\xe5\x89\x8d\xe8\xbd\xb4"), group), 0, 0);
    layout->addWidget(m_axisCombo, 0, 1, 1, 3);
    layout->addWidget(new QLabel(zh("\xe8\xbd\xb4"), group), 1, 0);
    layout->addWidget(new QLabel("X", group), 1, 1);
    layout->addWidget(new QLabel("Y", group), 1, 2);
    layout->addWidget(new QLabel("Z", group), 1, 3);
    layout->addWidget(new QLabel(zh("\xe4\xbd\x8d\xe7\xbd\xae"), group), 2, 0);
    layout->addWidget(m_posXLabel, 2, 1);
    layout->addWidget(m_posYLabel, 2, 2);
    layout->addWidget(m_posZLabel, 2, 3);
    layout->addWidget(new QLabel(zh("\xe8\xbd\xb4\xe7\x8a\xb6\xe6\x80\x81"), group), 3, 0);
    layout->addWidget(m_stateXLabel, 3, 1);
    layout->addWidget(m_stateYLabel, 3, 2);
    layout->addWidget(m_stateZLabel, 3, 3);
    layout->addWidget(new QLabel(zh("\xe9\x94\x99\xe8\xaf\xaf\xe7\xa0\x81"), group), 4, 0);
    layout->addWidget(m_errorXLabel, 4, 1);
    layout->addWidget(m_errorYLabel, 4, 2);
    layout->addWidget(m_errorZLabel, 4, 3);
    layout->addWidget(m_homeLabel, 5, 0, 1, 4);
    return group;
}

QGroupBox *MainWindow::buildSingleAxisGroup()
{
    QGroupBox *group = new QGroupBox(zh("\xe5\x8d\x95\xe8\xbd\xb4\xe6\x8e\xa7\xe5\x88\xb6"), this);
    QGridLayout *layout = new QGridLayout(group);

    m_enableButton = new QPushButton(zh("\xe4\xbd\xbf\xe8\x83\xbd"), group);
    m_disableButton = new QPushButton(zh("\xe5\xa4\xb1\xe8\x83\xbd"), group);
    m_homeButton = new QPushButton(zh("\xe5\x9b\x9e\xe9\x9b\xb6"), group);
    m_clearErrorButton = new QPushButton(zh("\xe6\xb8\x85\xe9\x94\x99"), group);
    m_stopAxisButton = new QPushButton(zh("\xe5\x81\x9c\xe6\xad\xa2\xe5\xbd\x93\xe5\x89\x8d\xe8\xbd\xb4"), group);

    QPushButton *absButton = new QPushButton(zh("\xe7\xbb\x9d\xe5\xaf\xb9\xe8\xbf\x90\xe5\x8a\xa8"), group);
    QPushButton *relButton = new QPushButton(zh("\xe7\x9b\xb8\xe5\xaf\xb9\xe8\xbf\x90\xe5\x8a\xa8"), group);
    QPushButton *positiveButton = new QPushButton(zh("\xe6\xad\xa3\xe5\x90\x91\xe8\xbf\x9e\xe7\xbb\xad"), group);
    QPushButton *negativeButton = new QPushButton(zh("\xe5\x8f\x8d\xe5\x90\x91\xe8\xbf\x9e\xe7\xbb\xad"), group);
    QPushButton *setVelocityButton = new QPushButton(zh("\xe8\xae\xbe\xe7\xbd\xae\xe9\x80\x9f\xe5\xba\xa6\xe5\x8f\x82\xe6\x95\xb0"), group);

    m_absPositionSpin = createDoubleSpin(-1000000.0, 1000000.0, 0.0);
    m_relDistanceSpin = createDoubleSpin(-1000000.0, 1000000.0, 0.0);
    m_velocitySpin = createDoubleSpin(0.001, 1000000.0, 10.0);
    m_accelerationSpin = createDoubleSpin(0.001, 1000000.0, 100.0);
    m_decelerationSpin = createDoubleSpin(0.001, 1000000.0, 100.0);
    const QList<QWidget *> controls = {
        m_enableButton, m_disableButton, m_homeButton, m_clearErrorButton, m_stopAxisButton,
        absButton, relButton, positiveButton, negativeButton, setVelocityButton,
        m_absPositionSpin, m_relDistanceSpin, m_velocitySpin, m_accelerationSpin, m_decelerationSpin
    };
    for (QWidget *widget : controls) {
        registerConnectedWidget(widget);
    }

    layout->addWidget(m_enableButton, 0, 0);
    layout->addWidget(m_disableButton, 0, 1);
    layout->addWidget(m_homeButton, 0, 2);
    layout->addWidget(m_clearErrorButton, 0, 3);
    layout->addWidget(m_stopAxisButton, 0, 4);
    layout->addWidget(new QLabel(zh("\xe7\xbb\x9d\xe5\xaf\xb9\xe4\xbd\x8d\xe7\xbd\xae"), group), 1, 0);
    layout->addWidget(m_absPositionSpin, 1, 1, 1, 2);
    layout->addWidget(absButton, 1, 3, 1, 2);
    layout->addWidget(new QLabel(zh("\xe7\x9b\xb8\xe5\xaf\xb9\xe8\xb7\x9d\xe7\xa6\xbb"), group), 2, 0);
    layout->addWidget(m_relDistanceSpin, 2, 1, 1, 2);
    layout->addWidget(relButton, 2, 3, 1, 2);
    layout->addWidget(positiveButton, 3, 0);
    layout->addWidget(negativeButton, 3, 1);
    layout->addWidget(new QLabel(zh("\xe5\x81\x9c\xe6\xad\xa2\xe8\xaf\xb7\xe4\xbd\xbf\xe7\x94\xa8\xe2\x80\x9c\xe5\x81\x9c\xe6\xad\xa2\xe5\xbd\x93\xe5\x89\x8d\xe8\xbd\xb4\xe2\x80\x9d\xe6\x8c\x89\xe9\x92\xae"), group), 3, 2, 1, 3);
    layout->addWidget(new QLabel(zh("\xe9\x80\x9f\xe5\xba\xa6"), group), 4, 0);
    layout->addWidget(m_velocitySpin, 4, 1);
    layout->addWidget(new QLabel(zh("\xe5\x8a\xa0\xe9\x80\x9f\xe5\xba\xa6"), group), 4, 2);
    layout->addWidget(m_accelerationSpin, 4, 3);
    layout->addWidget(new QLabel(zh("\xe5\x87\x8f\xe9\x80\x9f\xe5\xba\xa6"), group), 4, 4);
    layout->addWidget(m_decelerationSpin, 4, 5);
    layout->addWidget(setVelocityButton, 5, 0, 1, 6);

    connect(m_enableButton, &QPushButton::clicked, this, &MainWindow::onEnableClicked);
    connect(m_disableButton, &QPushButton::clicked, this, &MainWindow::onDisableClicked);
    connect(m_homeButton, &QPushButton::clicked, this, &MainWindow::onHomeClicked);
    connect(m_clearErrorButton, &QPushButton::clicked, this, &MainWindow::onClearErrorClicked);
    connect(m_stopAxisButton, &QPushButton::clicked, this, &MainWindow::onStopAxisClicked);
    connect(absButton, &QPushButton::clicked, this, &MainWindow::onAbsoluteMoveClicked);
    connect(relButton, &QPushButton::clicked, this, &MainWindow::onRelativeMoveClicked);
    connect(positiveButton, &QPushButton::clicked, this, &MainWindow::onContinuousPositiveClicked);
    connect(negativeButton, &QPushButton::clicked, this, &MainWindow::onContinuousNegativeClicked);
    connect(setVelocityButton, &QPushButton::clicked, this, &MainWindow::onSetVelocityClicked);
    return group;
}

QGroupBox *MainWindow::buildLineGroup()
{
    QGroupBox *group = new QGroupBox(zh("\x58\x59\x20\xe6\x8f\x92\xe8\xa1\xa5"), this);
    QGridLayout *layout = new QGridLayout(group);

    m_lineXSpin = createDoubleSpin(-1000000.0, 1000000.0, 0.0);
    m_lineYSpin = createDoubleSpin(-1000000.0, 1000000.0, 0.0);
    m_lineVelocitySpin = createDoubleSpin(0.001, 1000000.0, 10.0);
    m_lineAccelerationSpin = createDoubleSpin(0.001, 1000000.0, 100.0);
    m_lineDecelerationSpin = createDoubleSpin(0.001, 1000000.0, 100.0);
    QPushButton *moveButton = new QPushButton(zh("\xe6\x89\xa7\xe8\xa1\x8c\x20\x58\x59\x20\xe6\x8f\x92\xe8\xa1\xa5"), group);
    QPushButton *stopButton = new QPushButton(zh("\xe5\x81\x9c\xe6\xad\xa2\x20\x58\x59\x20\xe6\x8f\x92\xe8\xa1\xa5"), group);
    const QList<QWidget *> controls = {
        m_lineXSpin, m_lineYSpin, m_lineVelocitySpin, m_lineAccelerationSpin, m_lineDecelerationSpin,
        moveButton, stopButton
    };
    for (QWidget *widget : controls) {
        registerConnectedWidget(widget);
    }

    layout->addWidget(new QLabel(zh("\x58\x20\xe7\x9b\xae\xe6\xa0\x87"), group), 0, 0);
    layout->addWidget(m_lineXSpin, 0, 1);
    layout->addWidget(new QLabel(zh("\x59\x20\xe7\x9b\xae\xe6\xa0\x87"), group), 0, 2);
    layout->addWidget(m_lineYSpin, 0, 3);
    layout->addWidget(new QLabel(zh("\xe9\x80\x9f\xe5\xba\xa6"), group), 1, 0);
    layout->addWidget(m_lineVelocitySpin, 1, 1);
    layout->addWidget(new QLabel(zh("\xe5\x8a\xa0\xe9\x80\x9f\xe5\xba\xa6"), group), 1, 2);
    layout->addWidget(m_lineAccelerationSpin, 1, 3);
    layout->addWidget(new QLabel(zh("\xe5\x87\x8f\xe9\x80\x9f\xe5\xba\xa6"), group), 1, 4);
    layout->addWidget(m_lineDecelerationSpin, 1, 5);
    layout->addWidget(moveButton, 2, 0, 1, 3);
    layout->addWidget(stopButton, 2, 3, 1, 3);

    connect(moveButton, &QPushButton::clicked, this, &MainWindow::onLineMoveClicked);
    connect(stopButton, &QPushButton::clicked, this, &MainWindow::onStopLineClicked);
    return group;
}

QGroupBox *MainWindow::buildTriggerGroup()
{
    QGroupBox *group = new QGroupBox(zh("\xe8\xa7\xa6\xe5\x8f\x91\xe6\xb5\x8b\xe8\xaf\x95"), this);
    QGridLayout *layout = new QGridLayout(group);

    m_triggerAxisCombo = new QComboBox(group);
    m_triggerAxisCombo->addItem("X", Motion::AxisX);
    m_triggerAxisCombo->addItem("Y", Motion::AxisY);
    m_triggerAxisCombo->addItem("Z", Motion::AxisZ);
    m_triggerStartSpin = createDoubleSpin(-1000000.0, 1000000.0, 0.0);
    m_triggerEndSpin = createDoubleSpin(-1000000.0, 1000000.0, 10.0);
    m_triggerIntervalSpin = createDoubleSpin(0.001, 1000000.0, 1.0);
    m_triggerDirectionSpin = createIntSpin(-1, 1, 1);
    m_triggerDirectionSpin->setSingleStep(2);
    m_triggerPulseSpin = createIntSpin(1, 1000000, 500);

    QPushButton *setButton = new QPushButton(zh("\xe8\xae\xbe\xe7\xbd\xae\xe8\xa7\xa6\xe5\x8f\x91\xe5\x8f\x82\xe6\x95\xb0"), group);
    QPushButton *onButton = new QPushButton(zh("\xe5\xbc\x80\xe5\x90\xaf\xe8\xa7\xa6\xe5\x8f\x91"), group);
    QPushButton *offButton = new QPushButton(zh("\xe5\x85\xb3\xe9\x97\xad\xe8\xa7\xa6\xe5\x8f\x91"), group);
    const QList<QWidget *> controls = {
        m_triggerAxisCombo, m_triggerStartSpin, m_triggerEndSpin, m_triggerIntervalSpin,
        m_triggerDirectionSpin, m_triggerPulseSpin, setButton, onButton, offButton
    };
    for (QWidget *widget : controls) {
        registerConnectedWidget(widget);
    }

    layout->addWidget(new QLabel(zh("\xe8\xa7\xa6\xe5\x8f\x91\xe8\xbd\xb4"), group), 0, 0);
    layout->addWidget(m_triggerAxisCombo, 0, 1);
    layout->addWidget(new QLabel(zh("\xe8\xb5\xb7\xe5\xa7\x8b\xe4\xbd\x8d\xe7\xbd\xae"), group), 0, 2);
    layout->addWidget(m_triggerStartSpin, 0, 3);
    layout->addWidget(new QLabel(zh("\xe7\xbb\x93\xe6\x9d\x9f\xe4\xbd\x8d\xe7\xbd\xae"), group), 1, 0);
    layout->addWidget(m_triggerEndSpin, 1, 1);
    layout->addWidget(new QLabel(zh("\xe9\x97\xb4\xe8\xb7\x9d"), group), 1, 2);
    layout->addWidget(m_triggerIntervalSpin, 1, 3);
    layout->addWidget(new QLabel(zh("\xe6\x96\xb9\xe5\x90\x91"), group), 2, 0);
    layout->addWidget(m_triggerDirectionSpin, 2, 1);
    layout->addWidget(new QLabel(zh("\xe8\x84\x89\xe5\xae\xbd"), group), 2, 2);
    layout->addWidget(m_triggerPulseSpin, 2, 3);
    layout->addWidget(setButton, 3, 0, 1, 2);
    layout->addWidget(onButton, 3, 2);
    layout->addWidget(offButton, 3, 3);

    connect(setButton, &QPushButton::clicked, this, &MainWindow::onSetTriggerClicked);
    connect(onButton, &QPushButton::clicked, this, &MainWindow::onTriggerOnClicked);
    connect(offButton, &QPushButton::clicked, this, &MainWindow::onTriggerOffClicked);
    return group;
}

QGroupBox *MainWindow::buildRefreshGroup()
{
    QGroupBox *group = new QGroupBox(zh("\xe5\xae\x9e\xe6\x97\xb6\xe5\x88\xb7\xe6\x96\xb0"), this);
    QGridLayout *layout = new QGridLayout(group);

    m_autoReadCheck = new QCheckBox(zh("\xe6\x89\x8b\xe5\x8a\xa8\xe8\xbd\xae\xe8\xaf\xa2"), group);
    m_autoReadIntervalSpin = createIntSpin(50, 60000, 200);
    m_ctrlAutoSndCheck = new QCheckBox(zh("\xe6\x8e\xa7\xe5\x88\xb6\xe5\x99\xa8\xe8\x87\xaa\xe5\x8a\xa8\xe4\xb8\x8a\xe6\x8a\xa5"), group);
    m_ctrlAutoSndIntervalSpin = createIntSpin(10, 60000, 100);
    m_manualReadButton = new QPushButton(zh("\xe6\x89\x8b\xe5\x8a\xa8\xe8\xaf\xbb\xe5\x8f\x96"), group);
    const QList<QWidget *> controls = {
        m_autoReadCheck, m_autoReadIntervalSpin, m_ctrlAutoSndCheck,
        m_ctrlAutoSndIntervalSpin, m_manualReadButton
    };
    for (QWidget *widget : controls) {
        registerConnectedWidget(widget);
    }

    layout->addWidget(m_autoReadCheck, 0, 0);
    layout->addWidget(new QLabel(zh("\xe5\x91\xa8\xe6\x9c\x9f\x20\x6d\x73"), group), 0, 1);
    layout->addWidget(m_autoReadIntervalSpin, 0, 2);
    layout->addWidget(m_ctrlAutoSndCheck, 1, 0);
    layout->addWidget(new QLabel(zh("\xe5\x91\xa8\xe6\x9c\x9f\x20\x6d\x73"), group), 1, 1);
    layout->addWidget(m_ctrlAutoSndIntervalSpin, 1, 2);
    layout->addWidget(m_manualReadButton, 2, 0, 1, 3);

    connect(m_autoReadCheck, &QCheckBox::toggled, this, &MainWindow::onAutoReadToggled);
    connect(m_ctrlAutoSndCheck, &QCheckBox::toggled, this, &MainWindow::onCtrlAutoSndToggled);
    connect(m_manualReadButton, &QPushButton::clicked, this, &MainWindow::onManualReadClicked);
    return group;
}

QDoubleSpinBox *MainWindow::createDoubleSpin(double minValue, double maxValue, double value, int decimals) const
{
    QDoubleSpinBox *spin = new QDoubleSpinBox;
    spin->setRange(minValue, maxValue);
    spin->setDecimals(decimals);
    spin->setValue(value);
    spin->setSingleStep(1.0);
    return spin;
}

QSpinBox *MainWindow::createIntSpin(int minValue, int maxValue, int value) const
{
    QSpinBox *spin = new QSpinBox;
    spin->setRange(minValue, maxValue);
    spin->setValue(value);
    return spin;
}

int MainWindow::currentAxis() const
{
    return m_axisCombo->currentData().toInt();
}

int MainWindow::triggerAxis() const
{
    return m_triggerAxisCombo->currentData().toInt();
}

bool MainWindow::ensurePositive(QDoubleSpinBox *spin, const QString &name) const
{
    if (spin->value() <= 0.0) {
        const_cast<MainWindow *>(this)->appendLog(zh("\x25\x31\x20\xe5\xbf\x85\xe9\xa1\xbb\xe5\xa4\xa7\xe4\xba\x8e\x20\x30\xe3\x80\x82").arg(name));
        return false;
    }
    return true;
}

void MainWindow::registerConnectedWidget(QWidget *widget)
{
    if (widget != nullptr && !m_connectedWidgets.contains(widget)) {
        m_connectedWidgets.append(widget);
    }
}

void MainWindow::updateUiEnabled(bool connected)
{
    m_connectButton->setEnabled(!connected);
    m_disconnectButton->setEnabled(connected);

    for (QWidget *widget : m_connectedWidgets) {
        if (widget != nullptr) {
            widget->setEnabled(connected);
        }
    }
}
