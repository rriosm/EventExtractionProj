#ifndef VIDEOSEGMENTMODEL_H
#define VIDEOSEGMENTMODEL_H

#include <QAbstractListModel>

#include "VideoSegmentInfo.h"

enum Column {
    Id = 0,
    StartTime,
    Duration,
    Status,
    ColumnCount
};

class VideoSegmentModel : public QAbstractListModel
{
        Q_OBJECT

        Q_CLASSINFO("Author", "author")
        Q_CLASSINFO("Version", "1.0.1")
        Q_CLASSINFO("Status", "Active")
        Q_CLASSINFO("Description", "")

        /**
         * @brief classVersion
         */
        static const quint16 classVersion = quint16(101);

    public:

        /**
         * @brief The SegmentRoles enum
         */
        enum SegmentRoles {
                IdRole = Qt::UserRole + 1,
                PathRole,
                StartTimeRole,
                EndTimeRole,
                DurationRole,
                StateRole
            };

        /**
         * @brief VideoSegmentModel
         * @param parent
         */
        explicit VideoSegmentModel(QObject *parent = nullptr);

        /**
         * @brief clone
         * @return
         */
        QSharedPointer<VideoSegmentModel> clone() const;

        /**
         * @brief rowCount
         * @param parent
         * @return
         */
        int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

        /**
         * @brief columnCount
         * @param parent
         * @return
         */
        int columnCount(const QModelIndex &parent) const Q_DECL_OVERRIDE;

        /**
         * @brief data
         * @param index
         * @param role
         * @return
         */
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

        /**
         * @brief headerData
         * @param section
         * @param orientation
         * @param role
         * @return
         */
        QVariant headerData(int section, Qt::Orientation orientation, int role) const Q_DECL_OVERRIDE;

        /**
         * @brief roleNames
         * @return
         */
        QHash<int, QByteArray> roleNames() const Q_DECL_OVERRIDE;

        /**
         * @brief segments
         * @return
         */
        const VideoSegmentInfo::VideoSegmentInfoList_t& segments() const;

        /**
         * @brief setSegments: Method to update data from the Worker
         * @param segments
         */
        void setSegments(const VideoSegmentInfo::VideoSegmentInfoList_t &segments);

    public slots:
        /**
         * @brief Updates the model with new results.
         * Because of QSharedDataPointer, this assignment is O(1) complexity.
         */
        void updateResults(const VideoSegmentInfo::VideoSegmentInfoList_t &newResults);

        /**
         * @brief clear
         */
        void clear();

    private:

        VideoSegmentInfo::VideoSegmentInfoList_t m_segments;
};

Q_DECLARE_METATYPE(VideoSegmentModel *)
Q_DECLARE_OPAQUE_POINTER(VideoSegmentModel *)

#endif // VIDEOSEGMENTMODEL_H
