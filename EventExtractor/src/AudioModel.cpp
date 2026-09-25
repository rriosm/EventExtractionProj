#include "inc/AudioModel.h"

AudioModel::AudioModel(QObject *parent) :
    QAbstractListModel(parent)
{
    // TODO
}

QSharedPointer<AudioModel> AudioModel::clone() const
{
    auto newInstance = QSharedPointer<AudioModel>::create();

    // Manually copy your data fields here
    newInstance->updateResults(this->segments());

    return newInstance;
}

int AudioModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;

    return this->m_segments.count();
}

int AudioModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;

    return 6; // ID, Start, End, Duration, State (plus Path)
}

QVariant AudioModel::data(const QModelIndex &index, int role) const
{
    if ( ! index.isValid() ||
        (index.row() >= m_segments.size()))
    {
        return QVariant();
    }

    // Access via .at() to maintain the "Const" shared data pointer (prevents unnecessary detaching)
    const Audio &segmentInfo = m_segments.at(index.row());

    // 1. Handle DisplayRole (What the QTableView cells actually show)
    if (role == Qt::DisplayRole)
    {
        switch (index.column())
        {
        // Assuming UniqueId has a toString() or toHex() method
        case 0: return segmentInfo.id().toHex();

        case 1: return segmentInfo.filePath();

        case 2: return QString::number(segmentInfo.startTime(), 'f', 2); // Format to 2 decimals

        case 3: return QString::number(segmentInfo.endTime(), 'f', 2);

        case 4: return QString("%1s").arg(segmentInfo.duration(), 0, 'f', 2);

        case 5: return segmentInfo.state();

        default: return QVariant();
        }
    }

    // 2. Handle TextAlignmentRole (Center numbers for better look)
    if (role == Qt::TextAlignmentRole)
    {
        if ( (index.column() >= 2) &&
            (index.column() <= 4) ) // Times and Duration
        {
            return Qt::AlignCenter;
        }
    }

    // 3. Custom roles (for QML or custom delegates)
    switch (role)
    {
    case IdRole:        return segmentInfo.id().toHex();
    case PathRole:      return segmentInfo.filePath();
    case StartTimeRole: return segmentInfo.startTime();
    case EndTimeRole:   return segmentInfo.endTime();
    case DurationRole:  return segmentInfo.duration();
    case StateRole:     return segmentInfo.state();
    }

    return QVariant();
}

QVariant AudioModel::headerData(int section, Qt::Orientation orientation, int role) const
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

QHash<int, QByteArray> AudioModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[IdRole] = "segmentId";
    roles[PathRole] = "path";
    roles[StartTimeRole] = "startTime";
    roles[EndTimeRole] = "endTime";
    roles[DurationRole] = "duration";
    roles[StateRole] = "state";

    return roles;
}

const Audio::AudioList_t& AudioModel::segments() const
{
    return this->m_segments;
}

void AudioModel::setSegments(const Audio::AudioList_t &segments)
{
    beginResetModel();

    this->m_segments = segments; // Implicit sharing makes this copy very cheap

    endResetModel();
}

void AudioModel::updateResults(const Audio::AudioList_t &newResults)
{
    this->beginResetModel();

    // This assignment increments the reference count of the internal QSharedData.
    // No actual strings or doubles are copied here!
    this->m_segments = newResults;

    this->endResetModel();
}

void AudioModel::clear()
{
    beginResetModel();
    this->m_segments.clear();
    endResetModel();
}
