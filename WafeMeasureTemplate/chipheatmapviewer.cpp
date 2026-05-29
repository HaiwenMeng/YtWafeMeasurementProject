#include "chipheatmapviewer.h"


#include <QTime>

ChipHeatMapViewer::ChipHeatMapViewer(QCustomPlot* customPlot, QWidget* parent)
    : QWidget(parent)
{
    //初始化界面
    initViewer(customPlot);
  
   
}

ChipHeatMapViewer::~ChipHeatMapViewer()
{
}




void ChipHeatMapViewer::PaintZMGeatPointClouds(std::vector<double> SurfacePointCloud_Xs, std::vector<double> SurfacePointCloud_Ys, std::vector<double> SurfacePointCloud_ZMs, int theta_Points, int r_Points)
{
    
    int nx = 400, ny = 400;
    double xMin = 0, xMax = 0;
    double yMin = 0, yMax = 0;
    double zMin = 0, zMax = 0;
    
    for (int xIndex = 0; xIndex < theta_Points; xIndex++)
    {
        for (int yIndex = 0; yIndex < r_Points; yIndex++)
        {
            int idx = yIndex + xIndex * r_Points;
            double x = SurfacePointCloud_Xs[idx]/1000.0;
            double y = SurfacePointCloud_Ys[idx] / 1000.0;
            float z = SurfacePointCloud_ZMs[idx];
            if ((xMin == 0) && (xMax == 0))
            {
                xMin = x;
                xMax = x;
                yMin = y;
                yMax = y;
                zMin = z;
                zMax = z;
            }
            if (xMin > x)
                xMin = x;
            if (xMax < x)
                xMax = x;
            if (yMin > y)
                yMin = y;
            if (yMax < y)
                yMax = y;
            if (zMin > z)
                zMin = z;
            if (zMax < z)
                zMax = z;
           

        }
    }
    double xRange = xMax - yMin;
    double yRange = yMax - yMin;
    double dx = (double)nx / xRange;   // x步长
    double dy = (double)ny / yRange;   // x步长
    double xCen = (xMin + xMax) / 2.0;
    double notchR = xRange / 30;
    notchR = notchR * notchR;
    // 初始化数据网格 (200x200)
  
    QCPColorMapData* data = new QCPColorMapData(nx, ny, QCPRange(xMin, xMax), QCPRange(yMin, yMax));
    data->fillAlpha(0);
   
    for (int yIndex = 0; yIndex < theta_Points; yIndex++) {

        for (int xIndex = 0; xIndex < r_Points; xIndex++) {
            int idx = xIndex + yIndex * r_Points;
            double x = SurfacePointCloud_Xs[idx] / 1000.0;
            double y = SurfacePointCloud_Ys[idx] / 1000.0;
            float z = SurfacePointCloud_ZMs[idx];

            double dr = (x - xCen) * (x - xCen) + (y - yMin) * (y - yMin);
            if (notchR > dr)
                continue;

            int xidx = (x - xMin) * dx;
            int yidx = (y - yMin) * dy;
            data->coordToCell(x,y,&xidx,&yidx);
            data->setCell(xidx,yidx,z);
            data->setAlpha(xidx, yidx, 255);
        }

    }
    m_colorMap->antialiasedFill();
    // 绑定数据并重绘
    m_colorMap->setData(data);
   
    m_colorScale->axis()->setRange(zMin, zMax);
    m_colorMap->rescaleDataRange(true);
    m_customPlot->setAutoFillBackground(true);
    m_customPlot->setMouseTracking(true);
 
    m_customPlot->rescaleAxes();
    m_customPlot->replot();
}
void ChipHeatMapViewer::initViewer(QCustomPlot* customPlot)
{



    // configure axis rect:
    m_customPlot = customPlot;
    m_customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom); // this will also allow rescaling the color scale by dragging/zooming
    m_customPlot->axisRect()->setupFullAxesBox(true);

    //隐藏坐标，上下左右
    m_customPlot->xAxis->setVisible(true);
    m_customPlot->yAxis->setVisible(true);
    m_customPlot->xAxis2->setVisible(true);
    m_customPlot->yAxis2->setVisible(true);
    m_customPlot->yAxis2->setRange(QCPRange(0, 100));
    // set up the QCPColorMap:
    m_colorMap = new QCPColorMap(m_customPlot->xAxis, m_customPlot->yAxis);
    m_colorMap->setAntialiased(true);
 
    // add a color scale:
    m_colorScale = new QCPColorScale(m_customPlot);
    m_customPlot->plotLayout()->addElement(0, 1, m_colorScale); // add it to the right of the main axis rect
    m_colorScale->setType(QCPAxis::atRight); // scale shall be vertical bar with tick/axis labels right (actually atRight is already the default)
    m_colorMap->setColorScale(m_colorScale); // associate the color map with the color scale


    // set the color gradient of the color map to one of the presets:
    m_colorMap->setGradient(QCPColorGradient::gpJet);
    auto g = m_colorMap->gradient();
    g.setLevelCount(100);
    m_colorMap->setGradient(g);
 
    m_colorMap->rescaleDataRange();

    // make sure the axis rect and color scale synchronize their bottom and top margins (so they line up):
    QCPMarginGroup* marginGroup = new QCPMarginGroup(m_customPlot);
    m_customPlot->axisRect()->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);
    m_colorScale->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);

    
   
    // rescale the key (x) and value (y) axes so the whole color map is visible:
    m_customPlot->rescaleAxes();
    m_colorMap->data()->fillAlpha(0);
   
  
}

