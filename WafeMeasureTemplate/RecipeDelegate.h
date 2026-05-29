#ifndef RECIPEDELEGATE_H
#define RECIPEDELEGATE_H

#include <QObject>
#include <QStyledItemDelegate>
#include <QDateTime>
#include "./PermissionManager/delegate.h"

class RecipeDelegate : public RoleDelegate
{
    Q_OBJECT
public:
    explicit RecipeDelegate(QObject* parent = nullptr);

protected:
    QString displayText(const QVariant& value, const QLocale&) const override;

signals:
};



#endif // RECIPEDELEGATE_H
