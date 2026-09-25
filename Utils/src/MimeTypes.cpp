
#include "MimeTypes.h"

#include "Logger.h"
using namespace logger;

namespace utils {

    MIMETypesUtil * MIMETypesUtil::_instance = nullptr;

    MIMETypesUtil::MIMETypesUtil()
    {
        this->initNEMoMIMETypesTable();
    }

    MIMETypesUtil::~MIMETypesUtil()
    {
        // TODO

        this->disableLogFile();

        // TODO Check if it is needed to delete _instance to avoid memory leak.
        //  if (MIMETypesUtil::_instance)
        //  {
        //      delete MIMETypesUtil::_instance;
        //      MIMETypesUtil::_instance = nullptr;
        //  }
    }

    MIMETypesUtil *MIMETypesUtil::instance()
    {
        if ( ! MIMETypesUtil::_instance )
        {
            MIMETypesUtil::_instance = new MIMETypesUtil();
        }

        return (MIMETypesUtil::_instance);
    }

    void MIMETypesUtil::initNEMoMIMETypesTable()
    {
        this->mimeTypesHashTable.insert(MIMETypes_t::DEFAULT_NONE, qMakePair(QString("default/none"), QString("none")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_AUDIO_MIDI_MIDI, qMakePair(QString("audio/midi"), QString("midi")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_AUDIO_MIDI_KAR, qMakePair(QString("audio/midi"), QString("kar")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_AUDIO_MIDI_RMI, qMakePair(QString("audio/midi"), QString("rmi")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_AUDIO_MP4_MP4A, qMakePair(QString("audio/mp4"), QString("mp4a")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_AUDIO_MP4A_LATM_M4P, qMakePair(QString("audio/mp4a-latm"), QString("m4p")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_AUDIO_X_AAC_AAC, qMakePair(QString("audio/x-aac"), QString("aac")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_AUDIO_X_AIFF_AIF, qMakePair(QString("audio/x-aiff"), QString("aif")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_APPLICATION_MPEGURL_8, qMakePair(QString("application/mpegurl"), QString("m3u8")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_APPLICATION_X_MPEGURL_8, qMakePair(QString("application/x-mpegurl"), QString("m3u8")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_AUDIO_MPEGURL_8, qMakePair(QString("audio/mpegurl"), QString("m3u8")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_AUDIO_X_MPEGURL_8, qMakePair(QString("audio/x-mpegurl"), QString("m3u8")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_AUDIO_X_MPEGURL_M3U_8, qMakePair(QString("audio/x-m3u8"), QString("m3u8")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_VND_APPLE_MPEGURL, qMakePair(QString("application/vnd.apple.mpegurl"), QString("m3u8")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_VND_APPLE_MPEGURL_AUDIO, qMakePair(QString("application/vnd.apple.mpegurl.audio"), QString("m3u8")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_AUDIO_X_PN_REALAUDIO_RAM, qMakePair(QString("audio/x-pn-realaudio"), QString("ram")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_AUDIO_X_PN_REALAUDIO_RA, qMakePair(QString("audio/x-pn-realaudio"), QString("ra")));

        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_IMAGE_JPEG_JPG, qMakePair(QString("image/jpeg"), QString("jpg")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_IMAGE_JPEG_JPE, qMakePair(QString("image/jpeg"), QString("jpe")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_IMAGE_TIFF_TIFF, qMakePair(QString("image/tiff"), QString("tiff")));

        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_VIDEO_H264_H264, qMakePair(QString("video/h264"), QString("h264")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_VIDEO_JPEG_JPGV, qMakePair(QString("video/jpeg"), QString("jpgv")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_VIDEO_MP4_MP4, qMakePair(QString("video/mp4"), QString("mp4")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_VIDEO_MP4_MP4V, qMakePair(QString("video/mp4"), QString("mp4v")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_VIDEO_MP4_MPG4, qMakePair(QString("video/mp4"), QString("mpg4")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_VIDEO_MP4_M4V, qMakePair(QString("video/mp4"), QString("m4v")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_VIDEO_MPEG_MPG, qMakePair(QString("video/mpeg"), QString("mpg")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_VIDEO_MPEG_MPE, qMakePair(QString("video/mpeg"), QString("mpe")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_VIDEO_MPEG_M1V, qMakePair(QString("video/mpeg"), QString("m1v")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_VIDEO_MPEG_M2V, qMakePair(QString("video/mpeg"), QString("m2v")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_VIDEO_X_F4V_F4V, qMakePair(QString("video/x-f4v"), QString("f4v")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_VIDEO_X_M4V_M4V, qMakePair(QString("video/x-m4v"), QString("m4v")));

        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_TEXT_SMIL, qMakePair(QString("application/smil+xml"), QString("smil")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_TEXT_SPARQL_QUERY, qMakePair(QString("application/sparql-query"), QString("rq")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_TEXT_SPARQL_RESULTS, qMakePair(QString("application/sparql-results+xml"), QString("srx")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_TEXT_HTML, qMakePair(QString("text/html"), QString("html")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_TEXT_NEMo_NEL, qMakePair(QString("text/nel+xml"), QString("nel")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_TEXT_OWL, qMakePair(QString("application/rdf+xml"), QString("owl")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_TEXT_RDF, qMakePair(QString("application/rdf+xml"), QString("rdf")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_TEXT_TURTLE, qMakePair(QString("text/turtle"), QString("turtle")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_TEXT_TXT, qMakePair(QString("text/plain"), QString("txt")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_TEXT_XML, qMakePair(QString("text/xml"), QString("xml")));

        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_NEMo_NAR, qMakePair(QString("application/nar"), QString("nar")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEMo_NEMo_NEL, qMakePair(QString("application/nar"), QString("nel")));

        this->mimeTypesHashTable.insert(MIMETypes_t::NEATOOL_PROJECT_FILE, qMakePair(QString("application/npf"), QString("npf")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEATOOL_DATABASE_FILE, qMakePair(QString("application/nndb"), QString("ndb")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEATOOL_ANNOTATION_FILE, qMakePair(QString("application/naf"), QString("naf")));
        this->mimeTypesHashTable.insert(MIMETypes_t::NEATOOL_EXPERIENTIAL_OBJECT_FILE, qMakePair(QString("application/neo"), QString("neo")));
    }

    NEMoMIMEPair_t MIMETypesUtil::getNEMoMIMETypePair(const MIMETypes_t nmt) const
    {
        return (this->mimeTypesHashTable.value(nmt));
    }

    QString MIMETypesUtil::getMIMETypeStr(MIMETypes_t nmt) const
    {
        return (this->mimeTypesHashTable.value(nmt).first);
    }

    QString MIMETypesUtil::getMIMETypeExtension(MIMETypes_t nmt) const
    {
        return (this->mimeTypesHashTable.value(nmt).second);
    }

    MIMETypes_t MIMETypesUtil::getNEMoMIMETypeByMime(const QString &mime)
    {
        MIMETypes_t type_t = MIMETypes_t::DEFAULT_NONE;

        NEMoMIMETable_t::const_iterator cit = this->mimeTypesHashTable.constBegin();

        while (cit != this->mimeTypesHashTable.constEnd())
        {
            if ( (static_cast<NEMoMIMEPair_t> (cit.value())).first == mime)
            {
                type_t = (static_cast<MIMETypes_t> (cit.key()));
                break;
            }

            ++cit;
        }

        return (type_t);
    }

    MIMETypes_t MIMETypesUtil::getNEMoMIMETypeByExtension(const QString ext)
    {
        Logger::instance()->log(QtDebugMsg,
                                QString("NEMoMIMETypes::getNEMoMIMETypeByExtension(const QString) - Entered!"),
                                __FILE__,
                                __LINE__,
                                Q_FUNC_INFO);

        MIMETypes_t type_t = MIMETypes_t::DEFAULT_NONE;

        NEMoMIMETable_t::const_iterator cit = this->mimeTypesHashTable.constBegin();

        Logger::instance()->log(QtDebugMsg,
                                QString("NEMoMIMETypes::getNEMoMIMETypeByExtension(const QString) - MIME extension: %1!").arg(ext),
                                __FILE__,
                                __LINE__,
                                Q_FUNC_INFO);

        while (cit != this->mimeTypesHashTable.constEnd())
        {
            if ( (static_cast<NEMoMIMEPair_t>(cit.value())).second == ext)
            {
                Logger::instance()->log(QtDebugMsg,
                                        QString("NEMoMIMETypes::getNEMoMIMETypeByExtension(const QString) - Type found: %1!").arg(ext),
                                        __FILE__,
                                        __LINE__,
                                        Q_FUNC_INFO);

                type_t = (static_cast<MIMETypes_t>(cit.key()));

                break;
            }

            ++cit;
        }

        return (type_t);
    }

    void MIMETypesUtil::disableLogFile()
    {
        // TODO
    }

} // utils
