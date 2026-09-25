#ifndef NAMESPACEDEFS_H
#define NAMESPACEDEFS_H

#include "Utils_global.h"

QT_BEGIN_NAMESPACE
#include <QObject>
#include <QString>
#include <QUuid>
QT_END_INCLUDE_NAMESPACE

namespace utils {

    enum class UTILSSHARED_EXPORT NamespacedDefs_Error_e {
        NAMESPACEDEFS_URL_LIST_ERROR = 0x40100,
        NAMESPACEDEFS_URL_ERROR = 0x40101,
        NAMESPACEDEFS_URL_SUCCESS = 0x40102,
        NAMESPACEDEFS_URL_LIST_SUCCESS = 0x40103
    };

    typedef NamespacedDefs_Error_e NamespacedDefs_Error_t;

    inline bool operator==(const NamespacedDefs_Error_t type1, const NamespacedDefs_Error_t type2)
    {
        return (static_cast<quint64> (type1) == static_cast<quint64> (type2));
    }

    inline uint qHash(const NamespacedDefs_Error_t type, uint seed = 0)
    {
        return (::qHash(static_cast<quint64> (type), seed));
    }

    enum class UTILSSHARED_EXPORT Serialize_e {
        SERIALIZE_URL_DATA = 0X40200,
        SERIALIZE_URL_DATA_END = 0X40201,
        SERIALIZE_URL_NO_DATA = 0X40202,
        SERIALIZE_URL_LIST_DATA = 0X40203,
        SERIALIZE_URL_LIST_DATA_NUM_ENTRIES = 0X40204,
        SERIALIZE_URL_LIST_DATA_FIRST_ENTRY = 0X40205,
        SERIALIZE_URL_LIST_DATA_ENTRY = 0X40206,
        SERIALIZE_URL_LIST_DATA_ENTRIES_BEGIN = 0X40207,
        SERIALIZE_URL_LIST_DATA_ENTRIES_END = 0X40208,
        SERIALIZE_URL_LIST_DATA_END = 0X40209,
        SERIALIZE_URL_LIST_NO_DATA = 0X40210,

        SERIALIZE_UNIQUEID_DATA = 0x40300,
        SERIALIZE_UNIQUEID_DATA_END = 0X40301,
        SERIALIZE_UNIQUEID_NO_DATA = 0X40302
    };

    typedef Serialize_e Serialize_t;

    inline bool operator==(const Serialize_t type1, const Serialize_t type2)
    {
        return (static_cast<quint64> (type1) == static_cast<quint64> (type2));
    }

    inline uint qHash(const Serialize_t type, uint seed = 0)
    {
        return (::qHash(static_cast<quint64> (type), seed));
    }

    class UTILSSHARED_EXPORT NamespaceDefs : public QObject
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

            NamespaceDefs(QObject *parent = nullptr) :
                QObject(parent)
            {
                // TODO
            }

            virtual ~NamespaceDefs()
            {
                // TODO
            }

            // Base URI prefix
            static const QString & BASE_RESOURCE_NAMESPACE_URI()
            {
                static const QString BASE_RESOURCE_Namespace_URI = "http://www.telemidia.puc-rio.br/nemo/resource/";

                return (BASE_RESOURCE_Namespace_URI);
            }

            static const QString & BASE_RESOURCE_NAMESPACE_PREFIX()
            {
                static const QString BASE_RESOURCE_Namespace_PREFIX = "resource";

                return (BASE_RESOURCE_Namespace_PREFIX);
            }

            // Default namespaces
            // This ontology must be imported.
            static const QString & EXIF_RDF_NAMESPACE_URI()
            {
                static const QString EXIF_RDF_Namespace_URI = "http://www.kanzaki.com/ns/exif";

                return(EXIF_RDF_Namespace_URI);
            }

            static const QString & EXIF_RDF_NAMESPACE_PREFIX()
            {
                static const QString EXIF_RDF_Namespace_Prefix = "exif";

                return (EXIF_RDF_Namespace_Prefix);
            }

            static const QString & FOAF_RDF_NAMESPACE_URI()
            {
                static const QString FOAF_RDF_Namespace_URI= "http://xmlns.com/foaf/0.1/";

                return (FOAF_RDF_Namespace_URI);
            }

            static const QString & FOAF_RDF_NAMESPACE_PREFIX()
            {
                static const QString FOAF_RDF_Namespace_Prefix= "foaf";

                return (FOAF_RDF_Namespace_Prefix);
            }

            static const QString & ONT_ANNOT_RDF_NAMESPACE_URI()
            {
                static const QString ONT_ANNOT_RDF_Namespace_URI= "http://purl.org/net/ns/ontology-annot#";

                return (ONT_ANNOT_RDF_Namespace_URI);
            }

            static const QString & ONT_ANNOT_RDF_NAMESPACE_PREFIX()
            {
                static const QString ONT_ANNOT_RDF_Namespace_Prefix= "ont";

                return (ONT_ANNOT_RDF_Namespace_Prefix);
            }

            static const QString & OWL_NAMESPACE_URI()
            {
                static const QString OWL_Namespace_URI = "http://www.w3.org/2002/07/owl#";

                return (OWL_Namespace_URI);
            }

            static const QString & OWL_NAMESPACE_PREFIX()
            {
                static const QString OWL_Namespace_OWL = "owl";

                return (OWL_Namespace_OWL);
            }

            // UUID default namespaces. According to RFC 4122

            /**
             * @brief UUID_Nil: Name string is a fully-qualified domain name.
             * @return
             */
            static const QUuid & UUID_Nil()
            {
                static const QUuid Uuid_Nil = " ";

                return (Uuid_Nil);
            }

            /**
             * Name string is a fully-qualified domain name.
             */
            static const QUuid & UUID_NameSpace_DNS()
            {
                static const QUuid Uuid_NameSpace_DNS = "6ba7b810-9dad-11d1-80b4-00c04fd430c8";

                return (Uuid_NameSpace_DNS);
            }

            /**
            * Name string is a URL.
            */
            static const QUuid & UUID_NameSpace_URL()
            {
                static const QUuid Uuid_NameSpace_URL = "6ba7b811-9dad-11d1-80b4-00c04fd430c8";

                return (Uuid_NameSpace_URL);
            }

            /**
             * Name string is an ISO OID.
             */
            static const QUuid & UUID_NameSpace_OID()
            {
                static const QUuid Uuid_NameSpace_OID = "6ba7b812-9dad-11d1-80b4-00c04fd430c8";

                return (Uuid_NameSpace_OID);
            }

            /**
             * Name string is an X.500 DN (in DER or a text output format).
             */
            static const QUuid & UUID_NameSpace_X500()
            {
                static const QUuid Uuid_NameSpace_X500 = "6ba7b814-9dad-11d1-80b4-00c04fd430c8";

                return (Uuid_NameSpace_X500);
            }
    };

} // utils

#endif // NAMESPACEDEFS_H
