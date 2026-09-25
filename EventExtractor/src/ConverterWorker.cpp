
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTextStream>
#include <QTime>

// #include <cmath>

#include "inc/ConverterWorker.h"
#include "inc/LogUtils.h"

ConverterWorker::ConverterWorker(QObject *parent) :
    QObject(parent),
    m_converterType(ConverterType_t::None)
{
    this->m_transcriptionFileUrl.clear();
    this->m_annotationFileUrl.clear();

}

// Destructor
ConverterWorker::~ConverterWorker()
{
    // TODO
}

bool ConverterWorker::operator==(const ConverterWorker &right) const
{
    // Since QObjects are identities (non-copyable), we usually compare memory addresses.
    // If you need to compare internal state, add specific member checks here.
    return this == &right;
}

bool ConverterWorker::operator!=(const ConverterWorker &right) const
{
    return !(*this == right);
}

bool ConverterWorker::operator==(const ConverterWorker *ptr) const
{
    return this == ptr;
}

bool ConverterWorker::operator!=(const ConverterWorker *ptr) const
{
    return this != ptr;
}

Url ConverterWorker::annotationFileUrl() const
{
    return this->m_annotationFileUrl;
}

void ConverterWorker::setAnnotationFileUrl(const Url &newAnnotationFileUrl)
{
    if (newAnnotationFileUrl == this->m_annotationFileUrl) return;

    this->m_annotationFileUrl = newAnnotationFileUrl;
}

void ConverterWorker::setAnnotationFileUrl(const QString &newAnnotationFileUrl)
{
    if (newAnnotationFileUrl.isEmpty()) return;

    Url url(newAnnotationFileUrl);

    this->setAnnotationFileUrl(url);
}

Url ConverterWorker::transcriptionFileUrl() const
{
    return this->m_transcriptionFileUrl;
}

void ConverterWorker::setTranscriptionFileUrl(const Url &newTranscriptionFileUrl)
{
    if (newTranscriptionFileUrl == this->m_transcriptionFileUrl) return;

    this->m_transcriptionFileUrl = newTranscriptionFileUrl;
}

void ConverterWorker::setTranscriptionFileUrl(const QString &newTranscriptionFileUrl)
{
    if (newTranscriptionFileUrl.isEmpty()) return;

    Url url(newTranscriptionFileUrl);

    this->setTranscriptionFileUrl(url);
}

ConverterType_t ConverterWorker::converterType() const
{
    return this->m_converterType;
}

void ConverterWorker::setConverterType(ConverterType_t newConverterType)
{
    if (newConverterType == this->m_converterType) return;

    this->m_converterType = newConverterType;
}

qint32 ConverterWorker::convertToTranscript(const qint32 lineLength)
{
    if (this->transcriptionFileUrl().isEmpty() ||
        !this->transcriptionFileUrl().isValid()) return -1;

    // TODO Check input file validity and existence, it must be a JSon file; check output file vailidty

    QString msg = "Input file path provided: ";

    qInfo() << msg << this->annotationFileUrl().path() << Qt::endl;

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("ConverterWorker::convertToTranscript() --- %1%2")
                      .arg(msg, this->annotationFileUrl().path()),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    msg = " Output file path provided: ";

    qInfo() << msg << this->transcriptionFileUrl().path() << Qt::endl;

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("ConverterWorker::convertToTranscript() --- %1%2")
                      .arg(msg, this->transcriptionFileUrl().path()),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    // 1. Check the line length of the transcription.
    if (lineLength <= 0)
    {
        msg = "  ---> Error: Line length is zero.";
        qInfo() << msg << Qt::endl;

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("ConverterWorker::convertToTranscript() --- %1")
                      .arg(msg),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

        return -2;
    }

    // 2. Read Input JSON
    QFile inputFile(this->annotationFileUrl().path());

    if ( ! inputFile.open(QIODevice::ReadOnly | QIODevice::Text) )
    {
        qInfo() << "Error: Unable to open input file." << Qt::endl;

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("ConverterWorker::convertToTranscript() --- %1%2")
                          .arg(msg, this->annotationFileUrl().path()),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        return -3;
    }

    QByteArray jsonData = inputFile.readAll();
    inputFile.close();

    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);

    if ( jsonDoc.isNull() || ! jsonDoc.isObject() )
    {
        msg = "Error: Failed to parse JSON.";

        qInfo() << msg << Qt::endl;

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("ConverterWorker::convertToTranscript() --- %1")
                          .arg(msg),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        return -4;
    }

    QJsonObject dgSpeakers = jsonDoc.object();

    // Convert to SRT instead of WebVTT
    QString captions;

    switch (this->converterType())
    {
        case ConverterType_t::DeepgramSRTConverter:
        {
            msg = "Converting to Deepgram SRT!";

            qInfo() << msg << Qt::endl;

#ifdef QT_DEBUG
            LogUtils::safeLog(QString("ConverterWorker::convertToTranscript() --- %1")
                              .arg(msg),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
#endif

            captions = ConverterWorker::deepgramConvertToSrt(dgSpeakers, lineLength);
        }

        case ConverterType_t::DeepgramWebVttConverter:
        {
            msg = "Converting to Deepgram WebVTT!";

            qInfo() << msg << Qt::endl;

#ifdef QT_DEBUG
            LogUtils::safeLog(QString("ConverterWorker::convertToTranscript() --- %1")
                              .arg(msg),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
#endif
            captions = ConverterWorker::deepgramConvertToWebVtt(dgSpeakers, lineLength);

            break;
        }

        case ConverterType_t::None:
        {
            msg = "Error: no transcript format define! ";

            qInfo() << msg << Qt::endl;

#ifdef QT_DEBUG
            LogUtils::safeLog(QString("ConverterWorker::convertToTranscript() --- %1")
                              .arg(msg),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
#endif

            return -5;
        }
    }

    // Write Output File
    QFile outputFile(this->transcriptionFileUrl().path());

    if ( ! outputFile.open(QIODevice::WriteOnly | QIODevice::Text) )
    {
        msg = "Error: File not found or unable to write.";

        qInfo() << msg << Qt::endl;

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("ConverterWorker::convertToTranscript() --- %1")
                          .arg(msg),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        return -6;
    }

    QTextStream outStream(&outputFile);
    outStream << captions;
    outputFile.close();

    msg = "SRT file written successfully.";

    qInfo() << msg << Qt::endl;

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("ConverterWorker::convertToTranscript() --- %1")
                      .arg(msg),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    emit finished(0, 0);

    return 0;
}

void ConverterWorker::abort()
{
    // In a synchronous conversion (parsing JSON), "abort" is difficult to trigger
    // mid-function unless you process in a loop with event processing.
    // For now, we simply emit finished.
    emit finished(-1, 0);
}

void ConverterWorker::stop()
{
    this->abort();
}

QString ConverterWorker::deepgramConvertToWebVtt(const QJsonObject &deepgramData, const qint32 lineLength)
{
    QString output = "WEBVTT\n\n";

    // 1. Navigate to the words in the JSON structure
    // Path: results -> channels[0] -> alternatives[0] -> words
    QJsonValue results = deepgramData.value("results");
    QJsonArray channels = results.toObject().value("channels").toArray();

    if (channels.isEmpty()) return "Error: No channels found in JSON.";

    QJsonArray alternatives = channels[0].toObject().value("alternatives").toArray();

    if (alternatives.isEmpty()) return "Error: No alternatives found.";

    QJsonArray words = alternatives[0].toObject().value("words").toArray();

    // 2. Process words and build captions
    // This is a simplified logic to mimic the 'line_length' behavior
    QString currentCaptionText;
    double startTime = -1.0;
    double endTime = 0.0;

    for (const QJsonValue &wVal : qAsConst(words))
    {
        QJsonObject wObj = wVal.toObject();
        QString word = wObj.value("word").toString();
        double wordStart = wObj.value("start").toDouble();
        double wordEnd = wObj.value("end").toDouble();

        // Initialize start time for the new block
        if (startTime < 0) startTime = wordStart;

        // Check if adding this word exceeds line length
        if ( ! currentCaptionText.isEmpty() && (currentCaptionText.length() + word.length() + 1 > lineLength) )
        {
            // Flush current caption
            output += formatTimestamp(startTime) + " --> " + formatTimestamp(endTime) + "\n";
            output += currentCaptionText + "\n\n";

            // Reset for next block
            currentCaptionText = word;
            startTime = wordStart;
        } else {
            // Append to current line
            if ( ! currentCaptionText.isEmpty()) currentCaptionText += " ";

            currentCaptionText += word;
        }
        // Always update the end time of the current block
        endTime = wordEnd;
    }

    // Flush remaining text
    if (!currentCaptionText.isEmpty())
    {
        output += formatTimestamp(startTime) + " --> " + formatTimestamp(endTime) + "\n";
        output += currentCaptionText + "\n\n";
    }

    return output;
}

QString ConverterWorker::deepgramConvertToSrt(const QJsonObject &deepgramData, int lineLength)
{
    QString output;
    int sequenceNumber = 1; // SRT requires a running counter

    // 1. Navigate JSON (Same as before)
    QJsonValue results = deepgramData.value("results");
    QJsonArray channels = results.toObject().value("channels").toArray();

    if (channels.isEmpty()) return "Error: No channels found.";

    QJsonArray alternatives = channels[0].toObject().value("alternatives").toArray();

    if (alternatives.isEmpty()) return "Error: No alternatives found.";

    QJsonArray words = alternatives[0].toObject().value("words").toArray();

    // 2. Process words
    QString currentCaptionText;
    double startTime = -1.0;
    double endTime = 0.0;

    for (const QJsonValue &wVal : qAsConst(words))
    {
        QJsonObject wObj = wVal.toObject();
        QString word = wObj.value("word").toString();
        double wordStart = wObj.value("start").toDouble();
        double wordEnd = wObj.value("end").toDouble();

        if (startTime < 0) startTime = wordStart;

        // Check line length
        if (!currentCaptionText.isEmpty() && (currentCaptionText.length() + word.length() + 1 > lineLength))
        {
            // Flush current caption block
            output += QString::number(sequenceNumber++) + "\n";
            output += formatTimestamp(startTime) + " --> " + formatTimestamp(endTime) + "\n";
            output += currentCaptionText + "\n\n";

            currentCaptionText = word;
            startTime = wordStart;
        } else {

            if ( ! currentCaptionText.isEmpty() ) currentCaptionText += " ";
            currentCaptionText += word;
        }
        endTime = wordEnd;
    }

    // Flush final text
    if ( ! currentCaptionText.isEmpty() )
    {
        output += QString::number(sequenceNumber++) + "\n";
        output += formatTimestamp(startTime) + " --> " + formatTimestamp(endTime) + "\n";
        output += currentCaptionText + "\n\n";
    }

    return output;
}

QString ConverterWorker::formatTimestamp(double seconds)
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
