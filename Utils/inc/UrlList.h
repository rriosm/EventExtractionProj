#ifndef URLLIST_H
#define URLLIST_H

#include "Utils_global.h"

#include "Url.h"
#include "NamespaceDefs.h"

QT_BEGIN_NAMESPACE
#include <QObject>
#include <QDataStream>
#include <QHash>
#include <QExplicitlySharedDataPointer>
QT_END_NAMESPACE

namespace utils {

    typedef QHash<Url *, QUuid> UrlList_t;

    class UTILSSHARED_EXPORT UrlListData;

    class UTILSSHARED_EXPORT UrlList : public QObject
    {
        Q_OBJECT

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

        // friend declarations

        /**
         * @brief operator <<
         * @param outDataStream
         * @param urlList
         * @return
         */
        friend QDataStream &operator<<(QDataStream &outDataStream, const UrlList & urlList)
        {
            urlList.insertToDataStream(outDataStream);

            return (outDataStream);
        }

        /**
         * @brief operator >>
         * @param inDataStream
         * @param urlList
         * @return
         */
        friend QDataStream &operator>>(QDataStream &inDataStream, UrlList & urlList)
        {
            urlList.extractFromDataStream(inDataStream);

            return (inDataStream);
        }

        public:

            /**
             * @brief UrlList
             * @param parent
             */
            explicit UrlList(QObject *parent = nullptr);

            /**
             * @brief ~UrlList
             */
            virtual ~UrlList();

            /**
             * @brief UrlList
             * @param urlList
             */
            UrlList(const UrlList &urlList);

            /**
             * @brief operator !=
             * @param right
             * @return
             */
            bool operator !=(const UrlList &right) const;

            /**
             * @brief operator ==
             * @param right
             * @return
             */
            bool operator ==(const UrlList &right) const;

            /**
             * @brief operator ==
             * @param ptr
             * @return
             */
            bool operator==(const UrlList  *ptr) const;

            /**
             * @brief operator !=
             * @param ptr
             * @return
             */
            bool operator!=(const UrlList  *ptr) const;

            /**
             * @brief operator =
             * @param right
             * @return
             */
            const UrlList &operator =(const UrlList &right);

            /**
             * @brief detachedCopy
             * @return
             */
            UrlList detachedCopy();

            /**
             * @brief getList
             * @return
             */
            const UrlList_t & getList() const;

            /**
             * @brief setList
             * @param list
             */
            void setList(const UrlList_t & list);

            /**
             * @brief isEmpty
             * @return
             */
            bool isEmpty() const;

            /**
             * @brief size
             * @return
             */
            qint32 size() const;

            /**
             * @brief addUrl
             * @param url
             */
            NamespacedDefs_Error_t insert(Url * const & url, const QUuid & id);

            /**
             * @brief removeUrl
             * @param url
             * @return
             */
            NamespacedDefs_Error_t remove(Url * const & url);

        signals:

        private:

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

            QExplicitlySharedDataPointer<UrlListData> sharedDataP;
    };

} // utils

Q_DECLARE_METATYPE(utils::UrlList)
Q_DECLARE_METATYPE(utils::UrlList *)

Q_DECLARE_OPAQUE_POINTER(utils::UrlList *)

Q_DECLARE_TYPEINFO(utils::UrlList, Q_MOVABLE_TYPE);

#endif // URLLIST_H
