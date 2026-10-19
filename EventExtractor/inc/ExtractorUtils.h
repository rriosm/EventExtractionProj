#ifndef EXTRACTORUTILS_H
#define EXTRACTORUTILS_H

#include <QCryptographicHash>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QThread>

#include "inc/CommandLineDefs.h"
#include "inc/Defs.h"
#include "inc/VideoModelConfig.h"
#include "inc/VideoSegmentInfo.h"

inline void registerThreadCleanup(QThread* thread, QObject* worker)
{
    QObject::connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);
}

class ExtractorUtils
{
        /**
         * @brief classVersion
         */
        static const quint16 classVersion = quint16(101);

    public:

        /**
         * @brief bestVideoEncoder
         * @return
         */
        [[nodiscard]] static QString bestVideoEncoder();

        /**
         * @brief cleanupPartialAudioFiles
         * @param dir
         * @param pattern
         */
        static void cleanupPartialAudioFiles(const QString &dir, const QString &pattern);

        /**
         * @brief cleanupVideoPartialFiles
         * @param dir
         * @param pattern
         * @param modelCode
         */
        static void cleanupVideoPartialFiles(const QString &dir,
                                        const QString &pattern,
                                        const ModelCode_t modelCode);

        /**
         * @brief createGapEntry: Helper to create a consistent GAP metadata entry
         * @param index
         * @param nominalDuration
         * @return
         */
        [[nodiscard]] static VideoSegmentInfo createGapEntry(qint32 index, double nominalDuration);

        /**
         * @brief getConfig: Static helper to get the specific config for a Gemini model code.
         * @param modelCode
         * @return
         */
        [[nodiscard]] static VideoModelConfig getConfig(ModelCode_t modelCode);

        /**
         * @brief formatTime: Helper to format milliseconds into HH:mm:ss
         * @param ms
         * @return
         */
        [[nodiscard]] static QString formatTime(qint64 ms);

        /**
         * @brief hasSufficientDiskSpace
         * @param outDirPath
         * @param inputFilePath
         * @return
         */
        [[nodiscard]] static bool hasSufficientDiskSpace(const QString &outDirPath, const QString &inputFilePath);

        /**
         * @brief isDirWritable
         * @param path
         * @return
         */
        [[nodiscard]] static bool isDirWritable(const QString &path);

        /**
         * @brief checkIsValidFile
         * @param filePath
         * @return
         */
        [[nodiscard]] static bool checkIsValidFile(const QString &filePath);

        /**
         * @brief isAudioSegmentValid
         * @param filePath
         * @return
         */
        [[nodiscard]] static bool isAudioSegmentValid(const QString &filePath);

        /**
         * @brief isVideoSegmentValid: Checks if a segment is a valid, playable MP4.
         * @param filePath
         * @return
         */
        [[nodiscard]] static bool isVideoSegmentValid(const QString &filePath);

        /**
         * @brief ffmpegStateToString
         * @param state
         * @return
         */
        [[nodiscard]] static QString ffmpegStateToString(FFmpegState_t state);

        /**
         * @brief ffmpegProcessStateToString
         * @param state
         * @return
         */
        [[nodiscard]] static QString ffmpegProcessStateToString(QProcess::ProcessState state);

        /**
         * @brief performIdentitySanityCheck
         * @param results
         */
        static void performIdentitySanityCheck(const VideoSegmentInfo::VideoSegmentInfoList_t &results);

        /**
         * @brief generateContentSeed
         * @param contentData
         * @param algorithm
         * @return
         */
        [[nodiscard]] static QByteArray generateContentSeed(const QString &contentData, QCryptographicHash::Algorithm algorithm);

        /**
         * @brief optimalThreadCount
         * @return
         */
        [[nodiscard]] static qint32 optimalThreadCount();

        /**
         * @brief limit_to_cores
         * @param num_cores
         */
        static void limit_to_cores(qint32 num_cores);

        /**
         * @brief commandLineParser
         * @param args
         * @return
         */
        // static CmdArgumentsVec commandLineParser(const QStringList &args);

        /**
         * @brief commandLineParser
         * @param app
         * @return
         */
        [[nodiscard]] static CmdArgumentsVec commandLineParser(const QStringList &args);

        /**
         * @brief initCmdOptions
         * @param cmdArgsVec
         * @param cmdOptions
         * @return
         */
        [[nodiscard]] static qint32 initCmdOptions(CmdArgumentsVec &cmdArgsVec, CmdLineOptions * cmdOptions);

        /**
         * @brief findCmdOption
         * @param flag
         * @param cmdArgVec
         * @return
         */
        [[nodiscard]] static qint32 indexOfCmdArg(const CmdFlags_t opt, const CmdArgumentsVec & cmdArgsVec);

        /**
         * @brief isJsonFile
         * @param filePath
         * @return
         */
        [[nodiscard]] bool isJsonFile(const QString &filePath);

        /**
         * @brief repairJson: Repairs JSON strings truncated by API MAX_TOKENS limits.
         *                    Analyzes the open structural tokens and seals the JSON safely to permit
         *                    deserialization.
         * @param str
         * @return
         */
        [[nodiscard]] static QString repairJson(QString str);

        /**
         * @brief stripMarkdown
         * @param trimmedText
         * @return
         */
        [[nodiscard]] static QString stripMarkdown(const QString &trimmedText);

        /**
         * @brief toSeconds
         * @param timestamp
         * @return
         */
        [[nodiscard]] static double toSeconds(const QString& timestamp);

        /**
         * @brief getAPIKey
         * @param apiKey
         * @return
         */
        [[nodiscard]] static QString getAPIKey(const AIModels_t apiKey);

        /**
         * @brief getModelTypeFromString
         * @param val
         * @param AIModelMap
         * @return
         */
        [[nodiscard]] static AIModels_t getModelTypeFromString(const QString &val, const QHash<AIModels_t, AIModelConfig> &modelMap = getAIModelMap());

        /**
         * @brief headerTemplate
         * @param headerType
         * @return
         */
        [[nodiscard]] static QString headerTemplate(HeaderType_t headerType);

        /**
         * @brief authHeader
         * @param apiKey
         * @return
         */
        [[nodiscard]] static QString authHeader(const QString &apiKey, AIModels_t modelCode);

        /**
         * @brief Parses consecutive Gemini API JSON responses from a file, extracts
         * the embedded match analysis JSON blocks, and outputs a clean master JSON array.
         * @param filePath Path to the input file (e.g., .md or .json text file)
         */
        static void presentMdAsJsonFriendly(const QString &inputMDFilePath);

        /**
         * @brief extractGeminiPayloadText
         * @param rawText
         * @return
         */
        [[nodiscard]] static QString extractGeminiPayloadText(const QString &rawText);

        /**
         * @brief Parses consecutive Gemini API JSON responses from a file, extracts
         * the embedded match analysis JSON blocks, and outputs a clean master JSON array.
         * @param filePath Path to the input file (e.g., .md or .json text file)
         * @param outputFilePath Path to the output file (e.g., .json text file)
         */
        static void parseCompleteGeminiMarkdownAnswer(const QString &filePath, const QString &outputFilePath);

        /**
         * @brief Merges the Gemini AI analysis reports from a markdown file into a master sync manifest JSON file.
         * @param manifestJsonPath Path to the main 'synchronized_manifests-22h03min.json' file.
         * @param geminiMdPath Path to the 'Google_Gemini_API_25Flash-20260624-22h03-00h15min.md' file.
         * @param outputPath Path where the merged output JSON file will be saved.
         */
        static void mergeGeminiReportsToManifest(const QString &manifestJsonPath,
                                                 const QString &geminiMdPath,
                                                 const QString &outputPath);
};

#endif // EXTRACTORUTILS_H
