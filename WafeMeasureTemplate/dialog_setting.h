#ifndef DIALOG_SETTING_H
#define DIALOG_SETTING_H

#include <QDialog>
#include "paramsettings.h"
#include "xml_config_manager.h"

namespace Ui {
class DialogSetting;
}

class DialogSetting : public QDialog
{
    Q_OBJECT

public:
    explicit DialogSetting(QWidget *parent = nullptr);
    ~DialogSetting();

    inline ParamSettings& getSettings() { return m_currentSettings; }

    void saveToFileOnly();
    bool initSettings();
   
public slots:
    void on_pushButton_save_clicked();
    void on_pushButton_cancel_clicked();

signals:
    void s_updateStandardThicknessToMainView();

private slots:
    void on_pushButton_selectDir_clicked();

private:
    void loadSettingsToUi();
    void saveSettingsFromUi();

private:
    Ui::DialogSetting *ui;

    ParamSettings m_currentSettings;
    XmlConfigManager m_configManager;
};

#endif // DIALOG_SETTING_H
