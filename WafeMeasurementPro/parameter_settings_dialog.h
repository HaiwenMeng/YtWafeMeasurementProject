#ifndef WAFEMEASUREMENTPRO_PARAMETER_SETTINGS_DIALOG_H
#define WAFEMEASUREMENTPRO_PARAMETER_SETTINGS_DIALOG_H

#include "paramsettings.h"
#include "xml_config_manager.h"

#include <QDialog>

namespace Ui {
class ParameterSettingsDialog;
}

class ParameterSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ParameterSettingsDialog(QWidget *parent = nullptr);
    ~ParameterSettingsDialog() override;

    ParamSettings settings() const;
    bool reload(QString *errorMessage);

signals:
    void settingsSaved(const ParamSettings &settings);

private slots:
    void onSaveClicked();
    void onReloadClicked();

private:
    void setUiFromSettings(const ParamSettings &settings);
    ParamSettings readUiSettings() const;
    bool validate(const ParamSettings &settings, QString *errorMessage) const;
    bool isDoubleText(const QString &text) const;
    bool isIntText(const QString &text) const;
    void showError(const QString &message);

private:
    Ui::ParameterSettingsDialog *ui;
    XmlConfigManager m_config;
    ParamSettings m_settings;
};

#endif // WAFEMEASUREMENTPRO_PARAMETER_SETTINGS_DIALOG_H
