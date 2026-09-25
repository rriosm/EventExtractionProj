#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#include "Utils_global.h"

#include "Url.h"

QT_BEGIN_NAMESPACE
#include <QObject>
#include <QExplicitlySharedDataPointer>
#include <QList>
#include <QNetworkReply>
#include <QSslError>
QT_END_NAMESPACE

namespace utils {

    class UTILSSHARED_EXPORT DownloadManagerData;

    class UTILSSHARED_EXPORT DownloadManager : public QObject
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
             * @brief DownloadManager
             */
            explicit DownloadManager(QObject *parent = nullptr);

            /**
             * @brief ~DownloadManager
             */
            virtual ~DownloadManager();

        private:
            /**
             * @brief doDownload
             * @param url
             */
            void doDownload(const Url &url);

            /**
             * @brief saveFileName
             * @param url
             * @return
             */
            QString saveFileName(const QString dirname, const QString basename);

            /**
             * @brief saveToDisk
             * @param filename
             * @param data
             * @return
             */
            bool saveToDisk(const QString &filename, QIODevice *data);

        signals:

            /**
             * @brief downloadFinished
             * @param url
             */
            void downloadFinished(const utils::Url &url);

            /**
             * @brief downloadProgress
             * @param bytesExpected
             * @param bytesReceived
             * @param url
             */
            void downloadProgress(qint64 bytesExpected, qint64 bytesReceived, utils::Url &url);

            /**
             * @brief finished
             */
            void finished();

            /**
             * @brief error
             * @param err
             */
            void downloadError(QString err);

        public slots:

            /**
             * @brief addMediaFileForDownload
             * @param url
             */
            void addFileForDownload(const utils::Url &url);

            /**
             * @brief on_downloadFinished
             * @param reply
             */
            void on_downloadFinished(QNetworkReply *reply);

            /**
             * @brief on_downloadProgress
             * @param bytesExpected
             * @param bytesReceived
             */
            void on_downloadProgress(qint64 bytesExpected, qint64 bytesReceived);

            /**
             * @brief sslErrors
             * @param errors
             */
            void on_sslErrors(const QList<QSslError> &sslErrors);

            /**
             * @brief process
             */
            void process();

        private:

            QExplicitlySharedDataPointer<DownloadManagerData> sharedDataP;
    };
}

#endif // DOWNLOADMANAGER_H
