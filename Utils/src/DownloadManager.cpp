
#include "DownloadManager.h"

#include "Logger.h"
using namespace logger;

#include "Utils.h"

QT_BEGIN_NAMESPACE
#include <QFile>
#include <QFileInfo>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QMutex>
#include <QSharedData>
#include <QStringList>
#include <QTimer>
#include <QWaitCondition>
QT_END_NAMESPACE

namespace utils {

    class DownloadManagerData : public QSharedData
    {
        public :

            DownloadManagerData() :
                QSharedData()
            {
                // TODO
            }

            DownloadManagerData(const DownloadManagerData &prfxData) :
                QSharedData(prfxData),
                currentDownloadsList(prfxData.currentDownloadsList),
                mediaFileForDownloadList(prfxData.mediaFileForDownloadList),
                manager(prfxData.manager)
            {
                // TODO
            }

            virtual ~DownloadManagerData()
            {
                // TODO
            }

            QList<QNetworkReply *> currentDownloadsList;
            QList<Url *> mediaFileForDownloadList;

            QNetworkAccessManager * manager;

            QMutex _currentDownloadsMutex;
            QMutex _mediaFileforDownloadListMutex;
    };

    DownloadManager::DownloadManager(QObject *parent) :
        QObject(parent),
        sharedDataP(new DownloadManagerData)
    {
        Logger::instance()->log(QtDebugMsg,
                                "DownloadManager::DownloadManager(QObject *parent) - Entered!",
                                __FILE__,
                                __LINE__,
                                Q_FUNC_INFO);

        connect(this->sharedDataP->manager, SIGNAL(finished(QNetworkReply*)), SLOT(on_downloadFinished(QNetworkReply*)));

        Logger::instance()->log(QtDebugMsg,
                                "DownloadManager::DownloadManager(QObject *parent) - Leaving!",
                                __FILE__,
                                __LINE__,
                                Q_FUNC_INFO);
    }

    DownloadManager::~DownloadManager()
    {
        this->sharedDataP->_mediaFileforDownloadListMutex.lock();

        if ( ! this->sharedDataP->mediaFileForDownloadList.isEmpty() )
        {
            this->sharedDataP->mediaFileForDownloadList.clear();
        }

        this->sharedDataP->_mediaFileforDownloadListMutex.unlock();

        this->sharedDataP->_currentDownloadsMutex.lock();

        if ( ! this->sharedDataP->currentDownloadsList.isEmpty() )
        {
            this->sharedDataP->currentDownloadsList.clear();
        }

        this->sharedDataP->_currentDownloadsMutex.unlock();
    }

    void DownloadManager::addFileForDownload(const Url &url)
    {
        if (url.isValid() && !url.isEmpty())
        {
            this->sharedDataP->_mediaFileforDownloadListMutex.lock();

            Url *tUrl = new Url(url);

            this->sharedDataP->mediaFileForDownloadList.append(tUrl);

            this->sharedDataP->_mediaFileforDownloadListMutex.unlock();
        }
    }

    void DownloadManager::doDownload(const Url &url)
    {
        if (url.isLocalFile())
        {
            //                            Url temp(url.url());
            emit downloadFinished(url);

            return;
        }

        Logger::instance()->log(QtDebugMsg,
                                "DownloadManager::doDownload(const Url &url) - Entered!",
                                __FILE__,
                                __LINE__,
                                Q_FUNC_INFO);

        QNetworkRequest request(url);
        QNetworkReply *reply = this->sharedDataP->manager->get(request);

        connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(on_downloadProgress(qint64,qint64)));

    #ifndef QT_NO_SSL
        connect(reply, SIGNAL(sslErrors(QList<QSslError>)), SLOT(on_sslErrors(QList<QSslError>)));
    #endif
        this->sharedDataP->_currentDownloadsMutex.lock();

        this->sharedDataP->currentDownloadsList.append(reply);

        this->sharedDataP->_currentDownloadsMutex.unlock();

        Logger::instance()->log(QtDebugMsg,
                                "DownloadManager::doDownload(const Url &url) - Leaving!",
                                __FILE__,
                                __LINE__,
                                Q_FUNC_INFO);
    }

    QString DownloadManager::saveFileName(const QString dirname, const QString basename)
    {
        QString filename = dirname;

        Logger::instance()->log(QtDebugMsg,
                                "QString DownloadManager::saveFileName(const Url &url) - Leaving!",
                                __FILE__,
                                __LINE__,
                                Q_FUNC_INFO);

        if (basename.isEmpty())
        {
            filename += Utils::FILE_BASENAME();
        } else {
            filename += basename;
        }

        if (QFile::exists(filename))
        {
            // already exists, don't overwrite
            int i = 0;
            filename += '.';

            while(QFile::exists(filename + QString::number(i)))
            {
                ++i;
            }

            filename += QString::number(i);
        }

        Logger::instance()->log(QtDebugMsg,
                                "QString DownloadManager::saveFileName(const Url &url) - Leaving!",
                                __FILE__,
                                __LINE__,
                                Q_FUNC_INFO);

        return (filename);
    }

    bool DownloadManager::saveToDisk(const QString &filename, QIODevice *data)
    {
        Logger::instance()->log(QtDebugMsg,
                                "bool DownloadManager::saveToDisk(const QString &filename, QIODevice *data) - Entered!",
                                __FILE__,
                                __LINE__,
                                Q_FUNC_INFO);

        QFile file(filename);

        if (!file.open(QIODevice::WriteOnly))
        {
            Logger::instance()->log(QtDebugMsg,
                                    "bool DownloadManager::saveToDisk(const QString &filename, QIODevice *data) - Could not open file!",
                                    __FILE__,
                                    __LINE__,
                                    Q_FUNC_INFO);

            //                            fprintf(stderr, "Could not open %s for writing: %s\n",
            //                                    qPrintable(filename),
            //                                    qPrintable(file.errorString()));

            return (false);
        }

        file.write(data->readAll());
        file.close();

        Logger::instance()->log(QtDebugMsg,
                                "bool DownloadManager::saveToDisk(const QString &filename, QIODevice *data) - Leaving!",
                                __FILE__,
                                __LINE__,
                                Q_FUNC_INFO);

        return (true);
    }

    void DownloadManager::process()
    {
        this->sharedDataP->_currentDownloadsMutex.lock();

        bool go = ! this->sharedDataP->currentDownloadsList.isEmpty();

        this->sharedDataP->_currentDownloadsMutex.unlock();

        if ( go )
        {
            this->sharedDataP->_mediaFileforDownloadListMutex.lock();

            if ( ! this->sharedDataP->mediaFileForDownloadList.isEmpty() )
            {
                foreach(Url *url, this->sharedDataP->mediaFileForDownloadList)
                {
                    this->doDownload(*url);

                    this->sharedDataP->mediaFileForDownloadList.removeAll(url);
                }

                // Grants the list cleanup
                this->sharedDataP->mediaFileForDownloadList.clear();
            }

            this->sharedDataP->_mediaFileforDownloadListMutex.unlock();
        } else {

            // Terminates DownloadManager worker!
            emit finished();
        }
    }

    void DownloadManager::on_sslErrors(const QList<QSslError> &sslErrors)
    {
    #ifndef QT_NO_SSL
        foreach (const QSslError &error, sslErrors)
        {

            Logger::instance()->log(QtDebugMsg,
                                    "void DownloadManager::sslErrors(const QList<QSslError> &sslErrors) - Error!",
                                    __FILE__,
                                    __LINE__,
                                    Q_FUNC_INFO);
            emit downloadError(error.errorString());

            //                            fprintf(stderr, "SSL error: %s\n", qPrintable(error.errorString()));
        }
    #else
        Q_UNUSED(sslErrors);
    #endif
    }

    void DownloadManager::on_downloadFinished(QNetworkReply *reply)
    {
        Url url(reply->url());

        QString path = url.path();
        QString basename(QFileInfo(path).fileName());

        if (reply->error())
        {

            Logger::instance()->log(QtDebugMsg,
                                    "void DownloadManager::downloadFinished(QNetworkReply *reply) - Error!",
                                    __FILE__,
                                    __LINE__,
                                    Q_FUNC_INFO);

            //                            fprinf(stderr, "Download of %s failed: %s\n",
            //                                   url.toEncoded().constData(),
            //                                   qPrintable(reply->errorString()));
        } else {

            QString filename = this->saveFileName(Utils::DOWNLOAD_DIR_NAME(), basename);

            if (this->saveToDisk(filename, reply))
            {
                url.setPath(filename);

                Logger::instance()->log(QtDebugMsg,
                                        "void DownloadManager::downloadFinished(QNetworkReply *reply) - Success!",
                                        __FILE__,
                                        __LINE__,
                                        Q_FUNC_INFO);

                //                                printf("Download of %s succeeded (save to %s)\n",
                //                                       url.toEncoded().constData(),
                //                                       qPrintable(filename));

                emit downloadFinished(url);

            } // else ???
        }

        this->sharedDataP->currentDownloadsList.removeAll(reply);

        reply->deleteLater();
    }

    void DownloadManager::on_downloadProgress(qint64 bytesExpected, qint64 bytesReceived)
    {
        QNetworkReply *reply = qobject_cast<QNetworkReply *> (sender());

        Url url(reply->url());

        emit downloadProgress(bytesExpected, bytesReceived, url);
    }

} // utils
