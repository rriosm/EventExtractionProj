#ifndef TEMPORALALIGNMENTHARNESS_H
#define TEMPORALALIGNMENTHARNESS_H

#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>
#include <QRegularExpression>
#include <QString>
#include <QTextStream>
#include <QVector>

// Represents a single row from CSV file
struct GroundTruthEvent {
    QString segmentId;          // Column 01: N times dilated Video segment ID
    QString globalTimestamp;    // Column 02: Global Timestamp (real timestamp)
    QString localTimestamp;     // Column 03: Local Timestamp (50 sec timestamp)
    QString eventDetails;       // Column 04: Event category + (Team, jersey number)
};

// Data structure specific to Time-Domain Alignment testing
struct TemporalPayload {
    QString segmentId;
    QString videoId;
    QString rawModelResponse;
    QVector<GroundTruthEvent> groundTruthEvents;
};

// Maps a specific video segment ID to all of its logged events
using GroundTruthMap = QMap<QString, QVector<GroundTruthEvent>>;

struct AlignmentResult {
    QString segmentId;
    QString videoId;
    QString eventId;
    qint32  modelRealSeconds = -1;
    qint32  groundTruthRealSeconds = -1;
    qint32  deltaSeconds = -1;
    bool    withinTolerance = false;
    bool    jsonFailed = false;             // True JSON syntax or schema failure
    bool    noEventsDetected = false;       // Valid JSON, but events array is legitimately empty
    bool    missingGroundTruth = false;     // Video ID not found in CSV dataset
    bool    timestampParseFailed = false;   // Model returned malformed timestamp string
    bool    computedVttAccuracy = false;
};

class TemporalAlignmentHarness
{
    public:

        /**
         * @brief evaluateClockDomainAlignment: Audits the MLLM's temporal alignment accuracy.
         * @param payloads
         * @param toleranceWindow
         * @return
         */
        [[nodiscard]] static QVector<AlignmentResult> evaluateClockDomainAlignment(const QVector<TemporalPayload> &payloads, qint32 toleranceWindow);

        /**
          * @brief generateAlignmentReport: Generates the performance table for Event Spotting Precision.
          * @param results
          * @param toleranceWindow
          * @param outputPath
          */
        static void generateAlignmentReport(const QVector<AlignmentResult> &results, qint32 toleranceWindow, const QString &outputPath = QString());

        /**
         * @brief loadGroundTruthData
         * @param csvFilePath
         * @return
         */
        [[nodiscard]] static GroundTruthMap loadGroundTruthData(const QString& csvFilePath);

        /**
         * @brief loadPayloadsFromFile: Loads the experimental dataset and attaches ground truth time mappings.
         * @param filePath
         * @param groundTruthMap
         * @return
         */
        [[nodiscard]] static QVector<TemporalPayload> loadPayloadsFromFile(const QString& filePath, const GroundTruthMap& groundTruthMap);

    private:

        /**
         * @brief parseTimestampToSeconds: Parses standard event timestamps (e.g., "[01:47-01:48]", "00:01:56") into total seconds.
         *                                 Grabs the first valid time signature to represent the event start boundary.
         * @param timestampStr
         * @return
         */
        [[nodiscard]] static double parseTimestampToSeconds(const QString &timestampStr);
};

#endif // TEMPORALALIGNMENTHARNESS_H
