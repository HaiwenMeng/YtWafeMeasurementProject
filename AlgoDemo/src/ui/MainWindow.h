#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "../algo/WaferAlgorithm.h"
#include "../algo/WaferCsvLoader.h"

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void selectCsvFile();
    void selectCsvDirectory();
    void clearData();
    void validateData();
    void runManual();
    void runNew();
    void runAll();
    void exportResultCsv();
    void exportDetailCsv();
    void updateRadiusByProductSize();

private:
    Wafer::AlgorithmOptions readOptions() const;
    void applyLoadResult(const Wafer::LoadResult& loadResult);
    void showResult(const Wafer::AlgorithmResult& result);
    void showDetails(const QVector<Wafer::LineAlgorithmDetail>& details);
    void appendLog(const QString& text);
    void appendLogs(const QStringList& logs);
    QString displayNumber(bool hasValue, double value) const;
    bool ensureDataset() const;
    bool ensureResult() const;

    Ui::MainWindow* ui = nullptr;
    Wafer::WaferCsvLoader loader;
    Wafer::WaferAlgorithm algorithm;
    Wafer::WaferDataset dataset;
    Wafer::AlgorithmResult lastResult;
    bool hasDataset = false;
    bool hasResult = false;
};

#endif // MAINWINDOW_H
