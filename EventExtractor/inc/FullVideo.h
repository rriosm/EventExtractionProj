#ifndef FULLVIDEO_H
#define FULLVIDEO_H

#include <QByteArray>
#include <QDateTime>
#include <QJsonObject>
#include <QMetaType>
#include <QSharedDataPointer>

#include "UniqueId.h"
#include "Url.h"
using namespace utils;

class FullVideoData;

class FullVideo
{
        Q_GADGET

        Q_CLASSINFO("Author", "author")
        Q_CLASSINFO("Version", "1.0.1")
        Q_CLASSINFO("Status", "Active")
        Q_CLASSINFO("Description", "")

        Q_PROPERTY(UniqueId id READ id WRITE setId)
        Q_PROPERTY(QString name READ name WRITE setName)
        Q_PROPERTY(QDateTime matchDate READ matchDate WRITE setMatchDate)
        Q_PROPERTY(QByteArray fileData READ fileData WRITE setFileData)

    public:

        /**
         * @brief FullVideo
         */
        FullVideo();

        /**
         * @brief FullVideo
         * @param id
         * @param name
         * @param date
         * @param data
         */
        FullVideo(const UniqueId &id,
                  const QString &name,
                  const QDateTime &date,
                  const QByteArray &data);

        /**
         * @brief FullVideo
         * @param rhs
         */
        FullVideo(const FullVideo &rhs);

        /**
         * @brief ~FullVideo
         */
        virtual ~FullVideo();

        /**
         * @brief operator =
         * @param rhs
         * @return
         */
        FullVideo &operator=(const FullVideo &rhs);

        /**
         * @brief operator ==
         * @param rhs
         * @return
         */
        bool operator==(const FullVideo &rhs) const;

        /**
         * @brief operator !=
         * @param right
         * @return
         */
        bool operator !=(const FullVideo & right) const;

        /**
         * @brief operator ==
         * @param ptr
         * @return
         */
        bool operator==(const FullVideo *ptr) const;

        /**
         * @brief operator !=
         * @param ptr
         * @return
         */
        bool operator!=(const FullVideo *ptr) const;

        /**
         * @brief id
         * @return
         */
        UniqueId id() const;

        /**
         * @brief setId
         * @param newId
         */
        void setId(const UniqueId &newId);

        /**
         * @brief name
         * @return
         */
        QString name() const;

        /**
         * @brief setName
         * @param n
         */
        void setName(const QString &n);

        /**
         * @brief matchDate
         * @return
         */
        QDateTime matchDate() const;

        /**
         * @brief setMatchDate
         * @param date
         */
        void setMatchDate(const QDateTime &date);

        /**
         * @brief filePath
         * @return
         */
        QString filePath() const;

        /**
         * @brief setFilePath
         * @param newFilePath
         */
        void setFilePath(const QString &newFilePath);

        /**
         * @brief fileData
         * @return
         */
        QByteArray fileData() const;

        /**
         * @brief setFileData
         * @param data
         */
        void setFileData(const QByteArray &data);

        /**
         * @brief title
         * @return
         */
        const QString &title() const;

        /**
         * @brief setTitle
         * @param newTitle
         */
        void setTitle(const QString &newTitle);

        /**
         * @brief description
         * @return
         */
        const QString &description() const;

        /**
         * @brief setDescription
         * @param newDescription
         */
        void setDescription(const QString &newDescription);

        /**
         * @brief uploaderl
         * @return
         */
        const QString &uploader() const;

        /**
         * @brief setUploader
         * @param newUploaderChannel
         */
        void setUploader(const QString &newUploaderChannel);

        /**
         * @brief uploaderl
         * @return
         */
        const QString &channel() const;

        /**
         * @brief setUploader
         * @param newUploaderChannel
         */
        void setChannel(const QString &newChannel);

        /**
         * @brief uploadDate
         * @return
         */
        const QDate &uploadDate() const;

        /**
         * @brief setUploadDate
         * @param newUploadDate
         */
        void setUploadDate(const QDate &newUploadDate);

        /**
         * @brief duration
         * @return
         */
        quint64 duration() const;

        /**
         * @brief setDuration
         * @param newDuration
         */
        void setDuration(quint64 newDuration);

        /**
         * @brief viewCount
         * @return
         */
        quint64 viewCount() const;

        /**
         * @brief setViewCount
         * @param newViewCount
         */
        void setViewCount(quint64 newViewCount);

        /**
         * @brief likeCount
         * @return
         */
        quint16 likeCount() const;

        /**
         * @brief setLikeCount
         * @param newLikeCount
         */
        void setLikeCount(quint16 newLikeCount);

        /**
         * @brief commentCount
         * @return
         */
        quint64 commentCount() const;

        /**
         * @brief setCommentCount
         * @param newCommentCount
         */
        void setCommentCount(quint64 newCommentCount);

        /**
         * @brief tags
         * @return
         */
        const QStringList &tags() const;

        /**
         * @brief setTags
         * @param newTags
         */
        void setTags(const QStringList &newTags);

        /**
         * @brief categories
         * @return
         */
        const QStringList &categories() const;

        /**
         * @brief setCategories
         * @param newCategories
         */
        void setCategories(const QStringList &newCategories);

        /**
         * @brief resolution
         * @return
         */
        const QString &resolution() const;

        /**
         * @brief setResolution
         * @param newResolution
         */
        void setResolution(const QString &newResolution);

        /**
         * @brief fps
         * @return
         */
        const QString &fps() const;

        /**
         * @brief setFps
         * @param newFps
         */
        void setFps(const QString &newFps);

        /**
         * @brief acodec
         * @return
         */
        const QString &acodec() const;

        /**
         * @brief setAcodec
         * @param newAcodec
         */
        void setAcodec(const QString &newAcodec);

        /**
         * @brief vcodec
         * @return
         */
        const QString &vcodec() const;

        /**
         * @brief setVcodec
         * @param newVcodec
         */
        void setVcodec(const QString &newVcodec);

        /**
         * @brief fileSize
         * @return
         */
        quint16 fileSize() const;

        /**
         * @brief setFileSize
         * @param newFileSize
         */
        void setFileSize(quint16 newFileSize);

        /**
         * @brief ext
         * @return
         */
        const QString &ext() const;

        /**
         * @brief setExt
         * @param newExt
         */
        void setExt(const QString &newExt);

        /**
         * @brief chapters
         * @return
         */
        const QStringList &chapters() const;

        /**
         * @brief setChapters
         * @param newChapters
         */
        void setChapters(const QStringList &newChapters);

        /**
         * @brief thumbnails
         * @return
         */
        const QList<Url> &thumbnails() const;

        /**
         * @brief setThumbnails
         * @param newThumbnails
         */
        void setThumbnails(const QList<Url> &newThumbnails);

        /**
         * @brief subtitles
         * @return
         */
        const QStringList &subtitles() const;

        /**
         * @brief setSubtitles
         * @param newSubtitles
         */
        void setSubtitles(const QStringList &newSubtitles);

        /**
         * @brief heatmap
         * @return
         */
        const QString &heatmap() const;

        /**
         * @brief setHeatmap
         * @param newHeatmap
         */
        void setHeatmap(const QString &newHeatmap);

        /**
         * @brief fromJson
         * @param json
         * @return
         */
        static FullVideo fromJson(const QJsonObject &json);

        /**
         * @brief toJson
         * @return
         */
        QJsonObject toJson() const;

    private:

        QSharedDataPointer<FullVideoData> sharedDataP;

};

Q_DECLARE_METATYPE(FullVideo)

#endif // FULLVIDEO_H
