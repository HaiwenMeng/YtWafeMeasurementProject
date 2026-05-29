#ifndef RECIPESETTINGDIALOG_H
#define RECIPESETTINGDIALOG_H
#include <QDialog>
#include <QSqlTableModel>
#include <QSqlRecord>
namespace Ui {
class RecipeSettingDialog;
}

class RecipeSettingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RecipeSettingDialog(QWidget *parent = nullptr);
    ~RecipeSettingDialog();
    QList<QString> InitRecipes(int defaultTrimSize);
    QSqlRecord FindRecipeByName(QString name);
    QList<int> FindThicknessName();
    QString DefaultPath = "";
    QString DefaultThickness = "";
    QString DefaultSize = "";
    QString newRecipeName = "";
    bool ResetRecipe();
private slots:
    void on_pushButton_addRecipe_clicked();

    void on_pushButton_editRecipe_clicked();

    void on_pushButton_deleteRecipe_clicked();

    void on_pushButton_refreshTable_clicked();

 

    void on_pushButton_close_clicked();

    void on_pushButton_importRecipe_clicked();

    void on_pushButton_exportRecipe_clicked();

private:
    Ui::RecipeSettingDialog *ui;

    QSqlTableModel *model;
    void setupTable();

    int DefaultTrimSize = 0;
};

#endif // RECIPESETTINGDIALOG_H
