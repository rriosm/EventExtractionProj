#ifndef URL_H
#define URL_H

#include "Utils_global.h"

QT_BEGIN_NAMESPACE
#include <QDataStream>
#include <QHashFunctions>
#include <QExplicitlySharedDataPointer>
#include <QUrl>
#include <QUuid>
QT_END_NAMESPACE

namespace utils {

    enum class UTILSSHARED_EXPORT URL_Error_e {
        URL_Error = 0x50100,
        URL_Error_Bad_DataStream_Too_Old = 0x50101,
        URL_Error_Bad_StreamHeader_Too_Old = 0x50102,
        URL_Error_Bad_Class_Version_Too_Old = 0x50103,
        URL_Error_Data_Corrupted = 0x50105,
        URL_Error_Null = 0x50106,
        URL_Error_Unknown = 0x50107,
        URL_No_Data = 0x50108,
        URL_Success = 0x50109
    };

    typedef URL_Error_e URL_Error_t;

    inline bool operator==(const URL_Error_t type1, const URL_Error_t type2)
    {
        return (static_cast<quint64> (type1) == static_cast<quint64> (type2));
    }

    inline uint qHash(const URL_Error_t type, uint seed = 0)
    {
        return (::qHash(static_cast<quint64> (type), seed));
    }

    enum class UTILSSHARED_EXPORT URL_ParsingMode_e {
        TolerantMode = QUrl::ParsingMode::TolerantMode,
        StrictMode = QUrl::ParsingMode::StrictMode,
        DecodedMode = QUrl::ParsingMode::DecodedMode
    };

    typedef URL_ParsingMode_e URL_ParsingMode_t ;

    class UTILSSHARED_EXPORT UrlData;

    class UTILSSHARED_EXPORT Url : public QUrl
    {
        Q_CLASSINFO("Author", "author")
        Q_CLASSINFO("Version", "1.0.1")
        Q_CLASSINFO("Status", "Active")

        /**
         * @brief classVersion
         */
        static const quint16 classVersion = quint16(101);

        /**
         * @brief streamHeader
         */
        static const quint64 streamHeader = 0xA0B0C0D0F0;

        /**
         * @brief dataStreamVersion
         */
        static const qint32 dataStreamVersion =
#if QT_VERSION < 0x050f00
        static_cast<qint32>(QDataStream::Qt_5_12);
#elif QT_VERSION < 0x060000
        static_cast<qint32>(QDataStream::Qt_5_15);
#else
        static_cast<qint32>(QDataStream::Qt_6_0);
#endif
        /**
         * @brief operator <<
         * @param outDataStream
         * @param url
         * @return
         */
        friend QDataStream &operator<<(QDataStream &outDataStream, const Url & url)
        {
            url.insertToDataStream(outDataStream);

            return (outDataStream);
        }

        /**
         * @brief operator >>
         * @param inDataStream
         * @param url
         * @return
         */
        friend QDataStream &operator>>(QDataStream &inDataStream, Url & url)
        {
            url.extractFromDataStream(inDataStream);

            return (inDataStream);
        }

        public:

            /**
             * @brief Url
             */
            Url();

            /**
             * @brief Url
             * @param url
             * @param parsingMode
             * @param uuid
             */
            Url(const QString &url, URL_ParsingMode_t parsingMode = URL_ParsingMode_t::TolerantMode, const QUuid &uuid = QUuid());

            /**
             * @brief Url
             * @param url
             * @param uuid
             */
            Url(const QUrl &url, const QUuid &uuid = QUuid());

            /**
             * @brief Url
             * @param url
             */
            Url(const Url &url);

            /**
             * @brief operator ==
             * @param right
             * @return
             */
            bool operator ==(const Url &right) const;

            /**
             * @brief operator !=
             * @param right
             * @return
             */
            bool operator !=(const Url &right) const;

            /**
             * @brief operator ==
             * @param ptr
             * @return
             */
            bool operator==(const Url *ptr) const;

            /**
             * @brief operator !=
             * @param ptr
             * @return
             */
            bool operator!=(const Url *ptr) const;

            /**
             * @brief operator =
             * @param right
             * @return
             */
            // Url &operator =(const QString &right);

            /**
             * @brief operator =
             * @param right
             * @return
             */
            Url &operator =(const Url &right);

            /**
             * @brief ~Url
             */
            virtual ~Url();

            /**
             * @brief detachedCopy
             * @return
             */
            Url detachedCopy();

            /**
             * @brief clear
             */
            void reset();

            /**
             * @brief getFileBasename
             * @return
             */
            QString fileBaseName() const;

            /**
             * @brief getFileSuffix
             * @return
             */
            QString fileSuffix() const;

            /**
             * @brief getChecksumCRC16
             * @return
             */
            quint16 getChecksumCRC16();

            /**
             * @brief getChecksumMD4
             * @return
             */
            const QByteArray & getChecksumMD4();

            /**
             * @brief getChecksumMD5
             * @return
             */
            const QByteArray & getChecksumMD5();

            /**
             * @brief getUniqueID
             * @return
             */
            const QByteArray & getUniqueID() const;

            /**
             * @brief getUuid
             * @return
             */
            const QUuid & getUuid() const;

            // /**
            //  * @brief toLocalFile
            //  * @return
            //  */
            // QString toLocalFile() const;

            // /**
            //  * @brief fromLocalFile
            //  * @param localFile
            //  * @return
            //  */
            // Url fromLocalFile();

            // /**
            //  * @brief fromLocalFile
            //  * @param filePath
            //  * @return
            //  */
            // static Url fromLocalFile(const QString & filePath);

        private:

            /**
             * @brief hash
             */
            void hash();

            // Serialization

            /**
             * @brief extractFromDataStream
             * @param dataStream
             * @return
             */
            virtual URL_Error_t extractFromDataStream(QDataStream & dataStream);

            /**
             * @brief insertToDataStream
             * @param dataStream
             * @return
             */
            virtual URL_Error_t insertToDataStream(QDataStream & dataStream) const;

        private:

            QExplicitlySharedDataPointer<UrlData> sharedDataP;
    };

    inline bool operator<(const Url &url1, const Url &url2)
    {
        if (url1 != url2)
        {
            return (url1.getUniqueID() < url2.getUniqueID());
        }

        return (false);
    }

    inline uint qHash(const Url &url, uint seed = 0)
    {
        return (::qHash(url.getUniqueID(), seed));
    }

} // utils


Q_DECLARE_METATYPE(utils::Url)
Q_DECLARE_METATYPE(utils::Url *)

Q_DECLARE_OPAQUE_POINTER(utils::Url *)

Q_DECLARE_TYPEINFO(utils::Url, Q_MOVABLE_TYPE);

#endif // URL_H
