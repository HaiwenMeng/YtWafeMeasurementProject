#include "recipe_database.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QVariant>

RecipeDatabase::RecipeDatabase(QObject *parent)
    : QObject(parent),
      m_connectionName(QStringLiteral("WafeMeasurementProRecipeConnection_%1").arg(reinterpret_cast<quintptr>(this)))
{
}

RecipeDatabase::~RecipeDatabase()
{
    if (m_db.isValid()) {
        m_db.close();
        m_db = QSqlDatabase();
    }
    QSqlDatabase::removeDatabase(m_connectionName);
}

bool RecipeDatabase::open(QString *errorMessage)
{
    if (m_db.isOpen()) {
        return true;
    }
    if (!QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE"))) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("QSQLITE driver is not available.");
        }
        return false;
    }

    const QString dbPath = resolveDatabasePath(errorMessage);
    if (dbPath.isEmpty()) {
        return false;
    }

    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_db.setDatabaseName(dbPath);
    if (!m_db.open()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Open recipe database failed: %1, %2").arg(dbPath, m_db.lastError().text());
        }
        return false;
    }
    m_dbPath = dbPath;
    if (!createTables(errorMessage)) {
        return false;
    }
    return ensureDefaultRecipe(errorMessage);
}

QString RecipeDatabase::databasePath() const
{
    return m_dbPath;
}

QList<ProRecipe> RecipeDatabase::recipes(QString *errorMessage) const
{
    QList<ProRecipe> list;
    if (!m_db.isOpen()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Recipe database is not open.");
        }
        return list;
    }

    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral("SELECT id,name,measure_path,product_thickness,product_size,trim_size,line_sample_num FROM recipes ORDER BY name"))) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Load recipes failed: %1").arg(query.lastError().text());
        }
        return list;
    }
    while (query.next()) {
        ProRecipe recipe;
        recipe.id = query.value(0).toInt();
        recipe.name = query.value(1).toString();
        recipe.measurePath = query.value(2).toInt();
        recipe.productThickness = query.value(3).toInt();
        recipe.productSize = query.value(4).toInt();
        recipe.trimSize = query.value(5).toDouble();
        recipe.lineSampleNum = query.value(6).toInt();
        list.append(recipe);
    }
    return list;
}

bool RecipeDatabase::saveRecipe(const ProRecipe &recipe, QString *errorMessage)
{
    if (!m_db.isOpen()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Recipe database is not open.");
        }
        return false;
    }
    if (recipe.name.trimmed().isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Recipe name is empty.");
        }
        return false;
    }
    if (!(recipe.measurePath == 1 || recipe.measurePath == 2 || recipe.measurePath == 4 ||
          recipe.measurePath == 6 || recipe.measurePath == 8)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Measure path must be 1, 2, 4, 6 or 8.");
        }
        return false;
    }
    if (recipe.productSize <= 0 || recipe.productThickness <= 0 || recipe.lineSampleNum <= 1 || recipe.trimSize < 0.0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Recipe numeric values are invalid.");
        }
        return false;
    }

    QSqlQuery existsQuery(m_db);
    existsQuery.prepare(QStringLiteral("SELECT id FROM recipes WHERE lower(name)=lower(?) AND id<>? LIMIT 1"));
    existsQuery.addBindValue(recipe.name.trimmed());
    existsQuery.addBindValue(recipe.id > 0 ? recipe.id : -1);
    if (!existsQuery.exec()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Check recipe name failed: %1").arg(existsQuery.lastError().text());
        }
        return false;
    }
    if (existsQuery.next()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Recipe name already exists: %1").arg(recipe.name.trimmed());
        }
        return false;
    }

    QSqlQuery query(m_db);
    if (recipe.id > 0) {
        query.prepare(QStringLiteral("UPDATE recipes SET name=?, measure_path=?, product_thickness=?, product_size=?, trim_size=?, line_sample_num=? WHERE id=?"));
        query.addBindValue(recipe.name.trimmed());
        query.addBindValue(recipe.measurePath);
        query.addBindValue(recipe.productThickness);
        query.addBindValue(recipe.productSize);
        query.addBindValue(recipe.trimSize);
        query.addBindValue(recipe.lineSampleNum);
        query.addBindValue(recipe.id);
    } else {
        query.prepare(QStringLiteral("INSERT INTO recipes(name,measure_path,product_thickness,product_size,trim_size,line_sample_num) VALUES(?,?,?,?,?,?)"));
        query.addBindValue(recipe.name.trimmed());
        query.addBindValue(recipe.measurePath);
        query.addBindValue(recipe.productThickness);
        query.addBindValue(recipe.productSize);
        query.addBindValue(recipe.trimSize);
        query.addBindValue(recipe.lineSampleNum);
    }
    if (!query.exec()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Save recipe failed: %1").arg(query.lastError().text());
        }
        return false;
    }
    return true;
}

bool RecipeDatabase::deleteRecipe(int id, QString *errorMessage)
{
    if (!m_db.isOpen()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Recipe database is not open.");
        }
        return false;
    }
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("DELETE FROM recipes WHERE id=?"));
    query.addBindValue(id);
    if (!query.exec()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Delete recipe failed: %1").arg(query.lastError().text());
        }
        return false;
    }
    return true;
}

bool RecipeDatabase::ensureDefaultRecipe(QString *errorMessage)
{
    QSqlQuery countQuery(m_db);
    if (!countQuery.exec(QStringLiteral("SELECT COUNT(*) FROM recipes")) || !countQuery.next()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Check default recipe failed: %1").arg(countQuery.lastError().text());
        }
        return false;
    }
    if (countQuery.value(0).toInt() > 0) {
        return true;
    }

    ProRecipe recipe;
    recipe.name = QStringLiteral("Default-8Line");
    recipe.measurePath = 8;
    recipe.productThickness = 775;
    recipe.productSize = 8;
    recipe.trimSize = 2.0;
    recipe.lineSampleNum = 51;
    return saveRecipe(recipe, errorMessage);
}

bool RecipeDatabase::createTables(QString *errorMessage)
{
    QSqlQuery query(m_db);
    const QString sql = QStringLiteral(
        "CREATE TABLE IF NOT EXISTS recipes ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT UNIQUE NOT NULL,"
        "measure_path INTEGER NOT NULL,"
        "product_thickness INTEGER NOT NULL,"
        "product_size INTEGER NOT NULL,"
        "trim_size REAL NOT NULL,"
        "line_sample_num INTEGER NOT NULL,"
        "created_time DATETIME DEFAULT CURRENT_TIMESTAMP)");
    if (!query.exec(sql)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Create recipe table failed: %1").arg(query.lastError().text());
        }
        return false;
    }
    return true;
}

QString RecipeDatabase::resolveDatabasePath(QString *errorMessage) const
{
    const QString dbFileName = QStringLiteral("WafeMeasurementProRecipes.db");
    const QDir appDir(QCoreApplication::applicationDirPath());
    if (directoryWritable(appDir.absolutePath())) {
        return appDir.filePath(dbFileName);
    }

    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (appDataPath.isEmpty()) {
        appDataPath = QDir::home().filePath(QStringLiteral("AppData/Roaming/WafeMeasurementPro"));
    }

    QDir appDataDir(appDataPath);
    if (!appDataDir.exists() && !appDataDir.mkpath(QStringLiteral("."))) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Create recipe database directory failed: %1").arg(appDataDir.absolutePath());
        }
        return QString();
    }
    if (!directoryWritable(appDataDir.absolutePath())) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Recipe database directory is not writable: %1").arg(appDataDir.absolutePath());
        }
        return QString();
    }
    return appDataDir.filePath(dbFileName);
}

bool RecipeDatabase::directoryWritable(const QString &dirPath) const
{
    QDir dir(dirPath);
    if (!dir.exists()) {
        return false;
    }

    const QString probeName = QStringLiteral(".wafemeasurementpro_write_test_%1_%2.tmp")
                                  .arg(QCoreApplication::applicationPid())
                                  .arg(QDateTime::currentMSecsSinceEpoch());
    const QString probePath = dir.filePath(probeName);
    QFile probe(probePath);
    if (!probe.open(QIODevice::WriteOnly)) {
        return false;
    }
    probe.close();
    return QFile::remove(probePath);
}

QString RecipeDatabase::connectionName() const
{
    return m_connectionName;
}
