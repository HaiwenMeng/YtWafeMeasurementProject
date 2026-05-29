// permissionmanager.cpp
#include "permissionmanager.h"
#include <QSqlQuery>
#include <QJsonDocument>
#include <QJsonArray>

PermissionManager& PermissionManager::instance()
{
    static PermissionManager instance;
    return instance;
}

PermissionManager::PermissionManager(QObject *parent) : QObject(parent) {}

void PermissionManager::loadPermissions(int role)
{
    m_allowedModules.clear();
    
    QSqlQuery query;
    query.prepare("SELECT modules FROM role_permissions WHERE role = ?");
    query.addBindValue(role);
    
    if (query.exec() && query.next()) {
        QJsonDocument doc = QJsonDocument::fromJson(query.value(0).toByteArray());
        QJsonArray modulesArray = doc.array();
        for (const auto& module : modulesArray) {
            m_allowedModules.insert(module.toString());
        }
    }
    emit permissionsChanged();
}

bool PermissionManager::hasPermission(const QString& module) const
{
    return m_allowedModules.contains(module);
}
