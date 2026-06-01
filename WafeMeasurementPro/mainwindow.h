#ifndef WAFEMEASUREMENTPRO_MAINWINDOW_H
#define WAFEMEASUREMENTPRO_MAINWINDOW_H

#include "MotionController.h"
#include "WaferAlgorithm.h"
#include "colorFocus_control.h"
#include "paramsettings.h"
#include "pro_types.h"
#include "recipe_database.h"
#include "xml_config_manager.h"

#include <QFuture>
#include <QMainWindow>
#include <QMutex>
#include <QPointer>
#include <QTimer>
#include <QtCharts/QChartView>
#include <QtDataVisualization/Q3DSurface>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QAbstract3DSeries;
class QCloseEvent;
class QThread;
namespace QtCharts {
class QLineSeries;
class QValueAxis;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onInitializeClicked();
    void onRecipeManagerClicked();
    void onParameterSettingsClicked();
    void onEmergencyStopClicked();
    void onRecipeChanged(int index);
    void onCalibrationClicked();
    void onScanGravityClicked();
    void onLoadClicked();
    void onMeasureClicked();
    void onStopClicked();
    void onMotionConnected(bool connected);
    void onMotionPositionUpdated(const Motion::AxisPosition &position);
    void onMotionStateUpdated(const Motion::AxisStateSnapshot &state);
    void onColorFocusSampleUpdated(int sensorId, float distance, float intensity, int encoder1, int encoder2, int encoder3);
    void refreshStatusLabels();

private:
    struct DistanceSnapshot
    {
        bool topValid = false;
        bool bottomValid = false;
        double top = 0.0;
        double bottom = 0.0;
        int encoder = 0;
    };

    struct StandardSpec
    {
        int row = 0;
        int id = 0;
        double x = 0.0;
        double y = 0.0;
        double thickness = 0.0;
    };

    void setupTables();
    void setupCharts();
    void setupColorFocusThread();
    void connectSignals();
    bool loadSettings(QString *errorMessage);
    bool validateSettings(const ParamSettings &settings, QString *errorMessage) const;
    bool reloadRecipes(QString *errorMessage);
    ProRecipe currentRecipe() const;
    void setBusy(bool busy, const QString &message);
    void appendLog(const QString &message);
    void showError(const QString &message);
    void updateRecipeLabels();
    bool waitFor(std::function<bool()> predicate, int timeoutMs, const QString &timeoutMessage, QString *errorMessage);
    bool waitForMotionConnection(QString *errorMessage);
    bool waitForXy(double x, double y, QString *errorMessage);
    bool waitForZ(double z, QString *errorMessage);
    bool waitForFocusSamples(QString *errorMessage);
    bool invokeMotion(const std::function<bool()> &call, const QString &action, QString *errorMessage);
    bool invokeFocus(ColorFocusControl *control, const std::function<bool()> &call, const QString &action, QString *errorMessage);
    bool initializeDevices(QString *errorMessage);
    bool startContinuousFocusSamples(QString *errorMessage);
    bool moveLoadPosition(QString *errorMessage);
    bool moveXy(double x, double y, double velocity, QString *errorMessage);
    bool moveZ(double z, QString *errorMessage);
    bool runCalibration(ParamSettings settings, QString *errorMessage);
    bool measureStandard(const StandardSpec &spec, double dx, double dy, double *total, QString *errorMessage);
    QVector<StandardSpec> standardsFromSettings(const ParamSettings &settings, QString *errorMessage) const;
    QVector<ProPathLine> buildPathLines(const ProRecipe &recipe, const ParamSettings &settings, QString *errorMessage) const;
    bool scanRecipeLines(const ProRecipe &recipe, bool gravityMode, ProRunResult *runResult, QString *errorMessage);
    bool scanOneLine(const ProPathLine &line, const ProRecipe &recipe, bool gravityMode,
                     const QMap<int, double> &gravityMap, QVector<ProMeasurePoint> *points, QString *errorMessage);
    bool sampleCenterPoint(const ProRecipe &recipe, ProMeasurePoint *point, QString *errorMessage);
    DistanceSnapshot latestDistanceSnapshot() const;
    DistanceSnapshot distanceSnapshot() const;
    double calibrationTotalForRecipe(const ProRecipe &recipe) const;
    Wafer::WaferDataset buildDataset(const ProRecipe &recipe, const ProRunResult &runResult) const;
    Wafer::AlgorithmOptions algorithmOptions(const ProRecipe &recipe) const;
    bool saveGravityFiles(const ProRecipe &recipe, const ProRunResult &normalResult, const ProRunResult &oppositeResult, QString *errorMessage);
    bool loadGravityFiles(const ProRecipe &recipe, QMap<int, QMap<int, double>> *gravity, QString *errorMessage) const;
    QString gravityDir(const ProRecipe &recipe) const;
    bool writeResultFiles(ProRunResult *runResult, QString *errorMessage);
    bool writeRawCsv(const QString &path, const ProRunResult &runResult, QString *errorMessage) const;
    bool writeLineCsvs(const QString &dir, const ProRunResult &runResult, QString *errorMessage) const;
    bool writeSummaryCsv(const QString &path, const ProRunResult &runResult, QString *errorMessage) const;
    void updateCurve(const QVector<ProMeasurePoint> &linePoints);
    void updateHeatMap(const ProRunResult &runResult);
    void updateSurface(const ProRunResult &runResult);
    void updateResultTable(const ProRunResult &runResult);
    void updateCalibrationTable(const ParamSettings &settings);
    QMap<int, double> gravityMapForLine(const ProRecipe &recipe, int lineIndex, QString *errorMessage) const;
    void finishRunOnUi(const ProRunResult &runResult, const QString &message);
    double filteredValue(double value, QVector<double> *buffer, int windowSize) const;

private:
    Ui::MainWindow *ui;
    XmlConfigManager m_config;
    ParamSettings m_settings;
    RecipeDatabase m_recipeDatabase;
    QList<ProRecipe> m_recipes;
    MotionController m_motion;
    ColorFocusControl *m_topFocus;
    ColorFocusControl *m_bottomFocus;
    QThread *m_focusThread;
    QTimer m_statusTimer;
    QFuture<void> m_future;
    mutable QMutex m_stateMutex;
    Motion::AxisPosition m_position;
    Motion::AxisStateSnapshot m_axisState;
    bool m_motionConnected;
    bool m_topConnected;
    bool m_bottomConnected;
    bool m_topAcquiring;
    bool m_bottomAcquiring;
    bool m_initialized;
    bool m_busy;
    bool m_stopRequested;
    bool m_topValid;
    bool m_bottomValid;
    double m_topDistance;
    double m_bottomDistance;
    int m_lastEncoder;
    QVector<double> m_calibrationFilter;
    QtCharts::QChartView *m_curveView;
    QtCharts::QLineSeries *m_curveSeries;
    QtCharts::QValueAxis *m_curveAxisX;
    QtCharts::QValueAxis *m_curveAxisY;
    QtCharts::QChartView *m_heatLegendView;
    QtDataVisualization::Q3DSurface *m_surface;
    QWidget *m_surfaceContainer;
};

#endif // WAFEMEASUREMENTPRO_MAINWINDOW_H
