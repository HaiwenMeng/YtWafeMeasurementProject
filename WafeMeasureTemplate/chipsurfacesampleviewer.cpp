#include "chipsurfacesampleviewer.h"
#include <QtDataVisualization/QValue3DAxis>
#include <QtDataVisualization/Q3DTheme>
#include <QtGui/QImage>
#include <QtCore/qmath.h>
#include <QScatterDataArray>
#include <QtDataVisualization/QScatterDataProxy> // 包含QScatterDataArray的定义
#include <QtDataVisualization/Q3DScatter>
#include <QtDataVisualization/QScatter3DSeries>
using namespace QtDataVisualization;

const int sampleCountX = 50;
const int sampleCountZ = 50;
const int heightMapGridStepX = 6;
const int heightMapGridStepZ = 6;
const float sampleMin = -8.0f;
const float sampleMax = 8.0f;

ChipSurfaceSampleViewer::ChipSurfaceSampleViewer(Q3DSurface* surface)
    : m_graph(surface)
{

    
    m_graph->setShadowQuality(QAbstract3DGraph::ShadowQualitySoftMedium);
 
    // 启用所有交互（旋转、缩放、平移）
    // 启用所有交互
    m_graph->setFlags(m_graph->flags() | Qt::FramelessWindowHint);

    m_graph->setAxisX(new QValue3DAxis);
    m_graph->setAxisY(new QValue3DAxis);
    m_graph->setAxisZ(new QValue3DAxis);
   // m_graph->setShadowQuality(QAbstract3DGraph::ShadowQualitySoftMedium);
    m_graph->activeTheme()->setType(Q3DTheme::ThemePrimaryColors);
 
    QFont font = m_graph->activeTheme()->font();
    font.setPointSize(50);
    
    m_graph->activeTheme()->setFont(font);
    m_graph->setTitle("点云数据");
    //m_graph->activeTheme()->setAmbientLightStrength(1);
    //m_graph->activeTheme()->setHighlightLightStrength(10);
    // 交换Y轴和Z轴标签
    m_graph->axisX()->setTitleVisible(true);
    m_graph->axisY()->setTitleVisible(true);
    m_graph->axisZ()->setTitleVisible(true);
    m_graph->axisX()->setTitle("X");
    m_graph->axisY()->setTitle("Z"); // 原来Z轴的数据现在在Y轴上显示，所以标签改为Z
    m_graph->axisZ()->setTitle("Y"); // 原来Y轴的数据现在在Z轴上显示，所以标签改为Y

    double minValue = 10;
    double maxValue = 100;
    // 设置颜色映射条（右侧的热度图）
   // m_graph->axisZ()->setLabelFormat("%.2f");  // 设置Z轴标签格式
    //m_graph->axisZ()->setRange(minValue, maxValue);  // 设置Z轴范围
    //m_graph->axisZ()->setTitle("数值");  // 设置Z轴标题

    // 启用颜色映射条并设置位置
    m_graph->axisZ()->setTitleVisible(true);
    m_graph->setAxisZ(new QValue3DAxis());
    m_graph->axisZ()->setTitleVisible(true);
    m_graph->axisZ()->setSegmentCount(5);  // 设置分段数
    m_graph->axisZ()->setLabelAutoRotation(30);  // 设置标签旋转角度

  
   
   
    //! [0]
    m_sqrtSinProxy = new QSurfaceDataProxy();
    m_sqrtSinSeries = new QSurface3DSeries(m_sqrtSinProxy);
    m_sqrtSinSeries->setColorStyle(Q3DTheme::ColorStyleObjectGradient);
   /* QLinearGradient gr;
    gr.setColorAt(0.0, Qt::black);
    gr.setColorAt(0.20, Qt::blue);
    gr.setColorAt(0.45, Qt::cyan);
    gr.setColorAt(0.55, Qt::green);
    gr.setColorAt(0.65, Qt::yellow);
    gr.setColorAt(0.80, Qt::red);
    gr.setColorAt(1.0, Qt::darkRed);
  
    m_sqrtSinSeries->setBaseGradient(gr);*/
    m_sqrtSinSeries->setItemLabelFormat(QStringLiteral("X: @xLabel, Y: @zLabel, Z: @yLabel"));
    m_sqrtSinSeries->setDrawMode(QSurface3DSeries::DrawSurface);
    m_sqrtSinSeries->setFlatShadingEnabled(false);

   // QLinearGradient gradient;
   ///* gr.setColorAt(0.0, Qt::darkGreen);
   // gr.setColorAt(0.5, Qt::yellow);
   // gr.setColorAt(0.8, Qt::red);
   // gr.setColorAt(1.0, Qt::darkRed);*/
   // gradient.setColorAt(0.0, Qt::blue);
   // gradient.setColorAt(0.2, Qt::cyan);
   // gradient.setColorAt(0.5, Qt::green);
   // gradient.setColorAt(0.8, Qt::yellow);
   // gradient.setColorAt(1.0, Qt::red);
   // m_sqrtSinSeries->setBaseGradient(gradient);
   // m_sqrtSinSeries->setColorStyle(Q3DTheme::ColorStyleRangeGradient);
   // m_sqrtSinSeries->setMeshSmooth(true);
  
    m_sqrtSinSeries->setFlatShadingEnabled(true);  // 启用平面着色
    m_graph->addSeries(m_sqrtSinSeries);

    m_graph->scene()->activeCamera()->setCameraPosition(0.0f, 30.0f,120.0F); // 方位角45度，仰角30度
    //m_graph->scene()->activeCamera()->
    //! [0]
   // fillSqrtSinProxy();

  
    
  
}

ChipSurfaceSampleViewer::~ChipSurfaceSampleViewer()
{
  
       /* delete m_series;*/
}

void ChipSurfaceSampleViewer::FillZM3DSurfacePointClouds(std::vector<double> SurfacePointCloud_Xs, std::vector<double> SurfacePointCloud_Ys, std::vector<double> SurfacePointCloud_ZMs, int theta_Points, int r_Points)
{



    QSurfaceDataArray* dataArray = new QSurfaceDataArray;
   

    double xMin = 0, xMax = 0;
    double yMin = 0, yMax = 0;
    double zMin = 0, zMax = 0;
    for (int xIndex = 0; xIndex < theta_Points; xIndex++)
    {
        for (int yIndex = 0; yIndex < r_Points; yIndex++)
        {
            int idx = yIndex + xIndex * r_Points;
            double x = SurfacePointCloud_Xs[idx] / 1000.0;
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
    double zRange = zMax - zMin;
    double xCen = (xMin + xMax) / 2.0;
    double notchR = xRange / 40;
    notchR = notchR * notchR;
    double notchZ = -1;
    double notchy = -1;
    int thetaSampleInterval = 10;
    int rSampleInterval = 30;

   /* QString saveCSVFile = "SurfaceCluldPoints.csv";
     QFile file(saveCSVFile);


     if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
         QTextStream CSVout(&file);*/
    QSurfaceDataRow* firstnNewRow = new QSurfaceDataRow();
        int idx = 0;
      

        for (int i = 0; i < theta_Points; i++) {
       
            QSurfaceDataRow* newRow = new QSurfaceDataRow();
            for (int j = 0; j < r_Points; j++) {
                idx = i * r_Points + j;
                float x = SurfacePointCloud_Xs[idx]/1000.0f;
                float y = SurfacePointCloud_Ys[idx]/1000.0f;
                float z = SurfacePointCloud_ZMs[idx];
                double dr = (x - xCen) * (x - xCen) + (y - yMin) * (y - yMin);
                if (notchR > dr)
                {
                    newRow->append(QVector3D(x, z, y));
                    continue;
                    idx = (i-1) * r_Points-1;                           //计算上一条显示的网格线的最后一个序号
                    z = (z+ SurfacePointCloud_ZMs[idx]) / 2.0;
                    y = ((SurfacePointCloud_Ys[idx- rSampleInterval]/1000.0f) + (SurfacePointCloud_Ys[idx] / 1000.0f)) / 2.0;
                }
               /* if (y == yMin)
                    continue;*/
                if ((j % rSampleInterval == 0) || (i == r_Points - 1)) {
                    newRow->append(QVector3D(x, z, y));
                    if(i==0)
                        firstnNewRow->append(QVector3D(x, z, y));
                }
              /*  CSVout << QString("%1,%2,%3").arg(x).arg(y).arg(z) << "\n";*/

            }
            if (newRow->count()==0)
            {
                continue;
            }
            if((i% thetaSampleInterval ==0)||(i== theta_Points-1))
            dataArray->append(newRow);
        }

        dataArray->append(firstnNewRow);
   
  /*  }
    file.close();*/

    m_graph->axisX()->setRange(xMin, xMax);
    m_graph->axisZ()->setRange(yMin, yMax);
    m_graph->axisY()->setRange(zMin, zMax);
    m_sqrtSinProxy->resetArray(dataArray);
   
    ZMin = zMin;
    ZMax = zMax;
 
   

}
//! [1]
void ChipSurfaceSampleViewer::fillSqrtSinProxy()
{ 

    
  
}
//! [1]

void ChipSurfaceSampleViewer::enableSqrtSinModel(bool enable)
{
  
    if (enable) {
        //! [3]
      
        

    /*    m_graph->axisX()->setLabelFormat("%.2f");
        m_graph->axisZ()->setLabelFormat("%.2f");
        m_graph->axisX()->setRange(sampleMin, sampleMax);
        m_graph->axisY()->setRange(0.0f, 2.0f);
        m_graph->axisZ()->setRange(sampleMin, sampleMax);
        m_graph->axisX()->setLabelAutoRotation(30);
        m_graph->axisY()->setLabelAutoRotation(90);
        m_graph->axisZ()->setLabelAutoRotation(30);*/

      
        //m_graph->addSeries(m_sqrtSinSeries);
        // 设置颜色映射（热度图）
       
        m_sqrtSinSeries->setColorStyle(Q3DTheme::ColorStyleRangeGradient);
       // m_sqrtSinSeries->setDrawMode(Q3DSurface::DrawSurfaceAndWireframe);  // 显示表面和线框
        m_sqrtSinSeries->setFlatShadingEnabled(true);  // 启用平面着色
        
     
                                                       //! [8]
        // Reset range sliders for Sqrt&Sin
        m_rangeMinX = sampleMin;
        m_rangeMinZ = sampleMin;
        m_stepX = (sampleMax - sampleMin) / float(sampleCountX - 1);
        m_stepZ = (sampleMax - sampleMin) / float(sampleCountZ - 1);
      /*  m_axisMinSliderX->setMaximum(sampleCountX - 2);
        m_axisMinSliderX->setValue(0);
        m_axisMaxSliderX->setMaximum(sampleCountX - 1);
        m_axisMaxSliderX->setValue(sampleCountX - 1);
        m_axisMinSliderZ->setMaximum(sampleCountZ - 2);
        m_axisMinSliderZ->setValue(0);
        m_axisMaxSliderZ->setMaximum(sampleCountZ - 1);
        m_axisMaxSliderZ->setValue(sampleCountZ - 1);*/
        //! [8]
    }
}


void ChipSurfaceSampleViewer::adjustXMin(int min)
{
    float minX = m_stepX * float(min) + m_rangeMinX;

    int max = m_axisMaxSliderX->value();
    if (min >= max) {
        max = min + 1;
        m_axisMaxSliderX->setValue(max);
    }
    float maxX = m_stepX * max + m_rangeMinX;

    setAxisXRange(minX, maxX);
}

void ChipSurfaceSampleViewer::adjustXMax(int max)
{
    float maxX = m_stepX * float(max) + m_rangeMinX;

    int min = m_axisMinSliderX->value();
    if (max <= min) {
        min = max - 1;
        m_axisMinSliderX->setValue(min);
    }
    float minX = m_stepX * min + m_rangeMinX;

    setAxisXRange(minX, maxX);
}

void ChipSurfaceSampleViewer::adjustZMin(int min)
{
    float minZ = m_stepZ * float(min) + m_rangeMinZ;

    int max = m_axisMaxSliderZ->value();
    if (min >= max) {
        max = min + 1;
        m_axisMaxSliderZ->setValue(max);
    }
    float maxZ = m_stepZ * max + m_rangeMinZ;

    setAxisZRange(minZ, maxZ);
}

void ChipSurfaceSampleViewer::adjustZMax(int max)
{
    float maxX = m_stepZ * float(max) + m_rangeMinZ;

    int min = m_axisMinSliderZ->value();
    if (max <= min) {
        min = max - 1;
        m_axisMinSliderZ->setValue(min);
    }
    float minX = m_stepZ * min + m_rangeMinZ;

    setAxisZRange(minX, maxX);
}

//! [5]
void ChipSurfaceSampleViewer::setAxisXRange(float min, float max)
{
   // m_graph->axisX()->setRange(min, max);
}

void ChipSurfaceSampleViewer::setAxisZRange(float min, float max)
{
    //m_graph->axisZ()->setRange(min, max);
}
//! [5]

//! [6]
void ChipSurfaceSampleViewer::changeTheme(int theme)
{
   // m_graph->activeTheme()->setType(Q3DTheme::Theme(theme));
}
//! [6]

void ChipSurfaceSampleViewer::setBlackToYellowGradient()
{
    return;
    //! [7]
    QLinearGradient gr;
    gr.setColorAt(0.0, Qt::black);
    gr.setColorAt(0.33, Qt::blue);
    gr.setColorAt(0.67, Qt::red);
    gr.setColorAt(1.0, Qt::yellow);

    m_graph->seriesList().at(0)->setBaseGradient(gr);
    m_graph->seriesList().at(0)->setColorStyle(Q3DTheme::ColorStyleRangeGradient);
    //! [7]
}

void ChipSurfaceSampleViewer::setGreenToRedGradient()
{
    return;
    QLinearGradient gr;
    gr.setColorAt(0.0, Qt::darkGreen);
    gr.setColorAt(0.5, Qt::yellow);
    gr.setColorAt(0.8, Qt::red);
    gr.setColorAt(1.0, Qt::darkRed);

    m_graph->seriesList().at(0)->setBaseGradient(gr);
    m_graph->seriesList().at(0)->setColorStyle(Q3DTheme::ColorStyleRangeGradient);
}


