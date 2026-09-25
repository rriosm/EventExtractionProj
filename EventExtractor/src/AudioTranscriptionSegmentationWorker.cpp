
#include <QFileInfo>
#include <QDateTime>
#include <QTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QTextStream>

#include "inc/LogUtils.h"

#include "inc/AudioTranscriptionSegmentationWorker.h"

AudioTranscriptionSegmentationWorker::AudioTranscriptionSegmentationWorker(QObject *parent) :
    QObject{parent},
    m_clipSec(0.00)
{
    this->m_transcriptionManifest.clear();
    this->m_transcriptionClipsOutputDir.clear();
}

AudioTranscriptionSegmentationWorker::~AudioTranscriptionSegmentationWorker()
{
    // TODO
}

bool AudioTranscriptionSegmentationWorker::operator ==(const AudioTranscriptionSegmentationWorker &right) const
{
    if ((this->m_clipSec != right.m_clipSec) ||
        (this->m_lineLength != right.m_lineLength) ||
        (this->m_transcriptionManifest != right.m_transcriptionManifest) ||
        (this->m_transcriptionClipsOutputDir != right.m_transcriptionClipsOutputDir) )
    {
        return false;
    }

    return true;
}

bool AudioTranscriptionSegmentationWorker::operator !=(const AudioTranscriptionSegmentationWorker &right) const
{
    return !(*this == right);
}

bool AudioTranscriptionSegmentationWorker::operator==(const AudioTranscriptionSegmentationWorker *ptr) const
{
    return (this == ptr);
}

bool AudioTranscriptionSegmentationWorker::operator!=(const AudioTranscriptionSegmentationWorker *ptr) const
{
    return (this != ptr);
}


qreal AudioTranscriptionSegmentationWorker::clipSec() const
{
    QMutexLocker l(&this->m_mutexClipSec);

    return this->m_clipSec;
}

void AudioTranscriptionSegmentationWorker::setClipSec(qreal newClipSec)
{
    QMutexLocker l(&this->m_mutexClipSec);

    if (newClipSec == this->m_clipSec) return;

    this->m_clipSec = newClipSec;
}

qint32 AudioTranscriptionSegmentationWorker::lineLength() const
{
    QMutexLocker l(&this->m_mutexLineLength);

    return this->m_lineLength;
}

void AudioTranscriptionSegmentationWorker::setLineLength(qint32 newLineLength)
{
    QMutexLocker l(&this->m_mutexLineLength);

    if (newLineLength == this->m_lineLength) return;

    this->m_lineLength = newLineLength;
}

Url AudioTranscriptionSegmentationWorker::transcriptionManifest() const
{
    QMutexLocker l(&this->m_mutexTranscriptionManifest);

    return this->m_transcriptionManifest;
}

void AudioTranscriptionSegmentationWorker::setTranscriptionManifest(const Url &newTranscriptionManifest)
{
    QMutexLocker l(&this->m_mutexTranscriptionManifest);

    if (newTranscriptionManifest == this->m_transcriptionManifest) return;

    this->m_transcriptionManifest = newTranscriptionManifest;
}

QString AudioTranscriptionSegmentationWorker::transcriptionClipsOutputDir() const
{
    QMutexLocker l(&this->m_mutexTranscriptionClipsOutputDir);

    return this->m_transcriptionClipsOutputDir;
}

void AudioTranscriptionSegmentationWorker::setTranscriptionClipsOutputDir(const QString &newTranscriptionClipsOutputDir)
{
    QMutexLocker l(&this->m_mutexTranscriptionClipsOutputDir);

    if (newTranscriptionClipsOutputDir == this->m_transcriptionClipsOutputDir) return;

    this->m_transcriptionClipsOutputDir = newTranscriptionClipsOutputDir;
}

void AudioTranscriptionSegmentationWorker::segmentVttContent(const QString &vttFilePath)
{
    if (vttFilePath.isEmpty())
    {
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("AudioTranscriptionSegmentationWorker::segmentVttContent(...) - vttContentPath is empty!"),
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
        LogUtils::safeLog(QString("AudioTranscriptionSegmentationWorker::segmentVttContent(...) - Clip duration is invalid: %1!")
                          .arg(this->clipSec()),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        emit finished(-2);

        return;
    }

    QDir outputDir(this->transcriptionClipsOutputDir());

    if (!outputDir.exists())
    {
        if (outputDir.mkpath("."))
        {
#ifdef QT_DEBUG
            LogUtils::safeLog(QString("AudioTranscriptionSegmentationWorker::segmentVttContent(...) - Output directory created successfully: %1")
                                  .arg(this->transcriptionClipsOutputDir()),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
#endif

        } else {
#ifdef QT_DEBUG
            LogUtils::safeLog(QString("AudioTranscriptionSegmentationWorker::segmentVttContent(...) - JFailed to create directory: %1")
                                  .arg(this->transcriptionClipsOutputDir()),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
#endif

            emit finished(-3);

            return;
        }

    } else {
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("AudioTranscriptionSegmentationWorker::segmentVttContent(...) - Output directory already exists!"),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
    }

    // Open the file path provided by the FileInfo pipeline
    QFile inFile(vttFilePath);

    if (!inFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("AudioTranscriptionSegmentationWorker::segmentVttContent(...) - Failed to open VTT file: %1").arg(vttFilePath),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        emit finished(-9); // File read error
        return;
    }

    QTextStream inStream(&inFile);

    int segmentDurationMs = this->clipSec() * 1000;
    int currentSegmentIndex = -1;

    QFile *outFile = nullptr;
    QTextStream *outStream = nullptr;

    // Regex to match: 00:00:05.123 --> 00:00:10.456
    QRegularExpression timeRegex("^(\\d{2}:\\d{2}:\\d{2}\\.\\d{3}) --> (\\d{2}:\\d{2}:\\d{2}\\.\\d{3})");

    while (!inStream.atEnd())
    {
        QString line = inStream.readLine();
        QRegularExpressionMatch match = timeRegex.match(line);

        if (match.hasMatch())
        {
#ifdef QT_DEBUG
            LogUtils::safeLog(QString("AudioTranscriptionSegmentationWorker::segmentVttContent(...) - It has a match!"),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
#endif
            double startSeconds = this->parseVttTimestampToSeconds(match.captured(1));
            int startMs = static_cast<int>(startSeconds * 1000);

            int segmentIndex = startMs / segmentDurationMs;
            int offsetMs = segmentIndex * segmentDurationMs;

            if (segmentIndex != currentSegmentIndex)
            {
                if (outStream)
                {
                    delete outStream;
                    outStream = nullptr;
                }

                if (outFile)
                {
                    outFile->close();
                    delete outFile;
                    outFile = nullptr;
                }

                currentSegmentIndex = segmentIndex;
                QString fileName = QString("segment_%1.webvtt").arg(currentSegmentIndex, 3, 10, QChar('0'));

                outFile = new QFile(QDir(this->transcriptionClipsOutputDir()).filePath(fileName));

                if (outFile->open(QIODevice::WriteOnly | QIODevice::Text) )
                {
                    outStream = new QTextStream(outFile);

                    *outStream << "WEBVTT\n";
                    *outStream << "X-ORIGINAL-START-TIME: " << formatTimestamp(offsetMs / 1000.0) << "\n\n";
                } else
                {
#ifdef QT_DEBUG
                    LogUtils::safeLog(QString("AudioTranscriptionSegmentationWorker::segmentVttContent(...) - Error opening output file!"),
                                      __FILE__,
                                      __LINE__,
                                      Q_FUNC_INFO);
#endif
                    delete outFile;
                    outFile = nullptr;
                }
            }

            if (outStream)
            {
#ifdef QT_DEBUG
                LogUtils::safeLog(QString("AudioTranscriptionSegmentationWorker::segmentVttContent(...) - outStream ok!"),
                                  __FILE__,
                                  __LINE__,
                                  Q_FUNC_INFO);
#endif

                // Get end time to sync
                double endSeconds = parseVttTimestampToSeconds(match.captured(2));
                int endMs = static_cast<int>(endSeconds * 1000);

                QString syncStart = formatTimestamp((startMs - offsetMs) / 1000.0);
                QString syncEnd = formatTimestamp((endMs - offsetMs) / 1000.0);

                *outStream << "NOTE Original Time: " << match.captured(1) << " --> " << match.captured(2) << "\n";
                *outStream << syncStart << " --> " << syncEnd << "\n";
            }
        }
        else if (outStream && !line.startsWith("WEBVTT"))
        {
            *outStream << line << "\n";

#ifdef QT_DEBUG
            LogUtils::safeLog(QString("AudioTranscriptionSegmentationWorker::segmentVttContent(...) - Writing valid subtitle payload line!"),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
#endif
        }
    }

    inFile.close();

    if (outStream)
    {
        delete outStream;
        outStream = nullptr;
    }

    if (outFile)
    {
        outFile->close();
        delete outFile;
        outFile = nullptr;
    }

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("AudioTranscriptionSegmentationWorker::segmentVttContent(...) - Conversion finished!"),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    // Generate the JSON manifest right after finishing the file loop
    this->generateVttManifestToJson();
}

void AudioTranscriptionSegmentationWorker::generateVttManifestToJson()
{
#ifdef QT_DEBUG
    LogUtils::safeLog(QString("AudioTranscriptionSegmentationWorker::generateVttManifestToJson(...) - Starting! - %1")
                          .arg(this->transcriptionClipsOutputDir()),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    QDir dir(this->transcriptionClipsOutputDir());
    QStringList filters;
    filters << "segment_*.webvtt";
    QStringList vttFiles = dir.entryList(filters, QDir::Files, QDir::Unsorted);

    if (vttFiles.isEmpty())
    {
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("AudioTranscriptionSegmentationWorker::generateVttManifestToJson(...) - No VTT files!"),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        emit finished(-4);

        return;
    }

    // --- STEP 1: Parse and sort indices numerically to eliminate alphabetical order bugs ---
    QMap<qint32, QString> sortedSegmentMap;
    QRegularExpression re(QStringLiteral("segment_(\\d+)\\.webvtt"));

    for (const QString &fileName : qAsConst(vttFiles))
    {
        QRegularExpressionMatch match = re.match(fileName);

        if (match.hasMatch())
        {
            qint32 index = match.captured(1).toInt();
            sortedSegmentMap.insert(index, fileName);
        }
    }

    QJsonArray segmentsArray;
    QMap<qint32, QString>::const_iterator cit = sortedSegmentMap.constBegin();
    qint32 segmentIdx;
    QString fileName;

    while (cit != sortedSegmentMap.constEnd())
    {
        segmentIdx = cit.key();
        fileName = cit.value();

        QJsonObject segObj;
        segObj["segment_index"] = static_cast<double>(segmentIdx);
        segObj["file_name"] = fileName;
        segObj["file_path"] = dir.absoluteFilePath(fileName);

        // Calculate timing based on index and duration
        QJsonObject timingObj;
        timingObj[QStringLiteral("startTimeSec")] = static_cast<double>(segmentIdx * this->clipSec());
        timingObj[QStringLiteral("endTimeSec")] = static_cast<double>((segmentIdx + 1) * this->clipSec());
        segObj[QStringLiteral("timing")] = timingObj;

        segmentsArray.append(segObj);

        ++cit;
    }

    // Root Document Metadata
    QJsonObject rootObj;
    rootObj[QStringLiteral("exportTimestamp")] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    rootObj[QStringLiteral("segmentDurationSec")] = this->clipSec();
    rootObj[QStringLiteral("totalSegments")] = segmentsArray.size();
    rootObj[QStringLiteral("segments")] = segmentsArray;

    QJsonDocument doc(rootObj);
    QFileInfo fInfo(dir.absoluteFilePath("vtt_segmentation_manifest.json"));
    QFile manifestFile(dir.absoluteFilePath("vtt_segmentation_manifest.json"));

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("AudioTranscriptionSegmentationWorker::generateVttManifestToJson(...) - VTT Manifest: %1.")
                          .arg(fInfo.absoluteFilePath()),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    if (!manifestFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
#ifdef QT_DEBUG
        LogUtils::safeLog("AudioTranscriptionSegmentationWorker::generateVttManifestToJson(...) - VTT Manifest file open error.",
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        emit finished(-5);

        return;
    }

    manifestFile.write(doc.toJson(QJsonDocument::Indented));
    manifestFile.close();

    this->setTranscriptionManifest(fInfo.absoluteFilePath());

    emit finished(0);

#ifdef QT_DEBUG
    LogUtils::safeLog("AudioTranscriptionSegmentationWorker::generateVttManifestToJson(...) - VTT Manifest successfully generated.",
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif
}

QString AudioTranscriptionSegmentationWorker::formatTimestamp(double seconds)
{
    int h = static_cast<int>(seconds / 3600);
    int m = static_cast<int>((seconds - (h * 3600)) / 60);
    int s = static_cast<int>(seconds) % 60;
    int ms = static_cast<int>((seconds - static_cast<int>(seconds)) * 1000);

    return QString("%1:%2:%3.%4")
        .arg(h, 2, 10, QChar('0'))
        .arg(m, 2, 10, QChar('0'))
        .arg(s, 2, 10, QChar('0'))
        .arg(ms, 3, 10, QChar('0'));
}

double AudioTranscriptionSegmentationWorker::parseVttTimestampToSeconds(const QString &ts)
{
    QTime time = QTime::fromString(ts, "HH:mm:ss.zzz");

    return (time.hour() * 3600.0) + (time.minute() * 60.0) + time.second() + (time.msec() / 1000.0);
}
