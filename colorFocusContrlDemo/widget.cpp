#include "widget.h"
#include "ui_widget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMetaObject>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QThread>
#include <QVBoxLayout>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <climits>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget),
      m_control(new ColorFocusControl),
      m_controlThread(new QThread(this)),
      m_ipEdit(nullptr),
      m_sensorIdSpin(nullptr),
      m_connectButton(nullptr),
      m_startButton(nullptr),
      m_applyButton(nullptr),
      m_darkButton(nullptr),
      m_clearStackButton(nullptr),
      m_recenterButton(nullptr),
      m_connectionLabel(nullptr),
      m_acquisitionLabel(nullptr),
      m_distanceLabel(nullptr),
      m_intensityLabel(nullptr),
      m_encoderLabel(nullptr),
      m_measureModeCombo(nullptr),
      m_scanRateCombo(nullptr),
      m_triggerModeCombo(nullptr),
      m_triggerEdgeCombo(nullptr),
      m_burstNumSpin(nullptr),
      m_sensitivitySpin(nullptr),
      m_autoSensitivityCheck(nullptr),
      m_smoothingSpin(nullptr),
      m_encoderHeadSpin(nullptr),
      m_encoderTailSpin(nullptr),
      m_encoderIntervalSpin(nullptr),
      m_encoderSelectCombo(nullptr),
      m_encoderDirectionCombo(nullptr),
      m_logEdit(nullptr),
      m_chart(nullptr),
      m_chartView(nullptr),
      m_distanceSeries(nullptr),
      m_axisX(nullptr),
      m_axisY(nullptr),
      m_connected(false),
      m_acquiring(false),
      m_sampleIndex(0.0)
{
    ui->setupUi(this);
    BuildUi();
    ConnectSignals();

    m_control->moveToThread(m_controlThread);
    connect(m_controlThread, &QThread::finished, m_control, &QObject::deleteLater);
    m_controlThread->start();
    SetUiConnected(false);
    SetUiAcquiring(false);
    AppendLog("Demo initialized");
}

Widget::~Widget()
{
    if (m_controlThread && m_controlThread->isRunning()) {
        QMetaObject::invokeMethod(m_control, &ColorFocusControl::DisconnectDevice, Qt::BlockingQueuedConnection);
        m_controlThread->quit();
        m_controlThread->wait(3000);
    }
    delete ui;
}

void Widget::BuildUi()
{
    setWindowTitle("Color Focus Control Demo");
    resize(1100, 760);

    m_ipEdit = new QLineEdit("192.168.10.11", this);
    m_sensorIdSpin = new QSpinBox(this);
    m_sensorIdSpin->setRange(0, 15);
    m_sensorIdSpin->setValue(0);

    m_connectButton = new QPushButton("Connect", this);
    m_startButton = new QPushButton("Start", this);
    m_applyButton = new QPushButton("Apply Parameters", this);
    m_darkButton = new QPushButton("Dark", this);
    m_clearStackButton = new QPushButton("Clear Stack", this);
    m_recenterButton = new QPushButton("Recenter Encoders", this);

    m_connectionLabel = new QLabel("Disconnected", this);
    m_acquisitionLabel = new QLabel("Stopped", this);
    m_distanceLabel = new QLabel("--", this);
    m_intensityLabel = new QLabel("--", this);
    m_encoderLabel = new QLabel("--", this);

    QGroupBox *connectionGroup = new QGroupBox("Connection", this);
    QGridLayout *connectionLayout = new QGridLayout(connectionGroup);
    connectionLayout->addWidget(new QLabel("IP", this), 0, 0);
    connectionLayout->addWidget(m_ipEdit, 0, 1);
    connectionLayout->addWidget(new QLabel("Sensor ID", this), 0, 2);
    connectionLayout->addWidget(m_sensorIdSpin, 0, 3);
    connectionLayout->addWidget(m_connectButton, 0, 4);
    connectionLayout->addWidget(m_startButton, 0, 5);
    connectionLayout->addWidget(new QLabel("Connection", this), 1, 0);
    connectionLayout->addWidget(m_connectionLabel, 1, 1);
    connectionLayout->addWidget(new QLabel("Acquisition", this), 1, 2);
    connectionLayout->addWidget(m_acquisitionLabel, 1, 3);

    m_measureModeCombo = new QComboBox(this);
    m_measureModeCombo->addItem("Distance", CCS_DISTANCE_MODE);
    m_measureModeCombo->addItem("Thickness", CCS_THICKNESS_MODE);
    m_measureModeCombo->addItem("Multi layer", CCS_MULTI_LAYER_MODE);

    m_scanRateCombo = new QComboBox(this);
    const QStringList rates = QStringList()
            << "200 Hz" << "500 Hz" << "1000 Hz" << "2000 Hz" << "3000 Hz"
            << "4000 Hz" << "4500 Hz" << "5000 Hz" << "6000 Hz" << "7000 Hz"
            << "8000 Hz" << "9000 Hz" << "10000 Hz";
    for (int i = 0; i < rates.count(); ++i) {
        m_scanRateCombo->addItem(rates.at(i), i);
    }
    m_scanRateCombo->setCurrentIndex(2);

    m_triggerModeCombo = new QComboBox(this);
    m_triggerModeCombo->addItem("Continue", TriggerContinue);
    m_triggerModeCombo->addItem("Start by edge", TriggerStartByEdge);
    m_triggerModeCombo->addItem("Start stop by state", TriggerStartStopByState);
    m_triggerModeCombo->addItem("Start stop by edge", TriggerStartStopByEdge);
    m_triggerModeCombo->addItem("Burst", TriggerBurst);
    m_triggerModeCombo->addItem("Encoder", TriggerEncoder);

    m_triggerEdgeCombo = new QComboBox(this);
    m_triggerEdgeCombo->addItem("Falling or low", 0);
    m_triggerEdgeCombo->addItem("Rising or high", 1);

    m_burstNumSpin = new QSpinBox(this);
    m_burstNumSpin->setRange(1, 1000000);
    m_burstNumSpin->setValue(3000);
    m_sensitivitySpin = new QSpinBox(this);
    m_sensitivitySpin->setRange(1, 100);
    m_sensitivitySpin->setValue(50);
    m_autoSensitivityCheck = new QCheckBox("Auto sensitivity", this);
    m_smoothingSpin = new QSpinBox(this);
    m_smoothingSpin->setRange(0, 2048);
    m_smoothingSpin->setValue(0);

    QGroupBox *parameterGroup = new QGroupBox("Parameters", this);
    QGridLayout *parameterLayout = new QGridLayout(parameterGroup);
    parameterLayout->addWidget(new QLabel("Measure mode", this), 0, 0);
    parameterLayout->addWidget(m_measureModeCombo, 0, 1);
    parameterLayout->addWidget(new QLabel("Scan rate", this), 0, 2);
    parameterLayout->addWidget(m_scanRateCombo, 0, 3);
    parameterLayout->addWidget(new QLabel("Trigger mode", this), 1, 0);
    parameterLayout->addWidget(m_triggerModeCombo, 1, 1);
    parameterLayout->addWidget(new QLabel("Trigger edge", this), 1, 2);
    parameterLayout->addWidget(m_triggerEdgeCombo, 1, 3);
    parameterLayout->addWidget(new QLabel("Burst num", this), 2, 0);
    parameterLayout->addWidget(m_burstNumSpin, 2, 1);
    parameterLayout->addWidget(new QLabel("Sensitivity", this), 2, 2);
    parameterLayout->addWidget(m_sensitivitySpin, 2, 3);
    parameterLayout->addWidget(m_autoSensitivityCheck, 3, 0, 1, 2);
    parameterLayout->addWidget(new QLabel("Smoothing", this), 3, 2);
    parameterLayout->addWidget(m_smoothingSpin, 3, 3);
    parameterLayout->addWidget(m_applyButton, 4, 0, 1, 4);

    m_encoderHeadSpin = new QSpinBox(this);
    m_encoderHeadSpin->setRange(INT_MIN + 1, INT_MAX);
    m_encoderHeadSpin->setValue(INT_MIN + 1);
    m_encoderTailSpin = new QSpinBox(this);
    m_encoderTailSpin->setRange(INT_MIN + 1, INT_MAX);
    m_encoderTailSpin->setValue(INT_MAX);
    m_encoderIntervalSpin = new QSpinBox(this);
    m_encoderIntervalSpin->setRange(1, 1000000);
    m_encoderIntervalSpin->setValue(20);
    m_encoderSelectCombo = new QComboBox(this);
    m_encoderSelectCombo->addItem("Encoder 1", 0);
    m_encoderSelectCombo->addItem("Encoder 2", 1);
    m_encoderSelectCombo->addItem("Encoder 3", 2);
    m_encoderDirectionCombo = new QComboBox(this);
    m_encoderDirectionCombo->addItem("Forward", 1);
    m_encoderDirectionCombo->addItem("Backward", 0);

    QGroupBox *encoderGroup = new QGroupBox("Encoder Trigger", this);
    QGridLayout *encoderLayout = new QGridLayout(encoderGroup);
    encoderLayout->addWidget(new QLabel("Head", this), 0, 0);
    encoderLayout->addWidget(m_encoderHeadSpin, 0, 1);
    encoderLayout->addWidget(new QLabel("Tail", this), 0, 2);
    encoderLayout->addWidget(m_encoderTailSpin, 0, 3);
    encoderLayout->addWidget(new QLabel("Interval", this), 1, 0);
    encoderLayout->addWidget(m_encoderIntervalSpin, 1, 1);
    encoderLayout->addWidget(new QLabel("Source", this), 1, 2);
    encoderLayout->addWidget(m_encoderSelectCombo, 1, 3);
    encoderLayout->addWidget(new QLabel("Direction", this), 2, 0);
    encoderLayout->addWidget(m_encoderDirectionCombo, 2, 1);
    encoderLayout->addWidget(m_recenterButton, 2, 2, 1, 2);

    QGroupBox *sampleGroup = new QGroupBox("Realtime Values", this);
    QFormLayout *sampleLayout = new QFormLayout(sampleGroup);
    sampleLayout->addRow("Distance", m_distanceLabel);
    sampleLayout->addRow("Intensity", m_intensityLabel);
    sampleLayout->addRow("Encoders", m_encoderLabel);
    sampleLayout->addRow(m_darkButton, m_clearStackButton);

    m_distanceSeries = new QLineSeries(this);
    m_distanceSeries->setName("Distance");
    m_chart = new QChart();
    m_chart->addSeries(m_distanceSeries);
    m_chart->legend()->hide();
    m_chart->setTitle("Distance Realtime Chart");
    m_axisX = new QValueAxis(this);
    m_axisX->setTitleText("Sample");
    m_axisX->setRange(0, 120);
    m_axisY = new QValueAxis(this);
    m_axisY->setTitleText("Distance");
    m_axisY->setRange(-5, 5);
    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_chart->addAxis(m_axisY, Qt::AlignLeft);
    m_distanceSeries->attachAxis(m_axisX);
    m_distanceSeries->attachAxis(m_axisY);
    m_chartView = new QChartView(m_chart, this);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setMinimumHeight(360);

    m_logEdit = new QPlainTextEdit(this);
    m_logEdit->setReadOnly(true);
    m_logEdit->setMaximumBlockCount(500);

    QVBoxLayout *leftLayout = new QVBoxLayout();
    leftLayout->addWidget(connectionGroup);
    leftLayout->addWidget(parameterGroup);
    leftLayout->addWidget(encoderGroup);
    leftLayout->addWidget(sampleGroup);
    leftLayout->addStretch();

    QVBoxLayout *rightLayout = new QVBoxLayout();
    rightLayout->addWidget(m_chartView, 3);
    rightLayout->addWidget(m_logEdit, 1);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->addLayout(leftLayout, 1);
    mainLayout->addLayout(rightLayout, 2);
    setLayout(mainLayout);
}

void Widget::ConnectSignals()
{
    connect(m_connectButton, &QPushButton::clicked, this, &Widget::OnConnectClicked);
    connect(m_startButton, &QPushButton::clicked, this, &Widget::OnStartClicked);
    connect(m_applyButton, &QPushButton::clicked, this, &Widget::OnApplyParametersClicked);
    connect(m_darkButton, &QPushButton::clicked, this, &Widget::OnDarkClicked);
    connect(m_clearStackButton, &QPushButton::clicked, this, &Widget::OnClearStackClicked);
    connect(m_recenterButton, &QPushButton::clicked, this, &Widget::OnRecenterClicked);

    connect(m_control, &ColorFocusControl::connectionStateChanged, this, &Widget::OnConnectionStateChanged);
    connect(m_control, &ColorFocusControl::acquisitionStateChanged, this, &Widget::OnAcquisitionStateChanged);
    connect(m_control, &ColorFocusControl::sampleUpdated, this, &Widget::OnSampleUpdated);
    connect(m_control, &ColorFocusControl::errorOccurred, this, [this](int, const QString &message) {
        AppendLog(QString("ERROR: %1").arg(message));
    });
    connect(m_control, &ColorFocusControl::logMessage, this, &Widget::AppendLog);
    connect(m_control, &ColorFocusControl::bufferedDataUpdated, this, [this](int sensorId, int count) {
        AppendLog(QString("Buffered data updated. sensor=%1 count=%2").arg(sensorId).arg(count));
    });
}

void Widget::SetUiConnected(bool connected)
{
    m_connected = connected;
    m_connectionLabel->setText(connected ? "Connected" : "Disconnected");
    m_connectButton->setText(connected ? "Disconnect" : "Connect");
    m_startButton->setEnabled(connected);
    m_applyButton->setEnabled(connected);
    m_darkButton->setEnabled(connected);
    m_clearStackButton->setEnabled(connected);
    m_recenterButton->setEnabled(connected);
    m_ipEdit->setEnabled(!connected);
    m_sensorIdSpin->setEnabled(!connected);
    if (!connected) {
        SetUiAcquiring(false);
    }
}

void Widget::SetUiAcquiring(bool acquiring)
{
    m_acquiring = acquiring;
    m_acquisitionLabel->setText(acquiring ? "Running" : "Stopped");
    m_startButton->setText(acquiring ? "Stop" : "Start");
}

void Widget::AppendLog(const QString &message)
{
    const QString time = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    m_logEdit->appendPlainText(QString("[%1] %2").arg(time).arg(message));
}

void Widget::AppendSample(float distance, float intensity, int encoder1, int encoder2, int encoder3)
{
    distance = distance >0 ? distance:0;
    m_distanceLabel->setText(QString::number(distance, 'f', 4));
    m_intensityLabel->setText(QString::number(intensity, 'f', 4));
    m_encoderLabel->setText(QString("E1=%1 E2=%2 E3=%3").arg(encoder1).arg(encoder2).arg(encoder3));

    m_chartPoints.append(QPointF(m_sampleIndex, distance));
    m_sampleIndex += 1.0;
    while (m_chartPoints.count() > 120) {
        m_chartPoints.removeFirst();
    }
    m_distanceSeries->replace(m_chartPoints);
    if (!m_chartPoints.isEmpty()) {
        m_axisX->setRange(qMax(0.0, m_sampleIndex - 120.0), qMax(120.0, m_sampleIndex));
    }
    ApplyAxisRange(distance);
}

void Widget::ApplyAxisRange(float distance)
{
    double minY = distance - 5.0;
    double maxY = distance + 5.0;
    for (const QPointF &point : qAsConst(m_chartPoints)) {
        minY = qMin(minY, point.y());
        maxY = qMax(maxY, point.y());
    }
    if (qFuzzyCompare(minY, maxY)) {
        minY -= 1.0;
        maxY += 1.0;
    }
    m_axisY->setRange(minY - 1.0, maxY + 1.0);
}

void Widget::OnConnectClicked()
{
    if (!m_connected) {
        const int sensorId = m_sensorIdSpin->value();
        const QString ip = m_ipEdit->text().trimmed();
        QMetaObject::invokeMethod(m_control, [this, sensorId, ip]() {
            m_control->SetSensorId(sensorId);
            m_control->ConnectDevice(ip);
        }, Qt::QueuedConnection);
        AppendLog(QString("Connecting sensor=%1 ip=%2").arg(sensorId).arg(ip));
    } else {
        QMetaObject::invokeMethod(m_control, &ColorFocusControl::DisconnectDevice, Qt::QueuedConnection);
    }
}

void Widget::OnStartClicked()
{
    if (!m_acquiring) {
        const int triggerMode = m_triggerModeCombo->currentData().toInt();
        QMetaObject::invokeMethod(m_control, [this, triggerMode]() {
            m_control->StartAcquisition(triggerMode, 100);
        }, Qt::QueuedConnection);
    } else {
        QMetaObject::invokeMethod(m_control, &ColorFocusControl::StopAcquisition, Qt::QueuedConnection);
    }
}

void Widget::OnApplyParametersClicked()
{
    const int measureMode = m_measureModeCombo->currentData().toInt();
    const int scanRate = m_scanRateCombo->currentData().toInt();
    const int triggerMode = m_triggerModeCombo->currentData().toInt();
    const int triggerEdge = m_triggerEdgeCombo->currentData().toInt();
    const int burstNum = m_burstNumSpin->value();
    const int sensitivity = m_sensitivitySpin->value();
    const bool autoSensitivity = m_autoSensitivityCheck->isChecked();
    const int smoothing = m_smoothingSpin->value();
    const int head = m_encoderHeadSpin->value();
    const int tail = m_encoderTailSpin->value();
    const int interval = m_encoderIntervalSpin->value();
    const int encoderSelect = m_encoderSelectCombo->currentData().toInt();
    const int direction = m_encoderDirectionCombo->currentData().toInt();

    QMetaObject::invokeMethod(m_control, [this, measureMode, scanRate, triggerMode, triggerEdge,
                              burstNum, sensitivity, autoSensitivity, smoothing,
                              head, tail, interval, encoderSelect, direction]() {
        const bool ok = m_control->ApplyBasicParameters(measureMode, scanRate, triggerMode, triggerEdge,
                                                        burstNum, sensitivity, autoSensitivity, smoothing);
        if (ok && triggerMode == TriggerEncoder) {
            m_control->SetEncoderTriggerParam(head, tail, interval, encoderSelect, direction);
        }
    }, Qt::QueuedConnection);
}

void Widget::OnDarkClicked()
{
    QMetaObject::invokeMethod(m_control, &ColorFocusControl::Dark, Qt::QueuedConnection);
}

void Widget::OnClearStackClicked()
{
    QMetaObject::invokeMethod(m_control, &ColorFocusControl::ClearDataStack, Qt::QueuedConnection);
    m_chartPoints.clear();
    m_distanceSeries->clear();
    m_sampleIndex = 0.0;
    m_axisX->setRange(0, 120);
    m_axisY->setRange(-5, 5);
}

void Widget::OnRecenterClicked()
{
    QMetaObject::invokeMethod(m_control, &ColorFocusControl::RecenterAllEncoders, Qt::QueuedConnection);
}

void Widget::OnConnectionStateChanged(int sensorId, bool connected)
{
    Q_UNUSED(sensorId)
    SetUiConnected(connected);
}

void Widget::OnAcquisitionStateChanged(int sensorId, bool acquiring)
{
    Q_UNUSED(sensorId)
    SetUiAcquiring(acquiring);
}

void Widget::OnSampleUpdated(int sensorId, float distance, float intensity, int encoder1, int encoder2, int encoder3)
{
    Q_UNUSED(sensorId)
    AppendSample(distance, intensity, encoder1, encoder2, encoder3);
}
