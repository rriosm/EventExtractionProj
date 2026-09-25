
#include <QByteArray>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QSharedData>

#include "UniqueId.h"
#include "Url.h"
using namespace utils;

#include "inc/ExtractorUtils.h"

#include "inc/FullVideo.h"

class FullVideoData : public QSharedData
{
    public:

        FullVideoData() = default;
        ~FullVideoData() = default;

        FullVideoData(const FullVideoData &other) :
            QSharedData(other),
            m_id(other.m_id),
            m_name(other.m_name),
            m_matchDate(other.m_matchDate),
            m_fileData(other.m_fileData)
        {
            // TODO
        }

        bool operator==(const FullVideoData &other) const
        {
            return ( this->m_id == other.m_id &&
                     this->m_name == other.m_name &&
                     this->m_matchDate == other.m_matchDate &&
                     this->m_filePath == other.m_filePath &&
                     this->m_fileData == other.m_fileData &&
                     this->m_title == other.m_title &&
                     this->m_description == other.m_description &&
                     this->m_uploader == other.m_uploader &&
                     this->m_channel == other.m_channel &&
                     this->m_uploadDate == other.m_uploadDate &&
                     this->m_duration == other.m_duration &&
                     this->m_viewCount == other.m_viewCount &&
                     this->m_likeCount == other.m_likeCount &&
                     this->m_commentCount == other.m_commentCount &&
                     this->m_tags == other.m_tags &&
                     this->m_categories == other.m_categories &&
                     this->m_resolution == other.m_resolution &&
                     this->m_fps == other.m_fps &&
                     this->m_acodec == other.m_acodec &&
                     this->m_vcodec == other.m_vcodec &&
                     this->m_fileSize == other.m_fileSize &&
                     this->m_ext == other.m_ext &&
                     this->m_chapters == other.m_chapters &&
                     this->m_thumbnails == other.m_thumbnails &&
                     this->m_subtitles == other.m_subtitles &&
                     this->m_heatmap == other.m_heatmap);
        }

        UniqueId m_id;
        QString m_name;
        QDateTime m_matchDate;
        QString m_filePath;
        QByteArray m_fileData;

        // Basic Info
        QString m_title;            // The unique 11-character video ID.
        QString m_description;      // The full text in the description box.
        QString m_uploader;         // The unique ID of the uploader.
        QString m_channel;           // The unique ID of the channel.
        QDate m_uploadDate;         // The date it was published (formatted as YYYYMMDD).
        quint64 m_duration;         // The length of the video in seconds.
        quint64 m_viewCount;        // Total number of views.

        // Engagement & Social
        quint16 m_likeCount;        // Number of likes (Note: dislike_count is often unavailable or estimated since YouTube hid it).
        quint64 m_commentCount;     // Number of comments on the video.
        QStringList m_tags;         // A list of keywords the creator added to the video.
        QStringList m_categories;   // The genre (e.g., "Technology", "Gaming").

        // Technical & Stream Details
        QString m_resolution;       // The overall resolution (e.g., 1920x1080).
        QString m_fps;              // Frames per second.
        QString m_acodec;           // The video (e.g., avc1, vp9) codec.
        QString m_vcodec;           // The audio codec.
        quint16 m_fileSize;         // The estimated size of the file in bytes.
        QString m_ext;              // The file extension (e.g., mp4, mkv, webm).

        // Extra Content
        QStringList m_chapters;
        QList<Url> m_thumbnails;
        QStringList m_subtitles;
        QString m_heatmap;
};

void _updateInternalId(FullVideoData* data)
{
    if (data->m_id.isEmpty()) return;

    if (data->m_filePath.isEmpty()) return;
    {
        QByteArray videoSeed = ExtractorUtils::generateContentSeed(data->m_filePath, QCryptographicHash::Md4);

        data->m_id = UniqueId(videoSeed);
    }
}

FullVideo::FullVideo() :
    sharedDataP(new FullVideoData)
{
    // TODO
}

FullVideo::FullVideo(const UniqueId &id,
                     const QString &name,
                     const QDateTime &date,
                     const QByteArray &data) :
    sharedDataP(new FullVideoData)
{
    this->sharedDataP->m_id = id;
    this->sharedDataP->m_name = name;
    this->sharedDataP->m_matchDate = date;
    this->sharedDataP->m_fileData = data;
}

FullVideo::FullVideo(const FullVideo &rhs) :
    sharedDataP(rhs.sharedDataP)
{
    // TODO
}

FullVideo::~FullVideo()
{
    // TODO
}

FullVideo &FullVideo::operator=(const FullVideo &right)
{
    if (*this != right)
    {
        this->sharedDataP.operator=(right.sharedDataP);
    }

    return *this;
}

bool FullVideo::operator==(const FullVideo &right) const
{
    if (this->sharedDataP == right.sharedDataP) return true; // Same memory object

    return (*this->sharedDataP == *right.sharedDataP); // Deep value compare
}

bool FullVideo::operator !=(const FullVideo & right) const
{
    return !(*this == right);
}

bool FullVideo::operator==(const FullVideo *ptr) const
{
    return (this == ptr);
}

bool FullVideo::operator!=(const FullVideo *ptr) const
{
    return (this != ptr);
}

UniqueId FullVideo::id() const
{
    return this->sharedDataP->m_id;
}

void FullVideo::setId(const UniqueId &newId)
{
    if (newId == this->sharedDataP->m_id) return;

    this->sharedDataP.detach();

    this->sharedDataP->m_id = newId;
}

QString FullVideo::name() const
{
    return this->sharedDataP->m_name;
}

void FullVideo::setName(const QString &newName)
{
    this->sharedDataP->m_name = newName;
}

QDateTime FullVideo::matchDate() const
{
    return this->sharedDataP->m_matchDate;
}

void FullVideo::setMatchDate(const QDateTime &date)
{
    this->sharedDataP->m_matchDate = date;
}

QString FullVideo::filePath() const
{
    return this->sharedDataP->m_filePath;
}

void FullVideo::setFilePath(const QString &newFilePath)
{
    if (newFilePath == this->sharedDataP->m_filePath) return;

    this->sharedDataP.detach();

    this->sharedDataP->m_filePath = newFilePath;

    _updateInternalId(this->sharedDataP.data());
}

QByteArray FullVideo::fileData() const
{
    return this->sharedDataP->m_fileData;
}

void FullVideo::setFileData(const QByteArray &data)
{
    this->sharedDataP->m_fileData = data;
}

const QString &FullVideo::title() const
{
    return this->sharedDataP->m_title;
}

void FullVideo::setTitle(const QString &newTitle)
{
    if (newTitle == this->sharedDataP->m_title) return;

    this->sharedDataP->m_title = newTitle;
}

const QString &FullVideo::description() const
{
    return this->sharedDataP->m_description;
}

void FullVideo::setDescription(const QString &newDescription)
{
    if (newDescription == this->sharedDataP->m_description) return;

    this->sharedDataP->m_description = newDescription;
}

const QString &FullVideo::uploader() const
{
    return this->sharedDataP->m_uploader;
}

void FullVideo::setUploader(const QString &newUploader)
{
    if (newUploader == this->sharedDataP->m_uploader) return;

    this->sharedDataP->m_uploader = newUploader;
}

const QString &FullVideo::channel() const
{
    return this->sharedDataP->m_channel;
}

void FullVideo::setChannel(const QString &newChannel)
{
    if (newChannel == this->sharedDataP->m_channel) return;

    this->sharedDataP->m_channel = newChannel;
}

const QDate &FullVideo::uploadDate() const
{
    return this->sharedDataP->m_uploadDate;
}

void FullVideo::setUploadDate(const QDate &newUploadDate)
{
    if (newUploadDate == this->sharedDataP->m_uploadDate) return;

    this->sharedDataP->m_uploadDate = newUploadDate;
}

quint64 FullVideo::duration() const
{
    return this->sharedDataP->m_duration;
}

void FullVideo::setDuration(quint64 newDuration)
{
    if (newDuration == this->sharedDataP->m_duration) return;

    this->sharedDataP->m_duration = newDuration;
}

quint64 FullVideo::viewCount() const
{
    return this->sharedDataP->m_viewCount;
}

void FullVideo::setViewCount(quint64 newViewCount)
{
    if (newViewCount == this->sharedDataP->m_viewCount) return;

    this->sharedDataP->m_viewCount = newViewCount;
}

quint16 FullVideo::likeCount() const
{
    return this->sharedDataP->m_likeCount;
}

void FullVideo::setLikeCount(quint16 newLikeCount)
{
    if (newLikeCount == this->sharedDataP->m_likeCount) return;

    this->sharedDataP->m_likeCount = newLikeCount;
}

quint64 FullVideo::commentCount() const
{
    return this->sharedDataP->m_commentCount;
}

void FullVideo::setCommentCount(quint64 newCommentCount)
{
    if (newCommentCount == this->sharedDataP->m_commentCount) return;

    this->sharedDataP->m_commentCount = newCommentCount;
}

const QStringList &FullVideo::tags() const
{
    return this->sharedDataP->m_tags;
}

void FullVideo::setTags(const QStringList &newTags)
{
    if (newTags == this->sharedDataP->m_tags) return;

    this->sharedDataP->m_tags = newTags;
}

const QStringList &FullVideo::categories() const
{
    return this->sharedDataP->m_categories;
}

void FullVideo::setCategories(const QStringList &newCategories)
{
    if (newCategories == this->sharedDataP->m_categories) return;

    this->sharedDataP->m_categories = newCategories;
}

const QString &FullVideo::resolution() const
{
    return this->sharedDataP->m_resolution;
}

void FullVideo::setResolution(const QString &newResolution)
{
    if (newResolution == this->sharedDataP->m_resolution) return;

    this->sharedDataP->m_resolution = newResolution;
}

const QString &FullVideo::fps() const
{
    return this->sharedDataP->m_fps;
}

void FullVideo::setFps(const QString &newFps)
{
    if (newFps == this->sharedDataP->m_fps) return;

    this->sharedDataP->m_fps = newFps;
}

const QString &FullVideo::acodec() const
{
    return this->sharedDataP->m_acodec;
}

void FullVideo::setAcodec(const QString &newAcodec)
{
    if (newAcodec == this->sharedDataP->m_acodec) return;

    this->sharedDataP->m_acodec = newAcodec;
}

const QString &FullVideo::vcodec() const
{
    return this->sharedDataP->m_vcodec;
}

void FullVideo::setVcodec(const QString &newVcodec)
{
    if (newVcodec == this->sharedDataP->m_vcodec) return;

    this->sharedDataP->m_vcodec = newVcodec;
}

quint16 FullVideo::fileSize() const
{
    return this->sharedDataP->m_fileSize;
}

void FullVideo::setFileSize(quint16 newFileSize)
{
    if (newFileSize == this->sharedDataP->m_fileSize) return;

    this->sharedDataP->m_fileSize = newFileSize;
}

const QString &FullVideo::ext() const
{
    return this->sharedDataP->m_ext;
}

void FullVideo::setExt(const QString &newExt)
{
    if (newExt == this->sharedDataP->m_ext) return;

    this->sharedDataP->m_ext = newExt;
}

const QStringList &FullVideo::chapters() const
{
    return this->sharedDataP->m_chapters;
}

void FullVideo::setChapters(const QStringList &newChapters)
{
    if (newChapters == this->sharedDataP->m_chapters) return;

    this->sharedDataP->m_chapters = newChapters;
}

const QList<Url> &FullVideo::thumbnails() const
{
    return this->sharedDataP->m_thumbnails;
}

void FullVideo::setThumbnails(const QList<Url> &newThumbnails)
{
    if (newThumbnails == this->sharedDataP->m_thumbnails) return;

    this->sharedDataP->m_thumbnails = newThumbnails;
}

const QStringList &FullVideo::subtitles() const
{
    return this->sharedDataP->m_subtitles;
}

void FullVideo::setSubtitles(const QStringList &newSubtitles)
{
    if (newSubtitles == this->sharedDataP->m_subtitles) return;

    this->sharedDataP->m_subtitles = newSubtitles;
}

const QString &FullVideo::heatmap() const
{
    return this->sharedDataP->m_heatmap;
}

void FullVideo::setHeatmap(const QString &newHeatmap)
{
    if (newHeatmap == this->sharedDataP->m_heatmap) return;

    this->sharedDataP->m_heatmap = newHeatmap;
}

FullVideo FullVideo::fromJson(const QJsonObject &json)
{
    FullVideo video;

    if (json.contains("id"))
        video.setId(UniqueId::fromJson(json["id"].toObject()));

    video.setName(json["name"].toString());

    // DateTime
    if (json.contains("match_date"))
    {
        video.setMatchDate(QDateTime::fromString(json["match_date"].toString(), Qt::ISODate));
    }

    // Deserialize Binary Data
    if (json.contains("file_data_base64"))
    {
        QByteArray base64Data = json["file_data_base64"].toString().toLatin1();
        video.setFileData(QByteArray::fromBase64(base64Data));
    }

    if (json.contains("title"))
        video.setTitle(json["title"].toString());

    if (json.contains("description"))
        video.setTitle(json["description"].toString());

    if (json.contains("uploader"))
        video.setUploader(json["uploader"].toString());

    if (json.contains("channel"))
        video.setChannel(json["channel"].toString());

    if (json.contains("upload_date"))
        video.setUploadDate(QDate::fromString(json["upload_date"].toString()));

    if (json.contains("duration"))
        video.setDuration(json["duration"].toString().toULongLong());

    if (json.contains("view_count"))
        video.setViewCount(json["view_count"].toString().toULongLong());

    if (json.contains("comment_count"))
        video.setCommentCount(json["comment_count"].toString().toULongLong());

    if (json.contains("tags"))
    {
        QStringList tagsList;

        QJsonArray tagsArr = json["tags"].toArray();

        for (const QJsonValue &value : qAsConst(tagsArr))
        {
            if (value.isString())
                tagsList.append(value.toString());
        }

        if (!tagsList.isEmpty())
            video.setTags(tagsList);
    }

    if (json.contains("categories"))
    {
        QStringList categoriesList;

        QJsonArray categoriesArr = json["categories"].toArray();

        for (const QJsonValue &value : qAsConst(categoriesArr))
        {
            if (value.isString())
                categoriesList.append(value.toString());
        }

        if (!categoriesList.isEmpty())
            video.setCategories(categoriesList);
    }

    if (json.contains("resolution"))
        video.setResolution(json["resolution"].toString());

    if (json.contains("fps"))
        video.setFps(json["fps"].toString());

    if (json.contains("acodec"))
        video.setAcodec(json["acodec"].toString());

    if (json.contains("vcodec"))
        video.setVcodec(json["vcodec"].toString());

    if (json.contains("file_size"))
        video.setFileSize(json["file_size"].toString().toULongLong());

    if (json.contains("ext"))
        video.setExt(json["ext"].toString());

    if (json.contains("chapters"))
    {
        QStringList chaptersList;

        QJsonArray chaptersArr = json["chapters"].toArray();

        for (const QJsonValue &value : qAsConst(chaptersArr))
        {
            if (value.isString())
                chaptersList.append(value.toString());
        }

        if (!chaptersList.isEmpty())
            video.setChapters(chaptersList);
    }

    if (json.contains("thumbnails"))
    {
        QList<Url> thumbnailsList;

        QJsonArray thumbnailsArr = json["thumbnails"].toArray();

        for (const QJsonValue &value : qAsConst(thumbnailsArr))
        {
            if (value.isString())
            {
                Url url(value.toString());
                thumbnailsList.append(url);
            }
        }

        if (!thumbnailsList.isEmpty())
            video.setThumbnails(thumbnailsList);
    }

    if (json.contains("subtitles"))
    {
        QStringList subtitlesList;

        QJsonArray subtitlesArr = json["subtitles"].toArray();

        for (const QJsonValue &value : qAsConst(subtitlesArr))
        {
            if (value.isString())
                subtitlesList.append(value.toString());
        }

        if (!subtitlesList.isEmpty())
            video.setSubtitles(subtitlesList);
    }

    if (json.contains("heatmap"))
        video.setHeatmap(json["heatmap"].toString());

    return video;
}

QJsonObject FullVideo::toJson() const
{
    QJsonObject obj;
    obj["id"] = this->sharedDataP->m_id.toString();
    obj["name"] = this->sharedDataP->m_name;

    // DateTime as ISO String
    if (this->sharedDataP->m_matchDate.isValid())
    {
        obj["match_date"] = this->sharedDataP->m_matchDate.toString(Qt::ISODate);
    }

    // Serialize Binary Data as Base64 String
    if (!this->sharedDataP->m_fileData.isEmpty())
    {
        obj["file_data_base64"] = QString::fromLatin1(this->sharedDataP->m_fileData.toBase64());
    }

    if ( ! this->sharedDataP->m_title.isEmpty())
        obj["title"] = this->sharedDataP->m_title;

    if ( ! this->sharedDataP->m_description.isEmpty())
        obj["description"] = this->sharedDataP->m_description;

    if ( ! this->sharedDataP->m_uploader.isEmpty())
        obj["uploader"] = this->sharedDataP->m_uploader;

    if ( ! this->sharedDataP->m_channel.isEmpty())
        obj["channel"] = this->sharedDataP->m_channel;

    if ( ! this->sharedDataP->m_uploadDate.isNull())
        obj["upload_date"] = this->sharedDataP->m_uploadDate.toString(Qt::ISODate);

    obj["duration"] = QString("%1").arg(this->sharedDataP->m_duration);

    obj["view_count"] = QString("%1").arg(this->sharedDataP->m_viewCount);

    obj["comment_count"] = QString("%1").arg(this->sharedDataP->m_commentCount);

    if ( ! this->sharedDataP->m_tags.isEmpty() )
        obj["tags"] = QJsonArray::fromStringList(this->sharedDataP->m_tags);

    if ( ! this->sharedDataP->m_categories.isEmpty())
        obj["categories"] = QJsonArray::fromStringList(this->sharedDataP->m_categories);

    if ( ! this->sharedDataP->m_resolution.isEmpty())
        obj["resolution"] = this->sharedDataP->m_resolution;

    if ( ! this->sharedDataP->m_fps.isEmpty())
        obj["fps"] = this->sharedDataP->m_fps;

    if ( ! this->sharedDataP->m_acodec.isEmpty())
        obj["acodec"] = this->sharedDataP->m_acodec;

    if ( ! this->sharedDataP->m_vcodec.isEmpty())
        obj["vcodec"] = this->sharedDataP->m_vcodec;

    obj["file_size"] = QString("%1").arg(this->sharedDataP->m_title);

    if ( ! this->sharedDataP->m_ext.isEmpty())
        obj["ext"] = this->sharedDataP->m_ext;

    if ( ! this->sharedDataP->m_chapters.isEmpty())
        obj["chapters"] = QJsonArray::fromStringList(this->sharedDataP->m_chapters);

    QJsonArray thumbnailsArr;

    for (QList<Url>::const_iterator cit = this->sharedDataP->m_thumbnails.constBegin();
         cit != this->sharedDataP->m_thumbnails.constEnd();
         cit++)
    {
        QJsonObject partObj;
        partObj["url"] = cit->toString();

        thumbnailsArr.append(partObj);
    }

    obj["thumbnails"] = thumbnailsArr;

    if ( ! this->sharedDataP->m_subtitles.isEmpty())
        obj["subtitles"] = QJsonArray::fromStringList(this->sharedDataP->m_subtitles);

    if ( ! this->sharedDataP->m_heatmap.isEmpty())
        obj["heatmap"] = this->sharedDataP->m_heatmap;

    return obj;
}
