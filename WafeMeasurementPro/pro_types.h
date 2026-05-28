#ifndef WAFEMEASUREMENTPRO_PRO_TYPES_H
#define WAFEMEASUREMENTPRO_PRO_TYPES_H

#include "WaferAlgorithmResult.h"
#include "WaferTypes.h"

#include <QMap>
#include <QString>
#include <QVector>

struct ProRecipe
{
    int id = -1;
    QString name;
    int measurePath = 8;
    int productThickness = 775;
    int productSize = 8;
    double trimSize = 2.0;
    int lineSampleNum = 51;
};

struct ProMeasurePoint
{
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double topDistance = 0.0;
    double bottomDistance = 0.0;
    double thickness = 0.0;
    double zGravity = 0.0;
    int lineIndex = -1;
    int sampleIndex = -1;
    int encoder = 0;
    bool hasZGravity = false;
};

struct ProRunResult
{
    bool success = false;
    QString errorMessage;
    QVector<ProMeasurePoint> points;
    QMap<int, QVector<ProMeasurePoint>> linePoints;
    Wafer::AlgorithmResult algorithm;
    QString outputDir;
    QString rawCsvPath;
    QString summaryCsvPath;
    QString heatImagePath;
    QString surfaceImagePath;
};

struct ProPathLine
{
    int lineIndex = -1;
    double startX = 0.0;
    double startY = 0.0;
    double endX = 0.0;
    double endY = 0.0;
};

#endif // WAFEMEASUREMENTPRO_PRO_TYPES_H
