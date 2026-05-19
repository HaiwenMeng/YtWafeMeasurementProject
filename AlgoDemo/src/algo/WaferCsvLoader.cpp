#include "WaferCsvLoader.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>
#include <QtMath>

#include <algorithm>
#include <limits>

namespace Wafer
{

LoadResult WaferCsvLoader::loadFile(const QString& filePath, const AlgorithmOptions& options) const
{
    ParsedCsv parsed = readCsv(filePath);
    if (!parsed.success) {
        LoadResult result;
        result.errorMessage = parsed.errorMessage;
        result.logs = parsed.logs;
        return result;
    }

    LoadResult result = buildFromParsed(parsed, filePath, options, -1);
    if (result.success)
        finalizeSummary(&result.dataset);
    return result;
}

LoadResult WaferCsvLoader::loadDirectory(const QString& dirPath, const AlgorithmOptions& options) const
{
    LoadResult result;
    QDir dir(dirPath);
    if (!dir.exists()) {
        result.errorMessage = QStringLiteral("Directory does not exist: %1").arg(dirPath);
        return result;
    }

    QFileInfoList csvFiles = dir.entryInfoList(QStringList() << QStringLiteral("*.csv"),
                                               QDir::Files | QDir::Readable,
                                               QDir::Name);
    if (csvFiles.isEmpty()) {
        result.errorMessage = QStringLiteral("No csv files found: %1").arg(dirPath);
        return result;
    }

    QVector<QPair<QString, ParsedCsv>> parsedFiles;
    parsedFiles.reserve(csvFiles.size());
    bool allSingleColumn = true;
    for (const QFileInfo& info : csvFiles) {
        ParsedCsv parsed = readCsv(info.absoluteFilePath());
        if (!parsed.success) {
            result.errorMessage = parsed.errorMessage;
            result.logs += parsed.logs;
            return result;
        }
        int maxColumns = 0;
        for (const QStringList& row : parsed.rows)
            maxColumns = std::max(maxColumns, row.size());
        allSingleColumn = allSingleColumn && maxColumns == 1 && !parsed.hasHeader;
        parsedFiles.append(qMakePair(info.absoluteFilePath(), parsed));
    }

    if (allSingleColumn)
        return buildSingleColumnDataset(parsedFiles, dirPath, options);

    QMap<int, LineData> lineMap;
    bool containsZGravity = false;
    bool containsCenter = false;
    QStringList logs;
    for (const QPair<QString, ParsedCsv>& item : parsedFiles) {
        QFileInfo info(item.first);
        int lineIndex = -1;
        QRegularExpression re(QStringLiteral("(?:line_|line)(\\d+)"), QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch match = re.match(info.baseName());
        if (match.hasMatch())
            lineIndex = match.captured(1).toInt();
        if (lineIndex < 0)
            lineIndex = lineMap.size();

        LoadResult lineResult = buildFromParsed(item.second, item.first, options, lineIndex);
        if (!lineResult.success) {
            result.errorMessage = lineResult.errorMessage;
            result.logs = logs + lineResult.logs;
            return result;
        }
        logs += lineResult.logs;
        containsZGravity = containsZGravity || lineResult.dataset.containsZGravity;
        containsCenter = containsCenter || lineResult.dataset.containsCenterPoint;
        for (const LineData& line : lineResult.dataset.lines)
            lineMap[line.lineIndex] = line;
    }

    result.dataset.sourcePath = dirPath;
    result.dataset.lineCount = lineMap.size();
    result.dataset.productSize = options.productSize;
    result.dataset.nominalThickness = options.nominalThickness;
    result.dataset.calibrationTotal = options.calibrationTotal;
    result.dataset.centerX = options.centerX;
    result.dataset.centerY = options.centerY;
    result.dataset.containsZGravity = containsZGravity;
    result.dataset.containsCenterPoint = containsCenter;
    for (auto it = lineMap.constBegin(); it != lineMap.constEnd(); ++it)
        result.dataset.lines.append(it.value());
    finalizeSummary(&result.dataset);
    result.logs = logs;
    result.success = true;
    return result;
}

WaferCsvLoader::ParsedCsv WaferCsvLoader::readCsv(const QString& filePath) const
{
    ParsedCsv result;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.errorMessage = QStringLiteral("Failed to open csv: %1, %2").arg(filePath, file.errorString());
        return result;
    }

    QTextStream in(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    in.setCodec("System");
#endif

    QVector<QStringList> rawRows;
    int lineNumber = 0;
    while (!in.atEnd()) {
        const QString line = in.readLine();
        ++lineNumber;
        if (line.trimmed().isEmpty())
            continue;
        QStringList cells;
        QString error;
        if (!parseLine(line, &cells, &error)) {
            result.errorMessage = QStringLiteral("CSV parse error at %1:%2, %3").arg(filePath).arg(lineNumber).arg(error);
            return result;
        }
        rawRows.append(cells);
    }

    if (rawRows.isEmpty()) {
        result.errorMessage = QStringLiteral("CSV is empty: %1").arg(filePath);
        return result;
    }

    bool firstRowHasText = false;
    for (const QString& cell : rawRows.first()) {
        double value = 0.0;
        if (!toDouble(cell, &value)) {
            firstRowHasText = true;
            break;
        }
    }

    result.hasHeader = firstRowHasText;
    if (result.hasHeader) {
        result.headers = rawRows.takeFirst();
        if (rawRows.isEmpty()) {
            result.errorMessage = QStringLiteral("CSV has header but no data rows: %1").arg(filePath);
            return result;
        }
    }
    result.rows = rawRows;
    result.success = true;
    return result;
}

bool WaferCsvLoader::parseLine(const QString& line, QStringList* cells, QString* error) const
{
    cells->clear();
    QString cell;
    bool inQuotes = false;
    for (int i = 0; i < line.size(); ++i) {
        const QChar ch = line.at(i);
        if (ch == QLatin1Char('"')) {
            if (inQuotes && i + 1 < line.size() && line.at(i + 1) == QLatin1Char('"')) {
                cell.append(ch);
                ++i;
            } else {
                inQuotes = !inQuotes;
            }
        } else if (ch == QLatin1Char(',') && !inQuotes) {
            cells->append(cell.trimmed());
            cell.clear();
        } else {
            cell.append(ch);
        }
    }
    if (inQuotes) {
        if (error)
            *error = QStringLiteral("unterminated quote");
        return false;
    }
    cells->append(cell.trimmed());
    return true;
}

LoadResult WaferCsvLoader::buildFromParsed(const ParsedCsv& parsed, const QString& sourcePath,
                                           const AlgorithmOptions& options, int forcedLineIndex) const
{
    LoadResult result;
    int maxColumns = 0;
    for (const QStringList& row : parsed.rows)
        maxColumns = std::max(maxColumns, row.size());

    if (!parsed.hasHeader && maxColumns == 1) {
        QVector<QPair<QString, ParsedCsv>> files;
        files.append(qMakePair(sourcePath, parsed));
        return buildSingleColumnDataset(files, sourcePath, options);
    }

    QMap<QString, int> headerMap;
    if (parsed.hasHeader) {
        for (int i = 0; i < parsed.headers.size(); ++i)
            headerMap.insert(normalizedHeader(parsed.headers.at(i)), i);
    }

    int cLine = -1;
    int cX = -1;
    int cY = -1;
    int cTop = -1;
    int cBottom = -1;
    int cThk = -1;
    int cZm = -1;
    int cXe = -1;
    int cYe = -1;
    int cZg = -1;

    if (parsed.hasHeader) {
        cLine = findColumn(headerMap, QStringList() << QStringLiteral("lineindex") << QStringLiteral("line"));
        cX = findColumn(headerMap, QStringList() << QStringLiteral("x"));
        cY = findColumn(headerMap, QStringList() << QStringLiteral("y"));
        cTop = findColumn(headerMap, QStringList() << QStringLiteral("topdistance") << QStringLiteral("a"));
        cBottom = findColumn(headerMap, QStringList() << QStringLiteral("bottomdistance") << QStringLiteral("b"));
        cThk = findColumn(headerMap, QStringList() << QStringLiteral("thickness") << QStringLiteral("t"));
        cZm = findColumn(headerMap, QStringList() << QStringLiteral("zm"));
        cXe = findColumn(headerMap, QStringList() << QStringLiteral("xencoder") << QStringLiteral("xen"));
        cYe = findColumn(headerMap, QStringList() << QStringLiteral("yencoder") << QStringLiteral("yen"));
        cZg = findColumn(headerMap, QStringList() << QStringLiteral("zgravity") << QStringLiteral("zg"));
    } else {
        if (maxColumns == 8) {
            cX = 0;
            cY = 1;
            cXe = 2;
            cYe = 3;
            cTop = 4;
            cBottom = 5;
            cThk = 6;
            cZg = 7;
        } else if (maxColumns == 3) {
            result.errorMessage = QStringLiteral("CSV has x,y,distance only. Missing bottom confocal data or thickness data.");
            result.dataset.singleSensorOnly = true;
            return result;
        } else {
            result.errorMessage = QStringLiteral("CSV without header must have 1, 3, or 8 columns: %1").arg(sourcePath);
            return result;
        }
    }

    if (cX < 0 || cY < 0) {
        result.errorMessage = QStringLiteral("Missing x/y columns: %1").arg(sourcePath);
        return result;
    }
    if (cThk < 0 && (cTop < 0 || cBottom < 0)) {
        result.errorMessage = QStringLiteral("Missing bottom confocal data or thickness data: %1").arg(sourcePath);
        return result;
    }

    QMap<int, LineData> lineMap;
    bool containsZGravity = false;
    bool containsCenter = false;
    for (int rowIndex = 0; rowIndex < parsed.rows.size(); ++rowIndex) {
        const QStringList& row = parsed.rows.at(rowIndex);
        auto cell = [&row](int index) -> QString {
            return index >= 0 && index < row.size() ? row.at(index) : QString();
        };

        int lineIndex = forcedLineIndex >= 0 ? forcedLineIndex : 0;
        if (cLine >= 0 && !cell(cLine).isEmpty() && !toInt(cell(cLine), &lineIndex)) {
            result.errorMessage = QStringLiteral("Invalid lineIndex at row %1").arg(rowIndex + 1);
            return result;
        }

        DataPoint point;
        point.lineIndex = lineIndex;
        if (!toDouble(cell(cX), &point.x) || !toDouble(cell(cY), &point.y)) {
            result.errorMessage = QStringLiteral("Invalid x/y at row %1").arg(rowIndex + 1);
            return result;
        }
        if (cTop >= 0 && !cell(cTop).isEmpty()) {
            if (!toDouble(cell(cTop), &point.topDistance)) {
                result.errorMessage = QStringLiteral("Invalid topDistance at row %1").arg(rowIndex + 1);
                return result;
            }
            point.hasTopDistance = true;
        }
        if (cBottom >= 0 && !cell(cBottom).isEmpty()) {
            if (!toDouble(cell(cBottom), &point.bottomDistance)) {
                result.errorMessage = QStringLiteral("Invalid bottomDistance at row %1").arg(rowIndex + 1);
                return result;
            }
            point.hasBottomDistance = true;
        }
        if (cThk >= 0 && !cell(cThk).isEmpty()) {
            if (!toDouble(cell(cThk), &point.thickness)) {
                result.errorMessage = QStringLiteral("Invalid thickness at row %1").arg(rowIndex + 1);
                return result;
            }
            point.hasThickness = true;
        }
        if (!point.hasThickness && point.hasTopDistance && point.hasBottomDistance && options.calibrationTotal > 0.0) {
            point.thickness = options.calibrationTotal - point.topDistance - point.bottomDistance;
            point.hasThickness = true;
        }
        if (cZm >= 0 && !cell(cZm).isEmpty()) {
            if (!toDouble(cell(cZm), &point.zm)) {
                result.errorMessage = QStringLiteral("Invalid zm at row %1").arg(rowIndex + 1);
                return result;
            }
            point.hasZm = true;
        }
        if (cXe >= 0 && !cell(cXe).isEmpty())
            toInt(cell(cXe), &point.xEncoder);
        if (cYe >= 0 && !cell(cYe).isEmpty())
            toInt(cell(cYe), &point.yEncoder);
        if (cZg >= 0 && !cell(cZg).isEmpty()) {
            if (!toDouble(cell(cZg), &point.zGravity)) {
                result.errorMessage = QStringLiteral("Invalid zGravity at row %1").arg(rowIndex + 1);
                return result;
            }
            point.hasZGravity = true;
            containsZGravity = true;
        }

        const bool isCenter = qAbs(point.x - options.centerX) < 1e-6 && qAbs(point.y - options.centerY) < 1e-6;
        if (isCenter) {
            result.dataset.centerPoint.valid = true;
            result.dataset.centerPoint.point = point;
            containsCenter = true;
        }

        if (!lineMap.contains(lineIndex)) {
            LineData line;
            line.lineIndex = lineIndex;
            lineMap.insert(lineIndex, line);
        }
        lineMap[lineIndex].points.append(point);
    }

    if (lineMap.isEmpty()) {
        result.errorMessage = QStringLiteral("No valid data rows: %1").arg(sourcePath);
        return result;
    }

    result.dataset.sourcePath = sourcePath;
    result.dataset.lineCount = lineMap.size();
    result.dataset.productSize = options.productSize;
    result.dataset.nominalThickness = options.nominalThickness;
    result.dataset.calibrationTotal = options.calibrationTotal;
    result.dataset.centerX = options.centerX;
    result.dataset.centerY = options.centerY;
    result.dataset.containsZGravity = containsZGravity;
    result.dataset.containsCenterPoint = containsCenter;
    for (auto it = lineMap.constBegin(); it != lineMap.constEnd(); ++it)
        result.dataset.lines.append(it.value());
    result.success = true;
    return result;
}

LoadResult WaferCsvLoader::buildSingleColumnDataset(const QVector<QPair<QString, ParsedCsv>>& files,
                                                    const QString& sourcePath,
                                                    const AlgorithmOptions& options) const
{
    LoadResult result;
    const int lineCount = files.size() == 1 ? 1 : files.size();
    QVector<PathLine> paths = generatePathLines(options, lineCount);
    if (paths.size() != lineCount) {
        result.errorMessage = QStringLiteral("Failed to rebuild path geometry.");
        return result;
    }

    result.dataset.sourcePath = sourcePath;
    result.dataset.lineCount = lineCount;
    result.dataset.productSize = options.productSize;
    result.dataset.nominalThickness = options.nominalThickness;
    result.dataset.calibrationTotal = options.calibrationTotal;
    result.dataset.centerX = options.centerX;
    result.dataset.centerY = options.centerY;
    result.dataset.thicknessOnly = true;

    for (int fileIndex = 0; fileIndex < files.size(); ++fileIndex) {
        const ParsedCsv& parsed = files.at(fileIndex).second;
        const PathLine path = paths.at(fileIndex);
        LineData line;
        line.lineIndex = fileIndex;
        const int n = parsed.rows.size();
        if (n <= 0) {
            result.errorMessage = QStringLiteral("Empty single-column csv: %1").arg(files.at(fileIndex).first);
            return result;
        }
        for (int i = 0; i < n; ++i) {
            double thickness = 0.0;
            if (parsed.rows.at(i).size() != 1 || !toDouble(parsed.rows.at(i).first(), &thickness)) {
                result.errorMessage = QStringLiteral("Invalid single-column thickness at row %1: %2")
                                          .arg(i + 1).arg(files.at(fileIndex).first);
                return result;
            }
            const double t = n == 1 ? 0.0 : double(i) / double(n - 1);
            DataPoint point;
            point.lineIndex = fileIndex;
            point.x = options.centerX - (path.x1 + t * (path.x2 - path.x1));
            point.y = options.centerY + (path.y1 + t * (path.y2 - path.y1));
            point.thickness = thickness;
            point.hasThickness = true;
            point.xEncoder = qRound(point.x * 2000.0);
            point.yEncoder = qRound(point.y * 2000.0);
            line.points.append(point);
        }
        result.dataset.lines.append(line);
    }

    finalizeSummary(&result.dataset);
    result.logs << QStringLiteral("Single-column csv was loaded as thickness-only data.");
    result.success = true;
    return result;
}

QVector<WaferCsvLoader::PathLine> WaferCsvLoader::generatePathLines(const AlgorithmOptions& options, int lineCount) const
{
    QVector<PathLine> lines;
    if (lineCount <= 0)
        return lines;

    double radiusInner = 100.0 - options.trimSize;
    if (options.productSize == 6)
        radiusInner = 75.0 - options.trimSize;
    else if (options.productSize == 12)
        radiusInner = 150.0 - options.trimSize;

    double stepAngle = 0.0;
    switch (lineCount) {
    case 1:
        stepAngle = 0.0;
        break;
    case 2:
        stepAngle = 90.0;
        break;
    case 4:
        stepAngle = 45.0;
        break;
    case 6:
        stepAngle = 30.0;
        break;
    case 8:
        stepAngle = 22.5;
        break;
    default:
        stepAngle = 360.0 / double(lineCount * 2);
        break;
    }

    bool needLimit = true;
    double chordLimitY = -radiusInner;
    if (options.chordLength > 0.0 && options.chordLength < 2.0 * radiusInner) {
        chordLimitY = -qSqrt(radiusInner * radiusInner - (options.chordLength / 2.0) * (options.chordLength / 2.0));
    } else {
        needLimit = false;
    }

    for (int i = 0; i < lineCount; ++i) {
        const double angleDeg = 270.0 - stepAngle * i;
        const double rad = qDegreesToRadians(angleDeg);
        double x1 = radiusInner * qCos(rad);
        double y1 = radiusInner * qSin(rad);
        if (i % 2 != 0) {
            x1 *= -1.0;
            y1 *= -1.0;
        }
        if (needLimit && y1 < chordLimitY) {
            const double ratio = qFuzzyIsNull(y1) ? 0.0 : x1 / y1;
            y1 = chordLimitY;
            x1 = ratio * y1;
        }
        PathLine line;
        line.x1 = x1;
        line.y1 = y1;
        line.x2 = -x1;
        line.y2 = -y1;
        lines.append(line);
    }
    return lines;
}

bool WaferCsvLoader::toDouble(const QString& text, double* value) const
{
    bool ok = false;
    const double parsed = text.trimmed().toDouble(&ok);
    if (!ok || !qIsFinite(parsed))
        return false;
    if (value)
        *value = parsed;
    return true;
}

bool WaferCsvLoader::toInt(const QString& text, int* value) const
{
    bool ok = false;
    const int parsed = text.trimmed().toInt(&ok);
    if (!ok)
        return false;
    if (value)
        *value = parsed;
    return true;
}

int WaferCsvLoader::findColumn(const QMap<QString, int>& headerMap, const QStringList& names) const
{
    for (const QString& name : names) {
        const QString key = normalizedHeader(name);
        if (headerMap.contains(key))
            return headerMap.value(key);
    }
    return -1;
}

QString WaferCsvLoader::normalizedHeader(const QString& header) const
{
    QString value = header.trimmed().toLower();
    value.remove(QLatin1Char('_'));
    value.remove(QLatin1Char(' '));
    value.remove(QLatin1Char('-'));
    value.remove(QLatin1Char('.'));
    return value;
}

void WaferCsvLoader::finalizeSummary(WaferDataset* dataset) const
{
    QStringList lineCounts;
    bool hasZGravity = false;
    bool hasCenter = dataset->centerPoint.valid;
    for (const LineData& line : dataset->lines) {
        lineCounts << QStringLiteral("%1:%2").arg(line.lineIndex).arg(line.points.size());
        for (const DataPoint& point : line.points) {
            hasZGravity = hasZGravity || point.hasZGravity;
            if (qAbs(point.x - dataset->centerX) < 1e-6 && qAbs(point.y - dataset->centerY) < 1e-6)
                hasCenter = true;
        }
    }
    dataset->lineCount = dataset->lines.size();
    dataset->containsZGravity = dataset->containsZGravity || hasZGravity;
    dataset->containsCenterPoint = dataset->containsCenterPoint || hasCenter;
    dataset->summary = QStringLiteral("lines=%1, totalPoints=%2, perLine=[%3], zGravity=%4, centerPoint=%5, thicknessOnly=%6")
                           .arg(dataset->lineCount)
                           .arg(dataset->totalPointCount())
                           .arg(lineCounts.join(QStringLiteral("; ")))
                           .arg(dataset->containsZGravity ? QStringLiteral("yes") : QStringLiteral("no"))
                           .arg(dataset->containsCenterPoint ? QStringLiteral("yes") : QStringLiteral("no"))
                           .arg(dataset->thicknessOnly ? QStringLiteral("yes") : QStringLiteral("no"));
}

} // namespace Wafer
