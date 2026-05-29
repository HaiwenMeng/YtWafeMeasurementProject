#ifndef DIALOGMEASUREPARAMS_H
#define DIALOGMEASUREPARAMS_H

#include <QDialog>

namespace Ui {
class DialogMeasureParams;
}
/// <summary>
/// 定义测量参数的结构体
/// </summary>
struct MeasureValue
{
    float  MinValue;          //测量最小值
    float  MaxValue;          //测量最小值
    bool   IsInvalid;        //是否无效,如果不启用这个参数的控制请将这个值设置为true
    void ResetValue(float minValue, float maxValue, bool isInvalid)
    {
        MinValue = minValue;
        MaxValue = maxValue;
        IsInvalid = isInvalid;
    }
    bool CheckMinMaxValue() 
    {
        return MaxValue >= MinValue;
    }
};
Q_DECLARE_METATYPE(MeasureValue);
class DialogMeasureParams : public QDialog
{
    Q_OBJECT

public:
    explicit DialogMeasureParams(QWidget *parent = nullptr);
    ~DialogMeasureParams();

    
    MeasureValue BOWValue;
    MeasureValue WARPValue;
    MeasureValue CTHKValue;
    MeasureValue ATHKValue;
    MeasureValue TTVValue;
    MeasureValue SORIValue;
    int MeasureParamsId =1;
    void InitMeasureParams(QString recipeName);
    void UpdateParams(int paramsId);
private slots:
    

    void on_doubleSpinBox_maxSORI_valueChanged(double arg1);

    void on_pushButton_ok_clicked();
    void on_pushButton_cancel_clicked();

    void on_checkBox_useSORI_clicked(bool checked);

    void on_checkBox_useTTV_clicked(bool checked);

    void on_checkBox_useATHK_clicked(bool checked);

    void on_checkBox_useCTHK_clicked(bool checked);

    void on_checkBox_useWARP_clicked(bool checked);

    void on_checkBox_useBOW_clicked(bool checked);

    void on_doubleSpinBox_minBOW_valueChanged(double arg1);

    void on_doubleSpinBox_maxBOW_valueChanged(double arg1);

    void on_doubleSpinBox_minWARP_valueChanged(double arg1);

    void on_doubleSpinBox_maxWARP_valueChanged(double arg1);

    void on_doubleSpinBox_minCTHK_valueChanged(double arg1);

    void on_doubleSpinBox_maxCTHK_valueChanged(double arg1);

    void on_doubleSpinBox_minATHK_valueChanged(double arg1);

    void on_doubleSpinBox_maxATHK_valueChanged(double arg1);

    void on_doubleSpinBox_minTTV_valueChanged(double arg1);

    void on_doubleSpinBox_maxTTV_valueChanged(double arg1);

    void on_doubleSpinBox_minSORI_valueChanged(double arg1);

private:
    Ui::DialogMeasureParams *ui;
};


#endif // DIALOGMEASUREPARAMS_H
