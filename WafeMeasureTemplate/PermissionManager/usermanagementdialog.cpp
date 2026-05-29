#include "usermanagementdialog.h"
#include "ui_usermanagementdialog.h"
#include "adduserdialog.h"
#include "passwordhasher.h"
#include "currentuser.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QInputDialog>
#include "delegate.h"

UserManagementDialog::UserManagementDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::UserManagementDialog)
    , model(new QSqlTableModel(this))
{
    ui->setupUi(this);

    setupTable();
}

UserManagementDialog::~UserManagementDialog()
{
    delete ui;
}

void UserManagementDialog::on_pushButton_addUser_clicked()
{
    AddUserDialog dlg(this);
    if(dlg.exec() == QDialog::Accepted){
        auto [hash, salt] = PasswordHasher::hashPassword(dlg.password());

        QSqlQuery query;
        query.prepare("INSERT INTO users (username, password_hash, salt, role) "
                      "VALUES (?, ?, ?, ?)");
        query.addBindValue(dlg.username());
        query.addBindValue(hash);
        query.addBindValue(salt);
        query.addBindValue(dlg.role());

        if(!query.exec()){
            QMessageBox::critical(this, u8"错误", u8"添加用户失败: " + query.lastError().text());
        }
        model->select();
    }
}


void UserManagementDialog::on_pushButton_editUser_clicked()
{
    QModelIndex index = ui->tableView->currentIndex();
    if(!index.isValid()) return;

    int row = index.row();
    QString oldUsername = model->data(model->index(row, 0)).toString();
    QString newUsername = QInputDialog::getText(this, u8"修改用户名",
                                                u8"新用户名:", QLineEdit::Normal, oldUsername);

    if(!newUsername.isEmpty() && newUsername != oldUsername){
        QSqlQuery query;
        query.prepare("UPDATE users SET username = ? WHERE username = ?");
        query.addBindValue(newUsername);
        query.addBindValue(oldUsername);

        if(query.exec()){
            model->select();
        } else {
            QMessageBox::critical(this, u8"错误", u8"更新失败: " + query.lastError().text());
        }
    }
}


void UserManagementDialog::on_pushButton_deleteUser_clicked()
{
    QModelIndex index = ui->tableView->currentIndex();
    if(!index.isValid()) return;

    QString username = model->data(model->index(index.row(), 0)).toString();
    if(username == "admin"){
        QMessageBox::warning(this, u8"警告", u8"不能删除管理员账户");
        return;
    }

    if(QMessageBox::question(this, u8"确认删除",
                              u8"确定要删除用户 " + username + u8" 吗？") == QMessageBox::Yes){
        QSqlQuery query;
        query.prepare("DELETE FROM users WHERE username = ?");
        query.addBindValue(username);

        if(query.exec()){
            model->select();
        } else {
            QMessageBox::critical(this, u8"错误", u8"删除失败: " + query.lastError().text());
        }
    }
}


void UserManagementDialog::on_pushButton_resetPassword_clicked()
{
    QModelIndex index = ui->tableView->currentIndex();
    if(!index.isValid()) return;

    QString username = model->data(model->index(index.row(), 0)).toString();
    QString newPassword = QInputDialog::getText(this, u8"重置密码",
                                                u8"请输入新密码:", QLineEdit::Password);

    if(!newPassword.isEmpty()){
        auto [hash, salt] = PasswordHasher::hashPassword(newPassword);

        QSqlQuery query;
        query.prepare("UPDATE users SET password_hash = ?, salt = ? WHERE username = ?");
        query.addBindValue(hash);
        query.addBindValue(salt);
        query.addBindValue(username);

        if(query.exec()){
            QMessageBox::information(this, u8"成功", u8"密码已重置");
        } else {
            QMessageBox::critical(this, u8"错误", u8"重置失败: " + query.lastError().text());
        }
    }
}


void UserManagementDialog::on_pushButton_refreshTable_clicked()
{
    model->select();
}

void UserManagementDialog::setupTable()
{
    model->setTable("users");
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    model->select(); // 确保 model 已经加载数据

    // 先移除不需要的列
    model->removeColumn(model->fieldIndex("id"));
    model->removeColumn(model->fieldIndex("password_hash"));
    model->removeColumn(model->fieldIndex("salt"));

    // 设置表头显示名称
    model->setHeaderData(model->fieldIndex("username"), Qt::Horizontal, u8"用户名");
    model->setHeaderData(model->fieldIndex("role"), Qt::Horizontal, u8"角色");
    model->setHeaderData(model->fieldIndex("created_time"), Qt::Horizontal, u8"创建时间");

    ui->tableView->setModel(model);
    ui->tableView->horizontalHeader()->setStretchLastSection(true);

    // 给 role 列设置代理，转换数字为中文
    ui->tableView->setItemDelegateForColumn(model->fieldIndex("role"), new RoleDelegate(this));
    ui->tableView->setItemDelegateForColumn(model->fieldIndex("created_time"), new DateTimeDelegate(this));
}


void UserManagementDialog::on_pushButton_close_clicked()
{
    close();
}

