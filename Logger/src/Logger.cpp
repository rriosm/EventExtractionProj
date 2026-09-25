
#include "Logger.h"

QT_BEGIN_NAMESPACE
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QThread>
#include <QString>
QT_END_NAMESPACE

#ifdef Q_OS_WIN
#include <windows.h>
#endif

// #include <cstdlib>
// #include <iostream>

const qint64 LOGFILESIZE = 10 * 1024 * 1024; // 10MB Default

namespace logger {

    Logger* Logger::_instance = nullptr;
    QMutex Logger::_instanceMutex;

    Logger::Logger(QObject *parent) :
        QObject(parent),
        m_counter(0),
        m_maxFileSize(LOGFILESIZE)
    {
        this->m_logDir = QDir::tempPath();
        // Format: [TIMESTAMP] [TYPE] [TID] [File:Line] [Counter] [Function] -> Message
        qSetMessagePattern("[%{time}] - [%{type}] - [PID/ThreadID: %{pid}/%{threadid}] [%{file}:%{line}] [%{function}]: %{message}");

    #ifdef Q_OS_WIN
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD dwMode = 0;
        GetConsoleMode(hOut, &dwMode);
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
    #endif
    }

    Logger::~Logger()
    {
        this->disableLogFile();
    }

    Logger *Logger::instance()
    {
        QMutexLocker locker(&_instanceMutex);

        if ( ! _instance ) _instance = new Logger();

        return _instance;
    }

    void Logger::installHandler()
    {
        qInstallMessageHandler(Logger::qtMessageHandler);
    }

    void Logger::qtMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
    {
        QString fileName = QFileInfo(context.file).baseName(); // "Logger" instead of "/path/Logger.cpp"
        Logger::instance()->printLog(type, msg, qPrintable(fileName), context.line, context.function);
    }

    // TODO Check if this function changes the name of the file in the file current directory
    void Logger::checkRotation()
    {
        // No need to lock here; this is called from within printLog which is already locked
        if ( ! this->m_file.isOpen() || this->m_file.size() < this->m_maxFileSize) return;

        this->m_stream.setDevice(nullptr);
        this->m_file.close();

        QFileInfo fileInfo(this->m_file);
        QString logBaseName(fileInfo.baseName());
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        QString oldFile = logBaseName + QString::number(currentTime) + LOGFILENAMESUFFIX;

        if (QFile::exists(oldFile)) QFile::remove(oldFile);

        QFile::rename(this->m_file.fileName(), oldFile);

        if (this->m_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        {
            this->m_stream.setDevice(&this->m_file);
        }
    }

    QString Logger::logDir()
    {
        QMutexLocker locker(&m_dataMutex);

        return (this->m_logDir);
    }

    bool Logger::setLogDir(const QString newLogDir)
    {
        QMutexLocker locker(&m_dataMutex);

        if ( newLogDir.isEmpty() ) return false;

        if (this->m_logDir != newLogDir) this->m_logDir = newLogDir;

        return true;
    }

    void Logger::enableLogFile()
    {
        QMutexLocker locker(&m_dataMutex);

        if (this->m_file.isOpen()) return;

        if (this->m_logFileBasename.isEmpty()) return;

        QDir dir(this->m_logDir);

        if ( ! dir.exists() )
        {
            if ( ! dir.mkpath(".") ) return;
        }

        QString filename = QString(this->m_logFileBasename); // Using a fixed name for better rotation
        this->m_file.setFileName(this->m_logDir + QDir::separator() + LOGFILENNAMEPREFIX + filename + LOGFILENAMESUFFIX);

        if (this->m_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        {
            this->m_stream.setDevice(&this->m_file);
        }
    }

    void Logger::disableLogFile()
    {
        QMutexLocker locker(&this->m_dataMutex);

        if (this->m_file.isOpen())
        {
            this->m_stream.flush();
            this->m_file.close();
        }
    }

    QString Logger::logFileBasename()
    {
        QMutexLocker locker(&m_dataMutex);
        return this->m_logFileBasename;
    }

    bool Logger::setLogFileBasename(const QString &newLogFileName)
    {
        if (newLogFileName.isEmpty()) return false;

        QMutexLocker locker(&m_dataMutex);

        if  (this->m_logFileBasename != newLogFileName)
            this->m_logFileBasename = newLogFileName;

        return true;
    }

    void Logger::log(const QtMsgType type,
                     const QString message,
                     const QString & file,
                     qint32 line,
                     const QString & function)
    {
        this->printLog(type,
                       message,
                       file.toUtf8().data(),
                       line,
                       function.toUtf8().data());
    }

    void Logger::log(const QtMsgType type,
                     const std::string message,
                     const QString & file,
                     qint32 line,
                     const QString & function)
    {
        this->printLog(type,
                       QString::fromUtf8(message.c_str()),
                       file.toUtf8().data(),
                       line,
                       function.toUtf8().data());
    }

    void Logger::log(const QtMsgType type,
                     const char* message,
                     const QString & file,
                     qint32 line,
                     const QString & function)
    {
        this->printLog(type,
                       QString(message),
                       file.toUtf8().data(),
                       line,
                       function.toUtf8().data());
    }

    void Logger::debug(const QString message,
                       const QString & file,
                       qint32 line,
                       const QString & function)
    {
        printLog(QtDebugMsg,
                 message,
                 file.toUtf8().data(),
                 line,
                 function.toUtf8().data());
    }

    void Logger::info(const QString message,
                      const QString & file,
                      qint32 line,
                      const QString & function)
    {
        printLog(QtInfoMsg,
                 message,
                 file.toUtf8().data(),
                 line,
                 function.toUtf8().data());
    }

    void Logger::warning(const QString message,
                         const QString & file,
                         qint32 line,
                         const QString & function)
    {
        printLog(QtWarningMsg,
                 message,
                 file.toUtf8().data(),
                 line,
                 function.toUtf8().data());
    }

    void Logger::critical(const QString message,
                          const QString & file,
                          qint32 line,
                          const QString & function)
    {
        printLog(QtCriticalMsg,
                 message,
                 file.toUtf8().data(),
                 line,
                 function.toUtf8().data());
    }

    void Logger::fatal(const QString message,
                       const QString & file,
                       qint32 line,
                       const QString & function)
    {
        printLog(QtFatalMsg,
                 message,
                 file.toUtf8().data(),
                 line,
                 function.toUtf8().data());
    }

    quint64 Logger::getCurrentCounterValue()
    {
        QMutexLocker locker(&m_dataMutex);

        return (this->m_counter++);
    }

    QString Logger::msgTypeToString(QtMsgType type, LogTarget target)
    {
        bool color = (target == LogTarget::Console);

        // ANSI Escape Codes
        const char* reset  = "\033[0m";
        const char* debug  = "\033[36m"; // Cyan
        const char* info   = "\033[32m"; // Green
        const char* warn   = "\033[33m"; // Yellow
        const char* crit   = "\033[31m"; // Red
        const char* fatal  = "\033[35m"; // Magenta / Bold Red

        switch (type)
        {
        case QtDebugMsg:    return color ? QString("%1DEBUG%2").arg(debug, reset) : "DEBUG";
        case QtInfoMsg:     return color ? QString("%1INFO %2").arg(info, reset)  : "INFO ";
        case QtWarningMsg:  return color ? QString("%1WARN %2").arg(warn, reset)  : "WARN ";
        case QtCriticalMsg: return color ? QString("%1CRIT %2").arg(crit, reset)  : "CRIT ";
        case QtFatalMsg:    return color ? QString("%1FATAL%2").arg(fatal, reset) : "FATAL";
        default:            return "UNKN ";
        }
    }

    void Logger::printLog(const QtMsgType type,
                          const QString &message,
                          const char* file,
                          qint32 line,
                          const char* function)
    {
        QMutexLocker locker(&m_dataMutex);

        this->checkRotation(); // Ensure we haven't exceeded file size

        this->m_counter++;

        // Format the source context (File and Line)
        QString sourceContext;

        if (file)
        {
            QString fileName = file ? QFileInfo(file).baseName() : "???";

            sourceContext = QString(" [%1:%2]").arg(fileName).arg(line);
        }

        QString funcName = simplifyFunctionName(function);
        void* threadId = QThread::currentThread();

        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");

        // Final formatted string for the log
        // Format: [TIMESTAMP] [TYPE] [TID] [File:Line] [Counter] [Function] -> Message
        QString consoleLine = this->m_format
                                  .arg(timestamp,
                                       msgTypeToString(type, LogTarget::Console),// Helper to return "DEBUG", "INFO", etc.
                                       QString::number((qintptr)threadId, 16).right(4),
                                       sourceContext,
                                       QString::number(m_counter),
                                       funcName,
                                       message
                                       );

        QString fileLine = this->m_format
                               .arg(timestamp,
                                    msgTypeToString(type, LogTarget::File),// Helper to return "DEBUG", "INFO", etc.
                                    QString::number((qintptr)threadId, 16).right(4),
                                    sourceContext,
                                    QString::number(m_counter),
                                    funcName,
                                    message
                                    );

        // 1. Console Output
        if (type == QtWarningMsg || type == QtCriticalMsg || type == QtFatalMsg)
        {
            fprintf(stderr, "%s\n", qPrintable(consoleLine));
        } else {
            fprintf(stdout, "%s\n", qPrintable(consoleLine));
        }

        // 2. File Output
        if (m_file.isOpen())
        {
            this->m_stream << fileLine << "\n";
            this->m_stream.flush();
        }
    }

    QString Logger::simplifyFunctionName(const char* function)
    {
        if ( ! function ) return QString("unknown");

        QString func = QString::fromLatin1(function);

        // 1. Remove arguments (everything from the first '(' onwards)
        qint32 parenPos = func.indexOf('(');

        if (parenPos != -1)
        {
            func = func.left(parenPos);
        }

        // 2. Remove return types (everything before the last space before the function name)
        qint32 spacePos = func.lastIndexOf(' ');

        if (spacePos != -1)
        {
            func = func.mid(spacePos + 1);
        }

        // 3. Optional: Remove template parameters if present
        qint32 templatePos = func.indexOf('<');

        if (templatePos != -1)
        {
            func = func.left(templatePos);
        }

        return func;
    }

} // namespace logger
