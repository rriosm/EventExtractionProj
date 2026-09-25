
#include "UrlList.h"

QT_BEGIN_NAMESPACE
#include <QSharedData>
QT_END_NAMESPACE

namespace utils {

    class UrlListData : public QSharedData
    {
        public:

            UrlListData() :
                QSharedData()
            {
                this->m_url_Hash_iterator = this->m_Url_Hash.begin();
            }

            virtual ~UrlListData()
            {
                // TODO
            }

            UrlListData(const UrlListData &urlListData) :
                QSharedData(urlListData),
                m_Url_Hash(urlListData.m_Url_Hash)
            {
                int offset = 0;

                UrlList_t::const_iterator start = urlListData.m_Url_Hash.find(urlListData.m_url_Hash_iterator.key());

                for (offset = 0; start != UrlList_t::const_iterator(urlListData.m_url_Hash_iterator); offset++, start++);

                this->m_url_Hash_iterator = this->m_Url_Hash.find(urlListData.m_url_Hash_iterator.key());

                while (offset > 0)
                {
                    this->m_url_Hash_iterator++;
                    offset--;
                }
            }

            const UrlListData &operator=(const UrlListData &rhs)
            {
                if (this != &rhs)
                {
                    this->m_Url_Hash = rhs.m_Url_Hash;

                    int offset = 0;

                    UrlList_t::const_iterator start = rhs.m_Url_Hash.find(rhs.m_url_Hash_iterator.key());

                    for (offset = 0; start != UrlList_t::const_iterator(rhs.m_url_Hash_iterator); offset++, start++);

                    this->m_url_Hash_iterator = this->m_Url_Hash.find(rhs.m_url_Hash_iterator.key());

                    while (offset > 0)
                    {
                        this->m_url_Hash_iterator++;
                        offset--;
                    }
                }

                return ( *this );
            }

            bool operator !=(const UrlListData &rhs) const
            {
                return !(this == &rhs);
            }

            bool operator ==(const UrlListData &rhs) const
            {
                if ( (this->m_Url_Hash == rhs.m_Url_Hash) &&
                    (this->m_url_Hash_iterator == rhs.m_url_Hash_iterator) )
                {
                    return (true);
                }

                return (false);
            }

            bool operator==(const UrlListData *ptr) const
            {
                return (this == ptr);
            }

            bool operator!=(const UrlListData *ptr) const
            {
                return (this != ptr);
            }

            const UrlList_t & url_Hash() const
            {
                return (this->m_Url_Hash);
            }

            void set_Url_Hash(const UrlList_t &new_Url_Hash)
            {
                this->m_Url_Hash = new_Url_Hash;
                this->set_Url_Hash_iterator(this->m_Url_Hash.begin());
            }

            const UrlList_t::iterator & url_Hash_iterator() const
            {
                return (this->m_url_Hash_iterator);
            }

            void set_Url_Hash_iterator(const UrlList_t::iterator &new_Url_Hash_iterator)
            {
                this->m_url_Hash_iterator = new_Url_Hash_iterator;
            }

            void set_Url_Hash_iterator(const QUuid &uuidOfFirstItem)
            {
                UrlList_t::iterator tempIterm = this->m_Url_Hash.end();

                if ( ! uuidOfFirstItem.isNull() )
                {
                    Url * url = this->m_Url_Hash.key(uuidOfFirstItem);

                    // TODO Bug???
                    if ( url )
                    {
                        tempIterm = this->m_Url_Hash.find(url);
                    }
                }

                if (tempIterm == this->m_Url_Hash.end())
                {
                    this->m_url_Hash_iterator = this->m_Url_Hash.begin();
                }
            }

            NamespacedDefs_Error_t insert(Url * const & url, const QUuid & id)
            {
                if ( ( ! url->isEmpty() ) &&
                    url->isValid() )
                {
                    if (this->m_Url_Hash.insert(url, id) != this->m_Url_Hash.end())
                    {
                        return (NamespacedDefs_Error_t::NAMESPACEDEFS_URL_LIST_SUCCESS);
                    }
                }

                return (NamespacedDefs_Error_t::NAMESPACEDEFS_URL_LIST_ERROR);
            }

            NamespacedDefs_Error_t remove(Url *const & url)
            {
                if ( ( ! url->isEmpty() ) &&
                    (url->isValid()))
                {
                    if (this->m_Url_Hash.remove(url))
                    {
                        return (NamespacedDefs_Error_t::NAMESPACEDEFS_URL_LIST_SUCCESS);
                    }
                }

                return (NamespacedDefs_Error_t::NAMESPACEDEFS_URL_LIST_ERROR);
            }

            UrlList_t m_Url_Hash;

            UrlList_t::iterator m_url_Hash_iterator;
    };

    UrlList::UrlList(QObject *parent) :
        QObject(parent),
        sharedDataP(new UrlListData)
    {
        // TODO
    }

    UrlList::~UrlList()
    {
        // TODO
    }

    UrlList::UrlList(const UrlList &urlList)
    {
        if (this != &urlList)
        {
            this->sharedDataP = urlList.sharedDataP;
        }
    }

    bool UrlList::operator !=(const UrlList &right) const
    {
        return !(this == &right);
    }

    bool UrlList::operator ==(const UrlList &right) const
    {
        if ( (this->sharedDataP == right.sharedDataP) ||
            (*this->sharedDataP == *right.sharedDataP) )
        {
            return (true);
        }

        return (false);
    }

    bool UrlList::operator==(const UrlList  *ptr) const
    {
        return (this == ptr);
    }

    bool UrlList::operator!=(const UrlList *ptr) const
    {
        return (this != ptr);
    }

    const UrlList & UrlList::operator =(const UrlList &right)
    {
        if (this != &right)
        {
            this->sharedDataP = right.sharedDataP;
        }

        return (*this);
    }

    UrlList UrlList::detachedCopy()
    {
        UrlList copy(*this);
        copy.sharedDataP.detach();

        return (copy);
    }

    const UrlList_t & UrlList::getList() const
    {
        return (this->sharedDataP->url_Hash());
    }

    void UrlList::setList(const UrlList_t & list)
    {
        this->sharedDataP->set_Url_Hash(list);
    }

    bool UrlList::isEmpty() const
    {
        return (this->sharedDataP->url_Hash().isEmpty());
    }

    qint32 UrlList::size() const
    {
        return (this->sharedDataP->url_Hash().size());
    }

    NamespacedDefs_Error_t UrlList::insert(Url * const & url, const QUuid & id)
    {
        return (this->sharedDataP->insert(url, id));
    }

    NamespacedDefs_Error_t UrlList::remove(Url *const & url)
    {
        return (this->sharedDataP->remove(url));
    }

    // protected

    URL_Error_t UrlList::extractFromDataStream(QDataStream &dataStream)
    {
        if (dataStream.version() < UrlList::dataStreamVersion)
        {
            return (URL_Error_t::URL_Error_Bad_DataStream_Too_Old);
        }

        quint64 dsStreamHeader;
        quint16 dsClassVersion;

        dataStream >> dsStreamHeader >> dsClassVersion;

        if (dsStreamHeader < UrlList::streamHeader)
        {
            return (URL_Error_t::URL_Error_Bad_StreamHeader_Too_Old);
        }

        if (dsClassVersion < UrlList::classVersion)
        {
            return (URL_Error_t::URL_Error_Bad_Class_Version_Too_Old);
        }

        qint32 dsSerializeCode;
        dataStream >> dsSerializeCode;

        if (static_cast<Serialize_t>(dsSerializeCode) == Serialize_t::SERIALIZE_URL_LIST_NO_DATA)
        {
            return (URL_Error_t::URL_No_Data);
        }

        if (static_cast<Serialize_t>(dsSerializeCode) == Serialize_t::SERIALIZE_URL_LIST_DATA)
        {
            dataStream >> dsSerializeCode;

            if (static_cast<Serialize_t>(dsSerializeCode) != Serialize_t::SERIALIZE_URL_LIST_DATA_NUM_ENTRIES)
            {
                return (URL_Error_t::URL_No_Data);
            }

            qint32 numEntries;
            dataStream >> numEntries;

            if (numEntries)
            {
                dataStream >> dsSerializeCode;

                if (static_cast<Serialize_t>(dsSerializeCode) != Serialize_t::SERIALIZE_URL_LIST_DATA_FIRST_ENTRY)
                {
                    return (URL_Error_t::URL_Error_Data_Corrupted);
                }

                QUuid dsUuidOfFirstItem;
                dataStream >> dsUuidOfFirstItem;

                dataStream >> dsSerializeCode;

                if (static_cast<Serialize_t>(dsSerializeCode) != Serialize_t::SERIALIZE_URL_LIST_DATA_ENTRIES_BEGIN)
                {
                    return (URL_Error_t::URL_Error_Data_Corrupted);
                }

                Url dsUrl;
                UrlList_t url_Hash;

                for (int i = 0; i < numEntries; i++)
                {
                    dataStream >> dsSerializeCode;

                    if (static_cast<Serialize_t>(dsSerializeCode) == Serialize_t::SERIALIZE_URL_LIST_DATA_ENTRY)
                    {
                        dataStream >> dsUrl;
                        url_Hash.insert(&dsUrl, dsUrl.getUuid());
                    }
                }

                dataStream >> dsSerializeCode;

                if (dsSerializeCode != static_cast<qint32>(Serialize_t::SERIALIZE_URL_LIST_DATA_ENTRIES_END))
                {
                    return (URL_Error_t::URL_Error_Data_Corrupted);
                }

                dataStream >> dsSerializeCode;

                if (dsSerializeCode != static_cast<qint32>(Serialize_t::SERIALIZE_URL_LIST_DATA_END))
                {
                    return (URL_Error_t::URL_Error_Data_Corrupted);
                }

                if (url_Hash.size() > 0)
                {
                    this->sharedDataP->set_Url_Hash(url_Hash);
                    this->sharedDataP->set_Url_Hash_iterator(dsUuidOfFirstItem);
                }

            } else {
                return (URL_Error_t::URL_No_Data);
            }
        }

        return (URL_Error_t::URL_Success);
    }

    URL_Error_t UrlList::insertToDataStream(QDataStream &dataStream) const
    {
        dataStream.setVersion(UrlList::dataStreamVersion);

        dataStream << UrlList::streamHeader << UrlList::classVersion;

        const UrlList_t url_Hash = this->sharedDataP->url_Hash();
        const UrlList_t::iterator url_Hash_iterator = this->sharedDataP->url_Hash_iterator();
        qint32 numEntries = url_Hash.size();

        if (numEntries)
        {
            dataStream << static_cast<qint32>(Serialize_t::SERIALIZE_URL_LIST_DATA);

            dataStream << static_cast<qint32>(Serialize_t::SERIALIZE_URL_LIST_DATA_NUM_ENTRIES);
            dataStream << numEntries;

            dataStream << static_cast<qint32>(Serialize_t::SERIALIZE_URL_LIST_DATA_FIRST_ENTRY);
            dataStream << url_Hash_iterator.value();

            dataStream << static_cast<qint32>(Serialize_t::SERIALIZE_URL_LIST_DATA_ENTRIES_BEGIN);

            for (UrlList_t::const_iterator cit = url_Hash.constBegin();
                 cit != url_Hash.constEnd();
                 cit++)
            {
                if (cit.key())
                {
                    dataStream << static_cast<qint32>(Serialize_t::SERIALIZE_URL_LIST_DATA_ENTRY);
                    dataStream << *cit.key();
                }
            }

            dataStream << static_cast<qint32>(Serialize_t::SERIALIZE_URL_LIST_DATA_ENTRIES_END);
            dataStream << static_cast<qint32>(Serialize_t::SERIALIZE_URL_LIST_DATA_END);

        } else {
            dataStream << static_cast<qint32>(Serialize_t::SERIALIZE_URL_LIST_NO_DATA);
        }

        return (URL_Error_t::URL_Success);
    }

} // utils
