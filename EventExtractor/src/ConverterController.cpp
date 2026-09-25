
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QTextStream>

#include "inc/LogUtils.h"
#include "inc/Defs.h"

#include "inc/ConverterController.h"

#include "inc/ConverterWorker.h"

ConverterController::ConverterController(PipelineWorker *parent) :
    PipelineWorker(parent),
    m_workerThread(nullptr),
    m_worker(nullptr),
    m_cvtType(ConverterType_t::None),
    m_lineLength(200),
    m_clipSec(0.00)
{
    this->m_annotationFileUrl.clear();
    this->m_transcriptionFileUrl.clear();
    this->m_transcriptionClipsOutputDir.clear();
}

ConverterController::~ConverterController()
{
    if (this->m_workerThread && this->m_workerThread->isRunning())
    {
        this->m_workerThread->requestInterruption(); // Tell the curl callback to return 1
        this->m_workerThread->quit();

        if ( ! this->m_workerThread->wait(3000) ) // Wait up to 3 seconds
        {
            this->m_workerThread->terminate();  // Force kill if it hangs
            this->m_workerThread->wait();       // Still wait to ensure cleanup
        }
    }
}

bool ConverterController::operator ==(const ConverterController & right) const
{
    if ( (this->m_transcriptionFileUrl != right.m_transcriptionFileUrl) ||
         (this->m_cvtType != right.m_cvtType) ||
         (this->m_lineLength != right.m_lineLength) ||
         (this->m_clipSec != right.m_clipSec) ||
         (this->m_annotationFileUrl != right.m_annotationFileUrl) )
    {
        return false;
    }

    return true;
}

bool ConverterController::operator !=(const ConverterController & right) const
{
    return !(*this == right);
}

bool ConverterController::operator==(const ConverterController *ptr) const
{
    return (this == ptr);
}

bool ConverterController::operator!=(const ConverterController *ptr) const
{
    return (this != ptr);
}

Url ConverterController::annotationFileUrl() const
{
    return this->m_annotationFileUrl;
}

void ConverterController::setAnnotationFileUrl(const Url &newAnnotationFileUrl)
{
    if (newAnnotationFileUrl == this->m_annotationFileUrl) return;

    this->m_annotationFileUrl = newAnnotationFileUrl;
}

void ConverterController::setAnnotationFileUrl(const QString &newAnnotationFileUrl)
{
    if (newAnnotationFileUrl.isEmpty()) return;

    Url url(newAnnotationFileUrl);

    this->setAnnotationFileUrl(url);
}

const Url &ConverterController::transcriptionFileUrl() const
{
    return this->m_transcriptionFileUrl;
}

void ConverterController::setTranscriptionFileUrl(const Url & newTranscriptionFileUrl)
{
    if (newTranscriptionFileUrl == this->m_transcriptionFileUrl) return;

    this->m_transcriptionFileUrl = newTranscriptionFileUrl;
}

void ConverterController::setTranscriptionFileUrl(const QString & newTranscriptionFileUrl)
{
    if (newTranscriptionFileUrl.isEmpty()) return;

    Url url(newTranscriptionFileUrl);

    this->setTranscriptionFileUrl(url);
}

QString ConverterController::transcriptionClipsOutputDir() const
{
    return this->m_transcriptionClipsOutputDir;
}

void ConverterController::setTranscriptionClipsOutputDir(const QString &newTranscriptionClipOutputDir)
{
    if (newTranscriptionClipOutputDir == this->m_transcriptionClipsOutputDir) return;

    this->m_transcriptionClipsOutputDir = newTranscriptionClipOutputDir;
}

ConverterType_t ConverterController::converterType() const
{
    return this->m_cvtType;
}

void ConverterController::setConverterType(ConverterType_t newCvtType)
{
    if (newCvtType == this->m_cvtType) return;

    this->m_cvtType = newCvtType;
}

qint32 ConverterController::lineLength() const
{
    return this->m_lineLength;
}

void ConverterController::setLineLength(qint32 newLineLength)
{
    if (newLineLength == this->m_lineLength) return;

    this->m_lineLength = newLineLength;
}

qreal ConverterController::clipSec() const
{
    return this->m_clipSec;
}

void ConverterController::setClipSec(qreal newClipSec)
{
    if (qFuzzyCompare(newClipSec, this->m_clipSec)) return;

    this->m_clipSec = newClipSec;
}

void ConverterController::cancelConversion()
{
    if (this->m_workerThread && this->m_workerThread->isRunning())
    {
        emit statusMessage("Stopping transcription...");

        // This sets a flag that QThread::currentThread()->isInterruptionRequested()
        // will return 'true' for inside the worker thread.
        this->m_workerThread->requestInterruption();

        // Note: We do NOT call quit() here. We wait for the curl callback
        // to see the flag, abort the transfer, and let the worker finish naturally.
    }
}

void ConverterController::handleInput(FileInfo info)
{
#ifdef QT_DEBUG
    LogUtils::safeLog(QString("ConverterController::handleInput(...) - Starting!"),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    if (info.type != FileType_t::AnnotationFile) return;

    if (info.url.isEmpty() ||
        !info.url.isValid())
    {
        emit conversionError("ConverterController::startConversion(...) --- No Json input file provided!");

        return;
    }

    if (this->m_workerThread && this->m_workerThread->isRunning())
    {
        emit statusMessage("A transcription is already in progress.");

        return;
    }

    if (this->converterType() == ConverterType_t::None)
    {
        emit conversionError("ConverterController::startConversion(...) --- Conversion type not supported!");

        return;
    }

    QFileInfo inputJsonFile(info.url.path());

    if (! inputJsonFile.isFile() || ! inputJsonFile.exists() )
    {
        emit conversionError("ConverterController::startConversion(...) --- Json input file isn't a file or doesn't exist!");

        return;
    }

    this->setAnnotationFileUrl(info.url);

    QString extension;

    switch (this->converterType())
    {
        case ConverterType_t::DeepgramSRTConverter:       extension = "srt"; break;
        case ConverterType_t::DeepgramWebVttConverter:    extension = "webvtt"; break;
        default:
        {
            emit conversionError("ConverterController::startConversion(...) --- Conversion type not supported!");

            return;
        }
    }

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("ConverterController::handleInput(...) - Extension definition: %1!")
                          .arg(extension),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    // Safe replacement: replace the LAST occurrence of "json" (extension)
    QString outputConversionFile(this->annotationFileUrl().path());
    qint32 lastIndex = outputConversionFile.lastIndexOf("json", -1, Qt::CaseInsensitive);

    if (lastIndex != -1)
    {
        outputConversionFile.replace(lastIndex, 4, extension);
    } else {
        // Fallback: just append
        outputConversionFile += extension;
    }

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("ConverterController::handleInput(...) - Output converter file: %1!")
                          .arg(outputConversionFile),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    this->setTranscriptionFileUrl(outputConversionFile);

    // 1. Initialize Worker and Thread
    this->m_workerThread = new QThread(); // No parent, handled by deleteLater
    this->m_worker = new ConverterWorker();

    // 2. Configure the worker
    this->m_worker->moveToThread(this->m_workerThread);

    // 3. Setup Signal Forwarding
    // Progress: Map double (0.0-100.0) to int (0-100) for progress bar
    QObject::connect(this->m_worker,
                     &ConverterWorker::progressUpdated,
                     this,
                     [this](double p){
                         emit progressChanged(static_cast<int>(p));
                     }, Qt::QueuedConnection);

    // 4. Error Handling
    // (Assuming you add an errorOccurred signal to TranscribeAudio)
    QObject::connect(this->m_worker,
                     &ConverterWorker::error,
                     this,
                     &ConverterController::handleWorkerError);

    // 5. Lifecycle Management. Safe Start Lambda (Added 'this' context)
    qint32 lineLength = this->lineLength();

    this->m_worker->setAnnotationFileUrl(this->annotationFileUrl().path());
    this->m_worker->setTranscriptionFileUrl(this->transcriptionFileUrl().path());
    this->m_worker->setConverterType(this->converterType());

    QObject::connect(this->m_workerThread,
                     &QThread::started,
                     this,
                     [=]() {
                         this->m_worker->convertToTranscript(lineLength);
                     }, Qt::QueuedConnection);


    // 6. Thread Stopped -> UI Notification (Data extraction happens here)
    QObject::connect(this->m_worker,
                     &ConverterWorker::finished,
                     this,
                     &ConverterController::handleFinished);

    // 7. Auto-cleanup: delete worker when thread stops, delete thread when finished
    QObject::connect(this->m_workerThread,
                     &QThread::finished,
                     this->m_worker,
                     &QObject::deleteLater);

    QObject::connect(this->m_workerThread,
                     &QThread::finished,
                     this->m_workerThread,
                     &QObject::deleteLater);

    // 8. Ensure pointers are zeroed so the "is running" check works next time
    QObject::connect(this->m_workerThread,
                     &QObject::destroyed,
                     this,
                     [this](QObject *obj){
                         if (this->m_workerThread == obj) {
                             this->m_workerThread = nullptr;
                         }
                     });

    QObject::connect(this->m_worker,
                     &QObject::destroyed,
                     this,
                     [this](QObject* obj){
                         if (this->m_worker == obj) {
                             this->m_worker = nullptr;
                         }
                     });

    // 9. Start!
    this->m_workerThread->start();
    emit statusMessage("Initializing Conversion...");
}

void ConverterController::handleWorkerError(QString error)
{
    // 1. Notify the UI via the public signal
    emit conversionError(error);
    emit statusMessage("Conversion failed: " + error);

    // 2. Stop the thread
    if (this->m_workerThread) this->m_workerThread->quit();
}

void ConverterController::handleFinished()
{
#ifdef QT_DEBUG
    LogUtils::safeLog(QString("ConverterController::handleFinished() - Transcription file: %1!")
                          .arg(this->transcriptionFileUrl().path()),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    if (this->m_worker->converterType() == ConverterType_t::DeepgramWebVttConverter)
    {
//         if (this->transcriptionClipsOutputDir().isEmpty())
//         {
// #ifdef QT_DEBUG
//             LogUtils::safeLog(QString("ConverterController::handleFinished() - Error: clips output dir is empty."),
//                               __FILE__,
//                               __LINE__,
//                               Q_FUNC_INFO);
// #endif

//             return;
//         }

        QFile inputFile(this->transcriptionFileUrl().path());

        if ( ! inputFile.open(QIODevice::ReadOnly | QIODevice::Text) )
        {
#ifdef QT_DEBUG
            LogUtils::safeLog(QString("ConverterController::handleFinished() - Error: File not found or unable to read."),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
#endif

            emit conversionError("ConverterController::handleFinished() --- Error: File not found or unable to read.");

            return;
        }

        QTextStream inputStream(&inputFile);
        QString captions = inputStream.readAll();
        inputFile.close();

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("ConverterController::handleFinished() - Clip length: %1 -- %2 -- \n Captions length: %3!")
                              .arg(this->clipSec())
                              .arg(captions)
                              .arg(captions.length()),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

//         if (this->clipSec() > 0)
//         {
// #ifdef QT_DEBUG
//             LogUtils::safeLog(QString("ConverterController::handleFinished() - With clip lenght: %1!")
//                                   .arg(this->clipSec()),
//                               __FILE__,
//                               __LINE__,
//                               Q_FUNC_INFO);
// #endif
//             this->segmentVttContent(captions, this->transcriptionClipsOutputDir(), this->clipSec());

//         } else {

// #ifdef QT_DEBUG
//             LogUtils::safeLog(QString("ConverterController::handleFinished() - Without clip lenght!"),
//                               __FILE__,
//                               __LINE__,
//                               Q_FUNC_INFO);
// #endif
//             this->segmentVttContent(captions, this->transcriptionClipsOutputDir());
//         }
    }

    FileInfo transcriptionFileInfo(this->transcriptionFileUrl().path(), FileType_t::VTTTranscriptionFile, true);

    emit finished(transcriptionFileInfo);

    emit statusMessage("Conversion Complete.");

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("ConverterController::handleFinished() - Conversion Complete!"),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    if (this->m_workerThread) this->m_workerThread->quit();
}

void ConverterController::segmentVttContent(const QString &vttContentPath, const QString &outputDir, qreal clipSec)
{
#ifdef QT_DEBUG
    LogUtils::safeLog(QString("ConverterController::segmentVttContent(...) - Starting! - %1 -- %2 -- CLip length: %3")
                          .arg(vttContentPath, outputDir)
                          .arg(clipSec),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    if (vttContentPath.isEmpty() || clipSec <= 0)
    {
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("ConverterController::segmentVttContent(...) - vttContentPath is empty!"),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        return;
    }

    QStringList lines = vttContentPath.split('\n');
    int segmentDurationMs = clipSec * 1000;
    int currentSegmentIndex = -1;

    QFile *outFile = nullptr;
    QTextStream *outStream = nullptr;

    // Regex to match: 00:00:05.123 --> 00:00:10.456
    QRegularExpression timeRegex("^(\\d{2}:\\d{2}:\\d{2}\\.\\d{3}) --> (\\d{2}:\\d{2}:\\d{2}\\.\\d{3})");

    for (const QString &line : qAsConst(lines))
    {
        QRegularExpressionMatch match = timeRegex.match(line);

        if (match.hasMatch())
        {
#ifdef QT_DEBUG
            LogUtils::safeLog(QString("ConverterController::segmentVttContent(...) - It has a match!"),
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
                if (outFile)
                {
                    outFile->close();

                    delete outFile;
                }

                currentSegmentIndex = segmentIndex;
                QString fileName = QString("segment_%1.webvtt").arg(currentSegmentIndex, 3, 10, QChar('0'));
                outFile = new QFile(QDir(outputDir).filePath(fileName));

                if ( ! outFile->open(QIODevice::WriteOnly | QIODevice::Text) )
                {
#ifdef QT_DEBUG
                    LogUtils::safeLog(QString("ConverterController::segmentVttContent(...) - Error opening output file!"),
                                      __FILE__,
                                      __LINE__,
                                      Q_FUNC_INFO);
#endif
                }

                outStream = new QTextStream(outFile);

                *outStream << "WEBVTT\n";
                *outStream << "X-ORIGINAL-START-TIME: " << formatTimestamp(offsetMs / 1000.0) << "\n\n";
            }

            if (outStream)
            {
#ifdef QT_DEBUG
                LogUtils::safeLog(QString("ConverterController::segmentVttContent(...) - outStream ok!"),
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
            LogUtils::safeLog(QString("ConverterController::segmentVttContent(...) - Line starts with WEBVTT!"),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
#endif
        }
    }

    if (outFile)
    {
        outFile->close();
        delete outFile;
    }

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("ConverterController::segmentVttContent(...) - Conversion finished!"),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    // Generate the JSON manifest right after finishing the file loop
    this->generateVttManifestToJson(outputDir, clipSec);
}

void ConverterController::generateVttManifestToJson(const QString &outputDir, qint32 clipSec)
{
#ifdef QT_DEBUG
    LogUtils::safeLog(QString("ConverterController::generateVttManifestToJson(...) - Starting! - %1")
                          .arg(outputDir),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    if (outputDir.isEmpty() || clipSec <= 0)
    {
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("ConverterController::generateVttManifestToJson(...) - Output dir is empty!"),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        return;
    }

    QDir dir(outputDir);
    QStringList filters;
    filters << "segment_*.webvtt";
    QStringList vttFiles = dir.entryList(filters, QDir::Files, QDir::Unsorted);

    if (vttFiles.isEmpty())
    {
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("ConverterController::generateVttManifestToJson(...) - No VTT files!"),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

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
        timingObj[QStringLiteral("startTimeSec")] = static_cast<double>(segmentIdx * clipSec);
        timingObj[QStringLiteral("endTimeSec")] = static_cast<double>((segmentIdx + 1) * clipSec);
        segObj[QStringLiteral("timing")] = timingObj;

        segmentsArray.append(segObj);

        ++cit;
    }

    // Root Document Metadata
    QJsonObject rootObj;
    rootObj[QStringLiteral("exportTimestamp")] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    rootObj[QStringLiteral("segmentDurationSec")] = clipSec;
    rootObj[QStringLiteral("totalSegments")] = segmentsArray.size();
    rootObj[QStringLiteral("segments")] = segmentsArray;

    QJsonDocument doc(rootObj);
    QFileInfo fInfo(dir.absoluteFilePath("vtt_segmentation_manifest.json"));
    QFile manifestFile(dir.absoluteFilePath("vtt_segmentation_manifest.json"));

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("ConverterController::generateVttManifestToJson(...) - VTT Manifest: %1.")
                      .arg(fInfo.absoluteFilePath()),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    if (manifestFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        manifestFile.write(doc.toJson(QJsonDocument::Indented));
        manifestFile.close();

        QFileInfo manifesFileInfo(manifestFile);

        FileInfo manifestVTTTranscriptionFileInfo(manifesFileInfo.absoluteFilePath(), FileType_t::VTTManifest, true);

        emit manifestReady(manifestVTTTranscriptionFileInfo);

#ifdef QT_DEBUG
        LogUtils::safeLog("ConverterController::generateVttManifestToJson(...) - VTT Manifest successfully generated.",
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
    }
    else
    {
#ifdef QT_DEBUG
        LogUtils::safeLog("ConverterController::generateVttManifestToJson(...) - VTT Manifest file open error.",
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
    }
}

QString ConverterController::formatTimestamp(double seconds)
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

double ConverterController::parseVttTimestampToSeconds(const QString &ts)
{
    QTime time = QTime::fromString(ts, "HH:mm:ss.zzz");

    return (time.hour() * 3600.0) + (time.minute() * 60.0) + time.second() + (time.msec() / 1000.0);
}
