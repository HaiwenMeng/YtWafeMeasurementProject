#include "colorFocus_control.h"

#include <QByteArray>
#include <QDateTime>
#include <QFileInfo>
#include <QMetaObject>
#include <QtGlobal>

namespace
{
bool validLatestSample(float distance, float intensity)
{
    return qIsFinite(static_cast<double>(distance)) &&
           qIsFinite(static_cast<double>(intensity)) &&
           distance > 0.0f;
}

QString loadedCcsDllPath()
{
    HMODULE module = GetModuleHandleW(L"DLL_CCS.dll");
    if (!module) {
        return QStringLiteral("DLL_CCS.dll not loaded");
    }
    wchar_t path[MAX_PATH] = {0};
    const DWORD length = GetModuleFileNameW(module, path, MAX_PATH);
    if (length == 0) {
        return QStringLiteral("DLL_CCS.dll path unavailable");
    }
    return QString::fromWCharArray(path, static_cast<int>(length));
}

QString loadedCcsDllInfo()
{
    const QString path = loadedCcsDllPath();
    QFileInfo info(path);
    if (!info.exists()) {
        return path;
    }
    return QString("path=%1 size=%2 lastModified=%3")
        .arg(info.absoluteFilePath())
        .arg(info.size())
        .arg(info.lastModified().toString(Qt::ISODate));
}

QString ccsSdkBuildTag()
{
#ifdef CCS_SDK_V330
    return QStringLiteral("SDK_V3_30_x64");
#else
    return QStringLiteral("SDK build without CCS_SDK_V330");
#endif
}

bool ccsRuntimeMatchesBuild(QString *errorMessage)
{
#ifdef CCS_SDK_V330
    const QString path = loadedCcsDllPath();
    QFileInfo info(path);
    if (!info.exists()) {
        if (errorMessage) {
            *errorMessage = QString("DLL_CCS runtime check failed. %1 sdkBuild=%2")
                .arg(path)
                .arg(ccsSdkBuildTag());
        }
        return false;
    }
    constexpr qint64 kExpectedV330DllSize = 79872;
    if (info.size() != kExpectedV330DllSize) {
        if (errorMessage) {
            *errorMessage = QString("DLL_CCS runtime check failed. expected SDK_V3_30_x64 size=%1 actual %2")
                .arg(kExpectedV330DllSize)
                .arg(loadedCcsDllInfo());
        }
        return false;
    }
#else
    Q_UNUSED(errorMessage);
#endif
    return true;
}
}

ColorFocusControl::ColorFocusControl(QObject *parent)
    : QObject(parent),
      m_sensorId(0),
      m_connected(false),
      m_acquiring(false),
      m_pollTimer(nullptr),
      m_readThreadRunning(false),
      m_hasLatestSample(false),
      m_latestDistance(0.0f),
      m_latestIntensity(0.0f),
      m_endAcqEvent(nullptr),
      m_pollIntervalMs(100),
      m_currentTriggerMode(TriggerContinue)
{
}

ColorFocusControl::~ColorFocusControl()
{
    DisconnectDevice();
}

void ColorFocusControl::SetSensorId(int sensorId)
{
    if (m_connected) {
        m_lastErrorMessage = QString("SetSensorId failed. sensor=%1 is connected").arg(m_sensorId);
        emit errorOccurred(m_sensorId, m_lastErrorMessage);
        emit s_sendErrorMsg(m_sensorId, m_lastErrorMessage);
        reportLog(m_lastErrorMessage);
        return;
    }
    if (sensorId < 0 || sensorId >= CCS_MAX_SENSOR) {
        m_lastErrorMessage = QString("Invalid sensor id %1").arg(sensorId);
        emit errorOccurred(m_sensorId, m_lastErrorMessage);
        emit s_sendErrorMsg(m_sensorId, m_lastErrorMessage);
        reportLog(m_lastErrorMessage);
        return;
    }
    m_sensorId = static_cast<UINT16>(sensorId);
}

int ColorFocusControl::sensorId() const
{
    return static_cast<int>(m_sensorId);
}

bool ColorFocusControl::isConnected() const
{
    return m_connected;
}

bool ColorFocusControl::isAcquiring() const
{
    return m_acquiring;
}

QString ColorFocusControl::lastErrorMessage() const
{
    return m_lastErrorMessage;
}

bool ColorFocusControl::ConnectDevice(const QString &ipAddress)
{
    if (m_connected) {
        reportLog(QString("Sensor %1 is already connected").arg(m_sensorId));
        emit connectionStateChanged(m_sensorId, true);
        emit s_connectionStateChanged(m_sensorId, true);
        return true;
    }

    QByteArray ipBytes = ipAddress.toLatin1();
    if (ipBytes.isEmpty()) {
        m_lastErrorMessage = QString("ConnectDevice failed. sensor=%1 empty ip").arg(m_sensorId);
        emit errorOccurred(m_sensorId, m_lastErrorMessage);
        emit s_sendErrorMsg(m_sensorId, m_lastErrorMessage);
        reportLog(m_lastErrorMessage);
        m_connectFailed = true;
        return false;
    }

    QString runtimeError;
    if (!ccsRuntimeMatchesBuild(&runtimeError)) {
        m_lastErrorMessage = runtimeError;
        emit errorOccurred(m_sensorId, m_lastErrorMessage);
        emit s_sendErrorMsg(m_sensorId, m_lastErrorMessage);
        reportLog(m_lastErrorMessage);
        m_connectFailed = true;
        return false;
    }

    if (!CCS_ConnectSensor(m_sensorId, ipBytes.data())) {
        return reportSdkFailure(QString("CCS_ConnectSensor ip=%1").arg(QString::fromLatin1(ipBytes)));
    }

    m_connected = true;
    m_connectFailed = false;
    m_lastErrorMessage.clear();
    static std::atomic_bool s_reportedRuntime(false);
    if (!s_reportedRuntime.exchange(true)) {
        reportLog(QString("DLL_CCS runtime %1 sdkBuild=%2")
                      .arg(loadedCcsDllInfo())
                      .arg(ccsSdkBuildTag()));
    }
    emit connectionStateChanged(m_sensorId, true);
    emit s_connectionStateChanged(m_sensorId, true);
    reportLog(QString("Sensor %1 connected to %2").arg(m_sensorId).arg(QString::fromLatin1(ipBytes)));
    return true;
}

bool ColorFocusControl::DisconnectDevice()
{
    bool ok = true;
    if (m_acquiring || m_readThreadRunning.load()) {
        ok = StopAcquisition();
    }
    closeEventHandle();

    if (m_connected) {
        if (!CCS_CloseSensor(m_sensorId)) {
            ok = reportSdkFailure("CCS_CloseSensor");
        } else {
            reportLog(QString("Sensor %1 disconnected").arg(m_sensorId));
        }
        m_connected = false;
        emit connectionStateChanged(m_sensorId, false);
        emit s_connectionStateChanged(m_sensorId, false);
    }
    return ok;
}

bool ColorFocusControl::StartAcquisition(int triggerMode, int pollIntervalMs)
{
    if (!ensureConnected("StartAcquisition")) {
        return false;
    }
    if (m_acquiring) {
        reportLog(QString("Sensor %1 acquisition already started").arg(m_sensorId));
        return true;
    }

    if (!SetScanRate(5)) {
        return false;
    }
    if (!SetTriggerMode(triggerMode)) {
        return false;
    }
    if (!CCS_StartAcquisition(m_sensorId)) {
        return reportSdkFailure("CCS_StartAcquisition");
    }

    m_pollIntervalMs = qMax(20, pollIntervalMs);
    m_currentTriggerMode = triggerMode;
    m_acquiring = true;
    m_hasLatestSample.store(false);
    m_readThreadRunning = true;
    m_dataReadThread = std::thread(&ColorFocusControl::RunDataRead, this);

    emit acquisitionStateChanged(m_sensorId, true);
    reportLog(QString("Sensor %1 acquisition started").arg(m_sensorId));
    return true;
}

bool ColorFocusControl::StopAcquisition()
{
    if (m_pollTimer) {
        m_pollTimer->stop();
    }

    const bool readThreadWasRunning = m_readThreadRunning.load();
    const bool readThreadExists = m_dataReadThread.joinable();
    if (readThreadWasRunning) {
        m_readThreadRunning = false;
    }
    if (readThreadExists && m_dataReadThread.get_id() != std::this_thread::get_id()) {
        m_dataReadThread.join();
    }

    if (!m_acquiring) {
        return true;
    }

    if (!readThreadExists && !CCS_StopAcquisition(m_sensorId)) {
        return reportSdkFailure("CCS_StopAcquisition");
    }

    m_acquiring = false;
    emit acquisitionStateChanged(m_sensorId, false);
    reportLog(QString("Sensor %1 acquisition stopped").arg(m_sensorId));
    return true;
}

bool ColorFocusControl::StartAcquisition_CCS(TriggerMode triggerMode)
{
    if (!ensureConnected("StartAcquisition_CCS")) {
        return false;
    }

    m_distanceValues.clear();
    m_distanceValueMap.clear();
    m_encoders.clear();

    if (!SetScanRate(5)) {
        return false;
    }
    if (!SetTriggerMode(static_cast<int>(triggerMode))) {
        return false;
    }
    if (!CCS_StartAcquisition(m_sensorId)) {
        return reportSdkFailure("CCS_StartAcquisition");
    }

    m_currentTriggerMode = static_cast<int>(triggerMode);
    m_acquiring = true;
    emit acquisitionStateChanged(m_sensorId, true);
    reportLog(QString("Sensor %1 acquisition started").arg(m_sensorId));
    return true;
}

bool ColorFocusControl::StopAcquisition_CCS()
{
    if (m_pollTimer) {
        m_pollTimer->stop();
    }
    if (!CCS_StopAcquisition(m_sensorId)) {
        return reportSdkFailure("CCS_StopAcquisition");
    }
    m_acquiring = m_readThreadRunning.load();
    if (!m_acquiring) {
        emit acquisitionStateChanged(m_sensorId, false);
    }
    reportLog(QString("Sensor %1 acquisition stopped").arg(m_sensorId));
    return true;
}

bool ColorFocusControl::ClearDataStack()
{
    if (!ensureConnected("ClearDataStack")) {
        return false;
    }
    if (!CCS_ClearDataStack(m_sensorId)) {
        return reportSdkFailure("CCS_ClearDataStack");
    }
    m_distanceValues.clear();
    m_distanceValueMap.clear();
    m_encoders.clear();
    reportLog(QString("Sensor %1 data stack cleared").arg(m_sensorId));
    return true;
}

bool ColorFocusControl::InitAcquisitionEvent(int bufferLength)
{
    if (!ensureConnected("InitAcquisitionEvent")) {
        return false;
    }
    if (bufferLength <= 0) {
        m_lastErrorMessage = QString("InitAcquisitionEvent failed. invalid buffer length %1").arg(bufferLength);
        emit errorOccurred(m_sensorId, m_lastErrorMessage);
        emit s_sendErrorMsg(m_sensorId, m_lastErrorMessage);
        reportLog(m_lastErrorMessage);
        return false;
    }

    closeEventHandle();
    m_endAcqEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!m_endAcqEvent) {
        m_lastErrorMessage = QString("CreateEvent failed. winError=%1").arg(GetLastError());
        emit errorOccurred(m_sensorId, m_lastErrorMessage);
        emit s_sendErrorMsg(m_sensorId, m_lastErrorMessage);
        reportLog(m_lastErrorMessage);
        return false;
    }

    if (!CCS_InitAcquisitionEvent(m_sensorId, m_endAcqEvent, static_cast<DWORD>(bufferLength))) {
        closeEventHandle();
        return reportSdkFailure("CCS_InitAcquisitionEvent");
    }
    reportLog(QString("Sensor %1 acquisition event initialized").arg(m_sensorId));
    return true;
}

bool ColorFocusControl::ReadDistanceBuffer(int readNum, int triggerAxis)
{
    if (!ensureConnected("ReadDistanceBuffer")) {
        return false;
    }
    if (readNum <= 0) {
        m_lastErrorMessage = QString("ReadDistanceBuffer failed. invalid read num %1").arg(readNum);
        emit errorOccurred(m_sensorId, m_lastErrorMessage);
        emit s_sendErrorMsg(m_sensorId, m_lastErrorMessage);
        reportLog(m_lastErrorMessage);
        return false;
    }

    QVector<float> distances(readNum);
    QVector<float> intensities(readNum);
    QVector<INT32> encoder1(readNum);
    QVector<INT32> encoder2(readNum);
    QVector<INT32> encoder3(readNum);
    UINT actualCount = 0;
    if (!CCS_GetDistanceArray(m_sensorId,
                              distances.data(),
                              intensities.data(),
                              encoder1.data(),
                              encoder2.data(),
                              encoder3.data(),
                              static_cast<UINT>(readNum),
                              &actualCount)) {
        return reportSdkFailure("CCS_GetDistanceArray");
    }
    if (actualCount == 0) {
        m_lastErrorMessage = "CCS_GetDistanceArray returned zero points";
        emit errorOccurred(m_sensorId, m_lastErrorMessage);
        emit s_sendErrorMsg(m_sensorId, m_lastErrorMessage);
        reportLog(m_lastErrorMessage);
        return false;
    }

    m_distanceValues.clear();
    m_distanceValueMap.clear();
    m_encoders.clear();
    for (UINT i = 0; i < actualCount; ++i) {
        _DISTANCE_VALUE value;
        value.distance = distances.at(static_cast<int>(i));
        value.intensity = intensities.at(static_cast<int>(i));
        value.Encoder1 = encoder1.at(static_cast<int>(i));
        value.Encoder2 = encoder2.at(static_cast<int>(i));
        value.Encoder3 = encoder3.at(static_cast<int>(i));
        const INT32 encoder = triggerAxis == 1 ? value.Encoder2 : (triggerAxis == 2 ? value.Encoder3 : value.Encoder1);
        m_distanceValues.append(value);
        m_distanceValueMap.insert(encoder, value);
        m_encoders.append(encoder);
    }

    emit bufferedDataUpdated(m_sensorId, static_cast<int>(actualCount));
    reportLog(QString("Sensor %1 buffered points=%2").arg(m_sensorId).arg(actualCount));
    return true;
}

bool ColorFocusControl::CloseAcquisitionEvent()
{
    closeEventHandle();
    reportLog(QString("Sensor %1 acquisition event closed").arg(m_sensorId));
    return true;
}

bool ColorFocusControl::CloseAcquisitionEvent(int bufferLength, int triggerAxis)
{
    const bool ok = ReadDistanceBuffer(bufferLength, triggerAxis);
    closeEventHandle();
    return ok;
}

bool ColorFocusControl::ChangeTriggerMode(TriggerMode mode)
{
    return SetTriggerMode(static_cast<int>(mode));
}

bool ColorFocusControl::ApplyBasicParameters(int measureMode,
                                             int scanRate,
                                             int triggerMode,
                                             int triggerEdge,
                                             int burstNum,
                                             int sensitivity,
                                             bool autoSensitivity,
                                             int smoothingFactor)
{
    if (!ensureConnected("ApplyBasicParameters")) {
        return false;
    }

    bool restartNeeded = false;
    if (!pauseAcquisitionForParameterChange(&restartNeeded)) {
        return false;
    }

    bool ok = SetMeasureMode(measureMode)
              && SetScanRate(scanRate)
              && SetTriggerMode(triggerMode)
              && SetTriggerEdge(triggerEdge)
              && SetBurstNum(burstNum)
              && SetSensitivity(sensitivity)
              && SetAutoSensitivityEnabled(autoSensitivity)
              && SetSmoothingFactor(smoothingFactor);

    if (!resumeAcquisitionAfterParameterChange(restartNeeded)) {
        ok = false;
    }
    if (ok) {
        reportLog(QString("Sensor %1 parameters applied").arg(m_sensorId));
    }
    return ok;
}

bool ColorFocusControl::SetMeasureMode(int value)
{
    if (!ensureConnected("SetMeasureMode")) return false;
    if (!CCS_SetMeasureMode(m_sensorId, static_cast<DWORD>(value))) return reportSdkFailure("CCS_SetMeasureMode");
    return true;
}

bool ColorFocusControl::SetScanRate(int value)
{
    if (!ensureConnected("SetScanRate")) return false;
    if (!CCS_SetScanRate(m_sensorId, static_cast<DWORD>(value))) return reportSdkFailure("CCS_SetScanRate");
    reportLog(QString("Sensor %1 scanRate=%2").arg(m_sensorId).arg(value));
    return true;
}

bool ColorFocusControl::SetTriggerMode(int value)
{
    if (!ensureConnected("SetTriggerMode")) return false;
    if (!CCS_SetTriggerMode(m_sensorId, static_cast<DWORD>(value))) return reportSdkFailure("CCS_SetTriggerMode");
    m_currentTriggerMode = value;
    return true;
}

bool ColorFocusControl::SetTriggerEdge(int value)
{
    if (!ensureConnected("SetTriggerEdge")) return false;
    if (!CCS_SetTriggerEdge(m_sensorId, static_cast<DWORD>(value))) return reportSdkFailure("CCS_SetTriggerEdge");
    return true;
}

bool ColorFocusControl::SetBurstNum(int value)
{
    if (!ensureConnected("SetBurstNum")) return false;
    if (!CCS_SetBurstNum(m_sensorId, static_cast<DWORD>(value))) return reportSdkFailure("CCS_SetBurstNum");
    return true;
}

bool ColorFocusControl::SetSensitivity(int value)
{
    if (!ensureConnected("SetSensitivity")) return false;
    if (!CCS_SetSensitivity(m_sensorId, static_cast<DWORD>(value))) return reportSdkFailure("CCS_SetSensitivity");
    return true;
}

bool ColorFocusControl::SetAutoSensitivityEnabled(bool enabled)
{
    if (!ensureConnected("SetAutoSensitivityEnabled")) return false;
    if (!CCS_SetAutoSensitivityEn(m_sensorId, enabled ? 1 : 0)) return reportSdkFailure("CCS_SetAutoSensitivityEn");
    return true;
}

bool ColorFocusControl::SetSmoothingFactor(int value)
{
    if (!ensureConnected("SetSmoothingFactor")) return false;
    if (!CCS_SetSmoothingFactor(m_sensorId, static_cast<DWORD>(value))) return reportSdkFailure("CCS_SetSmoothingFactor");
    return true;
}

bool ColorFocusControl::SetSinglePointSmoothingFactor(int value)
{
    if (!ensureConnected("SetSinglePointSmoothingFactor")) return false;
#ifdef CCS_SDK_V330
    Q_UNUSED(value);
    m_lastErrorMessage = "SetSinglePointSmoothingFactor is not supported by DLL_CCS SDK V3.30";
    emit errorOccurred(m_sensorId, m_lastErrorMessage);
    emit s_sendErrorMsg(m_sensorId, m_lastErrorMessage);
    reportLog(m_lastErrorMessage);
    return false;
#else
    if (!CCS_SetSinglePointSmoothingFactor(m_sensorId, static_cast<DWORD>(value))) return reportSdkFailure("CCS_SetSinglePointSmoothingFactor");
    return true;
#endif
}

bool ColorFocusControl::SetRefractive(float value)
{
    if (!ensureConnected("SetRefractive")) return false;
    if (!CCS_SetRefractive(m_sensorId, value)) return reportSdkFailure("CCS_SetRefractive");
    return true;
}

bool ColorFocusControl::SetHoldLastValue(bool enabled)
{
    if (!ensureConnected("SetHoldLastValue")) return false;
    if (!CCS_SetHoldLastVal(m_sensorId, enabled ? 1 : 0)) return reportSdkFailure("CCS_SetHoldLastVal");
    return true;
}

bool ColorFocusControl::SetOpticalPen(int value)
{
    if (!ensureConnected("SetOpticalPen")) return false;
    if (!CCS_SetOpticalPen(m_sensorId, static_cast<DWORD>(value))) return reportSdkFailure("CCS_SetOpticalPen");
    return true;
}

bool ColorFocusControl::SetValueUnit(int value)
{
    if (!ensureConnected("SetValueUnit")) return false;
    if (!CCS_SetValueUnit(m_sensorId, static_cast<DWORD>(value))) return reportSdkFailure("CCS_SetValueUnit");
    return true;
}

bool ColorFocusControl::SetMirrorState(int value)
{
    if (!ensureConnected("SetMirrorState")) return false;
    if (!CCS_SetMirrorState(m_sensorId, static_cast<DWORD>(value))) return reportSdkFailure("CCS_SetMirrorState");
    return true;
}

bool ColorFocusControl::SetLightState(bool enabled)
{
    if (!ensureConnected("SetLightState")) return false;
    if (!CCS_SetLightState(m_sensorId, enabled ? 1 : 0)) return reportSdkFailure("CCS_SetLightState");
    return true;
}

bool ColorFocusControl::SetLightMode(int value)
{
    if (!ensureConnected("SetLightMode")) return false;
    if (!CCS_SetLightMode(m_sensorId, static_cast<DWORD>(value))) return reportSdkFailure("CCS_SetLightMode");
    return true;
}

bool ColorFocusControl::SetEncoderTriggerParam(int head, int tail, int interval, int encoderSelect, int direction)
{
    if (!ensureConnected("SetEncoderTriggerParam")) return false;
    if (!CCS_SetEcdTrgParam(m_sensorId,
                            static_cast<INT32>(head),
                            static_cast<INT32>(tail),
                            static_cast<INT32>(interval),
                            static_cast<BYTE>(encoderSelect),
                            static_cast<BYTE>(direction))) {
        return reportSdkFailure("CCS_SetEcdTrgParam");
    }
    return true;
}

bool ColorFocusControl::SetEcdTrgParam(INT32 head, INT32 tail, INT32 interval, BYTE encoderSelect)
{
    return SetEncoderTriggerParam(head, tail, interval, encoderSelect, 1);
}

bool ColorFocusControl::GetEcdTrgParam(PINT32 head, PINT32 tail, PINT32 interval, PBYTE encoderSelect, PBYTE direction)
{
    return GetEncoderTriggerParam(head, tail, interval, encoderSelect, direction);
}

bool ColorFocusControl::SetDAParam(float signalLowerLimit,
                                   float signalUpperLimit,
                                   float voltageLowerLimit,
                                   float voltageUpperLimit,
                                   float invalidValueVoltage)
{
    if (!ensureConnected("SetDAParam")) return false;
#ifdef CCS_SDK_V330
    Q_UNUSED(signalLowerLimit);
    Q_UNUSED(signalUpperLimit);
    Q_UNUSED(voltageLowerLimit);
    Q_UNUSED(voltageUpperLimit);
    Q_UNUSED(invalidValueVoltage);
    m_lastErrorMessage = "SetDAParam is not supported by DLL_CCS SDK V3.30";
    emit errorOccurred(m_sensorId, m_lastErrorMessage);
    emit s_sendErrorMsg(m_sensorId, m_lastErrorMessage);
    reportLog(m_lastErrorMessage);
    return false;
#else
    if (!CCS_SetDAParam(m_sensorId, signalLowerLimit, signalUpperLimit, voltageLowerLimit, voltageUpperLimit, invalidValueVoltage)) {
        return reportSdkFailure("CCS_SetDAParam");
    }
    return true;
#endif
}

bool ColorFocusControl::Dark()
{
    if (!ensureConnected("Dark")) return false;
    bool restartNeeded = false;
    if (!pauseAcquisitionForParameterChange(&restartNeeded)) return false;
    bool ok = true;
    if (!CCS_Dark(m_sensorId)) ok = reportSdkFailure("CCS_Dark");
    if (!resumeAcquisitionAfterParameterChange(restartNeeded)) ok = false;
    if (ok) reportLog(QString("Sensor %1 dark finished").arg(m_sensorId));
    return ok;
}

bool ColorFocusControl::RecenterEncoder1()
{
    if (!ensureConnected("RecenterEncoder1")) return false;
    if (!CCS_RecenterEncoder1(m_sensorId)) return reportSdkFailure("CCS_RecenterEncoder1");
    reportLog(QString("Sensor %1 encoder1 recentered").arg(m_sensorId));
    return true;
}

bool ColorFocusControl::RecenterEncoder2()
{
    if (!ensureConnected("RecenterEncoder2")) return false;
    if (!CCS_RecenterEncoder2(m_sensorId)) return reportSdkFailure("CCS_RecenterEncoder2");
    reportLog(QString("Sensor %1 encoder2 recentered").arg(m_sensorId));
    return true;
}

bool ColorFocusControl::RecenterEncoder3()
{
    if (!ensureConnected("RecenterEncoder3")) return false;
    if (!CCS_RecenterEncoder3(m_sensorId)) return reportSdkFailure("CCS_RecenterEncoder3");
    reportLog(QString("Sensor %1 encoder3 recentered").arg(m_sensorId));
    return true;
}

bool ColorFocusControl::RecenterAllEncoders()
{
    return RecenterEncoder1() && RecenterEncoder2() && RecenterEncoder3();
}

bool ColorFocusControl::ParamSave()
{
    if (!ensureConnected("ParamSave")) return false;
    if (!CCS_ParamSave(m_sensorId)) return reportSdkFailure("CCS_ParamSave");
    reportLog(QString("Sensor %1 params saved").arg(m_sensorId));
    return true;
}

bool ColorFocusControl::ParamDefault()
{
    if (!ensureConnected("ParamDefault")) return false;
    if (!CCS_ParamDefault(m_sensorId)) return reportSdkFailure("CCS_ParamDefault");
    reportLog(QString("Sensor %1 params restored to default").arg(m_sensorId));
    return true;
}

bool ColorFocusControl::StartSpectrum()
{
    if (!ensureConnected("StartSpectrum")) return false;
    if (!CCS_StartSpectrum(m_sensorId)) return reportSdkFailure("CCS_StartSpectrum");
    reportLog(QString("Sensor %1 spectrum started").arg(m_sensorId));
    return true;
}

bool ColorFocusControl::ReadSpectrum()
{
    if (!ensureConnected("ReadSpectrum")) return false;
    QVector<quint16> spectrum(600);
    UINT actualCount = 0;
    if (!CCS_GetSpectrumData(m_sensorId, reinterpret_cast<UINT16 *>(spectrum.data()), &actualCount)) {
        return reportSdkFailure("CCS_GetSpectrumData");
    }
    spectrum.resize(static_cast<int>(actualCount));
    emit spectrumUpdated(m_sensorId, spectrum);
    reportLog(QString("Sensor %1 spectrum points=%2").arg(m_sensorId).arg(actualCount));
    return true;
}

bool ColorFocusControl::SetWaveType(int value)
{
    if (!ensureConnected("SetWaveType")) return false;
    if (!CCS_SetWaveType(m_sensorId, static_cast<DWORD>(value))) return reportSdkFailure("CCS_SetWaveType");
    return true;
}

bool ColorFocusControl::SendSoftwareTrigger()
{
    if (!ensureConnected("SendSoftwareTrigger")) return false;
    if (!CCS_SendSoftwareTriggerCmd(m_sensorId)) return reportSdkFailure("CCS_SendSoftwareTriggerCmd");
    reportLog(QString("Sensor %1 software trigger sent").arg(m_sensorId));
    return true;
}

bool ColorFocusControl::GetMeasureMode(DWORD *value) { if (!ensureConnected("GetMeasureMode")) return false; if (!CCS_GetMeasureMode(m_sensorId, value)) return reportSdkFailure("CCS_GetMeasureMode"); return true; }
bool ColorFocusControl::GetScanRate(DWORD *value) { if (!ensureConnected("GetScanRate")) return false; if (!CCS_GetScanRate(m_sensorId, value)) return reportSdkFailure("CCS_GetScanRate"); return true; }
bool ColorFocusControl::GetTriggerMode(DWORD *value) { if (!ensureConnected("GetTriggerMode")) return false; if (!CCS_GetTriggerMode(m_sensorId, value)) return reportSdkFailure("CCS_GetTriggerMode"); return true; }
bool ColorFocusControl::GetTriggerEdge(DWORD *value) { if (!ensureConnected("GetTriggerEdge")) return false; if (!CCS_GetTriggerEdge(m_sensorId, value)) return reportSdkFailure("CCS_GetTriggerEdge"); return true; }
bool ColorFocusControl::GetBurstNum(DWORD *value) { if (!ensureConnected("GetBurstNum")) return false; if (!CCS_GetBurstNum(m_sensorId, value)) return reportSdkFailure("CCS_GetBurstNum"); return true; }
bool ColorFocusControl::GetSensitivity(DWORD *value) { if (!ensureConnected("GetSensitivity")) return false; if (!CCS_GetSensitivity(m_sensorId, value)) return reportSdkFailure("CCS_GetSensitivity"); return true; }
bool ColorFocusControl::GetAutoSensitivityEnabled(DWORD *value) { if (!ensureConnected("GetAutoSensitivityEnabled")) return false; if (!CCS_GetAutoSensitivityEn(m_sensorId, value)) return reportSdkFailure("CCS_GetAutoSensitivityEn"); return true; }
bool ColorFocusControl::GetSmoothingFactor(DWORD *value) { if (!ensureConnected("GetSmoothingFactor")) return false; if (!CCS_GetSmoothingFactor(m_sensorId, value)) return reportSdkFailure("CCS_GetSmoothingFactor"); return true; }
bool ColorFocusControl::GetSinglePointSmoothingFactor(DWORD *value)
{
    if (!ensureConnected("GetSinglePointSmoothingFactor")) return false;
#ifdef CCS_SDK_V330
    Q_UNUSED(value);
    m_lastErrorMessage = "GetSinglePointSmoothingFactor is not supported by DLL_CCS SDK V3.30";
    emit errorOccurred(m_sensorId, m_lastErrorMessage);
    emit s_sendErrorMsg(m_sensorId, m_lastErrorMessage);
    reportLog(m_lastErrorMessage);
    return false;
#else
    if (!CCS_GetSinglePointSmoothingFactor(m_sensorId, value)) return reportSdkFailure("CCS_GetSinglePointSmoothingFactor");
    return true;
#endif
}
bool ColorFocusControl::GetRefractive(float *value) { if (!ensureConnected("GetRefractive")) return false; if (!CCS_GetRefractive(m_sensorId, value)) return reportSdkFailure("CCS_GetRefractive"); return true; }
bool ColorFocusControl::GetHoldLastValue(DWORD *value) { if (!ensureConnected("GetHoldLastValue")) return false; if (!CCS_GetHoldLastVal(m_sensorId, value)) return reportSdkFailure("CCS_GetHoldLastVal"); return true; }
bool ColorFocusControl::GetOpticalPen(DWORD *value) { if (!ensureConnected("GetOpticalPen")) return false; if (!CCS_GetOpticalPen(m_sensorId, value)) return reportSdkFailure("CCS_GetOpticalPen"); return true; }
bool ColorFocusControl::GetScale(DWORD *value) { if (!ensureConnected("GetScale")) return false; if (!CCS_GetScale(m_sensorId, value)) return reportSdkFailure("CCS_GetScale"); return true; }
bool ColorFocusControl::GetValueUnit(DWORD *value) { if (!ensureConnected("GetValueUnit")) return false; if (!CCS_GetValueUnit(m_sensorId, value)) return reportSdkFailure("CCS_GetValueUnit"); return true; }
bool ColorFocusControl::GetMirrorState(DWORD *value) { if (!ensureConnected("GetMirrorState")) return false; if (!CCS_GetMirrorState(m_sensorId, value)) return reportSdkFailure("CCS_GetMirrorState"); return true; }
bool ColorFocusControl::GetLightState(DWORD *value) { if (!ensureConnected("GetLightState")) return false; if (!CCS_GetLightState(m_sensorId, value)) return reportSdkFailure("CCS_GetLightState"); return true; }
bool ColorFocusControl::GetLightMode(DWORD *value) { if (!ensureConnected("GetLightMode")) return false; if (!CCS_GetLightMode(m_sensorId, value)) return reportSdkFailure("CCS_GetLightMode"); return true; }

bool ColorFocusControl::GetEncoderTriggerParam(INT32 *head, INT32 *tail, INT32 *interval, BYTE *encoderSelect, BYTE *direction)
{
    if (!ensureConnected("GetEncoderTriggerParam")) return false;
    if (!CCS_GetEcdTrgParam(m_sensorId, head, tail, interval, encoderSelect, direction)) return reportSdkFailure("CCS_GetEcdTrgParam");
    return true;
}

bool ColorFocusControl::GetDAParam(float *signalLowerLimit,
                                   float *signalUpperLimit,
                                   float *voltageLowerLimit,
                                   float *voltageUpperLimit,
                                   float *invalidValueVoltage)
{
    if (!ensureConnected("GetDAParam")) return false;
#ifdef CCS_SDK_V330
    Q_UNUSED(signalLowerLimit);
    Q_UNUSED(signalUpperLimit);
    Q_UNUSED(voltageLowerLimit);
    Q_UNUSED(voltageUpperLimit);
    Q_UNUSED(invalidValueVoltage);
    m_lastErrorMessage = "GetDAParam is not supported by DLL_CCS SDK V3.30";
    emit errorOccurred(m_sensorId, m_lastErrorMessage);
    emit s_sendErrorMsg(m_sensorId, m_lastErrorMessage);
    reportLog(m_lastErrorMessage);
    return false;
#else
    if (!CCS_GetDAParam(m_sensorId, signalLowerLimit, signalUpperLimit, voltageLowerLimit, voltageUpperLimit, invalidValueVoltage)) {
        return reportSdkFailure("CCS_GetDAParam");
    }
    return true;
#endif
}

bool ColorFocusControl::GetDarkState(DWORD *value) { if (!ensureConnected("GetDarkState")) return false; if (!CCS_GetDarkState(m_sensorId, value)) return reportSdkFailure("CCS_GetDarkState"); return true; }
bool ColorFocusControl::GetDigitalInput(DWORD *value) { if (!ensureConnected("GetDigitalInput")) return false; if (!CCS_Digital_IO_Get_Input(m_sensorId, value)) return reportSdkFailure("CCS_Digital_IO_Get_Input"); return true; }
bool ColorFocusControl::SetDigitalOutput(DWORD value) { if (!ensureConnected("SetDigitalOutput")) return false; if (!CCS_Digital_IO_Set_Output(m_sensorId, value)) return reportSdkFailure("CCS_Digital_IO_Set_Output"); return true; }
bool ColorFocusControl::GetDigitalOutput(DWORD *value) { if (!ensureConnected("GetDigitalOutput")) return false; if (!CCS_Digital_IO_Get_Output(m_sensorId, value)) return reportSdkFailure("CCS_Digital_IO_Get_Output"); return true; }
bool ColorFocusControl::GetFirmwareVersion(DWORD *value) { if (!ensureConnected("GetFirmwareVersion")) return false; if (!CCS_GetFirmwareVersion(m_sensorId, value)) return reportSdkFailure("CCS_GetFirmwareVersion"); return true; }
bool ColorFocusControl::GetProductionDate(DWORD *value) { if (!ensureConnected("GetProductionDate")) return false; if (!CCS_Get_Production_Date(m_sensorId, value)) return reportSdkFailure("CCS_Get_Production_Date"); return true; }
bool ColorFocusControl::GetSerial(DWORD *value) { if (!ensureConnected("GetSerial")) return false; if (!CCS_Get_Serial(m_sensorId, value)) return reportSdkFailure("CCS_Get_Serial"); return true; }
bool ColorFocusControl::GetHardwareVersion(DWORD *value) { if (!ensureConnected("GetHardwareVersion")) return false; if (!CCS_Get_Hardware_Ver(m_sensorId, value)) return reportSdkFailure("CCS_Get_Hardware_Ver"); return true; }

double ColorFocusControl::GetCurrentDistance()
{
    float distance = 0.0f;
    float intensity = 0.0f;
    if (!ensureConnected("GetCurrentDistance")) {
        return 0.0;
    }
    if (!CCS_GetCurrentDistanceData(m_sensorId, &distance, &intensity)) {
        reportSdkFailure("CCS_GetCurrentDistanceData");
        return 0.0;
    }
    if (!validLatestSample(distance, intensity)) {
        m_hasLatestSample.store(false);
        m_lastErrorMessage = QString("CCS_GetCurrentDistanceData returned invalid sample. sensor=%1 distance=%2 intensity=%3")
                                 .arg(m_sensorId)
                                 .arg(distance)
                                 .arg(intensity);
        emit errorOccurred(m_sensorId, m_lastErrorMessage);
        emit s_sendErrorMsg(m_sensorId, m_lastErrorMessage);
        reportLog(m_lastErrorMessage);
        return 0.0;
    }
    m_latestDistance.store(distance);
    m_latestIntensity.store(intensity);
    m_hasLatestSample.store(true);
    emit s_colorFocusUpdated(m_sensorId, distance, intensity);
    return distance;
}

bool ColorFocusControl::GetLatestSample(float *distance, float *intensity) const
{
    if (!distance || !intensity) {
        return false;
    }
    if (!m_hasLatestSample.load()) {
        return false;
    }
    const float latestDistance = m_latestDistance.load();
    const float latestIntensity = m_latestIntensity.load();
    if (!validLatestSample(latestDistance, latestIntensity)) {
        return false;
    }
    *distance = latestDistance;
    *intensity = latestIntensity;
    return true;
}

INT32 ColorFocusControl::ChangeToEncoderValue(float position) const
{
    return static_cast<INT32>(qRound(position * 2000.0f));
}

float ColorFocusControl::ChangeToPositionValue(INT32 encoder) const
{
    return qRound(encoder / 2000.0f * 100.0f) / 100.0f;
}

void ColorFocusControl::ClearMeasureData()
{
    m_distanceValues.clear();
    m_distanceValueMap.clear();
    m_encoders.clear();
}

const QList<_DISTANCE_VALUE> &ColorFocusControl::distanceValues() const
{
    return m_distanceValues;
}

const QMap<INT32, _DISTANCE_VALUE> &ColorFocusControl::distanceValueMap() const
{
    return m_distanceValueMap;
}

const QVector<int> &ColorFocusControl::encoders() const
{
    return m_encoders;
}

void ColorFocusControl::PollCurrentSample()
{
    float distance = 0.0f;
    float intensity = 0.0f;
    if (!CCS_GetCurrentDistanceData(m_sensorId, &distance, &intensity)) {
        reportSdkFailure("CCS_GetCurrentDistanceData");
        StopAcquisition();
        return;
    }
    emit sampleUpdated(m_sensorId, distance, intensity, 0, 0, 0);
    emit s_colorFocusUpdated(m_sensorId, distance, intensity);
}

void ColorFocusControl::RunDataRead()
{
    float currentDistance[1] = {0.0f};
    float currentIntensity[1] = {0.0f};

    while (m_readThreadRunning.load()) {
        if (CCS_GetCurrentDistanceData(m_sensorId, currentDistance, currentIntensity)) {
            const UINT16 sensorId = m_sensorId;
            const float distance = currentDistance[0];
            const float intensity = currentIntensity[0];
            if (!validLatestSample(distance, intensity)) {
                m_hasLatestSample.store(false);
                Sleep(static_cast<DWORD>(m_pollIntervalMs));
                continue;
            }

            m_latestDistance.store(distance);
            m_latestIntensity.store(intensity);
            m_hasLatestSample.store(true);

            QMetaObject::invokeMethod(this, [this, sensorId, distance, intensity]() {
                emit sampleUpdated(sensorId, distance, intensity, 0, 0, 0);
                emit s_colorFocusUpdated(sensorId, distance, intensity);
            }, Qt::QueuedConnection);
        } else {
            m_hasLatestSample.store(false);
            reportSdkFailure("CCS_GetCurrentDistanceData");
            break;
        }

        Sleep(static_cast<DWORD>(m_pollIntervalMs));
    }

    m_readThreadRunning = false;
    CCS_StopAcquisition(m_sensorId);
}

bool ColorFocusControl::ensureConnected(const QString &action)
{
    if (m_connected) {
        return true;
    }
    m_lastErrorMessage = QString("%1 failed. sensor=%2 is not connected").arg(action).arg(m_sensorId);
    emit errorOccurred(m_sensorId, m_lastErrorMessage);
    emit s_sendErrorMsg(m_sensorId, m_lastErrorMessage);
    reportLog(m_lastErrorMessage);
    return false;
}

bool ColorFocusControl::reportSdkFailure(const QString &action)
{
#ifdef CCS_SDK_V330
    m_lastErrorMessage = QString("%1 failed. sensor=%2 sdkErrorCode=unavailable in DLL_CCS SDK V3.30").arg(action).arg(m_sensorId);
#else
    const int sdkError = CCS_GetErrorCode(m_sensorId);
    m_lastErrorMessage = QString("%1 failed. sensor=%2 sdkError=%3").arg(action).arg(m_sensorId).arg(sdkError);
#endif
    m_connectFailed = true;
    emit errorOccurred(m_sensorId, m_lastErrorMessage);
    emit s_sendErrorMsg(m_sensorId, m_lastErrorMessage);
    emit connectionTimeout();
    reportLog(m_lastErrorMessage);
    return false;
}

void ColorFocusControl::reportLog(const QString &message)
{
    emit logMessage(message);
    emit s_writeLog(message);
}

void ColorFocusControl::closeEventHandle()
{
    if (m_endAcqEvent) {
        CloseHandle(m_endAcqEvent);
        m_endAcqEvent = nullptr;
    }
}

bool ColorFocusControl::pauseAcquisitionForParameterChange(bool *restartNeeded)
{
    *restartNeeded = m_acquiring;
    if (!m_acquiring) {
        return true;
    }
    return StopAcquisition();
}

bool ColorFocusControl::resumeAcquisitionAfterParameterChange(bool restartNeeded)
{
    if (!restartNeeded) {
        return true;
    }
    return StartAcquisition(m_currentTriggerMode, m_pollIntervalMs);
}
