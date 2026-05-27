#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "../motion/MotionController.h"

#include <QList>
#include <QMainWindow>
#include <QTimer>

class QDoubleSpinBox;
class QWidget;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

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
    void initUiState();
    void connectUiSignals();
    void registerConnectedWidget(QWidget *widget);
    void updateUiEnabled(bool connected);
    int currentAxis() const;
    int triggerAxis() const;
    bool ensurePositive(QDoubleSpinBox *spin, const QString &name) const;

    Ui::MainWindow *ui;
    MotionController m_controller;
    QTimer m_readTimer;
    QList<QWidget *> m_connectedWidgets;
};

#endif // MAINWINDOW_H
