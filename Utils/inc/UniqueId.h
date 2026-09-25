#ifndef UNIQUEID_H
#define UNIQUEID_H

#include "Utils_global.h"

QT_BEGIN_NAMESPACE
#include <QByteArray>
#include <QDataStream>
#include <QJsonObject>
#include <QMetaType>
#include <QSharedDataPointer>
#include <QUuid>
QT_END_NAMESPACE

namespace utils {

    enum class UTILSSHARED_EXPORT UniqueId_Error_e {
        UniqueId_Error = 0x50100,
        UniqueId_Error_Bad_DataStream_Too_Old = 0x50101,
        UniqueId_Error_Bad_StreamHeader_Too_Old = 0x50102,
        UniqueId_Error_Bad_Class_Version_Too_Old = 0x50103,
        UniqueId_Error_Data_Corrupted = 0x50105,
        UniqueId_Error_Null = 0x50106,
        UniqueId_Error_Unknown = 0x50107,
        UniqueId_Error_No_Data = 0x50108,
        UniqueId_Success = 0x50109
    };

    typedef UniqueId_Error_e UniqueId_Error_t;

    inline bool operator==(const UniqueId_Error_t type1, const UniqueId_Error_t type2)
    {
        return (static_cast<quint64> (type1) == static_cast<quint64> (type2));
    }

    inline uint qHash(const UniqueId_Error_t type, uint seed = 0)
    {
        return (::qHash(static_cast<quint64> (type), seed));
    }

    // 1. Forward declare the class so the operators can see it
    class UTILSSHARED_EXPORT UniqueId;

    // 2. Forward declare the operators within the namespace
    QDataStream &operator<<(QDataStream &out, const UniqueId &uniqueId);
    QDataStream &operator>>(QDataStream &in, UniqueId &uniqueId);

    class UTILSSHARED_EXPORT UniqueIdData;

    class UTILSSHARED_EXPORT UniqueId
    {
        Q_GADGET

        Q_CLASSINFO("Author", "author")
        Q_CLASSINFO("Version", "1.0.1")
        Q_CLASSINFO("Status", "Active")
        Q_CLASSINFO("Description", "Class of Unique ID")

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

        public:

            /**
             * @brief UniqueId
             * @param seed
             */
            UniqueId(const QByteArray &seed = QByteArray());

            /**
             * @brief ~UniqueId
             */
            virtual ~UniqueId();

            /**
             * @brief UniqueId
             * @param rhs
             */
            UniqueId(const UniqueId &rhs);

            /**
             * @brief operator =
             * @param right
             * @return
             */
            UniqueId &operator=(const UniqueId &right);

            /**
             * @brief operator ==
             * @param right
             * @return
             */
            bool operator ==(const UniqueId &right) const;

            /**
             * @brief operator !=
             * @param right
             * @return
             */
            bool operator !=(const UniqueId &right) const;

            /**
             * @brief operator <
             * @param other
             * @return
             */
            bool operator<(const UniqueId &other) const;

            /**
             * @brief operator ==
             * @param ptr
             * @return
             */
            bool operator==(const UniqueId *ptr) const;

            /**
             * @brief operator !=
             * @param ptr
             * @return
             */
            bool operator!=(const UniqueId *ptr) const;

            /**
             * @brief detach
             */
            void detach();

            /**
             * @brief checksumCRC16
             * @return
             */
            quint16 checksumCRC16() const;

            /**
             * @brief checksumMD4
             * @return
             */
            const QByteArray & checksumMD4() const;

            /**
             * @brief checksumMD5
             * @return
             */
            const QByteArray & checksumMD5() const;

            /**
             * @brief id
             * @return
             */
            QByteArray id(const QByteArray &seed = QByteArray());

            /**
             * @brief uuid
             * @return
             */
            const QUuid & uuid() const;

            /**
             * @brief isEmpty
             * @return
             */
            bool isEmpty() const;

            /**
             * @brief toJson
             * @return
             */
            QJsonObject toJson() const;

            /**
             * @brief fromJson
             * @param json
             * @return
             */
            static UniqueId fromJson(const QJsonObject &json);

            /**
             * @brief toString
             * @return
             */
            QString toString() const;

            /**
             * @brief toHex
             * @return
             */
            QString toHex() const;

            /**
             * @brief fromString
             * @param str
             * @return
             */
            static UniqueId fromString(const QString &str);

            /**
             * @brief operator <<
             * @param outDataStream
             * @param uniqueId
             * @return
             */
            friend QDataStream &operator<<(QDataStream &outDataStream, const UniqueId & uniqueId);

            /**
             * @brief operator >>
             * @param inDataStream
             * @param uniqueId
             * @return
             */
            friend QDataStream &operator>>(QDataStream &inDataStream, UniqueId & uniqueId);

        private:

            explicit UniqueId(UniqueIdData *d);

            // Serialization

            /**
             * @brief extractFromDataStream
             * @param dataStream
             * @return
             */
            virtual UniqueId_Error_t extractFromDataStream(QDataStream & dataStream);

            /**
             * @brief insertToDataStream
             * @param dataStream
             * @return
             */
            virtual UniqueId_Error_t insertToDataStream(QDataStream & dataStream) const;

        private:

            QExplicitlySharedDataPointer<UniqueIdData> sharedDataP;
};

inline uint qHash(const UniqueId &uniqueId, uint seed = 0)
{
    // Combining hashes of the internal components
    uint h = ::qHash(uniqueId.uuid(), seed);
    h ^= ::qHash(uniqueId.checksumMD4(), seed);
    h ^= ::qHash(uniqueId.checksumMD5(), seed);
    h ^= ::qHash(uniqueId.checksumCRC16(), seed);
    return h;
}

} // utils

Q_DECLARE_METATYPE(utils::UniqueId)
// Q_DECLARE_METATYPE(utils::UniqueId *)
// Q_DECLARE_OPAQUE_POINTER(utils::UniqueId *)

Q_DECLARE_TYPEINFO(utils::UniqueId, Q_MOVABLE_TYPE);

#endif // UNIQUEID_H
