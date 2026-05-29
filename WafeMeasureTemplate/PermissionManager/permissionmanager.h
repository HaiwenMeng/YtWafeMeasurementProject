// permissionmanager.h
#ifndef PERMISSIONMANAGER_H
#define PERMISSIONMANAGER_H

#include <QObject>
#include <QSet>

class PermissionManager : public QObject
{
    Q_OBJECT
public:
    static PermissionManager& instance();
    
    void loadPermissions(int role);
    bool hasPermission(const QString& module) const;

signals:
    void permissionsChanged();

private:
    explicit PermissionManager(QObject *parent = nullptr);
    
    QSet<QString> m_allowedModules;
};

#endif // PERMISSIONMANAGER_H