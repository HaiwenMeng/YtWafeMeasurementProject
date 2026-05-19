#ifndef WAFERALGORITHMRESULT_H
#define WAFERALGORITHMRESULT_H

#include "WaferTypes.h"

#include <QString>
#include <QStringList>
#include <QVector>

namespace Wafer
{

struct LineAlgorithmDetail
{
    int lineIndex = -1;
    int pointCount = 0;
    double bow = 0.0;
    bool hasBow = false;
    double warp = 0.0;
    bool hasWarp = false;
    DirectionClass direction = DirectionClass::Other;
    double minThickness = 0.0;
    double maxThickness = 0.0;
    double averageThickness = 0.0;
    bool hasThicknessStats = false;
    QString errorMessage;
};

struct AlgorithmResult
{
    bool success = false;
    QString errorMessage;
    QStringList logs;

    double averageThk = 0.0;
    bool hasAverageThk = false;
    double centerThk = 0.0;
    bool hasCenterThk = false;
    double ttv = 0.0;
    bool hasTtv = false;
    double bow = 0.0;
    bool hasBow = false;
    double bowX = 0.0;
    bool hasBowX = false;
    double bowY = 0.0;
    bool hasBowY = false;
    double warp = 0.0;
    bool hasWarp = false;
    double warpX = 0.0;
    bool hasWarpX = false;
    double warpY = 0.0;
    bool hasWarpY = false;
    double sori = 0.0;
    bool hasSori = false;

    QVector<LineAlgorithmDetail> lineDetails;
};

} // namespace Wafer

#endif // WAFERALGORITHMRESULT_H
