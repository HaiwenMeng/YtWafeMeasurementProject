// currentuser.h
#ifndef CURRENTUSER_H
#define CURRENTUSER_H

#include <QObject>

class CurrentUser : public QObject
{
    Q_OBJECT
public:
    static CurrentUser& instance();
    
    void login(const QString& username, int role);
    void logout();
    
    QString username() const { return m_username; }
    int role() const { return m_role; }
    bool isLoggedIn() const { return m_loggedIn; }

signals:
    void userChanged();

private:
    explicit CurrentUser(QObject *parent = nullptr);
    
    QString m_username;
    int m_role = -1;
    bool m_loggedIn = false;
};

#endif // CURRENTUSER_H