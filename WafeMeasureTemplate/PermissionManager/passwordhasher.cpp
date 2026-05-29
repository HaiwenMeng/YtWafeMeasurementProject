#include "passwordhasher.h"
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QString>

PasswordHasher::PasswordHasher(QObject *parent)
    : QObject{parent}
{}

QPair<QByteArray, QByteArray> PasswordHasher::hashPassword(const QString& password)
{
    QByteArray salt = generateSalt();
    QByteArray hash = calculateHash(password, salt);
    return qMakePair(hash, salt);
}

bool PasswordHasher::validatePassword(const QString& password,
                                      const QByteArray& storedHash,
                                      const QByteArray& salt)
{
    return calculateHash(password, salt) == storedHash;
}

QByteArray PasswordHasher::generateSalt()
{
    QByteArray salt(32, 0);
    QRandomGenerator::global()->fillRange(reinterpret_cast<quint32*>(salt.data()), salt.size()/4);
    return salt;
}

QByteArray PasswordHasher::calculateHash(const QString& password, const QByteArray& salt)
{
    QCryptographicHash hasher(QCryptographicHash::Sha256);
    hasher.addData(password.toUtf8());
    hasher.addData(salt);
    return hasher.result();
}
