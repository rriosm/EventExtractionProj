#ifndef EVENTSYNCHRONIZER_H
#define EVENTSYNCHRONIZER_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <QSet>
#include <QHash>
#include <QJsonArray>

namespace SoccerAnalytics
{
    struct GeminiEvent {
        QString clipId;
        QString eventId;
        QString category;
        QString type;
        double tVideo = 0.0;
        QString description;
        QJsonArray players;

        QJsonObject toJson() const;
    };

    struct GroundTruthAnnotation {
        qint32 gtId = -1;
        qint32 period = 0;
        QString gameTime;
        QString label;
        QString team;
        double positionMs = 0.0;
        double tVideoGt = 0.0;
        bool matched = false;

        QJsonObject toJson() const;
    };

    struct MatchedPair {
        GeminiEvent prediction;
        GroundTruthAnnotation groundTruth;
        double deltaSec = 0.0;

        QJsonObject toJson() const;
    };

    struct MappingMetrics {
        qint32 truePositives = 0;
        qint32 falsePositives = 0;
        qint32 falseNegatives = 0;
        double precision = 0.0;
        double recall = 0.0;
        double f1Score = 0.0;

        QJsonObject toJson() const;
    };

    struct MappingResult {
        MappingMetrics metrics;
        QVector<MatchedPair> matchedPairs;
        QVector<GeminiEvent> unmatchedPredictions;
        QVector<GroundTruthAnnotation> unmatchedGroundTruth;

        QJsonObject toJson() const;
    };

    class EventSynchronizer
    {
        public:

        /**
             * @brief EventSynchronizer
             */
            EventSynchronizer();

            /**
             * @brief parseTimestampStr
             * @param tsStr
             * @return
             */
            static double parseTimestampStr(const QString &tsStr);

            /**
             * @brief loadAndSynchronize
             * @param mdFilePath
             * @param manifestFilePath
             * @return
             */
            QVector<GeminiEvent> loadAndSynchronize(const QString &mdFilePath,
                                                    const QString &manifestFilePath) const;

            /**
             * @brief applyTemporalNms
             * @param events
             * @param nmsWindowSec
             * @return
             */
            QVector<GeminiEvent> applyTemporalNms(const QVector<GeminiEvent> &events,
                                                  double nmsWindowSec = 6.0) const;

            /**
             * @brief mapToGroundTruth
             * @param geminiEvents
             * @param labelsFilePath
             * @param period2VideoOffsetSec
             * @param toleranceSec
             * @return
             */
            MappingResult mapToGroundTruth(const QVector<GeminiEvent> &geminiEvents,
                                           const QString &labelsFilePath,
                                           double period2VideoOffsetSec = 2730.173,
                                           double toleranceSec = 10.0) const;

        private:

            void initOntology();

        private:

            QHash<QString, QStringList> m_ontologyMap;
            QSet<QString> m_ignoreCategories;
    };
} // namespace SoccerAnalytics

#endif // EVENTSYNCHRONIZER_H
