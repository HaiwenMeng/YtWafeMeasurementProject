#include "dialog_setting.h"
#include "ui_dialog_setting.h"
#include <QFileDialog>
#include <QDebug>

DialogSetting::DialogSetting(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogSetting)
{
    ui->setupUi(this);

    initSettings();
}

DialogSetting::~DialogSetting()
{
    delete ui;
}

bool DialogSetting::initSettings()
{
    if(m_configManager.loadFromXml(m_currentSettings)) {
        loadSettingsToUi();
        ui->lineEdit_trim_size->setValidator(new QRegExpValidator(
            QRegExp("^-?\\d+\\.\\d+$"), ui->lineEdit_trim_size));
        QList<QLineEdit*> edits2 = ui->groupBox_2->findChildren<QLineEdit*>();
        for (QLineEdit* edit : edits2) {
            edit->setValidator(new QRegExpValidator(
                QRegExp("^-?\\d+\\.\\d+$"), edit));
        }
        QList<QLineEdit*> edits3 = ui->groupBox_3->findChildren<QLineEdit*>();
        for (QLineEdit* edit : edits3) {
            edit->setValidator(new QRegExpValidator(
                QRegExp("^-?\\d+\\.\\d+$"), edit));
        }
        QList<QLineEdit*> edits4 = ui->groupBox_4->findChildren<QLineEdit*>();
        for (QLineEdit* edit : edits4) {
            edit->setValidator(new QRegExpValidator(
                QRegExp("^-?\\d+\\.\\d+$"), edit));
        }
        QList<QLineEdit*> edits5 = ui->groupBox_5->findChildren<QLineEdit*>();
        for (QLineEdit* edit : edits5) {
            edit->setValidator(new QRegExpValidator(
                QRegExp("^-?\\d+\\.\\d+$"), edit));
        }
        return true;
    } else {
        return false;
    }
}

void DialogSetting::loadSettingsToUi()
{
    // 硬件设置
    ui->lineEdit_axis_ip->setText(m_currentSettings.axisIp);
    ui->lineEdit_axis_port->setText(m_currentSettings.axisPort);
    ui->lineEdit_colorFocus_ip_top->setText(m_currentSettings.colorFocusIpTop);
    ui->lineEdit_colorFocus_ip_bottom->setText(m_currentSettings.colorFocusIpBottom);
    ui->lineEdit_wid_ip->setText(m_currentSettings.widIp);

    //轴系预设位置
    ui->lineEdit_pos_load_x->setText(m_currentSettings.posLoadX);
    ui->lineEdit_pos_load_y->setText(m_currentSettings.posLoadY);
    ui->lineEdit_pos_wait_x_12->setText(m_currentSettings.posWaitX_12);
    ui->lineEdit_pos_wait_y_12->setText(m_currentSettings.posWaitY_12);
    ui->lineEdit_pos_wait_x_8->setText(m_currentSettings.posWaitX_8);
    ui->lineEdit_pos_wait_y_8->setText(m_currentSettings.posWaitY_8);
    ui->lineEdit_pos_wait_x_6->setText(m_currentSettings.posWaitX_6);
    ui->lineEdit_pos_wait_y_6->setText(m_currentSettings.posWaitY_6);

    ui->lineEdit_pos_standard_1_x->setText(m_currentSettings.posStandard1X);
    ui->lineEdit_pos_standard_1_y->setText(m_currentSettings.posStandard1Y);
    ui->lineEdit_pos_standard_2_x->setText(m_currentSettings.posStandard2X);
    ui->lineEdit_pos_standard_2_y->setText(m_currentSettings.posStandard2Y);
    ui->lineEdit_pos_standard_3_x->setText(m_currentSettings.posStandard3X);
    ui->lineEdit_pos_standard_3_y->setText(m_currentSettings.posStandard3Y);
    ui->lineEdit_pos_standard_4_x->setText(m_currentSettings.posStandard4X);
    ui->lineEdit_pos_standard_4_y->setText(m_currentSettings.posStandard4Y);

    // 轴系速度
    ui->lineEdit_velocity_measure->setText(m_currentSettings.velocityMeasure);
    ui->lineEdit_velocity_normal->setText(m_currentSettings.velocityNormal);

    // 共聚焦
    ui->lineEdit_scan_rate->setText(m_currentSettings.scanRate);
    ui->lineEdit_trigger_rate->setText(m_currentSettings.triggerRate);

    // 测量参数
    ui->lineEdit_trim_size->setText(m_currentSettings.trimSize);
    ui->lineEdit_measureFileDir->setText(m_currentSettings.measureFileDir);

    //校准参数
    ui->lineEdit_remind_Interval->setText(m_currentSettings.remindInterval);
    ui->lineEdit_z_default_123->setText(m_currentSettings.z_default_123);
    ui->lineEdit_z_default_4->setText(m_currentSettings.z_default_4);
    ui->lineEdit_standard_thickness_1->setText(m_currentSettings.standardThickness1);
    ui->lineEdit_standard_thickness_2->setText(m_currentSettings.standardThickness2);
    ui->lineEdit_standard_thickness_3->setText(m_currentSettings.standardThickness3);
    ui->lineEdit_standard_thickness_4->setText(m_currentSettings.standardThickness4);
    if (m_currentSettings.ChordLength.isNull() || m_currentSettings.ChordLength.isEmpty())
        m_currentSettings.ChordLength = "0";
    ui->doubleSpinBox_notch_width_->setValue(m_currentSettings.ChordLength.toDouble());
    if (m_currentSettings.IsUseStandard1550Flag.toInt() != 1)
    {
        ui->label_38->setText(u8"校准片1/2/3/4 Z轴高度（mm）");
        ui->label_39->setVisible(false);
        ui->lineEdit_z_default_4->setVisible(false);
    }
    ui->checkBox_save_zm->setChecked(m_currentSettings.IsSaveZM=="1");
    ui->checkBox_save_image->setChecked(m_currentSettings.IsSave3D=="1");
}

void DialogSetting::saveSettingsFromUi()
{
    // 硬件设置
    m_currentSettings.axisIp = ui->lineEdit_axis_ip->text();
    m_currentSettings.axisPort = ui->lineEdit_axis_port->text();
    m_currentSettings.colorFocusIpTop = ui->lineEdit_colorFocus_ip_top->text();
    m_currentSettings.colorFocusIpBottom = ui->lineEdit_colorFocus_ip_bottom->text();
    m_currentSettings.widIp = ui->lineEdit_wid_ip->text();

    //轴系预设位置
    m_currentSettings.posLoadX = ui->lineEdit_pos_load_x->text();
    m_currentSettings.posLoadY = ui->lineEdit_pos_load_y->text();
    m_currentSettings.posWaitX_12 = ui->lineEdit_pos_wait_x_12->text();
    m_currentSettings.posWaitY_12 = ui->lineEdit_pos_wait_y_12->text();
    m_currentSettings.posWaitX_8 = ui->lineEdit_pos_wait_x_8->text();
    m_currentSettings.posWaitY_8 = ui->lineEdit_pos_wait_y_8->text();
    m_currentSettings.posWaitX_6 = ui->lineEdit_pos_wait_x_6->text();
    m_currentSettings.posWaitY_6 = ui->lineEdit_pos_wait_y_6->text();

    m_currentSettings.posStandard1X = ui->lineEdit_pos_standard_1_x->text();
    m_currentSettings.posStandard1Y = ui->lineEdit_pos_standard_1_y->text();
    m_currentSettings.posStandard2X = ui->lineEdit_pos_standard_2_x->text();
    m_currentSettings.posStandard2Y = ui->lineEdit_pos_standard_2_y->text();
    m_currentSettings.posStandard3X = ui->lineEdit_pos_standard_3_x->text();
    m_currentSettings.posStandard3Y = ui->lineEdit_pos_standard_3_y->text();
    m_currentSettings.posStandard4X = ui->lineEdit_pos_standard_4_x->text();
    m_currentSettings.posStandard4Y = ui->lineEdit_pos_standard_4_y->text();

    // 轴系速度
    m_currentSettings.velocityMeasure = ui->lineEdit_velocity_measure->text();
    m_currentSettings.velocityNormal = ui->lineEdit_velocity_normal->text();

    //共聚焦
    m_currentSettings.scanRate =ui->lineEdit_scan_rate->text();
    m_currentSettings.triggerRate =ui->lineEdit_trigger_rate->text();

    //测量参数
    //m_currentSettings.trimSize = ui->lineEdit_trim_size->text();
    m_currentSettings.measureFileDir = ui->lineEdit_measureFileDir->text();

    //校准参数
    m_currentSettings.remindInterval = ui->lineEdit_remind_Interval->text();
    m_currentSettings.z_default_123 = ui->lineEdit_z_default_123->text();
    m_currentSettings.z_default_4 = ui->lineEdit_z_default_4->text();
    m_currentSettings.standardThickness1 = ui->lineEdit_standard_thickness_1->text();
    m_currentSettings.standardThickness2 = ui->lineEdit_standard_thickness_2->text();
    m_currentSettings.standardThickness3 = ui->lineEdit_standard_thickness_3->text();
    m_currentSettings.standardThickness4 = ui->lineEdit_standard_thickness_4->text(); 
    m_currentSettings.ChordLength = ui->doubleSpinBox_notch_width_->text();
    m_currentSettings.IsSaveZM = ui->checkBox_save_zm->isChecked() ? "1" : "0";
    m_currentSettings.IsSave3D = ui->checkBox_save_image->isChecked() ? "1" : "0";
}

void DialogSetting::saveToFileOnly()
{
    if(!m_configManager.saveToXml(m_currentSettings)) {
        // 处理保存失败
    }
}

void DialogSetting::on_pushButton_save_clicked()
{
    saveSettingsFromUi();
    if(!m_configManager.saveToXml(m_currentSettings)) {
        // 处理保存失败
    }
    // emit s_updateStandardThicknessToMainView();
    

    accept();
}


void DialogSetting::on_pushButton_cancel_clicked()
{
    loadSettingsToUi();
    close();
}


void DialogSetting::on_pushButton_selectDir_clicked()
{
    QString defaultDir;
    if(ui->lineEdit_measureFileDir->text().isEmpty()){
        defaultDir = QCoreApplication::applicationDirPath();
    }else{
        defaultDir = ui->lineEdit_measureFileDir->text();
    }

    QString folder = QFileDialog::getExistingDirectory(nullptr, u8"选择文件夹", defaultDir,
                                                       QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!folder.isEmpty()) {
        ui->lineEdit_measureFileDir->setText(folder);
    }
}

