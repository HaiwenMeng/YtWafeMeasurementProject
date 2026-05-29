#ifndef ADDUSERDIALOG_H
#define ADDUSERDIALOG_H

#include <QDialog>

namespace Ui {
class AddUserDialog;
}

class AddUserDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddUserDialog(QWidget *parent = nullptr);
    ~AddUserDialog();

    QString username() const;
    QString password() const;
    int role() const;

private slots:
    void on_pushButton_add_clicked();

    void on_pushButton_cancel_clicked();

    void on_lineEdit_username_textChanged(const QString &arg1);

    void on_lineEdit_password_textChanged(const QString &arg1);

    void on_lineEdit_confirm_textChanged(const QString &arg1);

private:
    Ui::AddUserDialog *ui;
};

#endif // ADDUSERDIALOG_H
