#include "xml_config_manager.h"

#include <QCoreApplication>
#include <QFile>
#include <QTextStream>

XmlConfigManager::XmlConfigManager()
    : m_configPath(QCoreApplication::applicationDirPath() + QStringLiteral("/config.xml"))
{
}

QString XmlConfigManager::configPath() const
{
    return m_configPath;
}

bool XmlConfigManager::loadFromXml(ParamSettings *settings, QString *errorMessage) const
{
    if (!settings) {
        if (errorMessage)
            *errorMessage = QStringLiteral("loadFromXml failed, settings is null.");
        return false;
    }

    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Failed to open config: %1, %2").arg(m_configPath, file.errorString());
        return false;
    }

    QDomDocument doc;
    QString parseError;
    int errorLine = 0;
    int errorColumn = 0;
    if (!doc.setContent(&file, &parseError, &errorLine, &errorColumn)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to parse config: %1 line=%2 column=%3 error=%4")
                .arg(m_configPath)
                .arg(errorLine)
                .arg(errorColumn)
                .arg(parseError);
        }
        return false;
    }

    const QDomElement root = doc.documentElement();
    if (root.tagName() != QStringLiteral("Settings")) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Invalid config root: %1").arg(root.tagName());
        return false;
    }

    const QDomElement hardware = root.firstChildElement(QStringLiteral("Hardware"));
    settings->axisIp = readText(hardware, QStringLiteral("AxisIP"));
    settings->axisPort = readText(hardware, QStringLiteral("AxisPort"));
    settings->colorFocusIpTop = readText(hardware, QStringLiteral("ColorFocusIpTop"));
    settings->colorFocusIpBottom = readText(hardware, QStringLiteral("ColorFocusIpBottom"));
    settings->widIp = readText(hardware, QStringLiteral("WidIp"));

    const QDomElement axisPreset = root.firstChildElement(QStringLiteral("AxisPresetPositions"));
    settings->posLoadX = readText(axisPreset, QStringLiteral("PosLoadX"));
    settings->posLoadY = readText(axisPreset, QStringLiteral("PosLoadY"));
    settings->posWaitX_12 = readText(axisPreset, QStringLiteral("PosWaitX_12"));
    settings->posWaitY_12 = readText(axisPreset, QStringLiteral("PosWaitY_12"));
    settings->posWaitX_8 = readText(axisPreset, QStringLiteral("PosWaitX_8"));
    settings->posWaitY_8 = readText(axisPreset, QStringLiteral("PosWaitY_8"));
    settings->posWaitX_6 = readText(axisPreset, QStringLiteral("PosWaitX_6"));
    settings->posWaitY_6 = readText(axisPreset, QStringLiteral("PosWaitY_6"));
    settings->posStandard1X = readText(axisPreset, QStringLiteral("PosStandard1X"));
    settings->posStandard1Y = readText(axisPreset, QStringLiteral("PosStandard1Y"));
    settings->posStandard2X = readText(axisPreset, QStringLiteral("PosStandard2X"));
    settings->posStandard2Y = readText(axisPreset, QStringLiteral("PosStandard2Y"));
    settings->posStandard3X = readText(axisPreset, QStringLiteral("PosStandard3X"));
    settings->posStandard3Y = readText(axisPreset, QStringLiteral("PosStandard3Y"));
    settings->posStandard4X = readText(axisPreset, QStringLiteral("PosStandard4X"));
    settings->posStandard4Y = readText(axisPreset, QStringLiteral("PosStandard4Y"));

    const QDomElement speed = root.firstChildElement(QStringLiteral("AxisSpeedSettings"));
    settings->velocityMeasure = readText(speed, QStringLiteral("MeasureSpeed"));
    settings->velocityNormal = readText(speed, QStringLiteral("NormalSpeed"));

    const QDomElement colorFocus = root.firstChildElement(QStringLiteral("ColorFocus"));
    settings->scanRate = readText(colorFocus, QStringLiteral("ScanRate"));
    settings->triggerRate = readText(colorFocus, QStringLiteral("TriggerRate"));

    const QDomElement measure = root.firstChildElement(QStringLiteral("Measure"));
    settings->trimSize = readText(measure, QStringLiteral("TrimSize"));
    settings->measureFileDir = readText(measure, QStringLiteral("MeasureFileDir"));

    const QDomElement calibration = root.firstChildElement(QStringLiteral("Calibration"));
    settings->remindInterval = readText(calibration, QStringLiteral("SemindInterval"));
    settings->z_default_123 = readText(calibration, QStringLiteral("ZDefault123"));
    settings->z_default_4 = readText(calibration, QStringLiteral("ZDefault4"));
    settings->standardThickness1 = readText(calibration, QStringLiteral("StandardThickness1"));
    settings->standardThickness2 = readText(calibration, QStringLiteral("StandardThickness2"));
    settings->standardThickness3 = readText(calibration, QStringLiteral("StandardThickness3"));
    settings->standardThickness4 = readText(calibration, QStringLiteral("StandardThickness4"));
    settings->standardTotalVal1 = readText(calibration, QStringLiteral("StandardTotalVal1"));
    settings->standardTotalVal2 = readText(calibration, QStringLiteral("StandardTotalVal2"));
    settings->standardTotalVal3 = readText(calibration, QStringLiteral("StandardTotalVal3"));
    settings->standardTotalVal4 = readText(calibration, QStringLiteral("StandardTotalVal4"));
    settings->lastCalTimeStamp_500_1500 = readText(calibration, QStringLiteral("LastCalTimeStamp_500_1500"));
    settings->lastCalTimeStamp_1550 = readText(calibration, QStringLiteral("lastCalTimeStamp_1550"));
    settings->CalibrationDirectionOfX = readText(calibration, QStringLiteral("CalibrationDirectionOfX"));
    settings->IsNewWarpAlg = readText(calibration, QStringLiteral("IsNewWarpAlg"));
    settings->IsNewBowAlg = readText(calibration, QStringLiteral("IsNewBowAlg"));
    settings->IsSaveZM = readText(calibration, QStringLiteral("IsSaveZM"));
    settings->IsSave3D = readText(calibration, QStringLiteral("IsSave3D"));
    settings->IsUseWID = readText(calibration, QStringLiteral("IsUseWID"));
    settings->LineSampleNum = readText(calibration, QStringLiteral("LineSampleNum"));
    settings->IsUseStandard1550Flag = readText(calibration, QStringLiteral("IsUseStandard1550Flag"));
    settings->ChordLength = readText(calibration, QStringLiteral("ChordLength"));
    return true;
}

bool XmlConfigManager::loadFromXml(ParamSettings &settings) const
{
    return loadFromXml(&settings, nullptr);
}

bool XmlConfigManager::saveToXml(const ParamSettings &settings, QString *errorMessage) const
{
    QDomDocument doc;
    QDomElement root = doc.createElement(QStringLiteral("Settings"));
    doc.appendChild(root);

    QDomElement hardware = doc.createElement(QStringLiteral("Hardware"));
    hardware.appendChild(createTextElement(&doc, QStringLiteral("AxisIP"), settings.axisIp));
    hardware.appendChild(createTextElement(&doc, QStringLiteral("AxisPort"), settings.axisPort));
    hardware.appendChild(createTextElement(&doc, QStringLiteral("ColorFocusIpTop"), settings.colorFocusIpTop));
    hardware.appendChild(createTextElement(&doc, QStringLiteral("ColorFocusIpBottom"), settings.colorFocusIpBottom));
    hardware.appendChild(createTextElement(&doc, QStringLiteral("WidIp"), settings.widIp));
    root.appendChild(hardware);

    QDomElement axisPreset = doc.createElement(QStringLiteral("AxisPresetPositions"));
    axisPreset.appendChild(createTextElement(&doc, QStringLiteral("PosLoadX"), settings.posLoadX));
    axisPreset.appendChild(createTextElement(&doc, QStringLiteral("PosLoadY"), settings.posLoadY));
    axisPreset.appendChild(createTextElement(&doc, QStringLiteral("PosWaitX_12"), settings.posWaitX_12));
    axisPreset.appendChild(createTextElement(&doc, QStringLiteral("PosWaitY_12"), settings.posWaitY_12));
    axisPreset.appendChild(createTextElement(&doc, QStringLiteral("PosWaitX_8"), settings.posWaitX_8));
    axisPreset.appendChild(createTextElement(&doc, QStringLiteral("PosWaitY_8"), settings.posWaitY_8));
    axisPreset.appendChild(createTextElement(&doc, QStringLiteral("PosWaitX_6"), settings.posWaitX_6));
    axisPreset.appendChild(createTextElement(&doc, QStringLiteral("PosWaitY_6"), settings.posWaitY_6));
    axisPreset.appendChild(createTextElement(&doc, QStringLiteral("PosStandard1X"), settings.posStandard1X));
    axisPreset.appendChild(createTextElement(&doc, QStringLiteral("PosStandard1Y"), settings.posStandard1Y));
    axisPreset.appendChild(createTextElement(&doc, QStringLiteral("PosStandard2X"), settings.posStandard2X));
    axisPreset.appendChild(createTextElement(&doc, QStringLiteral("PosStandard2Y"), settings.posStandard2Y));
    axisPreset.appendChild(createTextElement(&doc, QStringLiteral("PosStandard3X"), settings.posStandard3X));
    axisPreset.appendChild(createTextElement(&doc, QStringLiteral("PosStandard3Y"), settings.posStandard3Y));
    axisPreset.appendChild(createTextElement(&doc, QStringLiteral("PosStandard4X"), settings.posStandard4X));
    axisPreset.appendChild(createTextElement(&doc, QStringLiteral("PosStandard4Y"), settings.posStandard4Y));
    root.appendChild(axisPreset);

    QDomElement speed = doc.createElement(QStringLiteral("AxisSpeedSettings"));
    speed.appendChild(createTextElement(&doc, QStringLiteral("MeasureSpeed"), settings.velocityMeasure));
    speed.appendChild(createTextElement(&doc, QStringLiteral("NormalSpeed"), settings.velocityNormal));
    root.appendChild(speed);

    QDomElement colorFocus = doc.createElement(QStringLiteral("ColorFocus"));
    colorFocus.appendChild(createTextElement(&doc, QStringLiteral("ScanRate"), settings.scanRate));
    colorFocus.appendChild(createTextElement(&doc, QStringLiteral("TriggerRate"), settings.triggerRate));
    root.appendChild(colorFocus);

    QDomElement measure = doc.createElement(QStringLiteral("Measure"));
    measure.appendChild(createTextElement(&doc, QStringLiteral("TrimSize"), settings.trimSize));
    measure.appendChild(createTextElement(&doc, QStringLiteral("MeasureFileDir"), settings.measureFileDir));
    root.appendChild(measure);

    QDomElement calibration = doc.createElement(QStringLiteral("Calibration"));
    calibration.appendChild(createTextElement(&doc, QStringLiteral("SemindInterval"), settings.remindInterval));
    calibration.appendChild(createTextElement(&doc, QStringLiteral("ZDefault123"), settings.z_default_123));
    calibration.appendChild(createTextElement(&doc, QStringLiteral("ZDefault4"), settings.z_default_4));
    calibration.appendChild(createTextElement(&doc, QStringLiteral("StandardThickness1"), settings.standardThickness1));
    calibration.appendChild(createTextElement(&doc, QStringLiteral("StandardThickness2"), settings.standardThickness2));
    calibration.appendChild(createTextElement(&doc, QStringLiteral("StandardThickness3"), settings.standardThickness3));
    calibration.appendChild(createTextElement(&doc, QStringLiteral("StandardThickness4"), settings.standardThickness4));
    calibration.appendChild(createTextElement(&doc, QStringLiteral("StandardTotalVal1"), settings.standardTotalVal1));
    calibration.appendChild(createTextElement(&doc, QStringLiteral("StandardTotalVal2"), settings.standardTotalVal2));
    calibration.appendChild(createTextElement(&doc, QStringLiteral("StandardTotalVal3"), settings.standardTotalVal3));
    calibration.appendChild(createTextElement(&doc, QStringLiteral("StandardTotalVal4"), settings.standardTotalVal4));
    calibration.appendChild(createTextElement(&doc, QStringLiteral("LastCalTimeStamp_500_1500"), settings.lastCalTimeStamp_500_1500));
    calibration.appendChild(createTextElement(&doc, QStringLiteral("lastCalTimeStamp_1550"), settings.lastCalTimeStamp_1550));
    calibration.appendChild(createTextElement(&doc, QStringLiteral("CalibrationDirectionOfX"), settings.CalibrationDirectionOfX));
    calibration.appendChild(createTextElement(&doc, QStringLiteral("IsNewWarpAlg"), settings.IsNewWarpAlg));
    calibration.appendChild(createTextElement(&doc, QStringLiteral("IsNewBowAlg"), settings.IsNewBowAlg));
    calibration.appendChild(createTextElement(&doc, QStringLiteral("IsSaveZM"), settings.IsSaveZM));
    calibration.appendChild(createTextElement(&doc, QStringLiteral("IsSave3D"), settings.IsSave3D));
    calibration.appendChild(createTextElement(&doc, QStringLiteral("IsUseWID"), settings.IsUseWID));
    calibration.appendChild(createTextElement(&doc, QStringLiteral("LineSampleNum"), settings.LineSampleNum));
    calibration.appendChild(createTextElement(&doc, QStringLiteral("IsUseStandard1550Flag"), settings.IsUseStandard1550Flag));
    calibration.appendChild(createTextElement(&doc, QStringLiteral("ChordLength"), settings.ChordLength));
    root.appendChild(calibration);

    QFile file(m_configPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Failed to write config: %1, %2").arg(m_configPath, file.errorString());
        return false;
    }

    QTextStream stream(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    stream.setCodec("System");
#endif
    doc.save(stream, 4);
    return true;
}

QDomElement XmlConfigManager::createTextElement(QDomDocument *doc, const QString &tag, const QString &value) const
{
    QDomElement elem = doc->createElement(tag);
    elem.appendChild(doc->createTextNode(value));
    return elem;
}

QString XmlConfigManager::readText(const QDomElement &parent, const QString &tag) const
{
    return parent.firstChildElement(tag).text();
}
