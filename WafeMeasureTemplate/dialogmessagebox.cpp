#include "dialogmessagebox.h"
#include "ui_dialogmessagebox.h"

DialogMessageBox::DialogMessageBox(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogMessageBox)
{
    ui->setupUi(this);
}

DialogMessageBox::~DialogMessageBox()
{
    delete ui;
}

void DialogMessageBox::on_pushButton_ok_clicked()
{
    if(ui->radioButton_500_1100->isChecked())
    {
        m_bSelStandard_1550 = false;
    }
    else
    {
        m_bSelStandard_1550 = true;
    }
    accept();
}


void DialogMessageBox::on_pushButton_cancel_clicked()
{
    close();
}

