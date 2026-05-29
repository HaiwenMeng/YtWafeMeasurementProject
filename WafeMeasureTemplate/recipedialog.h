#ifndef RECIPEDIALOG_H
#define RECIPEDIALOG_H

#include <QDialog>
#include <dialogmeasureparams.h>
#include <QCloseEvent>
namespace Ui {
class RecipeDialog;
}

class RecipeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RecipeDialog(QWidget *parent = nullptr);
    ~RecipeDialog();

    QString RecipeName() const;
    QString MotionPath() const;
    QString ProductThickness() const;
    QString ProductSize() const;
    int  TrimSize() const;
    void InitMeasureParams(QString recipeName);
    QString EditingRecipeName ="";
    MeasureValue BOWValue;
    MeasureValue WARPValue;
    MeasureValue CTHKValue;
    MeasureValue ATHKValue;
    MeasureValue TTVValue;
    MeasureValue SORIValue;

    void SetDefaultName(QString defaultName);
    void SetDefaultTrimSize(int defaultTrimSize);
    void SetDefaultPath(QString defaultPath);
    void SetDefaultThicknesses(QList<int> defaultThicknesses);
    void SetDefaultThickness(QString defaultThickness);
    void SetDefaultSize(QString defaultSize);

protected:
    void accept() override;
private slots:
    void on_lineEdit_recipeName_textChanged(const QString &arg1);

    void onOkClicked();

    void onCancelClicked();
    void on_pushButton_removeThickness_clicked();

    void on_pushButton_addThickness_clicked();



    void on_doubleSpinBox_maxSORI_valueChanged(double arg1);

   

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

    void on_buttonBox_accepted();

private:
    Ui::RecipeDialog *ui;
    void intComboBox();
    bool IsReady = false;
    bool IsInit = false;
};

#endif // RECIPEDIALOG_H
