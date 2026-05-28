#ifndef WAFEMEASUREMENTPRO_RECIPE_MANAGER_DIALOG_H
#define WAFEMEASUREMENTPRO_RECIPE_MANAGER_DIALOG_H

#include "pro_types.h"
#include "recipe_database.h"

#include <QDialog>
#include <QList>

namespace Ui {
class RecipeManagerDialog;
}

class RecipeManagerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RecipeManagerDialog(RecipeDatabase *database, QWidget *parent = nullptr);
    ~RecipeManagerDialog() override;

signals:
    void recipesChanged();

private slots:
    void onAddClicked();
    void onUpdateClicked();
    void onDeleteClicked();
    void onSelectionChanged();

private:
    void reload();
    ProRecipe recipeFromUi() const;
    void showRecipe(const ProRecipe &recipe);
    void clearEditor();
    QString makeUniqueRecipeName(const QString &baseName) const;
    bool hasRecipeName(const QString &name, int excludeId) const;
    int currentRecipeId() const;
    void showError(const QString &message);

private:
    Ui::RecipeManagerDialog *ui;
    RecipeDatabase *m_database;
    QList<ProRecipe> m_recipes;
};

#endif // WAFEMEASUREMENTPRO_RECIPE_MANAGER_DIALOG_H
