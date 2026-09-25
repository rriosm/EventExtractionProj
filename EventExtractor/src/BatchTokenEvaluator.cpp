
#include "BatchTokenEvaluator.h"

qint32 BatchTokenEvaluator::estimateTokens(const QString &text)
{
    qDebug() << "BatchTokenEvaluator::estimateTokens(...)... ";

    if (text.isEmpty()) return 0;

    int estimatedTokens = 0;

    // Static regex to avoid recompiling it millions of times across a large batch
    static QRegularExpression regex("(\\w+|[^\\w\\s])");

    QRegularExpressionMatchIterator it = regex.globalMatch(text);

    while (it.hasNext())
    {
        QRegularExpressionMatch match = it.next();
        QString tokenFragment = match.captured(0);

        if (tokenFragment.length() > 4)
        {
            estimatedTokens += std::ceil(tokenFragment.length() / 4.0);
        } else {
            estimatedTokens += 1;
        }
    }

    return estimatedTokens;
}

void BatchTokenEvaluator::runBatchExperiment(const QString &jsonDirectoryPath, const QString &vttDirectoryPath, const QString &reportOutputPath)
{
    QDir jsonTargetDir(jsonDirectoryPath);

    if (!jsonTargetDir.exists())
    {
        qCritical() << "Operational Error: Target dataset directory does not exist:" << jsonDirectoryPath;

        return;
    }

    QDir vttTargetDir(vttDirectoryPath);

    if (!vttTargetDir.exists())
    {
        qCritical() << "Operational Error: Target dataset directory does not exist:" << vttDirectoryPath;

        return;
    }

    // Scan exclusively for baseline JSON files
    QStringList jsonFilters;
    jsonFilters << "segment_*.json";
    QFileInfoList jsonFiles = jsonTargetDir.entryInfoList(jsonFilters, QDir::Files, QDir::Name);

    qDebug() << "Initiating experiment. Found JSON baselines: " << jsonFiles.size();

    QVector<TrialResult> experimentalTrials;

    qint64 totalJsonTokens = 0;
    qint64 totalVttTokens = 0;

    for (const QFileInfo &jsonInfo : qAsConst(jsonFiles))
    {
        QString baseName = jsonInfo.baseName(); // e.g., "segment_01"

        // Dynamically resolve the expected VTT file name to guarantee 1:1 matching
        QString expectedVttName = baseName + ".webvtt";
        QFileInfo vttInfo(vttTargetDir.absoluteFilePath(expectedVttName));

        // Attempt to resolve matching candidate WebVTT file
        // QString correspondingVttPath = jsonInfo.absolutePath() + "/" + jsonInfo.baseName() + ".webvtt";
        // QFileInfo vttInfo(correspondingVttPath);

        if (!vttInfo.exists() || !vttInfo.isFile())
        {
            qWarning() << "Experiment Warning: Missing matching candidate WebVTT file for baseline:" << expectedVttName;

            continue;
        }

        QString jsonContent = readAllText(jsonInfo.absoluteFilePath());
        QString vttContent = readAllText(vttInfo.absoluteFilePath());

        qint32 jsonTokens = BatchTokenEvaluator::estimateTokens(jsonContent);
        qint32 vttTokens = BatchTokenEvaluator::estimateTokens(vttContent);

        if (jsonTokens == 0)
        {
            qWarning() << "Experiment Warning: JSON baseline yielded 0 tokens. Skipping:" << jsonInfo.fileName();

            continue;
        }

        double reduction = (1.0 - (static_cast<double>(vttTokens) / jsonTokens)) * 100.0;

        TrialResult trial;
        trial.fileName = baseName;
        trial.jsonTokens = jsonTokens;
        trial.vttTokens = vttTokens;
        trial.reductionPercentage = reduction;

        experimentalTrials.append(trial);

        totalJsonTokens += jsonTokens;
        totalVttTokens += vttTokens;
    }

    if (experimentalTrials.isEmpty())
    {
        qWarning() << "Experiment Aborted: No valid matched JSON/WebVTT pairs were identified for evaluation.";
        return;
    }

    // Compute macro performance aggregate
    double macroOverhaulReduction = (1.0 - (static_cast<double>(totalVttTokens) / totalJsonTokens)) * 100.0;

    // Formulate the formatted report matrix buffer
    QString reportBuffer;
    QTextStream stream(&reportBuffer);

    stream << "================================================================================\n";
    stream << "          EXPERIMENT 1: BATCH TOKEN CONSUMPTION OPTIMIZATION AUDIT              \n";
    stream << "================================================================================\n";
    stream << " Execution JSON Context Path: " << jsonTargetDir.absolutePath() << "\n";
    stream << " Execution VTT Context Path: " << vttTargetDir.absolutePath() << "\n";
    stream << " Total Validated Pairs:  " << experimentalTrials.size() << "\n";
    stream << "--------------------------------------------------------------------------------\n";
    stream << QString("%1 | %2 | %3 | %4")
                  .arg("Target Match Clip Name ID", -30)
                  .arg("JSON Tokens", -12)
                  .arg("VTT Tokens", -12)
                  .arg("Reduction %", -12) << "\n";
    stream << "--------------------------------------------------------------------------------\n";

    for (const TrialResult &trial : experimentalTrials)
    {
        stream << QString("%1 | %2 | %3 | %4%")
                      .arg(trial.fileName, -30)
                      .arg(QString::number(trial.jsonTokens), -12)
                      .arg(QString::number(trial.vttTokens), -12)
                      .arg(QString::number(trial.reductionPercentage, 'f', 2), -11)
               << "\n";
    }

    stream << "--------------------------------------------------------------------------------\n";
    stream << " GLOBAL AGGREGATE METRICS SUMMARY:\n";
    stream << "  -> Cumulative Baseline JSON Volume: " << totalJsonTokens << " tokens\n";
    stream << "  -> Cumulative Optimized WebVTT Volume: " << totalVttTokens << " tokens\n";
    stream << "  -> EMPIRICAL BATCH TOKEN OVERHAUL RATIO: " << QString::number(macroOverhaulReduction, 'f', 2) << "%\n";
    stream << "================================================================================\n";

    // Route output to console screen standard out
    QTextStream consoleOut(stdout);
    consoleOut << reportBuffer;
    consoleOut.flush();

    // Optional File Persistence Layer Check
    if (!reportOutputPath.isEmpty())
    {
        QFile reportFile(reportOutputPath);

        if (reportFile.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream fileOut(&reportFile);
            fileOut << reportBuffer;
            reportFile.close();
            qDebug() << "\n[System] Experimental forensic report securely persisted to path:" << reportOutputPath;
        } else {
            qCritical() << "\n[System Error] I/O Failure: Unable to open file descriptors for output path:" << reportOutputPath;
        }
    }
}

QString BatchTokenEvaluator::readAllText(const QString &filePath)
{
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qWarning() << "I/O Error: Cannot open file for reading:" << filePath;

        return QString();
    }

    QTextStream in(&file);
    in.setCodec("UTF-8"); // Ensure safe parsing of international characters

    return in.readAll();
}
