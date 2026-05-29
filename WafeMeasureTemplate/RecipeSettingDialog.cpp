#include "RecipeSettingDialog.h"
#include "ui_RecipeSettingDialog.h"
#include "PermissionManager/delegate.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QInputDialog>
#include "recipedialog.h"
#include <QFileDialog>
#include <QtWidgets/qfiledialog.h>
#include <QFile>
#include <QTextStream>
#include <qdebug.h>

RecipeSettingDialog::RecipeSettingDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::RecipeSettingDialog)
    , model(new QSqlTableModel(this))
{
    ui->setupUi(this);
    ui->pushButton_exportRecipe->setVisible(false);
    ui->pushButton_importRecipe->setVisible(false);
    setupTable();
}

RecipeSettingDialog::~RecipeSettingDialog()
{
    delete ui;
}
QList<QString> RecipeSettingDialog::InitRecipes(int defaultTrimSize)
{
    DefaultTrimSize = defaultTrimSize;
    int columnIndex = model->fieldIndex("recipename");
   
   // 去重存储值的容器
    QSet<QString> uniqueValues;

    // 遍历所有行，收集唯一值
    for (int row = 0; row < model->rowCount(); ++row) {
        QModelIndex index = model->index(row, columnIndex);
        QString value = model->data(index).toString();
        uniqueValues.insert(value);
        
    }
    return uniqueValues.toList();
   
}
bool RecipeSettingDialog::ResetRecipe()
{
    on_pushButton_addRecipe_clicked();
    return  newRecipeName != "";
}
void RecipeSettingDialog::on_pushButton_addRecipe_clicked()
{
   
    RecipeDialog dlg(this);
    dlg.SetDefaultPath(DefaultPath);

    QList<int> thicknessNames =FindThicknessName();
    dlg.SetDefaultName(newRecipeName);
    dlg.SetDefaultThicknesses(thicknessNames);
    dlg.SetDefaultThickness(DefaultThickness);
    dlg.SetDefaultSize(DefaultSize);
    dlg.SetDefaultTrimSize(DefaultTrimSize);
    dlg.InitMeasureParams(NULL);
    newRecipeName = "";
    if(dlg.exec() == QDialog::Accepted){
       
        auto sts = dlg.RecipeName();
        if (sts.isEmpty() || sts == "") 
        {
            QMessageBox::critical(this, u8"错误", u8"添加程式失败: 名称为空!");
            return;
        }

        QSqlQuery query;
        query.prepare("INSERT INTO recipes (recipename, motion_path, product_thickness, product_size, trim_size, MinBOW, MaxBOW, IsInvalidBOW, MinWARP, MaxWARP, IsInvalidWARP, MinCTHK, MaxCTHK, IsInvalidCTHK, MinATHK, MaxATHK, IsInvalidATHK, MinTTV, MaxTTV, IsInvalidTTV, MinSORI, MaxSORI, IsInvalidSORI) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
       
        query.addBindValue(dlg.RecipeName());
        query.addBindValue(dlg.MotionPath());
        query.addBindValue(dlg.ProductThickness());
        query.addBindValue(dlg.ProductSize());
        query.addBindValue(dlg.TrimSize());

        query.addBindValue(dlg.BOWValue.MinValue);
        query.addBindValue(dlg.BOWValue.MaxValue);
        query.addBindValue(dlg.BOWValue.IsInvalid);

        query.addBindValue(dlg.WARPValue.MinValue);
        query.addBindValue(dlg.WARPValue.MaxValue);
        query.addBindValue(dlg.WARPValue.IsInvalid);

        query.addBindValue(dlg.CTHKValue.MinValue);
        query.addBindValue(dlg.CTHKValue.MaxValue);
        query.addBindValue(dlg.CTHKValue.IsInvalid);

        query.addBindValue(dlg.ATHKValue.MinValue);
        query.addBindValue(dlg.ATHKValue.MaxValue);
        query.addBindValue(dlg.ATHKValue.IsInvalid);

        query.addBindValue(dlg.TTVValue.MinValue);
        query.addBindValue(dlg.TTVValue.MaxValue);
        query.addBindValue(dlg.TTVValue.IsInvalid);

        query.addBindValue(dlg.SORIValue.MinValue);
        query.addBindValue(dlg.SORIValue.MaxValue);
        query.addBindValue(dlg.SORIValue.IsInvalid);


        if(!query.exec()){
            QMessageBox::critical(this, u8"错误", u8"添加程式失败: " + query.lastError().text());
        }
        else
        {
            newRecipeName = sts;
        }
        model->select();
       
    }
}


void RecipeSettingDialog::on_pushButton_editRecipe_clicked()
{
    QModelIndex index = ui->tableView->currentIndex();
    if(!index.isValid()) return;
    RecipeDialog dlg(this);
    dlg.SetDefaultPath(DefaultPath);
    QString name = model->data(model->index(index.row(), 0)).toString();
    QList<int> thicknessNames = FindThicknessName();

    dlg.SetDefaultThicknesses(thicknessNames);

    dlg.InitMeasureParams(name);
    if (dlg.exec() == QDialog::Accepted)
    {

            QSqlQuery query;
            query.prepare("UPDATE recipes SET recipename = ?, motion_path = ?, product_thickness = ?, product_size = ?, trim_size = ?, MinBOW = ?, MaxBOW = ?, IsInvalidBOW = ?, MinWARP = ?, MaxWARP = ?, IsInvalidWARP = ?, MinCTHK = ?, MaxCTHK = ?, IsInvalidCTHK = ?, MinATHK = ?, MaxATHK = ?, IsInvalidATHK = ?, MinTTV = ?, MaxTTV = ?, IsInvalidTTV = ?, MinSORI = ?, MaxSORI = ?, IsInvalidSORI = ? WHERE recipename = ?");
            query.addBindValue(dlg.RecipeName());
            query.addBindValue(dlg.MotionPath());
            query.addBindValue(dlg.ProductThickness());
            query.addBindValue(dlg.ProductSize());
            query.addBindValue(dlg.TrimSize());

            query.addBindValue(dlg.BOWValue.MinValue);
            query.addBindValue(dlg.BOWValue.MaxValue);
            query.addBindValue(dlg.BOWValue.IsInvalid);

            query.addBindValue(dlg.WARPValue.MinValue);
            query.addBindValue(dlg.WARPValue.MaxValue);
            query.addBindValue(dlg.WARPValue.IsInvalid);

            query.addBindValue(dlg.CTHKValue.MinValue);
            query.addBindValue(dlg.CTHKValue.MaxValue);
            query.addBindValue(dlg.CTHKValue.IsInvalid);

            query.addBindValue(dlg.ATHKValue.MinValue);
            query.addBindValue(dlg.ATHKValue.MaxValue);
            query.addBindValue(dlg.ATHKValue.IsInvalid);

            query.addBindValue(dlg.TTVValue.MinValue);
            query.addBindValue(dlg.TTVValue.MaxValue);
            query.addBindValue(dlg.TTVValue.IsInvalid);

            query.addBindValue(dlg.SORIValue.MinValue);
            query.addBindValue(dlg.SORIValue.MaxValue);
            query.addBindValue(dlg.SORIValue.IsInvalid);
            query.addBindValue(name);
            if (query.exec()) {
                model->select();
            }
            else {
                QMessageBox::critical(this, u8"错误", u8"更新失败: " + query.lastError().text());
            }

    }
}


void RecipeSettingDialog::on_pushButton_deleteRecipe_clicked()
{
    QModelIndex index = ui->tableView->currentIndex();
    if(!index.isValid()) return;

    QString name = model->data(model->index(index.row(), 0)).toString();

    if(QMessageBox::question(this, u8"确认删除",
                              u8"确定要删除程式 " + name + u8" 吗？") == QMessageBox::Yes){
        QSqlQuery query;
        query.prepare("DELETE FROM recipes WHERE recipename = ?");
        query.addBindValue(name);

        if(query.exec()){
            model->select();
        } else {
            QMessageBox::critical(this, u8"错误", u8"删除失败: " + query.lastError().text());
        }
    }
}



void RecipeSettingDialog::on_pushButton_refreshTable_clicked()
{
    model->select();
}

void RecipeSettingDialog::setupTable()
{
   
    model->setTable("recipes");
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    model->select(); // 确保 model 已经加载数据

    // 先移除不需要的列
    model->removeColumn(model->fieldIndex("id"));
    model->removeColumn(model->fieldIndex("threshold_recipe_id"));


    // 设置表头显示名称
    model->setHeaderData(model->fieldIndex("recipename"), Qt::Horizontal, u8"程式名称");
    model->setHeaderData(model->fieldIndex("motion_path"), Qt::Horizontal, u8"测量路径");
    model->setHeaderData(model->fieldIndex("product_thickness"), Qt::Horizontal,u8"产品标准");
    model->setHeaderData(model->fieldIndex("product_size"), Qt::Horizontal, u8"产品尺寸");
    model->setHeaderData(model->fieldIndex("trim_size"), Qt::Horizontal, u8"裁边长度(mm)");
    model->setHeaderData(model->fieldIndex("created_time"), Qt::Horizontal, u8"创建时间");
    ui->tableView->setModel(model);
    ui->tableView->horizontalHeader()->setStretchLastSection(true);
   
    ui->tableView->setItemDelegateForColumn(model->fieldIndex("created_time"), new DateTimeDelegate(this));


   
}


QSqlRecord RecipeSettingDialog::FindRecipeByName(QString name)
{
    QSqlTableModel* m_model = new QSqlTableModel(this);
    m_model->setTable("recipes");
    m_model->setFilter(QString("recipename = '%1'").arg(name));
    m_model->select();
    if (m_model->rowCount() > 0) {
        // 获取第一行
        return m_model->record(0);


    }
    else {
        return QSqlRecord(); // 返回空记录表示未找到
    }
}
QList<int> RecipeSettingDialog::FindThicknessName()
{
    QSqlTableModel* m_model = new QSqlTableModel(this);
    m_model->setTable("thicknesses");
   /* m_model->setFilter(QString("thicknessname = '%1'").arg("*"));*/
    m_model->select();

    // 去重存储值的容器
    QSet<int> uniqueValues;

   
    int rowNum = m_model->rowCount();
    if (rowNum> 0) {
        // 获取第一行

        // 遍历所有行，收集唯一值
        for (int row = 0; row < rowNum; ++row) {
           
            int value = m_model->record(row).value("thicknessname").toInt();
            uniqueValues.insert(value);

        }

    }

    return uniqueValues.toList();
}

void RecipeSettingDialog::on_pushButton_close_clicked()
{
    close();
}


void RecipeSettingDialog::on_pushButton_importRecipe_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        u8"选择 CSV 文件",
        QDir::homePath(),
        u8"CSV 文件 (*.csv);;所有文件 (*)"
    );

    if (filePath.isEmpty()) {
        return; // 用户取消选择
    }
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QTextStream in(&file);
   


    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) {
            continue; // 跳过空行
        }

        // 按逗号分割字段（处理简单 CSV，不处理含逗号的复杂字段）
        QStringList fields = line.split(',');

        // 检查字段数量是否合法（例如预期 3 列）
        if (fields.size() != 6) {
            qDebug() << "无效的行：" << line;
          
        }
        else
        {
            QString name = fields[0];
            QString path = fields[1];
            QString thickness = fields[2];
            QString size = fields[3];
            QString trimSize = fields[4];
            int thresholdRecipeId = fields[5].toInt();
            QSqlQuery query;
            query.prepare("INSERT INTO recipes (recipename, motion_path, product_thickness, product_size, trim_size, threshold_recipe_id) "
                "VALUES (?, ?, ?, ?, ?, ?)");
         
            query.addBindValue(name);
            query.addBindValue(path);
            query.addBindValue(thickness);
            query.addBindValue(size);
            query.addBindValue(trimSize);
            query.addBindValue(thresholdRecipeId);
            if (!query.exec()) {
                QMessageBox::critical(this, u8"错误", u8"添加程式失败: " + query.lastError().text());
            }
            model->select();
        }
        break;
    }

    file.close();
  
}


void RecipeSettingDialog::on_pushButton_exportRecipe_clicked()
{
    QModelIndex index = ui->tableView->currentIndex();
    if (!index.isValid()) return;
    QString name = model->data(model->index(index.row(), 0)).toString();
    QString path = model->data(model->index(index.row(), 1)).toString();
    QString thickness = model->data(model->index(index.row(), 2)).toString();
    QString size = model->data(model->index(index.row(), 3)).toString();
    QString trimSize = model->data(model->index(index.row(),4)).toString();
    int thresholdRecipeId= model->data(model->index(index.row(), 5)).toInt();
    QString valueString = QString("%1,%2,%3,%4,%5,%6").arg(name).arg(path).arg(thickness).arg(size).arg(trimSize).arg(thresholdRecipeId);
    // 弹出保存对话框
    QString filePath = QFileDialog::getSaveFileName(
        this,                   // 父窗口
        u8"保存文件",              // 对话框标题
        QDir::homePath(),        // 默认目录
        u8"文本文件 (*.csv);;所有文件 (*)"  // 文件过滤器
    );

    if (filePath.isEmpty()) {
        // 用户取消保存
        return;
    }

    // 保存文件逻辑
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, u8"错误", u8"无法保存文件：" + file.errorString());
        return;
    }

    QTextStream out(&file);
    out << valueString;
    file.close();
}

