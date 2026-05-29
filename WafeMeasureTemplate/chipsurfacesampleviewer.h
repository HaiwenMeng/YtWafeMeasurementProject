#ifndef CHIPSURFACESAMPLEVIEWER_H
#define CHIPSURFACESAMPLEVIEWER_H
#include <QtDataVisualization/Q3DSurface>
#include <QtDataVisualization/QSurfaceDataProxy>
#include <QtDataVisualization/QHeightMapSurfaceDataProxy>
#include <QtDataVisualization/QSurface3DSeries>
#include <QtWidgets/QSlider>
#include <QtDataVisualization/QScatterDataProxy> // ����QScatterDataArray�Ķ���
#include <QtDataVisualization/Q3DScatter>
#include <QtDataVisualization/QScatter3DSeries>
using namespace QtDataVisualization;

class ChipSurfaceSampleViewer : public QObject
{
    Q_OBJECT
public:
    explicit ChipSurfaceSampleViewer(Q3DSurface* surface);
    ~ChipSurfaceSampleViewer();


    void enableSqrtSinModel(bool enable);

    //! [0]
    void toggleModeNone() { m_graph->setSelectionMode(QAbstract3DGraph::SelectionNone); }
    void toggleModeItem() { m_graph->setSelectionMode(QAbstract3DGraph::SelectionItem); }
    void toggleModeSliceRow() {
        m_graph->setSelectionMode(QAbstract3DGraph::SelectionItemAndRow
            | QAbstract3DGraph::SelectionSlice);
    }
    void toggleModeSliceColumn() {
        m_graph->setSelectionMode(QAbstract3DGraph::SelectionItemAndColumn
            | QAbstract3DGraph::SelectionSlice);
    }
    //! [0]

    void setBlackToYellowGradient();
    void setGreenToRedGradient();

    void setAxisMinSliderX(QSlider* slider) { m_axisMinSliderX = slider; }
    void setAxisMaxSliderX(QSlider* slider) { m_axisMaxSliderX = slider; }
    void setAxisMinSliderZ(QSlider* slider) { m_axisMinSliderZ = slider; }
    void setAxisMaxSliderZ(QSlider* slider) { m_axisMaxSliderZ = slider; }

    void adjustXMin(int min);
    void adjustXMax(int max);
    void adjustZMin(int min);
    void adjustZMax(int max);
    void FillZM3DSurfacePointClouds(std::vector<double> SurfacePointCloud_Xs, std::vector<double> SurfacePointCloud_Ys, std::vector<double> SurfacePointCloud_ZMs, int theta_Points,int r_Points);
    double ZMin;
    double ZMax;
    QSurface3DSeries* m_sqrtSinSeries;
public Q_SLOTS:
    void changeTheme(int theme);

private:
    Q3DSurface* m_graph;
    QHeightMapSurfaceDataProxy* m_heightMapProxy;
    QSurfaceDataProxy* m_sqrtSinProxy;
    QSurface3DSeries* m_heightMapSeries;
  

    

    QSlider* m_axisMinSliderX;
    QSlider* m_axisMaxSliderX;
    QSlider* m_axisMinSliderZ;
    QSlider* m_axisMaxSliderZ;
    float m_rangeMinX;
    float m_rangeMinZ;
    float m_stepX;
    float m_stepZ;
    int m_heightMapWidth;
    int m_heightMapHeight;

    void setAxisXRange(float min, float max);
    void setAxisZRange(float min, float max);
    void fillSqrtSinProxy();
   
};


#endif // CHIPSURFACESAMPLEVIEWER_H
