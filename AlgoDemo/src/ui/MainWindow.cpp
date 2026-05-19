#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->resultTable->setColumnCount(12);
    ui->resultTable->setHorizontalHeaderLabels(QStringList()
        << QStringLiteral("Average THK") << QStringLiteral("Center THK")
        << QStringLiteral("TTV") << QStringLiteral("Bow")
        << QStringLiteral("Bow(X)") << QStringLiteral("Bow(Y)")
        << QStringLiteral("Warp") << QStringLiteral("Warp(X)")
        << QStringLiteral("Warp(Y)") << QStringLiteral("SORI")
        << QStringLiteral("success") << QStringLiteral("errorMessage"));
    ui->resultTable->horizontalHeader()->setStretchLastSection(true);

    ui->detailTable->setColumnCount(8);
    ui->detailTable->setHorizontalHeaderLabels(QStringList()
        << QStringLiteral("lineIndex") << QStringLiteral("pointCount")
        << QStringLiteral("Bow") << QStringLiteral("Warp")
        << QStringLiteral("direction") << QStringLiteral("minThickness")
        << QStringLiteral("maxThickness") << QStringLiteral("averageThickness"));
    ui->detailTable->horizontalHeader()->setStretchLastSection(true);

    connect(ui->btnSelectFile, &QPushButton::clicked, this, &MainWindow::selectCsvFile);
    connect(ui->btnSelectDir, &QPushButton::clicked, this, &MainWindow::selectCsvDirectory);
    connect(ui->btnClear, &QPushButton::clicked, this, &MainWindow::clearData);
    connect(ui->btnValidate, &QPushButton::clicked, this, &MainWindow::validateData);
    connect(ui->btnRunManual, &QPushButton::clicked, this, &MainWindow::runManual);
    connect(ui->btnRunNew, &QPushButton::clicked, this, &MainWindow::runNew);
    connect(ui->btnRunAll, &QPushButton::clicked, this, &MainWindow::runAll);
    connect(ui->btnExportResult, &QPushButton::clicked, this, &MainWindow::exportResultCsv);
    connect(ui->btnExportDetail, &QPushButton::clicked, this, &MainWindow::exportDetailCsv);
    connect(ui->comboProductSize, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::updateRadiusByProductSize);

    updateRadiusByProductSize();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::selectCsvFile()
{
    const QString filePath = QFileDialog::getOpenFileName(this, QStringLiteral("Select CSV file"),
                                                          QString(), QStringLiteral("CSV (*.csv)"));
    if (filePath.isEmpty())
        return;
    applyLoadResult(loader.loadFile(filePath, readOptions()));
}

void MainWindow::selectCsvDirectory()
{
    const QString dirPath = QFileDialog::getExistingDirectory(this, QStringLiteral("Select CSV directory"));
    if (dirPath.isEmpty())
        return;
    applyLoadResult(loader.loadDirectory(dirPath, readOptions()));
}

void MainWindow::clearData()
{
    dataset = Wafer::WaferDataset();
    lastResult = Wafer::AlgorithmResult();
    hasDataset = false;
    hasResult = false;
    ui->lineEditPath->clear();
    ui->textSummary->clear();
    ui->resultTable->setRowCount(0);
    ui->detailTable->setRowCount(0);
    appendLog(QStringLiteral("Data cleared."));
}

void MainWindow::validateData()
{
    if (!ensureDataset())
        return;
    lastResult = algorithm.validateDataset(dataset, readOptions());
    hasResult = true;
    showResult(lastResult);
    appendLogs(lastResult.logs);
}

void MainWindow::runManual()
{
    if (!ensureDataset())
        return;
    lastResult = algorithm.runManualWarp(dataset, readOptions());
    hasResult = true;
    showResult(lastResult);
    appendLogs(lastResult.logs);
}

void MainWindow::runNew()
{
    if (!ensureDataset())
        return;
    lastResult = algorithm.runNewBowWarp(dataset, readOptions());
    hasResult = true;
    showResult(lastResult);
    appendLogs(lastResult.logs);
}

void MainWindow::runAll()
{
    if (!ensureDataset())
        return;
    lastResult = algorithm.runAll(dataset, readOptions());
    hasResult = true;
    showResult(lastResult);
    appendLogs(lastResult.logs);
}

void MainWindow::exportResultCsv()
{
    if (!ensureResult())
        return;
    const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("Export result csv"),
                                                      QStringLiteral("wafer_result.csv"),
                                                      QStringLiteral("CSV (*.csv)"));
    if (path.isEmpty())
        return;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, QStringLiteral("Export failed"),
                              QStringLiteral("Failed to write %1: %2").arg(path, file.errorString()));
        appendLog(QStringLiteral("Export result failed: %1, %2").arg(path, file.errorString()));
        return;
    }
    QTextStream out(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    out.setCodec("UTF-8");
#endif
    out << "Average THK,Center THK,TTV,Bow,Bow(X),Bow(Y),Warp,Warp(X),Warp(Y),SORI,success,errorMessage\n";
    out << displayNumber(lastResult.hasAverageThk, lastResult.averageThk) << ","
        << displayNumber(lastResult.hasCenterThk, lastResult.centerThk) << ","
        << displayNumber(lastResult.hasTtv, lastResult.ttv) << ","
        << displayNumber(lastResult.hasBow, lastResult.bow) << ","
        << displayNumber(lastResult.hasBowX, lastResult.bowX) << ","
        << displayNumber(lastResult.hasBowY, lastResult.bowY) << ","
        << displayNumber(lastResult.hasWarp, lastResult.warp) << ","
        << displayNumber(lastResult.hasWarpX, lastResult.warpX) << ","
        << displayNumber(lastResult.hasWarpY, lastResult.warpY) << ","
        << displayNumber(lastResult.hasSori, lastResult.sori) << ","
        << (lastResult.success ? "true" : "false") << ","
        << '"' << lastResult.errorMessage.replace('"', QStringLiteral("\"\"")) << '"' << "\n";
    appendLog(QStringLiteral("Result exported: %1").arg(path));
}

void MainWindow::exportDetailCsv()
{
    if (!ensureResult())
        return;
    const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("Export detail csv"),
                                                      QStringLiteral("wafer_detail.csv"),
                                                      QStringLiteral("CSV (*.csv)"));
    if (path.isEmpty())
        return;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, QStringLiteral("Export failed"),
                              QStringLiteral("Failed to write %1: %2").arg(path, file.errorString()));
        appendLog(QStringLiteral("Export detail failed: %1, %2").arg(path, file.errorString()));
        return;
    }
    QTextStream out(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    out.setCodec("UTF-8");
#endif
    out << "lineIndex,pointCount,Bow,Warp,direction,minThickness,maxThickness,averageThickness,errorMessage\n";
    for (const Wafer::LineAlgorithmDetail& detail : lastResult.lineDetails) {
        out << detail.lineIndex << ","
            << detail.pointCount << ","
            << displayNumber(detail.hasBow, detail.bow) << ","
            << displayNumber(detail.hasWarp, detail.warp) << ","
            << Wafer::directionToString(detail.direction) << ","
            << displayNumber(detail.hasThicknessStats, detail.minThickness) << ","
            << displayNumber(detail.hasThicknessStats, detail.maxThickness) << ","
            << displayNumber(detail.hasThicknessStats, detail.averageThickness) << ","
            << '"' << QString(detail.errorMessage).replace('"', QStringLiteral("\"\"")) << '"' << "\n";
    }
    appendLog(QStringLiteral("Detail exported: %1").arg(path));
}

void MainWindow::updateRadiusByProductSize()
{
    const int size = ui->comboProductSize->currentText().toInt();
    if (size == 6)
        ui->spinRadius->setValue(75.0);
    else if (size == 12)
        ui->spinRadius->setValue(150.0);
    else
        ui->spinRadius->setValue(100.0);
}

Wafer::AlgorithmOptions MainWindow::readOptions() const
{
    Wafer::AlgorithmOptions options;
    options.lineCount = ui->spinLineCount->value();
    options.productSize = ui->comboProductSize->currentText().toInt();
    options.nominalThickness = ui->spinNominalThickness->value();
    options.calibrationTotal = ui->spinCalibrationTotal->value();
    options.centerX = ui->spinCenterX->value();
    options.centerY = ui->spinCenterY->value();
    options.trimSize = ui->spinTrimSize->value();
    options.chordLength = ui->spinChordLength->value();
    options.lineSampleNum = ui->spinLineSampleNum->value();
    options.radius = ui->spinRadius->value();
    options.radiusCut = ui->spinRadiusCut->value();
    options.thetaPoints = ui->spinThetaPoints->value();
    options.rPoints = ui->spinRPoints->value();
    options.windowSize = ui->spinWindowSize->value();
    options.useNewBowAlg = ui->checkNewBow->isChecked();
    options.useNewWarpAlg = ui->checkNewWarp->isChecked();
    options.useCsvZGravity = ui->checkUseCsvZGravity->isChecked();
    options.allowDebugZeroZGravity = ui->checkDebugZeroZGravity->isChecked();
    options.unitMode = ui->comboUnitMode->currentIndex() == 0 ? Wafer::UnitMode::Millimeter
                                                             : Wafer::UnitMode::Micrometer;
    return options;
}

void MainWindow::applyLoadResult(const Wafer::LoadResult& loadResult)
{
    appendLogs(loadResult.logs);
    if (!loadResult.success) {
        hasDataset = false;
        QMessageBox::warning(this, QStringLiteral("Load failed"), loadResult.errorMessage);
        appendLog(QStringLiteral("Load failed: %1").arg(loadResult.errorMessage));
        return;
    }
    dataset = loadResult.dataset;
    hasDataset = true;
    hasResult = false;
    ui->lineEditPath->setText(dataset.sourcePath);
    ui->textSummary->setPlainText(dataset.summary);
    showDetails(algorithm.buildLineThicknessDetails(dataset));
    appendLog(QStringLiteral("Loaded: %1").arg(dataset.summary));
}

void MainWindow::showResult(const Wafer::AlgorithmResult& result)
{
    ui->resultTable->setRowCount(1);
    QStringList values;
    values << displayNumber(result.hasAverageThk, result.averageThk)
           << displayNumber(result.hasCenterThk, result.centerThk)
           << displayNumber(result.hasTtv, result.ttv)
           << displayNumber(result.hasBow, result.bow)
           << displayNumber(result.hasBowX, result.bowX)
           << displayNumber(result.hasBowY, result.bowY)
           << displayNumber(result.hasWarp, result.warp)
           << displayNumber(result.hasWarpX, result.warpX)
           << displayNumber(result.hasWarpY, result.warpY)
           << displayNumber(result.hasSori, result.sori)
           << (result.success ? QStringLiteral("true") : QStringLiteral("false"))
           << result.errorMessage;
    for (int col = 0; col < values.size(); ++col)
        ui->resultTable->setItem(0, col, new QTableWidgetItem(values.at(col)));
    showDetails(result.lineDetails);
}

void MainWindow::showDetails(const QVector<Wafer::LineAlgorithmDetail>& details)
{
    ui->detailTable->setRowCount(details.size());
    for (int row = 0; row < details.size(); ++row) {
        const Wafer::LineAlgorithmDetail& detail = details.at(row);
        QStringList values;
        values << QString::number(detail.lineIndex)
               << QString::number(detail.pointCount)
               << displayNumber(detail.hasBow, detail.bow)
               << displayNumber(detail.hasWarp, detail.warp)
               << Wafer::directionToString(detail.direction)
               << displayNumber(detail.hasThicknessStats, detail.minThickness)
               << displayNumber(detail.hasThicknessStats, detail.maxThickness)
               << displayNumber(detail.hasThicknessStats, detail.averageThickness);
        for (int col = 0; col < values.size(); ++col)
            ui->detailTable->setItem(row, col, new QTableWidgetItem(values.at(col)));
    }
}

void MainWindow::appendLog(const QString& text)
{
    ui->logText->append(QStringLiteral("[%1] %2")
                            .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")),
                                 text));
}

void MainWindow::appendLogs(const QStringList& logs)
{
    for (const QString& log : logs)
        appendLog(log);
}

QString MainWindow::displayNumber(bool hasValue, double value) const
{
    return hasValue ? QString::number(value, 'g', 12) : QStringLiteral("N/A");
}

bool MainWindow::ensureDataset() const
{
    if (!hasDataset) {
        QMessageBox::information(nullptr, QStringLiteral("No data"), QStringLiteral("Please load csv data first."));
        return false;
    }
    return true;
}

bool MainWindow::ensureResult() const
{
    if (!hasResult) {
        QMessageBox::information(nullptr, QStringLiteral("No result"), QStringLiteral("Please run an algorithm first."));
        return false;
    }
    return true;
}
