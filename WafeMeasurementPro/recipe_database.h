#ifndef WAFEMEASUREMENTPRO_RECIPE_DATABASE_H
#define WAFEMEASUREMENTPRO_RECIPE_DATABASE_H

#include "pro_types.h"

#include <QList>
#include <QObject>
#include <QSqlDatabase>

class RecipeDatabase : public QObject
{
    Q_OBJECT

public:
    explicit RecipeDatabase(QObject *parent = nullptr);
    ~RecipeDatabase() override;

    bool open(QString *errorMessage);
    QString databasePath() const;
    QList<ProRecipe> recipes(QString *errorMessage) const;
    bool saveRecipe(const ProRecipe &recipe, QString *errorMessage);
    bool deleteRecipe(int id, QString *errorMessage);
    bool ensureDefaultRecipe(QString *errorMessage);

private:
    bool createTables(QString *errorMessage);
    QString resolveDatabasePath(QString *errorMessage) const;
    bool directoryWritable(const QString &dirPath) const;
    QString connectionName() const;

private:
    QString m_connectionName;
    QString m_dbPath;
    mutable QSqlDatabase m_db;
};

#endif // WAFEMEASUREMENTPRO_RECIPE_DATABASE_H
