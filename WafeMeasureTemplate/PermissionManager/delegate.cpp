#include "delegate.h"

RoleDelegate::RoleDelegate(QObject *parent)
    : QStyledItemDelegate{parent}
{}

QString RoleDelegate::displayText(const QVariant &value, const QLocale &) const
{
    int roleValue = value.toInt();
    switch (roleValue)
    {
    case 0: return u8"管理员";
    case 1: return u8"工程师";
    case 2: return u8"操作工";
    default: return u8"未知";
    }
}

DateTimeDelegate::DateTimeDelegate(QObject *parent)
    : QStyledItemDelegate{parent}
{}

QString DateTimeDelegate::displayText(const QVariant &value, const QLocale &locale) const
{
    if (value.userType() == QMetaType::QString) {
        QDateTime dt = QDateTime::fromString(value.toString(), "yyyy-MM-dd hh:mm:ss");
        dt.setTimeSpec(Qt::UTC); // 明确指定为UTC时间
        return dt.toLocalTime().toString("yyyy-MM-dd hh:mm:ss");
    }
    return QStyledItemDelegate::displayText(value, locale);
}
