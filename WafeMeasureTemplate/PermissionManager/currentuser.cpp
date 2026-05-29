// currentuser.cpp
#include "currentuser.h"

CurrentUser& CurrentUser::instance()
{
    static CurrentUser instance;
    return instance;
}

CurrentUser::CurrentUser(QObject *parent) : QObject(parent) {}

void CurrentUser::login(const QString& username, int role)
{
    m_username = username;
    m_role = role;
    m_loggedIn = true;
    emit userChanged();
}

void CurrentUser::logout()
{
    m_username.clear();
    m_role = -1;
    m_loggedIn = false;
    emit userChanged();
}