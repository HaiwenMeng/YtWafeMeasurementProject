#ifndef USERMANAGEMENTDIALOG_H
#define USERMANAGEMENTDIALOG_H

#include <QDialog>
#include <QSqlTableModel>

namespace Ui {
class UserManagementDialog;
}

class UserManagementDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UserManagementDialog(QWidget *parent = nullptr);
    ~UserManagementDialog();

private slots:
    void on_pushButton_addUser_clicked();

    void on_pushButton_editUser_clicked();

    void on_pushButton_deleteUser_clicked();

    void on_pushButton_resetPassword_clicked();

    void on_pushButton_refreshTable_clicked();

    void on_pushButton_close_clicked();

private:
    Ui::UserManagementDialog *ui;

    QSqlTableModel *model;
    void setupTable();
};

#endif // USERMANAGEMENTDIALOG_H
