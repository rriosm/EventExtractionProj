
#include <QDebug>
#include <QFile>
#include <QMutexLocker>

#include "inc/ManifestSynchronizer.h"

#include "inc/LogUtils.h"

enum ManifestBit {
    VideoBit      = 1 << 0, // 0001
    AudioBit      = 1 << 1, // 0010
    AnnotationBit = 1 << 2, // 0100
    VttBit        = 1 << 3, // 1000
    AllBits       = 0xF     // 1111 (Required result)
};

ManifestSynchronizer::ManifestSynchronizer(PipelineWorker * parent) :
    PipelineWorker(parent),
    m_synchronizedSegmentCount(0)
{
    // this->m_annotationPath.clear();
    // this->m_audioPath.clear();
    this->m_outputPath.clear();
    // this->m_videoPath.clear();
    // this->m_vttPath.clear();
}

ManifestSynchronizer::~ManifestSynchronizer()
{
    // TODO
}

bool ManifestSynchronizer::synchronizeManifests(QVector<FileInfo> allData)
{
#ifdef QT_DEBUG
    LogUtils::safeLog(QString("ManifestSynchronizer::synchronizeManifests() - Synchronization starting!"),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    if (this->outputPath().isEmpty())
    {
        // this->m_lastError = "Output manifest file is invalid: " + this->outputPath();
        this->setLastError(QString("Output manifest file is invalid: %1").arg(this->outputPath()));

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("ManifestSynchronizer::synchronizeManifests() - %1!")
                              .arg(this->lastError()),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        FileInfo FileInfo(this->outputPath(), FileType_t::SynchronizedManifest, false);
        emit finished(FileInfo);

        return false;
    }

    // Load all manifest files
    QJsonDocument videoDoc, audioDoc, annotationDoc, vttDoc;

    quint8 loadedMask = 0;

    for (const FileInfo &fInfo : qAsConst(allData))
    {
        if ( fInfo.url.isEmpty() ||
             ! fInfo.url.isValid() )
        {
#ifdef QT_DEBUG
            LogUtils::safeLog(QString("ManifestSynchronizer::synchronizeManifests() - fInfo is invalid!"),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
#endif

            return false;
        }

        switch (fInfo.type)
        {
            case FileType_t::AnnotationManifest:
            {

                if (!loadJsonFile(fInfo.url.path(), annotationDoc))
                {
#ifdef QT_DEBUG
                    LogUtils::safeLog(QString("ManifestSynchronizer::synchronizeManifests() - AnnotationManifest not loaded!"),
                                      __FILE__,
                                      __LINE__,
                                      Q_FUNC_INFO);
#endif

                    return false;
                }

                loadedMask |= ManifestBit::AnnotationBit;

                break;
            }

            case FileType_t::AudioManifest:
            {
                if (!loadJsonFile(fInfo.url.path(), audioDoc))
                {
#ifdef QT_DEBUG
                    LogUtils::safeLog(QString("ManifestSynchronizer::synchronizeManifests() - AudioManifest not loaded!"),
                                      __FILE__,
                                      __LINE__,
                                      Q_FUNC_INFO);
#endif

                    return false;
                }

                loadedMask |= ManifestBit::AudioBit;

                break;
            }

            case FileType_t::VideoManifest:
            {
                if (!loadJsonFile(fInfo.url.path(), videoDoc))
                {
#ifdef QT_DEBUG
                    LogUtils::safeLog(QString("ManifestSynchronizer::synchronizeManifests() - VideoManifest not loaded!"),
                                      __FILE__,
                                      __LINE__,
                                      Q_FUNC_INFO);
#endif

                    return false;
                }

                loadedMask |= ManifestBit::VideoBit;

                break;
            }

            case FileType_t::VTTManifest:
            {
                if (!loadJsonFile(fInfo.url.path(), vttDoc))
                {
#ifdef QT_DEBUG
                    LogUtils::safeLog(QString("ManifestSynchronizer::synchronizeManifests() - VTTManifest not loaded!"),
                                      __FILE__,
                                      __LINE__,
                                      Q_FUNC_INFO);
#endif

                    return false;
                }

                loadedMask |= ManifestBit::VttBit;

                break;
            }

            default: break;
        }
    }

    if (loadedMask != ManifestBit::AllBits)
    {
        // Determine which one is missing for a better error message
        QStringList missing;

        if (!(loadedMask & VideoBit)) missing << "Video";
        if (!(loadedMask & AudioBit)) missing << "Audio";
        if (!(loadedMask & AnnotationBit)) missing << "Annotation";
        if (!(loadedMask & VttBit)) missing << "VTT";

        // this->m_lastError = "Missing required manifests: " + missing.join(", ");
        this->setLastError(QString("Missing required manifests: %1").arg(missing.join(", ")));

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("ManifestSynchronizer::synchronizeManifests() - %1 --- %2!")
                              .arg(this->lastError(), this->outputPath()),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        FileInfo FileInfo(this->outputPath(), FileType_t::SynchronizedManifest, false);
        emit finished(FileInfo);

        return false;
    }

    // Use video as the base document
    QJsonObject videoRoot         = videoDoc.object();
    QJsonObject audioRoot         = audioDoc.object();
    QJsonObject annotationRoot    = annotationDoc.object();
    QJsonObject vttRoot           = vttDoc.object();

    QJsonArray videoSegments      = videoRoot["segments"].toArray();
    QJsonArray audioSegments      = audioRoot["segments"].toArray();
    QJsonArray annotationSegments = annotationRoot["segments"].toArray();
    QJsonArray vttSegments        = vttRoot["segments"].toArray();

#ifdef QT_DEBUG
    QString allCounts = QString("ManifestSynchronizer::synchronizeManifests() - Segments count: Video(%1) Audio(%2) Annotation(%3) VTT(%4)")
                         .arg(videoSegments.size())
                         .arg(audioSegments.size())
                         .arg(annotationSegments.size())
                         .arg(vttSegments.size());

    LogUtils::safeLog(allCounts,
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

//     // Validate segment counts match
//     if (videoSegments.size() != audioSegments.size() ||
//         videoSegments.size() != annotationSegments.size() ||
//         videoSegments.size() != vttSegments.size())
//     {
//         QString error = QString("Segment count mismatch: Video(%1) Audio(%2) Annotation(%3) VTT(%4)")
//                             .arg(videoSegments.size(),
//                                  audioSegments.size(),
//                                  annotationSegments.size())
//                             .arg(vttSegments.size());

//         this->setLastError(error);

// #ifdef QT_DEBUG
//         LogUtils::safeLog(QString("ManifestSynchronizer::synchronizeManifests() - Synchronization failed: segment count mismatch!"),
//                           __FILE__,
//                           __LINE__,
//                           Q_FUNC_INFO);
// #endif

//         FileInfo FileInfo(this->outputPath(), FileType_t::SynchronizedManifest, false);
//         emit finished(FileInfo);

//         return false;
//     }

    QHash<qint32, QJsonObject> audiosMap;

    for (const QJsonValue &val : qAsConst(audioSegments))
    {
        QJsonObject obj = val.toObject();

        if (obj.contains(QStringLiteral("segment_index")))
        {
            qint32 idx = static_cast<qint32>(obj[QStringLiteral("segment_index")].toDouble());
            audiosMap.insert(idx, obj);
        }
    }

    QHash<qint32, QJsonObject> annotationsMap;

    for (const QJsonValue &val : qAsConst(annotationSegments))
    {
        QJsonObject obj = val.toObject();

        if (obj.contains(QStringLiteral("segment_index")))
        {
            qint32 idx = static_cast<qint32>(obj[QStringLiteral("segment_index")].toDouble());
            annotationsMap.insert(idx, obj);
        }
    }

    QHash<qint32, QJsonObject> vttsMap;

    for (const QJsonValue &val : qAsConst(vttSegments))
    {
        QJsonObject obj = val.toObject();

        if (obj.contains(QStringLiteral("segment_index")))
        {
            qint32 idx = static_cast<qint32>(obj[QStringLiteral("segment_index")].toDouble());
            vttsMap.insert(idx, obj);
        }
    }

    // Map and Sort Video segments to ensure the final array output is sequential
    QMap<qint32, QJsonObject> sortedVideoMap;

    for (const QJsonValue &val : qAsConst(videoSegments))
    {
        QJsonObject obj = val.toObject();

        if (obj.contains(QStringLiteral("segment_index")))
        {
            qint32 idx = static_cast<qint32>(obj[QStringLiteral("segment_index")].toDouble());
            sortedVideoMap.insert(idx, obj);
        }
    }

    QJsonArray synchronizedSegments;
    QMap<qint32, QJsonObject>::const_iterator cit = sortedVideoMap.constBegin();

    qint32 targetIdx;
    QJsonObject videoSegment;

    while (cit != sortedVideoMap.constEnd())
    {
        targetIdx = cit.key();
        videoSegment = cit.value();

        QJsonObject matchedAudioObj      = audiosMap.value(targetIdx, QJsonObject());
        QJsonObject matchedAnnotationObj = annotationsMap.value(targetIdx, QJsonObject());
        QJsonObject matchedVttObj        = vttsMap.value(targetIdx, QJsonObject());

        // Merge external segment data into the base video segment object
        this->mergeAudioSegment(videoSegment, matchedAudioObj);
        this->mergeAnnotationSegment(videoSegment, matchedAnnotationObj);
        this->mergeVttSegment(videoSegment, matchedVttObj);

        synchronizedSegments.append(videoSegment);

        ++cit;
    }

    // Replace the old segments array with the synchronized one
    videoRoot["segments"] = synchronizedSegments;
    this->m_synchronizedSegmentCount = synchronizedSegments.size();

    // Write output file
    QFile outputFile(this->outputPath());

    if (!outputFile.open(QIODevice::WriteOnly))
    {
        QString lasError = QString("ManifestSynchronizer::synchronizeManifests() - %1!")
                            .arg(this->outputPath());

        this->setLastError(lasError);

#ifdef QT_DEBUG
        LogUtils::safeLog(lasError,
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        emit error(lasError);

        FileInfo FileInfo(this->outputPath(), FileType_t::SynchronizedManifest, false);
        emit finished(FileInfo);

        return false;
    }

    QJsonDocument outputDoc(videoRoot);
    outputFile.write(outputDoc.toJson());
    outputFile.close();

    qDebug() << "Successfully synchronized" << this->m_synchronizedSegmentCount << "segments";

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("ManifestSynchronizer::synchronizeManifests() - Successfully synchronized %1 segments!")
                      .arg(this->m_synchronizedSegmentCount),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    FileInfo fileInfo(this->outputPath(), FileType_t::SynchronizedManifest, true);
    emit finished(fileInfo);

    return true;
}

VideoSegmentInfo::VideoSegmentInfoList_t ManifestSynchronizer::parseSynchronizedManifest(const QString &manifestPath)
{
    VideoSegmentInfo::VideoSegmentInfoList_t segmentList;
    QJsonDocument manifestDoc;

    // Utilize the existing loadJsonFile method which also handles setting lastError
    if (!loadJsonFile(manifestPath, manifestDoc))
    {
        qWarning() << "Failed to load manifest for parsing:" << this->lastError();
        return segmentList;
    }

    QJsonObject rootObj = manifestDoc.object();

    // Verify the segments array exists
    if (!rootObj.contains(QStringLiteral("segments")) || !rootObj[QStringLiteral("segments")].isArray())
    {
        this->setLastError("Manifest does not contain a valid 'segments' array.");
        qWarning() << this->lastError();
        return segmentList;
    }

    QJsonArray segmentsArray = rootObj[QStringLiteral("segments")].toArray();

    // Iterate through the array and parse each object using VideoSegmentInfo's built-in deserializer
    for (const QJsonValue &value : qAsConst(segmentsArray))
    {
        if (value.isObject())
        {
            QJsonObject segmentObj = value.toObject();

            // 1. Use the built-in deserializer for standard fields
            VideoSegmentInfo segmentInfo = VideoSegmentInfo::fromJson(segmentObj);

            // 2. Extract Audio object to modify it
            Audio audio = segmentInfo.audioTrack();

            // 3. Map annotation "file_path"
            if (segmentObj.contains(QStringLiteral("annotation")))
            {
                QString annotPath = segmentObj["annotation"].toObject()["file_path"].toString();
                QFile file(annotPath);

                if (file.open(QIODevice::ReadOnly))
                {
                    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
                    audio.setAnnotation(doc.object());
                    file.close();
                }
            }

            // 4. Map vtt_transcript "file_path"
            if (segmentObj.contains(QStringLiteral("vtt_transcript")))
            {
                QString vttPath = segmentObj["vtt_transcript"].toObject()["file_path"].toString();
                QFile file(vttPath);

                if (file.open(QIODevice::ReadOnly | QIODevice::Text))
                {
                    QTextStream in(&file);
                    in.setCodec("UTF-8");
                    audio.setTranscription(in.readAll());
                    file.close();
                }
            }

            // 5. Re-assign the modified audio track to the segment
            segmentInfo.setAudioTrack(audio);
            segmentList.append(segmentInfo);
        }
    }

    qDebug() << "Successfully parsed" << segmentList.size() << "VideoSegmentInfo objects.";

    return segmentList;
}

QString ManifestSynchronizer::lastError() const
{
    QMutexLocker l(&this->m_mutexLastError);

    return this->m_lastError;
}

void ManifestSynchronizer::setLastError(const QString &newLastError)
{
    QMutexLocker l(&this->m_mutexLastError);

    if (newLastError == this->m_lastError) return;

    this->m_lastError = newLastError;
}

int ManifestSynchronizer::getSynchronizedSegmentCount() const
{
    QMutexLocker l(&this->m_mutexSynchronizedSegmentCount);

    return this->m_synchronizedSegmentCount;
}

QString ManifestSynchronizer::outputPath() const
{
    return this->m_outputPath;
}

void ManifestSynchronizer::setOutputPath(const QString &newOutputPath)
{
    if (newOutputPath == this->m_outputPath) return;

    this->m_outputPath = newOutputPath;
}

// void ManifestSynchronizer::setVideoManifestFilePath(const QString &path)
// {
//     if (path == this->m_videoPath) return;

//     this->m_videoPath = path;

// #ifdef QT_DEBUG
//     LogUtils::safeLog(QString("ManifestSynchronizer::setVideoManifestFilePath(...) - Video Manifest File path: %1 ")
//                           .arg(this->m_videoPath),
//                       __FILE__,
//                       __LINE__,
//                       Q_FUNC_INFO);
// #endif

//     this->checkAndRun();
// }

// void ManifestSynchronizer::setAudioManifestFilePath(const QString &path)
// {
// #ifdef QT_DEBUG
//     LogUtils::safeLog(QString("ManifestSynchronizer::setAudioManifestFilePath(...) - Starting: %1 ")
//                           .arg(path),
//                       __FILE__,
//                       __LINE__,
//                       Q_FUNC_INFO);
// #endif

//     if (path == this->m_audioPath) return;

//     this->m_audioPath = path;

// #ifdef QT_DEBUG
//     LogUtils::safeLog(QString("ManifestSynchronizer::setAudioManifestFilePath(...) - Audio Manifest File path: %1 ")
//                           .arg(this->m_audioPath),
//                       __FILE__,
//                       __LINE__,
//                       Q_FUNC_INFO);
// #endif

//     this->checkAndRun();
// }

// void ManifestSynchronizer::setAnnotationManifestFilePath(const QString &path)
// {
//     if (path == this->m_annotationPath) return;

//     this->m_annotationPath = path;

// #ifdef QT_DEBUG
//     LogUtils::safeLog(QString("ManifestSynchronizer::setAnnotationManifestFilePath(...) - Annotation Manifest File path: %1 ")
//                           .arg(this->m_annotationPath),
//                       __FILE__,
//                       __LINE__,
//                       Q_FUNC_INFO);
// #endif

//     this->checkAndRun();
// }

// void ManifestSynchronizer::setVttManifestFilePath(const QString &path)
// {
//     if (path == this->m_vttPath) return;

//     this->m_vttPath = path;

// #ifdef QT_DEBUG
//     LogUtils::safeLog(QString("ManifestSynchronizer::setVttManifestFilePath(...) - VTT Manifest File path: %1 ")
//                           .arg(this->m_vttPath),
//                       __FILE__,
//                       __LINE__,
//                       Q_FUNC_INFO);
// #endif

//     this->checkAndRun();
// }

void ManifestSynchronizer::handleInput(FileInfo info)
{
    QMutexLocker l(&this->m_mutexCollectedData);

    // 1. Map the FileType to a Bit
    ManifestBit bit; // Default fallback
    QString manifestFile;

    switch (info.type)
    {
        case FileType_t::VideoManifest:      bit = ManifestBit::VideoBit; manifestFile = QString("Video Manifest: %1").arg(info.url.toString()); break;
        case FileType_t::AudioManifest:      bit = ManifestBit::AudioBit; manifestFile = QString("Audio Manifest: %1").arg(info.url.toString()); break;
        case FileType_t::AnnotationManifest: bit = ManifestBit::AnnotationBit; manifestFile = QString("Annotation Manifest: %1").arg(info.url.toString()); break;
        case FileType_t::VTTManifest:        bit = ManifestBit::VttBit; manifestFile = QString("VTT Manifest: %1").arg(info.url.toString()); break;
        default:
            return; // Ignore types that aren't part of the manifest sync
    }

    // 2. Check for duplicates (Optional but professional)
    if (this->m_currentMask & bit)
    {
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("ManifestSynchronizer::handleInput() - Warning: Duplicate manifest received for type %1")
                              .arg(static_cast<qint32>(info.type)),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        // Depending on your logic, you might want to replace the old entry
        // in m_collectedData here instead of just appending.
    }

    // 3. Update the collection and the mask
    this->m_collectedData.append(info);
    this->m_currentMask |= bit;

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("ManifestSynchronizer::handleInput() - Progress Mask: %1 --- %2")
                          .arg(this->m_currentMask, 0, 2)
                          .arg(manifestFile),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    // 4. Trigger only when the specific "AllRequired" pattern is matched
    if (this->m_currentMask == ManifestBit::AllBits)
    {
        emit collectionComplete(this->m_collectedData);

        // 5. Reset both the container AND the bitmask for the next cycle
        this->m_collectedData.clear();
        this->m_currentMask = 0;
    }
}

bool ManifestSynchronizer::loadJsonFile(const QString &filePath, QJsonDocument &jsonDoc)
{
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly))
    {
        // this->m_lastError = "Cannot open file: " + filePath;
        this->setLastError(QString("Cannot open file: %1").arg(filePath));

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("ManifestSynchronizer::loadJsonFile(...) - %1")
                              .arg(this->lastError()),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    jsonDoc = QJsonDocument::fromJson(data, &parseError);

    if (jsonDoc.isNull())
    {
        // this->m_lastError = QString("JSON parse error in %1: %2")
        //                         .arg(filePath, parseError.errorString());

        this->setLastError(QString("JSON parse error in %1: %2")
                               .arg(filePath, parseError.errorString()));

        return false;
    }

    return true;
}

void ManifestSynchronizer::mergeAudioSegment(QJsonObject &videoSegment, const QJsonObject &audioSegment)
{
    QJsonObject audioTrack;
    audioTrack["segment_index"] = audioSegment["segment_index"];
    audioTrack["audio_id"]      = audioSegment["audio_id"];
    audioTrack["duration"]      = audioSegment["duration"];
    audioTrack["end_time"]      = audioSegment["end_time"];
    audioTrack["start_time"]    = audioSegment["start_time"];
    audioTrack["state"]         = audioSegment["state"];
    audioTrack["file_path"]     = audioSegment["file_path"];

    videoSegment["audio_track"] = audioTrack;
}

void ManifestSynchronizer::mergeAnnotationSegment(QJsonObject &videoSegment, const QJsonObject &annotationSegment)
{
    QJsonObject annotationData;
    annotationData["segment_index"] = annotationSegment["segment_index"];
    annotationData["file_name"]     = annotationSegment["file_name"];
    annotationData["file_path"]     = annotationSegment["file_path"];
    annotationData["segment_index"] = annotationSegment["segment_index"];
    annotationData["timing"]        = annotationSegment["timing"];

    videoSegment["annotation"]      = annotationData;
}

void ManifestSynchronizer::mergeVttSegment(QJsonObject &videoSegment, const QJsonObject &vttSegment)
{
    QJsonObject vttData;
    vttData["segment_index"]       = vttData["segment_index"];
    vttData["file_name"]           = vttSegment["file_name"];
    vttData["file_path"]           = vttSegment["file_path"];
    vttData["segment_index"]       = vttSegment["segment_index"];
    vttData["timing"]              = vttSegment["timing"];

    videoSegment["vtt_transcript"] = vttData;
}

// void ManifestSynchronizer::checkAndRun()
// {
// #ifdef QT_DEBUG
//     LogUtils::safeLog(QString("ManifestSynchronizer::checkAndRun() - Checking Sync Status: \n"
//                               "                                                            V: %1 \n"
//                               "                                                            A: %2 \n"
//                               "                                                            An: %3 \n"
//                               "                                                            VTT: %4 \n")
//                       .arg(!m_videoPath.isEmpty())
//                       .arg(!m_audioPath.isEmpty())
//                       .arg(!m_annotationPath.isEmpty())
//                       .arg(!m_vttPath.isEmpty()),
//                       __FILE__,
//                       __LINE__,
//                       Q_FUNC_INFO);
// #endif

//     // Only proceed if all four paths are non-empty
//     if (this->m_videoPath.isEmpty() || this->m_audioPath.isEmpty() ||
//         this->m_annotationPath.isEmpty() || this->m_vttPath.isEmpty() ||
//         this->m_outputPath.isEmpty() )
//     {
//         return;
//     }

// #ifdef QT_DEBUG
//     LogUtils::safeLog(QString("ManifestSynchronizer::checkAndRun() - All files set"),
//                       __FILE__,
//                       __LINE__,
//                       Q_FUNC_INFO);
// #endif

//     bool success = this->synchronizeManifests(this->m_videoPath,
//                                               this->m_audioPath,
//                                               this->m_annotationPath,
//                                               this->m_vttPath,
//                                               this->m_outputPath);

//     FileInfo FileInfo(this->outputPath(), FileType_t::SynchronizedManifest, success);
//     emit finished(FileInfo);

// #ifdef QT_DEBUG
//     LogUtils::safeLog(QString("ManifestSynchronizer::checkAndRun() - Manifest files synchronized!"),
//                       __FILE__,
//                       __LINE__,
//                       Q_FUNC_INFO);
// #endif

//     // Optional: Clear paths if you want to allow a fresh synchronization cycle
//     this->m_videoPath.clear();

//     this->m_audioPath.clear();

//     this->m_annotationPath.clear();

//     this->m_vttPath.clear();
// }
