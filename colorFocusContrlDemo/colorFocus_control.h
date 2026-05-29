#ifndef COLORFOCUS_CONTROL_H
#define COLORFOCUS_CONTROL_H

#include <QObject>
#include <QList>
#include <QMap>
#include <QTimer>
#include <QVector>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "DLL_CCS.h"

#define COLOR_FOCUS_DEFAULT_BUFFER_LENGTH 50000
#define SENSOR_ID1 1
#define SENSOR_ID2 2
#define BUFFER_LENGTH COLOR_FOCUS_DEFAULT_BUFFER_LENGTH

enum TriggerMode
{
    Continue = 0,
    TriggerContinue = 0,
    Burst = 4,
    TriggerStartByEdge = 1,
    TriggerStartStopByState = 2,
    TriggerStartStopByEdge = 3,
    TriggerBurst = 4,
    Encoder = 5,
    TriggerEncoder = 5
};

class ColorFocusControl : public QObject
{
    Q_OBJECT

public:
    explicit ColorFocusControl(QObject *parent = nullptr);
    ~ColorFocusControl();

    Q_INVOKABLE void SetSensorId(int sensorId);
    Q_INVOKABLE int sensorId() const;
    Q_INVOKABLE bool isConnected() const;
    Q_INVOKABLE bool isAcquiring() const;
    Q_INVOKABLE QString lastErrorMessage() const;

    Q_INVOKABLE bool ConnectDevice(const QString &ipAddress);
    Q_INVOKABLE bool DisconnectDevice();

    Q_INVOKABLE bool StartAcquisition(int triggerMode, int pollIntervalMs = 100);
    Q_INVOKABLE bool StopAcquisition();
    bool StartAcquisition(TriggerMode triggerMode) { return StartAcquisition(static_cast<int>(triggerMode)); }
    bool StartAcquisition_CCS(TriggerMode triggerMode) { return StartAcquisition(triggerMode); }
    bool StopAcquisition_CCS() { return StopAcquisition(); }
    Q_INVOKABLE bool ClearDataStack();
    Q_INVOKABLE bool InitAcquisitionEvent(int bufferLength);
    Q_INVOKABLE bool ReadDistanceBuffer(int readNum, int triggerAxis = 0);
    Q_INVOKABLE bool CloseAcquisitionEvent();
    bool CloseAcquisitionEvent(int bufferLength, int triggerAxis = 0);
    bool ChangeTriggerMode(TriggerMode mode);

    Q_INVOKABLE bool ApplyBasicParameters(int measureMode,
                                          int scanRate,
                                          int triggerMode,
                                          int triggerEdge,
                                          int burstNum,
                                          int sensitivity,
                                          bool autoSensitivity,
                                          int smoothingFactor);

    Q_INVOKABLE bool SetMeasureMode(int value);
    Q_INVOKABLE bool SetScanRate(int value);
    Q_INVOKABLE bool SetTriggerMode(int value);
    Q_INVOKABLE bool SetTriggerEdge(int value);
    Q_INVOKABLE bool SetBurstNum(int value);
    Q_INVOKABLE bool SetSensitivity(int value);
    Q_INVOKABLE bool SetAutoSensitivityEnabled(bool enabled);
    Q_INVOKABLE bool SetSmoothingFactor(int value);
    Q_INVOKABLE bool SetSinglePointSmoothingFactor(int value);
    Q_INVOKABLE bool SetRefractive(float value);
    Q_INVOKABLE bool SetHoldLastValue(bool enabled);
    Q_INVOKABLE bool SetOpticalPen(int value);
    Q_INVOKABLE bool SetValueUnit(int value);
    Q_INVOKABLE bool SetMirrorState(int value);
    Q_INVOKABLE bool SetLightState(bool enabled);
    Q_INVOKABLE bool SetLightMode(int value);
    Q_INVOKABLE bool SetEncoderTriggerParam(int head, int tail, int interval, int encoderSelect, int direction);
    bool SetEcdTrgParam(INT32 head, INT32 tail, INT32 interval, BYTE encoderSelect);
    bool GetEcdTrgParam(PINT32 head, PINT32 tail, PINT32 interval, PBYTE encoderSelect, PBYTE direction);
    Q_INVOKABLE bool SetDAParam(float signalLowerLimit,
                                float signalUpperLimit,
                                float voltageLowerLimit,
                                float voltageUpperLimit,
                                float invalidValueVoltage);

    Q_INVOKABLE bool Dark();
    Q_INVOKABLE bool RecenterEncoder1();
    Q_INVOKABLE bool RecenterEncoder2();
    Q_INVOKABLE bool RecenterEncoder3();
    Q_INVOKABLE bool RecenterAllEncoders();
    bool RecenterEncoder() { return RecenterAllEncoders(); }
    bool XRecenterEncoder() { return RecenterEncoder1(); }
    bool YRecenterEncoder() { return RecenterEncoder2(); }
    Q_INVOKABLE bool ParamSave();
    Q_INVOKABLE bool ParamDefault();
    Q_INVOKABLE bool StartSpectrum();
    Q_INVOKABLE bool ReadSpectrum();
    Q_INVOKABLE bool SetWaveType(int value);
    Q_INVOKABLE bool SendSoftwareTrigger();

    bool GetMeasureMode(DWORD *value);
    bool GetScanRate(DWORD *value);
    bool GetTriggerMode(DWORD *value);
    bool GetTriggerEdge(DWORD *value);
    bool GetBurstNum(DWORD *value);
    bool GetSensitivity(DWORD *value);
    bool GetAutoSensitivityEnabled(DWORD *value);
    bool GetSmoothingFactor(DWORD *value);
    bool GetSinglePointSmoothingFactor(DWORD *value);
    bool GetRefractive(float *value);
    bool GetHoldLastValue(DWORD *value);
    bool GetOpticalPen(DWORD *value);
    bool GetScale(DWORD *value);
    bool GetValueUnit(DWORD *value);
    bool GetMirrorState(DWORD *value);
    bool GetLightState(DWORD *value);
    bool GetLightMode(DWORD *value);
    bool GetEncoderTriggerParam(INT32 *head, INT32 *tail, INT32 *interval, BYTE *encoderSelect, BYTE *direction);
    bool GetDAParam(float *signalLowerLimit,
                    float *signalUpperLimit,
                    float *voltageLowerLimit,
                    float *voltageUpperLimit,
                    float *invalidValueVoltage);
    bool GetDarkState(DWORD *value);
    bool GetDigitalInput(DWORD *value);
    bool SetDigitalOutput(DWORD value);
    bool GetDigitalOutput(DWORD *value);
    bool GetFirmwareVersion(DWORD *value);
    bool GetProductionDate(DWORD *value);
    bool GetSerial(DWORD *value);
    bool GetHardwareVersion(DWORD *value);

    double GetCurrentDistance();
    INT32 ChangeToEncoderValue(float position) const;
    float ChangeToPositionValue(INT32 encoder) const;
    const QList<_DISTANCE_VALUE> &distanceValues() const;
    const QMap<INT32, _DISTANCE_VALUE> &distanceValueMap() const;
    const QVector<int> &encoders() const;
    QMap<INT32, _DISTANCE_VALUE>& GetMeasurePointDistanceMap() { return m_distanceValueMap; }
    QList<_DISTANCE_VALUE>& GetMeasurePointDistanceList() { return m_distanceValues; }
    QVector<int>& GetEncoders() { return m_encoders; }
    void ClearMeasureData();
    void SetSensoId(UINT16 sensorId) { SetSensorId(sensorId); }
    void SetMeasureState(bool isMeasuring) { m_isMeasuring = isMeasuring; }
    bool getConnectFailState() const { return m_connectFailed; }

signals:
    void sampleUpdated(int sensorId, float distance, float intensity, int encoder1, int encoder2, int encoder3);
    void s_colorFocusUpdated(UINT16 sensorId, float distance, float intensity);
    void spectrumUpdated(int sensorId, QVector<quint16> spectrum);
    void bufferedDataUpdated(int sensorId, int count);
    void connectionStateChanged(int sensorId, bool connected);
    void s_connectionStateChanged(UINT16 sensorId, bool connected);
    void acquisitionStateChanged(int sensorId, bool acquiring);
    void errorOccurred(int sensorId, QString errorMessage);
    void s_sendErrorMsg(int sensorId, QString errorMessage);
    void logMessage(QString message);
    void s_writeLog(QString message);
    void connectionTimeout();

private slots:
    void PollCurrentSample();

private:
    bool ensureConnected(const QString &action);
    bool reportSdkFailure(const QString &action);
    void reportLog(const QString &message);
    void closeEventHandle();
    bool pauseAcquisitionForParameterChange(bool *restartNeeded);
    bool resumeAcquisitionAfterParameterChange(bool restartNeeded);

private:
    UINT16 m_sensorId;
    bool m_connected;
    bool m_acquiring;
    QString m_lastErrorMessage;
    QTimer *m_pollTimer;
    HANDLE m_endAcqEvent;
    int m_pollIntervalMs;
    int m_currentTriggerMode;
    bool m_connectFailed = false;
    bool m_isMeasuring = false;

    QList<_DISTANCE_VALUE> m_distanceValues;
    QMap<INT32, _DISTANCE_VALUE> m_distanceValueMap;
    QVector<int> m_encoders;
};

#endif // COLORFOCUS_CONTROL_H
