#ifndef DELEGATE_H
#define DELEGATE_H

#include <QObject>
#include <QStyledItemDelegate>
#include <QDateTime>

class RoleDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit RoleDelegate(QObject *parent = nullptr);

protected:
   virtual QString displayText(const QVariant &value, const QLocale &) const override;

signals:
};

class DateTimeDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit DateTimeDelegate(QObject *parent = nullptr);

protected:
    QString displayText(const QVariant &value, const QLocale &) const override;

signals:
};


#endif // DELEGATE_H
