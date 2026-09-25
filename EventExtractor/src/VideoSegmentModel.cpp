#include "VideoSegmentModel.h"

#include "VideoSegmentModel.h"

//#include <QColor>

VideoSegmentModel::VideoSegmentModel(QObject *parent) :
    QAbstractListModel(parent)
{
    // TODO
}

QSharedPointer<VideoSegmentModel> VideoSegmentModel::clone() const
{
    auto newInstance = QSharedPointer<VideoSegmentModel>::create();

    // Manually copy your data fields here
    newInstance->updateResults(this->segments());

    return newInstance;
}

int VideoSegmentModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;

    return this->m_segments.count();
}

int VideoSegmentModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;

    return 6; // ID, Start, End, Duration, State
}

QVariant VideoSegmentModel::data(const QModelIndex &index, qint32 role) const
{
    if ( ! index.isValid() ||
         (index.row() >= m_segments.size()))
    {
        return QVariant();
    }

    // Access via .at() to maintain the "Const" shared data pointer (prevents unnecessary detaching)
    const VideoSegmentInfo &segmentInfo = m_segments.at(index.row());

    // 2. Handle DisplayRole (What the QTableView cells actually show)
    if (role == Qt::DisplayRole)
    {
        switch (index.column())
        {
            case 0: return segmentInfo.id().toString();

            case 1: return segmentInfo.filePath();

            case 2: return QString::number(segmentInfo.startTime(), 'f', 2); // Format to 2 decimals

            case 3: return QString::number(segmentInfo.endTime(), 'f', 2);

            case 4: return QString("%1s").arg(segmentInfo.duration(), 0, 'f', 2);

            case 5: return segmentInfo.state();

            default: return QVariant();
        }
    }

//    // 3. NEW: Highlighting Logic for "CORRUPT" or "GAP"
//    if (role == Qt::BackgroundRole)
//    {
//        if (segmentInfo.state() == "CORRUPT") {
//            return QColor(255, 200, 200); // Light Red background
//        } else if (segmentInfo.state() == "GAP")
//        {
//            return QColor(255, 255, 224); // Light Yellow for Gaps
//        }
//    }

//    if (role == Qt::ForegroundRole)
//    {
//        if (segmentInfo.state() == "CORRUPT") {
//            return QColor(Qt::red); // Red text
//        }
//    }

//    // 4. Optional: Add a ToolTip to explain the failure
//    if (role == Qt::ToolTipRole)
//    {
//        if (segmentInfo.state() == "CORRUPT") {
//            return tr("This segment is corrupted or incomplete and will be ignored.");
//        }
//    }

    // 5. Handle TextAlignmentRole (Optional: Center numbers for better look)
    if (role == Qt::TextAlignmentRole)
    {
        if ( (index.column() >= 2) &&
             (index.column() <= 4) ) // Times and Duration
        {
            return Qt::AlignCenter;
        }
    }

    // Custom roles (for QML or custom delegates)
    switch (role)
    {
        case IdRole:        return segmentInfo.id().toString();
        case PathRole:      return segmentInfo.filePath();
        case StartTimeRole: return segmentInfo.startTime();
        case EndTimeRole:   return segmentInfo.endTime();
        case DurationRole:  return segmentInfo.duration();
        case StateRole:     return segmentInfo.state();
    }

    return QVariant();
}

QVariant VideoSegmentModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch (section)
        {
            case 0: return "ID";
            case 1: return "File Path";
            case 2: return "Start (s)";
            case 3: return "End (s)";
            case 4: return "Duration";
            case 5: return "Status";
        }
    }

    // Center the headers for numerical columns
    if (role == Qt::TextAlignmentRole && section >= 2 && section <= 4) {
        return Qt::AlignCenter;
    }

    return QVariant();
}

QHash<int, QByteArray> VideoSegmentModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[IdRole] = "segmentId";
    roles[PathRole] = "path";
    roles[StartTimeRole] = "startTime";
    roles[DurationRole] = "duration";
    roles[StateRole] = "state";

    return roles;
}

const VideoSegmentInfo::VideoSegmentInfoList_t& VideoSegmentModel::segments() const
{
    return this->m_segments;
}

void VideoSegmentModel::setSegments(const VideoSegmentInfo::VideoSegmentInfoList_t &segments)
{
    beginResetModel();

    this->m_segments = segments; // Implicit sharing makes this copy very cheap

    endResetModel();
}


void VideoSegmentModel::updateResults(const VideoSegmentInfo::VideoSegmentInfoList_t &newResults)
{
    this->beginResetModel();

    // This assignment increments the reference count of the internal QSharedData.
    // No actual strings or doubles are copied here!
    this->m_segments = newResults;

    this->endResetModel();
}

void VideoSegmentModel::clear()
{
    beginResetModel();
    this->m_segments.clear();
    endResetModel();
}
