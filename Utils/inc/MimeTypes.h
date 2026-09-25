#ifndef MIMETYPES_H
#define MIMETYPES_H

#include "Utils_global.h"

QT_BEGIN_NAMESPACE
#include <QMimeData>
#include <QMimeType>
#include <QMimeDatabase>
#include <QPair>
#include <QString>
QT_END_NAMESPACE

namespace utils {

    // TODO Must be synchronized with QMimeDatabase and QMimeType
    /**
         * @brief The MIMETypes enum
         */
    enum class UTILSSHARED_EXPORT MIMETypes_e {
        DEFAULT_NONE = 0x3003E8,
        NEMo_AUDIO = 0x3004E20,
        NEMo_AUDIO_MIDI_MID = 0x4300E21,
        NEMo_AUDIO_MIDI_MIDI = 0x3004E22,
        NEMo_AUDIO_MIDI_KAR = 0x3004E23,
        NEMo_AUDIO_MIDI_RMI = 0x3004E24,
        NEMo_AUDIO_MP4_MP4A = 0x3004E25,
        NEMo_AUDIO_MP4A_LATM = 0x3004E26,
        NEMo_AUDIO_MP4A_LATM_M4A = 0x3004E27,
        NEMo_AUDIO_MP4A_LATM_M4P = 0x3004E28,
        NEMo_AUDIO_OGG_OGA = 0x3004E29,
        NEMo_AUDIO_OGG_OGG = 0x3004E2A,
        NEMo_AUDIO_OGG_SPX = 0x3004E2B,
        NEMo_AUDIO_X_AAC_AAC = 0x3004E2C,
        NEMo_AUDIO_X_AIFF_AIF = 0x3004E2D,
        NEMo_AUDIO_X_AIFF_AIFF = 0x3004E2E,
        NEMo_AUDIO_X_AIFF_AIFC = 0x3004E2F,
        NEMo_AUDIO_MPEGURL = 0x3004E30,
        NEMo_AUDIO_MPEGURL_8 = 0x3004E31,
        NEMo_AUDIO_X_MPEGURL = 0x3004E32,
        NEMo_AUDIO_X_MPEGURL_8 = 0x3004E33,
        NEMo_AUDIO_X_MPEGURL_M3U = 0x3004E34,
        NEMo_AUDIO_X_MPEGURL_M3U_8 = 0x3004E35,
        NEMo_AUDIO_X_MS_WMA_WMA = 0x3004E36,
        NEMo_AUDIO_X_PN_REALAUDIO_RAM = 0x3004E37,
        NEMo_AUDIO_X_PN_REALAUDIO_RA = 0x3004E38,
        NEMo_AUDIO_X_WAV_WAV = 0x3004E40,
        NEMo_APPLICATION_MPEGURL = 0x3004E41,
        NEMo_APPLICATION_MPEGURL_8 = 0x3004E42,
        NEMo_APPLICATION_X_MPEGURL = 0x3004E43,
        NEMo_APPLICATION_X_MPEGURL_8 = 0x3004E44,
        NEMo_VND_APPLE_MPEGURL = 0x3004E45,
        NEMo_VND_APPLE_MPEGURL_AUDIO = 0x3004E46,
        NEMo_IMAGE = 0x3005208,
        NEMo_IMAGE_BMP_BMP = 0x3005209,
        NEMo_IMAGE_GIF_GIF = 0x300520A,
        NEMo_IMAGE_JPEG_JPEG = 0x300520B,
        NEMo_IMAGE_JPEG_JPG = 0x300520C,
        NEMo_IMAGE_JPEG_JPE = 0x300520D,
        NEMo_IMAGE_PNG_PNG = 0x300520E,
        NEMo_IMAGE_TIFF_TIFF = 0x300520f,
        NEMo_IMAGE_TIFF_TIF = 0x3005210,
        NEMo_VIDEO = 0x30055F0,
        NEMo_VIDEO_H264_H264 = 0x30055F1,
        NEMo_VIDEO_JPEG_JPGV = 0x30055F2,
        NEMo_VIDEO_MP4_MP4 = 0x30055F3,
        NEMo_VIDEO_MP4_MP4V = 0x30055F4,
        NEMo_VIDEO_MP4_MPG4 = 0x30055F5,
        NEMo_VIDEO_MP4_M4V = 0x30055F6,
        NEMo_VIDEO_MPEG_MPEG = 0x30055F7,
        NEMo_VIDEO_MPEG_MPG = 0x30055F8,
        NEMo_VIDEO_MPEG_MPE = 0x30055F9,
        NEMo_VIDEO_MPEG_M1V = 0x30055FA,
        NEMo_VIDEO_MPEG_M2V = 0x30055FB,
        NEMo_VIDEO_QUICKTIME_QT = 0x30055FC,
        NEMo_VIDEO_QUICKTIME_MOV = 0x30055FD,
        NEMo_VIDEO_X_F4V_F4V = 0x30055FE,
        NEMo_VIDEO_X_FLI_FLI = 0x30055FF,
        NEMo_VIDEO_X_FLV_FLV = 0x3005600,
        NEMo_VIDEO_X_M4V_M4V = 0x3005601,
        NEMo_VIDEO_X_MS_WMV_WMV = 0x3005602,
        NEMo_VIDEO_X_MSVIDEO_AVI = 0x3005603,
        NEMo_TEXT = 0x30059D8,
        NEMo_TEXT_HTML = 0x30059D9,
        NEMo_TEXT_NEMo_NEL = 0x30059DA,
        NEMo_TEXT_OWL = 0x30059DB,
        NEMo_TEXT_RDF = 0x30059DC,
        NEMo_TEXT_SMIL = 0x30059DD,
        NEMo_TEXT_SPARQL_QUERY = 0x30059DE,
        NEMo_TEXT_SPARQL_RESULTS = 0x30059DF,
        NEMo_TEXT_TURTLE = 0x30059E0,
        NEMo_TEXT_TXT = 0x30059E1,
        NEMo_TEXT_XML = 0x30059E2,
        NEMo_MUSIC = 0x3005DC0,
        NEMo_MUSIC_KAR = 0x3005DC1,
        NEMo_MUSIC_MID = 0x3005DC2,
        NEMo_MUSIC_XMID = 0x3005DC3,
        NEMo_MUSIC_MIDI = 0x3005DC4,
        NEMo_MUSIC_XMIDI = 0x3005DC5,
        NEMo_NEMo = 0x63001A0,
        NEMo_NEMo_NAR = 0x30061A2, // NEMo narrative binary format
        NEMo_NEMo_NEL = 0x30061A3,
        NEATOOL_DEFAULT_FILE = 0x3007000,
        NEATOOL_ANNOTATION_FILE = 0X3007001,
        NEATOOL_DATABASE_FILE = 0X3007002,
        NEATOOL_EXPERIENTIAL_OBJECT_FILE = 0X3007003,
        NEATOOL_FILE = 0X3007004,
        NEATOOL_ANNOTATION_SPECIFICATION_FILE = 0X3007005,
        NEATOOL_DATABASE_SPECIFICATION_FILE = 0X3007006,
        NEATOOL_EXPERIENTIAL_OBJECT_SPECIFICATION_FILE = 0X3007007,
        NEATOOL_NARRATIVE_SPECIFICATION_FILE = 0X3007008,

        NEATOOL_PROJECT_FILE = 0X3007009
    };

    typedef MIMETypes_e MIMETypes_t;

    inline bool operator==(const MIMETypes_t type1, const MIMETypes_t type2)
    {
        return (static_cast<quint64> (type1) == static_cast<quint64> (type2));
    }

    inline uint qHash(const MIMETypes_t type, uint seed = 0)
    {
        return (::qHash(static_cast<quint64> (type), seed));
    }

    /**
         * @brief NEMoMIMEPair_t
         */
    typedef QPair<QString, QString> NEMoMIMEPair_t;

    /**
         * @brief NEMoMIMETable_t
         */
    typedef QHash<MIMETypes_t, NEMoMIMEPair_t> NEMoMIMETable_t;

    class UTILSSHARED_EXPORT MIMETypesUtil : virtual public QMimeDatabase
    {
        Q_CLASSINFO("Author", "author")
        Q_CLASSINFO("Version", "1.0.1")
        Q_CLASSINFO("Status", "Active")

        /**
             * @brief classVersion
             */
        static const quint16 classVersion = quint16(101);

        public:

            /**
             * @brief instance
             * @return
             */
            static MIMETypesUtil *instance();

            /**
             * @brief ~MIMETypes
             */
            virtual ~MIMETypesUtil();

            /**
             * @brief getNEMoMIMETypePair
             * @param nmt
             * @return
             */
            NEMoMIMEPair_t getNEMoMIMETypePair(const MIMETypes_t nmt) const;

            /**
             * @brief getMIMEType
             * @param nmt
             * @return
             */
            QString getMIMETypeStr(MIMETypes_t nmt) const;

            /**
             * @brief getMIMETypeExtension
             * @param nmt
             * @return
             */
            QString getMIMETypeExtension(MIMETypes_t nmt) const;

            /**
             * @brief getNEMoMIMETypeByMime
             * @param type
             * @return
             */
            MIMETypes_t getNEMoMIMETypeByMime(const QString &mime);

            /**
             * @brief getNEMoMIMETypeByExtension
             * @param ext
             * @return
             */
            MIMETypes_t getNEMoMIMETypeByExtension(const QString ext);

        signals:

        public slots:

        private:

            /**
             * @brief MIMETypes
             */
            MIMETypesUtil();

            /**
             * @brief disableLogFile
             */
            void disableLogFile();

        private:

            NEMoMIMETable_t mimeTypesHashTable;

            void initNEMoMIMETypesTable();

            static MIMETypesUtil *_instance;
    };

} // utils

#endif // MIMETYPES_H
