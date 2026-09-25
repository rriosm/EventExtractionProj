
#ifndef LOGGER_H
#define LOGGER_H

#include "Logger_global.h"

/**
  * Qt Libraries
  */
QT_BEGIN_NAMESPACE
#include <QObject>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QTextStream>
#include <QString>
QT_END_NAMESPACE

/**
  * C++ Standard Libraries
  */
// #include <string.h>

const QString LOGFILENNAMEPREFIX = QString("LogFile-");
const QString LOGFILENAMESUFFIX = QString(".log");

namespace logger {

    enum class LOGGERSHARED_EXPORT LogTarget { Console, File };

    class LOGGERSHARED_EXPORT Logger : public QObject
    {
        Q_OBJECT

        Q_CLASSINFO("Author", "author")
        Q_CLASSINFO("Version", "1.0.1")
        Q_CLASSINFO("Status", "Active")

        /**
         * @brief classVersion
         */
        static const quint16 classVersion = quint16(101);

        public:

            /**
             * @brief instance Logger singleton class
             * @return
             */
            static Logger * instance();

            /**
             * Logger destructor class.
             */
            virtual ~Logger();

            /**
             * @brief logDir
             * @return
             */
            QString logDir();

            /**
             * @brief setLogDir
             * @param logDir
             */
            bool setLogDir(const QString newLogDir);

            /**
             * @brief enableLogFile
             */
            void enableLogFile();

            /**
             * @brief disableLogFile
             */
            void disableLogFile();

            /**
             * @brief logFileBasename
             * @return
             */
            QString logFileBasename();

            /**
             * @brief setLogFileBasename
             * @param newLogFileName
             * @return
             */
            bool setLogFileBasename(const QString &newLogFileName);

            /**
             * @brief setMaxFileSize
             * @param bytes
             */
            void setMaxFileSize(qint64 bytes);

            /**
             * @brief log
             * @param type
             * @param message
             */
            void log(const QtMsgType type,
                     const QString message,
                     const QString & file,
                     qint32 line,
                     const QString & function);

            /**
             * @brief log
             * @param type
             * @param message
             */
            void log(const QtMsgType type,
                     const std::string message,
                     const QString & file,
                     qint32 line,
                     const QString & function);

            /**
             * @brief log
             * @param type
             * @param message
             */
            void log(const QtMsgType type,
                     const char* message,
                     const QString & file,
                     qint32 line,
                     const QString & function);

            /**
             * @brief debug
             * @param message
             */
            void debug(const QString message,
                       const QString & file,
                       qint32 line,
                       const QString & function);

            /**
             * @brief info
             * @param message
             */
            void info(const QString message,
                      const QString & file,
                      qint32 line,
                      const QString & function);

            /**
             * @brief warning
             * @param message
             */
            void warning(const QString message,
                         const QString & file,
                         qint32 line,
                         const QString & function);

            /**
             * @brief critical
             * @param message
             */
            void critical(const QString message,
                          const QString & file,
                          qint32 line,
                          const QString & function);

            /**
             * @brief fatal
             * @param message
             */
            void fatal(const QString message,
                       const QString & file,
                       qint32 line,
                       const QString & function);

            /**
             * @brief installHandler
             */
            static void installHandler();

        signals:

        public slots:

        private:

            /**
             * @brief Logger
             * @param parent
             */
            explicit Logger(QObject *parent = nullptr);

            /**
             * @brief qtMessageHandler
             * @param type
             * @param context
             * @param msg
             */
            static void qtMessageHandler(QtMsgType type,
                                         const QMessageLogContext &context,
                                         const QString &msg);

            /**
             * @brief checkRotation
             */
            void checkRotation();

            /**
             * @brief getCurrentCounterValue
             * @return
             */
            quint64 getCurrentCounterValue();

            /**
             * @brief msgTypeToString
             * @param type
             * @param target
             * @return
             */
            QString msgTypeToString(QtMsgType type, LogTarget target);

            /**
             * @brief prqint32Log
             * @param type
             * @param message
             * @param file
             * @param line
             * @param function
             */
            void printLog(const QtMsgType type,
                          const QString &message,
                          const char* file = nullptr,
                          qint32 line = 0,
                          const char* function = nullptr);

            /**
             * @brief simplifyFunctionName
             * @param function
             * @return
             */
            QString simplifyFunctionName(const char* function);

        private:

            static Logger* _instance;
            static QMutex _instanceMutex;

            QMutex m_dataMutex;
            quint64 m_counter;
            QString m_logDir;
            QString m_logFileBasename;
            QFile m_file;
            QTextStream m_stream;

            qint64 m_maxFileSize;
            const QString m_format = "[%1] [%2] [%3] [%4] [Counter: %5] [%6]-> %7";

    }; // class Logger

} // namespace logger

#endif // LOGGER_H
