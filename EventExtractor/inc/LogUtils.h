#ifndef LOGUTILS_H
#define LOGUTILS_H

#include <QString>

class LogUtils
{
        /**
         * @brief classVersion
         */
        static const quint16 classVersion = quint16(101);

    public:
        // --- Error Handling ---
        /**
         * Converts an FFmpeg numeric error code into a human-readable QString.
         */
        static QString ffmpegErrorToString(int errNum);

        /**
         * Checks if a return value is an error. If so, logs it with context.
         * Ignores non-fatal codes like EAGAIN or EOF.
         */
        static void checkAndLog(int ret, const QString &context);

        // --- Logging & Diagnostics ---
        /**
         * Logs the current FFmpeg version and build configuration.
         */
        static void logFFmpegDiagnostics();

        /**
         * @brief safeLog: * Thread-safe log that includes Timestamp, Thread Type (Main/Worker), and Thread ID.
         * @param message
         * @param file
         * @param line
         * @param function
         */
        static void safeLog(const QString &message,
                            const QString & file,
                            int line,
                            const QString & function);

        /**
         * Writes a message to a persistent file with a 5MB rotation limit.
         */
        static void logToFile(const QString &message);

        /**
         * @brief enableLog
         */
        static void enableLog();

        /**
         * @brief disableLog
         */
        static void disableLog();

    private:

        static QMutex m_logMutex;
};

#endif // LOGUTILS_H
