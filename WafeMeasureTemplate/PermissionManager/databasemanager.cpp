// databasemanager.cpp
#include "databasemanager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QStandardPaths>
#include "passwordhasher.h"

DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager instance;
    return instance;
}

DatabaseManager::DatabaseManager(QObject *parent) : QObject(parent) {}

bool DatabaseManager::initializeDatabase(const QString& path)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(path);
    
    if (!db.open()) {
        qCritical() << "Cannot open database:" << db.lastError();
        return false;
    }
    
    createTables();
    createRecipeTables();
    createDefaultAdmin();
    return true;
}

void DatabaseManager::createTables()
{
    QSqlQuery query;
    query.exec("CREATE TABLE IF NOT EXISTS users ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "username TEXT UNIQUE NOT NULL,"
               "password_hash TEXT NOT NULL,"
               "salt TEXT NOT NULL,"
               "role INTEGER NOT NULL,"
               "created_time DATETIME DEFAULT CURRENT_TIMESTAMP)");

    query.exec("CREATE TABLE IF NOT EXISTS role_permissions ("
               "role INTEGER PRIMARY KEY,"
               "modules TEXT NOT NULL)");
}

void DatabaseManager::createRecipeTables()
{
    QSqlQuery query;
  /*  bool state11 = query.exec("DROP TABLE IF EXISTS recipes");*/

    bool state=query.exec("CREATE TABLE IF NOT EXISTS recipes ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "recipename TEXT UNIQUE NOT NULL,"
        "motion_path TEXT NOT NULL,"
        "product_thickness TEXT NOT NULL,"
        "product_size TEXT NOT NULL,"
        "trim_size INTEGER NOT NULL,"
        "MinBOW DOUBLE NOT NULL,"
        "MaxBOW DOUBLE NOT NULL,"
        "IsInvalidBOW bool NOT NULL,"
        "MinWARP DOUBLE NOT NULL,"
        "MaxWARP DOUBLE NOT NULL,"
        "IsInvalidWARP bool NOT NULL,"
        "MinCTHK DOUBLE NOT NULL,"
        "MaxCTHK DOUBLE NOT NULL,"
        "IsInvalidCTHK bool NOT NULL,"
        "MinATHK DOUBLE NOT NULL,"
        "MaxATHK DOUBLE NOT NULL,"
        "IsInvalidATHK bool NOT NULL,"
        "MinTTV DOUBLE NOT NULL,"
        "MaxTTV DOUBLE NOT NULL,"
        "IsInvalidTTV bool NOT NULL,"
        "MinSORI DOUBLE NOT NULL,"
        "MaxSORI DOUBLE NOT NULL,"
        "IsInvalidSORI bool NOT NULL,"
        "created_time DATETIME DEFAULT CURRENT_TIMESTAMP)");

   /* QSqlQuery checkQuery2("SELECT COUNT(*) FROM recipes");
    if (checkQuery2.next() && checkQuery2.value(0).toInt() == 0)
    {

        QSqlQuery query;
        query.prepare("INSERT INTO recipes (recipename, motion_path, product_thickness, product_size, trim_size, MinBOW, MaxBOW, IsInvalidBOW, MinWARP, MaxWARP, IsInvalidWARP, MinCTHK, MaxCTHK, IsInvalidCTHK, MinATHK, MaxATHK, IsInvalidATHK, MinTTV, MaxTTV, IsInvalidTTV, MinSORI, MaxSORI, IsInvalidSORI) "
            "VALUES ('default', '8线', '660um', '12英寸', 2, 0, 0, false, 0, 0, false, 0, 0, false, 0, 0, false, 0, 0, false, 0, 0, false)");

        bool state = query.exec();
    }*/

    QSqlQuery query1;
    bool state1 = query1.exec("CREATE TABLE IF NOT EXISTS thicknesses ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "thicknessname INTEGER UNIQUE NOT NULL,"
        "created_time DATETIME DEFAULT CURRENT_TIMESTAMP)");
    QSqlQuery checkQuery1("SELECT COUNT(*) FROM thicknesses");
    if (checkQuery1.next() && checkQuery1.value(0).toInt() <5)
    {

        QSqlQuery query;
        query.prepare("INSERT INTO thicknesses (thicknessname) "
            "VALUES (660)");
        bool state = query.exec();
        query.prepare("INSERT INTO thicknesses (thicknessname) "
            "VALUES (700)");
        state = query.exec();
        query.prepare("INSERT INTO thicknesses (thicknessname) "
            "VALUES (750)");
        state = query.exec();
        query.prepare("INSERT INTO thicknesses (thicknessname) "
            "VALUES (775)");
        state = query.exec();
        query.prepare("INSERT INTO thicknesses (thicknessname) "
            "VALUES (800)");
        state = query.exec();
    }
}

void DatabaseManager::createDefaultAdmin()
{
    QSqlQuery checkQuery("SELECT COUNT(*) FROM users WHERE role = 0");
    if (checkQuery.next() && checkQuery.value(0).toInt() == 0) {
        auto [hash, salt] = PasswordHasher::hashPassword("admin123");
        QSqlQuery insertQuery;
        insertQuery.prepare("INSERT INTO users (username, password_hash, salt, role) "
            "VALUES ('admin', ?, ?, 0)");
        insertQuery.addBindValue(hash);
        insertQuery.addBindValue(salt);
        insertQuery.exec();
    }

}
