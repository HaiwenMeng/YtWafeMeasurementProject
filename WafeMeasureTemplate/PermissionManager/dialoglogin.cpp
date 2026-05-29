#include "dialoglogin.h"
#include "ui_dialoglogin.h"
#include "authservice.h"
#include <QMessageBox>

DialogLogin::DialogLogin(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogLogin)
{
    ui->setupUi(this);
}

DialogLogin::~DialogLogin()
{
    delete ui;
}

void DialogLogin::on_pushButton_login_clicked()
{
    QString username = ui->lineEdit_username->text();
    QString password = ui->lineEdit_password->text();

    if (AuthService::authenticate(username, password)) {
        emit loginSuccess();
        accept();
    } else {
        QMessageBox::warning(this, u8"登录失败", u8"用户名或密码错误");
    }
}


void DialogLogin::on_pushButton_cancel_clicked()
{
    close();
}

