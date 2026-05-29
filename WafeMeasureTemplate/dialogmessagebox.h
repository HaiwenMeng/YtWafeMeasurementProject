#ifndef DIALOGMESSAGEBOX_H
#define DIALOGMESSAGEBOX_H

#include <QDialog>

namespace Ui {
class DialogMessageBox;
}

class DialogMessageBox : public QDialog
{
    Q_OBJECT

public:
    explicit DialogMessageBox(QWidget *parent = nullptr);
    ~DialogMessageBox();

    inline bool GetSelStandard1550Flag() { return m_bSelStandard_1550; }

private slots:
    void on_pushButton_ok_clicked();

    void on_pushButton_cancel_clicked();

private:
    Ui::DialogMessageBox *ui;

    bool m_bSelStandard_1550 = false;
};

#endif // DIALOGMESSAGEBOX_H
