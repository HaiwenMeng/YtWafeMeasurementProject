#ifndef XML_CONFIG_MANAGER_H
#define XML_CONFIG_MANAGER_H

#include "paramsettings.h"

#include <QDomDocument>
#include <QString>

class XmlConfigManager
{
public:
    XmlConfigManager();

    QString configPath() const;
    bool loadFromXml(ParamSettings *settings, QString *errorMessage = nullptr) const;
    bool loadFromXml(ParamSettings &settings) const;
    bool saveToXml(const ParamSettings &settings, QString *errorMessage = nullptr) const;

private:
    QDomElement createTextElement(QDomDocument *doc, const QString &tag, const QString &value) const;
    QString readText(const QDomElement &parent, const QString &tag) const;

private:
    QString m_configPath;
};

#endif // XML_CONFIG_MANAGER_H
