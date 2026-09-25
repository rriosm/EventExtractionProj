
#include "inc/LogUtils.h"

#include "inc/Defs.h"
#include "inc/ExtractorUtils.h"
#include "inc/GovernanceAuditHarness.h"

AuditMetrics GovernanceAuditHarness::auditGovernedOutputs(const QVector<Payload> &payloads)
{
    AuditMetrics metrics;
    metrics.totalEvaluatedRuns = payloads.size();

    qDebug() << "[Governance Audit] Commencing evaluation of" << metrics.totalEvaluatedRuns << "clips...";

    for (const Payload &payload : qAsConst(payloads))
    {
        QString rawText = payload.rawModelResponse.trimmed();

        // Step 1: Unpack API envelope
        QString deepJsonText = ExtractorUtils::extractGeminiPayloadText(rawText);
        QString trimmedText = deepJsonText.trimmed();

        // Step 2: Track Conversational Drift (Deviation)
        // Only flag true prose drift (e.g. "Here is the data..."). Standard JSON block markdown is accepted.
        if (!trimmedText.startsWith("{") &&
            !trimmedText.startsWith("```") &&
            !trimmedText.startsWith("["))
        {
            metrics.markdownOrProseDrift++;
        }

        // Step 3: Strip Markdown Formatting
        QString cleanStr = ExtractorUtils::stripMarkdown(trimmedText);

        const qint32 MAX_ROOT_PENALTY       = Constants::Q_QGS_REQUIRED_ROOT_KEYS->size();
        const qint32 MAX_CLIP_INFO_PENALTY  = Constants::Q_QGS_KEY_VIDEO_CLIP_INFO_KEYS->size();
        const qint32 MAX_VTT_PENALTY        = Constants::Q_QGS_REQUIRED_VTT_VERIFICATION_KEYS->size();
        const qint32 MAX_EVENT_PENALTY      = Constants::Q_QGS_REQUIRED_EVENTS_KEYS->size();

        // Step 4: Base Structural Checks
        if (!cleanStr.startsWith("{"))
        {
            metrics.jsonParsingFailures++;

            metrics.structuralKeyErrorsRoot             += MAX_ROOT_PENALTY;
            metrics.structuralKeyErrorsClipInfo         += MAX_CLIP_INFO_PENALTY;
            metrics.structuralKeyErrorsVTTVerification  += MAX_VTT_PENALTY;
            metrics.structuralKeyErrorsEvents           += MAX_EVENT_PENALTY;

            qWarning() << "[Model Compliance Alert] Clip" << payload.segId << "failed: No JSON object found (Conversational Prose detected).";

            continue;
        }

        // Step 5: Execute Automated Repair for Truncated Responses
        QString repairedJson = ExtractorUtils::repairJson(cleanStr);

        // Step 6: Programmatic parsing of the repaired JSON string
        QJsonParseError parseError;
        QJsonDocument targetDoc = QJsonDocument::fromJson(repairedJson.toUtf8(), &parseError);

        if (targetDoc.isNull() || !targetDoc.isObject())
        {
            metrics.jsonParsingFailures++;

            metrics.structuralKeyErrorsRoot             += MAX_ROOT_PENALTY;
            metrics.structuralKeyErrorsClipInfo         += MAX_CLIP_INFO_PENALTY;
            metrics.structuralKeyErrorsVTTVerification  += MAX_VTT_PENALTY;
            metrics.structuralKeyErrorsEvents           += MAX_EVENT_PENALTY;

            qWarning() << "[Model Compliance Alert] Clip" << payload.segId << "JSON syntax invalid (Unrepairable):" << parseError.errorString();

            continue;
        }

        QJsonObject rootObj = targetDoc.object();

        // Step 7: Check structural keys
        for (const QString &rootKey : *Constants::Q_QGS_REQUIRED_ROOT_KEYS)
        {
            if (!rootObj.contains(rootKey))
            {
                metrics.structuralKeyErrorsRoot++;

                qWarning() << "[Model Compliance Alert] Clip" << payload.segId
                           << "failed schema: Missing root key '" << rootKey << "'.";
            }
        }

        bool videoOnlyCounted = false; // <-- ADD THIS

        // Step 8: Count Structural Video Clip Info Rejections
        // Inspects video_clip_info and adds its elements
        if (rootObj.contains(*Constants::Q_QGS_KEY_VIDEO_CLIP_INFO) &&
            rootObj[*Constants::Q_QGS_KEY_VIDEO_CLIP_INFO].isObject())
        {
            QJsonObject videoClipInfoObj = rootObj[*Constants::Q_QGS_KEY_VIDEO_CLIP_INFO].toObject();

            for (const QString &reqKey : *Constants::Q_QGS_KEY_VIDEO_CLIP_INFO_KEYS)
            {
                if (!videoClipInfoObj.contains((reqKey)))
                {
                    metrics.structuralKeyErrorsClipInfo++;

                    qWarning() << "[Model Compliance Alert] Clip" << payload.segId
                               << "failed schema: Missing required key '" << reqKey << "' in video clip object.";
                }
            }

            // Accumulate video-only events from clip info metadata if present
            if (videoClipInfoObj.contains(*Constants::Q_QGS_KEY_VIDEO_CLIP_VIDEO_ONLY_EVENTS))
            {
                metrics.totalVideoOnlyEvents += videoClipInfoObj.value(*Constants::Q_QGS_KEY_VIDEO_CLIP_VIDEO_ONLY_EVENTS).toInt();

                videoOnlyCounted = true;
            }
        }
        else
        {
            // Apply penalty if video_clip_info object is entirely dropped
            metrics.structuralKeyErrorsClipInfo += MAX_CLIP_INFO_PENALTY;
        }

        // Step 9: Count Structured VTT Rejections
        /* Inspects vtt_verification.rejected_events and adds its element count (rejectedArray.size()) directly
         * to metrics.structuredVttRejections. Falls back to video_clip_info.vtt_events_rejected if
         * vtt_verification is omitted.
         */
        if (rootObj.contains(*Constants::Q_QGS_KEY_VTT_VERIFICATION) &&
            rootObj[*Constants::Q_QGS_KEY_VTT_VERIFICATION].isObject())
        {
            QJsonObject vttObj = rootObj.value(*Constants::Q_QGS_KEY_VTT_VERIFICATION).toObject();

            for (const QString &reqKey : *Constants::Q_QGS_REQUIRED_VTT_VERIFICATION_KEYS)
            {
                if (!vttObj.contains((reqKey)))
                {
                    metrics.structuralKeyErrorsVTTVerification++;

                    qWarning() << "[Model Compliance Alert] Clip" << payload.segId
                               << "failed schema: Missing required key '" << reqKey << "' in video clip object.";
                }
            }

            // Count Confirmed VTT Events
            if (vttObj.contains(*Constants::Q_QGS_KEY_VTT_VERIFICATION_CONFIRMED_EVENTS) &&
                vttObj.value(*Constants::Q_QGS_KEY_VTT_VERIFICATION_CONFIRMED_EVENTS).isArray())
            {
                QJsonArray confirmedArray = vttObj.value(*Constants::Q_QGS_KEY_VTT_VERIFICATION_CONFIRMED_EVENTS).toArray();
                metrics.totalConfirmedVttEvents += confirmedArray.size();
            }

            // Count Rejected VTT Events
            if (vttObj.contains(*Constants::Q_QGS_KEY_VTT_VERIFICATION_REJECTED_EVENTS) &&
                vttObj[*Constants::Q_QGS_KEY_VTT_VERIFICATION_REJECTED_EVENTS].isArray())
            {
                // Count rejected events
                QJsonArray rejectedArray = vttObj.value(*Constants::Q_QGS_KEY_VTT_VERIFICATION_REJECTED_EVENTS).toArray();

                // Increment the count of formally rejected VTT segments
                metrics.structuredVttEventsRejections += rejectedArray.size();
            }
        }
        else
        {
            metrics.structuralKeyErrorsVTTVerification +=  MAX_VTT_PENALTY;

            // Fallback logic moved here to prevent double-counting
            if (rootObj.contains(*Constants::Q_QGS_KEY_VIDEO_CLIP_INFO) &&
                rootObj[*Constants::Q_QGS_KEY_VIDEO_CLIP_INFO].isObject())
            {
                QJsonObject clipInfo = rootObj.value(*Constants::Q_QGS_KEY_VIDEO_CLIP_INFO).toObject();

                if (clipInfo.contains(*Constants::Q_QGS_KEY_VIDEO_CLIP_VTT_EVENTS_REJECTED))
                {
                    metrics.structuredVttEventsRejections += clipInfo.value(*Constants::Q_QGS_KEY_VIDEO_CLIP_VTT_EVENTS_REJECTED).toInt();
                }

                // NOTE: Verify if a constant exists for "vtt_events_confirmed" to replace the hardcoded string
                if (clipInfo.contains(*Constants::Q_QGS_KEY_VIDEO_CLIP_VTT_EVENTS_CONFIRMED))
                {
                    metrics.totalConfirmedVttEvents += clipInfo.value(*Constants::Q_QGS_KEY_VIDEO_CLIP_VTT_EVENTS_CONFIRMED).toInt();
                }
            }
        }

        // Fallback: If video_only_events was not in video_clip_info, count directly from the events array
        if (!videoOnlyCounted &&
            rootObj.contains(*Constants::Q_QGS_KEY_EVENTS) &&
            rootObj[*Constants::Q_QGS_KEY_EVENTS].isArray())
        {
            QJsonArray eventsArray = rootObj[*Constants::Q_QGS_KEY_EVENTS].toArray();

            for (const QJsonValue &value : qAsConst(eventsArray))
            {
                if (!value.isObject()) continue;

                QJsonObject eventObj = value.toObject();
                QString vttStatus = eventObj.value(*Constants::Q_QGS_KEY_EVENTS_VTT_STATUS).toString().toLower();

                if (vttStatus == QStringLiteral("video_only"))
                {
                    metrics.totalVideoOnlyEvents++;
                }
            }
        }

        // Step 10: Verify Hallucinations using ground-truth markers
        if (rootObj.contains(*Constants::Q_QGS_KEY_EVENTS) &&
            rootObj[*Constants::Q_QGS_KEY_EVENTS].isArray())
        {
            QJsonArray eventsArray = rootObj[*Constants::Q_QGS_KEY_EVENTS].toArray();

            // Exhaustive Schema Validation
            // Guarantees every element is checked regardless of logic flags
            for (const QJsonValue &value : qAsConst(eventsArray))
            {
                // Prevent crash if array element is a string/int hallucination
                if (!value.isObject())
                {
                    metrics.structuralKeyErrorsEvents += MAX_EVENT_PENALTY;
                    continue;
                }

                QJsonObject eventObj = value.toObject();

                for (const QString &reqKey : *Constants::Q_QGS_REQUIRED_EVENTS_KEYS)
                {
                    if (!eventObj.contains(reqKey))
                    {
                        metrics.structuralKeyErrorsEvents++;

                        qWarning() << "[Model Compliance Alert] Clip" << payload.segId
                                   << "failed schema: Missing required key '" << reqKey << "' in event object.";
                    }
                }
            }

            // Contextual Hallucination Detection
            // Safely breaks on the first match to prevent multi-counting
            for (const QJsonValue &value : qAsConst(eventsArray))
            {
                if (!value.isObject()) continue;

                QJsonObject eventObj = value.toObject();

                if (eventObj.contains(*Constants::Q_QGS_KEY_EVENTS_CATEGORY) &&
                    eventObj.contains(*Constants::Q_QGS_KEY_EVENTS_VTT_STATUS))
                {
                    QString category = eventObj[*Constants::Q_QGS_KEY_EVENTS_CATEGORY].toString().toUpper();
                    QString vttStatus = eventObj[*Constants::Q_QGS_KEY_EVENTS_VTT_STATUS].toString().toLower();

                    if (vttStatus == *Constants::Q_QGS_STATUS_CONFIRMED)
                    {
                        bool isMonitoredCategory = false;

                        for (const QString &monitoredCat : *Constants::Q_QGS_CRITICAL_CATEGORIES)
                        {
                            if (category == monitoredCat)
                            {
                                isMonitoredCategory = true;

                                break;
                            }
                        }

                        if (isMonitoredCategory && !payload.hasVisualGroundTruthEvent)
                        {
                            metrics.historicalHallucinations++;

                            qDebug() << "[Governance Alert] Contextual hallucination successfully flagged in" << payload.segId << "| Iteration:" << payload.iteration;

                            // Safe to break: schema validation is already complete
                            break;
                        }
                    }
                }
            }
        }
        else
        {
            // Apply penalty if events array is entirely dropped
            metrics.structuralKeyErrorsEvents += MAX_EVENT_PENALTY;
        }

        // Step 11: Track Unstructured Desync Notes (Schema Violations)
        for (const QString &noteKey : *Constants::Q_QGS_UNFORMATTED_NOTE_KEYS) // unformattedNoteKeys)
        {
            if (rootObj.contains(noteKey))
            {
                metrics.unstructuredDesyncNotes++;
                break;
            }
        }
    }

    metrics.totalStructuralKeyErrors = metrics.structuralKeyErrorsRoot +
                                       metrics.structuralKeyErrorsClipInfo +
                                       metrics.structuralKeyErrorsVTTVerification +
                                       metrics.structuralKeyErrorsEvents;

    qDebug() << "[Governance Audit] Batch evaluation completed.";

    return metrics;
}

AuditMetrics GovernanceAuditHarness::auditBaselineOutputs(const QVector<Payload> &payloads)
{
    AuditMetrics metrics;
    metrics.totalEvaluatedRuns = payloads.size();

    qDebug() << "[Baseline Audit] Evaluating" << metrics.totalEvaluatedRuns << "baseline clips...";

    for (const Payload &payload : qAsConst(payloads))
    {
        QString rawText = payload.rawModelResponse.trimmed();

        // Step 1: Unpack envelope and strip Markdown fences
        QString deepText = ExtractorUtils::extractGeminiPayloadText(rawText);
        QString trimmedDeepText = deepText.trimmed();

        // Step 2: Track Conversational/Prose Drift (Deviation)
        if (!trimmedDeepText.startsWith("{") &&
            !trimmedDeepText.startsWith("```") &&
            !trimmedDeepText.startsWith("["))
        {
            metrics.markdownOrProseDrift++;
        }

        // Step 3: Attempt JSON Deserialization. Strip Markdown code fences
        QString cleanStr = ExtractorUtils::stripMarkdown(trimmedDeepText);

        // Step 4: Attempt JSON Deserialization with Automated Repair
        QString repairedJson = ExtractorUtils::repairJson(cleanStr);

        // Step 5: Programmatic parsing of the repaired JSON string
        QJsonParseError parseError;
        QJsonDocument targetDoc = QJsonDocument::fromJson(repairedJson.toUtf8(), &parseError);

        QJsonArray detectedEvents;

        // Calculate the maximum possible schema penalty for completely unstructured payloads
        const qint32 MAX_ROOT_PENALTY = Constants::Q_QGS_REQUIRED_ROOT_KEYS->size();
        const qint32 MAX_CLIP_INFO_PENALTY = Constants::Q_QGS_KEY_VIDEO_CLIP_INFO_KEYS->size();
        const qint32 MAX_VTT_PENALTY = Constants::Q_QGS_REQUIRED_VTT_VERIFICATION_KEYS->size();
        const qint32 MAX_EVENT_PENALTY = Constants::Q_QGS_REQUIRED_EVENTS_KEYS->size();

        if (!targetDoc.isNull())
        {
            if (targetDoc.isObject())
            {
                QJsonObject rootObj = targetDoc.object();

                // Step 6: Check structural keys
                for (const QString &rootKey : *Constants::Q_QGS_REQUIRED_ROOT_KEYS)
                {
                    if (!rootObj.contains(rootKey))
                    {
                        metrics.structuralKeyErrorsRoot++;

                        qWarning() << "[Model Compliance Alert] Clip" << payload.segId
                                   << "failed schema: Missing root key '" << rootKey << "'.";
                    }
                }

                // Step 7: Count Structural Video Clip Info Rejections
                // Inspects video_clip_info and adds its elements
                if (rootObj.contains(*Constants::Q_QGS_KEY_VIDEO_CLIP_INFO) &&
                    rootObj[*Constants::Q_QGS_KEY_VIDEO_CLIP_INFO].isObject())
                {
                    QJsonObject videoClipInfoObj = rootObj[*Constants::Q_QGS_KEY_VIDEO_CLIP_INFO].toObject();

                    for (const QString &reqKey : *Constants::Q_QGS_KEY_VIDEO_CLIP_INFO_KEYS)
                    {
                        if (!videoClipInfoObj.contains((reqKey)))
                        {
                            metrics.structuralKeyErrorsClipInfo++;

                            qWarning() << "[Model Compliance Alert] Clip" << payload.segId
                                       << "failed schema: Missing required key '" << reqKey << "' in video clip object.";
                        }
                    }

                    if (videoClipInfoObj.contains(*Constants::Q_QGS_KEY_VIDEO_CLIP_VIDEO_ONLY_EVENTS))
                    {
                        metrics.totalVideoOnlyEvents += videoClipInfoObj.value(*Constants::Q_QGS_KEY_VIDEO_CLIP_VIDEO_ONLY_EVENTS).toInt();
                    }
                }
                else
                {
                    metrics.structuralKeyErrorsClipInfo += MAX_CLIP_INFO_PENALTY;
                }

                // Step 8: Count Structured VTT Rejections
                if (rootObj.contains(*Constants::Q_QGS_KEY_VTT_VERIFICATION) &&
                    rootObj[*Constants::Q_QGS_KEY_VTT_VERIFICATION].isObject())
                {
                    QJsonObject vttObj = rootObj.value(*Constants::Q_QGS_KEY_VTT_VERIFICATION).toObject();

                    for (const QString &reqKey : *Constants::Q_QGS_REQUIRED_VTT_VERIFICATION_KEYS)
                    {
                        if (!vttObj.contains((reqKey)))
                        {
                            metrics.structuralKeyErrorsVTTVerification++;

                            qWarning() << "[Model Compliance Alert] Clip" << payload.segId
                                       << "failed schema: Missing required key '" << reqKey << "' in video clip object.";
                        }
                    }

                    if (vttObj.contains(*Constants::Q_QGS_KEY_VTT_VERIFICATION_CONFIRMED_EVENTS) &&
                        vttObj.value(*Constants::Q_QGS_KEY_VTT_VERIFICATION_CONFIRMED_EVENTS).isArray())
                    {
                        // Count confirmed events
                        QJsonArray confirmedArray = vttObj.value(*Constants::Q_QGS_KEY_VTT_VERIFICATION_CONFIRMED_EVENTS).toArray();

                        // Increment the count of formally confirmed VTT segments
                        metrics.totalConfirmedVttEvents += confirmedArray.size();
                    }

                    if (vttObj.contains(*Constants::Q_QGS_KEY_VTT_VERIFICATION_REJECTED_EVENTS) &&
                        vttObj[*Constants::Q_QGS_KEY_VTT_VERIFICATION_REJECTED_EVENTS].isArray())
                    {
                        // Count rejected events
                        QJsonArray rejectedArray = vttObj.value(*Constants::Q_QGS_KEY_VTT_VERIFICATION_REJECTED_EVENTS).toArray();

                        // Increment the count of formally rejected VTT segments
                        metrics.structuredVttEventsRejections += rejectedArray.size();
                    }
                }
                else
                {
                    metrics.structuralKeyErrorsVTTVerification +=  MAX_VTT_PENALTY;

                    if (rootObj.contains(*Constants::Q_QGS_KEY_VIDEO_CLIP_INFO) &&
                        rootObj[*Constants::Q_QGS_KEY_VIDEO_CLIP_INFO].isObject())
                    {
                        QJsonObject videoClipInfoObj = rootObj.value(*Constants::Q_QGS_KEY_VIDEO_CLIP_INFO).toObject();

                        if (videoClipInfoObj.contains(*Constants::Q_QGS_KEY_VIDEO_CLIP_VTT_EVENTS_REJECTED))
                        {
                            metrics.structuredVttEventsRejections += videoClipInfoObj.value(*Constants::Q_QGS_KEY_VIDEO_CLIP_VTT_EVENTS_REJECTED).toInt();
                        }

                        if (videoClipInfoObj.contains(*Constants::Q_QGS_KEY_VIDEO_CLIP_VTT_EVENTS_CONFIRMED))
                        {
                            metrics.totalConfirmedVttEvents += videoClipInfoObj.value(*Constants::Q_QGS_KEY_VIDEO_CLIP_VTT_EVENTS_CONFIRMED).toInt();
                        }
                    }
                }

                // Step 9: Check standard data contract key. Extract Standard Events Array
                if (rootObj.contains(*Constants::Q_QGS_KEY_EVENTS) &&
                    rootObj.value(*Constants::Q_QGS_KEY_EVENTS).isArray())
                {
                    detectedEvents = rootObj.value(*Constants::Q_QGS_KEY_EVENTS).toArray();
                }
                else
                {
                    // Step 9.1: Check all known baseline event array keys
                    bool foundFallbackArray = false;

                    for (const QString &fallbackKey : *Constants::Q_QGS_KNOWN_UNSTRUCTURED_EVENT_KEYS)
                    {
                        if (rootObj.contains(fallbackKey) && rootObj.value(fallbackKey).isArray())
                        {
                            // Count unstructured event keys
                            metrics.unstructuredEventsKey++;

                            detectedEvents = rootObj.value(fallbackKey).toArray();

                            foundFallbackArray = true;

                            break;
                        }
                    }

                    // If no array was found at all, apply the maximum event penalty
                    if (!foundFallbackArray) metrics.structuralKeyErrorsEvents += MAX_EVENT_PENALTY;
                }

                // Step 10: Fallback for video_only if omitted from clip_info
                if (detectedEvents.size() > 0 &&
                    (!rootObj.contains(*Constants::Q_QGS_KEY_VIDEO_CLIP_INFO) ||
                     !rootObj[*Constants::Q_QGS_KEY_VIDEO_CLIP_INFO].toObject().contains(*Constants::Q_QGS_KEY_VIDEO_CLIP_VIDEO_ONLY_EVENTS)))
                {
                    for (const QJsonValue &val : qAsConst(detectedEvents))
                    {
                        if (!val.isObject()) continue;

                        QJsonObject evObj = val.toObject();
                        QString vttStatus = evObj.value(*Constants::Q_QGS_KEY_EVENTS_VTT_STATUS).toString().toLower();

                        if (vttStatus == QStringLiteral("video_only"))
                        {
                            metrics.totalVideoOnlyEvents++;
                        }
                    }
                }

                // Step 11: Iterate through whatever array was found and deeply verify its schema
                if (!detectedEvents.isEmpty())
                {
                    for (const QJsonValue &val : qAsConst(detectedEvents))
                    {
                        if (!val.isObject()) continue;

                        QJsonObject eventObj = val.toObject();
                        for (const QString &reqKey : *Constants::Q_QGS_REQUIRED_EVENTS_KEYS)
                        {
                            if (!eventObj.contains(reqKey))
                            {
                                metrics.structuralKeyErrorsEvents++;
                            }
                        }
                    }
                }

                // Track unstructured VTT desync notes inside JSON objects
                for (const QString &noteKey : *Constants::Q_QGS_UNFORMATTED_NOTE_KEYS)
                {
                    if (rootObj.contains(noteKey))
                    {
                        metrics.unstructuredDesyncNotes++;
                        break;
                    }
                }
            }
            else if (targetDoc.isArray())
            {
                // Penalize root array: lacks mandatory root container object
                // BASELINE ARRAY FALLBACK: The model completely bypassed the root object wrapper.
                // Apply the full structural penalty for missing the wrapper objects.
                metrics.structuralKeyErrorsRoot             += MAX_ROOT_PENALTY;
                metrics.structuralKeyErrorsClipInfo         += MAX_CLIP_INFO_PENALTY;
                metrics.structuralKeyErrorsVTTVerification  += MAX_VTT_PENALTY;
                metrics.structuralKeyErrorsEvents           += MAX_EVENT_PENALTY;

                // Handles clips where root is a raw JSON array
                detectedEvents = targetDoc.array();
            }
        }
        else
        {
            // Syntax failure: Unable to deserialize JSON even after repair
            metrics.jsonParsingFailures++;

            // Apply maximum schema penalty for outputting prose instead of JSON
            metrics.structuralKeyErrorsRoot             += MAX_ROOT_PENALTY;
            metrics.structuralKeyErrorsClipInfo         += MAX_CLIP_INFO_PENALTY;
            metrics.structuralKeyErrorsVTTVerification  += MAX_VTT_PENALTY;
            metrics.structuralKeyErrorsEvents           += MAX_EVENT_PENALTY;

            qWarning() << "[Baseline Audit] JSON Parse Failure in clip:" << payload.segId
                       << "-" << parseError.errorString();

            // Track unstructured desync notes in raw prose
            for (const QString &dsyncKey : *Constants::QGS_UNSTRUCTURED_DSYNC_NOTES)
            {
                if (cleanStr.contains(*Constants::Q_QGS_KEY_UNSTRUCUTRED_DESYNC_VTT, Qt::CaseInsensitive) &&
                    cleanStr.contains(dsyncKey, Qt::CaseInsensitive))
                {
                    metrics.unstructuredDesyncNotes++;

                    break;
                }
            }

            // Resilient Fallback: Regex scan for pure Markdown prose event declarations.
            // Extract declared events from Markdown bullet lists
            static const QRegularExpression proseEventRegex(
                QStringLiteral(R"(\*\s*\*\*Event(?:\s*\d*)?:\*\*\s*([A-Za-z\s\-/\(\)]+))"),
                // QStringLiteral(R"(\*\s*\*\*Event:\*\*\s*([A-Za-z\s\-]+))"),
                QRegularExpression::CaseInsensitiveOption);

            QRegularExpressionMatchIterator proseIt = proseEventRegex.globalMatch(cleanStr);

            while (proseIt.hasNext())
            {
                QJsonObject fallbackObj;
                fallbackObj.insert(*Constants::Q_QGS_KEY_UNSTRUCUTRED_EVENT_TYPE, proseIt.next().captured(1).trimmed());
                detectedEvents.append(fallbackObj);
            }
        }

        // Audit Contextual / Historical Hallucinations Against Ground Truth
        if (!detectedEvents.isEmpty())
        {
            bool clipHasCriticalEvent = false;

            for (const QJsonValue &val : qAsConst(detectedEvents))
            {
                QString eventType;
                QString description;

                if (val.isObject())
                {
                    QJsonObject evObj = val.toObject();
                    eventType   = evObj.value(*Constants::Q_QGS_KEY_UNSTRUCUTRED_EVENT_TYPE).toString().toUpper();

                    if (eventType.isEmpty()) eventType = evObj.value(*Constants::Q_QGS_KEY_EVENTS_CATEGORY).toString().toUpper();

                    description = evObj.value(*Constants::Q_QGS_KEY_UNSTRUCUTRED_DESCRIPTION).toString();

                    if (description.isEmpty()) description = evObj.value(*Constants::Q_QGS_KEY_UNSTRUCUTRED_DETAILS).toString();

                    if (description.isEmpty()) description = evObj.value(*Constants::Q_QGS_KEY_UNSTRUCUTRED_COMMENTARY).toString();

                    description = description.toUpper();

                }
                else if (val.isString())
                {
                    eventType = val.toString().toUpper();
                }

                // Check if the baseline model asserted a critical match action
                // Iterate and match against eventType and description
                for (const QString &cat : *Constants::Q_QGS_CRITICAL_CATEGORIES)
                {
                    if (eventType.contains(cat, Qt::CaseInsensitive) ||
                        description.contains(cat, Qt::CaseInsensitive))
                    {
                        clipHasCriticalEvent = true;

                        break; // Breaks the inner category check loop
                    }
                }

                if (clipHasCriticalEvent) break; // Breaks the outer event iteration loop
            }

            // Flag as Hallucination if an event was asserted when no infraction occurred on screen
            if (clipHasCriticalEvent && !payload.hasVisualGroundTruthEvent)
            {
                metrics.historicalHallucinations++;

                qDebug() << "[Baseline Hallucination Flagged] Clip:" << payload.segId
                         << "| Iteration:" << payload.iteration;
            }
        }
    }

    metrics.totalStructuralKeyErrors = metrics.structuralKeyErrorsRoot +
                                       metrics.structuralKeyErrorsClipInfo +
                                       metrics.structuralKeyErrorsVTTVerification +
                                       metrics.structuralKeyErrorsEvents;

    qDebug() << "[Baseline Audit] Completed. Total Hallucinations Flagged:"
             << metrics.historicalHallucinations;

    return metrics;
}

/**
* @brief Formulates and saves a structured evaluation report to disk.
*/
void GovernanceAuditHarness::generateAuditReport(const AuditMetrics &control, const AuditMetrics &governed, const QString &outputPath)
{
    QString report;
    QTextStream stream(&report);

    // Lambda to safely compute average per evaluated clip
    auto calculateAveragePerClip = [](qint32 totalEvents, qint32 totalRuns) -> QString {
        if (totalRuns <= 0) return QStringLiteral("0.00");
        double avg = static_cast<double>(totalEvents) / static_cast<double>(totalRuns);
        return QString::number(avg, 'f', 2);
    };

    stream << "================================================================================\n";
    stream << "        EXPERIMENT 2: GOVERNANCE, RISK, AND HALLUCINATION AUDIT REPORT         \n";
    stream << "================================================================================\n\n";

    stream << QString("%1 | %2 | %3\n")
                  .arg("Compliance Evaluation Metric", -40)
                  .arg("Control Group", -16)
                  .arg("Governed Group", -16);
    stream << "--------------------------------------------------------------------------------\n";
    stream << QString("%1 | %2 | %3\n").arg("Total Evaluated Match Clips", -40)
                  .arg(control.totalEvaluatedRuns, -16)
                  .arg(governed.totalEvaluatedRuns, -16);
    stream << QString("%1 | %2 | %3\n")
                  .arg("Markdown/Conversational Drift/Deviation", -40)
                  .arg(control.markdownOrProseDrift,  -16)
                  .arg(governed.markdownOrProseDrift);
    stream << QString("%1 | %2 | %3\n").arg("JSON Deserialization Crashes", -40)
                  .arg(control.jsonParsingFailures,  -16)
                  .arg(governed.jsonParsingFailures,  -16);

    stream << QString("%1 | %2 | %3\n").arg("Total Mandatory Schema Key Failures", -40)
                  .arg(control.totalStructuralKeyErrors,  -16)
                  .arg(governed.totalStructuralKeyErrors,  -16);
    stream << QString("  %1 | %2 | %3\n").arg("Root Schema Key Failures", -38)
                  .arg(control.structuralKeyErrorsRoot,  -16)
                  .arg(governed.structuralKeyErrorsRoot,  -16);
    stream << QString("  %1 | %2 | %3\n").arg("Clip Info Schema Key Failures", -38)
                  .arg(control.structuralKeyErrorsClipInfo,  -16)
                  .arg(governed.structuralKeyErrorsClipInfo,  -16);
    stream << QString("  %1 | %2 | %3\n").arg("VTT Schema Key Failures", -38)
                  .arg(control.structuralKeyErrorsVTTVerification,  -16)
                  .arg(governed.structuralKeyErrorsVTTVerification,  -16);
    stream << QString("  %1 | %2 | %3\n").arg("Events Schema Key Failures", -38)
                  .arg(control.structuralKeyErrorsEvents,  -16)
                  .arg(governed.structuralKeyErrorsEvents,  -16);

    stream << QString("%1 | %2 | %3\n")
                  .arg("Unstructured VTT Desync Notes", -40)
                  .arg(control.unstructuredDesyncNotes,  -16)
                  .arg(governed.unstructuredDesyncNotes,  -16);
    stream << QString("%1 | %2 | %3\n")
                  .arg("Unstructured Event Keys", -40)
                  .arg(control.unstructuredEventsKey,  -16)
                  .arg(governed.unstructuredEventsKey,  -16);

    // Total & Average Video-Only Events
    stream << QString("%1 | %2 | %3\n")
                  .arg("Total Video-Only Events", -40)
                  .arg(control.totalVideoOnlyEvents, -16)
                  .arg(governed.totalVideoOnlyEvents, -16);
    stream << QString("  %1 | %2 | %3\n")
                  .arg("Avg Video-Only Events / Clip", -38)
                  .arg(calculateAveragePerClip(control.totalVideoOnlyEvents, control.totalEvaluatedRuns), -16)
                  .arg(calculateAveragePerClip(governed.totalVideoOnlyEvents, governed.totalEvaluatedRuns), -16);

    // Total & Average Confirmed VTT Events
    stream << QString("%1 | %2 | %3\n")
                  .arg("Total Confirmed VTT Events", -40)
                  .arg(control.totalConfirmedVttEvents, -16)
                  .arg(governed.totalConfirmedVttEvents, -16);
    stream << QString("  %1 | %2 | %3\n")
                  .arg("Avg Confirmed VTT Events / Clip", -38)
                  .arg(calculateAveragePerClip(control.totalConfirmedVttEvents, control.totalEvaluatedRuns), -16)
                  .arg(calculateAveragePerClip(governed.totalConfirmedVttEvents, governed.totalEvaluatedRuns), -16);

    // Total & Average Rejected VTT Events
    stream << QString("%1 | %2 | %3\n")
                  .arg("Total Structured Rejected VTT Events", -40)
                  .arg(control.structuredVttEventsRejections, -16)
                  .arg(governed.structuredVttEventsRejections, -16);
    stream << QString("  %1 | %2 | %3\n")
                  .arg("Avg Rejected VTT Events / Clip", -38)
                  .arg(calculateAveragePerClip(control.structuredVttEventsRejections, control.totalEvaluatedRuns), -16)
                  .arg(calculateAveragePerClip(governed.structuredVttEventsRejections, governed.totalEvaluatedRuns), -16);


    stream << QString("%1 | %2 | %3\n")
                  .arg("Contextual/Historical Hallucinations", -40)
                  .arg(control.historicalHallucinations,  -16)
                  .arg(governed.historicalHallucinations,  -16);
    stream << "--------------------------------------------------------------------------------\n";

    auto calculateSuccessRate = [](const AuditMetrics& m) -> double {

        if (m.totalEvaluatedRuns == 0) return 0.0;

        qint32 failures = m.markdownOrProseDrift + m.jsonParsingFailures + m.historicalHallucinations;

        return ((m.totalEvaluatedRuns - failures) / static_cast<double>(m.totalEvaluatedRuns)) * 100.0;
    };

    double controlSuccessRate = calculateSuccessRate(control);
    double governedSuccessRate = calculateSuccessRate(governed);

    QString templateText = QString("%1").arg("TOTAL PIPELINE COMPLIANCE RATE", -40);
    QString cRate = QString("%1%").arg(QString::number(qMax(0.0, controlSuccessRate), 'f', 1));
    QString gRate = QString("%1%").arg(QString::number(qMax(0.0, governedSuccessRate), 'f', 1));

    stream << QString("%1 | %2 | %3\n")
                  .arg(templateText, -40)
                  .arg(cRate, -16)
                  .arg(gRate, -16);
    stream << "================================================================================\n";

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

            qDebug() << "\n[System] Governance report securely persisted to:" << outputPath;
        }
        else
        {
            qCritical() << "\n[System Error] I/O Failure: Unable to open file descriptor for output:" << outputPath;
        }
    }
}

QString GovernanceAuditHarness::generateEventsMarkdownTableFromGoverned(const Payload &payload)
{
    QString rawText = payload.rawModelResponse.trimmed();

    // 1. Unpack Gemini API Envelope
    QString deepJsonText = ExtractorUtils::extractGeminiPayloadText(rawText);

    // 2. Strip Markdown fences
    QString cleanStr = ExtractorUtils::stripMarkdown(deepJsonText);

    // 3. Repair incomplete or truncated JSON
    QString repairedJson = ExtractorUtils::repairJson(cleanStr);

    // 4. Parse target JSON document
    QJsonParseError parseError;
    QJsonDocument targetDoc = QJsonDocument::fromJson(repairedJson.toUtf8(), &parseError);

    if (targetDoc.isNull() || !targetDoc.isObject())
    {
        qWarning() << "[Event Table Generator] Failed to parse JSON for segment:"
                   << payload.segId << "-" << parseError.errorString();
        return QStringLiteral("*Error: Could not parse valid JSON for this payload.*");
    }

    QJsonObject rootObj = targetDoc.object();
    QJsonArray eventsArray = rootObj.value(QStringLiteral("events")).toArray();

    if (eventsArray.isEmpty())
    {
        return QStringLiteral("*No events found in this clip segment.*");
    }

    QString tableOutput;
    QTextStream stream(&tableOutput);

    // Table Header
    stream << "| ID | Timestamp | Category | VTT Status | Description | Players Involved | Verdict |\n";
    stream << "| :--- | :--- | :--- | :--- | :--- | :--- | :--- |\n";

    // Table Rows
    for (const QJsonValue &value : qAsConst(eventsArray))
    {
        QJsonObject ev = value.toObject();

        QString id          = ev.value(QStringLiteral("id")).toString();
        QString timestamp   = ev.value(QStringLiteral("timestamp_video")).toString();
        QString category    = ev.value(QStringLiteral("category")).toString();

        // Extract VTT Status and format it (e.g., "video_only" -> "Video only")
        QString vttStatus      = ev.value(QStringLiteral("vtt_status")).toString();
        if (!vttStatus.isEmpty() && vttStatus != QStringLiteral("N/A"))
        {
            vttStatus[0] = vttStatus[0].toUpper();
            vttStatus.replace(QStringLiteral("_"), QStringLiteral(" "));
        }

        QString description = ev.value(QStringLiteral("description")).toString();
        QString verdict     = ev.value(QStringLiteral("verdict")).toString(QStringLiteral("N/A"));
        QString players     = GovernanceAuditHarness::formatPlayersInvolved(ev.value(QStringLiteral("players")).toArray());

        stream << "| **" << id << "** "
               << "| `" << timestamp << "` "
               << "| **" << category << "** "
               << "| " << vttStatus << " "
               << "| " << description << " "
               << "| " << players << " "
               << "| " << verdict << " |\n";
    }

    return tableOutput;
}

QString GovernanceAuditHarness::generateEventsMarkdownTableFromGoverned(const QVector<Payload> &payloads, const QString &targetSegId)
{
    QString combinedOutput;
    QTextStream stream(&combinedOutput);

    for (const Payload &payload : payloads)
    {
        if (!targetSegId.isEmpty() && payload.segId != targetSegId)
        {
            continue;
        }

        stream << QStringLiteral("### Segment: %1 (Iteration %2)\n\n")
                      .arg(payload.segId)
                      .arg(payload.iteration);

        stream << GovernanceAuditHarness::generateEventsMarkdownTableFromGoverned(payload) << "\n\n";
    }

    return combinedOutput;
}

QString GovernanceAuditHarness::generateRejectedVttEventsTableFromGoverned(const Payload &payload)
{
    QString rawText = payload.rawModelResponse.trimmed();

    // 1. Unpack Gemini API Envelope
    QString deepJsonText = ExtractorUtils::extractGeminiPayloadText(rawText);

    // 2. Strip Markdown fences
    QString cleanStr = ExtractorUtils::stripMarkdown(deepJsonText);

    // 3. Repair incomplete or truncated JSON
    QString repairedJson = ExtractorUtils::repairJson(cleanStr);

    // 4. Parse target JSON document
    QJsonParseError parseError;
    QJsonDocument targetDoc = QJsonDocument::fromJson(repairedJson.toUtf8(), &parseError);

    if (targetDoc.isNull() || !targetDoc.isObject())
    {
        qWarning() << "[Rejected VTT Table Generator] Failed to parse JSON for segment:"
                   << payload.segId << "-" << parseError.errorString();
        return QStringLiteral("*Error: Could not parse valid JSON for this payload.*");
    }

    QJsonObject rootObj = targetDoc.object();
    QJsonObject vttVerification = rootObj.value(QStringLiteral("vtt_verification")).toObject();
    QJsonArray rejectedArray = vttVerification.value(QStringLiteral("rejected_events")).toArray();

    if (rejectedArray.isEmpty())
    {
        return QStringLiteral("*No unconfirmed/rejected VTT events found in this clip segment.*");
    }

    QString tableOutput;
    QTextStream stream(&tableOutput);

    // Table Header
    stream << "| VTT Timestamp | Unconfirmed Commentary / Rejection Reason |\n";
    stream << "| :--- | :--- |\n";

    // Table Rows
    for (const QJsonValue &value : qAsConst(rejectedArray))
    {
        QString rawEntry = value.toString().trimmed();
        if (rawEntry.isEmpty()) {
            continue;
        }

        QString timestamp = QStringLiteral("N/A");
        QString description = rawEntry;

        // Split the string at the first colon to separate the timestamp from the commentary
        int colonIdx = rawEntry.indexOf(QLatin1Char(':'));
        if (colonIdx > 0)
        {
            timestamp = rawEntry.left(colonIdx).trimmed();
            description = rawEntry.mid(colonIdx + 1).trimmed();
        }

        // Sanitize text to prevent Markdown table breakage
        description.replace(QLatin1Char('|'), QStringLiteral("\\|"));
        description.replace(QLatin1Char('\n'), QStringLiteral(" "));

        stream << "| `" << timestamp << "` "
               << "| " << description << " |\n";
    }

    return tableOutput;
}

QString GovernanceAuditHarness::generateRejectedVttEventsTableFromGoverned(const QVector<Payload> &payloads, const QString &targetSegId)
{
    QString combinedOutput;
    QTextStream stream(&combinedOutput);

    for (const Payload &payload : payloads)
    {
        if (!targetSegId.isEmpty() && payload.segId != targetSegId)
        {
            continue;
        }

        stream << QStringLiteral("### Segment: %1 (Iteration %2) - Unconfirmed VTT Events\n\n")
                      .arg(payload.segId)
                      .arg(payload.iteration);

        stream << generateRejectedVttEventsTableFromGoverned(payload) << "\n\n";
    }

    return combinedOutput;
}

QString GovernanceAuditHarness::printReport(const QVector<Payload> &payloads)
{
    QString report;
    QTextStream stream(&report);

    stream << "================================================================================\n";
    stream << "                 MATCH VIDEO ANALYSIS REPORT (HUMAN READABLE)                   \n";
    stream << "================================================================================\n\n";

    for (const Payload &payload : payloads)
    {
        stream << "--------------------------------------------------------------------------------\n";
        stream << QString(" CLIP ID: %1 | ITERATION: %2\n").arg(payload.segId).arg(payload.iteration);
        stream << "--------------------------------------------------------------------------------\n";

        // 1. Unpack API envelope -> Strip Markdown -> Auto-Repair Truncated JSON
        QString rawPayload = ExtractorUtils::extractGeminiPayloadText(payload.rawModelResponse);
        QString cleanJson  = ExtractorUtils::stripMarkdown(rawPayload);
        QString repaired   = ExtractorUtils::repairJson(cleanJson);

        // 2. Deserialize JSON
        QJsonParseError parseErr;
        QJsonDocument doc = QJsonDocument::fromJson(repaired.toUtf8(), &parseErr);

        if (doc.isNull() || !doc.isObject())
        {
            stream << " [!] JSON Parsing Failed: " << parseErr.errorString() << "\n\n";
            continue;
        }

        QJsonObject rootObj                         = doc.object();

        QJsonObject clipInfo                        = rootObj.value(QStringLiteral("video_clip_info")).toObject();
        QJsonArray priorityEventsDetected           = clipInfo.value(QStringLiteral("priority_events_detected")).toArray();
        QJsonArray missedEventsByVTT                = clipInfo.value(QStringLiteral("missed_events_by_vtt")).toArray();

        QJsonObject vttVerification                 = rootObj.value(QStringLiteral("vtt_verification")).toObject();
        QJsonArray vttVerificationConfirmedEvents   = vttVerification.value(QStringLiteral("confirmed_events")).toArray();
        QJsonArray vttVerificationRejectedEvents    = vttVerification.value(QStringLiteral("rejected_events")).toArray();

        QJsonArray events                           = rootObj.value(QStringLiteral("events")).toArray();

        // 3. Print Metadata & Statistics
        stream << QString("  Clip ID : %1")
                      .arg(clipInfo.value(QStringLiteral("clip_video_id")).toString());
        stream << QString("  Time Window  : %1 -> %2\n")
                      .arg(clipInfo.value(QStringLiteral("clip_start_video")).toString(),
                           clipInfo.value(QStringLiteral("clip_end_video")).toString());
        stream << QString("  Analysis Date: %1\n")
                      .arg(clipInfo.value(QStringLiteral("analysis_date")).toString());
        stream << QString("  Slowdown Factor: %1\n")
                      .arg(clipInfo.value(QStringLiteral("slowdown_factor")).toDouble());

        stream << QString("  Priority Events Detected: ");

        for (const auto &val : qAsConst(priorityEventsDetected))
        {
            stream << QString("%1; ").arg(val.toString());
        }

        stream << QString("\n");

        stream << QString("  VTT Accuracy : %1 (Confirmed: %2, Rejected: %3)\n")
                      .arg(clipInfo.value(QStringLiteral("vtt_accuracy")).toString())
                      .arg(clipInfo.value(QStringLiteral("vtt_events_confirmed")).toInt())
                      .arg(clipInfo.value(QStringLiteral("vtt_events_rejected")).toInt());

        stream << QString("  Missed Events by VTT: ");

        for (const auto &val : qAsConst(missedEventsByVTT))
        {
            stream << QString("%1 ").arg(val.toString());
        }

        stream << QString("\n");

        stream << QString("  Total Events : %1 (Video-Only: %2)\n\n")
                      .arg(clipInfo.value(QStringLiteral("total_events")).toInt())
                      .arg(clipInfo.value(QStringLiteral("video_only_events")).toInt());

        // 4. Print Event Log
        if (events.isEmpty())
        {
            stream << "  [Events Log] No events detected for this segment.\n\n";
            continue;
        }

        stream << QString("  [EVENTS LOG: %1 Detected]\n").arg(events.size());

        for (qint32 i = 0; i < events.size(); ++i)
        {
            QJsonObject ev = events.at(i).toObject();

            stream << QString("   #%1 [%2] Category: %3 | Video Time: %4  | Video Real Time: %5 | VTT Time: %6 | Priority: %7\n")
                          .arg(i + 1, 2, 10, QChar('0'))
                          .arg(ev.value(QStringLiteral("id")).toString(), -7)
                          .arg(ev.value(QStringLiteral("category")).toString(), -22)
                          .arg(ev.value(QStringLiteral("timestamp_video")).toString(), -12)
                          .arg(ev.value(QStringLiteral("timestamp_real")).toString(), -12)
                          .arg(ev.value(QStringLiteral("timestamp_vtt")).toString(), -12)
                          .arg(ev.value(QStringLiteral("priority_level")).toInt());

            stream << QString("       VTT Match: %1\n")
                          .arg(ev.value(QStringLiteral("vtt_match")).toString());
            stream << QString("       Event Type: %1\n")
                          .arg(ev.value(QStringLiteral("type")).toString());
            stream << QString("       VTT Status: %1\n")
                          .arg(ev.value(QStringLiteral("vtt_status")).toString());

            // Format players using your class function
            QString playersFormatted = GovernanceAuditHarness::formatPlayersInvolved(
                ev.value(QStringLiteral("players")).toArray());

            stream << "       Players : " << playersFormatted << "\n";
            stream << "       Desc    : " << ev.value(QStringLiteral("description")).toString() << "\n";
            stream << "       Verdict : " << ev.value(QStringLiteral("verdict")).toString(QStringLiteral("N/A")) << "\n";
            stream << "       Analysis: " << ev.value(QStringLiteral("expert_analysis")).toString() << "\n";
            stream << "       Referee Action: " << ev.value(QStringLiteral("referee_action")).toString() << "\n";
            stream << "       Previous Temporal Event: " << ev.value(QStringLiteral("prev_temporal")).toString() << "\n";
            stream << "       Previous Causal Event: " << ev.value(QStringLiteral("prev_causal")).toString() << "\n";
            stream << "       Broadcasters: " << ev.value(QStringLiteral("broadcasters")).toString() << "\n";
            stream << "       Confidence: " << ev.value(QStringLiteral("confidence")).toString() << "\n";
            stream << "       -------------------------------------------------------------------------\n";
        }

        stream << QString("  [EVENTS LOG: %1 VTT Rejected]\n").arg(vttVerificationRejectedEvents.size());

        for (qint32 i = 0; i < vttVerificationRejectedEvents.size(); ++i)
        {
            stream << QString("   #%1 Description %2\n")
                          .arg(i + 1, 2, 10, QChar('0'))
                          .arg(vttVerificationRejectedEvents.at(i).toString());
        }

        stream << "\n";
    }

    return report;
}

QVector<Payload> GovernanceAuditHarness::loadPayloadsFromFile(const QString& filePath, bool visualRealityFlag)
{
    QVector<Payload> payloads;
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

    // Strategy 1: Section Header Parsing for Consolidated Iteration Files
    // Matches headers like "## [CLIP_01_ITERATION_1]" or "## [CLIP_02_ITERATION_5]"
    static const QRegularExpression tagRegex(R"(##\s*\[(?<prefix>[A-Za-z]+)_(?<clipNum>[A-Za-z0-9\-]+)_(?<iterLabel>[A-Za-z]+)_(?<iterNum>\d+)\])");
    // static const QRegularExpression tagRegex(R"(##\s*\[CLIP(?<prefix>[A-Za-z]+)_(?<clipNum>[A-Za-z0-9\-]+)_(?<iterLabel>[A-Za-z]+)_(?<iterNum>\d+)\])");

    QRegularExpressionMatchIterator tagIt = tagRegex.globalMatch(content);

    if (tagIt.hasNext())
    {
        QList<QRegularExpressionMatch> matches;

        while (tagIt.hasNext())
        {
            matches.append(tagIt.next());
        }

        for (qint32 i = 0; i < matches.size(); ++i)
        {
            const QRegularExpressionMatch &currentMatch = matches[i];

            // 1. Extract the 4 tokenized components
            QString tagPrefix       = currentMatch.captured("prefix");          // e.g., "CLIP"
            QString tagClipNum      = currentMatch.captured("clipNum");         // e.g., "dk740YTv" (or "" if missing)
            QString tagIterLabel    = currentMatch.captured("iterLabel");       // e.g., "ITERATION"
            qint32  tagIterNum      = currentMatch.captured("iterNum").toInt(); // e.g., 1

            // 2. Extract the full string for legacy payload compatibility
            QString fullTagId       = currentMatch.captured("fullTag");         // e.g., "CLIP_01_ITERATION_1"

            // 3. Extract the JSON Code Payload
            qint32 startIdx = currentMatch.capturedEnd();
            qint32 endIdx = (i + 1 < matches.size()) ? matches[i + 1].capturedStart() : content.length();

            QString sectionPayload = content.mid(startIdx, endIdx - startIdx).trimmed();

            if (!sectionPayload.isEmpty())
            {
                Payload payload;
                payload.segId = tagClipNum;
                payload.iteration = tagIterNum;
                payload.hasVisualGroundTruthEvent = visualRealityFlag;
                payload.rawModelResponse = sectionPayload; // The JSON string

                payloads.append(payload);

#ifdef QT_DEBUG
                LogUtils::safeLog(QString("GovernanceAuditHarness::loadPayloadsFromFile(...) - Payload: %1!")
                                      .arg(payload.rawModelResponse),
                                  __FILE__,
                                  __LINE__,
                                  Q_FUNC_INFO);
#endif
            }
        }

        qDebug() << "[Data Loader] Successfully parsed" << payloads.size() << "tagged iteration payloads from:" << filePath;

        return payloads;
    }

    // Strategy 2: Legacy Fallback for Un-tagged Raw API Envelopes
    // Regex finds the start of every distinct API candidate envelope
    QRegularExpression envelopeRegex("\\{\\s*\"(?:candidates|usageMetadata)\"");
    QRegularExpressionMatchIterator it = envelopeRegex.globalMatch(content);

    qint32 clipCounter = 1;

    while (it.hasNext())
    {
        QRegularExpressionMatch match = it.next();
        qint32 startIdx = match.capturedStart();

        qint32 braceCount = 0;
        bool inString = false;
        bool isEscaped = false;
        qint32 endIdx = -1;

        // Brace-counting safely finds the exact bounds of the outer API JSON object
        for (qint32 i = startIdx; i < content.length(); ++i)
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
            Payload payload;
            payload.segId = QString("%1%2")
                                .arg(*Constants::Q_QGS_CLIP_TAG_STR)
                                .arg(clipCounter, 2, 10, QChar('0'));
            payload.hasVisualGroundTruthEvent = visualRealityFlag;
            payload.rawModelResponse = content.mid(startIdx, endIdx - startIdx + 1);
            payloads.append(payload);
            clipCounter++;
        }
    }

    if (payloads.isEmpty())
    {
        Payload payload;
        payload.segId = QString("%1UNKNOWN")
                             .arg(*Constants::Q_QGS_CLIP_TAG_STR);
        payload.hasVisualGroundTruthEvent = visualRealityFlag;
        payload.rawModelResponse = content;
        payloads.append(payload);
    }

    qDebug() << "[Data Loader] Successfully extracted" << payloads.size() << "clips from:" << filePath;

    return payloads;
}

// QVector<ClipExtractionResult> GovernanceAuditHarness::parseMultiClipFile(const QString &fileContent)
// {
//     QVector<ClipExtractionResult> results;

//     if (fileContent.trimmed().isEmpty())
//     {
//         return results;
//     }

//     // Matches markdown section headers: ## [CLIP_<id>_ITERATION_<iter>]
//     static const QRegularExpression headerRegex(
//         QStringLiteral(R"(##\s*\[CLIP_([a-zA-Z0-9]+)_ITERATION_(\d+)\])"));

//     QRegularExpressionMatchIterator it = headerRegex.globalMatch(fileContent);
//     QVector<QPair<QRegularExpressionMatch, int>> matchPositions;

//     while (it.hasNext())
//     {
//         QRegularExpressionMatch match = it.next();
//         matchPositions.append({match, match.capturedStart()});
//     }

//     for (int i = 0; i < matchPositions.size(); ++i)
//     {
//         ClipExtractionResult result;
//         const QRegularExpressionMatch &currMatch = matchPositions[i].first;
//         result.clipId = currMatch.captured(1);
//         result.iteration = currMatch.captured(2).toInt();

//         int jsonStart = currMatch.capturedEnd();
//         int jsonEnd = (i + 1 < matchPositions.size()) ? matchPositions[i + 1].second : fileContent.length();
//         QString jsonBlock = fileContent.mid(jsonStart, jsonEnd - jsonStart).trimmed();

//         // 1. Extract the inner text payload from the Gemini API envelope
//         QString payloadText = extractGeminiPayloadText(jsonBlock);

//         // 2. Strip the markdown fences (```json ... ```)
//         result.rawText = stripMarkdown(payloadText);

//         // 3. Parse the cleaned JSON text into a QJsonObject
//         QJsonParseError innerError;
//         QJsonDocument innerDoc = QJsonDocument::fromJson(result.rawText.toUtf8(), &innerError);

//         if (innerError.error == QJsonParseError::NoError && innerDoc.isObject())
//         {
//             result.parsedJson = innerDoc.object();
//             result.success = true;
//         } else {
//             result.success = false;
//             result.errorMessage = QStringLiteral("Failed to parse inner payload JSON: ") + innerError.errorString();
//         }

//         results.append(result);
//     }

//     return results;
// }

QString GovernanceAuditHarness::formatPlayersInvolved(const QJsonArray &playersArray)
{
    if (playersArray.isEmpty())
    {
        return QStringLiteral("None");
    }

    QStringList playerEntries;

    for (const QJsonValue &val : playersArray)
    {
        if (val.isObject())
        {
            QJsonObject pObj = val.toObject();
            QString team = pObj.value(QStringLiteral("team")).toString();
            QString name = pObj.value(QStringLiteral("name")).toString();

            QString numberStr;
            if (pObj.contains(QStringLiteral("number"))) {
                numberStr = QString::number(pObj.value(QStringLiteral("number")).toInt());
            } else if (pObj.contains(QStringLiteral("id"))) {
                numberStr = pObj.value(QStringLiteral("id")).toString().remove('#');
            }

            QString formatted;
            if (!team.isEmpty())
            {
                formatted += team;
            }

            if (!numberStr.isEmpty())
            {
                formatted += (formatted.isEmpty() ? QString() : " ") + QStringLiteral("(#%1)").arg(numberStr);
            }

            if (!name.isEmpty())
            {
                formatted += (formatted.isEmpty() ? name : QStringLiteral(" - %1").arg(name));
            }

            playerEntries.append(formatted.trimmed());

        }
        else if (val.isString())
        {
            QString raw = val.toString().trimmed();
            raw.remove('[').remove(']');
            playerEntries.append(raw);
        }
    }

    return playerEntries.isEmpty() ? QStringLiteral("None") : playerEntries.join(QStringLiteral(", "));
}
