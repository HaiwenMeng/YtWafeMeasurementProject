#include "recipe_manager_dialog.h"
#include "ui_recipe_manager_dialog.h"

#include <QAbstractItemView>
#include <QMessageBox>
#include <QTableWidgetItem>

RecipeManagerDialog::RecipeManagerDialog(RecipeDatabase *database, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::RecipeManagerDialog),
      m_database(database)
{
    ui->setupUi(this);
    ui->table_recipes->setColumnCount(6);
    ui->table_recipes->setHorizontalHeaderLabels(QStringList()
        << QString(u8"程序")
        << QString(u8"路径")
        << QString(u8"厚度")
        << QString(u8"尺寸")
        << QString(u8"裁边")
        << QString(u8"点数"));
    ui->table_recipes->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->table_recipes->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->table_recipes->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->combo_path->addItems(QStringList() << QStringLiteral("1") << QStringLiteral("2") << QStringLiteral("4") << QStringLiteral("6") << QStringLiteral("8"));
    ui->spin_size->setRange(1, 12);
    ui->spin_thickness->setRange(1, 5000);
    ui->double_trim->setRange(0.0, 100.0);
    ui->double_trim->setDecimals(3);
    ui->spin_sample->setRange(2, 20000);

    connect(ui->button_add, &QPushButton::clicked, this, &RecipeManagerDialog::onAddClicked);
    connect(ui->button_update, &QPushButton::clicked, this, &RecipeManagerDialog::onUpdateClicked);
    connect(ui->button_delete, &QPushButton::clicked, this, &RecipeManagerDialog::onDeleteClicked);
    connect(ui->button_close, &QPushButton::clicked, this, &RecipeManagerDialog::accept);
    connect(ui->table_recipes, &QTableWidget::itemSelectionChanged, this, &RecipeManagerDialog::onSelectionChanged);

    reload();
}

RecipeManagerDialog::~RecipeManagerDialog()
{
    delete ui;
}

void RecipeManagerDialog::onAddClicked()
{
    ProRecipe recipe = recipeFromUi();
    recipe.id = -1;
    recipe.name = makeUniqueRecipeName(recipe.name);
    QString error;
    if (!m_database->saveRecipe(recipe, &error)) {
        showError(error);
        return;
    }
    reload();
    emit recipesChanged();
}

void RecipeManagerDialog::onUpdateClicked()
{
    ProRecipe recipe = recipeFromUi();
    recipe.id = currentRecipeId();
    if (recipe.id <= 0) {
        showError(QStringLiteral("No recipe row is selected."));
        return;
    }
    QString error;
    if (!m_database->saveRecipe(recipe, &error)) {
        showError(error);
        return;
    }
    reload();
    emit recipesChanged();
}

void RecipeManagerDialog::onDeleteClicked()
{
    const int id = currentRecipeId();
    if (id <= 0) {
        showError(QStringLiteral("No recipe row is selected."));
        return;
    }
    if (QMessageBox::question(this, QString(u8"确认删除"),
                              QString(u8"确定删除当前配方吗?")) != QMessageBox::Yes) {
        return;
    }
    QString error;
    if (!m_database->deleteRecipe(id, &error)) {
        showError(error);
        return;
    }
    reload();
    emit recipesChanged();
}

void RecipeManagerDialog::onSelectionChanged()
{
    const int row = ui->table_recipes->currentRow();
    if (row < 0 || row >= m_recipes.size()) {
        clearEditor();
        return;
    }
    showRecipe(m_recipes.at(row));
}

void RecipeManagerDialog::reload()
{
    QString error;
    m_recipes = m_database->recipes(&error);
    if (!error.isEmpty()) {
        showError(error);
    }
    ui->table_recipes->setRowCount(m_recipes.size());
    for (int row = 0; row < m_recipes.size(); ++row) {
        const ProRecipe &recipe = m_recipes.at(row);
        ui->table_recipes->setItem(row, 0, new QTableWidgetItem(recipe.name));
        ui->table_recipes->setItem(row, 1, new QTableWidgetItem(QString::number(recipe.measurePath)));
        ui->table_recipes->setItem(row, 2, new QTableWidgetItem(QString::number(recipe.productThickness)));
        ui->table_recipes->setItem(row, 3, new QTableWidgetItem(QString::number(recipe.productSize)));
        ui->table_recipes->setItem(row, 4, new QTableWidgetItem(QString::number(recipe.trimSize, 'f', 3)));
        ui->table_recipes->setItem(row, 5, new QTableWidgetItem(QString::number(recipe.lineSampleNum)));
    }
    ui->table_recipes->resizeColumnsToContents();
    if (!m_recipes.isEmpty()) {
        ui->table_recipes->selectRow(0);
    } else {
        clearEditor();
    }
}

ProRecipe RecipeManagerDialog::recipeFromUi() const
{
    ProRecipe recipe;
    recipe.id = currentRecipeId();
    recipe.name = ui->line_name->text().trimmed();
    recipe.measurePath = ui->combo_path->currentText().toInt();
    recipe.productThickness = ui->spin_thickness->value();
    recipe.productSize = ui->spin_size->value();
    recipe.trimSize = ui->double_trim->value();
    recipe.lineSampleNum = ui->spin_sample->value();
    return recipe;
}

void RecipeManagerDialog::showRecipe(const ProRecipe &recipe)
{
    ui->line_name->setText(recipe.name);
    const int pathIndex = ui->combo_path->findText(QString::number(recipe.measurePath));
    ui->combo_path->setCurrentIndex(pathIndex >= 0 ? pathIndex : 0);
    ui->spin_thickness->setValue(recipe.productThickness);
    ui->spin_size->setValue(recipe.productSize);
    ui->double_trim->setValue(recipe.trimSize);
    ui->spin_sample->setValue(recipe.lineSampleNum);
}

void RecipeManagerDialog::clearEditor()
{
    ui->line_name->clear();
    ui->combo_path->setCurrentIndex(0);
    ui->spin_thickness->setValue(775);
    ui->spin_size->setValue(8);
    ui->double_trim->setValue(2.0);
    ui->spin_sample->setValue(51);
}

QString RecipeManagerDialog::makeUniqueRecipeName(const QString &baseName) const
{
    QString base = baseName.trimmed();
    if (base.isEmpty()) {
        base = QStringLiteral("Recipe");
    }
    if (!hasRecipeName(base, -1)) {
        return base;
    }

    for (int index = 1; index < 10000; ++index) {
        const QString candidate = QStringLiteral("%1-%2").arg(base).arg(index);
        if (!hasRecipeName(candidate, -1)) {
            return candidate;
        }
    }
    return QStringLiteral("%1-%2").arg(base).arg(m_recipes.size() + 1);
}

bool RecipeManagerDialog::hasRecipeName(const QString &name, int excludeId) const
{
    const QString trimmedName = name.trimmed();
    for (const ProRecipe &recipe : m_recipes) {
        if (recipe.id != excludeId && recipe.name.compare(trimmedName, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

int RecipeManagerDialog::currentRecipeId() const
{
    const int row = ui->table_recipes->currentRow();
    if (row < 0 || row >= m_recipes.size()) {
        return -1;
    }
    return m_recipes.at(row).id;
}

void RecipeManagerDialog::showError(const QString &message)
{
    QMessageBox::warning(this, QString(u8"配方管理"), message);
}
