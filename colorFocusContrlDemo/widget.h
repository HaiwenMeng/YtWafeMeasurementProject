#ifndef WIDGET_H
#define WIDGET_H

#include "colorFocus_control.h"

#include <QList>
#include <QPointF>
#include <QWidget>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QPlainTextEdit;
class QSpinBox;
class QThread;

QT_CHARTS_USE_NAMESPACE

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private:
    void BuildUi();
    void ConnectSignals();
    void SetUiConnected(bool connected);
    void SetUiAcquiring(bool acquiring);
    void AppendLog(const QString &message);
    void AppendSample(float distance, float intensity, int encoder1, int encoder2, int encoder3);
    void ApplyAxisRange(float distance);

private slots:
    void OnConnectClicked();
    void OnStartClicked();
    void OnApplyParametersClicked();
    void OnDarkClicked();
    void OnClearStackClicked();
    void OnRecenterClicked();
    void OnConnectionStateChanged(int sensorId, bool connected);
    void OnAcquisitionStateChanged(int sensorId, bool acquiring);
    void OnSampleUpdated(int sensorId, float distance, float intensity, int encoder1, int encoder2, int encoder3);

private:
    Ui::Widget *ui;
    ColorFocusControl *m_control;
    QThread *m_controlThread;

    QLineEdit *m_ipEdit;
    QSpinBox *m_sensorIdSpin;
    QPushButton *m_connectButton;
    QPushButton *m_startButton;
    QPushButton *m_applyButton;
    QPushButton *m_darkButton;
    QPushButton *m_clearStackButton;
    QPushButton *m_recenterButton;
    QLabel *m_connectionLabel;
    QLabel *m_acquisitionLabel;
    QLabel *m_distanceLabel;
    QLabel *m_intensityLabel;
    QLabel *m_encoderLabel;
    QComboBox *m_measureModeCombo;
    QComboBox *m_scanRateCombo;
    QComboBox *m_triggerModeCombo;
    QComboBox *m_triggerEdgeCombo;
    QSpinBox *m_burstNumSpin;
    QSpinBox *m_sensitivitySpin;
    QCheckBox *m_autoSensitivityCheck;
    QSpinBox *m_smoothingSpin;
    QSpinBox *m_encoderHeadSpin;
    QSpinBox *m_encoderTailSpin;
    QSpinBox *m_encoderIntervalSpin;
    QComboBox *m_encoderSelectCombo;
    QComboBox *m_encoderDirectionCombo;
    QPlainTextEdit *m_logEdit;

    QChart *m_chart;
    QChartView *m_chartView;
    QLineSeries *m_distanceSeries;
    QValueAxis *m_axisX;
    QValueAxis *m_axisY;
    QList<QPointF> m_chartPoints;
    bool m_connected;
    bool m_acquiring;
    double m_sampleIndex;
};
#endif // WIDGET_H
