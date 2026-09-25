
#include "inc/Defs.h"
#include "inc/ExtractorUtils.h"
#include "inc/TemporalAlignmentHarness.h"

#include <QDebug>
#include <QFile>
#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>

#include <cmath>
#include <qmath.h>

QVector<AlignmentResult> TemporalAlignmentHarness::evaluateClockDomainAlignment(const QVector<TemporalPayload> &payloads, qint32 toleranceWindow)
{
    QVector<AlignmentResult> results;

    qDebug() << "[Temporal Audit] Executing Event Spotting Alignment on" << payloads.size() << "clips...";

    for (const TemporalPayload &payload : qAsConst(payloads))
    {
        QString rawText = payload.rawModelResponse.trimmed();
        QString deepJsonText = ExtractorUtils::extractGeminiPayloadText(rawText);

        QString cleanStr = deepJsonText.trimmed();

        if (cleanStr.startsWith("```json")) cleanStr = cleanStr.mid(7).trimmed();
        else if (cleanStr.startsWith("```")) cleanStr = cleanStr.mid(3).trimmed();
        if (cleanStr.endsWith("```")) { cleanStr.chop(3); cleanStr = cleanStr.trimmed(); }

        QString repairedJson = ExtractorUtils::repairJson(cleanStr);

        QJsonParseError parseError;
        QJsonDocument targetDoc = QJsonDocument::fromJson(repairedJson.toUtf8(), &parseError);

        // State 1: Actual JSON syntax or serialization failure
        if (targetDoc.isNull() || !targetDoc.isObject())
        {
            AlignmentResult res;
            res.segmentId = payload.segmentId;
            res.videoId = payload.videoId;
            res.jsonFailed = true;
            results.append(res);

            continue;
        }

        QJsonObject rootObj = targetDoc.object();

        // Verify VTT Accuracy Metric (Step 8 CoT)
        // VTT accuracy is nested inside video_clip_info
        bool computedVttAcc = false;

        // Check for 8th Step CoT Verification (VTT Accuracy Metric)
        if (rootObj.contains(*Constants::Q_QGS_KEY_VIDEO_CLIP_INFO))
        {
            QJsonObject clipInfo = rootObj[*Constants::Q_QGS_KEY_VIDEO_CLIP_INFO].toObject();
            computedVttAcc = clipInfo.contains(*Constants::Q_QGS_KEY_VIDEO_CLIP_VTT_ACCURACY);
        }

        // Loop through ALL events in the clip
        if (rootObj.contains(*Constants::Q_QGS_KEY_EVENTS) && rootObj[*Constants::Q_QGS_KEY_EVENTS].isArray())
        {
            QJsonArray eventsArray = rootObj[*Constants::Q_QGS_KEY_EVENTS].toArray();

            // State 2: Valid schema, but 0 events occurred in this 50-second window
            if (eventsArray.isEmpty())
            {
                AlignmentResult res;
                res.segmentId = payload.segmentId;
                res.videoId = payload.videoId;
                res.computedVttAccuracy = computedVttAcc;
                res.noEventsDetected = true; // Logged as neutral state, NOT format failure
                results.append(res);

                continue;
            }

            // State 3: Model detected events, but clip is not mapped in the ground-truth CSV
            if (payload.groundTruthEvents.isEmpty())
            {
                for (const QJsonValue &val : qAsConst(eventsArray))
                {
                    if (!val.isObject()) continue;

                    QJsonObject eventObj = val.toObject();

                    AlignmentResult res;
                    res.segmentId = payload.segmentId;
                    res.videoId = payload.videoId;
                    res.computedVttAccuracy = computedVttAcc;
                    res.eventId = eventObj[*Constants::Q_QGS_KEY_EVENTS_ID].toString("eva_XX");
                    res.missingGroundTruth = true;
                    results.append(res);
                }

                continue;
            }

            for (const QJsonValue &val : qAsConst(eventsArray))
            {
                if (!val.isObject()) continue;

                QJsonObject eventObj = val.toObject();
                AlignmentResult res;
                res.segmentId = payload.segmentId;
                res.videoId = payload.videoId;
                res.computedVttAccuracy = computedVttAcc;
                res.eventId = eventObj[*Constants::Q_QGS_KEY_EVENTS_ID].toString("eva_XX");

                // Extract the model's computed real-world time
                QString timestampReal;
                QJsonValue tsVal = eventObj[*Constants::Q_QGS_KEY_EVENTS_TIMESTAMP_REAL];

                if (tsVal.isString())
                {
                    timestampReal = tsVal.toString();

                }
                else if (tsVal.isDouble())
                {
                    // Captures if the model hallucinates an int (25) or double (25.5)
                    timestampReal = QString::number(tsVal.toDouble());
                }

                res.modelRealSeconds = TemporalAlignmentHarness::parseTimestampToSeconds(timestampReal);

                if (res.modelRealSeconds <= -1)
                {
                    res.timestampParseFailed = true;
                    results.append(res);
                    continue;
                }

                // Nearest-Neighbor Match: Find the closest Ground Truth timestamp for this model event
                qint32 minDelta = std::numeric_limits<qint32>::max();
                qint32 matchedGtSeconds = -1;

                for (const GroundTruthEvent &gt : payload.groundTruthEvents)
                {
                    qint32 gtSeconds = TemporalAlignmentHarness::parseTimestampToSeconds(gt.localTimestamp);

                    if (gtSeconds > -1)
                    {
                        qint32 diff = std::abs(res.modelRealSeconds - gtSeconds);

                        if (diff < minDelta)
                        {
                            minDelta = diff;
                            matchedGtSeconds = gtSeconds;
                        }
                    }
                }

                if (matchedGtSeconds > -1)
                {
                    res.groundTruthRealSeconds = matchedGtSeconds;
                    res.deltaSeconds = minDelta;
                    res.withinTolerance = (res.deltaSeconds <= toleranceWindow);
                }
                else
                {
                    res.jsonFailed = true; // Could not parse GT times
                }

                results.append(res);
            }
        }
        else
        {
            AlignmentResult res;
            res.segmentId = payload.segmentId;
            res.videoId = payload.videoId;
            res.jsonFailed = true;
            results.append(res);
        }
    }

    return results;
}

void TemporalAlignmentHarness::generateAlignmentReport(const QVector<AlignmentResult> &results, qint32 toleranceWindow, const QString &outputPath)
{
    QString report;
    QTextStream stream(&report);

    qint32 totalEvaluatedEvents = 0;
    qint32 totalSuccess = 0;
    qint32 totalVttMetricsComputed = 0;
    qint32 totalEmptyClips = 0;
    qint32 totalMissingGtClips = 0;
    qint32 totalSyntaxFailures = 0;
    double cumulativeDelta = 0.0;

    stream << "==========================================================================================================\n";
    stream << "                      EXPERIMENT 3: PRECISE EVENT SPOTTING & CLOCK-DOMAIN ALIGNMENT                       \n";
    stream << "==========================================================================================================\n";
    stream << QString(" Tolerance Window Threshold: \u00B1%1 seconds\n").arg(toleranceWindow);
    stream << "----------------------------------------------------------------------------------------------------------\n";
    stream << QString("%1 | %2 | %3 | %4 | %5 | %6 | %7\n")
                  .arg("Clip ID", -10)
                  .arg("Video ID (Head)", -15)
                  .arg("Event ID", -10)
                  .arg("GT Time (s)", -12)
                  .arg("Model Time", -12)
                  .arg("Delta (\u0394 t)", -12)
                  .arg("Status", -10);
    stream << "-----------------------------------------------------------------------------------------------------------\n";

    for (const AlignmentResult &res : results)
    {
        if (res.computedVttAccuracy) totalVttMetricsComputed++;

        QString vidHead = res.videoId.isEmpty() ? "MISSING" : res.videoId.left(8) + "..";

        if (res.jsonFailed)
        {
            totalSyntaxFailures++;

            stream << QString("%1 | %2 | %3 | %4 | %5 | %6 | %7\n")
                          .arg(res.segmentId, -10).arg(vidHead, -16).arg("N/A", -10)
                          .arg("N/A", -12).arg("N/A", -12).arg("N/A", -12)
                          .arg("[JSON SYNTAX ERROR]", -24);

            continue;
        }

        if (res.noEventsDetected)
        {
            totalEmptyClips++;
            stream << QString("%1 | %2 | %3 | %4 | %5 | %6 | %7\n")
                          .arg(res.segmentId, -10).arg(vidHead, -16).arg("N/A", -10)
                          .arg("N/A", -12).arg("N/A", -12).arg("N/A", -12)
                          .arg("[NO EVENTS DETECTED]", -24);
            continue;
        }

        if (res.missingGroundTruth)
        {
            totalMissingGtClips++;
            stream << QString("%1 | %2 | %3 | %4 | %5 | %6 | %7\n")
                          .arg(res.segmentId, -10).arg(vidHead, -16).arg(res.eventId, -10)
                          .arg("UNMAPPED", -12).arg(res.modelRealSeconds == -1 ? "N/A" : QString::number(res.modelRealSeconds), -12)
                          .arg("N/A", -12).arg("[MISSING CSV GT]", -24);
            continue;
        }

        if (res.timestampParseFailed)
        {
            stream << QString("%1 | %2 | %3 | %4 | %5 | %6 | %7\n")
            .arg(res.segmentId, -10).arg(vidHead, -16).arg(res.eventId, -10)
                .arg("N/A", -12).arg("UNPARSEABLE", -12).arg("N/A", -12)
                .arg("[TIMESTAMP PARSE ERROR]", -24);
            continue;
        }

        // Successfully evaluated temporal event
        totalEvaluatedEvents++;

        if (res.withinTolerance) totalSuccess++;

        cumulativeDelta += res.deltaSeconds;

        QString status = res.withinTolerance ? "[PASS]" : "[FAIL]";
        stream << QString("%1 | %2 | %3 | %4 | %5 | %6 | %7\n")
                      .arg(res.segmentId, -10)
                      .arg(vidHead, -16)
                      .arg(res.eventId, -10)
                      .arg(res.groundTruthRealSeconds, -12)
                      .arg(res.modelRealSeconds, -12)
                      .arg(res.deltaSeconds, -12)
                      .arg(status);
    }

    double successRate = totalEvaluatedEvents > 0 ? (static_cast<double>(totalSuccess) / totalEvaluatedEvents) * 100.0 : 0.0;
    double avgDelta = totalEvaluatedEvents > 0 ? (cumulativeDelta / totalEvaluatedEvents) : 0.0;
    double vttMetricCompliance = results.isEmpty() ? 0.0 : (static_cast<double>(totalVttMetricsComputed) / results.size()) * 100.0;

    stream << "----------------------------------------------------------------------------------------------------------\n";
    stream << " GLOBAL ALIGNMENT METRICS:\n";
    stream << "  -> Evaluated Event Points:    " << totalEvaluatedEvents << "\n";
    stream << "  -> Average Temporal Error:  " << QString::number(avgDelta, 'f', 2) << " seconds\n";
    stream << "  -> \u0394 t \u2264 " << toleranceWindow << "s Success Rate:  " << QString::number(successRate, 'f', 2) << "%\n";
    stream << "  -> Zero-Event Clips:        " << totalEmptyClips << " (Legitimate inactive windows)\n";
    stream << "  -> Unmapped Video Clips:    " << totalMissingGtClips << " (Pending CSV ground truth)\n";
    stream << "  -> Schema/Syntax Failures:  " << totalSyntaxFailures << "\n";
    stream << "  -> CoT Step 9 Compliance:   " << QString::number(vttMetricCompliance, 'f', 2) << "% (VTT Accuracy Metric Present)\n";
    stream << "==========================================================================================================\n";

    QTextStream console(stdout);
    console << report;
    console.flush();

    if (!outputPath.isEmpty())
    {
        QFile file(outputPath);

        if (file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream fileStream(&file);
            fileStream.setCodec("UTF-8");
            fileStream << report;
            file.close();

            qDebug() << "\n[System] Temporal Alignment report securely persisted to:" << outputPath;
        }
    }
}

GroundTruthMap TemporalAlignmentHarness::loadGroundTruthData(const QString& csvFilePath)
{
    GroundTruthMap truthMap;
    QFile file(csvFilePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qCritical() << "[I/O Error] Failed to open ground-truth CSV file:" << csvFilePath;
        return truthMap;
    }

    QTextStream in(&file);
    in.setCodec("UTF-8");

    bool isFirstLine = true;
    qint32 totalEventsParsed = 0;

    // Variable to hold the dynamically detected delimiter
    QChar delimiter = ',';

    while (!in.atEnd())
    {
        QString line = in.readLine().trimmed();

        if (line.isEmpty()) continue;

        // Handle the CSV Header row if one exists
        if (isFirstLine)
        {
            // Auto-detect delimiter based on character frequency
            if (line.count(';') > line.count(','))
            {
                delimiter = ';';
            } else {
                delimiter = ',';
            }

            isFirstLine = false;

            QStringList headerParts = line.split(delimiter);

            // Assuming the first column header contains "Clip", "ID", or similar
            if (!headerParts.isEmpty() && headerParts.first().contains("Clip", Qt::CaseInsensitive))
            {
                continue;
            }
        }

        QStringList parts = line.split(delimiter);

        if (parts.size() >= 4)
        {
            GroundTruthEvent event;
            event.segmentId       = parts[0].trimmed();
            event.globalTimestamp = parts[1].trimmed();
            event.localTimestamp  = parts[2].trimmed();

            // If Column 04 contains commas (e.g., "Pass, (Everton, #10)"),
            // the split() function divided it. We re-join everything from index 3 onwards.
            event.eventDetails = parts.mid(3).join(',').trimmed();

            // Append the event to the list of events for this specific clip ID
            truthMap[event.segmentId].append(event);
            totalEventsParsed++;
        }
        else
        {
            qWarning() << "[Parse Warning] Malformed CSV line (less than 4 columns):" << line;
        }
    }

    file.close();

    qDebug() << "[Data Loader] Successfully loaded" << totalEventsParsed
             << "ground-truth events across" << truthMap.size() << "unique clips.";

    return truthMap;
}

QVector<TemporalPayload> TemporalAlignmentHarness::loadPayloadsFromFile(const QString& filePath, const GroundTruthMap& groundTruthMap)
{
    QVector<TemporalPayload> payloads;
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qCritical() << "[I/O Error] Failed to open dataset file:" << filePath;

        return payloads;
    }

    QTextStream in(&file);
    in.setCodec("UTF-8");
    QString content = in.readAll();
    file.close();

    if (content.trimmed().isEmpty())
    {
        qWarning() << "[Data Loader] Target dataset file is empty:" << filePath;

        return payloads;
    }

    // Capture the candidate JSON block along with its preceding markdown header tag (e.g., [CLIP_7705ac7e_ITERATION_1])
    static const QRegularExpression headerRegex(QStringLiteral(R"(##\s*\[CLIP_([0-9a-fA-F]+)_ITERATION_(\d+)\])"));
    QRegularExpression jsonCandidateRegex(QStringLiteral(R"(\{\s*\"(?:candidates|usageMetadata)\")"));
    QRegularExpressionMatchIterator it = jsonCandidateRegex.globalMatch(content);

    qint32 clipCounter = 1;

    while (it.hasNext())
    {
        QRegularExpressionMatch match = it.next();
        int startIdx = match.capturedStart();
        int braceCount = 0, endIdx = -1;
        bool inString = false, isEscaped = false;

        for (int i = startIdx; i < content.length(); ++i)
        {
            QChar c = content[i];

            if (isEscaped) { isEscaped = false; continue; }
            if (c == '\\') { isEscaped = true; continue; }
            if (c == '"') { inString = !inString; continue; }

            if (!inString)
            {
                if (c == '{') braceCount++;
                else if (c == '}') braceCount--;
            }

            if (braceCount == 0 && !inString)
            {
                endIdx = i;
                break;
            }
        }

        if (endIdx != -1)
        {
            QString rawJson = content.mid(startIdx, endIdx - startIdx + 1);

            // Step 1: Recover header ID from markdown context above this JSON block
            QString headerClipId;
            int lookbackStart = qMax(0, startIdx - 200);
            QString precedingSnippet = content.mid(lookbackStart, startIdx - lookbackStart);
            QRegularExpressionMatch headerMatch = headerRegex.match(precedingSnippet);

            if (headerMatch.hasMatch())
            {
                headerClipId = headerMatch.captured(1).trimmed();
            }

            // Step 2: Unpack API envelope and clean inner JSON
            QString deepText = ExtractorUtils::extractGeminiPayloadText(rawJson);
            QString cleanStr = ExtractorUtils::stripMarkdown(deepText.trimmed());
            QString repairedJson = ExtractorUtils::repairJson(cleanStr);



            // Extract the hexadecimal video ID for map lookup
            QString videoId;
            QJsonParseError parseError;
            QJsonDocument innerDoc = QJsonDocument::fromJson(repairedJson.toUtf8(), &parseError);

            if (parseError.error == QJsonParseError::NoError)
            {
                videoId = innerDoc.object()
                              .value(*Constants::Q_QGS_KEY_VIDEO_CLIP_INFO).toObject()
                              .value(*Constants::Q_QGS_KEY_VIDEO_CLIP_ID).toString().trimmed();
            }

            // If videoId was missing or empty in JSON, fallback to the markdown header tag
            if (videoId.isEmpty() && !headerClipId.isEmpty())
            {
                videoId = headerClipId;
            }

            TemporalPayload payload;
            payload.segmentId = QString("clip_%1").arg(clipCounter, 2, 10, QChar('0'));;
            payload.videoId = videoId;
            payload.rawModelResponse = rawJson;

            // Step 3: Ground Truth Lookup with safe prefix and full-hash matching
            if (!videoId.isEmpty())
            {
                // Exact key match
                if (groundTruthMap.contains(videoId))
                {
                    payload.groundTruthEvents = groundTruthMap.value(videoId);
                }
                else
                {
                    QString shortId = videoId.left(8);

                    // Search for prefix match (ensure shortId is at least 6 chars to avoid false positives)
                    if (shortId.length() >= 6)
                    {
                        for (GroundTruthMap::const_iterator gtIt = groundTruthMap.cbegin(); gtIt != groundTruthMap.cend(); ++gtIt)
                        {
                            const QString &key = gtIt.key();

                            if (key.startsWith(shortId, Qt::CaseInsensitive) || videoId.startsWith(key, Qt::CaseInsensitive))
                            {
                                payload.groundTruthEvents = gtIt.value();
                                break;
                            }
                        }
                    }
                }
            }

            // Fallback to markdown header ID if payload.groundTruthEvents is still empty
            if (payload.groundTruthEvents.isEmpty() && !headerClipId.isEmpty() && headerClipId != videoId)
            {
                if (groundTruthMap.contains(headerClipId))
                {
                    payload.groundTruthEvents = groundTruthMap.value(headerClipId);
                }
                else
                {
                    for (GroundTruthMap::const_iterator gtIt = groundTruthMap.cbegin(); gtIt != groundTruthMap.cend(); ++gtIt)
                    {
                        if (gtIt.key().startsWith(headerClipId, Qt::CaseInsensitive))
                        {
                            payload.groundTruthEvents = gtIt.value();
                            break;
                        }
                    }
                }
            }

            payloads.append(payload);
            clipCounter++;
        }
    }

    qDebug() << "[Data Loader] Loaded" << payloads.size() << "temporal payloads from" << filePath;
    return payloads;
}

double TemporalAlignmentHarness::parseTimestampToSeconds(const QString &timestampStr)
{
    qDebug() << "TemporalAlignmentHarness::parseTimestampToMilliseconds(...) - Timestamp: " << timestampStr;

    // Clean the string of brackets, quotes, and alphabetic characters (like "s" or "seconds")
    QString cleanStr = timestampStr;
    cleanStr.remove('[').remove(']').remove('"').remove(QRegularExpression("[a-zA-Z\\s]"));

    if (cleanStr.isEmpty())
    {
        qDebug() << "TemporalAlignmentHarness::parseTimestampToMilliseconds(...) - Timestamp is empty! ";

        return -1;
    }

    // Case 1: Model output a range (e.g., "00:45-00:48" or "45-48")
    // Split by hyphen and take the first value
    QString firstTimestamp = cleanStr.split('-').first();

    // 3. Separate the fraction from the main time using a dot or comma
    QStringList dotParts = firstTimestamp.split(QRegularExpression("[\\.,]"));
    QString timePart = dotParts.first(); // e.g., "00:05:00"

    double fraction = 0.0;

    if (dotParts.size() > 1)
    {
        fraction = QString("0." + dotParts.last()).toDouble();
    }

    qDebug() << "TemporalAlignmentHarness::parseTimestampToMilliseconds(...) - Fraction: " << fraction;

    // 4. Split the main time into Hours, Minutes, Seconds
    QStringList timeParts = timePart.split(':');

    qint32 totalSeconds = 0;

    if (timeParts.size() == 3)
    {
        // Format: HH:MM:SS
        totalSeconds = (timeParts[0].toInt() * 3600) + (timeParts[1].toInt() * 60) + timeParts[2].toInt();
    }
    else if (timeParts.size() == 2)
    {
        // Format: MM:SS
        totalSeconds = (timeParts[0].toInt() * 60) + timeParts[1].toInt();
    }
    else if (timeParts.size() == 1)
    {
        // Format: SS (Raw integer hallucination)
        bool isNum;
        totalSeconds = timeParts[0].toInt(&isNum);

        if (!isNum)
        {
            qDebug() << "TemporalAlignmentHarness::parseTimestampToMilliseconds(...) - Raw integer hallucination! ";

            return -3;
        }
    }
    else
    {
        qDebug() << "TemporalAlignmentHarness::parseTimestampToMilliseconds(...) - Parsing failure! ";

        return -3; // Parsing failure
    }

    qDebug() << "TemporalAlignmentHarness::parseTimestampToMilliseconds(...) - totalSeconds: " << totalSeconds;

    // 5. Apply rounding to the nearest whole second based on the fraction (e.g., .750 rounds up)
    qDebug() << "TemporalAlignmentHarness::parseTimestampToMilliseconds(...)" << qRound(totalSeconds + fraction);

    return qRound(totalSeconds + fraction);
}
