#ifndef GOVERNANCEAUDITHARNESS_H
#define GOVERNANCEAUDITHARNESS_H

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>
#include <QPair>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QVector>

struct AuditMetrics {
    qint32 totalEvaluatedRuns                   = 0;
    qint32 markdownOrProseDrift                 = 0; // Detects conversational prefixes/wrappers
    qint32 jsonParsingFailures                  = 0; // Severe deserialization crashes

    qint32 structuralKeyErrorsRoot              = 0;
    qint32 structuralKeyErrorsClipInfo          = 0;
    qint32 structuralKeyErrorsVTTVerification   = 0;
    qint32 structuralKeyErrorsEvents            = 0;

    /*
     * Missing mandatory governance fields!
     * totalStructuralKeyErrors tracks schema contract violations, specifically counting how many times
     * the model's JSON response fails to include mandatory top-level structural keys.
     */
    qint32 totalStructuralKeyErrors             = 0;

    qint32 unstructuredDesyncNotes              = 0;
    qint32 unstructuredEventsKey                = 0;

    // VTT Verification & Extraction Totals
    /*
     * structuredVttEventsRejections = vtt_verification.rejected_events + video_clip_info.vtt_events_rejected (if vtt_verification.rejected_events
     * is omitted).
     */
    qint32 structuredVttEventsRejections        = 0;
    qint32 totalConfirmedVttEvents              = 0; // Total confirmed VTT events
    qint32 totalVideoOnlyEvents                 = 0; // Total video-only events

    /*
     * The field 'historicalHallucinations' is used to detect Contextual / Historical Hallucinations.
     * Model accepted a historical text cue without visual verification
     */
    qint32 historicalHallucinations             = 0;
};

struct Payload {
    QString segId;
    qint32 iteration;
    QString rawModelResponse;

    /*
     *  Ground truth check for active visual frames. An external ground-truth oracle indicating whether
     *  a real match event or infraction physically occurred in the video footage (true) or not (false).
     *  */
    bool hasVisualGroundTruthEvent;
};

/*
 * Payload.visualRealityHasInfraction serves as an external ground-truth oracle indicating whether a
 * real match event or infraction physically occurred in the video footage (true) or not (false).
 *
 * Purpose in auditModelOutputs: In Step 8 of auditModelOutputs, it is used to detect Contextual / Historical Hallucinations
 *
 * What It Detects
 *      - Text-Induced Visual Hallucination: Broadcasters and commentators frequently discuss past events,
 *      previous bookings, or historical goals (e.g., "He's on nine yellow cards this season..." or "Remember
 *      that foul against Liverpool...").
 *      - Model Failure Mode: An ungrounded or poorly governed multimodal model often reads these historical
 *      commentary cues in the VTT transcript and incorrectly outputs vtt_status = "confirmed", claiming that
 *      a goal, foul, or card is actively happening in the current video frames.
 *      - Audit Detection:
 *          If the model claims an event is confirmed (visually present and matching commentary),
 *          BUT payload.visualRealityHasInfraction is false (ground truth confirms nothing happened on screen),
 *          The harness flags a hallucination and increments metrics.historicalHallucinations.
 */

struct ClipExtractionResult {
    QString clipId;
    qint32 iteration = 1;
    bool success = false;
    QString rawText;
    QJsonObject parsedJson;
    QString errorMessage;
};

struct MatchEvent {
    QString id;
    QString timestamp;
    QString category;
    QString description;
    QString playersInvolved;
    QString verdict;
};

class GovernanceAuditHarness
{
    public:

        /**
         * @brief auditGovernedOutputs: Audits a specific test payload group to determine enterprise compliance.
         * @param payloads
         * @return
         */
        [[nodiscard]] static AuditMetrics auditGovernedOutputs(const QVector<Payload> &payloads);

        /**
         * @brief auditBaselineOutputs
         * @param payloads
         * @return
         */
        [[nodiscard]] static AuditMetrics auditBaselineOutputs(const QVector<Payload> &payloads);

        /**
         * @brief generateAuditReport: Formulates and saves a structured evaluation report to disk.
         * @param control
         * @param governed
         * @param outputPath
         */
        static void generateAuditReport(const AuditMetrics &control, const AuditMetrics &governed, const QString &outputPath = QString());

        /**
         * @brief generateEventsMarkdownTableFromGoverned Extracts the events list from parsed clip JSON and formats
         *                                    it into a Markdown table.
         * @param payload
         * @return
         */
        [[nodiscard]] static QString generateEventsMarkdownTableFromGoverned(const Payload &payload);

        /**
         * @brief generateEventsMarkdownTableFromGoverned Extracts the events list from parsed clip JSON and formats
         *                                    it into a Markdown table.
         * @param payloads
         * @param targetSegId
         * @return
         */
        [[nodiscard]] static QString generateEventsMarkdownTableFromGoverned(const QVector<Payload> &payloads, const QString &targetSegId = QString());

        /**
         * @brief generateRejectedVttEventsTableFromGoverned Parses a single Payload and builds a Markdown table of VTT events that were rejected
         *                                       (present in audio/transcript but not visually confirmed in the video).
         * @param payload
         * @return
         */
        [[nodiscard]] static QString generateRejectedVttEventsTableFromGoverned(const Payload &payload);

        /**
         * @brief generateRejectedVttEventsTableFromGoverned Batch generates the rejected VTT events table across a list of payloads.
         * @param payloads
         * @param targetSegId If specified, only generates for that specific segment ID.
         * @return
         */
        [[nodiscard]] static QString generateRejectedVttEventsTableFromGoverned(const QVector<Payload> &payloads, const QString &targetSegId = QString());

        /**
         * @brief printReport
         * @param payloads
         */
        [[nodiscard]]   static QString printReport(const QVector<Payload> &payloads);

        /**
         * @brief loadPayloadsFromFile: Robustly slices a multi-response Gemini API markdown/json export into
         *                              clean individual clip blocks.
         * @param filePath
         * @param visualRealityFlag
         * @return
         */
        [[nodiscard]] static QVector<Payload> loadPayloadsFromFile(const QString& filePath, bool visualRealityFlag);

        // /**
        //  * @brief parseMultiClipFile
        //  * @param fileContent
        //  * @return
        //  */
        // [[nodiscard]] static QVector<ClipExtractionResult> parseMultiClipFile(const QString &fileContent);

    private:

        /**
         * @brief formatPlayersInvolved Formats the 'players' JSON array into a clean, human-readable string.
         *                              Handles both structured JSON objects and raw string variations in the
         *                              dataset.
         * @param playersArray
         * @return
         */
        [[nodiscard]] static QString formatPlayersInvolved(const QJsonArray &playersArray);
};

#endif // GOVERNANCEAUDITHARNESS_H
