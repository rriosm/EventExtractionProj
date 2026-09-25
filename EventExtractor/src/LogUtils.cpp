#include "LogUtils.h"

#include "Logger.h"
using namespace logger;

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QDebug>

// FFmpeg C headers
extern "C" {
#include <libavutil/error.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
}

QMutex LogUtils::m_logMutex;
static bool m_logEnabled = false;

// --- Error Handling Implementation ---

QString LogUtils::ffmpegErrorToString(int errNum)
{
    char errBuf[AV_ERROR_MAX_STRING_SIZE];

    if (av_strerror(errNum, errBuf, sizeof(errBuf)) == 0)
    {
        return QString::fromLocal8Bit(errBuf);
    }

    return QString("Unknown FFmpeg error: %1").arg(errNum);
}

void LogUtils::checkAndLog(int ret, const QString &context)
{
    // We ignore success (0) and common non-error codes used in loops
    if (ret < 0 && ret != AVERROR_EOF && ret != AVERROR(EAGAIN))
    {
        QString errStr = ffmpegErrorToString(ret);
        QString fullMsg = QString("%1 | Error: %2 (Code: %3)").arg(context, errStr).arg(ret);

        LogUtils::safeLog(fullMsg,
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
    }
}

// --- Logging & Diagnostics Implementation ---

void LogUtils::logFFmpegDiagnostics()
{
#ifdef QT_DEBUG
    LogUtils::safeLog("--- FFmpeg Runtime Info ---",
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
    LogUtils::safeLog(QString("LogUtils::logFFmpegDiagnostics() --- Version: %1").arg(av_version_info()),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
    LogUtils::safeLog(QString("LogUtils::logFFmpegDiagnostics() --- avcodec: %1").arg(avcodec_version()),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);

#if defined(AV_CHANNEL_LAYOUT_STEREO) || FFMPEG_NEW_CHANNEL_LAYOUT
    LogUtils::safeLog("LogUtils::logFFmpegDiagnostics() --- Mode: FFmpeg 7+ (New Channel Layout supported)",
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#else
    LogUtils::safeLog("Mode: Legacy FFmpeg (Old Channel Layout)"",
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif
#endif
}

void LogUtils::safeLog(const QString &message,
                       const QString & file,
                       int line,
                       const QString & function)
{
    QMutexLocker locker(&m_logMutex);

    if ( ! m_logEnabled ) return;

    // Identify if we are in the GUI thread or a Worker thread
    bool isMain = (QThread::currentThread() == QCoreApplication::instance()->thread());
    QString threadType = isMain ? "MAIN" : "WORKER";

//    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");

    // Hexadecimal Thread ID for easier debugging
    QString threadId = QString("0x%1").arg(reinterpret_cast<quintptr>(QThread::currentThreadId()), 0, 16);

//    QString formattedMessage = QString("[%1] [%2] [%3] %4")
//                                   .arg(timestamp, threadType, threadId, message);
    QString formattedMessage = QString("[%1] [%2] %3")
                                   .arg(threadType, threadId, message);

    // Standard output for IDE/Console
//    qDebug().noquote() << formattedMessage;

    // Output to console and to persistent output (file) for user machine debugging
    Logger::instance()->debug(formattedMessage, file, line, function);

    // Persistent output for user machine debugging
//    LogUtils::logToFile(formattedMessage);
}

void LogUtils::logToFile(const QString &message)
{
    // 1. Define the path (Uses AppData to ensure write permissions on User machines)
    // Path: usually in the same directory as the app or a 'logs' subfolder
    QString logDirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString logPath = logDirPath + "/debug_log" + timestamp + ".txt";

    QDir dir(logDirPath);

    if (!dir.exists())
    {
        dir.mkpath(".");
    }

    QFile logFile(logPath);

    // 3. Maintenance: Check file size (5MB limit) before opening for writing
    if (logFile.exists() && logFile.size() > 5 * 1024 * 1024)
    {
        // Option A: Delete the old log
        logFile.remove();
        // Option B: Rename to .old if you want to keep the previous session
        // logFile.rename(logPath + timestamp);
    }

    // Open in Append mode so we keep history
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
    {
        QTextStream stream(&logFile);

        stream << "[" << timestamp << "] " << message << "\n";

        logFile.close();
    }
}

void LogUtils::enableLog()
{
    QMutexLocker locker(&m_logMutex);

    if ( m_logEnabled ) return;

    if (Logger::instance()->logDir() == QDir::tempPath())
    {
        QString logLocation = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QCoreApplication::applicationName() + "/logs";
        Logger::instance()->setLogDir(logLocation);
#ifdef QT_DEBUG
        Logger::instance()->setLogFileBasename(QCoreApplication::applicationName());
        Logger::instance()->debug(logLocation,
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
    }

    Logger::instance()->enableLogFile();

    m_logEnabled = true;
}

void LogUtils::disableLog()
{
    QMutexLocker locker(&m_logMutex);

    if (m_logEnabled)
    {
        Logger::instance()->disableLogFile();

        m_logEnabled = false;
    }
}
