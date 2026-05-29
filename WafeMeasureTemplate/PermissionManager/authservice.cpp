#include "authservice.h"
#include "currentuser.h"
#include "passwordhasher.h"
#include <QSqlQuery>
#include <QVariant>

AuthService::AuthService(QObject *parent)
    : QObject{parent}
{}

bool AuthService::authenticate(const QString& username, const QString& password)
{
    QSqlQuery query;
    query.prepare("SELECT password_hash, salt, role FROM users WHERE username = ?");
    query.addBindValue(QVariant(username));

    if (!query.exec() || !query.next()) return false;

    QByteArray storedHash = query.value(0).toByteArray();
    QByteArray salt = query.value(1).toByteArray();
    int role = query.value(2).toInt();

    if (PasswordHasher::validatePassword(password, storedHash, salt)) {
        CurrentUser::instance().login(username, role);
        return true;
    }
    return false;
}
