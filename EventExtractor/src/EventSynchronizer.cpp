
#include "EventSynchronizer.h"

#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <algorithm>
#include <cmath>
#include <limits>

namespace SoccerAnalytics {

    // ============================================================================
    // Model JSON Serializers
    // ============================================================================

    QJsonObject GeminiEvent::toJson() const
    {
        QJsonObject obj;

        obj[QStringLiteral("clip_id")] = clipId;
        obj[QStringLiteral("event_id")] = eventId;
        obj[QStringLiteral("category")] = category;
        obj[QStringLiteral("type")] = type;
        obj[QStringLiteral("t_video")] = tVideo;
        obj[QStringLiteral("description")] = description;
        obj[QStringLiteral("players")] = players;

        return obj;
    }

    QJsonObject GroundTruthAnnotation::toJson() const
    {
        QJsonObject obj;

        obj[QStringLiteral("gt_id")] = gtId;
        obj[QStringLiteral("period")] = period;
        obj[QStringLiteral("gameTime")] = gameTime;
        obj[QStringLiteral("label")] = label;
        obj[QStringLiteral("team")] = team;
        obj[QStringLiteral("position_ms")] = positionMs;
        obj[QStringLiteral("t_video_gt")] = tVideoGt;
        obj[QStringLiteral("matched")] = matched;

        return obj;
    }

    QJsonObject MatchedPair::toJson() const
    {
        QJsonObject obj;

        obj[QStringLiteral("prediction")] = prediction.toJson();
        obj[QStringLiteral("ground_truth")] = groundTruth.toJson();
        obj[QStringLiteral("delta_sec")] = deltaSec;

        return obj;
    }

    QJsonObject MappingMetrics::toJson() const
    {
        QJsonObject obj;

        obj[QStringLiteral("True_Positives")] = truePositives;
        obj[QStringLiteral("False_Positives")] = falsePositives;
        obj[QStringLiteral("False_Negatives")] = falseNegatives;
        obj[QStringLiteral("Precision")] = precision;
        obj[QStringLiteral("Recall")] = recall;
        obj[QStringLiteral("F1_Score")] = f1Score;

        return obj;
    }

    QJsonObject MappingResult::toJson() const
    {
        QJsonObject root;
        root[QStringLiteral("metrics")] = metrics.toJson();

        QJsonArray matchedArr;

        for (const auto &pair : matchedPairs)
        {
            matchedArr.append(pair.toJson());
        }

        root[QStringLiteral("matched_pairs")] = matchedArr;

        QJsonArray unmatchedPredArr;

        for (const auto &pred : unmatchedPredictions)
        {
            unmatchedPredArr.append(pred.toJson());
        }
        root[QStringLiteral("unmatched_predictions")] = unmatchedPredArr;

        QJsonArray unmatchedGtArr;

        for (const auto &gt : unmatchedGroundTruth)
        {
            unmatchedGtArr.append(gt.toJson());
        }

        root[QStringLiteral("unmatched_ground_truth")] = unmatchedGtArr;

        return root;
    }

    // ============================================================================
    // EventSynchronizer Implementation
    // ============================================================================

    EventSynchronizer::EventSynchronizer()
    {
        this->initOntology();
    }

    void EventSynchronizer::initOntology()
    {
        this->m_ontologyMap = {
            {QStringLiteral("GOAL"),         {QStringLiteral("Goal")}},
            {QStringLiteral("HAT TRICK"),    {QStringLiteral("Goal")}},
            {QStringLiteral("KICK-OFF"),     {QStringLiteral("Kick-off")}},
            {QStringLiteral("KICKOFF"),      {QStringLiteral("Kick-off")}},
            {QStringLiteral("CORNER KICK"),  {QStringLiteral("Corner")}},
            {QStringLiteral("YELLOW CARD"),  {QStringLiteral("Yellow card")}},
            {QStringLiteral("FOUL"),         {QStringLiteral("Foul")}},
            {QStringLiteral("FREE KICK"),    {QStringLiteral("Direct free-kick"), QStringLiteral("Indirect free-kick")}},
            {QStringLiteral("THROW-IN"),     {QStringLiteral("Throw-in")}},
            {QStringLiteral("SUBSTITUTION"), {QStringLiteral("Substitution")}},
            {QStringLiteral("SHOT"),         {QStringLiteral("Shots on target"), QStringLiteral("Shots off target")}},
            {QStringLiteral("HEADER"),       {QStringLiteral("Clearance"), QStringLiteral("Shots on target"), QStringLiteral("Shots off target")}},
            {QStringLiteral("SAVE"),         {QStringLiteral("Clearance")}}
        };

        this->m_ignoreCategories = {
            QStringLiteral("BROADCASTER VISUALS"),
            QStringLiteral("MANAGER REACTION"),
            QStringLiteral("CELEBRATION MOMENT"),
            QStringLiteral("COMMENTARY"),
            QStringLiteral("PASS"),
            QStringLiteral("PLAYER BLOCK"),
            QStringLiteral("PLAYER SUCCESSFUL TACKLE"),
            QStringLiteral("TEAM HUDDLE"),
            QStringLiteral("INJURY"),
            QStringLiteral("INJURY TIMEOUT"),
            QStringLiteral("BALL OUT"),
            QStringLiteral("HIGH PASS"),
            QStringLiteral("REPLAY OF GOAL"),
            QStringLiteral("DRIVE"),
            QStringLiteral("REFEREE WHISTLE")
        };
    }

    double EventSynchronizer::parseTimestampStr(const QString &tsStr)
    {
        QString clean = tsStr.trimmed();

        // Strip enclosing brackets [ ... ]
        if (clean.startsWith(QLatin1Char('[')) && clean.endsWith(QLatin1Char(']')))
        {
            clean = clean.mid(1, clean.length() - 2).trimmed();
        }

        // Split intervals: e.g. "00:00:27.000-00:00:46.000" -> takes start
        if (clean.contains(QLatin1Char('-'))) {
            clean = clean.section(QLatin1Char('-'), 0, 0).trimmed();
        }

        const QStringList parts = clean.split(QLatin1Char(':'));

        if (parts.size() == 3)
        {
            bool okH = false, okM = false, okS = false;
            const qint32 h = parts[0].toInt(&okH);
            const qint32 m = parts[1].toInt(&okM);
            const double s = parts[2].toDouble(&okS);

            if (okH && okM && okS)
            {
                return (h * 3600.0) + (m * 60.0) + s;
            }

        } else if (parts.size() == 2)
        {
            bool okM = false, okS = false;
            const int m = parts[0].toInt(&okM);
            const double s = parts[1].toDouble(&okS);

            if (okM && okS)
            {
                return (m * 60.0) + s;
            }
        }

        return clean.toDouble();
    }

    QVector<GeminiEvent> EventSynchronizer::loadAndSynchronize(const QString &mdFilePath,
                                                               const QString &manifestFilePath) const
    {
        // 1. Parse Manifest JSON
        QFile manifestFile(manifestFilePath);

        if (!manifestFile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            return {};
        }

        const QByteArray manifestBytes = manifestFile.readAll();
        manifestFile.close();

        const QJsonDocument manifestDoc = QJsonDocument::fromJson(manifestBytes);
        QHash<QString, double> segmentStartTimes;

        const QJsonArray segments = manifestDoc.object().value(QStringLiteral("segments")).toArray();

        for (const QJsonValue &segVal : segments)
        {
            const QJsonObject segObj = segVal.toObject();
            const QString segId = segObj.value(QStringLiteral("segment_id")).toString();
            const double startTime = segObj.value(QStringLiteral("start_time")).toDouble(0.0);
            segmentStartTimes.insert(segId, startTime);
        }

        // 2. Read Markdown content
        QFile mdFile(mdFilePath);

        if (!mdFile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            return {};
        }

        QTextStream in(&mdFile);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        in.setCodec("UTF-8");
#endif
        const QString mdText = in.readAll();
        mdFile.close();

        // 3. Extract JSON payload blocks inside ```json ... ``` code blocks
        static const QRegularExpression jsonBlockRegex(
            QStringLiteral("```json\\s*(\\{[\\s\\S]*?\\})\\s*```")
            );

        QVector<GeminiEvent> geminiEvents;
        QRegularExpressionMatchIterator it = jsonBlockRegex.globalMatch(mdText);

        while (it.hasNext())
        {
            const QRegularExpressionMatch match = it.next();
            const QString jsonStr = match.captured(1);

            QJsonParseError parseError;
            const QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &parseError);

            if (parseError.error != QJsonParseError::NoError || !doc.isObject())
            {
                continue;
            }

            const QJsonObject rootObj = doc.object();
            const QJsonObject clipInfo = rootObj.value(QStringLiteral("video_clip_info")).toObject();
            const QString clipId = clipInfo.value(QStringLiteral("clip_video_id")).toString();

            const double segStartSec = segmentStartTimes.value(clipId, 0.0);
            const QJsonArray eventsArr = rootObj.value(QStringLiteral("events")).toArray();

            for (const QJsonValue &evVal : eventsArr)
            {
                const QJsonObject evObj = evVal.toObject();
                const QString category = evObj.value(QStringLiteral("category")).toString().trimmed().toUpper();

                if (this->m_ignoreCategories.contains(category))
                {
                    continue;
                }

                QString tsRealStr = evObj.value(QStringLiteral("timestamp_real")).toString();

                if (tsRealStr.isEmpty() || tsRealStr == QLatin1String("N/A"))
                {
                    tsRealStr = evObj.value(QStringLiteral("timestamp_video")).toString();
                }

                const double tReal = parseTimestampStr(tsRealStr);
                const double tVideo = segStartSec + tReal;

                GeminiEvent event;
                event.clipId = clipId;
                event.eventId = evObj.value(QStringLiteral("id")).toString();
                event.category = category;
                event.type = evObj.value(QStringLiteral("type")).toString();
                event.tVideo = tVideo;
                event.description = evObj.value(QStringLiteral("description")).toString();
                event.players = evObj.value(QStringLiteral("players")).toArray();

                geminiEvents.append(event);
            }
        }

        // 4. Sort chronologically by continuous match timeline
        std::sort(geminiEvents.begin(), geminiEvents.end(), [](const GeminiEvent &a, const GeminiEvent &b) {
            return a.tVideo < b.tVideo;
        });

        return geminiEvents;
    }

    QVector<GeminiEvent> EventSynchronizer::applyTemporalNms(const QVector<GeminiEvent> &events,
                                                             double nmsWindowSec) const
    {
        QVector<GeminiEvent> filtered;
        filtered.reserve(events.size());

        for (const auto &ev : events)
        {
            bool duplicate = false;

            for (const auto &kept : filtered)
            {
                if (ev.category == kept.category &&
                    std::abs(ev.tVideo - kept.tVideo) < nmsWindowSec)
                {
                    duplicate = true;
                    break;
                }
            }

            if (!duplicate)
            {
                filtered.append(ev);
            }
        }

        return filtered;
    }

    MappingResult EventSynchronizer::mapToGroundTruth(const QVector<GeminiEvent> &geminiEvents,
                                                      const QString &labelsFilePath,
                                                      double period2VideoOffsetSec,
                                                      double toleranceSec) const {
        MappingResult result;

        // 1. Read Ground Truth Labels
        QFile labelsFile(labelsFilePath);

        if (!labelsFile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            return result;
        }

        const QByteArray labelsBytes = labelsFile.readAll();
        labelsFile.close();

        const QJsonDocument labelsDoc = QJsonDocument::fromJson(labelsBytes);
        const QJsonArray annotations = labelsDoc.object().value(QStringLiteral("annotations")).toArray();

        QVector<GroundTruthAnnotation> parsedGt;
        parsedGt.reserve(annotations.size());

        qint32 idx = 0;

        for (const QJsonValue &val : annotations)
        {
            const QJsonObject gtObj = val.toObject();
            const QString gameTimeStr = gtObj.value(QStringLiteral("gameTime")).toString();

            const QStringList parts = gameTimeStr.split(QStringLiteral(" - "));
            const qint32 period = (parts.size() >= 1) ? parts[0].toInt() : 1;

            const double posMs = gtObj.value(QStringLiteral("position")).toString().toDouble();
            const double posSec = posMs / 1000.0;

            const double tVideoGt = (period == 1) ? posSec : (period2VideoOffsetSec + posSec);

            GroundTruthAnnotation ann;
            ann.gtId = idx++;
            ann.period = period;
            ann.gameTime = gameTimeStr;
            ann.label = gtObj.value(QStringLiteral("label")).toString();
            ann.team = gtObj.value(QStringLiteral("team")).toString();
            ann.positionMs = posMs;
            ann.tVideoGt = tVideoGt;
            ann.matched = false;

            parsedGt.append(ann);
        }

        // 2. Greedy bipartite matching
        for (const auto &pred : geminiEvents)
        {
            const QStringList validTargets = this->m_ontologyMap.value(pred.category);

            if (validTargets.isEmpty())
            {
                result.unmatchedPredictions.append(pred);
                continue;
            }

            qint32 bestGtIdx = -1;
            double bestDiff = std::numeric_limits<double>::infinity();

            for (qint32 i = 0; i < parsedGt.size(); ++i)
            {
                if (!parsedGt[i].matched && validTargets.contains(parsedGt[i].label))
                {
                    const double diff = std::abs(pred.tVideo - parsedGt[i].tVideoGt);

                    if (diff <= toleranceSec && diff < bestDiff)
                    {
                        bestDiff = diff;
                        bestGtIdx = i;
                    }
                }
            }

            if (bestGtIdx != -1) {
                parsedGt[bestGtIdx].matched = true;

                MatchedPair pair;
                pair.prediction = pred;
                pair.groundTruth = parsedGt[bestGtIdx];
                pair.deltaSec = std::round(bestDiff * 1000.0) / 1000.0;
                result.matchedPairs.append(pair);

            } else {
                result.unmatchedPredictions.append(pred);
            }
        }

        // 3. Identify unmatched ground-truth records
        for (const auto &gt : qAsConst(parsedGt))
        {
            if (!gt.matched)
            {
                result.unmatchedGroundTruth.append(gt);
            }
        }

        // 4. Calculate Precision, Recall, and F1
        const qint32 tp = result.matchedPairs.size();
        const qint32 fp = result.unmatchedPredictions.size();
        const qint32 fn = result.unmatchedGroundTruth.size();

        const double precision = (tp + fp > 0) ? (static_cast<double>(tp) / (tp + fp)) : 0.0;
        const double recall = (tp + fn > 0) ? (static_cast<double>(tp) / (tp + fn)) : 0.0;
        const double f1 = (precision + recall > 0.0) ? (2.0 * precision * recall / (precision + recall)) : 0.0;

        result.metrics.truePositives = tp;
        result.metrics.falsePositives = fp;
        result.metrics.falseNegatives = fn;
        result.metrics.precision = std::round(precision * 10000.0) / 10000.0;
        result.metrics.recall = std::round(recall * 10000.0) / 10000.0;
        result.metrics.f1Score = std::round(f1 * 10000.0) / 10000.0;

        return result;
    }
}
