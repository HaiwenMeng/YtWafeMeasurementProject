#include "dialogmeasureparams.h"
#include "ui_dialogmeasureparams.h"
#include  <QSqlTableModel>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QSqlRecord>

DialogMeasureParams::DialogMeasureParams(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogMeasureParams)
{

      


    ui->setupUi(this);
}

DialogMeasureParams::~DialogMeasureParams()
{
    delete ui;
}

void DialogMeasureParams::on_pushButton_cancel_clicked()
{
    accept();
}


void DialogMeasureParams::on_pushButton_ok_clicked()
{
    UpdateParams(MeasureParamsId);
    reject();
}

void DialogMeasureParams::InitMeasureParams(QString recipeName)
{
 
    
    QSqlTableModel* m_model = new QSqlTableModel(this);
    m_model->setTable("recipes");
    m_model->setFilter(QString("recipename = '%1'").arg(recipeName));
    bool state=m_model->select();
    if (m_model->rowCount() > 0) {
        // 获取第一行
        auto record =m_model->record(0);
        BOWValue.MinValue =record.value("MinBOW").toDouble();
        BOWValue.MaxValue = record.value("MaxBOW").toDouble();
        BOWValue.IsInvalid = record.value("IsInvalidBOW").toBool();

        //ui->doubleSpinBox_minBOW->setValue(BOWValue.MinValue);
        //ui->doubleSpinBox_maxBOW->setValue(BOWValue.MaxValue);
        ui->checkBox_useBOW->setChecked(BOWValue.IsInvalid);

        WARPValue.MinValue = record.value("MinWARP").toDouble();
        WARPValue.MaxValue = record.value("MaxWARP").toDouble();
        WARPValue.IsInvalid = record.value("IsInvalidWARP").toBool();
        //ui->doubleSpinBox_minWARP->setValue(WARPValue.MinValue);
        //ui->doubleSpinBox_maxWARP->setValue(WARPValue.MaxValue);
        ui->checkBox_useWARP->setChecked(WARPValue.IsInvalid);

        CTHKValue.MinValue = record.value("MinCTHK").toDouble();
        CTHKValue.MaxValue = record.value("MaxCTHK").toDouble();
        CTHKValue.IsInvalid = record.value("IsInvalidCTHK").toBool();
        //ui->doubleSpinBox_minCTHK->setValue(CTHKValue.MinValue);
       // ui->doubleSpinBox_maxCTHK->setValue(CTHKValue.MaxValue);
        //ui->checkBox_useCTHK->setChecked(CTHKValue.IsInvalid);

        ATHKValue.MinValue = record.value("MinATHK").toDouble();
        ATHKValue.MaxValue = record.value("MaxATHK").toDouble();
        ATHKValue.IsInvalid = record.value("IsInvalidATHK").toBool();
        //ui->doubleSpinBox_minATHK->setValue(ATHKValue.MinValue);
        //ui->doubleSpinBox_maxATHK->setValue(ATHKValue.MaxValue);
        //ui->checkBox_useATHK->setChecked(ATHKValue.IsInvalid);

        TTVValue.MinValue = record.value("MinTTV").toDouble();
        TTVValue.MaxValue = record.value("MaxTTV").toDouble();
        TTVValue.IsInvalid = record.value("IsInvalidTTV").toBool();
        //ui->doubleSpinBox_minTTV->setValue(TTVValue.MinValue);
        //ui->doubleSpinBox_maxTTV->setValue(TTVValue.MaxValue);
       // ui->checkBox_useTTV->setChecked(TTVValue.IsInvalid);

        SORIValue.MinValue = record.value("MinSORI").toDouble();
        SORIValue.MaxValue = record.value("MaxSORI").toDouble();
        SORIValue.IsInvalid = record.value("IsInvalidSORI").toBool();
       // ui->doubleSpinBox_minSORI->setValue(SORIValue.MinValue);
        //ui->doubleSpinBox_maxSORI->setValue(SORIValue.MaxValue);
        //ui->checkBox_useSORI->setChecked(SORIValue.IsInvalid);
    }
    /*on_checkBox_useSORI_clicked(ui->checkBox_useSORI->isChecked());


    on_checkBox_useTTV_clicked(ui->checkBox_useTTV->isChecked());



    on_checkBox_useATHK_clicked(ui->checkBox_useATHK->isChecked());



    on_checkBox_useCTHK_clicked(ui->checkBox_useCTHK->isChecked());



    on_checkBox_useWARP_clicked(ui->checkBox_useWARP->isChecked());



    on_checkBox_useBOW_clicked(ui->checkBox_useBOW->isChecked());*/
}
void DialogMeasureParams::UpdateParams(int paramsId)
{
    if (paramsId != -1)
    {
        MeasureParamsId = paramsId;
    }
    QSqlQuery query;
    query.prepare("UPDATE params SET MinBOW = ?, MaxBOW = ?, IsInvalidBOW = ?, MinWARP = ?, MaxWARP = ?, IsInvalidWARP = ?, MinCTHK = ?, MaxCTHK = ?, IsInvalidCTHK = ?, MinATHK = ?, MaxATHK = ?, IsInvalidATHK = ?, MinTTV = ?, MaxTTV = ?, IsInvalidTTV = ?, MinSORI = ?, MaxSORI = ?, IsInvalidSORI = ? WHERE id = ?");
    query.addBindValue(BOWValue.MinValue);
    query.addBindValue(BOWValue.MaxValue);
    query.addBindValue(BOWValue.IsInvalid);

    query.addBindValue(WARPValue.MinValue);
    query.addBindValue(WARPValue.MaxValue);
    query.addBindValue(WARPValue.IsInvalid);

    query.addBindValue(CTHKValue.MinValue);
    query.addBindValue(CTHKValue.MaxValue);
    query.addBindValue(CTHKValue.IsInvalid);

    query.addBindValue(ATHKValue.MinValue);
    query.addBindValue(ATHKValue.MaxValue);
    query.addBindValue(ATHKValue.IsInvalid);

    query.addBindValue(TTVValue.MinValue);
    query.addBindValue(TTVValue.MaxValue);
    query.addBindValue(TTVValue.IsInvalid);

    query.addBindValue(SORIValue.MinValue);
    query.addBindValue(SORIValue.MaxValue);
    query.addBindValue(SORIValue.IsInvalid);

    query.addBindValue(MeasureParamsId);

    if (!query.exec()) {
        QMessageBox::critical(this, u8"错误", u8"更新失败: " + query.lastError().text());
    }
}









void DialogMeasureParams::on_checkBox_useSORI_clicked(bool checked)
{
    ui->doubleSpinBox_maxSORI->setEnabled(!checked);
    ui->doubleSpinBox_minSORI->setEnabled(!checked);
   // SORIValue.ResetValue(ui->doubleSpinBox_minSORI->value(), ui->doubleSpinBox_maxSORI->value(),checked);
}


void DialogMeasureParams::on_checkBox_useTTV_clicked(bool checked)
{
    ui->doubleSpinBox_maxTTV->setEnabled(!checked);
    ui->doubleSpinBox_minTTV->setEnabled(!checked);
    //TTVValue.ResetValue(ui->doubleSpinBox_minTTV->value(), ui->doubleSpinBox_maxTTV->value(), checked);
}


void DialogMeasureParams::on_checkBox_useATHK_clicked(bool checked)
{
    ui->doubleSpinBox_maxATHK->setEnabled(!checked);
    ui->doubleSpinBox_minATHK->setEnabled(!checked);
    //ATHKValue.ResetValue(ui->doubleSpinBox_minATHK->value(), ui->doubleSpinBox_maxATHK->value(), checked);
}


void DialogMeasureParams::on_checkBox_useCTHK_clicked(bool checked)
{
    ui->doubleSpinBox_maxCTHK->setEnabled(!checked);
    ui->doubleSpinBox_minCTHK->setEnabled(!checked);
   // CTHKValue.ResetValue(ui->doubleSpinBox_minCTHK->value(), ui->doubleSpinBox_maxCTHK->value(), checked);
}


void DialogMeasureParams::on_checkBox_useWARP_clicked(bool checked)
{
    ui->doubleSpinBox_maxWARP->setEnabled(!checked);
    ui->doubleSpinBox_minWARP->setEnabled(!checked);
   // WARPValue.ResetValue(ui->doubleSpinBox_minWARP->value(), ui->doubleSpinBox_maxWARP->value(), checked);
}


void DialogMeasureParams::on_checkBox_useBOW_clicked(bool checked)
{
    ui->doubleSpinBox_maxBOW->setEnabled(!checked);
    ui->doubleSpinBox_minBOW->setEnabled(!checked);
   // BOWValue.ResetValue(ui->doubleSpinBox_minBOW->value(), ui->doubleSpinBox_maxBOW->value(), checked);
}


void DialogMeasureParams::on_doubleSpinBox_minBOW_valueChanged(double arg1)
{
    double max = ui->doubleSpinBox_maxBOW->value();
    if (arg1 > max)
    {
        ui->doubleSpinBox_minBOW->setValue(max);
    }
   // BOWValue.ResetValue(ui->doubleSpinBox_minBOW->value(), ui->doubleSpinBox_maxBOW->value(), ui->checkBox_useBOW->isChecked());
}


void DialogMeasureParams::on_doubleSpinBox_maxBOW_valueChanged(double arg1)
{
    double min = ui->doubleSpinBox_minBOW->value();
    if (arg1 <= min)
    {
        ui->doubleSpinBox_maxBOW->setValue(min);
    }
    //BOWValue.ResetValue(ui->doubleSpinBox_minBOW->value(), ui->doubleSpinBox_maxBOW->value(), ui->checkBox_useBOW->isChecked());
}


void DialogMeasureParams::on_doubleSpinBox_minWARP_valueChanged(double arg1)
{
    double max = ui->doubleSpinBox_maxWARP->value();
    if (arg1 > max)
    {
        ui->doubleSpinBox_minWARP->setValue(max);
    }
   // WARPValue.ResetValue(ui->doubleSpinBox_minWARP->value(), ui->doubleSpinBox_maxWARP->value(), ui->checkBox_useWARP->isChecked());
}


void DialogMeasureParams::on_doubleSpinBox_maxWARP_valueChanged(double arg1)
{
    double min = ui->doubleSpinBox_minWARP->value();
    if (arg1 <= min)
    {
        ui->doubleSpinBox_maxWARP->setValue(min);
    }
   // WARPValue.ResetValue(ui->doubleSpinBox_minWARP->value(), ui->doubleSpinBox_maxWARP->value(), ui->checkBox_useWARP->isChecked());
}


void DialogMeasureParams::on_doubleSpinBox_minCTHK_valueChanged(double arg1)
{
    double max = ui->doubleSpinBox_maxCTHK->value();
    if (arg1 > max)
    {
        ui->doubleSpinBox_minCTHK->setValue(max);
    }
   // CTHKValue.ResetValue(ui->doubleSpinBox_minCTHK->value(), ui->doubleSpinBox_maxCTHK->value(), ui->checkBox_useCTHK->isChecked());
}


void DialogMeasureParams::on_doubleSpinBox_maxCTHK_valueChanged(double arg1)
{
    double min = ui->doubleSpinBox_minCTHK->value();
    if (arg1 <= min)
    {
        ui->doubleSpinBox_maxCTHK->setValue(min);
    }
   // CTHKValue.ResetValue(ui->doubleSpinBox_minCTHK->value(), ui->doubleSpinBox_maxCTHK->value(), ui->checkBox_useCTHK->isChecked());
}


void DialogMeasureParams::on_doubleSpinBox_minATHK_valueChanged(double arg1)
{
    double max = ui->doubleSpinBox_maxATHK->value();
    if (arg1 > max)
    {
        ui->doubleSpinBox_minATHK->setValue(max);
    }
   // ATHKValue.ResetValue(ui->doubleSpinBox_minATHK->value(), ui->doubleSpinBox_maxATHK->value(), ui->checkBox_useATHK->isChecked());
}


void DialogMeasureParams::on_doubleSpinBox_maxATHK_valueChanged(double arg1)
{
    double min = ui->doubleSpinBox_minATHK->value();
    if (arg1 <= min)
    {
        ui->doubleSpinBox_maxATHK->setValue(min);
    }
   // ATHKValue.ResetValue(ui->doubleSpinBox_minATHK->value(), ui->doubleSpinBox_maxATHK->value(), ui->checkBox_useATHK->isChecked());
}


void DialogMeasureParams::on_doubleSpinBox_minTTV_valueChanged(double arg1)
{
    double max = ui->doubleSpinBox_maxTTV->value();
    if (arg1 > max)
    {
        ui->doubleSpinBox_minTTV->setValue(max);
    }
   // TTVValue.ResetValue(ui->doubleSpinBox_minTTV->value(), ui->doubleSpinBox_maxTTV->value(), ui->checkBox_useTTV->isChecked());
}


void DialogMeasureParams::on_doubleSpinBox_maxTTV_valueChanged(double arg1)
{
    double min = ui->doubleSpinBox_minTTV->value();
    if (arg1 <= min)
    {
        ui->doubleSpinBox_maxTTV->setValue(min);
    }
   // TTVValue.ResetValue(ui->doubleSpinBox_minTTV->value(), ui->doubleSpinBox_maxTTV->value(), ui->checkBox_useTTV->isChecked());
}


void DialogMeasureParams::on_doubleSpinBox_minSORI_valueChanged(double arg1)
{
    double max = ui->doubleSpinBox_maxSORI->value();
    if (arg1 > max)
    {
        ui->doubleSpinBox_minSORI->setValue(max);
    }
   // SORIValue.ResetValue(ui->doubleSpinBox_minSORI->value(), ui->doubleSpinBox_maxSORI->value(), ui->checkBox_useSORI->isChecked());
}
void DialogMeasureParams::on_doubleSpinBox_maxSORI_valueChanged(double arg1)
{
    double min = ui->doubleSpinBox_minSORI->value();
    if (arg1 <= min) 
    {
        ui->doubleSpinBox_maxSORI->setValue(min);
    }
   // SORIValue.ResetValue(ui->doubleSpinBox_minSORI->value(), ui->doubleSpinBox_maxSORI->value(), ui->checkBox_useSORI->isChecked());
}
