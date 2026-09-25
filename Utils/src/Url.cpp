
#include "Url.h"

#include "NamespaceDefs.h"

#include "Logger.h"
using namespace logger;

#include <QSharedData>
#include <QCryptographicHash>
#include <QDebug>
#include <QFileInfo>
#include <QHash>

namespace utils {

    class UrlData : public QSharedData
    {
        public:

            UrlData() :
                QSharedData(),
                m_crc16(quint16(0))
            {
                this->m_uuid = QUuid::createUuid();
            }

            virtual ~UrlData()
            {
                // TODO
            }

            UrlData(const UrlData &urlData) :
                QSharedData(urlData),
                m_uuid(urlData.m_uuid),
                m_uniqueID(urlData.m_uniqueID),
                m_md4Hash(urlData.m_md4Hash),
                m_md5Hash(urlData.m_md5Hash),
                m_crc16(urlData.m_crc16)/*,
                    m_localFileInfo(urlData.m_localFileInfo)*/
            {
                // TODO
            }

            const UrlData &operator=(const UrlData &rhs)
            {
                if (this != &rhs)
                {
                    // this->m_localFileInfo == rhs.m_localFileInfo;
                    this->m_uuid = rhs.m_uuid;
                    this->m_uniqueID = rhs.m_uniqueID;
                    this->m_md4Hash = rhs.m_md4Hash;
                    this->m_md5Hash = rhs.m_md5Hash;
                    this->m_crc16 = rhs.m_crc16;
                }

                return ( *this );
            }

            bool operator !=(const UrlData &rhs) const
            {
                return !(this == &rhs);
            }

            bool operator ==(const UrlData &rhs) const
            {
                if ( // (this->m_localFileInfo == rhs.m_localFileInfo) &&
                    (this->m_uuid == rhs.m_uuid) &&
                    (this->m_uniqueID == rhs.m_uniqueID) &&
                    (this->m_md4Hash == rhs.m_md4Hash) &&
                    (this->m_md5Hash == rhs.m_md5Hash) &&
                    (this->m_crc16 == rhs.m_crc16) )
                {
                    return (true);
                }

                return (false);
            }

            bool operator==(const UrlData *ptr) const
            {
                return (this == ptr);
            }

            bool operator!=(const UrlData *ptr) const
            {
                return (this != ptr);
            }

            const QUuid & uuid() const
            {
                return (this->m_uuid);
            }

            void setUuid(const QUuid & newUuid)
            {
                if ( ( ! newUuid.isNull() ) &&
                    (newUuid != this->m_uuid) )
                {
                    this->m_uuid = newUuid;
                }
            }

            const QByteArray & uniqueID() const
            {
                return (this->m_uniqueID);
            }

            void setUniqueID()
            {
                this->m_uniqueID.clear();

                this->m_uniqueID.append(static_cast<qint8>(this->m_crc16));
                this->m_uniqueID.append(this->m_md4Hash);
                this->m_uniqueID.append(this->m_md5Hash);

            }

            void setUniqueID(const QByteArray & newUniqueID)
            {
                if ( ( ! newUniqueID.isEmpty() ) &&
                    (newUniqueID != this->m_uniqueID) )
                {
                    this->m_uniqueID = newUniqueID;
                }
            }

            const QByteArray & md4Hash() const
            {
                return (this->m_md4Hash);
            }

            void setMd4Hash(const QByteArray & newMd4Hash)
            {
                if ( ( ! newMd4Hash.isEmpty() ) &&
                    ( newMd4Hash != this->m_md4Hash ) )
                {
                    this->m_md4Hash = newMd4Hash;
                }
            }

            const QByteArray & md5Hash() const
            {
                return (this->m_md5Hash);
            }

            void setMd5Hash(const QByteArray & newMd5Hash)
            {
                if ( ( ! newMd5Hash.isEmpty() ) &&
                    ( newMd5Hash != this->m_md5Hash ) )
                {
                    this->m_md5Hash = newMd5Hash;
                }
            }

            quint16 crc16() const
            {
                return (this->m_crc16);
            }

            void setCrc16(quint16 newCrc16)
            {
                if (newCrc16 != this->m_crc16)
                {
                    this->m_crc16 = newCrc16;
                }
            }

            bool hasSchemeAndAuthority(const QString &newUrl)
            {
                QUrl url(newUrl);

                if ( url.scheme().isEmpty() &&
                    url.authority().isEmpty() )
                {
                    return (false);
                }

                return (true);
            }

           // const QFileInfo & localFileInfo() const
           // {
           //     return (this->m_localFileInfo);
           // }

           // void setLocalFileInfo(const QFileInfo & newLocalFileInfo)
           // {
           //     if (newLocalFileInfo != this->m_localFileInfo)
           //     {
           //         this->m_localFileInfo = newLocalFileInfo;
           //     }
           // }

           // void setLocalFileInfo(const QString & newStrLocalFileInfo)
           // {
           //     QFileInfo newLocalFileInfo(newStrLocalFileInfo);

           //     if (newLocalFileInfo != this->m_localFileInfo)
           //     {
           //         this->m_localFileInfo = newLocalFileInfo;
           //     }
           // }

            void resetData()
            {
                this->m_localFileInfo.setFile(QString());
                this->m_uniqueID.clear();
                this->m_md4Hash.clear();
                this->m_md5Hash.clear();
                this->m_crc16 = qint8(0);
            }

        private:

            QUuid m_uuid;

            QByteArray m_uniqueID;

            QByteArray m_md4Hash;

            QByteArray m_md5Hash;

            quint16 m_crc16;

            // TODO Change QFileInfo to QByteArray. The file must be temporarely storaged!
            //      It must be storage in the database.
            QFileInfo m_localFileInfo;
    };

    // public

    Url::Url() :
        sharedDataP(new UrlData)
    {
        // TODO
    }

    Url::Url(const QString &urlStr, URL_ParsingMode_t parsingMode, const QUuid &uuid) :
        QUrl(urlStr, static_cast<QUrl::ParsingMode>(parsingMode)),
        sharedDataP(new UrlData)
    {
        if ( ! uuid.isNull() )
        {
            this->sharedDataP->setUuid(uuid);
        }

        if ( ! urlStr.isNull() &&
            ! urlStr.isEmpty() )
        {
            if ( ! this->sharedDataP->hasSchemeAndAuthority(urlStr) ) // It has no scheme "file"
            {
                this->setScheme("file");
            }

            this->hash();

            if ( ! uuid.isNull() )
            {
                this->sharedDataP->setUuid(uuid);
            }
        }
    }

    Url::Url(const QUrl &url, const QUuid &uuid) :
        QUrl(url),
        sharedDataP(new UrlData)
    {
        if ( ! this->isEmpty() &&
            this->isValid() )
        {
            if ( ! this->sharedDataP->hasSchemeAndAuthority(url.toString()) ) // It has no scheme "file"
            {
                this->setScheme("file");
            }

            this->hash();
        }

        if ( ! uuid.isNull() )
        {
            this->sharedDataP->setUuid(uuid);
        }
    }

    Url::Url(const Url &url) :
        QUrl(static_cast<QUrl>(url))
    {
        this->sharedDataP = url.sharedDataP;
    }

    bool Url::operator ==(const Url &right) const
    {
        if ( (QUrl::operator ==(right) ) &&
            ( (this->sharedDataP == right.sharedDataP) ||
             (*this->sharedDataP == *right.sharedDataP) ) )
        {
            return (true);
        }

        return (false);
    }

    bool Url::operator !=(const Url &right) const
    {
        return !(this == &right);
    }

    bool Url::operator==(const Url *ptr) const
    {
        return (this == ptr);
    }

    bool Url::operator!=(const Url *ptr) const
    {
        return !(this == ptr);
    }

    Url & Url::operator =(const Url & right)
    {
        if (this != &right)
        {
            QUrl::operator =(right);
            this->sharedDataP = right.sharedDataP;
        }

        return (*this);
    }

    Url::~Url()
    {
        // TODO
    }

    Url Url::detachedCopy()
    {
        Url copy(*this);
        copy.sharedDataP.detach();

        return (copy);
    }

    void Url::reset()
    {
        this->sharedDataP->resetData();

        this->QUrl::clear();
    }

    QString Url::fileBaseName() const
    {
        if ( this->isLocalFile() )
        {
            QFileInfo fName(this->path());

            return (fName.baseName());
        }

        return (QString());
    }

    QString Url::fileSuffix() const
    {
        if ( this->isLocalFile() )
        {
            QFileInfo fName(this->path());

            return (fName.suffix());
        }

        return (QString());
    }

    quint16 Url::getChecksumCRC16()
    {
        return (this->sharedDataP->crc16());
    }

    const QByteArray & Url::getChecksumMD4()
    {
        return (this->sharedDataP->md4Hash());
    }

    const QByteArray & Url::getChecksumMD5()
    {
        return (this->sharedDataP->md5Hash());
    }

    const QByteArray & Url::getUniqueID() const
    {
        return (this->sharedDataP->uniqueID());
    }

    const QUuid & Url::getUuid() const
    {
        return (this->sharedDataP->uuid());
    }

    // QString Url::toLocalFile() const
    // {
    //    return (this->QUrl::toLocalFile());
    // }

    // Url Url::fromLocalFile()
    // {
    //    return (QUrl::fromLocalFile(this->sharedDataP->localFileInfo().absoluteFilePath()));
    // }

    // Url Url::fromLocalFile(const QString & filePath)
    // {
    //    Url url(QUrl::fromLocalFile(filePath));

    //    return (url);
    // }

    // private

    void Url::hash()
    {
        // QFileInfo file(this->path());

        // if ( ! file.exists() ||
        //     ! file.isFile() )
        // {
        //    return;
        // }

        if (this->isEmpty())
        {
            return;
        }

        QByteArray ba(this->toLocalFile().toLatin1().constData());

        if ( ! ba.isEmpty() &&
            ! ba.isNull() )
        {
            this->sharedDataP->setMd4Hash(QCryptographicHash::hash(ba, QCryptographicHash::Md4));
            this->sharedDataP->setMd5Hash(QCryptographicHash::hash(ba, QCryptographicHash::Md5));
            this->sharedDataP->setCrc16(qChecksum(ba.constData(),
                                                  ba.toUInt(),
                                                  Qt::ChecksumType::ChecksumIso3309));

            this->sharedDataP->setUniqueID();
        }
    }

    URL_Error_t Url::extractFromDataStream(QDataStream &dataStream)
    {
        if (dataStream.version() < Url::dataStreamVersion)
        {
            return (URL_Error_t::URL_Error_Bad_DataStream_Too_Old);
        }

        quint64 dsStreamHeader;
        quint16 dsClassVersion;

        dataStream >> dsStreamHeader >> dsClassVersion;

        if (dsStreamHeader < Url::streamHeader)
        {
            return (URL_Error_t::URL_Error_Bad_StreamHeader_Too_Old);
        }

        if (dsClassVersion < Url::classVersion)
        {
            return (URL_Error_t::URL_Error_Bad_Class_Version_Too_Old);
        }

        qint32 dsSerializeCode;
        dataStream >> dsSerializeCode;

        QUuid dsID;
        dataStream >> dsID;
        this->sharedDataP->setUuid(dsID);

        QByteArray dsUID;
        dataStream >> dsUID;
        this->sharedDataP->setUniqueID(dsUID);

        QByteArray dsMD4H;
        dataStream >> dsMD4H;
        this->sharedDataP->setMd4Hash(dsMD4H);

        QByteArray dsMD5H;
        dataStream >> dsMD5H;
        this->sharedDataP->setMd5Hash(dsMD5H);

        quint16 dsC16;
        dataStream >> dsC16;
        this->sharedDataP->setCrc16(static_cast<quint16>(dsC16));

        // QString dsLFile;
        // dataStream >> dsLFile;
        // this->sharedDataP->setLocalFileInfo(dsLFile);

        return (URL_Error_t::URL_Success);
    }

    URL_Error_t Url::insertToDataStream(QDataStream &dataStream) const
    {
        dataStream.setVersion(Url::dataStreamVersion);

        dataStream << Url::streamHeader << Url::classVersion;

        dataStream << static_cast<qint32>(Serialize_t::SERIALIZE_URL_DATA);

        dataStream << this->getUuid();
        dataStream << this->getUniqueID();
        dataStream << this->sharedDataP->md4Hash();
        dataStream << this->sharedDataP->md5Hash();
        dataStream << static_cast<quint16>(this->sharedDataP->crc16());
        //                        dataStream << this->path();

        return (URL_Error_t::URL_Success);
    }

} // utils
