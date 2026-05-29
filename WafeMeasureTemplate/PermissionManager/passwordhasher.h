#ifndef PASSWORDHASHER_H
#define PASSWORDHASHER_H

#include <QObject>
#include <QByteArray>
#include <QPair>

class PasswordHasher : public QObject
{
    Q_OBJECT
public:
    explicit PasswordHasher(QObject *parent = nullptr);

    static QPair<QByteArray, QByteArray> hashPassword(const QString& password);
    static bool validatePassword(const QString& password,
                                 const QByteArray& storedHash,
                                 const QByteArray& salt);

signals:

private:
    static QByteArray generateSalt();
    static QByteArray calculateHash(const QString& password, const QByteArray& salt);
};

#endif // PASSWORDHASHER_H
