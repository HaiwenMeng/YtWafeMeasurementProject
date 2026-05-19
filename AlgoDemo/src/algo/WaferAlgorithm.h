#ifndef WAFERALGORITHM_H
#define WAFERALGORITHM_H

#include "WaferAlgorithmResult.h"
#include "WaferTypes.h"

#include <Eigen/Dense>

#include <QStringList>

namespace Wafer
{

class WaferAlgorithm
{
public:
    AlgorithmResult runManualWarp(const WaferDataset& dataset, const AlgorithmOptions& options) const;
    AlgorithmResult runNewBowWarp(const WaferDataset& dataset, const AlgorithmOptions& options) const;
    AlgorithmResult runAll(const WaferDataset& dataset, const AlgorithmOptions& options) const;
    AlgorithmResult validateDataset(const WaferDataset& dataset, const AlgorithmOptions& options) const;
    QVector<LineAlgorithmDetail> buildLineThicknessDetails(const WaferDataset& dataset) const;

private:
    struct PreparedPoint
    {
        DataPoint point;
        double xUm = 0.0;
        double yUm = 0.0;
        double zm = 0.0;
        double thickness = 0.0;
    };

    bool preparePoints(const WaferDataset& dataset, const AlgorithmOptions& options,
                       QVector<PreparedPoint>* points, QStringList* logs, QString* error) const;
    bool resolveZm(const DataPoint& point, const AlgorithmOptions& options, double* zm, QString* error) const;
    bool computeThicknessStats(const WaferDataset& dataset, const AlgorithmOptions& options,
                               AlgorithmResult* result) const;
    bool findCenterThickness(const WaferDataset& dataset, double* value, QString* error) const;
    DirectionClass classifyDirection(const LineData& line) const;

    bool manualCore(const QVector<PreparedPoint>& points, const AlgorithmOptions& options,
                    AlgorithmResult* result, QString* error) const;
    bool calculateBowOnly(const QVector<PreparedPoint>& points, const AlgorithmOptions& options,
                          double* bow, QString* error) const;
    bool calcuBowLine(QVector<double> x, QVector<double> y, QVector<double> zm, double diameter,
                      int sampleNum, double* bow, QStringList* logs, QString* error) const;
    bool calculWarpSingleLine(QVector<double> x, QVector<double> y, QVector<double> zm,
                              QVector<double> thk, int sampleNum, double* warp,
                              QVector<double>* outX, QVector<double>* outY,
                              QVector<double>* outZmFit, QVector<double>* outThk,
                              QStringList* logs, QString* error) const;
    bool calculWarpAllLine(QVector<double> x, QVector<double> y, QVector<double> zm,
                           QVector<double> thk, int numLine, int sampleNum,
                           double* warp, double* sori, QString* error) const;
    void rotatePointsToBase(QVector<double>* x, QVector<double>* y, QVector<double>* zm) const;
    bool curveFitting(const Eigen::VectorXd& x, const Eigen::VectorXd& y, int degree,
                      Eigen::VectorXd* coeff, QString* error) const;
    bool fitLineZm(QVector<double>* x, QVector<double>* y, QVector<double>* zm,
                   int degree, QString* error) const;
    bool extractPointsLine(const QVector<double>& x, const QVector<double>& y, const QVector<double>& zm,
                           int outCount, QVector<double>* outX, QVector<double>* outY,
                           QVector<double>* outZm, QVector<int>* indices, QString* error) const;
    bool polyFit(const Eigen::VectorXd& x, const Eigen::VectorXd& y, int degree,
                 Eigen::VectorXd* coeff, QString* error) const;
    bool linearFit(double x1, double y1, double x2, double y2,
                   Eigen::Vector2d* coeff, QString* error) const;
    bool alignZValues(QVector<double>* z, int numLines, int pointsPerLine, QString* error) const;
    bool planeFit(const QVector<double>& x, const QVector<double>& y, const QVector<double>& z,
                  Eigen::Vector3d* coeff, QString* error) const;
    bool lineFit(const QVector<double>& x, const QVector<double>& z,
                 Eigen::Vector2d* coeff, QString* error) const;
    double toUm(double value, const AlgorithmOptions& options) const;
    void setFailure(AlgorithmResult* result, const QString& message) const;
};

} // namespace Wafer

#endif // WAFERALGORITHM_H
