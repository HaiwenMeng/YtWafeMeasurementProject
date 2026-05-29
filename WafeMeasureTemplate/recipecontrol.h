#ifndef RECIPECONTROL_H
#define RECIPECONTROL_H

#include <QWidget>
#include <recipecontrol.h>
namespace Ui {
class RecipeControl;
}

class RecipeControl : public QWidget
{
    Q_OBJECT

public:
    explicit RecipeControl(QWidget *parent = nullptr);
    ~RecipeControl();

private slots:
    void on_lineEdit_recipeName_textChanged(const QString &arg1);

private:
    Ui::RecipeControl *ui;
};

#endif // RECIPECONTROL_H
