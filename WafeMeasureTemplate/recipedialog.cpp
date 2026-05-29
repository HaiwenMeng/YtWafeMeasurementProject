#include "recipedialog.h"
#include "ui_recipedialog.h"
#include <QMessageBox>
#include <QtWidgets/qinputdialog.h>
#include  <QSqlTableModel>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QSqlRecord>
RecipeDialog::RecipeDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::RecipeDialog)
{
    ui->setupUi(this);
    // 连接按钮信号到槽函数
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &RecipeDialog::onOkClicked);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &RecipeDialog::onCancelClicked);
    intComboBox();
}


QString RecipeDialog::RecipeName() const { return ui->lineEdit_recipeName->text().remove(QRegularExpression("\\s")); }
QString RecipeDialog::MotionPath() const { return ui->comboBox_path->currentText(); }
QString RecipeDialog::ProductThickness() const { return ui->comboBox_thickness->currentText(); }
QString  RecipeDialog::ProductSize() const { return  ui->comboBox_size->currentText(); }
int  RecipeDialog::TrimSize() const { return ui->lineEdit_trimSize->text().toInt(); }


void RecipeDialog::InitMeasureParams(QString recipeName=NULL)
{
    if (recipeName != NULL)
    {
        EditingRecipeName = recipeName;
        QSqlTableModel* m_model = new QSqlTableModel(this);
        m_model->setTable("recipes");
        m_model->setFilter(QString("recipename = '%1'").arg(recipeName));
        bool state = m_model->select();
        if (m_model->rowCount() > 0) {
            // 获取第一行
            QSqlRecord record = m_model->record(0);

            SetDefaultName(record.value("recipename").toString());
          
            SetDefaultPath(record.value("motion_path").toString());
            SetDefaultThickness(record.value("product_thickness").toString());
            SetDefaultSize(record.value("product_size").toString());
            SetDefaultTrimSize(record.value("trim_size").toInt());
            

            BOWValue.MinValue = record.value("MinBOW").toDouble();
            BOWValue.MaxValue = record.value("MaxBOW").toDouble();
            BOWValue.IsInvalid = record.value("IsInvalidBOW").toBool();

          

            WARPValue.MinValue = record.value("MinWARP").toDouble();
            WARPValue.MaxValue = record.value("MaxWARP").toDouble();
            WARPValue.IsInvalid = record.value("IsInvalidWARP").toBool();
          

            CTHKValue.MinValue = record.value("MinCTHK").toDouble();
            CTHKValue.MaxValue = record.value("MaxCTHK").toDouble();
            CTHKValue.IsInvalid = record.value("IsInvalidCTHK").toBool();
          

            ATHKValue.MinValue = record.value("MinATHK").toDouble();
            ATHKValue.MaxValue = record.value("MaxATHK").toDouble();
            ATHKValue.IsInvalid = record.value("IsInvalidATHK").toBool();
           

            TTVValue.MinValue = record.value("MinTTV").toDouble();
            TTVValue.MaxValue = record.value("MaxTTV").toDouble();
            TTVValue.IsInvalid = record.value("IsInvalidTTV").toBool();
          

            SORIValue.MinValue = record.value("MinSORI").toDouble();
            SORIValue.MaxValue = record.value("MaxSORI").toDouble();
            SORIValue.IsInvalid = record.value("IsInvalidSORI").toBool();
            
        }
      
      
       
    }
    else
    {
 
        BOWValue.ResetValue(0,9999,false);
        WARPValue.ResetValue(0,9999,false);
        CTHKValue.ResetValue(0,9999,false);
        ATHKValue.ResetValue(0,9999,false);
        TTVValue.ResetValue(0,9999,false);
        SORIValue.ResetValue(0,9999,false);
    }
    ui->doubleSpinBox_maxBOW->setValue(BOWValue.MaxValue);
    ui->doubleSpinBox_minBOW->setValue(BOWValue.MinValue);

    ui->checkBox_useBOW->setChecked(BOWValue.IsInvalid);
    ui->doubleSpinBox_minBOW->setEnabled(BOWValue.IsInvalid);
    ui->doubleSpinBox_maxBOW->setEnabled(BOWValue.IsInvalid);

    ui->doubleSpinBox_maxWARP->setValue(WARPValue.MaxValue);
    ui->doubleSpinBox_minWARP->setValue(WARPValue.MinValue);

    ui->checkBox_useWARP->setChecked(WARPValue.IsInvalid);
    ui->doubleSpinBox_minWARP->setEnabled(WARPValue.IsInvalid);
    ui->doubleSpinBox_maxWARP->setEnabled(WARPValue.IsInvalid);

    ui->doubleSpinBox_maxCTHK->setValue(CTHKValue.MaxValue);
    ui->doubleSpinBox_minCTHK->setValue(CTHKValue.MinValue);

    ui->checkBox_useCTHK->setChecked(CTHKValue.IsInvalid);
    ui->doubleSpinBox_minCTHK->setEnabled(CTHKValue.IsInvalid);
    ui->doubleSpinBox_maxCTHK->setEnabled(CTHKValue.IsInvalid);

    ui->doubleSpinBox_maxATHK->setValue(ATHKValue.MaxValue);
    ui->doubleSpinBox_minATHK->setValue(ATHKValue.MinValue);

    ui->checkBox_useATHK->setChecked(ATHKValue.IsInvalid);
    ui->doubleSpinBox_minATHK->setEnabled(ATHKValue.IsInvalid);
    ui->doubleSpinBox_maxATHK->setEnabled(ATHKValue.IsInvalid);

    ui->doubleSpinBox_maxTTV->setValue(TTVValue.MaxValue);
    ui->doubleSpinBox_minTTV->setValue(TTVValue.MinValue);

    ui->checkBox_useTTV->setChecked(TTVValue.IsInvalid);
    ui->doubleSpinBox_minTTV->setEnabled(TTVValue.IsInvalid);
    ui->doubleSpinBox_maxTTV->setEnabled(TTVValue.IsInvalid);

    ui->doubleSpinBox_maxSORI->setValue(SORIValue.MaxValue);
    ui->doubleSpinBox_minSORI->setValue(SORIValue.MinValue);

    ui->checkBox_useSORI->setChecked(SORIValue.IsInvalid);
    ui->doubleSpinBox_minSORI->setEnabled(SORIValue.IsInvalid);
    ui->doubleSpinBox_maxSORI->setEnabled(SORIValue.IsInvalid);
    IsInit = true;
   
    on_checkBox_useSORI_clicked(ui->checkBox_useSORI->isChecked());


    on_checkBox_useTTV_clicked(ui->checkBox_useTTV->isChecked());



    on_checkBox_useATHK_clicked(ui->checkBox_useATHK->isChecked());



    on_checkBox_useCTHK_clicked(ui->checkBox_useCTHK->isChecked());



    on_checkBox_useWARP_clicked(ui->checkBox_useWARP->isChecked());



    on_checkBox_useBOW_clicked(ui->checkBox_useBOW->isChecked());
   
}

void  RecipeDialog::SetDefaultTrimSize(int defaultTrimSize)
{
    ui->lineEdit_trimSize->setText(QString::number(defaultTrimSize));
}
void  RecipeDialog::SetDefaultName(QString defaultName)
{
    ui->lineEdit_recipeName->setText(defaultName);
}
void  RecipeDialog::SetDefaultPath(QString defaultPath)
{
    int index = ui->comboBox_path->findText(defaultPath);
    if (index != -1)
        ui->comboBox_path->setCurrentIndex(index);
   
}
//产品厚度
void  RecipeDialog::SetDefaultThicknesses(QList<int> defaultThicknesses)
{
    for (int i = 0; i < defaultThicknesses.count(); i++)
    {
        ui->comboBox_thickness->addItem(QString::number(defaultThicknesses[i]) + u8"μm", defaultThicknesses[i]);
    }
}

void  RecipeDialog::SetDefaultThickness(QString defaultThickness)
{
    int index = ui->comboBox_thickness->findText(defaultThickness);
    if (index != -1)
        ui->comboBox_thickness->setCurrentIndex(index);
}
void  RecipeDialog::SetDefaultSize(QString defaultSize)
{
    int index = ui->comboBox_size->findText(defaultSize);
    if (index != -1)
        ui->comboBox_size->setCurrentIndex(index);
}

void RecipeDialog::intComboBox()
{
   //测量路径
   ui->comboBox_path->addItem(u8"1线", 1);
   ui->comboBox_path->addItem(u8"2线", 2);
   ui->comboBox_path->addItem(u8"4线", 4);
   ui->comboBox_path->addItem(u8"6线", 6);
   ui->comboBox_path->addItem(u8"8线", 8);
   ui->comboBox_path->setCurrentIndex(4);
   


   //产品尺寸
   ui->comboBox_size->addItem(u8"6英寸", 6);
   ui->comboBox_size->addItem(u8"8英寸", 8);
   ui->comboBox_size->addItem(u8"12英寸", 12);
   ui->comboBox_size->setCurrentIndex(2);
  
}


RecipeDialog::~RecipeDialog()
{
    delete ui;
}




void RecipeDialog::on_lineEdit_recipeName_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1);

    if (EditingRecipeName == ui->lineEdit_recipeName->text().remove(QRegularExpression("\\s")))
    {
        this->setWindowTitle(u8"编辑程式");
        IsReady = true;
        return;
    }

    // 检查用户名是否已存在
    QSqlQuery checkQuery;
    checkQuery.prepare("SELECT 1 FROM recipes WHERE recipename = ?");
    checkQuery.addBindValue(RecipeDialog::RecipeName());
    checkQuery.exec();
    if (checkQuery.next()) {
        ui->label_status->setText(u8"程式名已存在");
        IsReady = false;
    }
    else
    {
        IsReady = true;
        ui->label_status->clear();
    }
   
  
}


void  RecipeDialog::accept()
{
    bool isValueValidable = WARPValue.CheckMinMaxValue() && BOWValue.CheckMinMaxValue() && CTHKValue.CheckMinMaxValue() && ATHKValue.CheckMinMaxValue() && ATHKValue.CheckMinMaxValue() && TTVValue.CheckMinMaxValue() && SORIValue.CheckMinMaxValue();
    if (!isValueValidable)
    {
         QMessageBox::warning(this, u8"错误", u8"请确保设置的最大值大于等于最小值！");
    }
    else
    {
        QDialog::accept();
    }

}
    void RecipeDialog::onOkClicked() {
        // 确定按钮逻辑（例如数据验证）
        if (IsReady&&(ui->lineEdit_recipeName->text().remove(QRegularExpression("\\s")).length() != 0))
        {
            bool isValueValidable = WARPValue.CheckMinMaxValue() && BOWValue.CheckMinMaxValue() && CTHKValue.CheckMinMaxValue() && ATHKValue.CheckMinMaxValue() && ATHKValue.CheckMinMaxValue() && TTVValue.CheckMinMaxValue() && SORIValue.CheckMinMaxValue();
            if (isValueValidable)
            {
                accept(); // 关闭对话框并返回 QDialog::Accepted
            }
        }
        else {
            QMessageBox::warning(this, u8"错误", u8"数据无效，请检查输入！");
            reject();
        }
    }

    void RecipeDialog::onCancelClicked() {
        reject();
    }

   
    void RecipeDialog::on_pushButton_removeThickness_clicked()
    {
        QString name = ui->comboBox_thickness->currentText();
        // int value = ui->comboBox_thickness->currentData().toInt();
        if (QMessageBox::question(this, u8"确认删除",
            u8"确定要删除用户 " + name + u8" 吗？") == QMessageBox::Yes) {
            QSqlQuery query;
            query.prepare("DELETE FROM thicknesses WHERE thicknessname = ?");
            query.addBindValue(name);

            if (!query.exec()) {
                QMessageBox::critical(this, u8"错误", u8"删除失败: " + query.lastError().text());
            }
            else
            {
                int idx = ui->comboBox_thickness->currentIndex();
                ui->comboBox_thickness->removeItem(idx);
            }
        }
    }


    void RecipeDialog::on_pushButton_addThickness_clicked()
    {
        bool bRet = false;

       
        int value = QInputDialog::getInt(this,u8"添加新标准",u8"请输入新标准:", 500, 0, 10000, 1, &bRet);
        if (bRet)
        {
            QSqlQuery query;
            query.prepare("INSERT INTO thicknesses (thicknessname) "
                "VALUES (?)");

            query.addBindValue(value);

            if (!query.exec()) {
                QMessageBox::critical(this, u8"错误", u8"添加标准失败: " + query.lastError().text());
            }
            else
            {
                ui->comboBox_thickness->addItem(QString::number(value) + u8"μm", value);
                ui->comboBox_thickness->setCurrentIndex(ui->comboBox_thickness->count() - 1);
            }
           
        }  

    }


    void RecipeDialog::on_checkBox_useSORI_clicked(bool checked)
    {
        ui->doubleSpinBox_maxSORI->setEnabled(!checked);
        ui->doubleSpinBox_minSORI->setEnabled(!checked);
        SORIValue.ResetValue(ui->doubleSpinBox_minSORI->value(), ui->doubleSpinBox_maxSORI->value(), checked);
    }


    void RecipeDialog::on_checkBox_useTTV_clicked(bool checked)
    {
        ui->doubleSpinBox_maxTTV->setEnabled(!checked);
        ui->doubleSpinBox_minTTV->setEnabled(!checked);
        TTVValue.ResetValue(ui->doubleSpinBox_minTTV->value(), ui->doubleSpinBox_maxTTV->value(), checked);
    }


    void RecipeDialog::on_checkBox_useATHK_clicked(bool checked)
    {
        ui->doubleSpinBox_maxATHK->setEnabled(!checked);
        ui->doubleSpinBox_minATHK->setEnabled(!checked);
        ATHKValue.ResetValue(ui->doubleSpinBox_minATHK->value(), ui->doubleSpinBox_maxATHK->value(), checked);
    }


    void RecipeDialog::on_checkBox_useCTHK_clicked(bool checked)
    {
        ui->doubleSpinBox_maxCTHK->setEnabled(!checked);
        ui->doubleSpinBox_minCTHK->setEnabled(!checked);
        CTHKValue.ResetValue(ui->doubleSpinBox_minCTHK->value(), ui->doubleSpinBox_maxCTHK->value(), checked);
    }


    void RecipeDialog::on_checkBox_useWARP_clicked(bool checked)
    {
        ui->doubleSpinBox_maxWARP->setEnabled(!checked);
        ui->doubleSpinBox_minWARP->setEnabled(!checked);
        WARPValue.ResetValue(ui->doubleSpinBox_minWARP->value(), ui->doubleSpinBox_maxWARP->value(), checked);
    }


    void RecipeDialog::on_checkBox_useBOW_clicked(bool checked)
    {
        ui->doubleSpinBox_maxBOW->setEnabled(!checked);
        ui->doubleSpinBox_minBOW->setEnabled(!checked);
        BOWValue.ResetValue(ui->doubleSpinBox_minBOW->value(), ui->doubleSpinBox_maxBOW->value(), checked);
    }


    void RecipeDialog::on_doubleSpinBox_minBOW_valueChanged(double arg1)
    {
        Q_UNUSED(arg1);

        if (!IsInit)
            return;
        BOWValue.ResetValue(ui->doubleSpinBox_minBOW->value(), ui->doubleSpinBox_maxBOW->value(), ui->checkBox_useBOW->isChecked());
    }


    void RecipeDialog::on_doubleSpinBox_maxBOW_valueChanged(double arg1)
    {
        Q_UNUSED(arg1);

        if (!IsInit)
            return;
    
        BOWValue.ResetValue(ui->doubleSpinBox_minBOW->value(), ui->doubleSpinBox_maxBOW->value(), ui->checkBox_useBOW->isChecked());
    }


    void RecipeDialog::on_doubleSpinBox_minWARP_valueChanged(double arg1)
    {
        Q_UNUSED(arg1);

        if (!IsInit)
            return;
        WARPValue.ResetValue(ui->doubleSpinBox_minWARP->value(), ui->doubleSpinBox_maxWARP->value(), ui->checkBox_useWARP->isChecked());
    }


    void RecipeDialog::on_doubleSpinBox_maxWARP_valueChanged(double arg1)
    {
        Q_UNUSED(arg1);

        if (!IsInit)
            return;
        WARPValue.ResetValue(ui->doubleSpinBox_minWARP->value(), ui->doubleSpinBox_maxWARP->value(), ui->checkBox_useWARP->isChecked());
    }


    void RecipeDialog::on_doubleSpinBox_minCTHK_valueChanged(double arg1)
    {
        Q_UNUSED(arg1);

        if (!IsInit)
            return;
        CTHKValue.ResetValue(ui->doubleSpinBox_minCTHK->value(), ui->doubleSpinBox_maxCTHK->value(), ui->checkBox_useCTHK->isChecked());
    }


    void RecipeDialog::on_doubleSpinBox_maxCTHK_valueChanged(double arg1)
    {
        Q_UNUSED(arg1);

        if (!IsInit)
            return;
        CTHKValue.ResetValue(ui->doubleSpinBox_minCTHK->value(), ui->doubleSpinBox_maxCTHK->value(), ui->checkBox_useCTHK->isChecked());
    }


    void RecipeDialog::on_doubleSpinBox_minATHK_valueChanged(double arg1)
    {
        Q_UNUSED(arg1);

        if (!IsInit)
            return;
        ATHKValue.ResetValue(ui->doubleSpinBox_minATHK->value(), ui->doubleSpinBox_maxATHK->value(), ui->checkBox_useATHK->isChecked());
    }


    void RecipeDialog::on_doubleSpinBox_maxATHK_valueChanged(double arg1)
    {
        Q_UNUSED(arg1);

        if (!IsInit)
            return;
        ATHKValue.ResetValue(ui->doubleSpinBox_minATHK->value(), ui->doubleSpinBox_maxATHK->value(), ui->checkBox_useATHK->isChecked());
    }


    void RecipeDialog::on_doubleSpinBox_minTTV_valueChanged(double arg1)
    {
        Q_UNUSED(arg1);

        if (!IsInit)
            return;
        TTVValue.ResetValue(ui->doubleSpinBox_minTTV->value(), ui->doubleSpinBox_maxTTV->value(), ui->checkBox_useTTV->isChecked());
    }


    void RecipeDialog::on_doubleSpinBox_maxTTV_valueChanged(double arg1)
    {
        Q_UNUSED(arg1);

        if (!IsInit)
            return;
        TTVValue.ResetValue(ui->doubleSpinBox_minTTV->value(), ui->doubleSpinBox_maxTTV->value(), ui->checkBox_useTTV->isChecked());
    }


    void RecipeDialog::on_doubleSpinBox_minSORI_valueChanged(double arg1)
    {
        Q_UNUSED(arg1);

        if (!IsInit)
            return;
        SORIValue.ResetValue(ui->doubleSpinBox_minSORI->value(), ui->doubleSpinBox_maxSORI->value(), ui->checkBox_useSORI->isChecked());
    }
    void RecipeDialog::on_doubleSpinBox_maxSORI_valueChanged(double arg1)
    {
        Q_UNUSED(arg1);

        if (!IsInit)
            return;
        SORIValue.ResetValue(ui->doubleSpinBox_minSORI->value(), ui->doubleSpinBox_maxSORI->value(), ui->checkBox_useSORI->isChecked());
    }



    void RecipeDialog::on_buttonBox_accepted()
    {
        return;
    }

