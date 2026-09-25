#ifndef KEYWORD_SEARCH_ENGINE_H
#define KEYWORD_SEARCH_ENGINE_H

#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>

#include "inc/Defs.h"
#include "inc/VideoSegmentInfo.h"

class KeywordSearchEngine
{
    public:

        /**
         * @brief Filters video segments based on spoken keywords using JSON annotations or WebVTT fallback.
         * @param inputSegments The list of multimodal triples to process.
         * @param searchTerms The keywords to look for (e.g. "goal", "foul").
         * @param matchType Requires ANY or ALL terms to be matched.
         * @param minConfidenceThreshold Minimum average confidence (0.0 to 1.0) required for an utterance block to be searched.
         * @return Filtered list of VideoSegmentInfo.
         */
        static VideoSegmentInfo::VideoSegmentInfoList_t filterSegments(const VideoSegmentInfo::VideoSegmentInfoList_t& inputSegments,
                                                                       const QStringList& searchTerms,
                                                                       KeywordMatchType_t matchType = KeywordMatchType_t::Any,
                                                                       double minConfidenceThreshold = *Constants::Q_QGS_CONFIDENCE_THRESHOLD);

    private:

        struct UtteranceBlock {
            QString text;
            double confidence;
        };

        /**
         * @brief extractUtteranceBlocks
         * @param annotation
         * @return
         */
        static QList<UtteranceBlock> extractUtteranceBlocks(const QJsonObject& annotation);

        /**
         * @brief textMatchesKeywords
         * @param text
         * @param keywords
         * @param matchType
         * @return
         */
        static bool textMatchesKeywords(const QString& text, const QStringList& keywords, KeywordMatchType_t matchType);

        /**
         * @brief removeAccents
         * @param input
         * @return
         */
        static QString removeAccents(const QString& input);
};

#endif // KEYWORD_SEARCH_ENGINE_H
