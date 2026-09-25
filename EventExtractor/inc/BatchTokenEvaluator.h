#ifndef BATCHTOKENEVALUATOR_H
#define BATCHTOKENEVALUATOR_H

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QString>
#include <QRegularExpression>
#include <QFileInfo>
#include <QVector>

struct TrialResult {
    QString fileName;
    int jsonTokens;
    int vttTokens;
    double reductionPercentage;
};

class BatchTokenEvaluator
{
    public:

    /**
         * @brief estimateTokens: Estimates token counts utilizing a sub-word and punctuation heuristic.
         *                        Mimics BPE mechanics by isolating structural metadata arrays and symbols.
         * @param text
         * @return
         */
        [[nodiscard]] static qint32 estimateTokens(const QString &text);

        /**
         * @brief runBatchExperiment: Executes Experiment 1 across a directory containing paired JSON and WebVTT datasets.
         * @param jsonDirectoryPath: Target folder containing JSON match clips.
         * @param vttDirectoryPath: Target folder containing VTT match clips.
         * @param reportOutputPath: Optional path to persist the final structured report text file.
         */
        static void runBatchExperiment(const QString &jsonDirectoryPath, const QString &vttDirectoryPath, const QString &reportOutputPath = QString());

    private:

        /**
         * @brief readAllText
         * @param filePath
         * @return
         */
        [[nodiscard]] static QString readAllText(const QString &filePath);
};

#endif // BATCHTOKENEVALUATOR_H
