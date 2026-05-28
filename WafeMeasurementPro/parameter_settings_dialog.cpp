#include "parameter_settings_dialog.h"
#include "ui_parameter_settings_dialog.h"

#include <QMessageBox>

ParameterSettingsDialog::ParameterSettingsDialog(QWidget *parent)
    : QDialog(parent),
      ui(new Ui::ParameterSettingsDialog)
{
    ui->setupUi(this);
    connect(ui->button_save, &QPushButton::clicked, this, &ParameterSettingsDialog::onSaveClicked);
    connect(ui->button_reload, &QPushButton::clicked, this, &ParameterSettingsDialog::onReloadClicked);
    connect(ui->button_cancel, &QPushButton::clicked, this, &ParameterSettingsDialog::reject);
    QString error;
    if (!reload(&error)) {
        showError(error);
    }
}

ParameterSettingsDialog::~ParameterSettingsDialog()
{
    delete ui;
}

ParamSettings ParameterSettingsDialog::settings() const
{
    return m_settings;
}

bool ParameterSettingsDialog::reload(QString *errorMessage)
{
    ParamSettings settings;
    if (!m_config.loadFromXml(&settings, errorMessage)) {
        return false;
    }
    m_settings = settings;
    setUiFromSettings(m_settings);
    return true;
}

void ParameterSettingsDialog::onSaveClicked()
{
    ParamSettings settings = readUiSettings();
    QString error;
    if (!validate(settings, &error)) {
        showError(error);
        return;
    }
    if (!m_config.saveToXml(settings, &error)) {
        showError(error);
        return;
    }
    m_settings = settings;
    emit settingsSaved(m_settings);
    accept();
}

void ParameterSettingsDialog::onReloadClicked()
{
    QString error;
    if (!reload(&error)) {
        showError(error);
    }
}

void ParameterSettingsDialog::setUiFromSettings(const ParamSettings &settings)
{
    ui->line_axis_ip->setText(settings.axisIp);
    ui->line_axis_port->setText(settings.axisPort);
    ui->line_top_ip->setText(settings.colorFocusIpTop);
    ui->line_bottom_ip->setText(settings.colorFocusIpBottom);
    ui->line_load_x->setText(settings.posLoadX);
    ui->line_load_y->setText(settings.posLoadY);
    ui->line_wait_x8->setText(settings.posWaitX_8);
    ui->line_wait_y8->setText(settings.posWaitY_8);
    ui->line_wait_x12->setText(settings.posWaitX_12);
    ui->line_wait_y12->setText(settings.posWaitY_12);
    ui->line_std1_x->setText(settings.posStandard1X);
    ui->line_std1_y->setText(settings.posStandard1Y);
    ui->line_std2_x->setText(settings.posStandard2X);
    ui->line_std2_y->setText(settings.posStandard2Y);
    ui->line_std3_x->setText(settings.posStandard3X);
    ui->line_std3_y->setText(settings.posStandard3Y);
    ui->line_std4_x->setText(settings.posStandard4X);
    ui->line_std4_y->setText(settings.posStandard4Y);
    ui->line_velocity_measure->setText(settings.velocityMeasure);
    ui->line_velocity_normal->setText(settings.velocityNormal);
    ui->line_scan_rate->setText(settings.scanRate);
    ui->line_trigger_rate->setText(settings.triggerRate);
    ui->line_z123->setText(settings.z_default_123);
    ui->line_z4->setText(settings.z_default_4);
    ui->line_standard1->setText(settings.standardThickness1);
    ui->line_standard2->setText(settings.standardThickness2);
    ui->line_standard3->setText(settings.standardThickness3);
    ui->line_standard4->setText(settings.standardThickness4);
    ui->line_total1->setText(settings.standardTotalVal1);
    ui->line_total2->setText(settings.standardTotalVal2);
    ui->line_total3->setText(settings.standardTotalVal3);
    ui->line_total4->setText(settings.standardTotalVal4);
    ui->line_trim->setText(settings.trimSize);
    ui->line_output_dir->setText(settings.measureFileDir);
    ui->line_line_sample->setText(settings.LineSampleNum);
    ui->line_chord->setText(settings.ChordLength);
}

ParamSettings ParameterSettingsDialog::readUiSettings() const
{
    ParamSettings settings = m_settings;
    settings.axisIp = ui->line_axis_ip->text().trimmed();
    settings.axisPort = ui->line_axis_port->text().trimmed();
    settings.colorFocusIpTop = ui->line_top_ip->text().trimmed();
    settings.colorFocusIpBottom = ui->line_bottom_ip->text().trimmed();
    settings.posLoadX = ui->line_load_x->text().trimmed();
    settings.posLoadY = ui->line_load_y->text().trimmed();
    settings.posWaitX_8 = ui->line_wait_x8->text().trimmed();
    settings.posWaitY_8 = ui->line_wait_y8->text().trimmed();
    settings.posWaitX_12 = ui->line_wait_x12->text().trimmed();
    settings.posWaitY_12 = ui->line_wait_y12->text().trimmed();
    settings.posStandard1X = ui->line_std1_x->text().trimmed();
    settings.posStandard1Y = ui->line_std1_y->text().trimmed();
    settings.posStandard2X = ui->line_std2_x->text().trimmed();
    settings.posStandard2Y = ui->line_std2_y->text().trimmed();
    settings.posStandard3X = ui->line_std3_x->text().trimmed();
    settings.posStandard3Y = ui->line_std3_y->text().trimmed();
    settings.posStandard4X = ui->line_std4_x->text().trimmed();
    settings.posStandard4Y = ui->line_std4_y->text().trimmed();
    settings.velocityMeasure = ui->line_velocity_measure->text().trimmed();
    settings.velocityNormal = ui->line_velocity_normal->text().trimmed();
    settings.scanRate = ui->line_scan_rate->text().trimmed();
    settings.triggerRate = ui->line_trigger_rate->text().trimmed();
    settings.z_default_123 = ui->line_z123->text().trimmed();
    settings.z_default_4 = ui->line_z4->text().trimmed();
    settings.standardThickness1 = ui->line_standard1->text().trimmed();
    settings.standardThickness2 = ui->line_standard2->text().trimmed();
    settings.standardThickness3 = ui->line_standard3->text().trimmed();
    settings.standardThickness4 = ui->line_standard4->text().trimmed();
    settings.standardTotalVal1 = ui->line_total1->text().trimmed();
    settings.standardTotalVal2 = ui->line_total2->text().trimmed();
    settings.standardTotalVal3 = ui->line_total3->text().trimmed();
    settings.standardTotalVal4 = ui->line_total4->text().trimmed();
    settings.trimSize = ui->line_trim->text().trimmed();
    settings.measureFileDir = ui->line_output_dir->text().trimmed();
    settings.LineSampleNum = ui->line_line_sample->text().trimmed();
    settings.ChordLength = ui->line_chord->text().trimmed();
    return settings;
}

bool ParameterSettingsDialog::validate(const ParamSettings &settings, QString *errorMessage) const
{
    if (settings.axisIp.isEmpty() || settings.axisPort.isEmpty() ||
        settings.colorFocusIpTop.isEmpty() || settings.colorFocusIpBottom.isEmpty()) {
        *errorMessage = QStringLiteral("Hardware IP or port is empty.");
        return false;
    }
    const QStringList doubleFields = {
        settings.posLoadX, settings.posLoadY, settings.posWaitX_8, settings.posWaitY_8,
        settings.posWaitX_12, settings.posWaitY_12,
        settings.posStandard1X, settings.posStandard1Y, settings.posStandard2X, settings.posStandard2Y,
        settings.posStandard3X, settings.posStandard3Y, settings.posStandard4X, settings.posStandard4Y,
        settings.velocityMeasure, settings.velocityNormal, settings.z_default_123, settings.z_default_4,
        settings.standardThickness1, settings.standardThickness2, settings.standardThickness3, settings.standardThickness4,
        settings.standardTotalVal1, settings.standardTotalVal2, settings.standardTotalVal3, settings.standardTotalVal4,
        settings.trimSize, settings.ChordLength
    };
    for (const QString &value : doubleFields) {
        if (!isDoubleText(value)) {
            *errorMessage = QStringLiteral("Numeric field is invalid: %1").arg(value);
            return false;
        }
    }
    if (!isIntText(settings.axisPort) || !isIntText(settings.scanRate) ||
        !isIntText(settings.triggerRate) || !isIntText(settings.LineSampleNum)) {
        *errorMessage = QStringLiteral("Integer field is invalid.");
        return false;
    }
    return true;
}

bool ParameterSettingsDialog::isDoubleText(const QString &text) const
{
    bool ok = false;
    text.toDouble(&ok);
    return ok;
}

bool ParameterSettingsDialog::isIntText(const QString &text) const
{
    bool ok = false;
    text.toInt(&ok);
    return ok;
}

void ParameterSettingsDialog::showError(const QString &message)
{
    QMessageBox::warning(this, QString(u8"≤Œ ˝…Ë÷√"), message);
}
