
#include <QGlobalStatic>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QDir>

#include "inc/LogUtils.h"

#include "inc/AudioAnnotationSegmentationWorker.h"

AudioAnnotationSegmentationWorker::AudioAnnotationSegmentationWorker(QObject *parent)
    : QObject{parent},
    m_clipSec(0.00)
{
    this->m_annotationManifest.clear();
    this->m_annotationClipsOutputDir.clear();
}

AudioAnnotationSegmentationWorker::~AudioAnnotationSegmentationWorker()
{
    // TODO
}

bool AudioAnnotationSegmentationWorker::operator ==(const AudioAnnotationSegmentationWorker &right) const
{
    if ((this->m_clipSec != right.m_clipSec) ||
        (this->m_annotationManifest != right.m_annotationManifest) ||
        (this->m_annotationClipsOutputDir != right.m_annotationClipsOutputDir) )
    {
        return false;
    }

    return true;
}

bool AudioAnnotationSegmentationWorker::operator !=(const AudioAnnotationSegmentationWorker &right) const
{
    return !(*this == right);
}

bool AudioAnnotationSegmentationWorker::operator==(const AudioAnnotationSegmentationWorker *ptr) const
{
    return (this == ptr);
}

bool AudioAnnotationSegmentationWorker::operator!=(const AudioAnnotationSegmentationWorker *ptr) const
{
    return (this != ptr);
}


qreal AudioAnnotationSegmentationWorker::clipSec() const
{
    QMutexLocker l(&this->m_mutexClipSec);

    return this->m_clipSec;
}

void AudioAnnotationSegmentationWorker::setClipSec(qreal newClipSec)
{
    QMutexLocker l(&this->m_mutexClipSec);

    if (newClipSec == this->m_clipSec) return;

    this->m_clipSec = newClipSec;
}

Url AudioAnnotationSegmentationWorker::annotationManifest() const
{
    QMutexLocker l(&this->m_mutexAnnotationManifest);

    return this->m_annotationManifest;
}

void AudioAnnotationSegmentationWorker::setAnnotationManifest(const Url &newAnnotationManifest)
{
    QMutexLocker l(&this->m_mutexAnnotationManifest);

    if (newAnnotationManifest == this->m_annotationManifest) return;

    this->m_annotationManifest = newAnnotationManifest;
}

QString AudioAnnotationSegmentationWorker::annotationClipsOutputDir() const
{
    QMutexLocker l(&this->m_mutexAnnotationClipsOutputDir);

    return this->m_annotationClipsOutputDir;
}

void AudioAnnotationSegmentationWorker::setAnnotationClipsOutputDir(const QString &newAnnotationClipsOutputDir)
{
    QMutexLocker l(&this->m_mutexAnnotationClipsOutputDir);

    if (newAnnotationClipsOutputDir == this->m_annotationClipsOutputDir) return;

    this->m_annotationClipsOutputDir = newAnnotationClipsOutputDir;
}

void AudioAnnotationSegmentationWorker::segmentJsonContent(const QString &annotationJsonPath)
{
    if (annotationJsonPath.isEmpty())
    {

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("AudioAnnotationSegmentationWorker::generateJsonManifest(...) - Aborting: Annotation path is empty."),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        emit finished(-1);

        return;
    }

    if (qFuzzyIsNull(this->clipSec()) || (this->clipSec() < 0.00))
    {
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("AudioAnnotationSegmentationWorker::generateJsonManifest(...) - Aborting: Invalid clip duration: %1.")
                              .arg(this->clipSec()),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        emit finished(-2);

        return;
    }

    QDir outputDir(this->annotationClipsOutputDir());

    if (!outputDir.exists())
    {
        if (outputDir.mkpath("."))
        {
#ifdef QT_DEBUG
            LogUtils::safeLog(QString("AudioAnnotationSegmentationWorker::segmentJsonContent(...) - Output directory created successfully: %1")
                                  .arg(this->annotationClipsOutputDir()),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
#endif

        } else {
#ifdef QT_DEBUG
            LogUtils::safeLog(QString("AudioAnnotationSegmentationWorker::segmentJsonContent(...) - JFailed to create directory: %1")
                                  .arg(this->annotationClipsOutputDir()),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
#endif

            emit finished(-3);

            return;
        }

    } else {
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("AudioAnnotationSegmentationWorker::segmentJsonContent(...) - Output directory already exists!"),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
    }

    // Load and Parse Deepgram JSON
    QFile inputFile(annotationJsonPath);
    if (!inputFile.open(QIODevice::ReadOnly))
    {
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("AudioAnnotationSegmentationWorker::segmentJsonContent(...) - Aborting: Could not open Annotation file: %1.")
                              .arg(annotationJsonPath),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        emit finished(-4);

        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(inputFile.readAll());
    inputFile.close();

    // Navigate to the words array: results -> channels[0] -> alternatives[0] -> words
    QJsonObject rootObj = doc.object();
    QJsonObject results = rootObj.value("results").toObject();
    QJsonArray channels = results.value("channels").toArray();

    if (channels.isEmpty())
    {
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("AudioAnnotationSegmentationWorker::segmentJsonContent(...) - Aborting: JSON channels element is empty."),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        emit finished(-5);

        return;
    }

    QJsonObject firstChannel = channels.at(0).toObject();
    QJsonArray alternatives = firstChannel.value("alternatives").toArray();

    if (alternatives.isEmpty())
    {
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("AudioAnnotationSegmentationWorker::segmentJsonContent(...) - Aborting: JSON alternatives element is empty."),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        emit finished(-6);

        return;
    }

    QJsonObject firstAlt = alternatives.at(0).toObject();
    QJsonArray words = firstAlt.value("words").toArray();

    int segmentDurationMs = this->clipSec() * 1000;
    int currentSegmentIndex = -1;
    QJsonArray currentSegmentWords;

    auto saveCurrentSegment = [&]() {

        // 1. Silently ignore the initial startup transition
        if (currentSegmentIndex == -1) {
            return;
        }

        if (currentSegmentWords.isEmpty())
        {
#ifdef QT_DEBUG
            LogUtils::safeLog(QString("AudioAnnotationSegmentationWorker::segmentJsonContent(...) - Aborting: Skipping empty segment...."),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
#endif
            return;
        }

        QString fileName = QString("segment_%1.json").arg(currentSegmentIndex, 3, 10, QChar('0'));
        QFile outFile(QDir(outputDir).filePath(fileName));

        if (outFile.open(QIODevice::WriteOnly)) {
            QJsonObject root;
            root.insert("words", currentSegmentWords);
            outFile.write(QJsonDocument(root).toJson());
            outFile.close();
        }

        currentSegmentWords = QJsonArray();
    };

    for (const QJsonValue &wordValue : qAsConst(words))
    {
        QJsonObject wordObj = wordValue.toObject();
        double startSec = wordObj["start"].toDouble();
        int startMs = static_cast<int>(startSec * 1000);

        int segmentIndex = startMs / segmentDurationMs;
        int offsetMs = segmentIndex * segmentDurationMs;

        // If we move to a new segment, save the previous one
        if (segmentIndex != currentSegmentIndex)
        {
            saveCurrentSegment();
            currentSegmentIndex = segmentIndex;
        }

        // Adjust timestamps relative to the segment start (Parity with VTT logic)
        QJsonObject adjustedWord = wordObj;
        adjustedWord.insert("start_relative", (startMs - offsetMs) / 1000.0);
        adjustedWord.insert("end_relative", (static_cast<int>(wordObj["end"].toDouble() * 1000) - offsetMs) / 1000.0);

        currentSegmentWords.append(adjustedWord);
    }

    // Save the final segment
    saveCurrentSegment();

    // Generate the JSON manifest for these segments
    this->generateJsonManifest();
}

void AudioAnnotationSegmentationWorker::generateJsonManifest()
{
    QDir dir(this->annotationClipsOutputDir());

    // Filter for the specific segment naming pattern: segment_***.json
    QStringList filters;
    filters << "segment_*.json";
    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files, QDir::Unsorted);

    QMap<qint32, QFileInfo> sortedFileMap;
    QRegularExpression regex("segment_(\\d+)\\.json"); // Relaxed to \\d+ to support varying padding digits safely

    for (const QFileInfo &fileInfo : qAsConst(fileList))
    {
        QRegularExpressionMatch match = regex.match(fileInfo.fileName());

        if (match.hasMatch())
        {
            qint32 segmentIndex = match.captured(1).toInt();
            sortedFileMap.insert(segmentIndex, fileInfo);
        }
    }

    QJsonArray segmentsArray;
    int totalSegments = sortedFileMap.size();

    // Iterate through the numerically sorted collection
    QMap<qint32, QFileInfo>::const_iterator cit = sortedFileMap.constBegin();
    qint32 segmentIndex;

    while (cit != sortedFileMap.constEnd())
    {
        segmentIndex = cit.key();
        const QFileInfo &fileInfo = cit.value();

        int startTime = segmentIndex * this->clipSec();
        int endTime = startTime + this->clipSec();

        QJsonObject segmentObj;
        segmentObj.insert("segment_index", segmentIndex);
        segmentObj.insert("file_name", fileInfo.fileName());
        segmentObj.insert("file_path", fileInfo.absoluteFilePath());

        QJsonObject timing;
        timing.insert("startTimeSec", startTime);
        timing.insert("endTimeSec", endTime);
        segmentObj.insert("timing", timing);

        segmentsArray.append(segmentObj);
        ++cit;
    }

    QJsonObject root;
    root.insert("exportTimestamp", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    root.insert("segmentDurationSec", this->clipSec());
    root.insert("totalSegments", totalSegments);
    root.insert("segments", segmentsArray);

    QString finalManifestLocalPath;

    if (this->annotationManifest().isEmpty())
    {
        finalManifestLocalPath = dir.filePath("deepgram_segmentation_manifest.json");

        this->setAnnotationManifest(finalManifestLocalPath);
    } else {

        // Strip file:// prefix if the URL class appended it, so QFile can read it
        finalManifestLocalPath = this->annotationManifest().toString();

        if (finalManifestLocalPath.startsWith("file://"))
        {
            finalManifestLocalPath.remove("file://");
        }

        QFileInfo fInfo(finalManifestLocalPath);

        QDir manifestDir(fInfo.absoluteDir());

        if (!manifestDir.exists())
        {
            if (manifestDir.mkpath("."))
            {
#ifdef QT_DEBUG
                LogUtils::safeLog(QString("AudioAnnotationSegmentationWorker::generateJsonManifest() - Output directory created successfully: %1")
                                      .arg(manifestDir.path()),
                                  __FILE__,
                                  __LINE__,
                                  Q_FUNC_INFO);
#endif

            } else {
#ifdef QT_DEBUG
                LogUtils::safeLog(QString("AudioAnnotationSegmentationWorker::generateJsonManifest() - Failed to create directory: %1")
                                      .arg(manifestDir.path()),
                                  __FILE__,
                                  __LINE__,
                                  Q_FUNC_INFO);
#endif

                emit finished(-7);

                return;
            }
        } else {
#ifdef QT_DEBUG
            LogUtils::safeLog(QString("AudioAnnotationSegmentationWorker::generateJsonManifest() - Manifest directory already exists!"),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
#endif
        }

    }

    // Save the manifest file
    QFile outFile(finalManifestLocalPath);

    if (outFile.open(QIODevice::WriteOnly))
    {
        outFile.write(QJsonDocument(root).toJson());
        outFile.close();

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("AudioAnnotationSegmentationWorker::generateJsonManifest() - JSON manifest generated at: %1")
                              .arg(finalManifestLocalPath),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        emit finished(0);
    }
    else
    {
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("AudioAnnotationSegmentationWorker::generateJsonManifest() - JSON manifest generated at: - FATAL: Could not open output file for writing: %1")
                              .arg(finalManifestLocalPath),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        emit finished(-8);
    }
}
