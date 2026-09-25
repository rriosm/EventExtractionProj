
#include "inc/KeywordSearchEngine.h"
#include "inc/Audio.h"

VideoSegmentInfo::VideoSegmentInfoList_t KeywordSearchEngine::filterSegments(const VideoSegmentInfo::VideoSegmentInfoList_t& inputSegments,
                                                                             const QStringList& searchTerms,
                                                                             KeywordMatchType_t matchType,
                                                                             double minConfidenceThreshold)
{
    VideoSegmentInfo::VideoSegmentInfoList_t filteredResults;

    for (const VideoSegmentInfo& seg : inputSegments)
    {
        Audio audio = seg.audioTrack();
        QJsonObject annotation = audio.annotation();
        QString vtt = audio.transcription();

        bool useVttFallback = true;
        bool keywordMatched = false;

        // 1. Primary Source: Process Deepgram JSON Annotation
        if (!annotation.isEmpty() &&
            (annotation.contains(QStringLiteral("results")) ||
                                      annotation.contains(QStringLiteral("words")) ))
        {
            QList<UtteranceBlock> utterances = extractUtteranceBlocks(annotation);

            if (!utterances.isEmpty())
            {
                QStringList validUtteranceTexts;

                // Evaluate gates: filter out blocks below the confidence threshold
                for (const UtteranceBlock& block : qAsConst(utterances))
                {
                    if (block.confidence >= minConfidenceThreshold)
                    {
                        validUtteranceTexts.append(block.text);
                    }
                }

                // If any blocks survived the confidence gating, process the search
                if (!validUtteranceTexts.isEmpty())
                {
                    useVttFallback = false; // Successfully used the JSON, disengage fallback
                    QString combinedValidText = validUtteranceTexts.join(QLatin1Char(' '));
                    keywordMatched = textMatchesKeywords(combinedValidText, searchTerms, matchType);
                }

                // Else: The annotation existed but every utterance was suppressed (useVttFallback remains true)
            }
        }

        // 2. Secondary Source: Fallback to WebVTT
        // Triggered if annotation is completely empty or if all utterances were gated out by poor audio quality.
        if (useVttFallback && !vtt.isEmpty())
        {
            keywordMatched = textMatchesKeywords(vtt, searchTerms, matchType);
        }

        // 3. Keep the triple if a match occurred
        if (keywordMatched)
        {
            filteredResults.append(seg);
        }
    }

    return filteredResults;
}

QList<KeywordSearchEngine::UtteranceBlock> KeywordSearchEngine::extractUtteranceBlocks(const QJsonObject& annotation)
{
    QList<UtteranceBlock> blocks;
    QJsonArray words;

    if (annotation.contains(QStringLiteral("words")))
    {
        words = annotation.value(QStringLiteral("words")).toArray();
    }
    // Check if the Deepgram JSON already provides natively structured "utterances"
    else if (annotation.contains(QStringLiteral("results")))
    {
        QJsonObject resultsObj = annotation.value(QStringLiteral("results")).toObject();

        if (resultsObj.contains(QStringLiteral("utterances")))
        {
            QJsonArray utts = resultsObj.value(QStringLiteral("utterances")).toArray();
            for (const QJsonValue& v : qAsConst(utts))
            {
                QJsonObject u = v.toObject();
                blocks.append({
                    u.value(QStringLiteral("transcript")).toString(),
                    u.value(QStringLiteral("confidence")).toDouble(0.0)
                });
            }

            return blocks;
        }

        // Otherwise, manually process the flattened 'words' array into blocks
        QJsonArray channels = resultsObj.value(QStringLiteral("channels")).toArray();

        if (!channels.isEmpty())
        {
            QJsonArray alts = channels.first().toObject().value(QStringLiteral("alternatives")).toArray();

            if (!alts.isEmpty())
            {
                words = alts.first().toObject().value(QStringLiteral("words")).toArray();
            }
        }
    }

    if (words.isEmpty()) return blocks;

    QStringList currentWords;
    double totalConfidence = 0.0;
    int wordCount = 0;

    for (const QJsonValue& wv : qAsConst(words))
    {
        QJsonObject wObj = wv.toObject();

        // Grab punctuated word if available, fallback to raw word
        QString word = wObj.value(QStringLiteral("punctuated_word")).toString();

        if (word.isEmpty())
        {
            word = wObj.value(QStringLiteral("word")).toString();
        }

        currentWords.append(word);
        totalConfidence += wObj.value(QStringLiteral("confidence")).toDouble(0.0);
        wordCount++;

        // Simple sentence boundary parsing to group into utterances
        if (word.endsWith(QLatin1Char('.')) ||
            word.endsWith(QLatin1Char('?')) ||
            word.endsWith(QLatin1Char('!')))
        {
            blocks.append({currentWords.join(QLatin1Char(' ')), totalConfidence / wordCount});
            currentWords.clear();
            totalConfidence = 0.0;
            wordCount = 0;
        }
    }

    // Append any leftover trailing words as the final block
    if (wordCount > 0)
    {
        blocks.append({currentWords.join(QLatin1Char(' ')), totalConfidence / wordCount});
    }

    return blocks;
}

bool KeywordSearchEngine::textMatchesKeywords(const QString& text, const QStringList& keywords, KeywordMatchType_t matchType)
{
    if (keywords.isEmpty() || text.isEmpty()) return false;

    QString cleanText = KeywordSearchEngine::removeAccents(text).toLower();

    for (const QString& kw : keywords)
    {
        QString cleanKw = KeywordSearchEngine::removeAccents(kw).toLower();

        // Use regex for clean word-boundary matching, preventing partial-word false positives.
        // e.g. searching "goal" won't mistakenly flag "goalkeeper".
        QRegularExpression re(QStringLiteral("\\b") + QRegularExpression::escape(cleanKw) + QStringLiteral("\\b"),
                              QRegularExpression::CaseInsensitiveOption | QRegularExpression::UseUnicodePropertiesOption);

        bool containsTerm = cleanText.contains(re);

        if (matchType == KeywordMatchType_t::Any && containsTerm)
        {
            return true;
        }

        if (matchType == KeywordMatchType_t::All && !containsTerm)
        {
            return false;
        }
    }

    // If we reach here, we evaluated the entire array.
    // If it's `All`, no checks failed, so return true.
    // If it's `Any`, no checks succeeded, so return false.
    return (matchType == KeywordMatchType_t::All);
}

QString KeywordSearchEngine::removeAccents(const QString& input)
{
    // Decompose characters into base letters and diacritic marks
    QString normalized = input.normalized(QString::NormalizationForm_KD);
    QString result;

    for (const QChar& ch : qAsConst(normalized))
    {
        // Only append characters that are NOT diacritic marks
        if (ch.category() != QChar::Mark_NonSpacing)
        {
            result.append(ch);
        }
    }

    return result;
}
