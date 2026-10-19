
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCryptographicHash>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonParseError>
#include <QMap>
#include <QStorageInfo>
#include <QThread>

#include "UniqueId.h"
using namespace utils;

#include "inc/ExtractorUtils.h"
#include "inc/LogUtils.h"

// Qt 6 headers
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    #include <QStringDecoder>
    #include <QStringConverter>
#else
// Qt 5 headers
    #include <QTextCodec>
#endif

#include "inc/ffmpeg_gen.h"

#if defined(Q_OS_LINUX)
#include <pthread.h>
#include <sched.h>
#elif defined(Q_OS_WIN)
#include <windows.h>
#endif

QString ExtractorUtils::bestVideoEncoder()
{
#ifdef Q_OS_MACOS
    // Q_PROCESSOR_ARM is defined by Qt when compiling for Apple Silicon (M1/M2/M3)
#if defined(Q_PROCESSOR_ARM)
        return "h264_videotoolbox";
#else
    // Intel Macs also support VideoToolbox, but libx264 is often
    // higher quality on older Intel chips. However, for speed:
    return "h264_videotoolbox";
#endif
#else
    // Default for Linux/Windows
    return "libx264";
#endif
}

void ExtractorUtils::cleanupPartialAudioFiles(const QString &dir, const QString &pattern)
{
    if (dir.isEmpty() || pattern.isEmpty()) return;

    QDir directory(dir);

    // Find all files matching the audio pattern (e.g., "segment_*.mp3")
    QStringList files = directory.entryList({pattern}, QDir::Files);

    for (const QString &fileName : qAsConst(files))
    {
        QString fullPath = directory.absoluteFilePath(fileName);
        QFileInfo info(fullPath);

        // Logic: If we aborted, any file with 0 size or very small size
        // is likely a partial/corrupt segment.
        // 1KB is a reasonable threshold for an empty/broken MP3 wrapper.
        if (info.size() < 1024)
        {
            if ( QFile::remove(fullPath) )
            {
                QString errMsg = QString("Cleaned up partial/empty audio segment: %1").arg(fileName);

#ifdef QT_DEBUG
                LogUtils::safeLog(errMsg,
                                  __FILE__,
                                  __LINE__,
                                  Q_FUNC_INFO);
#endif
            }

            continue;
        }

        // IMPORTANT: Only delete if the MP3 is actually broken/corrupt.
        // This preserves all previously completed segments.
        if ( ! ExtractorUtils::isAudioSegmentValid(fullPath) )
        {
            if ( QFile::remove(fullPath) )
            {
                QString errMsg = QString("Removing only the corrupted/partial audio segment: %1").arg(fileName);

#ifdef QT_DEBUG
                LogUtils::safeLog(errMsg,
                                  __FILE__,
                                  __LINE__,
                                  Q_FUNC_INFO);
#endif
            }
        }
    }
}

void ExtractorUtils::cleanupVideoPartialFiles(const QString &dir, const QString &pattern, const ModelCode_t modelCode)
{
    if (dir.isEmpty()) return;

    VideoModelConfig cfg = ExtractorUtils::getConfig(modelCode);

    if ( ! cfg.isValid() ) return;

    QDir directory(dir);

    // Find all files matching the current model's suffix pattern
    QString filter = QString(pattern).arg(cfg.suffix());
    QStringList files = directory.entryList({filter}, QDir::Files);

    for (const QString &fileName : qAsConst(files))
    {
        QString fullPath = directory.absoluteFilePath(fileName);
        QFileInfo info(fullPath);

        // Logic: If we aborted, any file with 0 size or very small size
        // is likely a partial/corrupt segment.
        // Alternatively, you can be aggressive and delete ALL files in the
        // output dir if you want a "clean slate" on failure.
        if (info.size() < 1024) // Less than 1KB
        {
            if ( QFile::remove(fullPath) )
            {
                QString errMsg = QString("Cleaned up partial/empty segment:%1").arg(fileName);

#ifdef QT_DEBUG
                LogUtils::safeLog(errMsg,
                                  __FILE__,
                                  __LINE__,
                                  Q_FUNC_INFO);
#endif
            }

            continue;
        }

        // 2. IMPORTANT: Only delete if the MP4 is actually broken/corrupt.
        // This preserves all previously completed segments.
        if ( ! ExtractorUtils::isVideoSegmentValid(fullPath) )
        {
            if ( QFile::remove(fullPath) )
            {
                QString errMsg = QString("Removing only the corrupted/partial segment: %1").arg(fileName);

#ifdef QT_DEBUG
                LogUtils::safeLog(errMsg,
                                  __FILE__,
                                  __LINE__,
                                  Q_FUNC_INFO);
#endif
            }
        }
    }
}

VideoSegmentInfo ExtractorUtils::createGapEntry(qint32 index, double nominalDuration)
{
    VideoSegmentInfo gap;
    // Format ID to 3 digits to match filename convention (001, 002...)
    gap.setId(UniqueId());
    gap.setState("GAP");
    gap.setStartTime(index * nominalDuration);
    gap.setTiming(index * nominalDuration, index * nominalDuration);
    gap.setFilePath(QString());
    return gap;
}

VideoModelConfig ExtractorUtils::getConfig(ModelCode_t modelCode)
{
    // Local static is only initialized once
    static const QMap<ModelCode_t, VideoModelConfig> configs = {
        {ModelCode_t::Gemini20_FlashLite2fps23crf, {"_gemini20_flashlite_2fps23crf", 2, 23}},
        {ModelCode_t::Gemini20_FlashLite001_2fps23crf, {"_gemini20_flashlite001_2fps23crf", 2, 23}},
        {ModelCode_t::Gemini20_Flash5fps21crf, {"_gemini20_flash_5fps21crf", 5, 21}},
        {ModelCode_t::Gemini20_Flash001_5fps21crf, {"_gemini20_flash001_5fps21crf", 5, 21}},
        {ModelCode_t::Gemini25_FlashLite2fps23crf, {"_gemini25_flashlite2fps23crf", 2, 23}},
        {ModelCode_t::Gemini25_Flash5fps21crf, {"_gemini25_flash_5fps21crf", 5, 23}},
        {ModelCode_t::Gemini25_Pro5fps20crf, {"_gemini25_pro_5fps20crf", 5, 20}},
        {ModelCode_t::Gemini25_Pro10fps20crf, {"_gemini25_pro_10fps20crf", 10, 20}},
        {ModelCode_t::Gemini3_FlashPreview5fps21crf, {"_gemini3_flashpreview5fps21crf", 5, 21}},
        {ModelCode_t::Gemini3_Flash5fps21crf, {"_gemini3_flash_5fps21crf", 5, 21}},
        {ModelCode_t::Gemini31_FlashLitePreview1fps23crf, {"_gemini31_flashlitepreview1fps23crf", 1, 23}},
        {ModelCode_t::Gemini31_FlashLitePreview2fps23crf, {"_gemini31_flashlitepreview2fps23crf", 2, 23}},
        {ModelCode_t::Gemini31_FlashPreview5fps21crf, {"_gemini31_flashpreview5fps21crf", 5, 21}},
        {ModelCode_t::Gemini35_Flash5fps21crf, {"_gemini35_flash5fps21crf", 5, 21}},
        {ModelCode_t::Gemini_FlashLiteLatest1fps23crf, {"_gemini31_flashlatest1fps23crf", 1, 23}},
        {ModelCode_t::Gemini_FlashLiteLatest2fps23crf, {"_gemini31_flashlatest2fps23crf", 2, 23}},
        {ModelCode_t::Gemini_FlashLatest5fps21crf, {"_gemini31_flashlatest5fps21crf", 5, 21}},
        {ModelCode_t::Gemini3_ProPreview5fps20crf, {"_gemini31_propreview5fps20crf", 5, 20}},
        {ModelCode_t::Gemini3_ProPreview10fps20crf, {"_gemini31_propreview10fps20crf", 10, 20}},
        {ModelCode_t::Gemini31_ProPreview5fps20crf, {"_gemini31_propreview5fps20crf", 5, 20}},
        {ModelCode_t::Gemini31_ProPreview10fps20crf, {"_gemini31_propreview10fps20crf", 10, 20}},
        {ModelCode_t::Gemini31_ProPreviewCustomTools5fps20crf, {"_gemini31_propreview5fps20crf", 5, 20}},
        {ModelCode_t::Gemini31_ProPreviewCustomTools10fps20crf, {"_gemini31_propreview10fps20crf", 10, 20}},
        {ModelCode_t::Gemini_ProLatest5fps20crf, {"_gemini31_prolatest5fps20crf", 5, 20}},
        {ModelCode_t::Gemini_ProLatest10fps20crf, {"_gemini31_prolatest10fps20crf", 10, 20}},
        {ModelCode_t::Grok3_5fps22crf, {"_grok3_5fps22crf", 5, 22}},
        {ModelCode_t::Grok3Mini_5fps22crf, {"_grok3Mini_5fps22crf", 5, 22}},
        {ModelCode_t::Grok4_10fps22crf, {"_grok4_10fps22crf", 10, 22}},
        {ModelCode_t::GrokMini_5fps22crf, {"_grokMini_5fps22crf", 5, 22}},
        {ModelCode_t::GPT4_10fps21crf, {"_gpt4_10fps21crf", 10, 22}},
        {ModelCode_t::GPT45_10fps21crf, {"_gpt45_10fps21crf", 10, 21}},
        {ModelCode_t::Llama30_5fps23crf, {"_llama30_5fps23crf", 5, 23}},
        {ModelCode_t::Llama31_5fps23crf, {"_llama31_5fps23crf", 5, 23}},
        {ModelCode_t::Llama4Scout_10fps23crf, {"_llama4Scout_10fps23crf", 10, 23}},
        {ModelCode_t::Llama4Maverick_10fps28crf, {"_llama4Maverick_10fps28crf", 10, 28}},
        {ModelCode_t::Llama4Behemoth_10fps28crf, {"_llama4Behemoth_10fps28crf", 10, 28}},
        {ModelCode_t::DeepSeekVL2_5fps28crf, {"_deepseekvl2_5fps28crf", 5, 28}},
        {ModelCode_t::Qwen3_5fps20crf, {"_qwen3_5fps20crf", 5, 28}}
    };

    auto it = configs.find(modelCode);
    return (it != configs.end()) ? it.value() : VideoModelConfig();
}

QString ExtractorUtils::formatTime(qint64 ms)
{
    qint64 totalSeconds = ms / 1000;
    int seconds = totalSeconds % 60;
    int minutes = (totalSeconds / 60) % 60;
    int hours = (totalSeconds / 3600);

    if (hours > 0)
    {
        return QString("%1:%2:%3")
                .arg(hours, 2, 10, QChar('0'))
                .arg(minutes, 2, 10, QChar('0'))
                .arg(seconds, 2, 10, QChar('0'));
    }

    return QString("%1:%2")
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
}

bool ExtractorUtils::hasSufficientDiskSpace(const QString &outDirPath, const QString &inputFilePath)
{
    // 1. Get input file size
    QFileInfo inputFile(inputFilePath);

    if ( ! inputFile.exists() )
    {
#ifdef QT_DEBUG
        LogUtils::safeLog("Utils::hasSufficientDiskSpace(...) --- Input file doesn't exist: " + inputFilePath,
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        return false;
    }

    qint64 inputSize = inputFile.size();

    // 2. Estimate output size (Input Size * 1.5 factor for safety)
    qint64 estimatedRequired = static_cast<qint64>(inputSize * 1.5);

    // 3. Check storage info for the output directory
    QStorageInfo storage(outDirPath);

    // If the directory doesn't exist yet, check the parent directory
    if ( ! storage.isValid() ||
         ! storage.isReady() )
    {
        storage.setPath(QFileInfo(outDirPath).absolutePath());
    }

    qint64 availableSpace = storage.bytesAvailable();

    if (availableSpace < estimatedRequired)
    {
        qCritical() << "Insufficient disk space. Available:" << (availableSpace / 1024 / 1024) << "MB"
                    << "Estimated required:" << (estimatedRequired / 1024 / 1024) << "MB";
        return false;
    }

#ifdef QT_DEBUG
    LogUtils::safeLog("Utils::hasSufficientDiskSpace(...) --- Necessary space is: " + QString::number(estimatedRequired),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    return true;
}

bool ExtractorUtils::isDirWritable(const QString &path)
{
    if (path.isEmpty()) return false;

    QDir dir(path);

    // 1. Try to create the directory if it doesn't exist
    if ( ! dir.exists() )
    {
        if (!dir.mkpath("."))
        {
            qCritical() << "Could not create directory:" << path;
            return false;
        }
    }

    // 2. Check for write permissions
    // On some OSs, QFileInfo::isWritable() can be unreliable for network drives.
    // A foolproof way is to try creating a temporary dummy file.
    QString testFileName = path + "/.write_test";
    QFile testFile(testFileName);

    if (testFile.open(QIODevice::WriteOnly))
    {
        testFile.close();
        testFile.remove();

        return true;
    }

    qCritical() << "Directory not writable:" << path;

    return false;
}

bool ExtractorUtils::checkIsValidFile(const QString &filePath)
{
    // Basic validation to prevent unnecessary filesystem calls
    if (filePath.trimmed().isEmpty())
    {
        return false;
    }

    QFileInfo fileInfo(filePath);

    return (fileInfo.exists() && fileInfo.isFile());
}

bool ExtractorUtils::isAudioSegmentValid(const QString &filePath)
{
    if ( filePath.isEmpty() || (! QFile::exists(filePath)) ) return false;

    AVFormatContext *fmt_ctx = nullptr;

    // Try to open the file and read the stream information
    if (avformat_open_input(&fmt_ctx, filePath.toUtf8().constData(), nullptr, nullptr) < 0) return false; // Cannot open header

    bool valid = false;

    if (avformat_find_stream_info(fmt_ctx, nullptr) >= 0 && fmt_ctx->duration > 0 && fmt_ctx->nb_streams > 0)
    {
        // Verify that the file actually contains at least one valid audio stream
        for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++)
        {
            if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
            {
                valid = true;
                break; // Found a valid audio stream, no need to check the rest
            }
        }
    }

    avformat_close_input(&fmt_ctx);

    return valid;
}

bool ExtractorUtils::isVideoSegmentValid(const QString &filePath)
{
    if ( filePath.isEmpty() || (! QFile::exists(filePath)) ) return false;

    AVFormatContext *fmt_ctx = nullptr;
    // Try to open the file and read the stream information
    if (avformat_open_input(&fmt_ctx, filePath.toUtf8().constData(), nullptr, nullptr) < 0) return false; // Cannot open header

    bool valid = ( ( avformat_find_stream_info(fmt_ctx, nullptr) >= 0 ) &&
                   (fmt_ctx->duration > 0) &&
                   (fmt_ctx->nb_streams > 0) );

    avformat_close_input(&fmt_ctx);

    return valid;
}

QString ExtractorUtils::ffmpegStateToString(FFmpegState_t state)
{
    switch (state)
    {
        case FFmpegState_t::Idle:                 return "Idle";
        case FFmpegState_t::OpeningInput:         return "Opening Input File";
        case FFmpegState_t::ConfiguringCodecs:    return "Initializing Encoders";
        case FFmpegState_t::Filtering:            return "Setting up FPS Filters";
        case FFmpegState_t::Processing:           return "Segmenting Video";
        case FFmpegState_t::Flushing:             return "Flushing Buffers";
        case FFmpegState_t::Finalizing:           return "Writing Trailer";
        case FFmpegState_t::Finished:             return "Completed";
        case FFmpegState_t::Error:                return "Error Occurred";
        default:                                return "Unknown";
    }
}

QString ExtractorUtils::ffmpegProcessStateToString(QProcess::ProcessState state)
{
    switch (state)
    {
        case QProcess::NotRunning: return "Not Running";

        case QProcess::Starting:   return "Starting";

        case QProcess::Running:    return "Running";

        default:         return "Unknown";
    }
}

void ExtractorUtils::performIdentitySanityCheck(const VideoSegmentInfo::VideoSegmentInfoList_t &results)
{
    QSet<UniqueId> seenIds;

    bool hasIssue = false;

    for (const auto &info : qAsConst(results))
    {
        if (info.state() != "OK") continue;

        if (seenIds.contains(info.id()))
        {
            hasIssue = true;

            QString errorMsg = QString("IDENTITY COLLISION: Segment %1 shares a UniqueId already in the set.")
                                   .arg(info.filePath());

#ifdef QT_DEBUG
            LogUtils::safeLog(QString("Utils::performIdentitySanityCheck(...) - %1")
                              .arg(errorMsg),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
#endif

        } else {
            seenIds.insert(info.id());
        }
    }

    if (!hasIssue)
    {
        qInfo() << "Sanity check passed for" << results.size() << "segments.";

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("Utils::performIdentitySanityCheck(...) - Sanity check passed for %1 segments.")
                              .arg(results.size()),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
    }
}

QByteArray ExtractorUtils::generateContentSeed(const QString &contentData, QCryptographicHash::Algorithm algorithm)
{
    if (contentData.isEmpty()) return QByteArray();

    // MD4 is fast for content-based seeds, though you can use Sha256
    // if you need higher collision resistance for your academic work.
    QCryptographicHash hash(algorithm);

    if (QFileInfo::exists(contentData))
    {
        QFile file(contentData);

        if (!file.open(QIODevice::OpenMode{QIODevice::ReadOnly}))
        {
            return QByteArray();
        }

        const qint64 chunkSize = 1024 * 1024; // 1MB

        QByteArray buffer;

        // Read in 1MB chunks - efficient for disk I/O and easy on RAM
        while (!file.atEnd())
        {
            buffer = file.read(chunkSize);

            if (buffer.isEmpty()) break; // Handle unexpected read failures

            hash.addData(buffer);
        }

        file.close();
    } else {
        hash.addData(contentData.toUtf8());
    }

    return hash.result();
}

qint32 ExtractorUtils::optimalThreadCount()
{
    // Returns the number of logical CPU cores
    qint32 cores = QThread::idealThreadCount();

    // Safety check: if the OS fails to report, default to 0 (which tells FFmpeg to auto-detect)
    if (cores <= 0) return 0;

    // Optional: Leave 1 core free for the OS/UI if the machine has many cores
    if (cores > 4)
    {
        return cores - 2;
    }

    return 1;
}

void ExtractorUtils::limit_to_cores(qint32 num_cores)
{
#if defined(Q_OS_LINUX)
    cpu_set_t cpuset;
    // cpu_set_set cpuset;
    CPU_ZERO(&cpuset);

    // Bind to the first 'num_cores' processors
    for (int i = 0; i < num_cores; ++i)
    {
        CPU_SET(i, &cpuset);
    }

    // Apply to the current process/thread
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

#elif defined(Q_OS_WIN)

    // Create a bitmask where each bit represents an allowed core
    // E.g., if num_cores is 4, mask is 00001111 (binary) = 0xF
    DWORD_PTR affinityMask = 0;

    DWORD_PTR affinityMask = 0;
    if (num_cores >= 64)
    {
        affinityMask = ~0ULL; // Enable all cores
    }
    else
    {
        affinityMask = (1ULL << num_cores) - 1;
    }

    // Get the handle to the current thread
    HANDLE hThread = GetCurrentThread();

    // Set the affinity mask for the current thread
    SetThreadAffinityMask(hThread, affinityMask);

#else

    // macOS (Darwin) does not support thread affinity via a public API.
    // The kernel is responsible for scheduling.
    Q_UNUSED(num_cores);

#endif
}

// Working: 20260803
// qint32 Utils::initCmdOptions(CmdArgumentsVec &cmdArgsVec, CmdLineOptions * cmdOptions)
// {
//     if (cmdArgsVec.isEmpty())
//     {
// #ifdef QT_DEBUG
//         LogUtils::safeLog(QString("Utils::initCmdOptions(CmdArgumentsVec &, CmdLineOptions *) - Error: cmdArgsVec is empty!"),
//                           __FILE__,
//                           __LINE__,
//                           Q_FUNC_INFO);
// #endif
//         return -1;
//     }

//     if (cmdOptions == nullptr)
//     {
// #ifdef QT_DEBUG
//         LogUtils::safeLog(QString("Utils::initCmdOptions(CmdArgumentsVec &, CmdLineOptions *) - Error: cmdOptions is null!"),
//                           __FILE__,
//                           __LINE__,
//                           Q_FUNC_INFO);
// #endif

//         return -2;
//     }

//     // Init cmdOptions
//     cmdOptions->fileInfo.inVideoMediaUrl.clear();
//     cmdOptions->fileInfo.outAudioMediaUrl.clear();
//     cmdOptions->fileInfo.outputAudioFormat.clear();
//     cmdOptions->webVTTSRT.srtConverterCmd.clear();
//     cmdOptions->webVTTSRT.srtConverterParams.clear();
//     cmdOptions->slowdownFactors.clear();
//     cmdOptions->clipDuration = DEFAULT_CLIP_SEC;

//     QString targetDirStr;
//     QString inputFileStr;
//     QString outputAudioFormatStr;

//     for (qint32 idx = 0; idx < cmdArgsVec.size(); ++idx)
//     {
//         // Skip previously consumed and cleared options
//         if (cmdArgsVec.at(idx).flag == CmdFlags_t::None)
//         {
//             continue;
//         }

// #ifdef QT_DEBUG
//         LogUtils::safeLog(QString("Utils::initCmdOptions(CmdArgumentsVec &, CmdLineOptions *) - Argument index/arg: %1 / %2 --- size: %3")
//                           .arg(idx).arg(cmdArgsVec.at(idx).value).arg(cmdArgsVec.size()),
//                           __FILE__,
//                           __LINE__,
//                           Q_FUNC_INFO);
// #endif

// #pragma clang diagnostic ignored "-Wswitch"
//         switch (cmdArgsVec.at(idx).flag)
//         {
//             case CmdFlags_t::InputFile:
//             case CmdFlags_t::TargetDir:
//             {
//                 qint32 tDirIdx = Utils::indexOfCmdArg(CmdFlags_t::TargetDir, cmdArgsVec);

// #ifdef QT_DEBUG
//                 LogUtils::safeLog(QString("Utils::initCmdOptions(CmdArgumentsVec &, CmdLineOptions *) - Target dir argument index/arg: %1 / %2!")
//                                   .arg(tDirIdx).arg(cmdArgsVec.at(tDirIdx).value),
//                                   __FILE__,
//                                   __LINE__,
//                                   Q_FUNC_INFO);
// #endif

//                 qint32 fileIdx = Utils::indexOfCmdArg(CmdFlags_t::InputFile, cmdArgsVec);

// #ifdef QT_DEBUG
//                 LogUtils::safeLog(QString("Utils::initCmdOptions(CmdArgumentsVec &, CmdLineOptions *) - Input file argument index/arg: %1 / %2!")
//                                       .arg(fileIdx).arg(cmdArgsVec.at(fileIdx).value),
//                                   __FILE__,
//                                   __LINE__,
//                                   Q_FUNC_INFO);
// #endif

//                 // Validation Guard: Ensure both critical path arguments are available before processing paths
//                 if (tDirIdx < 0 || fileIdx < 0)
//                 {
//                     continue;
//                 }

//                 QDir tDir;
//                 if ( ! cmdArgsVec.at(tDirIdx).value.isEmpty() )
//                 {
//                     if (! tDir.exists(cmdArgsVec.at(tDirIdx).value) &&
//                         ! tDir.mkpath(cmdArgsVec.at(tDirIdx).value) )
//                     {
// #ifdef QT_DEBUG
//                         LogUtils::safeLog(QString("Utils::initCmdOptions(CmdArgumentsVec &, CmdLineOptions *) - Error creating target dir: %1!")
//                                               .arg(cmdArgsVec.at(tDirIdx).value),
//                                           __FILE__,
//                                           __LINE__,
//                                           Q_FUNC_INFO);
// #endif

//                         return -100;

//                     }

//                     cmdOptions->dirInfo.defaultRootDir.setFile(cmdArgsVec.at(tDirIdx).value);

// #ifdef QT_DEBUG

//                     LogUtils::safeLog(QString("Utils::initCmdOptions(CmdArgumentsVec &, CmdLineOptions *) - Default root dir: %1 / %2!")
//                                           .arg(cmdOptions->dirInfo.defaultRootDir.absoluteFilePath(), cmdArgsVec.at(tDirIdx).value),
//                                       __FILE__,
//                                       __LINE__,
//                                       Q_FUNC_INFO);
// #endif
//                 }

//                 // Check and create directories.

//                 if ( cmdOptions->dirInfo.defaultRootDir.isDir() &&
//                      ! tDir.exists(cmdOptions->dirInfo.defaultRootDir.absoluteFilePath()) &&
//                      ! tDir.mkpath(cmdOptions->dirInfo.defaultRootDir.absoluteFilePath()) )
//                 {
// #ifdef QT_DEBUG
//                     LogUtils::safeLog(QString("Utils::initCmdOptions(CmdArgumentsVec &, CmdLineOptions *) - Error creating default root target dir: %1!")
//                                       .arg(cmdOptions->dirInfo.defaultRootDir.absoluteFilePath()),
//                                       __FILE__,
//                                       __LINE__,
//                                       Q_FUNC_INFO);
// #endif

//                     return -101;

//                 }

//                 QFileInfo inFileInfo(cmdArgsVec.at(fileIdx).value);

//                 if (! inFileInfo.isFile() ||
//                     ! inFileInfo.exists() )
//                 {
// #ifdef QT_DEBUG
//                     LogUtils::safeLog(QString("Utils::initCmdOptions(CmdArgumentsVec &, CmdLineOptions *) - InFile (%1) error!")
//                                           .arg(inFileInfo.absoluteFilePath()),
//                                       __FILE__,
//                                       __LINE__,
//                                       Q_FUNC_INFO);
// #endif

//                     return -102;
//                 }

//                 // Create inFile directory in home directory using inFile's UUID, from Url.
//                 Url fileUrl(inFileInfo.absoluteFilePath());

//                 if ( ! fileUrl.isEmpty() )
//                 {
// #ifdef QT_DEBUG
//                     if (fileUrl.isLocalFile())
//                     {
//                         LogUtils::safeLog(QString("Utils::initCmdOptions(CmdArgumentsVec &, CmdLineOptions *) - Local file path: %1!")
//                                               .arg(fileUrl.path()),
//                                               __FILE__,
//                                               __LINE__,
//                                               Q_FUNC_INFO);
//                     }
// #endif

//                     QString newFileHomePath(QDir(cmdOptions->dirInfo.defaultRootDir.absoluteFilePath())
//                                                 .filePath(
//                                                     fileUrl.getUuid().toString().remove('{').remove('}') +
//                                                     QDir::separator()));

// #ifdef QT_DEBUG
//                     LogUtils::safeLog(QString("Utils::initCmdOptions(CmdArgumentsVec &, CmdLineOptions *) - New possible file home path: %1!")
//                                           .arg(newFileHomePath),
//                                       __FILE__,
//                                       __LINE__,
//                                       Q_FUNC_INFO);
// #endif

//                     QDir fileHomeDir;

//                     if (! fileHomeDir.exists(newFileHomePath) &&
//                         ! fileHomeDir.mkpath(newFileHomePath) )
//                     {
// #ifdef QT_DEBUG
//                         LogUtils::safeLog(QString("Utils::initCmdOptions(CmdArgumentsVec &, CmdLineOptions *) - Could not create file dir path: %1!")
//                                               .arg(newFileHomePath),
//                                           __FILE__,
//                                           __LINE__,
//                                           Q_FUNC_INFO);
// #endif
//                         return -103;
//                     }

//                     cmdOptions->dirInfo.defaultRootDir.setFile(newFileHomePath);

// #ifdef QT_DEBUG
//                     LogUtils::safeLog(QString("Utils::initCmdOptions(CmdArgumentsVec &, CmdLineOptions *) - New file dir path: %1")
//                                           .arg(cmdOptions->dirInfo.defaultRootDir.path()),
//                                       __FILE__,
//                                       __LINE__,
//                                       Q_FUNC_INFO);
// #endif

//                     // QFileInfo outputFileInfo(cmdOptions->fileInfo.inVideoMediaUrl.toLocalFile());
//                     QFileInfo outputFileInfo(cmdOptions->dirInfo.defaultRootDir);

//                     // cmdOptions->dirInfo.transcriptsDir.setFile(outputFileInfo.path() + QwDir::separator()+ "transcripts");
//                     cmdOptions->dirInfo.annotation_and_transcriptsDir.setFile(QDir(outputFileInfo.absoluteFilePath()).filePath("annotation_and_transcripts"));
//                     cmdOptions->dirInfo.transcriptClipsDir.setFile(QDir(cmdOptions->dirInfo.annotation_and_transcriptsDir.absoluteFilePath()).filePath("transcriptClips"));

//                     cmdOptions->dirInfo.annotationClipsDir.setFile(QDir(cmdOptions->dirInfo.annotation_and_transcriptsDir.absoluteFilePath()).filePath("annotationClips"));

//                     // cmdOptions->dirInfo.audioDir.setFile(outputFileInfo.path() + QDir::separator()+ "audio");
//                     cmdOptions->dirInfo.audioDir.setFile(QDir(outputFileInfo.absoluteFilePath()).filePath("audio"));
//                     cmdOptions->dirInfo.audioClipsDir.setFile(QDir(cmdOptions->dirInfo.audioDir.absoluteFilePath()).filePath("audioClips"));

//                     // cmdOptions->dirInfo.videoClipsDir.setFile(outputFileInfo.path() + QDir::separator() + "videoShots");
//                     cmdOptions->dirInfo.videoDir.setFile(QDir(outputFileInfo.absoluteFilePath()).filePath("video") + QDir::separator());
//                     cmdOptions->dirInfo.videoClipsDir.setFile(QDir(cmdOptions->dirInfo.videoDir.absoluteFilePath()).filePath("videoClips"));

//                     if (! tDir.exists(cmdOptions->dirInfo.annotation_and_transcriptsDir.absoluteFilePath()) &&
//                         ! tDir.mkpath(cmdOptions->dirInfo.annotation_and_transcriptsDir.absoluteFilePath()) )
//                     {
// #ifdef QT_DEBUG
//                         LogUtils::safeLog(QString("Utils::initCmdOptions(CmdArgumentsVec &, CmdLineOptions *) - Error creating transcripts target dir: %1!")
//                                               .arg(cmdOptions->dirInfo.annotation_and_transcriptsDir.absoluteFilePath()),
//                                           __FILE__,
//                                           __LINE__,
//                                           Q_FUNC_INFO);
// #endif

//                         return -104;

//                     }

//                     if (! tDir.exists(cmdOptions->dirInfo.transcriptClipsDir.absoluteFilePath()) &&
//                         ! tDir.mkpath(cmdOptions->dirInfo.transcriptClipsDir.absoluteFilePath()) )
//                     {
// #ifdef QT_DEBUG
//                         LogUtils::safeLog(QString("Utils::initCmdOptions(CmdArgumentsVec &, CmdLineOptions *) - Error creating transcriptClips target dir: %1!")
//                                               .arg(cmdOptions->dirInfo.transcriptClipsDir.absoluteFilePath()),
//                                           __FILE__,
//                                           __LINE__,
//                                           Q_FUNC_INFO);
// #endif

//                         return -105;

//                     }

//                     if (! tDir.exists(cmdOptions->dirInfo.annotationClipsDir.absoluteFilePath()) &&
//                         ! tDir.mkpath(cmdOptions->dirInfo.annotationClipsDir.absoluteFilePath()) )
//                     {
// #ifdef QT_DEBUG
//                         LogUtils::safeLog(QString("Utils::initCmdOptions(CmdArgumentsVec &, CmdLineOptions *) - Error creating annotationClips clips target dir: %1!")
//                                               .arg(cmdOptions->dirInfo.annotationClipsDir.absoluteFilePath()),
//                                           __FILE__,
//                                           __LINE__,
//                                           Q_FUNC_INFO);
// #endif

//                         return -106;

//                     }

//                     if ( ! tDir.exists(cmdOptions->dirInfo.audioDir.absoluteFilePath()) &&
//                         ! tDir.mkpath(cmdOptions->dirInfo.audioDir.absoluteFilePath()) )
//                     {
// #ifdef QT_DEBUG
//                         LogUtils::safeLog(QString("Utils::initCmdOptions(CmdArgumentsVec &, CmdLineOptions *) - Error creating audio target dir: %1!")
//                                               .arg(cmdOptions->dirInfo.audioDir.absoluteFilePath()),
//                                           __FILE__,
//                                           __LINE__,
//                                           Q_FUNC_INFO);
// #endif

//                         return -107;

//                     }

//                     if ( ! tDir.exists(cmdOptions->dirInfo.audioClipsDir.absoluteFilePath()) &&
//                         ! tDir.mkpath(cmdOptions->dirInfo.audioClipsDir.absoluteFilePath()) )
//                     {
// #ifdef QT_DEBUG
//                         LogUtils::safeLog(QString("Utils::initCmdOptions(CmdArgumentsVec &, CmdLineOptions *) - Error creating audio clips target dir: %1!")
//                                               .arg(cmdOptions->dirInfo.audioClipsDir.absoluteFilePath()),
//                                           __FILE__,
//                                           __LINE__,
//                                           Q_FUNC_INFO);
// #endif

//                         return -108;

//                     }

//                     if ( ! tDir.exists(cmdOptions->dirInfo.videoDir.absoluteFilePath()) &&
//                         ! tDir.mkpath(cmdOptions->dirInfo.videoDir.absoluteFilePath()) )
//                     {
// #ifdef QT_DEBUG
//                         LogUtils::safeLog(QString("Utils::initCmdOptions(CmdArgumentsVec &, CmdLineOptions *) - Error creating video target dir: %1!")
//                                               .arg(cmdOptions->dirInfo.videoDir.absoluteFilePath()),
//                                           __FILE__,
//                                           __LINE__,
//                                           Q_FUNC_INFO);
// #endif
//                         return -109;

//                     }

//                     if ( ! tDir.exists(cmdOptions->dirInfo.videoClipsDir.absoluteFilePath()) &&
//                         ! tDir.mkpath(cmdOptions->dirInfo.videoClipsDir.absoluteFilePath()) )
//                     {
// #ifdef QT_DEBUG
//                         LogUtils::safeLog(QString("Utils::initCmdOptions(CmdArgumentsVec &, CmdLineOptions *) - Error creating video clips target dir: %1!")
//                                               .arg(cmdOptions->dirInfo.videoClipsDir.absoluteFilePath()),
//                                           __FILE__,
//                                           __LINE__,
//                                           Q_FUNC_INFO);
// #endif
//                         return -110;

//                     }

//                     // New absolute file path
//                     QString newFilePath(QDir(cmdOptions->dirInfo.videoDir.absolutePath()).filePath(
//                         fileUrl.getUuid().toString().remove('{').remove('}') + "." + inFileInfo.suffix()));

// #ifdef QT_DEBUG
//                     LogUtils::safeLog(QString("Utils::initCmdOptions(CmdArgumentsVec &, CmdLineOptions *) - ------>>>>>> Video directory: %1!")
//                                           .arg(newFilePath),
//                                       __FILE__,
//                                       __LINE__,
//                                       Q_FUNC_INFO);
// #endif

//                     if ( QFile::exists(newFilePath) &&
//                         ! QFile::remove(newFilePath) )
//                     {
// #ifdef QT_DEBUG
//                         LogUtils::safeLog(QString("Utils::initCmdOptions(CmdArgumentsVec &, CmdLineOptions *) - Error removing old file: %1!")
//                                               .arg(newFilePath),
//                                           __FILE__,
//                                           __LINE__,
//                                           Q_FUNC_INFO);
// #endif
//                         return -111;
//                     }

//                     // Copy input file to new location and new name.
//                     if ( ! QFile::copy(inFileInfo.absoluteFilePath(), newFilePath) )
//                     {
// #ifdef QT_DEBUG
//                         LogUtils::safeLog(QString("Utils::initCmdOptions(CmdArgumentsVec &, CmdLineOptions *) - Error copying input file to new location: %1!")
//                                               .arg(newFilePath),
//                                           __FILE__,
//                                           __LINE__,
//                                           Q_FUNC_INFO);
// #endif
//                         return -112;
//                     }

//                     // Set inMediaUrl new file path
//                     cmdOptions->fileInfo.inVideoMediaUrl.setPath(newFilePath);
//                     cmdOptions->fileInfo.inVideoMediaUrl.setScheme("file");

// #ifdef QT_DEBUG
//                     if (cmdOptions->fileInfo.inVideoMediaUrl.isLocalFile())
//                     {
//                         LogUtils::safeLog(QString("Utils::initCmdOptions(CmdArgumentsVec &, CmdLineOptions *) - Input file (%1): it is local file!")
//                                               .arg(cmdOptions->fileInfo.inVideoMediaUrl.path()),
//                                               __FILE__,
//                                               __LINE__,
//                                               Q_FUNC_INFO);
//                     } else {
//                         LogUtils::safeLog(QString("Utils::initCmdOptions(CmdArgumentsVec &, CmdLineOptions *) - Input file (%1): it is not local file! --- %2")
//                                               .arg(cmdOptions->fileInfo.inVideoMediaUrl.path(), newFilePath),
//                                               __FILE__,
//                                               __LINE__,
//                                               Q_FUNC_INFO);
//                     }
// #endif
//                 }

//                 cmdArgsVec.replace(tDirIdx, CmdArgument_st(CmdFlags_t::None, ""));
//                 cmdArgsVec.replace(fileIdx, CmdArgument_st(CmdFlags_t::None, ""));

//                 break;
//             }

//             case CmdFlags_t::OutputFormat:
//             {
//                 if (cmdArgsVec.at(idx).value.isEmpty())
//                 {
// #ifdef QT_DEBUG
//                     LogUtils::safeLog(QString("Utils::initCmdOptions(CmdArgumentsVec &, CmdLineOptions *) - Error: No output file format!"),
//                                       __FILE__,
//                                       __LINE__,
//                                       Q_FUNC_INFO);
// #endif
//                     return -200;
//                 }

//                 cmdOptions->fileInfo.outputAudioFormat = cmdArgsVec[idx].value;

// #ifdef QT_DEBUG
//                 LogUtils::safeLog(QString("Utils::initCmdOptions(CmdArgumentsVec &, CmdLineOptions *) - Output file format is: %1!")
//                                   .arg(cmdArgsVec.at(idx).value),
//                                   __FILE__,
//                                   __LINE__,
//                                   Q_FUNC_INFO);
// #endif

//                 QFileInfo inFile(cmdOptions->fileInfo.inVideoMediaUrl.path());

//                 cmdOptions->fileInfo.outAudioMediaUrl.setPath(QString(cmdOptions->dirInfo.audioDir.absoluteFilePath() + QDir::separator()+ inFile.fileName()).replace(inFile.suffix(), cmdOptions->fileInfo.outputAudioFormat, Qt::CaseInsensitive));

//                 cmdOptions->fileInfo.outAudioMediaUrl.setScheme("file");

//                 cmdArgsVec.replace(idx, CmdArgument_st(CmdFlags_t::None, ""));

//                 break;
//             }

//             case CmdFlags_t::Test:
//             {
// #ifdef QT_DEBUG
//                 LogUtils::safeLog(QString("Utils::initCmdOptions(CmdArgumentsVec &, CmdLineOptions *) - Testing %1!")
//                                   .arg(cmdArgsVec.at(idx).value),
//                                   __FILE__,
//                                   __LINE__,
//                                   Q_FUNC_INFO);
// #endif

//                 cmdArgsVec.replace(idx, CmdArgument_st(CmdFlags_t::None, ""));

//                 break;
//             }

//             case CmdFlags_t::SlowdownFactors:
//             {
//                 if (cmdArgsVec.at(idx).value.isEmpty())
//                 {
// #ifdef QT_DEBUG
//                     LogUtils::safeLog(QString("Utils::initCmdOptions(CmdArgumentsVec &, CmdLineOptions *) - Error: No slowdown value(s)!"),
//                                       __FILE__,
//                                       __LINE__,
//                                       Q_FUNC_INFO);
// #endif
//                     return -300;
//                 }

//                 QStringList slowdownValues = cmdArgsVec.at(idx).value.split(',', Qt::SkipEmptyParts);

//                 for (const QString &val : qAsConst(slowdownValues))
//                 {
//                     cmdOptions->slowdownFactors.append(val.toDouble());
//                 }

//                 cmdArgsVec.replace(idx, CmdArgument_st(CmdFlags_t::None, ""));

//                 break;
//             }

//             case CmdFlags_t::ClipDuration:
//             {
//                 if (cmdArgsVec.at(idx).value.isEmpty())
//                 {
// #ifdef QT_DEBUG
//                     LogUtils::safeLog(QString("Utils::initCmdOptions(CmdArgumentsVec &, CmdLineOptions *) - Error: No clip duration value!"),
//                                       __FILE__,
//                                       __LINE__,
//                                       Q_FUNC_INFO);
// #endif
//                     return -400;
//                 }

//                 // Converts "300" string sequence into qreal / double format safely
//                 cmdOptions->clipDuration = cmdArgsVec.at(idx).value.toDouble();
//                 cmdArgsVec.replace(idx, CmdArgument_st(CmdFlags_t::None, ""));
//                 break;
//             }

//             case CmdFlags_t::SearchedTerms:
//             {
//                 if (cmdArgsVec.at(idx).value.isEmpty())
//                 {
// #ifdef QT_DEBUG
//                     LogUtils::safeLog(QString("Utils::initCmdOptions(CmdArgumentsVec &, CmdLineOptions *) - Error: No searched terms value(s)!"),
//                                       __FILE__,
//                                       __LINE__,
//                                       Q_FUNC_INFO);
// #endif
//                     return -500;
//                 }

//                 QStringList searchedTermsValues = cmdArgsVec.at(idx).value.split(',', Qt::SkipEmptyParts);

//                 for (const QString &val : qAsConst(searchedTermsValues))
//                 {
//                     QString cleanedTerm = val.trimmed();

//                     if (!cleanedTerm.isEmpty())
//                     {
//                         cmdOptions->searchedTerms.append(cleanedTerm);
//                     }
//                 }

//                 cmdArgsVec.replace(idx, CmdArgument_st(CmdFlags_t::None, ""));

//                 break;
//             }

//             case CmdFlags_t::ConfidenceThreshould:
//             {
//                 if (cmdArgsVec.at(idx).value.isEmpty())
//                 {
// #ifdef QT_DEBUG
//                     LogUtils::safeLog(QString("Utils::initCmdOptions(CmdArgumentsVec &, CmdLineOptions *) - Error: No confidence threshould value!"),
//                                       __FILE__,
//                                       __LINE__,
//                                       Q_FUNC_INFO);
// #endif
//                     return -600;
//                 }

//                 double parsedThresh = cmdArgsVec.at(idx).value.toDouble();

//                 // Converts "300" string sequence into qreal / double format safely
//                 if (parsedThresh >= 0.00 &&
//                     parsedThresh <= 1.00)
//                 {
//                     cmdOptions->confidenceThreshould = parsedThresh;
//                 }

//                 cmdArgsVec.replace(idx, CmdArgument_st(CmdFlags_t::None, ""));
//                 break;
//             }

//             case CmdFlags_t::AnnotationFile:
//             {
//                 if (cmdArgsVec.at(idx).value.isEmpty())
//                 {
// #ifdef QT_DEBUG
//                     LogUtils::safeLog(QString("Utils::initCmdOptions(CmdArgumentsVec &, CmdLineOptions *) - Error: No annotation file provided!"),
//                                       __FILE__,
//                                       __LINE__,
//                                       Q_FUNC_INFO);
// #endif
//                     return -700;
//                 }

//                 QString annotationFile = cmdArgsVec.at(idx).value;

//                 if (Utils::checkIsValidFile(annotationFile))
//                 {
//                     cmdOptions->annotationFile = annotationFile;
//                 }

//                 cmdArgsVec.replace(idx, CmdArgument_st(CmdFlags_t::None, ""));
//                 break;
//             }

//             case CmdFlags_t::AudioFile:
//             {
//                 if (cmdArgsVec.at(idx).value.isEmpty())
//                 {
// #ifdef QT_DEBUG
//                     LogUtils::safeLog(QString("Utils::initCmdOptions(CmdArgumentsVec &, CmdLineOptions *) - Error: No audio file provided!"),
//                                       __FILE__,
//                                       __LINE__,
//                                       Q_FUNC_INFO);
// #endif
//                     return -701;
//                 }

//                 QString audioFile = cmdArgsVec.at(idx).value;

//                 if (Utils::checkIsValidFile(audioFile))
//                 {
//                     cmdOptions->audioFile = audioFile;
//                 }

//                 cmdArgsVec.replace(idx, CmdArgument_st(CmdFlags_t::None, ""));
//                 break;
//             }

//             case CmdFlags_t::SyncedManifestsFile:
//             {
//                 if (cmdArgsVec.at(idx).value.isEmpty())
//                 {
// #ifdef QT_DEBUG
//                     LogUtils::safeLog(QString("Utils::initCmdOptions(CmdArgumentsVec &, CmdLineOptions *) - Error: No synced manifests file provided!"),
//                                       __FILE__,
//                                       __LINE__,
//                                       Q_FUNC_INFO);
// #endif
//                     return -702;
//                 }

//                 QString syncedManifestsFile = cmdArgsVec.at(idx).value;

//                 if (Utils::checkIsValidFile(syncedManifestsFile))
//                 {
//                     cmdOptions->syncedManifestsJsonFile = syncedManifestsFile;
//                 }

//                 cmdArgsVec.replace(idx, CmdArgument_st(CmdFlags_t::None, ""));
//                 break;
//             }

//             case CmdFlags_t::TranscriptionFile:
//             {
//                 if (cmdArgsVec.at(idx).value.isEmpty())
//                 {
// #ifdef QT_DEBUG
//                     LogUtils::safeLog(QString("Utils::initCmdOptions(CmdArgumentsVec &, CmdLineOptions *) - Error: No transcription file provided!"),
//                                       __FILE__,
//                                       __LINE__,
//                                       Q_FUNC_INFO);
// #endif
//                     return -703;
//                 }

//                 QString transcriptionFile = cmdArgsVec.at(idx).value;

//                 if (Utils::checkIsValidFile(transcriptionFile))
//                 {
//                     cmdOptions->transcriptionFile = transcriptionFile;
//                 }

//                 cmdArgsVec.replace(idx, CmdArgument_st(CmdFlags_t::None, ""));
//                 break;
//             }

//             case CmdFlags_t::VideoFile:
//             {
//                 if (cmdArgsVec.at(idx).value.isEmpty())
//                 {
// #ifdef QT_DEBUG
//                     LogUtils::safeLog(QString("Utils::initCmdOptions(CmdArgumentsVec &, CmdLineOptions *) - Error: No video file provided!"),
//                                       __FILE__,
//                                       __LINE__,
//                                       Q_FUNC_INFO);
// #endif
//                     return -704;
//                 }

//                 QString videoFile = cmdArgsVec.at(idx).value;

//                 // Converts "300" string sequence into qreal / double format safely
//                 if (Utils::checkIsValidFile(videoFile))
//                 {
//                     cmdOptions->videoFile = videoFile;
//                 }

//                 cmdArgsVec.replace(idx, CmdArgument_st(CmdFlags_t::None, ""));
//                 break;
//             }

//             case CmdFlags_t::PipelineMode:
//             {
//                 if (cmdArgsVec.at(idx).value.isEmpty())
//                 {
// #ifdef QT_DEBUG
//                     LogUtils::safeLog(QString("Utils::initCmdOptions(CmdArgumentsVec &, CmdLineOptions *) - Error: No option term value provided!"),
//                                       __FILE__,
//                                       __LINE__,
//                                       Q_FUNC_INFO);
// #endif
//                     return -800;
//                 }

//                 cmdOptions->pipelineOption = PipelineModeMap.key(cmdArgsVec.at(idx).value, PipelineMode_t::None);

//                 cmdArgsVec.replace(idx, CmdArgument_st(CmdFlags_t::None, ""));

//                 break;
//             }
//         }
//     }

//     return 0;
// }

qint32 ExtractorUtils::initCmdOptions(CmdArgumentsVec &cmdArgsVec, CmdLineOptions * cmdOptions)
{
    // ==================================
    // 0. Safety Guards & Initialization
    // ==================================
    if (cmdArgsVec.isEmpty())
    {
#ifdef QT_DEBUG
        LogUtils::safeLog("Utils::initCmdOptions(...) - Error: cmdArgsVec is empty!",
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        return -1;
    }

    if (cmdOptions == nullptr)
    {
#ifdef QT_DEBUG
        LogUtils::safeLog("Utils::initCmdOptions(...) - Error: cmdOptions is null!",
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        return -2;
    }

    // Reset Defaults
    cmdOptions->fileInfo.inVideoMediaUrl.clear();
    cmdOptions->fileInfo.outAudioMediaUrl.clear();
    cmdOptions->fileInfo.outputAudioFormat.clear();
    cmdOptions->webVTTSRT.srtConverterCmd.clear();
    cmdOptions->webVTTSRT.srtConverterParams.clear();
    cmdOptions->slowdownFactors.clear();
    cmdOptions->clipDuration = *Constants::Q_QGS_DEFAULT_CLIP_SEC;

    // Local extraction caches to fix command-line ordering bugs
    QString targetDirStr;
    QString inputFileStr;
    QString outputAudioFormatStr;

    // ===========================================
    // Phase 1: Order-Independent Data Extraction
    // ===========================================
    for (qint32 idx = 0; idx < cmdArgsVec.size(); ++idx)
    {
        CmdArgument_st &arg = cmdArgsVec[idx];

        if (arg.flag == CmdFlags_t::None)
        {
            continue;
        }

        const QString &val = arg.value;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch"
        switch (arg.flag)
        {
            case CmdFlags_t::PipelineMode:
            {
                if (!val.isEmpty())
                    cmdOptions->pipelineMode = PipelineModeMap.key(val, PipelineMode_t::None);
                break;
            }

            case CmdFlags_t::OutputFormat:
            {
                if (!val.isEmpty()) outputAudioFormatStr = val;

                break;
            }

            case CmdFlags_t::SlowdownFactors:
            {
                if (!val.isEmpty())
                    for (const QString &v : val.split(',', Qt::SkipEmptyParts))
                    {
                        cmdOptions->slowdownFactors.append(v.toDouble());
                    }

                break;
            }

            case CmdFlags_t::ClipDuration:
            {
                if (!val.isEmpty()) cmdOptions->clipDuration = val.toDouble();

                break;
            }

            case CmdFlags_t::ConfidenceThreshould:
            {
                if (!val.isEmpty())
                {
                    double parsedThresh = val.toDouble();

                    if (parsedThresh >= 0.00 && parsedThresh <= 1.00)
                    {
                        cmdOptions->confidenceThreshold = parsedThresh;
                    }
                }

                break;
            }

            case CmdFlags_t::TranscriptionConverter:
            {
                if (!val.isEmpty())
                    cmdOptions->transcriptionConverterType = TranscriptionConverterMap.key(val, ConverterType_t::None);

                break;
            }

            case CmdFlags_t::AIModelType:
            {
                if (!val.isEmpty())
                    cmdOptions->aiModelType = ExtractorUtils::getModelTypeFromString(val, getAIModelMap());

                break;
            }

            case CmdFlags_t::S2TModelType:
            {
                if (!val.isEmpty())
                    cmdOptions->speech2TextModel = DeepgramAIS2TT2SModelMap.key(val, Deepgram_AIS2TT2SModels_t::NONE);

                break;
            }

            case CmdFlags_t::SystemProfile:
            {
                if (!val.isEmpty())
                    cmdOptions->systemProfile = PersonaTypeMap.key(val, Persona::None);

                break;
            }

            case CmdFlags_t::TaskAnalysis:
            {
                if (!val.isEmpty())
                    cmdOptions->taskAnalysis = TaskTemplateTypeMap.key(val, TaskTemplate::None);

                break;
            }

            case CmdFlags_t::VideoSegmentationModel:
            {
                if (!val.isEmpty())
                    cmdOptions->videoSegmentationModel = ModelCodeMap.key(val, ModelCode_t::None);

                break;
            }


            case CmdFlags_t::SearchType:
            {
                if (!val.isEmpty())
                {
                    cmdOptions->searchType = KeywordMatchTypeMap.key(val.toLower(), KeywordMatchType_t::Any);
                }

                break;
            }

            case CmdFlags_t::SearchedTerms:
            {
                if (!val.isEmpty())
                    for (const QString &v : val.split(',', Qt::SkipEmptyParts))
                    {
                        QString cleanedTerm = v.trimmed();

                        if (!cleanedTerm.isEmpty()) cmdOptions->searchedTerms.append(cleanedTerm);
                    }
                break;
            }


            case CmdFlags_t::TargetDir:
            {
                if (!val.isEmpty()) targetDirStr = val;

                break;
            }


            case CmdFlags_t::AnnotationFile:
            {
                if (!val.isEmpty())
                    if (ExtractorUtils::checkIsValidFile(val)) cmdOptions->annotationFile = val;

                break;
            }

            case CmdFlags_t::AudioFile:
            {
                if (!val.isEmpty())
                    if (ExtractorUtils::checkIsValidFile(val)) cmdOptions->audioFile = val;

                break;
            }

            case CmdFlags_t::InputFile:
            {
                if (!val.isEmpty()) inputFileStr = val;

                break;
            }


            case CmdFlags_t::SyncedManifestsFile:
            {
                if (!val.isEmpty())
                    if (ExtractorUtils::checkIsValidFile(val)) cmdOptions->syncedManifestsJsonFile = val;

                break;
            }

            case CmdFlags_t::TranscriptionFile:
            {
                if (!val.isEmpty())
                    if (ExtractorUtils::checkIsValidFile(val)) cmdOptions->transcriptionFile = val;

                break;
            }

            case CmdFlags_t::VideoFile:
            {
                if (!val.isEmpty())
                    if (ExtractorUtils::checkIsValidFile(val)) cmdOptions->videoFile = val;

                break;
            }

            case CmdFlags_t::ExperimentMode:
            {
                if (!val.isEmpty())
                    cmdOptions->experimentOption = ExperimentModeMap.key(val, ExperimentMode_t::None);

                break;
            }

            case CmdFlags_t::BaselineMarkdown:
            {
                if (!val.isEmpty())
                    if (ExtractorUtils::checkIsValidFile(val)) cmdOptions->baselineMarkdownFile = val;

                break;
            }

            case CmdFlags_t::GovernedMarkdown:
            {
                if (!val.isEmpty())
                    if (ExtractorUtils::checkIsValidFile(val)) cmdOptions->governedMarkdownFile = val;

                break;
            }

            case CmdFlags_t::GroundTruth:
            {
                if (!val.isEmpty())
                    if (ExtractorUtils::checkIsValidFile(val)) cmdOptions->groundTruthFile = val;

                break;
            }

            case CmdFlags_t::TemporalTolerance:
            {
                if (!val.isEmpty() &&
                    (val.toInt() > 0))
                {
                    cmdOptions->temporalTolerance = val.toInt();
                }

                break;
            }

            case CmdFlags_t::Test:
            {
                // Test mode handling is usually pre-empted in main, safely ignore here
                break;
            }
        }
#pragma clang diagnostic pop

        // Mark as consumed
        arg.flag = CmdFlags_t::None;
        arg.value = "";
    }

    // =========================================================================
    // Phase 2: Strict Semantic Field Validation
    // =========================================================================

    // 2.1 Universal Requirement: Pipeline Mode
    if (cmdOptions->pipelineMode == PipelineMode_t::None)
    {
        QString err_msg("Usage Error: Missing mandatory pipeline mode.\nRequired: -m <mode> (e.g., transcode_only, transcribe_only, convert_only)");

        qCritical().noquote() << err_msg;

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("Utils::initCmdOptions(...) - %1")
                              .arg(err_msg),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        return -3;
    }

    // 2.2 Mode-Specific Dependencies
    bool missingArgs = false;
    QString missingMsg = "Usage Error: Missing mandatory fields for the selected mode.\nRequired for ";

    switch (cmdOptions->pipelineMode)
    {
        case PipelineMode_t::TranscodeOnly:
        {
            if (inputFileStr.isEmpty() ||
                outputAudioFormatStr.isEmpty() ||
                targetDirStr.isEmpty())
            {
                missingArgs = true;
                missingMsg += "Transcode only:\n -m <mode> -i <video_file> -af <audio_format> -td <target_dir>";
            }

            break;
        }

        case PipelineMode_t::TranscribeOnly:
        {
            if (cmdOptions->audioFile.isEmpty() ||
                (cmdOptions->speech2TextModel == Deepgram_AIS2TT2SModels_t::NONE) ||
                targetDirStr.isEmpty())
            {
                missingArgs = true;
                missingMsg += "Transcribe only:\n -m <mode> -a <audio_file> -s2tm <audio_speech_2_text_model> -td <target_dir>"; // -s2t DeepgramS2TNova2GeneralEN
            }

            break;
        }
        case PipelineMode_t::ConvertOnly:
        {
            if (cmdOptions->annotationFile.isEmpty() ||
                (cmdOptions->transcriptionConverterType == ConverterType_t::None) ||
                targetDirStr.isEmpty())
            {
                missingArgs = true;
                missingMsg += "Convert only:\n -m <mode> -an <annotation_file> -convType <converter_type> -td <target_dir>"; // DeepgramWebVttConverter
            }

            break;
        }

        case PipelineMode_t::SegmentOnly:
        {
            if (cmdOptions->videoFile.isEmpty() ||
                cmdOptions->audioFile.isEmpty() ||
                cmdOptions->annotationFile.isEmpty() ||
                cmdOptions->transcriptionFile.isEmpty() ||
                (cmdOptions->clipDuration <= 0.00) ||
                (cmdOptions->videoSegmentationModel == ModelCode_t::None) ||
                cmdOptions->slowdownFactors.isEmpty() ||
                targetDirStr.isEmpty())
            {
                missingArgs = true;
                missingMsg += "Segment only:\n -m <mode> -v <video> -a <audio> -an <annotation> -tr <transcript> -cd <clip_duration> -vsm <video_seg_mode> -sf <slowdown_factors> -td <target_dir>"; // -vsm Gemini25_Flash5fps21crf
            }

            break;
        }

        // If there is no search terms, submit all video segments.
        case PipelineMode_t::EvtsExtractionOnly:
        {
            if (cmdOptions->syncedManifestsJsonFile.isEmpty() ||
                (cmdOptions->aiModelType == AIModels_t::None) ||
                cmdOptions->slowdownFactors.isEmpty() ||
                (cmdOptions->clipDuration <= 0.00) ||
                (!cmdOptions->searchedTerms.isEmpty() &&
                 ((cmdOptions->searchType == KeywordMatchType_t::None) ||
                  (cmdOptions->confidenceThreshold <= 0.00) ||
                  (cmdOptions->confidenceThreshold > 1.00))) ||
                (cmdOptions->taskAnalysis == TaskTemplate::None) ||
                (cmdOptions->systemProfile == Persona::None) ||
                targetDirStr.isEmpty())
            {
                missingArgs = true;
                missingMsg += "Event extraction only:\n -m <mode> -sm <synced_manifests> -aim <ai_model_type>"
                              " -sf \"['<slowdown_factor>', '<slowdown_factors>']\" -cd <clip_duration> "
                              "-ta <task_analisis> -sp <system_profile> -st <searched_terms> "
                              "-sType <search_type> -ct <confidence_threshold> -td <target_dir>";
                // -aim gemini-2.5-flash -ta video_analysis_task -sp fifa_referee -sType any
            }

            break;
        }

        // If there is no search terms, submit all video segments.
        case PipelineMode_t::Pipepline:
        {
            if (inputFileStr.isEmpty() ||
                outputAudioFormatStr.isEmpty() ||
                (cmdOptions->clipDuration <= 0.00) ||
                (cmdOptions->speech2TextModel == Deepgram_AIS2TT2SModels_t::NONE) ||
                (cmdOptions->aiModelType == AIModels_t::None) ||
                cmdOptions->slowdownFactors.isEmpty() ||
                (!cmdOptions->searchedTerms.isEmpty() &&
                 ((cmdOptions->searchType == KeywordMatchType_t::None) ||
                  (cmdOptions->confidenceThreshold <= 0.00) ||
                  (cmdOptions->confidenceThreshold > 1.00))) ||
                (cmdOptions->taskAnalysis == TaskTemplate::None) ||
                (cmdOptions->systemProfile == Persona::None) ||
                targetDirStr.isEmpty()
                )
            {
                missingArgs = true;
                missingMsg += "entire pipeline:\n -m <mode> -i <video_file> -af <audio_format> "
                              "-s2tm <audio_speech_2_text_model> -convType <converter_type> "
                              "-vsm <video_seg_mode> -cd <clip_duration> -aim <ai_model_type> "
                              "-sf <slowdown_factors> -ta <task_analisis> -sp <system_profile> "
                              "-st <searched_terms> -sType <search_type> -ct <confidence_threshold> "
                              "-td <target_dir>";
            }
        }

        case PipelineMode_t::Experiment:
        {
            switch (cmdOptions->experimentOption)
            {
                case ExperimentMode_t::TokenValidation:
                {
                    if (targetDirStr.isEmpty())
                    {
                        missingArgs = true;
                        missingMsg += " token validation experiment:\n -m experiment -exp token_validation_experiment -td <path_to_annotation_and_vtt_parent_dir>";
                    }

                    break;
                }

                // If there is no search terms, submit all video segments.
                case ExperimentMode_t::BaselineGoverned:
                {
                    if (targetDirStr.isEmpty() ||
                        (cmdOptions->aiModelType == AIModels_t::None) ||
                        (!cmdOptions->searchedTerms.isEmpty() &&
                         ((cmdOptions->searchType == KeywordMatchType_t::None) ||
                          (cmdOptions->confidenceThreshold <= 0.00) ||
                          (cmdOptions->confidenceThreshold > 1.00))) ||
                        cmdOptions->syncedManifestsJsonFile.isEmpty()
                        )
                    {
                        missingArgs = true;
                        missingMsg += "baseline & governed events extraction runs:\n -m experiment -exp baseline_governed_experiment -aim <ai_model_type> -st <searched_terms> -sType <search_type> -ct <confidence_threshold> -td <path_to_where_the_extractions_results_must_be_saved> -sm <synced_manifests>";
                    }

                    break;
                }

                case ExperimentMode_t::GovernanceAudit:
                {
                    if (targetDirStr.isEmpty() ||
                        (cmdOptions->aiModelType == AIModels_t::None) ||
                        cmdOptions->governedMarkdownFile.isEmpty() ||
                        cmdOptions->baselineMarkdownFile.isEmpty())
                    {
                        missingArgs = true;
                        missingMsg += "governance audit :\n -m experiment -exp governance_audit -aim <ai_model_type> -td <target_dir> -gmd <filepath> -bmd <filepath>";
                    }

                    break;
                }

                case ExperimentMode_t::ReportEventsFound:
                {
                    if (targetDirStr.isEmpty() ||
                        cmdOptions->governedMarkdownFile.isEmpty())
                    {
                        missingArgs = true;
                        missingMsg += "governance audit :\n -m experiment -exp report_events_found -td <target_dir> -gmd <filepath>";
                    }

                    break;
                }

                case ExperimentMode_t::ReportEventsRejected:
                {
                    if (targetDirStr.isEmpty() ||
                        cmdOptions->governedMarkdownFile.isEmpty())
                    {
                        missingArgs = true;
                        missingMsg += "governance audit :\n -m experiment -exp report_events_rejected -td <target_dir> -gmd <filepath>";
                    }

                    break;
                }

                case ExperimentMode_t::PrintReportGoverned:
                {
                    if (targetDirStr.isEmpty() ||
                        cmdOptions->governedMarkdownFile.isEmpty())
                    {
                        missingArgs = true;
                        missingMsg += "governance audit :\n -m experiment -exp print_report_governed -td <target_dir> -gmd <filepath>";
                    }

                    break;
                }

                case ExperimentMode_t::EventSpottingDomainAlignment:
                {
                    if (targetDirStr.isEmpty() ||
                        (cmdOptions->aiModelType == AIModels_t::None) ||
                        cmdOptions->governedMarkdownFile.isEmpty() ||
                        cmdOptions->groundTruthFile.isEmpty()
                        )
                    {
                        missingArgs = true;
                        missingMsg += "precise event spotting & clock-domain alingment:\n -m experiment -exp event_spotting_domain_alignment -aim <ai_model_type> -gt <groundTruthFile> -gmd <filepath> -td <target_dir>";
                    }

                    break;
                }

                case ExperimentMode_t::None:
                {
                    break;
                }
            }

            break;
        }

        case PipelineMode_t::None:
        {
            break; // Handled above
        }
    }

    if (missingArgs)
    {
        qCritical().noquote() << missingMsg;

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("Utils::initCmdOptions(...) - %1!")
                              .arg(missingMsg),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        return -4;
    }

    // =========================================================================
    // Phase 3: Adaptive I/O Orchestration (Directory Generation)
    // =========================================================================
    if (!targetDirStr.isEmpty())
    {
        QString newFileHomePath;
        QFileInfo inFileInfo;

        // If a video input is provided (Pipeline or Transcode), generate the UUID directory
        if (!inputFileStr.isEmpty())
        {
            inFileInfo.setFile(inputFileStr);

            if (!inFileInfo.isFile() || !inFileInfo.exists())
            {
#ifdef QT_DEBUG
                LogUtils::safeLog(QString("Utils::initCmdOptions(...) - InFile (%1) error!")
                                      .arg(inFileInfo.absoluteFilePath()),
                                  __FILE__,
                                  __LINE__,
                                  Q_FUNC_INFO);
#endif
                return -5;
            }

            Url fileUrl(inFileInfo.absoluteFilePath());
            newFileHomePath = QDir(targetDirStr).filePath(fileUrl.getUuid().toString().remove('{').remove('}'));
        }
        else
        {
            // For isolated downstream runs (Transcribe, Convert), the targetDir is treated as the direct root
            newFileHomePath = targetDirStr;
        }

        // Pre-configure all directory paths in the struct
        cmdOptions->dirInfo.defaultRootDir.setFile(newFileHomePath);

        QDir root(newFileHomePath);
        cmdOptions->dirInfo.annotation_and_transcriptsDir.setFile(root.filePath("annotation_and_transcripts") + QDir::separator());
        cmdOptions->dirInfo.transcriptClipsDir.setFile(QDir(cmdOptions->dirInfo.annotation_and_transcriptsDir.absoluteFilePath()).filePath("transcriptClips"));
        cmdOptions->dirInfo.annotationClipsDir.setFile(QDir(cmdOptions->dirInfo.annotation_and_transcriptsDir.absoluteFilePath()).filePath("annotationClips"));

        cmdOptions->dirInfo.audioDir.setFile(root.filePath("audio") + QDir::separator());
        cmdOptions->dirInfo.audioClipsDir.setFile(QDir(cmdOptions->dirInfo.audioDir.absoluteFilePath()).filePath("audioClips") + QDir::separator());

        cmdOptions->dirInfo.videoDir.setFile(root.filePath("video") + QDir::separator());
        cmdOptions->dirInfo.videoClipsDir.setFile(QDir(cmdOptions->dirInfo.videoDir.absoluteFilePath()).filePath("videoClips") + QDir::separator());

        // Helper lambda to DRY up the directory creation logic
        auto ensureDir = [](const QFileInfo &dirInfo,
                            qint32 errCode,
                            const QString &logName) -> qint32 {
#ifndef QT_DEBUG
            Q_UNUSED(logName)
#endif
            QDir d;

            if (!d.exists(dirInfo.absoluteFilePath()) && !d.mkpath(dirInfo.absoluteFilePath()))
            {
#ifdef QT_DEBUG
                LogUtils::safeLog(QString("Utils::initCmdOptions(...) - Error creating %1 target dir: %2")
                                      .arg(logName,
                                           dirInfo.absoluteFilePath()),
                                  __FILE__,
                                  __LINE__,
                                  Q_FUNC_INFO);
#endif

                return errCode;
            }

            return 0;
        };

        // Create the directory tree safely
        qint32 err;
        if ((err = ensureDir(QFileInfo(targetDirStr), -100, "base target")) < 0) return err;
        if ((err = ensureDir(cmdOptions->dirInfo.defaultRootDir, -103, "root (UUID)")) < 0) return err; // Using -103 to match your legacy fileHomeDir code
        if ((err = ensureDir(cmdOptions->dirInfo.annotation_and_transcriptsDir, -104, "transcripts")) < 0) return err;
        if ((err = ensureDir(cmdOptions->dirInfo.transcriptClipsDir, -105, "transcript clips")) < 0) return err;
        if ((err = ensureDir(cmdOptions->dirInfo.annotationClipsDir, -106, "annotation clips")) < 0) return err;
        if ((err = ensureDir(cmdOptions->dirInfo.audioDir, -107, "audio")) < 0) return err;
        if ((err = ensureDir(cmdOptions->dirInfo.audioClipsDir, -108, "audio clips")) < 0) return err;
        if ((err = ensureDir(cmdOptions->dirInfo.videoDir, -109, "video")) < 0) return err;
        if ((err = ensureDir(cmdOptions->dirInfo.videoClipsDir, -110, "video clips")) < 0) return err;

        if (!inputFileStr.isEmpty())
        {
            // Copy source video to the new working directory
            QString newFilePath = QDir(cmdOptions->dirInfo.videoDir.absolutePath()).filePath(Url(inFileInfo.absoluteFilePath()).getUuid().toString().remove('{').remove('}') + "." + inFileInfo.suffix());

            if (QFile::exists(newFilePath) && !QFile::remove(newFilePath))
            {
                return -6;
            }

            if (!QFile::copy(inFileInfo.absoluteFilePath(), newFilePath))
            {
                return -7;
            }

            // Finalize Video Path state
            cmdOptions->fileInfo.inVideoMediaUrl.setPath(newFilePath);
            cmdOptions->fileInfo.inVideoMediaUrl.setScheme("file");
        }
    }

    // =========================================================================
    // Phase 4: Setup Audio Output Path
    // (Requires Phase 2 to be fully completed first)
    // =========================================================================
    if (!outputAudioFormatStr.isEmpty() &&
        !cmdOptions->fileInfo.inVideoMediaUrl.isEmpty())
    {
        cmdOptions->fileInfo.outputAudioFormat = outputAudioFormatStr;
        QFileInfo inFile(cmdOptions->fileInfo.inVideoMediaUrl.path());

        QString outAudioName = inFile.fileName().replace(inFile.suffix(), cmdOptions->fileInfo.outputAudioFormat, Qt::CaseInsensitive);
        cmdOptions->fileInfo.outAudioMediaUrl.setPath(QDir(cmdOptions->dirInfo.audioDir.absoluteFilePath()).filePath(outAudioName));
        cmdOptions->fileInfo.outAudioMediaUrl.setScheme("file");
    }

#ifdef QT_DEBUG
    LogUtils::safeLog("Utils::initCmdOptions() - Exiting successfully.",
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    return 0;
}

//Link:  https://codingbyexample.com/2019/01/14/command-line-and-function-arguments/
// Working: 20260803
// CmdArgumentsVec Utils::commandLineParser(const QStringList &args)
// {
// #ifdef QT_DEBUG
//         LogUtils::safeLog(QString("Utils::commandLineParser(...) - Entering"),
//                 __FILE__,
//                 __LINE__,
//                 Q_FUNC_INFO);
// #endif

//      CmdArgumentsVec parsedOptions;

//      qint32 idx = 1;
//      while (idx < args.size())
//      {
// #ifdef QT_DEBUG
//          LogUtils::safeLog(QString("Utils::commandLineParser(...) - Argument index: %1")
//                            .arg(idx),
//                            __FILE__,
//                            __LINE__,
//                            Q_FUNC_INFO);
// #endif
//          const QString &currentArg = args.at(idx);

//          if (currentArg.startsWith("--") || currentArg.startsWith("-"))
//          {
//              QString flag = currentArg;
//              QString value = "";
//              // bool hasInlineValue = false;

//              if (currentArg.contains('='))
//              {
//                  qint32 eqIdx = currentArg.indexOf('=');
//                  flag = currentArg.left(eqIdx);
//                  value = currentArg.mid(eqIdx + 1);
//                  // hasInlineValue = true;
//                  idx++;

//              } else
//              {
//                  // =========================================================================
//                  // Look-ahead value consumption
//                  // If the next token exists and isn't another flag, we safely consume it
//                  // by incrementing 'idx' immediately within this scope.
//                  // =========================================================================
//                  if (idx + 1 < args.size() &&
//                      !args.at(idx + 1).startsWith("-") &&
//                      !args.at(idx + 1).startsWith("--"))
//                  {
//                      value = args.at(idx +1);
//                      idx += 2;
//                  } else
//                  {
//                      idx++;
//                  }
//              }

//              bool validFlag = true;
//              bool internalAppendSuccess = false;

//              if ((flag == "-i") || (flag == "--input"))
//              {
//                  if (!value.isEmpty())
//                  {
//                      parsedOptions.append(CmdArgument_st(CmdFlags_t::InputFile, value));
//                      internalAppendSuccess = true;
//                  }
//              }
//              else if ((flag == "-af") || (flag == "--audio_format"))
//              {
//                  if (!value.isEmpty())
//                  {
//                      parsedOptions.append(CmdArgument_st(CmdFlags_t::OutputFormat, value));
//                      internalAppendSuccess = true;
//                  }
//              }
//              else if ((flag == "-td") || (flag == "--target_dir"))
//              {
//                  if (!value.isEmpty())
//                  {
//                      parsedOptions.append(CmdArgument_st(CmdFlags_t::TargetDir, value));
//                      internalAppendSuccess = true;
//                  }
//              }
//              else if ((flag == "-sf") || (flag == "--slowdownFactors"))
//              {
//                  if (!value.isEmpty())
//                  {
//                      // Cleans away inline/spaced square brackets and whitespaces
//                      // Strip brackets and ALL quotes (single and double)
//                      QString cleanVectorStr = value;
//                      cleanVectorStr.remove('[').remove(']').remove('\'').remove('\"');

//                      // Safe to remove spaces here since slowdown factors are just numbers
//                      cleanVectorStr.remove(' ');

//                      // Stores the comma-separated sequence inside the type structure definition
//                      parsedOptions.append(CmdArgument_st(CmdFlags_t::SlowdownFactors, cleanVectorStr));

//                      internalAppendSuccess = true;
//                  }
//              }
//              else if ((flag == "-cd") || (flag == "--clipDuration"))
//              {
//                  if (!value.isEmpty())
//                  {
//                      parsedOptions.append(CmdArgument_st(CmdFlags_t::ClipDuration, value));
//                      internalAppendSuccess = true;
//                  }
//              }
//              else if ((flag == "-st") || (flag == "--searchedTerms"))
//              {
//                  if (!value.isEmpty())
//                  {
//                      // Cleans away inline/spaced square brackets and whitespaces
//                      QString cleanVectorStr = value;
//                      // 1. Remove brackets and ALL quote variations
//                      cleanVectorStr.remove('[').remove(']').remove('\'').remove('\"');

//                      // Stores the comma-separated sequence inside the type structure definition
//                      // 2. DO NOT remove(' ') here! It will destroy "red card".
//                      // Note: It is assumed that the downstream Utils::initCmdOptions splits this by
//                      // comma and calls .trimmed() on each item to clean up the edge spaces.
//                      parsedOptions.append(CmdArgument_st(CmdFlags_t::SearchedTerms, cleanVectorStr));

//                      internalAppendSuccess = true;
//                  }
//              }
//              else if ((flag == "-ct") || (flag == "--confidenceThreshold") )
//              {
//                  if (!value.isEmpty())
//                  {
//                      parsedOptions.append(CmdArgument_st(CmdFlags_t::ConfidenceThreshould, value));
//                      internalAppendSuccess = true;
//                  }
//              }
//              else if ((flag == "-t") || (flag == "--test"))
//              {
//                  if (!value.isEmpty())
//                  {
//                      parsedOptions.append(CmdArgument_st(CmdFlags_t::Test, value));

//                      internalAppendSuccess = true;
//                  }
//              }
//              else if ((flag == "-m") || (flag == "--mode"))
//              {
//                  if (!value.isEmpty())
//                  {
//                      parsedOptions.append(CmdArgument_st(CmdFlags_t::PipelineMode, value));

//                      internalAppendSuccess = true;
//                  }
//              }
//              else if ((flag == "-v") || (flag == "--video"))
//              {
//                  if (!value.isEmpty())
//                  {
//                      parsedOptions.append(CmdArgument_st(CmdFlags_t::VideoFile, value));
//                      internalAppendSuccess = true;
//                  }
//              }
//              else if ((flag == "-a") || (flag == "--audio"))
//              {
//                  if (!value.isEmpty())
//                  {
//                      parsedOptions.append(CmdArgument_st(CmdFlags_t::AudioFile, value));
//                      internalAppendSuccess = true;
//                  }
//              }
//              else if ((flag == "-an") || (flag == "--annotation"))
//              {
//                  if (!value.isEmpty())
//                  {
//                      parsedOptions.append(CmdArgument_st(CmdFlags_t::AnnotationFile, value));
//                      internalAppendSuccess = true;
//                  }
//              }
//              else if ((flag == "-tr") || (flag == "--transcript"))
//              {
//                  if (!value.isEmpty())
//                  {
//                      parsedOptions.append(CmdArgument_st(CmdFlags_t::TranscriptionFile, value));
//                      internalAppendSuccess = true;
//                  }
//              }
//              else if ((flag == "-sm") || (flag == "--syncedManifests"))
//              {
//                  if (!value.isEmpty())
//                  {
//                      parsedOptions.append(CmdArgument_st(CmdFlags_t::SyncedManifestsFile, value));

//                      internalAppendSuccess = true;
//                  }
//              }
//              else {
//                  qWarning() << "Ignored unknown flag: " << flag;
//                  validFlag = false;
//              }

//              if (validFlag && !internalAppendSuccess)
//              {
//                  qWarning() << "Error: Known flag" << flag << "is missing a value specification (found another flag or end of line).";
// #ifdef QT_DEBUG
//                  LogUtils::safeLog(QString("Utils::commandLineParser(...) - Error: Flag: %1 is missing a value specifier.")
//                                        .arg(flag),
//                                    __FILE__,
//                                    __LINE__,
//                                    Q_FUNC_INFO);
// #endif
//              }

// #ifdef QT_DEBUG
//              if (internalAppendSuccess)
//              {
//                  LogUtils::safeLog(QString("Utils::commandLineParser(...) - Argument parsed for flag [%1] is: %2")
//                                        .arg(flag, value),
//                                        __FILE__,
//                                        __LINE__,
//                                        Q_FUNC_INFO);
//              }
// #endif
//          }
//          else {
//              qWarning() << "Warning: Loose argument sitting outside flag associations ignored:" << currentArg;

//              idx++;
//          }
//      }

// #ifdef QT_DEBUG
//      LogUtils::safeLog(QString("Utils::commandLineParser(...) - Leaving!"),
//                        __FILE__,
//                        __LINE__,
//                        Q_FUNC_INFO);
// #endif

//      return parsedOptions;
// }

CmdArgumentsVec ExtractorUtils::commandLineParser(const QStringList &args)
{
    QCommandLineParser parser;
    parser.setApplicationDescription("Event Extractor Tool");
    parser.addHelpOption(); // Automatically handles -h and --help
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);

    // Define options
    QCommandLineOption pipelineModeOpt(QStringList()                << "m"          << "mode", "Operation mode (e.g., transcode_only, transcribe_only...)", "mode");
    QCommandLineOption audioFormatOpt(QStringList()                 << "af"         << "audioformat", "Audio output format", "filepath");
    QCommandLineOption slowdownFactorsOpt(QStringList()             << "sf"         << "slowdownFactors", "Slowdown factors", "decimal values");
    QCommandLineOption clipDurationOpt(QStringList()                << "cd"         << "clipDuration", "Clip duration", "decimal value");
    QCommandLineOption confidenceThresholdOpt(QStringList()         << "ct"         << "confidenceThreshold", "Confidence threshold", "decimal value");
    QCommandLineOption transcriptionConverterTypeOpt(QStringList()  << "convType"   << "transcriptionConverterType", "Transcription ConverterType", "type");

    QCommandLineOption aiModelTypeOpt(QStringList()                 << "aim"        << "aiModelType", "AI Model Type", "type");
    QCommandLineOption s2tModelOpt(QStringList()                    << "s2tm"       << "speec2textModel", "Speech2Text Model Type", "type");
    QCommandLineOption sysProfileOpt(QStringList()                  << "sp"         << "sysProfile", "System Profile Type", "type");
    QCommandLineOption taskAnalysisOpt(QStringList()                << "ta"         << "taskAnalysis", "Task Analysis Type", "type");
    QCommandLineOption videoSegModelOpt(QStringList()               << "vsm"        << "videoSegModel", "Video Segmeation Model", "type");

    QCommandLineOption searchTypeOpt(QStringList()                  << "sType"      << "search_type", "Search type", "type");
    QCommandLineOption searchedTermsOpt(QStringList()               << "st"         << "searched_terms", "Searched terms", "string list");

    QCommandLineOption targetDirOpt(QStringList()                   << "td"         << "target_dir", "Path to target directory", "dirpath");

    QCommandLineOption annotationFileOpt(QStringList()              << "an"         << "annotation", "Path to annotation file", "filepath");
    QCommandLineOption audioFileOpt(QStringList()                   << "a"          << "audio", "Path to audio file", "filepath");
    QCommandLineOption inputFileOpt(QStringList()                   << "i"          << "input", "Path to video+audio input file", "filepath");
    QCommandLineOption syncedManifestsFileOpt(QStringList()         << "sm"         << "syncedManifests", "Path to synchronized manifests file", "filepath");
    QCommandLineOption transcriptionFileOpt(QStringList()           << "tr"         << "transcription", "Path to transcription file", "filepath");
    QCommandLineOption videoFileOpt(QStringList()                   << "v"          << "video", "Path to video file", "filepath");

    QCommandLineOption experimentOpt(QStringList()                  << "exp"        << "experiment", "Experiment mode", "mode");

    QCommandLineOption baselineMarkdownFileOpt(QStringList()        << "bmd"        << "baselineMarkdown", "Baseline Markdown File", "filepath");

    QCommandLineOption governedMarkdownFileOpt(QStringList()        << "gmd"        << "governedMarkdown", "Baseline Markdown File", "filepath");
    QCommandLineOption groundTruthFileOpt(QStringList()             << "gt"         << "groundTruth", "Ground Truth CSV File", "filepath");

    QCommandLineOption temporalToleranceOpt(QStringList()           << "tt"         << "temporalTolerance", "Temporal Tolerance", "<int value>");

    QCommandLineOption testOpt(QStringList()                        << "t"          << "test", "Test option", "test mode");

    parser.addOption(pipelineModeOpt);
    parser.addOption(audioFormatOpt);
    parser.addOption(slowdownFactorsOpt);
    parser.addOption(clipDurationOpt);
    parser.addOption(confidenceThresholdOpt);
    parser.addOption(transcriptionConverterTypeOpt);
    parser.addOption(aiModelTypeOpt);
    parser.addOption(s2tModelOpt);
    parser.addOption(sysProfileOpt);
    parser.addOption(taskAnalysisOpt);
    parser.addOption(videoSegModelOpt);

    parser.addOption(searchTypeOpt);
    parser.addOption(searchedTermsOpt);

    parser.addOption(targetDirOpt);

    parser.addOption(annotationFileOpt);
    parser.addOption(audioFileOpt);
    parser.addOption(inputFileOpt);
    parser.addOption(syncedManifestsFileOpt);
    parser.addOption(transcriptionFileOpt);
    parser.addOption(videoFileOpt);

    parser.addOption(experimentOpt);

    parser.addOption(baselineMarkdownFileOpt);
    parser.addOption(governedMarkdownFileOpt);
    parser.addOption(groundTruthFileOpt);

    parser.addOption(temporalToleranceOpt);

    parser.addOption(testOpt);

    // This single line replaces all the while-loop string manipulation
    parser.parse(args);

    // 1. Handle Unknown Flags
    for (const QString &unknown : parser.unknownOptionNames())
    {
        qWarning() << "Ignored unknown flag: " << "-" + unknown;
    }

    // 2. Handle Loose Arguments (Not tied to a flag)
    for (const QString &looseArg : parser.positionalArguments())
    {
        qWarning() << "Warning: Loose argument sitting outside flag associations ignored:" << looseArg;
    }

    CmdArgumentsVec parsedOptions;

    // Lambda to safely extract, optionally clean, append, and log the values
    auto processOption = [&](const QCommandLineOption &opt,
                             CmdFlags_t flagType,
                             const QString &logName,
                             bool cleanBrackets = false,
                             bool cleanSpaces = false) {

        if (parser.isSet(opt))
        {
            QString value = parser.value(opt);

            // Missing Value Guard
            if (value.isEmpty())
            {
                qWarning() << "Error: Known flag" << logName << "is missing a value specification (found another flag or end of line).";

#ifdef QT_DEBUG
                LogUtils::safeLog(QString("Utils::commandLineParser(...) - Error: Flag: %1 is missing a value specifier.")
                                      .arg(logName),
                                  __FILE__,
                                  __LINE__,
                                  Q_FUNC_INFO);
#endif
                return;
            }

            // String Cleaning Execution
            if (cleanBrackets)
            {
                value.remove('[').remove(']').remove('\'').remove('\"');
            }
            if (cleanSpaces)
            {
                value.remove(' ');
            }

            parsedOptions.append(CmdArgument_st(flagType, value));

#ifdef QT_DEBUG
            LogUtils::safeLog(QString("Utils::commandLineParser(...) - Argument parsed for flag [%1] is: %2")
                                  .arg(logName, value),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
#endif
        }
    };

    // 3. Extract and map all defined options
    processOption(pipelineModeOpt,                  CmdFlags_t::PipelineMode, "-m");
    processOption(audioFormatOpt,                   CmdFlags_t::OutputFormat, "-af");
    // Slowdown factors: Cleans brackets, quotes, AND spaces
    processOption(slowdownFactorsOpt,               CmdFlags_t::SlowdownFactors, "-sf", true, true);
    processOption(clipDurationOpt,                  CmdFlags_t::ClipDuration, "-cd");
    processOption(confidenceThresholdOpt,           CmdFlags_t::ConfidenceThreshould, "-ct");
    processOption(transcriptionConverterTypeOpt,    CmdFlags_t::TranscriptionConverter, "-convType");
    processOption(aiModelTypeOpt,                   CmdFlags_t::AIModelType, "-aim");
    processOption(s2tModelOpt,                      CmdFlags_t::S2TModelType, "-s2tm");
    processOption(sysProfileOpt,                    CmdFlags_t::SystemProfile, "-sp");
    processOption(taskAnalysisOpt,                  CmdFlags_t::TaskAnalysis, "-ta");
    processOption(videoSegModelOpt,                 CmdFlags_t::VideoSegmentationModel, "-vgm");

    processOption(searchTypeOpt,                    CmdFlags_t::SearchType, "-sType");
    // Searched terms: Cleans brackets and quotes, but KEEPS spaces
    processOption(searchedTermsOpt,                 CmdFlags_t::SearchedTerms, "-st", true, false);

    processOption(targetDirOpt,                     CmdFlags_t::TargetDir, "-td");

    processOption(annotationFileOpt,                CmdFlags_t::AnnotationFile, "-an");
    processOption(audioFileOpt,                     CmdFlags_t::AudioFile, "-a");
    processOption(inputFileOpt,                     CmdFlags_t::InputFile, "-i");
    processOption(syncedManifestsFileOpt,           CmdFlags_t::SyncedManifestsFile, "-sm");
    processOption(transcriptionFileOpt,             CmdFlags_t::TranscriptionFile, "-tr");
    processOption(videoFileOpt,                     CmdFlags_t::VideoFile, "-v");

    processOption(experimentOpt,                    CmdFlags_t::ExperimentMode, "-exp");

    processOption(baselineMarkdownFileOpt,          CmdFlags_t::BaselineMarkdown, "-bmd");

    processOption(governedMarkdownFileOpt,          CmdFlags_t::GovernedMarkdown, "-gmd");

    processOption(groundTruthFileOpt,               CmdFlags_t::GroundTruth, "-gt");

    processOption(temporalToleranceOpt,             CmdFlags_t::TemporalTolerance, "-tt");

    processOption(testOpt,                          CmdFlags_t::Test, "-t");

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("Utils::commandLineParser(...) - Leaving!"),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    return parsedOptions;
}

qint32 ExtractorUtils::indexOfCmdArg(const CmdFlags_t opt, const CmdArgumentsVec & cmdArgsVec)
{
    for (qint32 idx = 0; idx < cmdArgsVec.size(); idx++)
    {
        if (cmdArgsVec[idx].flag == opt)
        {
            return (idx);
        }
    }

    return (-1);
}

/**
 * Tries to verify if a file is JSON, handling BOMs and legacy encodings.
 */
bool ExtractorUtils::isJsonFile(const QString &filePath)
{
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly))
    {
#ifdef QT_DEBUG
         LogUtils::safeLog(QString("Utils::isJsonFileAutoDetect(...) - Could not open file: %1!")
                           .arg(filePath),
                           __FILE__,
                           __LINE__,
                           Q_FUNC_INFO);
#endif
        qWarning() << "Cannot open file:" << filePath;
        return false;
    }

    QByteArray rawData = file.readAll();
    file.close();

    if (rawData.isEmpty())
    {
#ifdef QT_DEBUG
         LogUtils::safeLog(QString("Utils::isJsonFile(...) - File is empty."),
                           __FILE__,
                           __LINE__,
                           Q_FUNC_INFO);
#endif
        qWarning() << "File is empty.";
        return false;
    }

    QString unicodeString;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // --- STEP 1: Check for Byte Order Mark (BOM) ---
    // QStringConverter checks the first few bytes for UTF-8/16/32 BOMs.
    auto detectedEncoding = QStringConverter::encodingForData(rawData);

    if (detectedEncoding.has_value())
    {
        // A BOM was found (e.g., UTF-16LE). We MUST use this encoding.

#ifdef QT_DEBUG
         LogUtils::safeLog(QString("Utils::isJsonFile(...) - BOM detected. Decoding..."),
                           __FILE__,
                           __LINE__,
                           Q_FUNC_INFO);
#endif

        qDebug() << "BOM detected. Decoding...";
        QStringDecoder decoder(detectedEncoding.value());
        QString unicodeString = decoder(rawData);
    } else {
        // No BOM found. Assume UTF-8 first.
        QStringDecoder utf8Decoder(QStringDecoder::Utf8);
        unicodeString = utf8Decoder(rawData);

        // If UTF-8 produced errors (invalid bytes), try System Locale fallback
        if (utf8Decoder.hasError())
        {
#ifdef QT_DEBUG
             LogUtils::safeLog(QString("Utils::isJsonFile(...) - UTF-8 decoding failed. Falling back to System Locale."),
                               __FILE__,
                               __LINE__,
                               Q_FUNC_INFO);
#endif
             qWarning() << "UTF-8 decoding failed. Falling back to System Locale.";
             QStringDecoder systemDecoder(QStringDecoder::System);
             unicodeString = systemDecoder(rawData);
        }
    }
#else

    // Step A: Detect BOM or Default to UTF-8
    // codecForUtfText looks for BOM. If none, it uses the 2nd arg (UTF-8).
    QTextCodec *utf8Codec = QTextCodec::codecForName("UTF-8");
    QTextCodec *codec = QTextCodec::codecForUtfText(rawData, utf8Codec);

    // Step B: Decode
    // We create a generic state to check for invalid characters
    QTextCodec::ConverterState state;
    unicodeString = codec->toUnicode(rawData.constData(), rawData.size(), &state);

    // Step C: Fallback
    // If we had invalid chars (state.invalidChars > 0) AND we were using UTF-8,
    // try the System Locale (legacy ANSI files).
    if (state.invalidChars > 0 && codec == utf8Codec)
    {
#ifdef QT_DEBUG
         LogUtils::safeLog(QString("Utils::isJsonFile(...) - UTF-8 decoding failed (invalid chars). Falling back to System Locale."),
                           __FILE__,
                           __LINE__,
                           Q_FUNC_INFO);
#endif

        qWarning() << "UTF-8 decoding failed (invalid chars). Falling back to System Locale.";

        QTextCodec *systemCodec = QTextCodec::codecForLocale();

        if (systemCodec)
        {
            unicodeString = systemCodec->toUnicode(rawData);
        }
    }

#endif

    // QJsonDocument::fromJson requires UTF-8 bytes.
    // We convert our clean Unicode string back to UTF-8.
    QByteArray utf8ForJson = unicodeString.toUtf8();

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(utf8ForJson, &err);

    if (err.error != QJsonParseError::NoError)
    {
#ifdef QT_DEBUG
         LogUtils::safeLog(QString("Utils::isJsonFile(...) - JSON Parse Error: %1 at offset: %2")
                           .arg(err.errorString())
                           .arg(err.offset),
                           __FILE__,
                           __LINE__,
                           Q_FUNC_INFO);
#endif
        qWarning() << "JSON Parse Error:" << err.errorString()
                   << "at offset:" << err.offset;

        return false;
    }

    // Optional: Ensure it is an Object or Array (not just a primitive value like "true")
    if (!doc.isObject() && !doc.isArray())
    {
#ifdef QT_DEBUG
         LogUtils::safeLog(QString("Utils::isJsonFile(...) - Valid JSON structure, but root is not an Object or Array."),
                           __FILE__,
                           __LINE__,
                           Q_FUNC_INFO);
#endif

        qWarning() << "Valid JSON structure, but root is not an Object or Array.";
        // return false; // Uncomment if strict format is required
    }

    return true;
}

QString ExtractorUtils::repairJson(QString str)
{
    str = str.trimmed();
    if (str.isEmpty()) return str;

    enum ContainerType { Object, Array };
    enum ObjectState { ExpectKey, AfterKey, ExpectValue, AfterValue };
    enum ArrayState { ExpectElem, AfterElem };

    struct Frame {
        ContainerType type;
        qint32 state; // Holds ObjectState or ArrayState
    };

    QVector<Frame> stack;
    bool inString = false;
    bool isEscaped = false;

    // Pass 1: Parse structure & track context state
    for (qint32 i = 0; i < str.length(); ++i)
    {
        QChar c = str[i];

        if (isEscaped)
        {
            isEscaped = false;
            continue;
        }

        if (c == '\\' && inString)
        {
            isEscaped = true;
            continue;
        }

        if (c == '"')
        {
            inString = !inString;

            if (!stack.isEmpty())
            {
                Frame &top = stack.last();

                if (top.type == Object)
                {
                    if (!inString) // Quote was just closed
                    {
                        if (top.state == ExpectKey) top.state = AfterKey;
                        else if (top.state == ExpectValue) top.state = AfterValue;
                    }
                }
                else // Array
                {
                    if (!inString) top.state = AfterElem;
                }
            }

            continue;
        }

        if (inString) continue;

        if (c.isSpace()) continue;

        if (c == '{') stack.push_back({Object, ExpectKey});
        else if (c == '[') stack.push_back({Array, ExpectElem});
        else if (c == '}')
        {
            if (!stack.isEmpty() && stack.last().type == Object) stack.pop_back();

            if (!stack.isEmpty())
            {
                Frame &top = stack.last();

                if (top.type == Object && top.state == ExpectValue) top.state = AfterValue;
                else if (top.type == Array) top.state = AfterElem;
            }
        }
        else if (c ==']')
        {
            if (!stack.isEmpty() && stack.last().type == Array) stack.pop_back();

            if (!stack.isEmpty())
            {
                Frame &top = stack.last();

                if (top.type == Object && top.state == ExpectValue) top.state = AfterValue;
                else if (top.type == Array) top.state = AfterElem;
            }
        }
        else if (c == ':')
        {
            if (!stack.isEmpty() && stack.last().type == Object) stack.last().state = ExpectValue;
        }
        else if (c == ',')
        {
            if (!stack.isEmpty())
            {
                if (stack.last().type == Object) stack.last().state = ExpectKey;
                else stack.last().state = ExpectElem;
            }
        }
    }

    // Step 1: Close dangling string literal. Close any strings that were cut off mid-sentence
    if (inString)
    {
        str += "\"";

        if (!stack.isEmpty())
        {
            Frame &top = stack.last();

            if (top.type == Object)
            {
                if (top.state == ExpectKey) top.state = AfterKey;
                else if (top.state == ExpectValue) top.state = AfterValue;
            }
            else
            {
                top.state = AfterElem;
            }
        }
    }

    str = str.trimmed();

    // Step 2: Fix missing value / name separators
    if (!stack.isEmpty())
    {
        Frame &top = stack.last();

        if (top.type == Object)
        {
            // Truncated after an object key: {"key" -> append": null"
            if (top.state == AfterKey)
            {
                str += ": null";
                top.state = AfterValue;
            }
            // Trucated right after colon: { "key": -> append "null"
            else if (top.state == ExpectValue)
            {
                if (str.endsWith(":")) str += " null";
                else if (str.endsWith(",")) str.chop(1);
            }
            // Truncated after comma: { "key": 1, -> remove trailing comma
            else if (top.state == ExpectKey)
            {
                if (str.endsWith(",")) str.chop(1);
            }
        }
        else // Array
        {
            if (str.endsWith(",")) str.chop(1);
        }
    }
    else
    {
        if (str.endsWith(",")) str.chop(1);
        else if (str.endsWith(": ")) str += " null";
    }

    str = str.trimmed();

    if (str.endsWith(",")) str.chop(1);

    // Step 3: Unwind and close remaining open containers
    for (qint32 i = stack.size() -1; i >= 0; --i)
    {
        if (stack[i].type == Object) str += "}";
        else if (stack[i].type == Array) str += "]";
    }

    return str;
}

QString ExtractorUtils::stripMarkdown(const QString &trimmedText)
{
    QString cleanStr = trimmedText.trimmed();

    qint32 start = 0;

    if (cleanStr.startsWith("```json"))
    {
        start = cleanStr.indexOf("```json") + 7;

        cleanStr = cleanStr.mid(start).trimmed();
    }
    else if (cleanStr.startsWith("```"))
    {
        start = cleanStr.indexOf("```") + 3;

        cleanStr = cleanStr.mid(start).trimmed();
    }

    if (cleanStr.endsWith("```"))
    {
        cleanStr.chop(3);
        cleanStr = cleanStr.trimmed();
    }

    return cleanStr;
}

// Helper to convert "HH:MM:SS" to total seconds (double)
double ExtractorUtils::toSeconds(const QString& timestamp)
{
    QStringList parts = timestamp.split(':');

    // Reject formats that don't match mm:ss or HH:mm:ss
    if (parts.size() < 2 || parts.size() > 3) return 0.0;

    bool ok = false;
    qint32 hours = 0;
    qint32 mins = 0;
    double secs = 0.0;

    if (parts.size() == 3)
    {
        hours = parts[0].toInt(&ok);

        if (!ok) return 0.0;

        mins = parts[1].toInt(&ok);

        if (!ok) return 0.0;

        secs = parts[2].toDouble(&ok);

        if (!ok) return 0.0;

    } else { // parts.size() == 2 (mm:ss)

        mins = parts[0].toInt(&ok);

        if (!ok) return 0.0;

        secs = parts[1].toDouble(&ok);

        if (!ok) return 0.0;
    }

    return (hours * 3600.0) + (mins * 60.0) + secs;
}

QString ExtractorUtils::getAPIKey(const AIModels_t apiKey)
{
    // SECURITY: Get key from Environment Variable
    // In Qt Creator: Projects -> Run -> Environment -> Add "DEEPGRAM_API_KEY"
    QString key;
    key.clear();

    switch (apiKey)
    {
        case AIModels_t::UserDefined_API:
        {
            // TODO
            break;
        }

        case AIModels_t::Deepgram_API:
        {
            key = QProcessEnvironment::systemEnvironment().value("DEEPGRAM_API_KEY");
            break;
        }

        case AIModels_t::Deepseek_API:
        {
            key = QProcessEnvironment::systemEnvironment().value("DEEPSEEK_API_KEY");
            break;
        }

        // case AIModels_t::Google_Gemini_API_20Flash:
        case AIModels_t::Google_Gemini_API_20FlashLite:
        case AIModels_t::Google_Gemini_API_20FlashLite001:
        case AIModels_t::Google_Gemini_API_20Flash:
        case AIModels_t::Google_Gemini_API_20Flash001:
        case AIModels_t::Google_Gemini_API_25Flash:
        case AIModels_t::Google_Gemini_API_25FlashLite:
        case AIModels_t::Google_Gemini_API_25Pro:
        case AIModels_t::Google_Gemini_API_31FlashLite:
        case AIModels_t::Google_Gemini_API_31FlashLitePreview:
        case AIModels_t::Google_Gemini_API_FlashLiteLatest:
        case AIModels_t::Google_Gemini_API_FlashLatest:
        case AIModels_t::Google_Gemini_API_3FlashPreview:
        case AIModels_t::Google_Gemini_API_3ProPreview:
        case AIModels_t::Google_Gemini_API_31ProPreview:
        case AIModels_t::Google_Gemini_API_31ProPreviewCustomTools:
        case AIModels_t::Google_Gemini_API_ProLatest:
        {
            key = QProcessEnvironment::systemEnvironment().value("GOOGLE_GEMINI_API_KEY");
            break;
        }

        case AIModels_t::Google_HuggingFace_API:
        {
            key = QProcessEnvironment::systemEnvironment().value("GOOGLE_HUGGINGFACE_API_KEY");

            break;
        }

        case AIModels_t::GPT_4o_Mini_API:
        {
            key = QProcessEnvironment::systemEnvironment().value("GPT_4o_MINI_API_KEY");

            break;
        }

        case AIModels_t::OpenAI_Google_API:
        {
            key = QProcessEnvironment::systemEnvironment().value("GOOGLE_OPENAI_API_KEY");

            break;
        }

        case AIModels_t::OpenRouter_API_GoogleGemini2_5ProExperimental:
        case AIModels_t::OpenRouter_API_GoogleGemini2_0FlashExperimental:
        case AIModels_t::OpenRouter_API_GoogleGemma3_12B:
        case AIModels_t::OpenRouter_API_GoogleGemma3_27B:
        {
            key = QProcessEnvironment::systemEnvironment().value("GOOGLE_OPENROUTER_API_KEY");

            break;
        }

        case AIModels_t::OpenRouter_API_Deepseekv2:
        case AIModels_t::OpenRouter_API_Deepseekv3Base:
        case AIModels_t::OpenRouter_API_DeepSeekDeepSeekV3:
        case AIModels_t::OpenRouter_API_GPT_4o_Mini:
        case AIModels_t::OpenRouter_API_MetaLlama4Maverick:
        case AIModels_t::OpenRouter_API_MetaLlama4Scout:
        case AIModels_t::OpenRouter_API_MicrosoftMAIDSR1:
        case AIModels_t::OpenRouter_API_MistralMistralNemo:
        case AIModels_t::OpenRouter_API_Qwen3235BA22B:
        {
            key = QProcessEnvironment::systemEnvironment().value("OpenRouter_API_KEY");
            break;
        }

        case AIModels_t::None:
        default:
        {
            // TODO
        }
    }

    if (key.isEmpty())
    {
        qWarning() << "CRITICAL: Model API_KEY environment variable is empty.";
        // Fallback for Debugging ONLY (Remove in production)
        // key = "YOUR_DEBUG_KEY";
    }

    return key;
}

AIModels_t ExtractorUtils::getModelTypeFromString(const QString &val, const QHash<AIModels_t, AIModelConfig> &modelMap)
{
    for (auto it = modelMap.constBegin(); it != modelMap.constEnd(); ++it)
    {
        if (it.value().name == val)
        {
            return it.key();
        }
    }

    return AIModels_t::None; // Fallback default
}

QString ExtractorUtils::headerTemplate(const HeaderType_t headerType)
{
    switch (headerType)
    {
        case HeaderType_t::Authorization:
        {
            return *Constants::Q_QGS_URLHeaderAuthorizationTemplate_Str;
        }

        case HeaderType_t::GoogleAuthorisation:
        {
            return *Constants::Q_QGS_URLHeaderGoogleAuthorizationTemplate_Str;
        }

        case HeaderType_t::Content_Type:
        {
            return *Constants::Q_QGS_URLHeaderContentTypeTemplate_Str;
        }

        case HeaderType_t::HttpReferer:
        {
            return *Constants::Q_QGS_URLHeaderRefererTemplate_Str;
        }

        case HeaderType_t::X_Title:
        {
            return *Constants::Q_QGS_URLHeaderXTitleTemplate_Str;
        }
    }

    return (QString());
}

QString ExtractorUtils::authHeader(const QString &apiKey, AIModels_t modelCode)
{
    QString key;

    if ( ! apiKey.isEmpty() )
    {
        key = apiKey;

    } else if ((key = ExtractorUtils::getAPIKey(modelCode)).isEmpty())
    {
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("Utils::authHeader(...) - API key is empty!"),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        return QString();
    }

    QString authHeader;

#pragma clang diagnostic ignored "-Wswitch"
    switch (modelCode)
    {
        case AIModels_t::Google_Gemini_API_20FlashLite:
        case AIModels_t::Google_Gemini_API_20FlashLite001:
        case AIModels_t::Google_Gemini_API_20Flash:
        case AIModels_t::Google_Gemini_API_20Flash001:
        case AIModels_t::Google_Gemini_API_25Flash:
        case AIModels_t::Google_Gemini_API_25FlashLite:
        case AIModels_t::Google_Gemini_API_25Pro:
        case AIModels_t::Google_Gemini_API_31FlashLite:
        case AIModels_t::Google_Gemini_API_31FlashLitePreview:
        case AIModels_t::Google_Gemini_API_FlashLiteLatest:
        case AIModels_t::Google_Gemini_API_FlashLatest:
        case AIModels_t::Google_Gemini_API_3FlashPreview:
        case AIModels_t::Google_Gemini_API_3ProPreview:
        case AIModels_t::Google_Gemini_API_31ProPreview:
        case AIModels_t::Google_Gemini_API_31ProPreviewCustomTools:
        case AIModels_t::Google_Gemini_API_ProLatest:
        {
            authHeader = ExtractorUtils::headerTemplate(HeaderType_t::GoogleAuthorisation);
            break;
        }

        case AIModels_t::Google_HuggingFace_API:
        case AIModels_t::GPT_4o_Mini_API:
        case AIModels_t::OpenAI_Google_API:
        case AIModels_t::OpenRouter_API_FreeModel:
        case AIModels_t::OpenRouter_API_Deepseekv2:
        case AIModels_t::OpenRouter_API_Deepseekv3Base:
        case AIModels_t::OpenRouter_API_DeepSeekDeepSeekV3:
        case AIModels_t::OpenRouter_API_GoogleGemini2_0FlashExperimental:
        case AIModels_t::OpenRouter_API_GoogleGemini2_5ProExperimental:
        case AIModels_t::OpenRouter_API_GoogleGemma3_12B:
        case AIModels_t::OpenRouter_API_GoogleGemma3_27B:
        case AIModels_t::OpenRouter_API_GPT_4o_Mini:
        case AIModels_t::OpenRouter_API_MetaLlama4Maverick:
        case AIModels_t::OpenRouter_API_MetaLlama4Scout:
        case AIModels_t::OpenRouter_API_MicrosoftMAIDSR1:
        case AIModels_t::OpenRouter_API_MistralMistralNemo:
        case AIModels_t::OpenRouter_API_Qwen3235BA22B:
        {
            authHeader = ExtractorUtils::headerTemplate(HeaderType_t::Authorization);
            break;
        }

        case AIModels_t::UserDefined_API:
        case AIModels_t::None:
        {
            break;
        }
    }

    if ( ! authHeader.isEmpty() ) authHeader.replace("<KEY>", key);

    return authHeader;
}

void ExtractorUtils::presentMdAsJsonFriendly(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qWarning() << "Error: Could not open file: " << filePath << Qt::endl;
        return;
    }

    QString fileContent = file.readAll();
    file.close();

    // Master array to collect all individual match reports
    QJsonArray masterReportsArray;

    // Regex to capture each valid individual root JSON object block safely
    // It captures balanced blocks starting with {"candidates" until a closing '}' followed by space/newlines
    QRegularExpression rootObjectRegex("(\\{[\\s]*\"candidates\".*?\\n\\})",
                                       QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatchIterator it = rootObjectRegex.globalMatch(fileContent);

    // Regex to extract the raw JSON data block from the inner markdown code block markup: ```json ... ```
    QRegularExpression markdownCodeBlockRegex("```json\\s*(.*?)\\s*```",
                                              QRegularExpression::DotMatchesEverythingOption);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString jsonStr = match.captured(1);

        QJsonDocument rootDoc = QJsonDocument::fromJson(jsonStr.toUtf8());
        if (rootDoc.isNull() || !rootDoc.isObject()) {
            continue;
        }

        // Navigate the Gemini response structure: candidates -> content -> parts -> text
        QJsonArray candidates = rootDoc.object().value("candidates").toArray();
        for (const QJsonValue &candValue : qAsConst(candidates)) {
            QJsonObject content = candValue.toObject().value("content").toObject();
            QJsonArray parts = content.value("parts").toArray();

            for (const QJsonValue &partValue : qAsConst(parts)) {
                QString rawText = partValue.toObject().value("text").toString();
                if (rawText.isEmpty()) continue;

                // Check if the inner text contains an embedded markdown code block
                QRegularExpressionMatch codeBlockMatch = markdownCodeBlockRegex.match(rawText);
                QString innerJsonCandidate = codeBlockMatch.hasMatch() ? codeBlockMatch.captured(1) : rawText;

                // Attempt to parse the extracted string into a proper JSON Object
                QJsonDocument reportDoc = QJsonDocument::fromJson(innerJsonCandidate.toUtf8());
                if (!reportDoc.isNull() && reportDoc.isObject()) {
                    masterReportsArray.append(reportDoc.object());
                } else {
                    // Fallback: If it's plain text instead of JSON, keep it readable under a descriptive property
                    QJsonObject textFallback;
                    textFallback["type"] = "plain_text_commentary";
                    textFallback["content"] = rawText.trimmed();
                    masterReportsArray.append(textFallback);
                }
            }
        }
    }

    // Wrap the results into a clean unified final document
    QJsonObject cleanResultObject;
    cleanResultObject["total_reports_found"] = masterReportsArray.size();
    cleanResultObject["match_reports"] = masterReportsArray;

    QJsonDocument finalJsonDoc(cleanResultObject);

    // Output the formatted JSON to standard output
    qDebug().noquote() << finalJsonDoc.toJson(QJsonDocument::Indented);
}

QString ExtractorUtils::extractGeminiPayloadText(const QString &rawText)
{
    QJsonParseError envelopeError;
    QJsonDocument initialDoc = QJsonDocument::fromJson(rawText.toUtf8(), &envelopeError);

    if (!initialDoc.isNull() && initialDoc.isObject())
    {
        QJsonObject envelope = initialDoc.object();

        if (envelope.contains(*Constants::Q_QGS_KEY_CANDIDATES))
        {
            QJsonArray candidates = envelope[*Constants::Q_QGS_KEY_CANDIDATES].toArray();

            if (!candidates.isEmpty())
            {
                QJsonObject firstCand = candidates[0].toObject();
                QJsonObject contentObj = firstCand[*Constants::Q_QGS_KEY_CONTENT].toObject();
                QJsonArray parts = contentObj[*Constants::Q_QGS_KEY_PARTS].toArray();

                if (!parts.isEmpty())
                {
                    return parts[0].toObject()[*Constants::Q_QGS_KEY_TEXT].toString();
                }
            }
        }
    }

    return rawText; // Fallback to raw text if no valid envelope structure exists
}


void ExtractorUtils::parseCompleteGeminiMarkdownAnswer(const QString &filePath, const QString &outputFilePath)
{
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qWarning() << "Error: Could not open file: " << filePath << Qt::endl;
        return;
    }

    QString fileContent = file.readAll();
    file.close();

    // Master array to collect all parsed model report items
    QJsonArray masterReportsArray;

    // Regex to locate individual root JSON structures in the raw file stream safely
    QRegularExpression rootObjectRegex("(\\{[\\s]*\"candidates\".*?\\n\\})",
                                       QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatchIterator it = rootObjectRegex.globalMatch(fileContent);

    // Regex to extract pure JSON payload out of markdown markup strings (```json ... ```)
    QRegularExpression markdownCodeBlockRegex("```json\\s*(.*?)\\s*```",
                                              QRegularExpression::DotMatchesEverythingOption);

    while (it.hasNext())
    {
        QRegularExpressionMatch match = it.next();
        QString jsonStr = match.captured(1);

        QJsonDocument rootDoc = QJsonDocument::fromJson(jsonStr.toUtf8());

        if (rootDoc.isNull() || !rootDoc.isObject())
        {
            continue;
        }

        QJsonObject rootObj = rootDoc.object();
        QJsonArray candidates = rootObj.value("candidates").toArray();

        for (const QJsonValue &candValue : qAsConst(candidates))
        {
            QJsonObject candObj = candValue.toObject();

            // Extract outer structural tracking properties
            QString finishReason = candObj.value("finishReason").toString();
            int index = candObj.value("index").toInt();

            QJsonObject contentObj = candObj.value("content").toObject();
            QString role = contentObj.value("role").toString();
            QJsonArray parts = contentObj.value("parts").toArray();

            for (const QJsonValue &partValue : qAsConst(parts))
            {
                QString rawText = partValue.toObject().value("text").toString();
                if (rawText.isEmpty()) continue;

                // Extract pure JSON data string out of the markdown markdown wrap
                QRegularExpressionMatch codeBlockMatch = markdownCodeBlockRegex.match(rawText);
                QString innerJsonStr = codeBlockMatch.hasMatch() ? codeBlockMatch.captured(1) : rawText;

                QJsonDocument innerDoc = QJsonDocument::fromJson(innerJsonStr.toUtf8());

                if (innerDoc.isNull() || !innerDoc.isObject())
                {
                    continue;
                }

                QJsonObject innerObj = innerDoc.object();

                // Construct the comprehensive consolidated schema object model
                QJsonObject processedReportObj;

                // Safe handling and structuring of 'vtt_verification' parameters
                QJsonObject videoClipInfo = innerObj.value("video_clip_info").toObject();

                if (innerObj.contains("vtt_verification"))
                {
                    QJsonObject vttVerif = innerObj.value("vtt_verification").toObject();

                    // Normalize event mapping variation properties gracefully
                    if (vttVerif.contains("confirmed_events") && !vttVerif.contains("confirmed")) {
                        vttVerif["confirmed"] = vttVerif.value("confirmed_events");
                    }
                    if (vttVerif.contains("rejected_events") && !vttVerif.contains("rejected")) {
                        vttVerif["rejected"] = vttVerif.value("rejected_events");
                    }
                    videoClipInfo["vtt_verification"] = vttVerif;
                }

                // Map out structural sibling nodes cleanly matching model requirements
                processedReportObj["video_clip_info"] = videoClipInfo;
                processedReportObj["events"] = innerObj.value("events").toArray();
                processedReportObj["role"] = role.isEmpty() ? "model" : role;
                processedReportObj["finishReason"] = finishReason.isEmpty() ? "STOP" : finishReason;
                processedReportObj["index"] = index;
                processedReportObj["usageMetadata"] = rootObj.value("usageMetadata").toObject();
                processedReportObj["modelVersion"] = rootObj.value("modelVersion").toString();
                processedReportObj["responseId"] = rootObj.value("responseId").toString();

                masterReportsArray.append(processedReportObj);
            }
        }
    }

    // Wrap final arrays under an industry-standard root node
    QJsonObject masterResultObject;
    masterResultObject["total_parsed_reports"] = masterReportsArray.size();
    masterResultObject["ai_model_reports"] = masterReportsArray;

    QJsonDocument finalJsonDoc(masterResultObject);

    QFile outputFile(outputFilePath);
    // Open the file in WriteOnly mode (Truncate resets any prior contents)
    if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        qWarning() << "Error: Could not open output file for writing: " << outputFilePath;
        return;
    }

    // Convert document to beautifully indented UTF-8 bytes and write
    qint64 bytesWritten = outputFile.write(finalJsonDoc.toJson(QJsonDocument::Indented));
    outputFile.close();

    if (bytesWritten == -1)
    {
        qWarning() << "Error: Failed to write data to output file.";
    }
    else
    {
        qInfo() << "Successfully saved complete parsed structure to:" << outputFilePath;
    }

    // Print out structurally valid JSON using qDebug() completely unquoted
    qDebug().noquote() << finalJsonDoc.toJson(QJsonDocument::Indented);
}

void ExtractorUtils::mergeGeminiReportsToManifest(const QString &manifestJsonPath,
                                         const QString &geminiMdPath,
                                         const QString &outputPath)
{
    // 1. Read and parse the main Manifest JSON file
    QFile manifestFile(manifestJsonPath);
    if (!manifestFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Error: Could not open manifest file:" << manifestJsonPath;
        return;
    }
    QByteArray manifestData = manifestFile.readAll();
    manifestFile.close();

    QJsonDocument manifestDoc = QJsonDocument::fromJson(manifestData);
    if (manifestDoc.isNull() || !manifestDoc.isObject()) {
        qWarning() << "Error: Invalid manifest JSON format.";
        return;
    }
    QJsonObject manifestObj = manifestDoc.object();
    QJsonArray segmentsArray = manifestObj.value("segments").toArray();

    // 2. Read the Gemini Markdown file content
    QFile mdFile(geminiMdPath);
    if (!mdFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Error: Could not open Gemini Markdown file:" << geminiMdPath;
        return;
    }
    QString mdContent = mdFile.readAll();
    mdFile.close();

    // Regex to locate individual root JSON structures in the text stream
    QRegularExpression rootObjectRegex("(\\{[\\s]*\"candidates\".*?\\n\\})",
                                       QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatchIterator it = rootObjectRegex.globalMatch(mdContent);

    // Regex to clean markdown code blocks wrapped inside text fields
    QRegularExpression markdownCodeBlockRegex("```json\\s*(.*?)\\s*```",
                                              QRegularExpression::DotMatchesEverythingOption);

    // A lookup map matching 'clip_video_id' to an array of processed report objects
    QMap<QString, QJsonArray> reportsLookupMap;

    // 3. Process and extract all individual report blocks from the markdown file
    while (it.hasNext())
    {
        QRegularExpressionMatch match = it.next();
        QString jsonStr = match.captured(1);

        QJsonDocument rootDoc = QJsonDocument::fromJson(jsonStr.toUtf8());
        if (rootDoc.isNull() || !rootDoc.isObject())
        {
            continue;
        }

        QJsonObject rootObj = rootDoc.object();
        QJsonArray candidates = rootObj.value("candidates").toArray();

        for (const QJsonValue &candValue : qAsConst(candidates))
        {
            QJsonObject candObj = candValue.toObject();
            QString finishReason = candObj.value("finishReason").toString();
            int index = candObj.value("index").toInt();

            QJsonObject contentObj = candObj.value("content").toObject();
            QString role = contentObj.value("role").toString();
            QJsonArray parts = contentObj.value("parts").toArray();

            for (const QJsonValue &partValue : qAsConst(parts))
            {
                QString rawText = partValue.toObject().value("text").toString();
                if (rawText.isEmpty()) continue;

                // Extract pure JSON payload from markdown markup wrappers
                QRegularExpressionMatch codeBlockMatch = markdownCodeBlockRegex.match(rawText);
                QString innerJsonStr = codeBlockMatch.hasMatch() ? codeBlockMatch.captured(1) : rawText;

                QJsonDocument innerDoc = QJsonDocument::fromJson(innerJsonStr.toUtf8());
                if (innerDoc.isNull() || !innerDoc.isObject()) continue;

                QJsonObject innerObj = innerDoc.object();
                QJsonObject videoClipInfo = innerObj.value("video_clip_info").toObject();
                QString clipVideoId = videoClipInfo.value("clip_video_id").toString();

                if (clipVideoId.isEmpty()) continue;

                // Build the precise composite report element required by your schema model
                QJsonObject reportItemObj;

                // Align structure layout where 'vtt_verification' is cleanly consolidated
                QJsonObject targetVideoClipInfo = videoClipInfo;
                if (innerObj.contains("vtt_verification")) {
                    QJsonObject vttVerif = innerObj.value("vtt_verification").toObject();

                    // Normalize event array naming syntax to standard model fields
                    if (vttVerif.contains("confirmed_events") && !vttVerif.contains("confirmed")) {
                        vttVerif["confirmed"] = vttVerif.value("confirmed_events");
                    }

                    if (vttVerif.contains("rejected_events") && !vttVerif.contains("rejected")) {
                        vttVerif["rejected"] = vttVerif.value("rejected_events");
                    }
                    targetVideoClipInfo["vtt_verification"] = vttVerif;
                }

                reportItemObj["video_clip_info"] = targetVideoClipInfo;
                reportItemObj["events"] = innerObj.value("events").toArray();
                reportItemObj["role"] = role.isEmpty() ? "model" : role;
                reportItemObj["finishReason"] = finishReason.isEmpty() ? "STOP" : finishReason;
                reportItemObj["index"] = index;
                reportItemObj["usageMetadata"] = rootObj.value("usageMetadata").toObject();
                reportItemObj["modelVersion"] = rootObj.value("modelVersion").toString();
                reportItemObj["responseId"] = rootObj.value("responseId").toString();

                // Group reports dynamically by their video identifiers
                reportsLookupMap[clipVideoId].append(reportItemObj);
            }
        }
    }

    // 4. Match and merge records against the main segments tracking table
    QJsonArray updatedSegmentsArray;
    for (int i = 0; i < segmentsArray.size(); ++i)
    {
        QJsonObject segmentObj = segmentsArray.at(i).toObject();
        QString segmentId = segmentObj.value("segment_id").toString();

        // Inject the generated ai_model_reports array if a key equivalence match is confirmed
        if (reportsLookupMap.contains(segmentId))
        {
            segmentObj["ai_model_reports"] = reportsLookupMap.value(segmentId);
        }

        updatedSegmentsArray.append(segmentObj);
    }

    // Update main object payload tracking tree
    manifestObj["segments"] = updatedSegmentsArray;
    QJsonDocument outputDoc(manifestObj);

    // 5. Serialize and write the unified file output to disk
    QFile outputFile(outputPath);
    if (!outputFile.open(QIODevice::WriteOnly))
    {
        qWarning() << "Error: Could not open destination file path for serialization:" << outputPath;
        return;
    }

    outputFile.write(outputDoc.toJson(QJsonDocument::Indented));
    outputFile.close();

    qInfo() << "Successfully merged and saved file content structure to:" << outputPath;
}


/////////////////////////// Old functions

#include <iostream>

/**
 * Print MD file
 */
void parseExtractedJson_20260303(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << "Could not open file: " << filePath.toStdString() << std::endl;
        return;
    }

    QString fileContent = file.readAll();
    file.close();

    std::cout << "==========================================================" << std::endl;
    std::cout << "                SOCCER MATCH ANALYSIS REPORT              " << std::endl;
    std::cout << "==========================================================" << std::endl;

    // This regex looks for the start of the actual JSON content: {"candidates": ... }
    // It captures everything from the first '{' until it finds a '}' followed by a newline or end of string.
    QRegularExpression re("(\\{[\\s]*\"candidates\".*?\\n\\})", QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatchIterator i = re.globalMatch(fileContent);

    bool found = false;
    while (i.hasNext()) {
        found = true;
        QRegularExpressionMatch match = i.next();
        QString jsonStr = match.captured(1);

        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
        if (doc.isNull()) continue;

        QJsonArray candidates = doc.object().value("candidates").toArray();
        for (const QJsonValue &candValue : qAsConst(candidates)) {
            QJsonObject content = candValue.toObject().value("content").toObject();
            QJsonArray parts = content.value("parts").toArray();

            for (const QJsonValue &partValue : qAsConst(parts)) {
                QString analysisText = partValue.toObject().value("text").toString();
                if (!analysisText.isEmpty()) {
                    std::cout << analysisText.toStdString() << std::endl;
                    std::cout << "----------------------------------------------------------" << std::endl;
                }
            }
        }
    }

    if (!found)
    {
        std::cout << "No valid analysis blocks found. Check file encoding/format." << std::endl;
    }
}

void parseExtractedJson_20260304(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << "Could not open file: " << filePath.toStdString() << std::endl;
        return;
    }

    QString fileContent = file.readAll();
    file.close();

    std::cout << "==========================================================" << std::endl;
    std::cout << "                SOCCER MATCH ANALYSIS REPORT              " << std::endl;
    std::cout << "==========================================================" << std::endl;

    // Captures the JSON blocks from the Gemini API response
    QRegularExpression jsonRe(R"(\{\s*"candidates"\s*:.*?\n\})", QRegularExpression::DotMatchesEverythingOption);

    // Captures the timestamped block and EVERYTHING following it until the next block or end
    QRegularExpression eventRe(R"(\[(\d{2}:\d{2}-\d{2}:\d{2})\] – (.*?)\n(.*?)(?=\n\[|$))",
                               QRegularExpression::DotMatchesEverythingOption);

    QRegularExpressionMatchIterator jsonIt = jsonRe.globalMatch(fileContent);

    while (jsonIt.hasNext()) {
        QJsonDocument doc = QJsonDocument::fromJson(jsonIt.next().captured(0).toUtf8());
        if (doc.isNull()) continue;

        QJsonObject jObj = doc.object();
        QJsonArray parts = jObj["candidates"].toArray()[0].toObject()["content"].toObject()["parts"].toArray();

        for (const QJsonValue &part : qAsConst(parts))
        {
            jObj = part.toObject();
            QString analysisText = jObj["text"].toString();

            QRegularExpressionMatchIterator eventIt = eventRe.globalMatch(analysisText);
            while (eventIt.hasNext()) {
                QRegularExpressionMatch match = eventIt.next();

                QString timeRange = match.captured(1);
                QString eventType = match.captured(2);
                QString details   = match.captured(3).trimmed(); // This captures the description, players, and previous events

                std::cout << "[" << timeRange.toStdString() << "] " << eventType.toStdString() << std::endl;
                std::cout << details.toStdString() << std::endl;
                std::cout << "----------------------------------------------------------" << std::endl;
            }
        }
    }
}

void parseExtractedJson_20260305_01(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << "Could not open file: " << filePath.toStdString() << std::endl;
        return;
    }

    QString fileContent = file.readAll();
    file.close();

    std::cout << "==========================================================" << std::endl;
    std::cout << "                SOCCER MATCH ANALYSIS REPORT              " << std::endl;
    std::cout << "==========================================================" << std::endl;

    // 1. Robust JSON Block Extraction
    // Bypasses internal braces by explicitly matching from the start of the
    // object to the "responseId" key, which Gemini always places at the very end.
    QRegularExpression jsonRe(R"(\{\s*"candidates"[\s\S]*?"responseId"\s*:\s*"[^"]*"\s*\})",
                              QRegularExpression::DotMatchesEverythingOption);

    // 2. Robust Event Extraction
    // - ([\d:\-]+) catches any combination of numbers, colons, and hyphens (e.g., 00:09, 00:27:38-00:27:43).
    // - [-–] catches both standard hyphens and en-dashes.
    // - The positive lookahead guarantees it only stops at the NEXT timestamp or the end of the string.
    QRegularExpression eventRe(R"(\[([\d:\-]+)\]\s*[-–]\s*(.*?)\n(.*?)(?=\n\[[\d:\-]+\]\s*[-–]|$))",
                               QRegularExpression::DotMatchesEverythingOption);

    QRegularExpressionMatchIterator jsonIt = jsonRe.globalMatch(fileContent);

    qint32 eventCounter = 1;

    while (jsonIt.hasNext())
    {
        QJsonDocument doc = QJsonDocument::fromJson(jsonIt.next().captured(0).toUtf8());
        if (doc.isNull())
        {
            std::cerr << "[WARNING] Failed to parse a JSON block. Skipping." << std::endl;
            continue;
        }

        QJsonObject jObj = doc.object();
        QJsonArray candidates = jObj["candidates"].toArray();
        if (candidates.isEmpty()) continue;

        jObj = candidates[0].toObject();
        QJsonArray parts = jObj["content"].toObject()["parts"].toArray();

        for (const QJsonValue &part : qAsConst(parts))
        {
            jObj = part.toObject();
            QString analysisText = jObj["text"].toString();

            QRegularExpressionMatchIterator eventIt = eventRe.globalMatch(analysisText);

            while (eventIt.hasNext())
            {
                QRegularExpressionMatch match = eventIt.next();

                QString timeRange = match.captured(1).trimmed();
                QString eventType = match.captured(2).trimmed();
                QString details   = match.captured(3).trimmed();

                std::cout << "Event #" << eventCounter++ << " | [" << timeRange.toStdString() << "] " << eventType.toStdString() << std::endl;
                std::cout << details.toStdString() << std::endl;
                std::cout << "----------------------------------------------------------" << std::endl;
            }
        }
    }

    if (eventCounter == 1)
    {
        std::cout << "No events found. Please verify the file content." << std::endl;
    }
    else {
        std::cout << "Successfully parsed " << (eventCounter - 1) << " total events." << std::endl;
    }
}

void parseExtractedJson_20260305_02(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << "Could not open file: " << filePath.toStdString() << std::endl;
        return;
    }

    QString fileContent = file.readAll();
    file.close();

    std::cout << "==========================================================" << std::endl;
    std::cout << "                SOCCER MATCH ANALYSIS REPORT              " << std::endl;
    std::cout << "==========================================================" << std::endl;

    // 1. Robust JSON Block Extraction
    QRegularExpression jsonRe(R"(\{\s*"candidates"[\s\S]*?"responseId"\s*:\s*"[^"]*"\s*\})",
                              QRegularExpression::DotMatchesEverythingOption);

    // 2. Event Block Extraction (Captures Timestamp, Title, and Raw Details Body)
    QRegularExpression eventRe(R"(\[([\d:\-]+)\]\s*[-–]\s*(.*?)\n(.*?)(?=\n\[[\d:\-]+\]\s*[-–]|$))",
                               QRegularExpression::DotMatchesEverythingOption);

    // 3. Dynamic Key-Value Extraction (The "Deep Parse")
    // Matches "* Key: Value" and safely captures multi-line values until the next "* " or end of string.
    QRegularExpression attributeRe(R"(\*\s*([^:]+):\s*(.*?)(?=\n\s*\*|$))",
                                   QRegularExpression::DotMatchesEverythingOption);

    QRegularExpressionMatchIterator jsonIt = jsonRe.globalMatch(fileContent);
    qint32 eventCounter = 1;

    while (jsonIt.hasNext())
    {
        QJsonDocument doc = QJsonDocument::fromJson(jsonIt.next().captured(0).toUtf8());
        if (doc.isNull()) continue;

        QJsonObject jObj = doc.object();
        QJsonArray candidates = jObj["candidates"].toArray();
        if (candidates.isEmpty()) continue;

        jObj = candidates[0].toObject();
        QJsonArray parts = jObj["content"].toObject()["parts"].toArray();

        for (const QJsonValue &part : qAsConst(parts))
        {
            jObj = part.toObject();
            QString analysisText = jObj["text"].toString();

            QRegularExpressionMatchIterator eventIt = eventRe.globalMatch(analysisText);

            while (eventIt.hasNext())
            {
                QRegularExpressionMatch match = eventIt.next();

                QString timeRange  = match.captured(1).trimmed();
                QString eventTitle = match.captured(2).trimmed();
                QString rawDetails = match.captured(3).trimmed();

                std::cout << "Event #" << eventCounter++ << std::endl;
                std::cout << "  [Time]  : " << timeRange.toStdString() << std::endl;
                std::cout << "  [Title] : " << eventTitle.toStdString() << std::endl;

                // Parse the specific attributes out of the raw details
                QRegularExpressionMatchIterator attrIt = attributeRe.globalMatch(rawDetails);

                // Optional: You could store these inside a QMap<QString, QString> here!
                // QMap<QString, QString> eventData;

                while (attrIt.hasNext())
                {
                    QRegularExpressionMatch attrMatch = attrIt.next();

                    QString key = attrMatch.captured(1).trimmed();
                    QString value = attrMatch.captured(2).trimmed();

                    // Formatting fix: If the value has line breaks (like the Causal Event Chain),
                    // indent the subsequent lines to align nicely in the console.
                    value.replace("\n", "\n            ");

                    // eventData.insert(key, value); // Store it if building a UI

                    // Pad the key with spaces so the colons align perfectly in the console
                    QString paddedKey = key.leftJustified(15, ' ');
                    std::cout << "   -> " << paddedKey.toStdString() << ": " << value.toStdString() << std::endl;
                }

                std::cout << "----------------------------------------------------------" << std::endl;
            }
        }
    }

    if (eventCounter == 1) {
        std::cout << "No events found. Please verify the file content." << std::endl;
    } else {
        std::cout << "Successfully parsed and mapped " << (eventCounter - 1) << " total events." << std::endl;
    }
}

void parseExtractedJson_20260305_03(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << "Could not open file: " << filePath.toStdString() << std::endl;
        return;
    }

    QString fileContent = file.readAll();
    file.close();

    std::cout << "=========================================================================" << std::endl;
    std::cout << "                      SOCCER MATCH ANALYSIS REPORT                       " << std::endl;
    std::cout << "=========================================================================" << std::endl;

    // 1. Robust JSON Block Extraction
    // Bypasses internal braces by matching up to the "responseId" key.
    QRegularExpression jsonRe(R"(\{\s*"candidates"[\s\S]*?"responseId"\s*:\s*"[^"]*"\s*\})",
                              QRegularExpression::DotMatchesEverythingOption);

    // 2. Event Block Extraction
    // - [\d\s:\-]+ safely captures timestamps with or without spaces (e.g., "00:00-00:10" or "00:00 - 00:21")
    // - [-–—] catches standard hyphens, en-dashes, and em-dashes.
    QRegularExpression eventRe(R"(\[([\d\s:\-]+)\]\s*[-–—]\s*([^\n]*)\n(.*?)(?=\n\s*\[[\d\s:\-]+\]\s*[-–—]|$))",
                               QRegularExpression::DotMatchesEverythingOption);

    // 3. Dynamic Key-Value Extraction
    // - \*\s* safely handles bullets with or without trailing spaces (e.g., "*Type:" vs "* Type:")
    QRegularExpression attributeRe(R"(\*\s*([^:]+):\s*(.*?)(?=\n\s*\*|$))",
                                   QRegularExpression::DotMatchesEverythingOption);

    QRegularExpressionMatchIterator jsonIt = jsonRe.globalMatch(fileContent);
    qint32 eventCounter = 1;

    while (jsonIt.hasNext())
    {
        QJsonDocument doc = QJsonDocument::fromJson(jsonIt.next().captured(0).toUtf8());
        if (doc.isNull()) continue;

        QJsonObject jObj = doc.object();
        QJsonArray candidates = jObj["candidates"].toArray();
        if (candidates.isEmpty()) continue;

        jObj = candidates[0].toObject();
        QJsonArray parts = jObj["content"].toObject()["parts"].toArray();

        for (const QJsonValue &part : qAsConst(parts))
        {
            jObj = part.toObject();
            QString analysisText = jObj["text"].toString();

            QRegularExpressionMatchIterator eventIt = eventRe.globalMatch(analysisText);

            while (eventIt.hasNext())
            {
                QRegularExpressionMatch match = eventIt.next();

                QString timeRange  = match.captured(1).trimmed();
                QString eventTitle = match.captured(2).trimmed();
                QString rawDetails = match.captured(3).trimmed();

                // --- CLEANUP: Remove trailing conversational text from the AI ---
                // If the block ends with conversational text (e.g., "That concludes my analysis.")
                // separated by a double newline, we strip it so it isn't parsed as an attribute value.
                int lastDoubleNewline = rawDetails.lastIndexOf("\n\n");

                if (lastDoubleNewline != -1)
                {
                    QString trailingText = rawDetails.mid(lastDoubleNewline).trimmed();

                    if (!trailingText.startsWith("*") && !trailingText.startsWith("["))
                    {
                        rawDetails = rawDetails.left(lastDoubleNewline).trimmed();
                    }
                }

                std::cout << "Event #" << eventCounter++ << std::endl;
                std::cout << "  [Time]  : " << timeRange.toStdString() << std::endl;
                std::cout << "  [Title] : " << eventTitle.toStdString() << std::endl;

                // Parse the specific attributes out of the raw details
                QRegularExpressionMatchIterator attrIt = attributeRe.globalMatch(rawDetails);

                while (attrIt.hasNext())
                {
                    QRegularExpressionMatch attrMatch = attrIt.next();

                    QString key = attrMatch.captured(1).trimmed();
                    QString value = attrMatch.captured(2).trimmed();

                    // Formatting fix: If the value has line breaks (like the numbered Causal Event Chains),
                    // indent the subsequent lines to align perfectly under the colon.
                    value.replace("\n", "\n                                   ");

                    // Pad the key with spaces so the colons align perfectly in the console
                    QString paddedKey = key.leftJustified(25, ' ');
                    std::cout << "   -> " << paddedKey.toStdString() << ": " << value.toStdString() << std::endl;
                }

                std::cout << "-------------------------------------------------------------------------" << std::endl;
            }
        }
    }

    if (eventCounter == 1)
    {
        std::cout << "No events found. Please verify the file content." << std::endl;
    } else {
        std::cout << "\n[SUCCESS] Successfully parsed and deeply mapped " << (eventCounter - 1) << " total events." << std::endl;
    }
}

struct ExtractedEvent {
    QTime startTime;
    QTime endTime;
    QString type;
    QString description;
    QString rawBlock;

    // Operator for std::sort
    bool operator<(const ExtractedEvent& other) const {
        return startTime < other.startTime;
    }
};

void printUnifiedTimeline(const QList<ExtractedEvent> &timeline)
{
    std::cout << "\n==========================================================" << std::endl;
    std::cout << "             UNIFIED CHRONOLOGICAL TIMELINE               " << std::endl;
    std::cout << "==========================================================" << std::endl;

    for (const auto &ev : timeline) {
        std::cout << "[" << ev.startTime.toString("hh:mm:ss").toStdString() << " - "
                  << ev.endTime.toString("hh:mm:ss").toStdString() << "] "
                  << ev.type.toStdString() << std::endl;

        // Clean up the description (remove bullet points for the summary)
        QString cleanDesc = ev.description;
        cleanDesc.replace(QRegularExpression("^\\* Description: "), "");

        std::cout << "  " << cleanDesc.split('\n').first().toStdString() << std::endl;
        std::cout << "----------------------------------------------------------" << std::endl;
    }
}

void mergeAnalysisTimelines(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    QString fileContent = file.readAll();
    file.close();

    QList<ExtractedEvent> masterTimeline;

    // 1. Extract JSON blocks from the Gemini log
    QRegularExpression jsonRe(R"(\{\s*"candidates"\s*:.*?\n\})", QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatchIterator jsonIt = jsonRe.globalMatch(fileContent);

    // 2. Regex to find event patterns: [MM:SS-MM:SS] or [HH:MM:SS-HH:MM:SS] – TYPE\n* Description: ...
    QRegularExpression eventRe(R"(\[(\d{2,3}:\d{2}(?::\d{2})?)\s*[-–—]\s*(\d{2,3}:\d{2}(?::\d{2})?)\]\s*[-–—]\s*(.*?)\n(.*?)(?=\n\[|\nEnd|\n\n|$))",
                               QRegularExpression::DotMatchesEverythingOption);

    while (jsonIt.hasNext()) {
        QJsonDocument doc = QJsonDocument::fromJson(jsonIt.next().captured(0).toUtf8());
        if (doc.isNull()) continue;

        QJsonObject jObj = doc.object();
        QJsonArray parts = jObj["candidates"].toArray()[0].toObject()["content"].toObject()["parts"].toArray();

        for (const QJsonValue &part : qAsConst(parts))
        {
            jObj = part.toObject();
            QString analysisText = jObj["text"].toString();

            QRegularExpressionMatchIterator eventIt = eventRe.globalMatch(analysisText);
            while (eventIt.hasNext()) {
                QRegularExpressionMatch match = eventIt.next();

                auto parseTime = [](const QString &str) -> QTime {
                    // If it has two colons, it's hh:mm:ss; if one, it's mm:ss
                    const QString format = (str.count(':') == 2) ? QStringLiteral("hh:mm:ss")
                                                                 : QStringLiteral("mm:ss");
                    return QTime::fromString(str, format);
                };

                ExtractedEvent ev;
                ev.startTime = parseTime(match.captured(1));
                ev.endTime   = parseTime(match.captured(2));
                ev.type      = match.captured(3).trimmed();
                ev.description = match.captured(4).trimmed();
                ev.rawBlock  = match.captured(0);

                masterTimeline.append(ev);
            }
        }
    }

    // 3. Sort the master list chronologically
    std::sort(masterTimeline.begin(), masterTimeline.end());

    // 4. Output the Unified Report
    printUnifiedTimeline(masterTimeline);
}

