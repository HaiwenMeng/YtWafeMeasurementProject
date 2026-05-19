#ifndef WAFERCSVLOADER_H
#define WAFERCSVLOADER_H

#include "WaferTypes.h"

#include <QMap>
#include <QObject>
#include <QStringList>

namespace Wafer
{

struct LoadResult
{
    bool success = false;
    QString errorMessage;
    QStringList logs;
    WaferDataset dataset;
};

class WaferCsvLoader
{
public:
    LoadResult loadFile(const QString& filePath, const AlgorithmOptions& options) const;
    LoadResult loadDirectory(const QString& dirPath, const AlgorithmOptions& options) const;

private:
    struct ParsedCsv
    {
        bool success = false;
        QString errorMessage;
        QStringList logs;
        bool hasHeader = false;
        QStringList headers;
        QVector<QStringList> rows;
    };

    struct PathLine
    {
        double x1 = 0.0;
        double y1 = 0.0;
        double x2 = 0.0;
        double y2 = 0.0;
    };

    ParsedCsv readCsv(const QString& filePath) const;
    bool parseLine(const QString& line, QStringList* cells, QString* error) const;
    LoadResult buildFromParsed(const ParsedCsv& parsed, const QString& sourcePath,
                               const AlgorithmOptions& options, int forcedLineIndex) const;
    LoadResult buildSingleColumnDataset(const QVector<QPair<QString, ParsedCsv>>& files,
                                        const QString& sourcePath,
                                        const AlgorithmOptions& options) const;
    QVector<PathLine> generatePathLines(const AlgorithmOptions& options, int lineCount) const;
    bool toDouble(const QString& text, double* value) const;
    bool toInt(const QString& text, int* value) const;
    int findColumn(const QMap<QString, int>& headerMap, const QStringList& names) const;
    QString normalizedHeader(const QString& header) const;
    void finalizeSummary(WaferDataset* dataset) const;
};

} // namespace Wafer

#endif // WAFERCSVLOADER_H
