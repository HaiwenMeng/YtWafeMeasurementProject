#include "RecipeDelegate.h"
RecipeDelegate::RecipeDelegate(QObject* parent)
    : RoleDelegate{ parent }
{}

QString  RecipeDelegate::displayText(const QVariant& value, const QLocale&) const
{
    int roleValue = value.toInt();
    switch (roleValue)
    {
    case 0: return u8"����Ա";
    case 1: return u8"����ʦ";
    case 2: return u8"������";
    default: return u8"δ֪";
    }
}
