#ifndef AUDIOMODEL_H
#define AUDIOMODEL_H

#include <QAbstractListModel>
#include <QSharedPointer>

#include "inc/Audio.h"

// Note: If you don't have an AudioSegmentInfo class and are using the main Audio
// class directly, replace AudioSegmentInfo with Audio throughout this model.

class AudioModel : public QAbstractListModel
{
        Q_OBJECT

        Q_CLASSINFO("Author", "author")
        Q_CLASSINFO("Version", "1.0.1")
        Q_CLASSINFO("Status", "Active")
        Q_CLASSINFO("Description", "List model for Audio Segments for QTableView/QML")

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
         * @brief AudioModel
         * @param parent
         */
        explicit AudioModel(QObject *parent = nullptr);

        /**
         * @brief clone
         * @return
         */
        QSharedPointer<AudioModel> clone() const;

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
        int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

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
        const Audio::AudioList_t& segments() const;

        /**
         * @brief setSegments: Method to update data from the Worker
         * @param segments
         */
        void setSegments(const Audio::AudioList_t &segments);

    public slots:

        /**
         * @brief Updates the model with new results.
         * Because of QSharedDataPointer, this assignment is O(1) complexity.
         */
        void updateResults(const Audio::AudioList_t &newResults);

        /**
         * @brief clear
         */
        void clear();

    private:

        Audio::AudioList_t m_segments;
};

Q_DECLARE_METATYPE(AudioModel *)
Q_DECLARE_OPAQUE_POINTER(AudioModel *)

#endif // AUDIOMODEL_H
