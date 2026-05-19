#include "WaferAlgorithm.h"

#include <QElapsedTimer>
#include <QtMath>

#include <algorithm>
#include <limits>
#include <numeric>

namespace Wafer
{

namespace
{
constexpr double kEpsilon = 1e-9;

bool validNumber(double value)
{
    return qIsFinite(value);
}

double signedMaxAbs(double currentValue, bool* hasValue, double nextValue)
{
    if (!*hasValue || qAbs(nextValue) > qAbs(currentValue)) {
        *hasValue = true;
        return nextValue;
    }
    return currentValue;
}

} // namespace

AlgorithmResult WaferAlgorithm::runManualWarp(const WaferDataset& dataset, const AlgorithmOptions& options) const
{
    QElapsedTimer timer;
    timer.start();
    AlgorithmResult result = validateDataset(dataset, options);
    result.logs << QStringLiteral("Average THK uses valid thickness values. TTV is max(thickness)-min(thickness).");
    if (!result.errorMessage.isEmpty() && dataset.thicknessOnly) {
        result.logs << QStringLiteral("Shape metrics skipped because thickness-only csv has no ZM or confocal pair data.");
        result.logs << QStringLiteral("Elapsed ms: %1").arg(timer.elapsed());
        return result;
    }

    QVector<PreparedPoint> points;
    QString error;
    if (!preparePoints(dataset, options, &points, &result.logs, &error)) {
        setFailure(&result, error);
        result.logs << QStringLiteral("Elapsed ms: %1").arg(timer.elapsed());
        return result;
    }

    if (!manualCore(points, options, &result, &error)) {
        setFailure(&result, error);
        result.logs << QStringLiteral("Elapsed ms: %1").arg(timer.elapsed());
        return result;
    }

    result.lineDetails = buildLineThicknessDetails(dataset);
    result.success = true;
    result.errorMessage.clear();
    result.logs << QStringLiteral("ManualWarp completed.");
    result.logs << QStringLiteral("Elapsed ms: %1").arg(timer.elapsed());
    return result;
}

AlgorithmResult WaferAlgorithm::runNewBowWarp(const WaferDataset& dataset, const AlgorithmOptions& options) const
{
    QElapsedTimer timer;
    timer.start();
    AlgorithmResult result = validateDataset(dataset, options);
    if (!result.errorMessage.isEmpty() && dataset.thicknessOnly) {
        result.logs << QStringLiteral("New Bow/Warp skipped because thickness-only csv has no ZM or confocal pair data.");
        result.logs << QStringLiteral("Elapsed ms: %1").arg(timer.elapsed());
        return result;
    }

    QString error;
    QVector<PreparedPoint> allPoints;
    if (!preparePoints(dataset, options, &allPoints, &result.logs, &error)) {
        setFailure(&result, error);
        result.logs << QStringLiteral("Elapsed ms: %1").arg(timer.elapsed());
        return result;
    }

    bool hasBow = false;
    bool hasBowX = false;
    bool hasBowY = false;
    bool hasWarpX = false;
    bool hasWarpY = false;
    QVector<double> outAllX;
    QVector<double> outAllY;
    QVector<double> outAllZm;
    QVector<double> outAllThk;
    QVector<LineAlgorithmDetail> details = buildLineThicknessDetails(dataset);

    for (int lineIndex = 0; lineIndex < dataset.lines.size(); ++lineIndex) {
        const LineData& line = dataset.lines.at(lineIndex);
        QVector<double> x;
        QVector<double> y;
        QVector<double> zm;
        QVector<double> thk;
        x.reserve(line.points.size());
        y.reserve(line.points.size());
        zm.reserve(line.points.size());
        thk.reserve(line.points.size());

        for (const DataPoint& point : line.points) {
            double z = 0.0;
            if (!resolveZm(point, options, &z, &error)) {
                setFailure(&result, QStringLiteral("line %1: %2").arg(line.lineIndex).arg(error));
                result.logs << QStringLiteral("Elapsed ms: %1").arg(timer.elapsed());
                return result;
            }
            x.append(point.x);
            y.append(point.y);
            zm.append(z);
            thk.append(point.hasThickness ? point.thickness : 0.0);
        }

        double lineBow = 0.0;
        if (!calcuBowLine(x, y, zm, options.radius * 2.0, options.lineSampleNum, &lineBow, &result.logs, &error)) {
            setFailure(&result, QStringLiteral("line %1 Bow failed: %2").arg(line.lineIndex).arg(error));
            result.logs << QStringLiteral("Elapsed ms: %1").arg(timer.elapsed());
            return result;
        }

        double lineWarp = 0.0;
        QVector<double> outX;
        QVector<double> outY;
        QVector<double> outZmFit;
        QVector<double> outThk;
        if (!calculWarpSingleLine(x, y, zm, thk, options.lineSampleNum, &lineWarp,
                                  &outX, &outY, &outZmFit, &outThk, &result.logs, &error)) {
            setFailure(&result, QStringLiteral("line %1 Warp failed: %2").arg(line.lineIndex).arg(error));
            result.logs << QStringLiteral("Elapsed ms: %1").arg(timer.elapsed());
            return result;
        }

        DirectionClass direction = classifyDirection(line);
        for (LineAlgorithmDetail& detail : details) {
            if (detail.lineIndex == line.lineIndex) {
                detail.bow = lineBow;
                detail.hasBow = true;
                detail.warp = lineWarp;
                detail.hasWarp = true;
                detail.direction = direction;
                break;
            }
        }

        result.bow = signedMaxAbs(result.bow, &hasBow, lineBow);
        if (direction == DirectionClass::X) {
            result.bowX = signedMaxAbs(result.bowX, &hasBowX, lineBow);
            if (!hasWarpX || lineWarp > result.warpX) {
                result.warpX = lineWarp;
                hasWarpX = true;
            }
        } else if (direction == DirectionClass::Y) {
            result.bowY = signedMaxAbs(result.bowY, &hasBowY, lineBow);
            if (!hasWarpY || lineWarp > result.warpY) {
                result.warpY = lineWarp;
                hasWarpY = true;
            }
        }

        outAllX += outX;
        outAllY += outY;
        outAllZm += outZmFit;
        outAllThk += outThk;
    }

    double warp = 0.0;
    double sori = 0.0;
    if (!calculWarpAllLine(outAllX, outAllY, outAllZm, outAllThk,
                           dataset.lines.size(), options.lineSampleNum + 1, &warp, &sori, &error)) {
        setFailure(&result, error);
        result.logs << QStringLiteral("Elapsed ms: %1").arg(timer.elapsed());
        return result;
    }

    result.hasBow = hasBow;
    result.hasBowX = hasBowX;
    result.hasBowY = hasBowY;
    result.warp = warp;
    result.hasWarp = true;
    result.hasWarpX = hasWarpX;
    result.hasWarpY = hasWarpY;
    result.sori = sori;
    result.hasSori = true;
    result.lineDetails = details;
    result.success = true;
    result.errorMessage.clear();
    result.logs << QStringLiteral("Bow(X/Y) uses signed max absolute line Bow by direction.");
    result.logs << QStringLiteral("Warp(X/Y) uses max line Warp by direction.");
    result.logs << QStringLiteral("Elapsed ms: %1").arg(timer.elapsed());
    return result;
}

AlgorithmResult WaferAlgorithm::runAll(const WaferDataset& dataset, const AlgorithmOptions& options) const
{
    AlgorithmResult result = options.useNewBowAlg || options.useNewWarpAlg
                                 ? runNewBowWarp(dataset, options)
                                 : runManualWarp(dataset, options);

    if (!options.useNewBowAlg && result.success) {
        QVector<PreparedPoint> points;
        QString error;
        if (preparePoints(dataset, options, &points, &result.logs, &error)) {
            double bow = 0.0;
            if (calculateBowOnly(points, options, &bow, &error)) {
                result.bow = bow;
                result.hasBow = true;
            }
        }
    }
    return result;
}

AlgorithmResult WaferAlgorithm::validateDataset(const WaferDataset& dataset, const AlgorithmOptions& options) const
{
    AlgorithmResult result;
    result.lineDetails = buildLineThicknessDetails(dataset);
    computeThicknessStats(dataset, options, &result);

    if (dataset.lines.isEmpty()) {
        setFailure(&result, QStringLiteral("Dataset has no lines."));
        return result;
    }
    if (options.lineCount > 0 && dataset.lineCount > 0 && dataset.lineCount != options.lineCount) {
        result.logs << QStringLiteral("Loaded line count %1 differs from option line count %2.")
                           .arg(dataset.lineCount).arg(options.lineCount);
    }
    for (const LineData& line : dataset.lines) {
        if (line.points.isEmpty()) {
            setFailure(&result, QStringLiteral("Line %1 has no points.").arg(line.lineIndex));
            return result;
        }
    }
    if (dataset.thicknessOnly) {
        setFailure(&result, QStringLiteral("Thickness-only data can compute Average THK, Center THK and TTV only."));
        return result;
    }

    result.success = true;
    return result;
}

bool WaferAlgorithm::preparePoints(const WaferDataset& dataset, const AlgorithmOptions& options,
                                   QVector<PreparedPoint>* points, QStringList* logs, QString* error) const
{
    points->clear();
    if (dataset.lines.isEmpty()) {
        *error = QStringLiteral("Dataset has no lines.");
        return false;
    }

    for (const LineData& line : dataset.lines) {
        for (const DataPoint& point : line.points) {
            if (!point.hasThickness) {
                *error = QStringLiteral("Point is missing thickness.");
                return false;
            }
            double zm = 0.0;
            if (!resolveZm(point, options, &zm, error))
                return false;
            PreparedPoint prepared;
            prepared.point = point;
            prepared.xUm = toUm(point.x, options);
            prepared.yUm = toUm(point.y, options);
            prepared.zm = zm;
            prepared.thickness = point.thickness;
            points->append(prepared);
        }
    }

    if (points->size() < 3) {
        *error = QStringLiteral("At least 3 points are required.");
        return false;
    }
    if (!dataset.containsZGravity && options.allowDebugZeroZGravity && logs)
        logs->append(QStringLiteral("zGravity=0 debug mode is enabled. This is not a formal compensation result."));
    return true;
}

bool WaferAlgorithm::resolveZm(const DataPoint& point, const AlgorithmOptions& options, double* zm, QString* error) const
{
    if (point.hasZm) {
        *zm = point.zm;
        return true;
    }
    if (!point.hasTopDistance || !point.hasBottomDistance) {
        *error = QStringLiteral("Missing ZM and missing top/bottom confocal data.");
        return false;
    }
    double zg = 0.0;
    if (point.hasZGravity && options.useCsvZGravity) {
        zg = point.zGravity;
    } else if (options.allowDebugZeroZGravity) {
        zg = 0.0;
    } else {
        *error = QStringLiteral("Missing zGravity. Enable debug zero zGravity only for non-formal checks.");
        return false;
    }
    *zm = (point.bottomDistance - point.topDistance) / 2.0 - zg;
    return validNumber(*zm);
}

bool WaferAlgorithm::computeThicknessStats(const WaferDataset& dataset, const AlgorithmOptions&, AlgorithmResult* result) const
{
    QVector<double> thickness;
    for (const LineData& line : dataset.lines) {
        for (const DataPoint& point : line.points) {
            if (point.hasThickness && validNumber(point.thickness))
                thickness.append(point.thickness);
        }
    }
    if (thickness.isEmpty()) {
        result->logs << QStringLiteral("No valid thickness values found.");
        return false;
    }
    const auto mm = std::minmax_element(thickness.constBegin(), thickness.constEnd());
    const double sum = std::accumulate(thickness.constBegin(), thickness.constEnd(), 0.0);
    result->averageThk = sum / double(thickness.size());
    result->hasAverageThk = true;
    result->ttv = *mm.second - *mm.first;
    result->hasTtv = true;
    QString error;
    double centerThk = 0.0;
    if (findCenterThickness(dataset, &centerThk, &error)) {
        result->centerThk = centerThk;
        result->hasCenterThk = true;
    } else {
        result->logs << error;
    }
    return true;
}

bool WaferAlgorithm::findCenterThickness(const WaferDataset& dataset, double* value, QString* error) const
{
    if (dataset.centerPoint.valid && dataset.centerPoint.point.hasThickness) {
        *value = dataset.centerPoint.point.thickness;
        return true;
    }

    bool found = false;
    double bestDist = std::numeric_limits<double>::max();
    double bestValue = 0.0;
    for (const LineData& line : dataset.lines) {
        for (const DataPoint& point : line.points) {
            if (!point.hasThickness)
                continue;
            const double dx = point.x - dataset.centerX;
            const double dy = point.y - dataset.centerY;
            const double dist = dx * dx + dy * dy;
            if (!found || dist < bestDist) {
                found = true;
                bestDist = dist;
                bestValue = point.thickness;
            }
        }
    }
    if (!found) {
        *error = QStringLiteral("Center THK cannot be resolved because no thickness point exists.");
        return false;
    }
    *value = bestValue;
    return true;
}

QVector<LineAlgorithmDetail> WaferAlgorithm::buildLineThicknessDetails(const WaferDataset& dataset) const
{
    QVector<LineAlgorithmDetail> details;
    for (const LineData& line : dataset.lines) {
        LineAlgorithmDetail detail;
        detail.lineIndex = line.lineIndex;
        detail.pointCount = line.points.size();
        detail.direction = classifyDirection(line);
        QVector<double> values;
        for (const DataPoint& point : line.points) {
            if (point.hasThickness && validNumber(point.thickness))
                values.append(point.thickness);
        }
        if (!values.isEmpty()) {
            const auto mm = std::minmax_element(values.constBegin(), values.constEnd());
            const double sum = std::accumulate(values.constBegin(), values.constEnd(), 0.0);
            detail.minThickness = *mm.first;
            detail.maxThickness = *mm.second;
            detail.averageThickness = sum / double(values.size());
            detail.hasThicknessStats = true;
        }
        details.append(detail);
    }
    return details;
}

DirectionClass WaferAlgorithm::classifyDirection(const LineData& line) const
{
    if (line.points.size() < 2)
        return DirectionClass::Other;
    const DataPoint& first = line.points.first();
    const DataPoint& last = line.points.last();
    const double dx = qAbs(last.x - first.x);
    const double dy = qAbs(last.y - first.y);
    if (dx < kEpsilon && dy < kEpsilon)
        return DirectionClass::Other;
    return dx >= dy ? DirectionClass::X : DirectionClass::Y;
}

bool WaferAlgorithm::manualCore(const QVector<PreparedPoint>& points, const AlgorithmOptions& options,
                                AlgorithmResult* result, QString* error) const
{
    QVector<double> x;
    QVector<double> y;
    QVector<double> zm;
    QVector<double> thk;
    x.reserve(points.size());
    y.reserve(points.size());
    zm.reserve(points.size());
    thk.reserve(points.size());
    for (const PreparedPoint& point : points) {
        x.append(point.xUm);
        y.append(point.yUm);
        zm.append(point.zm);
        thk.append(point.thickness);
    }

    Eigen::Vector3d plane;
    if (!planeFit(x, y, zm, &plane, error))
        return false;
    QVector<double> diff;
    diff.reserve(zm.size());
    for (int i = 0; i < zm.size(); ++i)
        diff.append(zm.at(i) - (plane[0] * x.at(i) + plane[1] * y.at(i) + plane[2]));
    const auto mm = std::minmax_element(diff.constBegin(), diff.constEnd());
    result->warp = *mm.second - *mm.first;
    result->hasWarp = true;

    const double centerX = toUm(options.centerX, options);
    const double centerY = toUm(options.centerY, options);
    result->bow = zm.last() - (plane[0] * centerX + plane[1] * centerY + plane[2]);
    result->hasBow = true;

    QVector<double> zFcom;
    zFcom.reserve(points.size());
    for (int i = 0; i < points.size(); ++i)
        zFcom.append(zm.at(i) + thk.at(i) / 2.0);
    Eigen::Vector3d fcomPlane;
    if (!planeFit(x, y, zFcom, &fcomPlane, error))
        return false;
    QVector<double> soriDiff;
    soriDiff.reserve(zFcom.size());
    for (int i = 0; i < zFcom.size(); ++i)
        soriDiff.append(zFcom.at(i) - (fcomPlane[0] * x.at(i) + fcomPlane[1] * y.at(i) + fcomPlane[2]));
    const auto soriMm = std::minmax_element(soriDiff.constBegin(), soriDiff.constEnd());
    result->sori = *soriMm.second - *soriMm.first;
    result->hasSori = true;

    return true;
}

bool WaferAlgorithm::calculateBowOnly(const QVector<PreparedPoint>& points, const AlgorithmOptions& options,
                                      double* bow, QString* error) const
{
    QVector<PreparedPoint> withoutCenter = points;
    if (withoutCenter.size() < 4) {
        *error = QStringLiteral("Calculate_Bow requires at least 4 points.");
        return false;
    }
    withoutCenter.removeLast();
    AlgorithmResult temp;
    if (!manualCore(withoutCenter, options, &temp, error))
        return false;
    const PreparedPoint& centerPoint = points.last();
    QVector<double> x;
    QVector<double> y;
    QVector<double> zm;
    for (const PreparedPoint& p : withoutCenter) {
        x.append(p.xUm);
        y.append(p.yUm);
        zm.append(p.zm);
    }
    Eigen::Vector3d plane;
    if (!planeFit(x, y, zm, &plane, error))
        return false;
    *bow = centerPoint.zm - (plane[0] * toUm(options.centerX, options)
                            + plane[1] * toUm(options.centerY, options) + plane[2]);
    return true;
}

bool WaferAlgorithm::calcuBowLine(QVector<double> x, QVector<double> y, QVector<double> zm, double diameter,
                                  int sampleNum, double* bow, QStringList* logs, QString* error) const
{
    const int pointNum = x.size();
    if (pointNum < 7 || y.size() != pointNum || zm.size() != pointNum) {
        *error = QStringLiteral("CalcuBowLine requires at least 7 aligned points.");
        return false;
    }
    if (sampleNum > pointNum) {
        if (logs)
            logs->append(QStringLiteral("lineSampleNum %1 exceeds point count %2, downgraded.").arg(sampleNum).arg(pointNum));
        sampleNum = pointNum;
    }
    if (sampleNum < 3) {
        *error = QStringLiteral("CalcuBowLine sample count must be >= 3.");
        return false;
    }

    for (int i = 0; i < pointNum; ++i) {
        x[i] = toUm(x.at(i), AlgorithmOptions());
        y[i] = toUm(y.at(i), AlgorithmOptions());
    }

    Eigen::Vector3d plane;
    if (!planeFit(x, y, zm, &plane, error))
        return false;
    for (int i = 0; i < pointNum; ++i)
        zm[i] = zm.at(i) - (plane[0] * x.at(i) + plane[1] * y.at(i) + plane[2]);

    rotatePointsToBase(&x, &y, &zm);
    const double centerX = x.last();
    if (!fitLineZm(&x, &y, &zm, 6, error))
        return false;

    QVector<double> sx;
    QVector<double> sy;
    QVector<double> sz;
    QVector<int> indices;
    if (!extractPointsLine(x, y, zm, sampleNum, &sx, &sy, &sz, &indices, error))
        return false;

    Eigen::VectorXd vx(sampleNum);
    Eigen::VectorXd vz(sampleNum);
    for (int i = 0; i < sampleNum; ++i) {
        vx[i] = sx.at(i);
        vz[i] = sz.at(i);
    }
    Eigen::VectorXd coeff;
    if (!polyFit(vx, vz, 2, &coeff, error))
        return false;
    const double end1 = centerX - diameter * 1000.0 / 2.0;
    const double end2 = centerX + diameter * 1000.0 / 2.0;
    const double z0 = coeff[0] + coeff[1] * centerX + coeff[2] * centerX * centerX;
    const double z1 = coeff[0] + coeff[1] * end1 + coeff[2] * end1 * end1;
    const double z2 = coeff[0] + coeff[1] * end2 + coeff[2] * end2 * end2;
    Eigen::Vector2d lineCoeff;
    if (!linearFit(end1, z1, end2, z2, &lineCoeff, error))
        return false;
    const double zFit0 = lineCoeff[0] + lineCoeff[1] * centerX;
    *bow = -(z0 - zFit0);
    return validNumber(*bow);
}

bool WaferAlgorithm::calculWarpSingleLine(QVector<double> x, QVector<double> y, QVector<double> zm,
                                          QVector<double> thk, int sampleNum, double* warp,
                                          QVector<double>* outX, QVector<double>* outY,
                                          QVector<double>* outZmFit, QVector<double>* outThk,
                                          QStringList* logs, QString* error) const
{
    const int pointNum = x.size();
    if (pointNum < 7 || y.size() != pointNum || zm.size() != pointNum || thk.size() != pointNum) {
        *error = QStringLiteral("CalculWarpSingleLine requires aligned x/y/zm/thickness arrays.");
        return false;
    }
    if (sampleNum > pointNum) {
        if (logs)
            logs->append(QStringLiteral("lineSampleNum %1 exceeds point count %2, downgraded.").arg(sampleNum).arg(pointNum));
        sampleNum = pointNum;
    }
    if (sampleNum < 3) {
        *error = QStringLiteral("CalculWarpSingleLine sample count must be >= 3.");
        return false;
    }

    QVector<double> xUm = x;
    QVector<double> yUm = y;
    for (int i = 0; i < pointNum; ++i) {
        xUm[i] *= 1000.0;
        yUm[i] *= 1000.0;
    }

    Eigen::Vector3d plane;
    if (!planeFit(xUm, yUm, zm, &plane, error))
        return false;
    for (int i = 0; i < pointNum; ++i)
        zm[i] = zm.at(i) - (plane[0] * xUm.at(i) + plane[1] * yUm.at(i) + plane[2]);

    rotatePointsToBase(&xUm, &yUm, &zm);
    if (!fitLineZm(&xUm, &yUm, &zm, 6, error))
        return false;

    QVector<double> sx;
    QVector<double> sy;
    QVector<double> sz;
    QVector<int> indices;
    if (!extractPointsLine(xUm, yUm, zm, sampleNum, &sx, &sy, &sz, &indices, error))
        return false;

    Eigen::VectorXd vx(sampleNum);
    Eigen::VectorXd vz(sampleNum);
    double meanX = 0.0;
    for (int i = 0; i < sampleNum; ++i) {
        vx[i] = sx.at(i);
        vz[i] = sz.at(i);
        meanX += sx.at(i);
    }
    meanX /= double(sampleNum);
    Eigen::VectorXd coeff;
    if (!polyFit(vx, vz, 2, &coeff, error))
        return false;

    QVector<double> fitted(sampleNum);
    for (int i = 0; i < sampleNum; ++i)
        fitted[i] = coeff[0] + coeff[1] * vx[i] + coeff[2] * vx[i] * vx[i];
    const double zFitMean = coeff[0] + coeff[1] * meanX + coeff[2] * meanX * meanX;

    outX->clear();
    outY->clear();
    outZmFit->clear();
    outThk->clear();
    for (int i = 0; i < sampleNum; ++i) {
        const int src = qBound(0, indices.at(i), pointNum - 1);
        outX->append(x.at(src));
        outY->append(y.at(src));
        outZmFit->append(fitted.at(i));
        outThk->append(thk.at(src));
    }
    outX->append(x.last());
    outY->append(y.last());
    outZmFit->append(zFitMean);
    outThk->append(thk.last());

    Eigen::Vector2d lineCoeff;
    if (!lineFit(sx, fitted, &lineCoeff, error))
        return false;
    QVector<double> diff;
    for (int i = 0; i < sampleNum; ++i)
        diff.append(fitted.at(i) - (lineCoeff[0] * sx.at(i) + lineCoeff[1]));
    const auto mm = std::minmax_element(diff.constBegin(), diff.constEnd());
    *warp = *mm.second - *mm.first;
    return validNumber(*warp);
}

bool WaferAlgorithm::calculWarpAllLine(QVector<double> x, QVector<double> y, QVector<double> zm,
                                       QVector<double> thk, int numLine, int pointsPerLine,
                                       double* warp, double* sori, QString* error) const
{
    const int n = x.size();
    if (n < 3 || y.size() != n || zm.size() != n || thk.size() != n) {
        *error = QStringLiteral("CalculWarpAllLine requires aligned arrays.");
        return false;
    }
    if (numLine <= 0 || pointsPerLine <= 0 || n != numLine * pointsPerLine) {
        *error = QStringLiteral("CalculWarpAllLine invalid line/point count.");
        return false;
    }
    if (!alignZValues(&zm, numLine, pointsPerLine, error))
        return false;

    QVector<double> xUm = x;
    QVector<double> yUm = y;
    for (int i = 0; i < n; ++i) {
        xUm[i] *= 1000.0;
        yUm[i] *= 1000.0;
    }
    Eigen::Vector3d plane;
    if (!planeFit(xUm, yUm, zm, &plane, error))
        return false;
    QVector<double> diff;
    for (int i = 0; i < n; ++i)
        diff.append(zm.at(i) - (plane[0] * xUm.at(i) + plane[1] * yUm.at(i) + plane[2]));
    const auto mm = std::minmax_element(diff.constBegin(), diff.constEnd());
    *warp = *mm.second - *mm.first;

    QVector<double> zFcom;
    for (int i = 0; i < n; ++i)
        zFcom.append(zm.at(i) + thk.at(i) / 2.0);
    Eigen::Vector3d soriPlane;
    if (!planeFit(xUm, yUm, zFcom, &soriPlane, error))
        return false;
    QVector<double> soriDiff;
    for (int i = 0; i < n; ++i)
        soriDiff.append(zFcom.at(i) - (soriPlane[0] * xUm.at(i) + soriPlane[1] * yUm.at(i) + soriPlane[2]));
    const auto smm = std::minmax_element(soriDiff.constBegin(), soriDiff.constEnd());
    *sori = *smm.second - *smm.first;
    return validNumber(*warp) && validNumber(*sori);
}

void WaferAlgorithm::rotatePointsToBase(QVector<double>* x, QVector<double>* y, QVector<double>* zm) const
{
    if (!x || !y || !zm || x->isEmpty())
        return;
    const double baseX = x->first();
    const double baseY = y->first();
    const double baseZ = zm->first();
    const double theta = -qAtan2(x->last() - x->first() == 0.0 ? y->last() - y->first() : y->last() - y->first(),
                                 x->last() - x->first());
    const double c = qCos(theta);
    const double s = qSin(theta);
    for (int i = 0; i < x->size(); ++i) {
        const double tx = x->at(i) - baseX;
        const double ty = y->at(i) - baseY;
        const double tz = zm->at(i) - baseZ;
        (*x)[i] = tx * c - ty * s + baseX;
        (*y)[i] = tx * s + ty * c + baseY;
        (*zm)[i] = tz + baseZ;
    }
}

bool WaferAlgorithm::curveFitting(const Eigen::VectorXd& x, const Eigen::VectorXd& y, int degree,
                                  Eigen::VectorXd* coeff, QString* error) const
{
    if (x.size() != y.size() || x.size() < degree + 1 || degree < 0) {
        *error = QStringLiteral("curveFitting invalid matrix dimensions.");
        return false;
    }
    Eigen::MatrixXd a(x.size(), degree + 1);
    for (int i = 0; i < x.size(); ++i) {
        for (int j = 0; j <= degree; ++j)
            a(i, j) = qPow(x(i), j);
    }
    *coeff = a.colPivHouseholderQr().solve(y);
    return coeff->allFinite();
}

bool WaferAlgorithm::fitLineZm(QVector<double>* x, QVector<double>* y, QVector<double>* zm,
                               int degree, QString* error) const
{
    Q_UNUSED(y);
    if (!x || !zm || x->size() != zm->size() || x->size() < degree + 1) {
        *error = QStringLiteral("FitLineZM invalid input.");
        return false;
    }
    Eigen::VectorXd vx(x->size());
    Eigen::VectorXd vz(zm->size());
    for (int i = 0; i < x->size(); ++i) {
        vx[i] = x->at(i);
        vz[i] = zm->at(i);
    }
    Eigen::VectorXd coeff;
    if (!curveFitting(vx, vz, degree, &coeff, error))
        return false;
    for (int i = 0; i < x->size(); ++i) {
        double fit = coeff[degree];
        for (int j = degree - 1; j >= 0; --j)
            fit = fit * x->at(i) + coeff[j];
        (*zm)[i] = fit;
    }
    return true;
}

bool WaferAlgorithm::extractPointsLine(const QVector<double>& x, const QVector<double>& y, const QVector<double>& zm,
                                       int outCount, QVector<double>* outX, QVector<double>* outY,
                                       QVector<double>* outZm, QVector<int>* indices, QString* error) const
{
    if (x.size() != y.size() || x.size() != zm.size() || x.isEmpty() || outCount < 2 || outCount > x.size()) {
        *error = QStringLiteral("ExtractdPointsLine invalid input.");
        return false;
    }
    struct Point
    {
        double x;
        double y;
        double z;
        int originalIndex;
    };
    QVector<Point> points;
    for (int i = 0; i < x.size(); ++i)
        points.append(Point{x.at(i), y.at(i), zm.at(i), i});
    std::sort(points.begin(), points.end(), [](const Point& a, const Point& b) { return a.x < b.x; });

    outX->clear();
    outY->clear();
    outZm->clear();
    indices->clear();
    const int middleCount = outCount - 1;
    const double startX = points.first().x;
    const double step = (points.last().x - points.first().x) / double(middleCount);
    for (int i = 0; i < middleCount; ++i) {
        const double target = startX + i * step;
        auto it = std::lower_bound(points.begin(), points.end(), target,
                                   [](const Point& p, double value) { return p.x < value; });
        int idx = int(std::distance(points.begin(), it));
        if (idx <= 0) {
            idx = 0;
        } else if (idx >= points.size()) {
            idx = points.size() - 1;
        } else {
            const double prevDist = qAbs(points.at(idx - 1).x - target);
            const double nextDist = qAbs(points.at(idx).x - target);
            if (prevDist < nextDist)
                --idx;
        }
        outX->append(points.at(idx).x);
        outY->append(points.at(idx).y);
        outZm->append(points.at(idx).z);
        indices->append(points.at(idx).originalIndex);
    }
    outX->append(points.last().x);
    outY->append(points.last().y);
    outZm->append(points.last().z);
    indices->append(points.last().originalIndex);
    return true;
}

bool WaferAlgorithm::polyFit(const Eigen::VectorXd& x, const Eigen::VectorXd& y, int degree,
                             Eigen::VectorXd* coeff, QString* error) const
{
    if (degree != 2) {
        *error = QStringLiteral("polyFit currently expects degree 2.");
        return false;
    }
    if (x.size() != y.size() || x.size() < degree + 1) {
        *error = QStringLiteral("polyFit invalid input.");
        return false;
    }
    Eigen::MatrixXd a(x.size(), degree + 1);
    for (int i = 0; i < x.size(); ++i) {
        a(i, 0) = 1.0;
        a(i, 1) = x(i);
        a(i, 2) = x(i) * x(i);
    }
    *coeff = a.colPivHouseholderQr().solve(y);
    return coeff->allFinite();
}

bool WaferAlgorithm::linearFit(double x1, double y1, double x2, double y2,
                               Eigen::Vector2d* coeff, QString* error) const
{
    if (qAbs(x1 - x2) < kEpsilon) {
        *error = QStringLiteral("linearFit cannot fit vertical line with same x.");
        return false;
    }
    Eigen::Matrix2d a;
    a << 1.0, x1, 1.0, x2;
    Eigen::Vector2d b(y1, y2);
    *coeff = a.colPivHouseholderQr().solve(b);
    return coeff->allFinite();
}

bool WaferAlgorithm::alignZValues(QVector<double>* z, int numLines, int pointsPerLine, QString* error) const
{
    if (!z || numLines <= 0 || pointsPerLine <= 0 || z->size() != numLines * pointsPerLine) {
        *error = QStringLiteral("alignZValues invalid input.");
        return false;
    }
    QVector<double> centerZ;
    for (int line = 0; line < numLines; ++line)
        centerZ.append(z->at(line * pointsPerLine + pointsPerLine - 1));
    const double maxZ = *std::max_element(centerZ.constBegin(), centerZ.constEnd());
    for (int line = 0; line < numLines; ++line) {
        const double delta = maxZ - centerZ.at(line);
        for (int pt = 0; pt < pointsPerLine; ++pt)
            (*z)[line * pointsPerLine + pt] += delta;
    }
    return true;
}

bool WaferAlgorithm::planeFit(const QVector<double>& x, const QVector<double>& y, const QVector<double>& z,
                              Eigen::Vector3d* coeff, QString* error) const
{
    const int n = x.size();
    if (n < 3 || y.size() != n || z.size() != n) {
        *error = QStringLiteral("Plane fit requires at least 3 aligned points.");
        return false;
    }
    Eigen::MatrixXd a(n, 3);
    Eigen::VectorXd b(n);
    for (int i = 0; i < n; ++i) {
        if (!validNumber(x.at(i)) || !validNumber(y.at(i)) || !validNumber(z.at(i))) {
            *error = QStringLiteral("Plane fit contains invalid number.");
            return false;
        }
        a(i, 0) = x.at(i);
        a(i, 1) = y.at(i);
        a(i, 2) = 1.0;
        b(i) = z.at(i);
    }
    *coeff = a.bdcSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(b);
    if (!coeff->allFinite()) {
        *error = QStringLiteral("Plane fit failed.");
        return false;
    }
    return true;
}

bool WaferAlgorithm::lineFit(const QVector<double>& x, const QVector<double>& z,
                             Eigen::Vector2d* coeff, QString* error) const
{
    const int n = x.size();
    if (n < 2 || z.size() != n) {
        *error = QStringLiteral("Line fit requires at least 2 aligned points.");
        return false;
    }
    Eigen::MatrixXd a(n, 2);
    Eigen::VectorXd b(n);
    for (int i = 0; i < n; ++i) {
        a(i, 0) = x.at(i);
        a(i, 1) = 1.0;
        b(i) = z.at(i);
    }
    Eigen::VectorXd solved = a.bdcSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(b);
    if (!solved.allFinite()) {
        *error = QStringLiteral("Line fit failed.");
        return false;
    }
    (*coeff)[0] = solved[0];
    (*coeff)[1] = solved[1];
    return true;
}

double WaferAlgorithm::toUm(double value, const AlgorithmOptions& options) const
{
    return options.unitMode == UnitMode::Millimeter ? value * 1000.0 : value;
}

void WaferAlgorithm::setFailure(AlgorithmResult* result, const QString& message) const
{
    result->success = false;
    result->errorMessage = message;
    result->logs << QStringLiteral("ERROR: %1").arg(message);
}

} // namespace Wafer
