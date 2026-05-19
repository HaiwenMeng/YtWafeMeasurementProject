#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "../motion/MotionController.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTimer>
#include <QList>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onConnectClicked();
    void onDisconnectClicked();
    void onEnableClicked();
    void onDisableClicked();
    void onHomeClicked();
    void onClearErrorClicked();
    void onStopAxisClicked();
    void onAbsoluteMoveClicked();
    void onRelativeMoveClicked();
    void onContinuousPositiveClicked();
    void onContinuousNegativeClicked();
    void onSetVelocityClicked();
    void onLineMoveClicked();
    void onStopLineClicked();
    void onSetTriggerClicked();
    void onTriggerOnClicked();
    void onTriggerOffClicked();
    void onAutoReadToggled(bool enabled);
    void onCtrlAutoSndToggled(bool enabled);
    void onManualReadClicked();
    void onTimerRead();
    void onConnectionChanged(bool connected);
    void appendLog(const QString &message);
    void onPositionUpdated(const Motion::AxisPosition &position);
    void onStateUpdated(const Motion::AxisStateSnapshot &state);
    void onHomeStatusUpdated(bool homeX, bool homeY, bool homeZ);

private:
    QWidget *buildCentralWidget();
    QGroupBox *buildConnectionGroup();
    QGroupBox *buildAxisGroup();
    QGroupBox *buildSingleAxisGroup();
    QGroupBox *buildLineGroup();
    QGroupBox *buildTriggerGroup();
    QGroupBox *buildRefreshGroup();
    QDoubleSpinBox *createDoubleSpin(double minValue, double maxValue, double value, int decimals = 3) const;
    QSpinBox *createIntSpin(int minValue, int maxValue, int value) const;
    int currentAxis() const;
    int triggerAxis() const;
    bool ensurePositive(QDoubleSpinBox *spin, const QString &name) const;
    void registerConnectedWidget(QWidget *widget);
    void updateUiEnabled(bool connected);

    MotionController m_controller;
    QTimer m_readTimer;

    QLineEdit *m_ipEdit;
    QSpinBox *m_portSpin;
    QPushButton *m_connectButton;
    QPushButton *m_disconnectButton;
    QLabel *m_connectionStatusLabel;
    QPlainTextEdit *m_logEdit;

    QComboBox *m_axisCombo;
    QLabel *m_posXLabel;
    QLabel *m_posYLabel;
    QLabel *m_posZLabel;
    QLabel *m_stateXLabel;
    QLabel *m_stateYLabel;
    QLabel *m_stateZLabel;
    QLabel *m_errorXLabel;
    QLabel *m_errorYLabel;
    QLabel *m_errorZLabel;
    QLabel *m_homeLabel;

    QPushButton *m_enableButton;
    QPushButton *m_disableButton;
    QPushButton *m_homeButton;
    QPushButton *m_clearErrorButton;
    QPushButton *m_stopAxisButton;
    QDoubleSpinBox *m_absPositionSpin;
    QDoubleSpinBox *m_relDistanceSpin;
    QDoubleSpinBox *m_velocitySpin;
    QDoubleSpinBox *m_accelerationSpin;
    QDoubleSpinBox *m_decelerationSpin;

    QDoubleSpinBox *m_lineXSpin;
    QDoubleSpinBox *m_lineYSpin;
    QDoubleSpinBox *m_lineVelocitySpin;
    QDoubleSpinBox *m_lineAccelerationSpin;
    QDoubleSpinBox *m_lineDecelerationSpin;

    QComboBox *m_triggerAxisCombo;
    QDoubleSpinBox *m_triggerStartSpin;
    QDoubleSpinBox *m_triggerEndSpin;
    QDoubleSpinBox *m_triggerIntervalSpin;
    QSpinBox *m_triggerDirectionSpin;
    QSpinBox *m_triggerPulseSpin;

    QCheckBox *m_autoReadCheck;
    QSpinBox *m_autoReadIntervalSpin;
    QCheckBox *m_ctrlAutoSndCheck;
    QSpinBox *m_ctrlAutoSndIntervalSpin;
    QPushButton *m_manualReadButton;
    QList<QWidget *> m_connectedWidgets;
};

#endif // MAINWINDOW_H
