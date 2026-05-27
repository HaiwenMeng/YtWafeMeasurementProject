#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "colorFocus_control.h"
#include "MotionController.h"
#include "paramsettings.h"
#include "xml_config_manager.h"

#include <QFuture>
#include <QMainWindow>
#include <QMutex>
#include <QPointF>
#include <QVector>
#include <atomic>
#include <functional>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QCloseEvent;
class QThread;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onLoadConfigClicked();
    void onSaveConfigClicked();
    void onConnectMotionClicked();
    void onDisconnectMotionClicked();
    void onConnectColorFocusClicked();
    void onStartAcquisitionClicked();
    void onStopAcquisitionClicked();
    void onStopMotionClicked();
    void onMoveStandardClicked();
    void onFourCalibrationClicked();
    void onStandard1550CalibrationClicked();
    void onBrowseCsvClicked();
    void onRunAlgorithmClicked();
    void onMotionConnectedChanged(bool connected);
    void onMotionPositionUpdated(const Motion::AxisPosition &position);
    void onMotionStateUpdated(const Motion::AxisStateSnapshot &state);
    void onColorFocusSampleUpdated(int sensorId, float distance, float intensity, int encoder1, int encoder2, int encoder3);
    void appendLog(const QString &message);

private:
    struct StandardSpec
    {
        int row = 0;
        int id = 0;
        double x = 0.0;
        double y = 0.0;
        double thickness = 0.0;
        double historyTotal = 0.0;
    };

    struct DistanceSnapshot
    {
        bool topValid = false;
        bool bottomValid = false;
        double top = 0.0;
        double bottom = 0.0;
    };

    void setupUiState();
    void setupColorFocusThread();
    void connectSignals();
    bool loadConfigToUi();
    bool saveUiToConfig();
    void applySettingsToUi(const ParamSettings &settings);
    ParamSettings readSettingsFromUi() const;
    bool validateSettings(const ParamSettings &settings, QString *errorMessage) const;
    bool readDoubleRequired(const QString &text, const QString &name, double *value, QString *errorMessage) const;
    bool readPortRequired(const QString &text, quint16 *value, QString *errorMessage) const;
    QVector<StandardSpec> buildStandards(const ParamSettings &settings, QString *errorMessage) const;
    bool ensureDevicesReady(QString *errorMessage) const;
    void setBusy(bool busy);
    void setMotionStatusText(const QString &text);
    void setColorFocusStatusText();
    void updateResultRow(const StandardSpec &spec, double top, double bottom, double total);
    void clearRunTotals();
    void finishCalibration(bool success, const QString &message, const ParamSettings &settings);
    void failCalibration(const QString &message, const ParamSettings &settings);
    void runFourCalibration(ParamSettings settings);
    void runStandard1550Calibration(ParamSettings settings);
    bool moveZBlocking(double z, QString *errorMessage);
    bool moveXyBlocking(double x, double y, double velocity, QString *errorMessage);
    bool waitForZ(double z, QString *errorMessage);
    bool waitForXy(double x, double y, QString *errorMessage);
    bool invokeMotion(const std::function<bool()> &call, const QString &action, QString *errorMessage);
    bool measureStandard(const StandardSpec &spec, double dx, double dy, double velocity, double *total, QString *errorMessage);
    double filterValue(double newValue, int windowSize = 5);
    DistanceSnapshot distanceSnapshot() const;
    QString formatNumber(double value, int precision = 3) const;
    void stopColorFocus();
    void stopColorFocusAcquisition();
    bool invokeColorFocus(ColorFocusControl *control, const std::function<bool()> &call, const QString &action);

private:
    Ui::MainWindow *ui;
    XmlConfigManager m_configManager;
    ParamSettings m_settings;
    MotionController m_motionController;
    ColorFocusControl *m_topFocus;
    ColorFocusControl *m_bottomFocus;
    QThread *m_focusThread;
    QFuture<void> m_calibrationFuture;
    mutable QMutex m_stateMutex;
    Motion::AxisPosition m_axisPosition;
    Motion::AxisStateSnapshot m_axisState;
    bool m_topConnected;
    bool m_bottomConnected;
    bool m_topAcquiring;
    bool m_bottomAcquiring;
    bool m_topDistanceValid;
    bool m_bottomDistanceValid;
    double m_topDistance;
    double m_bottomDistance;
    QVector<double> m_filterBuffer;
    std::atomic_bool m_cancelRequested;
    bool m_busy;
};

#endif // MAINWINDOW_H
