#ifndef AUTHSERVICE_H
#define AUTHSERVICE_H

#include <QObject>

class AuthService : public QObject
{
    Q_OBJECT
public:
    explicit AuthService(QObject *parent = nullptr);

    static bool authenticate(const QString& username, const QString& password);
signals:
};

#endif // AUTHSERVICE_H
