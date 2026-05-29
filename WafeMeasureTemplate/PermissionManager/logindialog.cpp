// logindialog.cpp
#include "logindialog.h"
#include "ui_logindialog.h"
#include "authservice.h"
#include <QMessageBox>

LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    connect(ui->btnLogin, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::onLoginClicked()
{
    QString username = ui->txtUsername->text();
    QString password = ui->txtPassword->text();
    
    if (AuthService::authenticate(username, password)) {
        emit loginSuccess();
        accept();
    } else {
        QMessageBox::warning(this, "登录失败", "用户名或密码错误");
    }
}