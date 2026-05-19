#ifndef WAFERTYPES_H
#define WAFERTYPES_H

#include <QMetaType>
#include <QString>
#include <QVector>

namespace Wafer
{

enum class UnitMode
{
    Millimeter = 0,
    Micrometer = 1
};

enum class DirectionClass
{
    X = 0,
    Y = 1,
    Other = 2
};

struct DataPoint
{
    double x = 0.0;
    double y = 0.0;
    double topDistance = 0.0;
    double bottomDistance = 0.0;
    double thickness = 0.0;
    double zm = 0.0;
    int xEncoder = 0;
    int yEncoder = 0;
    int lineIndex = -1;

    bool hasTopDistance = false;
    bool hasBottomDistance = false;
    bool hasThickness = false;
    bool hasZm = false;
    bool hasZGravity = false;
    double zGravity = 0.0;
};

struct LineData
{
    int lineIndex = -1;
    QVector<DataPoint> points;
};

struct CenterPoint
{
    bool valid = false;
    DataPoint point;
};

struct WaferDataset
{
    int lineCount = 0;
    int productSize = 8;
    double nominalThickness = 0.0;
    double calibrationTotal = 0.0;
    double centerX = 0.0;
    double centerY = 0.0;
    QVector<LineData> lines;
    CenterPoint centerPoint;
    bool containsZGravity = false;
    bool containsCenterPoint = false;
    bool thicknessOnly = false;
    bool singleSensorOnly = false;
    QString sourcePath;
    QString summary;

    int totalPointCount() const
    {
        int count = 0;
        for (const LineData& line : lines)
            count += line.points.size();
        return count;
    }
};

struct AlgorithmOptions
{
    int lineSampleNum = 51;
    double radius = 100.0;
    double radiusCut = 0.0;
    int thetaPoints = 36;
    int rPoints = 20;
    bool useNewBowAlg = true;
    bool useNewWarpAlg = true;
    double trimSize = 0.0;
    double chordLength = 0.0;
    int windowSize = 40000;
    UnitMode unitMode = UnitMode::Millimeter;
    bool useCsvZGravity = true;
    bool allowDebugZeroZGravity = false;
    int lineCount = 8;
    int productSize = 8;
    double nominalThickness = 0.0;
    double calibrationTotal = 0.0;
    double centerX = 0.0;
    double centerY = 0.0;
};

inline QString directionToString(DirectionClass direction)
{
    switch (direction) {
    case DirectionClass::X:
        return QStringLiteral("X");
    case DirectionClass::Y:
        return QStringLiteral("Y");
    default:
        return QStringLiteral("Other");
    }
}

} // namespace Wafer

Q_DECLARE_METATYPE(Wafer::DirectionClass)

#endif // WAFERTYPES_H
