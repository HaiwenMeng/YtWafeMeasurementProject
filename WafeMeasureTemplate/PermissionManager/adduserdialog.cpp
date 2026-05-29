#include "adduserdialog.h"
#include "ui_adduserdialog.h"
#include <QSqlQuery>
#include <QMessageBox>

AddUserDialog::AddUserDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AddUserDialog)
{
    ui->setupUi(this);

    ui->comboBox_role->addItem(u8"工程师", 1);
    ui->comboBox_role->addItem(u8"操作工", 2);
}

AddUserDialog::~AddUserDialog()
{
    delete ui;
}

QString AddUserDialog::username() const { return ui->lineEdit_username->text(); }
QString AddUserDialog::password() const { return ui->lineEdit_password->text(); }
int AddUserDialog::role() const { return ui->comboBox_role->currentData().toInt(); }

void AddUserDialog::on_pushButton_add_clicked()
{
    accept();
}


void AddUserDialog::on_pushButton_cancel_clicked()
{
    reject();
}


void AddUserDialog::on_lineEdit_username_textChanged(const QString &arg1)
{
    bool valid = true;

    // 检查用户名是否已存在
    QSqlQuery checkQuery;
    checkQuery.prepare("SELECT 1 FROM users WHERE username = ?");
    checkQuery.addBindValue(username());
    checkQuery.exec();
    if(checkQuery.next()){
        ui->label_status->setText(u8"用户名已存在");
        valid = false;
    }

    ui->pushButton_add->setEnabled(valid);
    if(valid) ui->label_status->clear();
}


void AddUserDialog::on_lineEdit_password_textChanged(const QString &arg1)
{
    bool valid = true;

    // 检查密码强度
    if(password().length() < 6){
        ui->label_status->setText(u8"密码至少需要6位");
        valid = false;
    }

    ui->pushButton_add->setEnabled(valid);
    if(valid) ui->label_status->clear();
}


void AddUserDialog::on_lineEdit_confirm_textChanged(const QString &arg1)
{
    bool valid = true;

    // 确认密码匹配
    if(ui->lineEdit_password->text() != ui->lineEdit_confirm->text()){
        ui->label_status->setText(u8"两次输入的密码不一致");
        valid = false;
    }

    ui->pushButton_add->setEnabled(valid);
    if(valid) ui->label_status->clear();
}

