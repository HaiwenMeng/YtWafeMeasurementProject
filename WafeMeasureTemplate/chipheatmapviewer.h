#ifndef CHIPHEATMAPVIEWER_H
#define CHIPHEATMAPVIEWER_H

#include <QWidget>
#include <QTimer>
#include <QHash>
#include "qcustomplot.h"
#include <QDebug>

class ChipHeatMapViewer : public QWidget
{
    Q_OBJECT

public:
    ChipHeatMapViewer(QCustomPlot* customPlot, QWidget* parent = nullptr);
    ~ChipHeatMapViewer();
   
     void PaintZMGeatPointClouds(std::vector<double> SurfacePointCloud_Xs, std::vector<double> SurfacePointCloud_Ys, std::vector<double> SurfacePointCloud_ZMs, int theta_Points,int r_Points);
     QCPColorMap* m_colorMap;
     QCPColorScale* m_colorScale;
private:
    void initViewer(QCustomPlot* customPlot);
   

    //��ͼ
    QCustomPlot* m_customPlot;


    int m_contourCnt;//�����ߵĸ���

};

#endif // CHIPHEATMAPVIEWER_H
