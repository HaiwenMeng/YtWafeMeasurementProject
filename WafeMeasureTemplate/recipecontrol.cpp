#include "recipecontrol.h"
#include "ui_recipecontrol.h"

RecipeControl::RecipeControl(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::RecipeControl)
{
    ui->setupUi(this);
}

RecipeControl::~RecipeControl()
{
    delete ui;
}
void RecipeControl::on_lineEdit_recipeName_textChanged(const QString &arg1)
{
    
}



