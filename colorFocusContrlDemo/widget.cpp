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
    AppendLog(QString::fromUtf8("\x44\x65\x6D\x6F\xE5\x88\x9D\xE5\xA7\x8B\xE5\x8C\x96\xE5\xAE\x8C\xE6\x88\x90"));
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
    m_ipEdit = ui->m_ipEdit;
    m_sensorIdSpin = ui->m_sensorIdSpin;
    m_connectButton = ui->m_connectButton;
    m_startButton = ui->m_startButton;
    m_applyButton = ui->m_applyButton;
    m_darkButton = ui->m_darkButton;
    m_clearStackButton = ui->m_clearStackButton;
    m_recenterButton = ui->m_recenterButton;
    m_connectionLabel = ui->m_connectionLabel;
    m_acquisitionLabel = ui->m_acquisitionLabel;
    m_distanceLabel = ui->m_distanceLabel;
    m_intensityLabel = ui->m_intensityLabel;
    m_encoderLabel = ui->m_encoderLabel;
    m_measureModeCombo = ui->m_measureModeCombo;
    m_scanRateCombo = ui->m_scanRateCombo;
    m_triggerModeCombo = ui->m_triggerModeCombo;
    m_triggerEdgeCombo = ui->m_triggerEdgeCombo;
    m_burstNumSpin = ui->m_burstNumSpin;
    m_sensitivitySpin = ui->m_sensitivitySpin;
    m_autoSensitivityCheck = ui->m_autoSensitivityCheck;
    m_smoothingSpin = ui->m_smoothingSpin;
    m_encoderHeadSpin = ui->m_encoderHeadSpin;
    m_encoderTailSpin = ui->m_encoderTailSpin;
    m_encoderIntervalSpin = ui->m_encoderIntervalSpin;
    m_encoderSelectCombo = ui->m_encoderSelectCombo;
    m_encoderDirectionCombo = ui->m_encoderDirectionCombo;
    m_logEdit = ui->m_logEdit;

    m_sensorIdSpin->setRange(0, 15);
    m_sensorIdSpin->setValue(0);
    m_ipEdit->setText("192.168.10.11");

    m_measureModeCombo->clear();
    m_measureModeCombo->addItem(QString::fromUtf8("\xE4\xBD\x8D\xE7\xA7\xBB"), CCS_DISTANCE_MODE);
    m_measureModeCombo->addItem(QString::fromUtf8("\xE5\x8E\x9A\xE5\xBA\xA6"), CCS_THICKNESS_MODE);
    m_measureModeCombo->addItem(QString::fromUtf8("\xE5\xA4\x9A\xE5\xB1\x82"), CCS_MULTI_LAYER_MODE);

    m_scanRateCombo->clear();
    const QStringList rates = QStringList()
            << "200 Hz" << "500 Hz" << "1000 Hz" << "2000 Hz" << "3000 Hz"
            << "4000 Hz" << "4500 Hz" << "5000 Hz" << "6000 Hz" << "7000 Hz"
            << "8000 Hz" << "9000 Hz" << "10000 Hz";
    for (int i = 0; i < rates.count(); ++i) {
        m_scanRateCombo->addItem(rates.at(i), i);
    }
    m_scanRateCombo->setCurrentIndex(2);

    m_triggerModeCombo->clear();
    m_triggerModeCombo->addItem(QString::fromUtf8("\xE8\xBF\x9E\xE7\xBB\xAD\xE6\xA8\xA1\xE5\xBC\x8F"), TriggerContinue);
    m_triggerModeCombo->addItem(QString::fromUtf8("\xE8\xBE\xB9\xE6\xB2\xBF\xE5\x90\xAF\xE5\x8A\xA8"), TriggerStartByEdge);
    m_triggerModeCombo->addItem(QString::fromUtf8("\xE7\x94\xB5\xE5\xB9\xB3\xE5\x90\xAF\xE5\x81\x9C"), TriggerStartStopByState);
    m_triggerModeCombo->addItem(QString::fromUtf8("\xE8\xBE\xB9\xE6\xB2\xBF\xE5\x90\xAF\xE5\x81\x9C"), TriggerStartStopByEdge);
    m_triggerModeCombo->addItem(QString::fromUtf8("\x42\x75\x72\x73\x74\xE6\xA8\xA1\xE5\xBC\x8F"), TriggerBurst);
    m_triggerModeCombo->addItem(QString::fromUtf8("\xE7\xBC\x96\xE7\xA0\x81\xE5\x99\xA8\xE8\xA7\xA6\xE5\x8F\x91"), TriggerEncoder);

    m_triggerEdgeCombo->clear();
    m_triggerEdgeCombo->addItem(QString::fromUtf8("\xE4\xB8\x8B\xE9\x99\x8D\xE6\xB2\xBF\xE6\x88\x96\xE4\xBD\x8E\xE7\x94\xB5\xE5\xB9\xB3"), 0);
    m_triggerEdgeCombo->addItem(QString::fromUtf8("\xE4\xB8\x8A\xE5\x8D\x87\xE6\xB2\xBF\xE6\x88\x96\xE9\xAB\x98\xE7\x94\xB5\xE5\xB9\xB3"), 1);

    m_burstNumSpin->setRange(1, 1000000);
    m_burstNumSpin->setValue(3000);
    m_sensitivitySpin->setRange(1, 100);
    m_sensitivitySpin->setValue(50);
    m_smoothingSpin->setRange(0, 2048);
    m_smoothingSpin->setValue(0);

    m_encoderHeadSpin->setRange(INT_MIN + 1, INT_MAX);
    m_encoderHeadSpin->setValue(INT_MIN + 1);
    m_encoderTailSpin->setRange(INT_MIN + 1, INT_MAX);
    m_encoderTailSpin->setValue(INT_MAX);
    m_encoderIntervalSpin->setRange(1, 1000000);
    m_encoderIntervalSpin->setValue(20);
    m_encoderSelectCombo->clear();
    m_encoderSelectCombo->addItem(QString::fromUtf8("\xE7\xBC\x96\xE7\xA0\x81\xE5\x99\xA8\x31"), 0);
    m_encoderSelectCombo->addItem(QString::fromUtf8("\xE7\xBC\x96\xE7\xA0\x81\xE5\x99\xA8\x32"), 1);
    m_encoderSelectCombo->addItem(QString::fromUtf8("\xE7\xBC\x96\xE7\xA0\x81\xE5\x99\xA8\x33"), 2);
    m_encoderDirectionCombo->clear();
    m_encoderDirectionCombo->addItem(QString::fromUtf8("\xE6\xAD\xA3\xE5\x90\x91"), 1);
    m_encoderDirectionCombo->addItem(QString::fromUtf8("\xE5\x8F\x8D\xE5\x90\x91"), 0);

    m_distanceSeries = new QLineSeries(this);
    m_distanceSeries->setName(QString::fromUtf8("\xE8\xB7\x9D\xE7\xA6\xBB"));
    m_chart = new QChart();
    m_chart->addSeries(m_distanceSeries);
    m_chart->legend()->hide();
    m_chart->setTitle(QString::fromUtf8("\xE8\xB7\x9D\xE7\xA6\xBB\xE5\xAE\x9E\xE6\x97\xB6\xE6\x9B\xB2\xE7\xBA\xBF"));
    m_axisX = new QValueAxis(this);
    m_axisX->setTitleText(QString::fromUtf8("\xE9\x87\x87\xE6\xA0\xB7\xE7\x82\xB9"));
    m_axisX->setRange(0, 120);
    m_axisY = new QValueAxis(this);
    m_axisY->setTitleText(QString::fromUtf8("\xE8\xB7\x9D\xE7\xA6\xBB"));
    m_axisY->setRange(-5, 5);
    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_chart->addAxis(m_axisY, Qt::AlignLeft);
    m_distanceSeries->attachAxis(m_axisX);
    m_distanceSeries->attachAxis(m_axisY);
    m_chartView = new QChartView(m_chart, ui->m_chartContainer);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setMinimumHeight(360);

    QVBoxLayout *chartLayout = new QVBoxLayout(ui->m_chartContainer);
    chartLayout->setContentsMargins(0, 0, 0, 0);
    chartLayout->addWidget(m_chartView);
    ui->m_chartContainer->setLayout(chartLayout);

    m_logEdit->setReadOnly(true);
    m_logEdit->setMaximumBlockCount(500);
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
        AppendLog(QString::fromUtf8("\xE9\x94\x99\xE8\xAF\xAF\x3A\x20\x25\x31").arg(message));
    });
    connect(m_control, &ColorFocusControl::logMessage, this, &Widget::AppendLog);
    connect(m_control, &ColorFocusControl::bufferedDataUpdated, this, [this](int sensorId, int count) {
        AppendLog(QString::fromUtf8("\xE7\xBC\x93\xE5\xAD\x98\xE6\x95\xB0\xE6\x8D\xAE\xE5\xB7\xB2\xE6\x9B\xB4\xE6\x96\xB0\x2E\x20\xE4\xBC\xA0\xE6\x84\x9F\xE5\x99\xA8\x3D\x25\x31\x20\xE7\x82\xB9\xE6\x95\xB0\x3D\x25\x32").arg(sensorId).arg(count));
    });
}

void Widget::SetUiConnected(bool connected)
{
    m_connected = connected;
    m_connectionLabel->setText(connected ? QString::fromUtf8("\xE5\xB7\xB2\xE8\xBF\x9E\xE6\x8E\xA5") : QString::fromUtf8("\xE6\x9C\xAA\xE8\xBF\x9E\xE6\x8E\xA5"));
    m_connectButton->setText(connected ? QString::fromUtf8("\xE6\x96\xAD\xE5\xBC\x80") : QString::fromUtf8("\xE8\xBF\x9E\xE6\x8E\xA5"));
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
    m_acquisitionLabel->setText(acquiring ? QString::fromUtf8("\xE9\x87\x87\xE9\x9B\x86\xE4\xB8\xAD") : QString::fromUtf8("\xE5\xB7\xB2\xE5\x81\x9C\xE6\xAD\xA2"));
    m_startButton->setText(acquiring ? QString::fromUtf8("\xE5\x81\x9C\xE6\xAD\xA2\xE9\x87\x87\xE9\x9B\x86") : QString::fromUtf8("\xE5\xBC\x80\xE5\xA7\x8B\xE9\x87\x87\xE9\x9B\x86"));
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
    m_encoderLabel->setText(QString::fromUtf8("\xE7\xBC\x96\xE7\xA0\x81\xE5\x99\xA8\x31\x3D\x25\x31\x20\xE7\xBC\x96\xE7\xA0\x81\xE5\x99\xA8\x32\x3D\x25\x32\x20\xE7\xBC\x96\xE7\xA0\x81\xE5\x99\xA8\x33\x3D\x25\x33").arg(encoder1).arg(encoder2).arg(encoder3));

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
        AppendLog(QString::fromUtf8("\xE6\xAD\xA3\xE5\x9C\xA8\xE8\xBF\x9E\xE6\x8E\xA5\xE4\xBC\xA0\xE6\x84\x9F\xE5\x99\xA8\x3D\x25\x31\x20\x49\x50\x3D\x25\x32").arg(sensorId).arg(ip));
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
