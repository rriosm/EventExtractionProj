
#include <QApplication>

#include <QtGlobal>
#include <QDebug>
#include <QLocale>
#include <QTranslator>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>
#include <QPointer>
#include <QProcess>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QThread>
#include <QTime>
#include <QTimer>

#include "UniqueId.h"
using namespace utils;

#include "inc/TranscodeWorker.h"
#include "inc/TranscodeController.h"
#include "inc/AudioTranscriptionWorker.h"
#include "inc/AudioTranscriptionController.h"
#include "inc/ConverterController.h"
#include "inc/AudioAnnotationSegmentationController.h"
#include "inc/AudioTranscriptionSegmentationController.h"
#include "inc/AudioSegmentationController.h"
#include "inc/VideoSegmentationController.h"
#include "inc/ManifestSynchronizer.h"
// #include "inc/KeywordSearchEngine.h"
// #include "inc/EventExtractionWorker.h"

#include "inc/EventExtractionController.h"
// #include "inc/VideoSegmentationWorkerFactory.h"

#include "inc/ExtractorUtils.h"
#include "inc/LogUtils.h"
#include "inc/BatchTokenEvaluator.h"
#include "inc/GovernanceAuditHarness.h"
#include "inc/TemporalAlignmentHarness.h"

// #include "tests/inc/AIModelsTest.h"

// Database Manager
#include <QSharedPointer>

// Database Manager and Entities
#include "inc/EventDatabaseManager.h"
#include "inc/EventModel.h"
#include "inc/FullVideo.h"
#include "inc/Audio.h"
#include "inc/VideoSegmentInfo.h"

// AI Prompts
#include "inc/PromptManager.h"

// extern "C" {
//     #include "curl/curl.h"
// }

#include <functional>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void testModelSubmission()
{
#ifdef QT_DEBUG
    LogUtils::safeLog(QString("testModelSubmission() - Entering!"),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif
    // Extract events from transcription.
    // EventExtractionWorker evtExtractionWorker;
    EventExtractionController evtExtractionController;

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("testModelSubmission() - Created!"),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    // evtExtractionWorker.setModel(AIModels_t::Google_Gemini_API_20FlashLite);
    evtExtractionController.setupModel(AIModels_t::Google_Gemini_API_25Flash);

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("testModelSubmission() - Model set!"),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    // evtExtractionWorker.setContentType(ContentMimeType::Json);
    // evtExtractionWorker.setContentType(ContentMimeType_e::VideoMP4);

    // #ifdef QT_DEBUG
    //     LogUtils::safeLog(QString("testModelSubmission() - Event extraction: request set!"),
    //                       __FILE__,
    //                       __LINE__,
    //                       Q_FUNC_INFO);
    // #endif

    QString sysRefereePrompt = R"(Profile: You are a referee commentator with over 30 years of experience, having refereed World Cup finals and international classics. Your analysis is technical, accurate, slightly pragmatic and authoritative. You are completely impartial and show no emotion when goals are scored; you analyse their legality.

Objective: To monitor and record every event that occurs in a football match, from technical plays to peripheral occurrences in the stadium.

1. Technical Observation Guidelines (Match Events)
For each play, you must identify:
* Infractions and Contacts: Differentiate shoulder-to-shoulder contact from improper use of the arm. Analyse the intensity and point of contact.
* Rule 11 (Offside): Evaluate the moment of the pass, interference with the opponent, and gaining an advantage.
* Discipline: Identify behaviour warranting a yellow card (recklessness) or red card (excessive force/brutality).
* Game Management: Observe time-wasting (delaying the restart), simulation and blatant complaints.

2. Off-Field and Peripheral Events
Nothing escapes your eyes. You must record:
* Bench Behaviour: Reactions from coaches, invasion of the technical area, or irregular warm-up of reserves.
* Environment and Fans: Throwing of objects, flares, discriminatory chants, or pitch invasions.
* Logistical Conditions: Problems with lighting, pitch condition, holes in the net or weather conditions that affect safety.

3. Output Format (Logs)
Whenever an event occurs, record it in the following format:
[HH:MM:SS] – TYPE OF EVENT
* Description: What happened in a factual manner.
* Expert Analysis: Your interpretation based on FIFA rules.
* Verdict: Decision made (or that should have been made).

4. Tone and Style
* Be direct and use technical terms (e.g., ‘recklessness,’ ‘lever,’ ‘regulatory distance’).
* Maintain absolute impartiality.
* If an event is bizarre or unusual, use subtle irony, typical of someone who has seen it all in football.

Example of application:
[42:15] – EXTRA-GAME EVENT
* Description: The visiting team's coach kicked a water bottle onto the pitch after a foul was called.
* Expert Analysis: Clear unsportsmanlike conduct. The coach demonstrates a lack of self-control and challenges the authority of the referee. The rule is clear: irresponsible behaviour in the technical area.
* Verdict: Mandatory yellow card for the coach.)";

    QString userPrompt = R"(Analyse this video and identify all soccer events and other events that are in the video. Consider only the images and disconsider any audio of narrators that may be present in the video. Provide the start and end date/time stamps in MM:SS format and a brief description.)";

    // evtExtractionWorker.setSystemPrompt(sysRefereePrompt);
    evtExtractionController.setSystemPrompt(sysRefereePrompt);

    // evtExtractionWorker.addUserMessageWithVideo("Analyse this video and identify all soccer events and other events that are in the video. Consider only the images and disconsider any audio of narrators that may be present in the video. Provide the start and end date/time stamps in HH:MM:SS format and a brief description.", "/Users/rrios/Downloads/EventExtrationDB/Pipeline-Test/bfee4c95-b189-4db2-97b5-150275336368/videoShots/segments-Gemini25_Flash5fps/segment_flash_5fps_013.mp4");

    QDir videosDir("/Users/rrios/Downloads/EventExtrationDB/Pipeline-Test/bfee4c95-b189-4db2-97b5-150275336368/videoShots/segments-Gemini25_Flash5fps");

    if (!videosDir.exists())
    {
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("testModelSubmission() - Videos directory does not exists: %1!")
                              .arg(videosDir.absolutePath()),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
    }

    videosDir.setFilter(QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks);

    QFileInfoList filesList = videosDir.entryInfoList();
    QStringList filePaths;

    for (const QFileInfo &fileInfo : qAsConst(filesList))
    {
        filePaths.append(fileInfo.absoluteFilePath());
    }

    if (filePaths.isEmpty()) return;

    evtExtractionController.addUserMessageWithVideosList(userPrompt, filePaths);

    // #ifdef QT_DEBUG
    //    LogUtils::safeLog(QString("testModelSubmission() - Event extraction: model set! %1 ")
    //                      .arg(test.first().stringfy()),
    //                      __FILE__,
    //                      __LINE__,
    //                      Q_FUNC_INFO);
    // #endif

    // evtExtractionWorker.setOutputFile("/Users/rrios/Downloads/EventExtrationDB/Pipeline-Test/test-20260210.json");
    evtExtractionController.setOutputFile("/Users/rrios/Downloads/EventExtrationDB/Pipeline-Test/test-20260225.json");

#ifdef QT_DEBUG

    LogUtils::safeLog(QString("testModelSubmission() - Event extraction: output file set!"),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    // evtExtractionWorker.process();

    evtExtractionController.startEventExtraction();

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("testModelSubmission() - Event extraction: processed!"),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("testModelSubmission() - Exit!"),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif
}

qint32 transcriptConversionCLI(CmdLineOptions cmdOptions, QString transcriptionJsonFile)
{
    // If the Deepgram Python script is used for converting the transcription.
    // Convert result to WebVTT/SRT
    QProcess process;

    cmdOptions.webVTTSRT.srtConverterCmd.append("/Users/rrios/Downloads/AudioExtrationDB/bin/WebVTTConverter.py");
    cmdOptions.webVTTSRT.srtConverterParams << transcriptionJsonFile << transcriptionJsonFile.replace("json", "webvtt");

    if (process.execute(cmdOptions.webVTTSRT.srtConverterCmd, cmdOptions.webVTTSRT.srtConverterParams) < 0)
    {
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractor::main(int, char**) - Execute: %1 \n"
                                        "                                     Error: %2!")
                          .arg(process.readAll().toStdString().c_str(),
                               process.readAllStandardError().toStdString().c_str()),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO
                                );
#endif
    }

    return 0;
}

int transcode(QStringList & args, CmdLineOptions & cmdOptions)
{
    CmdArgumentsVec cmdLineOptVec = ExtractorUtils::commandLineParser(args);

    if ( cmdLineOptVec.isEmpty())
    {
        return (-1);
    }

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("main::transcode(QStringList &, CmdLineOptions *) - Options parsed!"),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    for (CmdArgumentsVec::const_iterator cit = cmdLineOptVec.constBegin();
         cit != cmdLineOptVec.constEnd();
         cit++)
    {

#pragma clang diagnostic ignored "-Wswitch"
        switch (cit->flag)
        {
            case CmdFlags_t::Test:
            {

#ifdef QT_DEBUG
                LogUtils::safeLog(QString("main::transcode(QStringList &, CmdLineOptions *) - Remove Options!"),
                                  __FILE__,
                                  __LINE__,
                                  Q_FUNC_INFO);
#endif

                CmdArgument_st optKeyValue(cit->flag, cit->value);

                cmdLineOptVec.clear();

                cmdLineOptVec.append(optKeyValue);

                break;
            }
        }
    }

    qint32 idx = -1;

    if ( (idx = ExtractorUtils::indexOfCmdArg(CmdFlags_t::InputFile, cmdLineOptVec))  >= 0 )
    {
        QFileInfo inFileInfo(cmdLineOptVec[idx].value);

        if ( ! inFileInfo.isFile() ||
             ! inFileInfo.exists() )
        {
#ifdef QT_DEBUG
            LogUtils::safeLog(QString("EventExtractor::main(int, char**) - InFile (%1) error!")
                              .arg(inFileInfo.absoluteFilePath()),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
#endif

            return (-2);
        }

        // Create inFile directory in home directory using inFile's UUID, from Url.
        Url fileUrl(inFileInfo.absoluteFilePath());

        if ( ! fileUrl.isEmpty() )
        {
#ifdef QT_DEBUG
            if (fileUrl.isLocalFile())
            {
                LogUtils::safeLog(QString("EventExtractor::main(int, char**) - Local file path: %1!")
                                  .arg(fileUrl.path()),
                                  __FILE__,
                                  __LINE__,
                                  Q_FUNC_INFO);
            }
#endif
            QString newFileHomePath(cmdOptions.dirInfo.defaultRootDir.absolutePath() +
                                   QDir::separator() +
                                   fileUrl.getUuid().toString().remove('{').remove('}') +
                                   QDir::separator());

#ifdef QT_DEBUG
            LogUtils::safeLog(QString("EventExtractor::main(int, char**) - New possible file home path: %1!")
                              .arg(newFileHomePath),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
#endif

            QDir fileHomeDir;

            if ( ! fileHomeDir.exists(newFileHomePath) )
            {
                if ( ! fileHomeDir.mkpath(newFileHomePath) )
                {
                    return (-3);
                }
            }

            cmdOptions.dirInfo.videoDir.setFile(newFileHomePath);

#ifdef QT_DEBUG
            LogUtils::safeLog(QString("EventExtractor::main(int, char**) - New file home path: %1!")
                              .arg(cmdOptions.dirInfo.videoDir.absolutePath()),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
#endif
        }

        // Set file home directory

        // New absolute file path
        QString newFilePath(cmdOptions.dirInfo.videoDir.absolutePath() +
                            QDir::separator() +
                            fileUrl.getUuid().toString().remove('{').remove('}') +
                            "." +
                            inFileInfo.suffix() );

        if ( QFile::exists(newFilePath) &&
             ! QFile::remove(newFilePath) )
        {
            return (-4);
        }

        if ( ! QFile::copy(inFileInfo.absoluteFilePath(), newFilePath) )
        {
            return (-5);
        }

        // Set inMediaUrl new file path
        cmdOptions.fileInfo.inVideoMediaUrl.setPath(newFilePath);
        cmdOptions.fileInfo.inVideoMediaUrl.setScheme("file");

#ifdef QT_DEBUG
        if (cmdOptions.fileInfo.inVideoMediaUrl.isLocalFile())
        {
            LogUtils::safeLog(QString("EventExtractor::main(ing, char**) - Input file (%1): it is local file!")
                              .arg(cmdOptions.fileInfo.inVideoMediaUrl.path()),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
        } else {
            LogUtils::safeLog(QString("EventExtractor::main(ing, char**) - Input file (%1): it is not local file! --- %2")
                              .arg(cmdOptions.fileInfo.inVideoMediaUrl.path(), newFilePath),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
        }
#endif
    } else {

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractor::main(int, char**) - Input file is empty!"),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        return (-6);
    }

    if ( (idx = ExtractorUtils::indexOfCmdArg(CmdFlags_t::OutputFormat, cmdLineOptVec)) >= 0 )
    {
        cmdOptions.fileInfo.outputAudioFormat.append(cmdLineOptVec[idx].value);

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractor::main(int, char**) - Output file format is: %1!")
                          .arg(cmdLineOptVec[idx].value),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
    } else {

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractor::main(int, char**) - Output format is empty!"),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        return (-7);
    }

    QFileInfo inFile(cmdOptions.fileInfo.inVideoMediaUrl.path());

    cmdOptions.fileInfo.outAudioMediaUrl.setScheme("file");
    cmdOptions.fileInfo.outAudioMediaUrl.setPath(inFile.absoluteFilePath().replace(inFile.suffix(), cmdOptions.fileInfo.outputAudioFormat, Qt::CaseInsensitive));

#ifdef QT_DEBUG
    if (cmdOptions.fileInfo.outAudioMediaUrl.isLocalFile())
    {
        LogUtils::safeLog(QString("EventExtractor::main(ing, char**) - Output file (%1): it is local file!")
                          .arg(cmdOptions.fileInfo.outAudioMediaUrl.path()),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
    } else {
        LogUtils::safeLog(QString("EventExtractor::main(ing, char**) - Output file (%1): it is not local file!")
                          .arg(cmdOptions.fileInfo.outAudioMediaUrl.path()),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
    }
#endif

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractor::main(ing, char**) - Transcode instantiation!"),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    // Convert media file!
    TranscodeWorker conversion(cmdOptions.fileInfo.inVideoMediaUrl, cmdOptions.fileInfo.outAudioMediaUrl);
    conversion.process();

    // Transcribe audio using AI audio transcriptior.
    AudioTranscriptionWorker transAudio;

    // Text to Specch
//    transAudio.setText2SpeechAIModel(AIModels_t::DeepgramT2SAura2ThaliaEN);
//    QString textToSpeechText("Hello, how can I help you today?");
//    transAudio.processText2Speech(textToSpeechText);

    // Specch to Text
    transAudio.setSpeech2TextAIModel(Deepgram_AIS2TT2SModels_t::DeepgramS2TNova2GeneralPTBR);
    QString speechTextFile(cmdOptions.fileInfo.outAudioMediaUrl.toString());
    transAudio.processSpeech2Text(speechTextFile);

    // Convert result to WebVTT/SRT
    QProcess process;

    cmdOptions.webVTTSRT.srtConverterCmd.append("/Users/rrios/Downloads/AudioExtrationDB/bin/WebVTTConverter.py");
    cmdOptions.webVTTSRT.srtConverterParams << transAudio.annotationResultFile() << transAudio.annotationResultFile().replace("json", "webvtt");

    if (process.execute(cmdOptions.webVTTSRT.srtConverterCmd, cmdOptions.webVTTSRT.srtConverterParams) < 0)
    {
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractor::main(int, char**) - Execute: %1 \n"
                                  "                                    Error: %2!")
                          .arg(process.readAll().toStdString().c_str(),
                               process.readAllStandardError().toStdString().c_str()),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO
                                );
#endif
    }

    return (0);
}


void setupLoggingConnetion(EventDatabaseManager &db)
{
    QObject::connect(&db,
                     &EventDatabaseManager::dbConnected,
                     []() {
                         qDebug() << "[Signal] Datab ase Connected Successfully.";
                     });

    QObject::connect(&db,
                     &EventDatabaseManager::errorOccurred,
                     [](const QString &err) {
                         qCritical() << "[Signal] Database Error: " << err;
                     });

    QObject::connect(&db,
                     &EventDatabaseManager::eventSaved,
                     [](const QString &id) {
                         qDebug() << "[Signal] Event Saved:" << id;
                     });
}

UniqueId createMatchMediaAssets(EventDatabaseManager &db)
{
    // ==========================================
    // STEP 4: Create & Save Media Assets
    // ==========================================

    // A. Create Full Video (The Match)
    FullVideo matchVideo;
    matchVideo.setName("Champions League Final 2024");
    matchVideo.setMatchDate(QDateTime::currentDateTime());
    // Simulate binary data (e.g., file header)
    matchVideo.setFileData(QByteArray("HEX_VIDEO_DATA_HEADER..."));

    if (db.addFullVideo(matchVideo)) {
        qDebug() << "   -> Full Video added.";
    }

    return matchVideo.id();
}

UniqueId createVideoSegment(EventDatabaseManager &db, const UniqueId &parentVieoId)
{
    // B. Create Audio Track
    Audio commentary;
    // commentary.setId(QUuid::createUuid());
    commentary.setId(UniqueId());
    commentary.setLanguage("en-GB");
    QJsonObject trans;
    trans["text"] = "What a magnificent strike from outside the box!";
    trans["speaker"] = "Commentator A";
    commentary.setAnnotation(trans);

    // C. Create Video Segment (The Shot)
    // QUuid segmentId = QUuid::createUuid();
    VideoSegmentInfo segment;
    segment.setFullVideoId(parentVieoId); // Link to parent
    segment.setTiming(450.0, 465.0); // 15 seconds duration
    segment.setState("Analyzed");
    segment.setAudioTrack(commentary); // Embed audio

    if (db.addVideoSegment(segment)) {
        qDebug() << "   -> Segment & Audio added.";
    }

    return segment.id();
}

QSharedPointer<CompoundEvent> createEventHierarchy(const UniqueId &linkedMediaId, const QString &mediaFilePath)
{
    // A. Root Event (Compound)
    auto matchEvent = QSharedPointer<CompoundEvent>::create();
    matchEvent->setName("Match: Real Madrid vs Dortmund");
    matchEvent->setDescription("Final match event container.");
    matchEvent->setLocation(Location("Wembley Stadium", "London", "Olympic Way", "", "1", "UK"));
    matchEvent->setTimeRange(QDateTime::currentDateTime(), QDateTime::currentDateTime().addSecs(7200));

    // B. Child Event (Atomic)
    auto goalEvent = QSharedPointer<AtomicEvent>::create();
    goalEvent->setName("Goal: Vinicius Jr");
    goalEvent->setConcept("Goal");

    // 1. Link the physical file path for the UI/Player
    goalEvent->setMediaPath(mediaFilePath);

    // 2. USE THE ID: Use the linkedMediaId as a seed for this event's ID
    // This ensures that the event's identity is tied to the media it represents.
    // If your Event class has a setLinkedMediaId, use that;
    // otherwise, we refresh the ID using the media ID as part of the context.
    QByteArray seed = linkedMediaId.toHex().toUtf8() + goalEvent->name().toUtf8();
    goalEvent->setId(UniqueId(ExtractorUtils::generateContentSeed(seed, QCryptographicHash::Md4)));

    goalEvent->setLinkedMediaId(linkedMediaId);

    // Add Actors
    goalEvent->addActor(Actor(UniqueId(), "Vinicius Jr", "Forward", 10));
    goalEvent->addActor(Actor(UniqueId(), "Kobel", "Goalkeeper", 17));

    // Add Child to Parent
    matchEvent->addSubEvent(goalEvent);

    return matchEvent;
}

void resolveAndPrintMedia(EventDatabaseManager &db, const QString &path)
{
    UniqueId segId = UniqueId(ExtractorUtils::generateContentSeed(path, QCryptographicHash::Md4));
    VideoSegmentInfo seg = db.getSegment(segId);

    if (!seg.id().isEmpty())
    {
        qDebug() << "      [Linked Media Found] Duration:" << seg.duration() << "s";
        qDebug() << "      Text:" << seg.audioTrack().annotation()["text"].toString();
    }
}

void verifySavedData(EventDatabaseManager &db)
{
    qDebug() << "\n=== Verifying Data via Load ===";

    auto loadedRoots = db.getAllRootEvents();

    for (const auto &root : qAsConst(loadedRoots))
    {
        qDebug() <<  "Root:" << root->name() << "| Loc:" << root->location().city();

        if (auto compound = qSharedPointerDynamicCast<CompoundEvent>(root))
        {
            for (const auto &child : compound->subEvents())
            {
                qDebug() << " - Child: " << child->name() << " (Actors: "<< child->actors().size() << ")";

                // Use a helper to resolve media links to keep loop clean
                resolveAndPrintMedia(db, child->mediaPath());
            }
        }
    }
}

void setupLoggingConnection(EventDatabaseManager &db)
{
    // Connect the error signal to a lambda that prints to the critical log
    QObject::connect(&db, &EventDatabaseManager::errorOccurred, [](const QString &msg) {
        qCritical() << "[DATABASE ERROR]:" << msg;
    });

    // Connect success signals to keep the user informed during long operations
    QObject::connect(&db, &EventDatabaseManager::dbConnected, []() {
        qInfo() << "[DATABASE]: Connection established successfully.";
    });

    QObject::connect(&db, &EventDatabaseManager::eventSaved, [](const QString &hexId) {
        qInfo() << "[DATABASE]: Event saved successfully with ID:" << hexId;
    });

    QObject::connect(&db, &EventDatabaseManager::fullVideoSaved, [](const UniqueId &id) {
        qInfo() << "[DATABASE]: Video assets registered:" << id.toHex().left(8) << "...";
    });
}

// Example of how to use the timeline in your UI
void updateTimelineUI(const UniqueId &videoId)
{
    auto timeline = EventDatabaseManager::instance().getTimelineForVideo(videoId);

    if (timeline.isEmpty()) return;

    QDateTime matchStart = timeline.first()->startTime();

    for (const auto &event : qAsConst(timeline))
    {
        // Calculate minutes elapsed since the start of the match
        qint64 minutes = matchStart.secsTo(event->startTime()) / 60;

        qDebug() << QString("[%1'] %2 - %3")
                        .arg(minutes)
                        .arg(event->name(), event->concept());
    }
}

/**
 * Event Database Manager
 */
void runEventDatabaseDemo()
{
    qDebug() << "=================================================";
    qDebug() << "   NARRATIVE VIDEO ANALYTICS: FULL SYSTEM DEMO   ";
    qDebug() << "=================================================";

    // 1. Singleton Access & Logging
    EventDatabaseManager &db = EventDatabaseManager::instance();
    setupLoggingConnection(db);

    // 2. Database Connection
    // Initializes SQLite, FTS5 Virtual Tables, and Sync Triggers
    QString dbPath = "match_analysis_pro.db";
    if (!db.connectToDatabase(dbPath))
    {
        qCritical() << "Aborting: Database connection failed.";
        return;
    }

    // 3. Phase 1: Ingest Media & Assets
    qDebug() << "\n[PHASE 1] Ingesting Media Assets...";
    UniqueId videoId = createMatchMediaAssets(db);

    // Create a segment (e.g., 10s clip) and define its physical location
    QString segmentFilePath = "media/segments/final_goal_vini.mp4";
    UniqueId segmentId = createVideoSegment(db, videoId);
    qDebug() << "-> Video Segment Linked:" << segmentId.toHex().left(12);

    // 4. Phase 2: Construct Narrative Hierarchy
    qDebug() << "\n[PHASE 2] Constructing Narrative Tree...";
    // Link the event to the database segment ID and the UI file path
    auto matchEvent = createEventHierarchy(segmentId, segmentFilePath);

    // 5. Phase 3: High-Performance Persistence
    qDebug() << "\n[PHASE 3] Persisting Data (Atomic Transaction)...";
    // This triggers recursive saving, actor junction updates, and FTS5 indexing
    if (!db.saveEvent(matchEvent))
    {
        qCritical() << "Persistence failed.";
        return;
    }

    // 6. Phase 4: Verification via Recursive CTE
    qDebug() << "\n[PHASE 4] Verifying Rehydration (Recursive CTE Pass)...";
    // Fetch the entire tree in ONE SQL query
    auto loadedMatch = db.getEventById(matchEvent->id().toHex());

    if (loadedMatch)
    {
        qDebug() << "-> Root Recovered:" << loadedMatch->name();

        auto children = loadedMatch.staticCast<CompoundEvent>()->subEvents();
        if (!children.isEmpty())
        {
            const auto &goal = children.first();
            qDebug() << "-> Sub-event:" << goal->name();
            qDebug() << "-> Breadcrumb:" << goal->breadcrumb(" > ");
            qDebug() << "-> Media Link:" << (goal->linkedMediaId() == segmentId ? "VERIFIED" : "BROKEN");
        }
    }

    // 7. Phase 5: Advanced Search (FTS5)
    qDebug() << "\n[PHASE 5] Testing Search Index (FTS5 Rank)...";
    auto results = db.searchEvents("Vinicius");
    qDebug() << "-> Search hits for 'Vinicius':" << results.size();
    for (const auto &res : qAsConst(results))
    {
        qDebug() << "   * Found:" << res->name() << "| Concept:" << res->concept();
    }

    // 8. Phase 6: Timeline & Relational Joins
    qDebug() << "\n[PHASE 6] Generating Chronological Timeline...";
    // Joins Events -> Segments -> Full Videos
    auto timeline = db.getTimelineForVideo(videoId);
    for (int i = 0; i < timeline.size(); ++i)
    {
        const auto &e = timeline.at(i);
        qDebug() << QString("[%1] %2 | Start: %3 | Path: %4")
                        .arg(i + 1)
                        .arg(e->name(), e->startTime().toString("hh:mm:ss"), e->mediaPath());
    }

    // 9. Phase 7: Export & Portable Package
    qDebug() << "\n[PHASE 7] Packaging Portable Match Bundle...";
    QString exportDir = "./match_export_vini_2026";
    if (db.exportMatchPackage(matchEvent->id().toHex(), exportDir))
    {
        qDebug() << "-> Export Successful. Manifest and media ready.";

        // Final Test: Re-import to simulate "New Machine" migration
        qDebug() << "-> Simulating Import on New Machine...";
        if (db.importMatchPackage(exportDir))
        {
            qDebug() << "-> IMPORT SUCCESSFUL: Tree reconstructed and paths re-mapped.";
        }
    }

    qDebug() << "\n=================================================";
    qDebug() << "           DEMO COMPLETED SUCCESSFULLY           ";
    qDebug() << "=================================================";
}


// =============================================== Complete Event Extraction Function ===============================================


// ====================================
TranscodeController * setupTranscodeController(QApplication * evtExtractorApp, QSharedPointer<CmdLineOptions> cmdOptions, QThread *& thread)
{
    TranscodeController * transcodeController = new TranscodeController(); // generator

    transcodeController->setupAudioTranscoding(cmdOptions->fileInfo.inVideoMediaUrl,
                                               cmdOptions->fileInfo.outAudioMediaUrl);

    thread = new QThread(evtExtractorApp);
    transcodeController->moveToThread(thread);

    QObject::connect(thread,
                     &QThread::finished,
                     transcodeController,
                     &QObject::deleteLater);

    return transcodeController;
}

AudioTranscriptionController * setupAudioTranscriptionController(QApplication * evtExtractorApp, QSharedPointer<CmdLineOptions> cmdOptions, QThread *& thread)
{
    AudioTranscriptionController * audioTranscriptionController = new AudioTranscriptionController;

    audioTranscriptionController->setModel(Deepgram_AIS2TT2SModels_t::DeepgramS2TNova2GeneralPTBR);
    audioTranscriptionController->setAnnotationDir(cmdOptions->dirInfo.annotation_and_transcriptsDir.absoluteFilePath());
    audioTranscriptionController->setAnnotationClipsOutputDir(cmdOptions->dirInfo.annotationClipsDir.absoluteFilePath());

    thread = new QThread(evtExtractorApp);
    audioTranscriptionController->moveToThread(thread);

    QObject::connect(thread,
                     &QThread::finished,
                     audioTranscriptionController,
                     &QObject::deleteLater);

    return audioTranscriptionController;
}

ConverterController * setupConverterController(QApplication * evtExtractorApp, QSharedPointer<CmdLineOptions> cmdOptions, QThread *& thread)
{
    ConverterController * convertTranscriptionController = new ConverterController; // convert transcription to SRT/WebVTT
    convertTranscriptionController->setLineLength(200);
    convertTranscriptionController->setConverterType(ConverterType_t::DeepgramWebVttConverter);
    convertTranscriptionController->setClipSec(*Constants::Q_QGS_DEFAULT_CLIP_SEC/6.0); // 5 min
    convertTranscriptionController->setTranscriptionClipsOutputDir(cmdOptions->dirInfo.transcriptClipsDir.absoluteFilePath());

    thread = new QThread(evtExtractorApp);
    convertTranscriptionController->moveToThread(thread);

    QObject::connect(thread,
                     &QThread::finished,
                     convertTranscriptionController,
                     &QObject::deleteLater);

    return convertTranscriptionController;
}

AudioSegmentationController * setupAudioSegmentationController(QApplication * evtExtractorApp, QSharedPointer<CmdLineOptions> cmdOptions, QThread *& thread)
{
    AudioSegmentationController * audioSegController = new AudioSegmentationController;
    // audioSegController->setInputAudio(cmdOptions.fileInfo.outAudioMediaUrl.path());
    audioSegController->setClipSec(*Constants::Q_QGS_DEFAULT_CLIP_SEC/6.0); // 5 min clips

    // audioSegController->setOutputDir(QDir(cmdOptions.dirInfo.audioClipsDir.absoluteFilePath()).filePath(cmdOptions.fileInfo.outAudioMediaUrl.fileBaseName()));
    audioSegController->setOutputDir(cmdOptions->dirInfo.audioClipsDir.absoluteFilePath());

    thread = new QThread(evtExtractorApp);
    audioSegController->moveToThread(thread);

    QObject::connect(thread,
                     &QThread::finished,
                     audioSegController,
                     &QObject::deleteLater);

    return audioSegController;
}

VideoSegmentationController * setupVideoSegmentationController(QApplication * evtExtractorApp, QSharedPointer<CmdLineOptions> cmdOptions, QThread *& thread)
{
    VideoSegmentationController * videoSegController  = new VideoSegmentationController; // consumer

    QString targetVideoClipsDir = QDir(cmdOptions->dirInfo.videoClipsDir.absoluteFilePath()).filePath("segments-Gemini25_Flash5fps");

    videoSegController->setInputVideo(cmdOptions->fileInfo.inVideoMediaUrl.path());

    videoSegController->setupVideoCutter(ModelCode_t::Gemini25_Flash5fps21crf,
                                         targetVideoClipsDir,
                                         *Constants::Q_QGS_DEFAULT_CLIP_SEC,
                                         6.0,
                                         SegmentationMode_t::UseLibrary);

    thread = new QThread(evtExtractorApp);
    videoSegController->moveToThread(thread);

    QObject::connect(thread,
                     &QThread::finished,
                     videoSegController,
                     &QObject::deleteLater);

    return videoSegController;
}

EventExtractionController * setupEventExtractionController(QApplication * evtExtractorApp, QSharedPointer<CmdLineOptions> cmdOptions, QThread *& thread)
{
    EventExtractionController * evtExtractionController = new EventExtractionController;

    // Set up the controller (Replace with your actual testModelSubmission logic)
    evtExtractionController->setupModel(AIModels_t::Google_Gemini_API_25Flash);

    // evtExtractionController->setOutputFile("/Users/rrios/Downloads/EventExtrationDB/Pipeline-Test/Google_Gemini_API_20Flash-20260413-15h20.md");
    evtExtractionController->setOutputFile(cmdOptions->dirInfo.defaultRootDir.absoluteFilePath()+"/Google_Gemini_API_20Flash-20260413-15h20.md");

    QString sysRefereePrompt = PromptManager::instance().getSystemProfile(Persona::FIFA_Referee);
    evtExtractionController->setSystemPrompt(sysRefereePrompt);

    qDebug() << "main() ------- System profile: " << sysRefereePrompt;

    thread = new QThread(evtExtractorApp);
    evtExtractionController->moveToThread(thread);

    QObject::connect(thread,
                     &QThread::finished,
                     evtExtractionController,
                     &QObject::deleteLater);

    return evtExtractionController;
}

// ====================================


//////////////////// The complete working Pipeline Main Function ////////////////////

// Complete Working Pipeline main
// int main(int argc, char *argv[])
int mainCompleteWorkingPipeline(int argc, char *argv[])
{
    QApplication EventExtractorApp(argc, argv);
    // FORCE Qt to keep the event loop running even with no windows open
    EventExtractorApp.setQuitOnLastWindowClosed(false);

    qRegisterMetaType<utils::UniqueId>("UniqueId");
    qRegisterMetaType<FileInfo>("FileInfo");
    qRegisterMetaType<ClipData>("ClipData");
    qRegisterMetaType<SegmentsWithPrompts>("SegmentsWithPrompts");
    // qRegisterMetaType<Audio>("Actor");
    // qRegisterMetaType<Audio>("Audio");
    // qRegisterMetaType<Audio::AudioList_t>("Audio::AudioList_t");
    // qRegisterMetaType<VideoSegmentInfo>("VideoSegmentInfo");
    // qRegisterMetaType<VideoSegmentInfo::VideoSegmentInfoList_t>("VideoSegmentInfo::VideoSegmentInfoList_t");

#ifdef QT_DEBUG
    // The LogUtils must be run before the application logic start.
    LogUtils::enableLog();
#endif

    if (argc <= 0 || argv == nullptr)
    {
        qCritical() << "Invalid command line argumtens.";
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractorApp::main(int, char**) - Invalid command line argumtens."),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        return -1;
    }

    QTranslator translator;

    const QStringList uiLanguages = QLocale::system().uiLanguages();

    for (const QString &locale : uiLanguages)
    {
        const QString baseName = "EventExtractorApp_" + QLocale(locale).name();

        if (translator.load(":/i18n/" + baseName))
        {
            EventExtractorApp.installTranslator(&translator);

            break;
        }
    }

    // 1. Argument Parsing
    // Note: Qt arguments include the program name at index 0, matching sys.argv
    QStringList args = EventExtractorApp.arguments();

    // The app arguments list must have at least seven items: the command, the input flag
    // ("-i" or "--input"), the input file (a video), the output audio format flag
    // ("-of" or "--audio_format"), the output audio format ("mp3", "wav", etc.),
    // target dir flag ("-td" of "--target_dir"), and the target dir.
    if (args.length() < 7)
    {
        qInfo().noquote() << "Please provide the <input file path>, <output file path> and target audio file format as command line input" << Qt::endl;

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractorApp::main(int, char**) - Usage: %1 -i <input video file> -af <output audio format> -td <destination dir>!")
                              .arg(args.at(0)),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        return -2; // sys.exit() equivalent
    }

    CmdArgumentsVec parsedOptions = ExtractorUtils::commandLineParser(args);

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractorApp::main(int, char**) - Arguments parsed: %1!")
                          .arg(args.at(0)),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    qint32 idx = -1;

    if ( (idx = ExtractorUtils::indexOfCmdArg(CmdFlags_t::Test, parsedOptions) ) >= 0 )
    {
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractorApp::main(int, char**) - Testing!"),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractorApp::main(int, char**) - Testing %1!")
                              .arg(parsedOptions.at(idx).value),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        if ( ! QString::compare(parsedOptions[idx].value, "preamble", Qt::CaseInsensitive) )
        {
            //            testPreamble();

        } else if ( ! QString::compare(parsedOptions[idx].value, "message", Qt::CaseInsensitive) )
        {
            //            testMessage();

        } else if ( ! QString::compare(parsedOptions[idx].value, "request", Qt::CaseInsensitive) )
        {
            //            testRequest();

        } else if ( ! QString::compare(parsedOptions[idx].value, "response", Qt::CaseInsensitive) )
        {
            //            testResponse();
        }

        return 0;
    }

    auto cmdOptions = QSharedPointer<CmdLineOptions>::create();

    if (ExtractorUtils::initCmdOptions(parsedOptions, cmdOptions.data()) < 0)
    {
        return -3;
    }

#ifdef QT_DEBUG
    if (cmdOptions->fileInfo.outAudioMediaUrl.isLocalFile())
    {
        LogUtils::safeLog(QString("EventExtractorApp::main(ing, char**) - Output file (%1): it is local file!")
                              .arg(cmdOptions->fileInfo.outAudioMediaUrl.path()),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
    } else {
        LogUtils::safeLog(QString("EventExtractorApp::main(ing, char**) - Output file (%1): it is not local file!")
                              .arg(cmdOptions->fileInfo.outAudioMediaUrl.path()),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
    }

    LogUtils::safeLog(QString("EventExtractor::main(ing, char**) - Transcode instantiation!"),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    // 4. Centralized Error Handling
    auto handleError = [&EventExtractorApp](const QString &msg) {
        qCritical() << "EventExtractorApp::main() -- Fatal Pipeline Error:" << msg;
        // Exit with error code 1
        EventExtractorApp.exit(1);
    };

    // 2. Create Controllers
    TranscodeController * transcodeController = new TranscodeController; // generator

    transcodeController->setupAudioTranscoding(cmdOptions->fileInfo.inVideoMediaUrl,
                                               cmdOptions->fileInfo.outAudioMediaUrl);

    QThread * transcodeControllerThread = new QThread(&EventExtractorApp);
    transcodeController->moveToThread(transcodeControllerThread);

    QObject::connect(transcodeControllerThread,
                     &QThread::finished,
                     transcodeController,
                     &QObject::deleteLater);

    QObject::connect(transcodeController,
                     &TranscodeController::error,
                     &EventExtractorApp,
                     handleError);

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractorApp::main(ing, char**) - Input file: %1!")
                          .arg(cmdOptions->fileInfo.inVideoMediaUrl.path()),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    AudioTranscriptionController * transcribeAudioController = new AudioTranscriptionController;
    transcribeAudioController->setModel(Deepgram_AIS2TT2SModels_t::DeepgramS2TNova2GeneralPTBR);
    transcribeAudioController->setAnnotationDir(cmdOptions->dirInfo.annotation_and_transcriptsDir.absoluteFilePath());
    transcribeAudioController->setAnnotationClipsOutputDir(cmdOptions->dirInfo.annotationClipsDir.absoluteFilePath());
    transcribeAudioController->setClipSec(*Constants::Q_QGS_DEFAULT_CLIP_SEC/6.0); // 5 min / 6.0x slowdown factor

    QThread * transcribeAudioControllerThread = new QThread(&EventExtractorApp);
    transcribeAudioController->moveToThread(transcribeAudioControllerThread);

    QObject::connect(transcribeAudioControllerThread,
                     &QThread::finished,
                     transcribeAudioController,
                     &QObject::deleteLater);

    QObject::connect(transcribeAudioController,
                     &AudioTranscriptionController::error,
                     &EventExtractorApp,
                     handleError);

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractorApp::main(ing, char**) - Transcription directory: %1 --- %2!")
                          .arg(cmdOptions->dirInfo.annotation_and_transcriptsDir.absoluteDir().path(),
                               cmdOptions->dirInfo.annotation_and_transcriptsDir.fileName()),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    // 2.3. Transcription conversion worker creation.
    ConverterController * convertTranscriptionController = new ConverterController; // convert transcription to SRT/WebVTT

    convertTranscriptionController->setLineLength(200);
    convertTranscriptionController->setConverterType(ConverterType_t::DeepgramWebVttConverter);
    convertTranscriptionController->setClipSec(*Constants::Q_QGS_DEFAULT_CLIP_SEC/6.0); // 5 min / 6.0x slowdown factor
    convertTranscriptionController->setTranscriptionClipsOutputDir(cmdOptions->dirInfo.transcriptClipsDir.absoluteFilePath());

    QThread * convertTranscriptionControllerThread = new QThread(&EventExtractorApp);
    convertTranscriptionController->moveToThread(convertTranscriptionControllerThread);

    QObject::connect(convertTranscriptionControllerThread,
                     &QThread::finished,
                     convertTranscriptionController,
                     &QObject::deleteLater);

    QObject::connect(convertTranscriptionController,
                     &ConverterController::conversionError,
                     &EventExtractorApp,
                     handleError);

    // 2.4. Audio segmentation worker creation.
    AudioSegmentationController * audioSegController = new AudioSegmentationController;
    audioSegController->setClipSec(*Constants::Q_QGS_DEFAULT_CLIP_SEC/6.0); // 5 min clips / 6.0x slowdown factor
    audioSegController->setOutputDir(cmdOptions->dirInfo.audioClipsDir.absoluteFilePath());

    QThread * audioSegControllerThread = new QThread(&EventExtractorApp);
    audioSegController->moveToThread(audioSegControllerThread);

    QObject::connect(audioSegControllerThread,
                     &QThread::finished,
                     audioSegController,
                     &QObject::deleteLater);

    QObject::connect(audioSegController,
                     &AudioSegmentationController::error,
                     &EventExtractorApp,
                     handleError);

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractorApp::main(ing, char**) - Output MP3 audio file: %1!")
                          .arg(cmdOptions->fileInfo.outAudioMediaUrl.path()),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    // 2.5. Video segmentation worker creation.
    // The VideoProcessor constructor will internally initialize
    // VideoProcessorData and the VideoSegmentModel.
    // Process video and split it in shots of 5 min and no audio (audio is silence).
    VideoSegmentationController * videoSegController  = new VideoSegmentationController; // consumer
    videoSegController->setInputVideo(cmdOptions->fileInfo.inVideoMediaUrl.path());

    QString targetVideoClipsDir = QDir(cmdOptions->dirInfo.videoClipsDir.absoluteFilePath()).filePath("segments-Gemini25_Flash5fps-5minCLip-6.0Slowdown");
    videoSegController->setupVideoCutter(ModelCode_t::Gemini25_Flash5fps21crf,
                                         targetVideoClipsDir,
                                         *Constants::Q_QGS_DEFAULT_CLIP_SEC,
                                         6.0, // Slowdown Factor
                                         SegmentationMode_t::UseLibrary);

    QThread * videoSegControllerThread = new QThread(&EventExtractorApp);
    videoSegController->moveToThread(videoSegControllerThread);

    QObject::connect(videoSegControllerThread,
                     &QThread::finished,
                     videoSegController,
                     &QObject::deleteLater);

    QObject::connect(videoSegController,
                     &VideoSegmentationController::error,
                     &EventExtractorApp,
                     handleError);

    // 2.6 Create the controller on the HEAP so it survives the scope of main()
    ManifestSynchronizer * manifestsSynchronizer = new ManifestSynchronizer;
    manifestsSynchronizer->setOutputPath(QDir(cmdOptions->dirInfo.defaultRootDir.absoluteFilePath()).filePath("synchronized_manifests.json"));

    QThread * manifestsSynchronizerThread = new QThread(&EventExtractorApp);
    manifestsSynchronizer->moveToThread(manifestsSynchronizerThread);

    QObject::connect(manifestsSynchronizerThread,
                     &QThread::finished,
                     manifestsSynchronizer,
                     &QObject::deleteLater);

    QObject::connect(manifestsSynchronizer,
                     &ManifestSynchronizer::error,
                     &EventExtractorApp,
                     handleError);

    QObject::connect(manifestsSynchronizer,
                     &ManifestSynchronizer::collectionComplete,
                     manifestsSynchronizer,
                     &ManifestSynchronizer::synchronizeManifests,
                     Qt::QueuedConnection);

    QObject::connect(manifestsSynchronizer,
                     &ManifestSynchronizer::finished,
                     [manifestsSynchronizer](FileInfo fInfo) {

                         if (fInfo.success)
                         {
                             qDebug() << "✅ Synchronization completed successfully!";
                             qDebug() << "  Synchronized segments:" << manifestsSynchronizer->getSynchronizedSegmentCount();
                             qDebug() << "  Output file:" << fInfo.url.path();
                         } else {
                             qDebug() << "❌ Synchronization failed!";
                             qDebug() << "  Error:" << manifestsSynchronizer->lastError();
                             qCritical() << "Sync failed!";

                             // Optional: Stop the event loop once finished
                             QApplication::quit();
                         }
                     });

    // 2.6 Create the controller on the HEAP so it survives the scope of main()
    EventExtractionController * evtExtractionController = new EventExtractionController;

    // Set up the controller (Replace with your actual testModelSubmission logic)
    evtExtractionController->setupModel(AIModels_t::Google_Gemini_API_25Flash);

    evtExtractionController->setOutputFile(cmdOptions->dirInfo.defaultRootDir.absoluteFilePath()+"/Google_Gemini_API_25-20260619-17h08.md");

    // QStringList searchTerms = {"goal", "foul", "corner kick", "penalty"};
    QStringList searchTerms = {"gol", "falta", "escanteio", "penalti", "cartao vermelho", "cartao amarelo"};

    evtExtractionController->addTermsForSearching(searchTerms);
    evtExtractionController->setSearchType(KeywordMatchType_t::Any);
    evtExtractionController->setMinConfidenceThreshold(0.80);

    evtExtractionController->setTaskTemplateType(TaskTemplate::Video_Analysis_Task);
    QString sysRefereePrompt = PromptManager::instance().getSystemProfile(Persona::FIFA_Referee);
    evtExtractionController->setSystemPrompt(sysRefereePrompt);

    QThread * evtExtractionControllerThread = new QThread(&EventExtractorApp);
    evtExtractionController->moveToThread(evtExtractionControllerThread);

    QObject::connect(evtExtractionControllerThread,
                     &QThread::finished,
                     evtExtractionController,
                     &QObject::deleteLater);

    QObject::connect(evtExtractionController,
                     &EventExtractionController::error,
                     &EventExtractorApp,
                     handleError);

    qDebug() << "main() ------- System profile: " << sysRefereePrompt;

    QObject::connect(transcribeAudioController,
                     &AudioTranscriptionController::manifestReady,
                     manifestsSynchronizer,
                     &ManifestSynchronizer::handleInput);

    QObject::connect(transcribeAudioController,
                     &AudioTranscriptionController::manifestReady,
                     &EventExtractorApp,
                     [](const FileInfo &fInfo){
                         qDebug() << "MAIN THREAD: Received AudioTranscriptionController Manifest Relay:" << fInfo.url.path();
                     });


    QObject::connect(convertTranscriptionController,
                     &ConverterController::manifestReady,
                     manifestsSynchronizer,
                     &ManifestSynchronizer::handleInput);

    QObject::connect(convertTranscriptionController,
                     &ConverterController::manifestReady,
                     &EventExtractorApp,
                     [](const FileInfo &fInfo){
                         qDebug() << "MAIN THREAD: Received ConverterController Manifest Relay:" << fInfo.url.path();
                     });


    QObject::connect(audioSegController,
                     &AudioSegmentationController::manifestReady,
                     manifestsSynchronizer,
                     &ManifestSynchronizer::handleInput);

    QObject::connect(audioSegController,
                     &AudioSegmentationController::manifestReady,
                     &EventExtractorApp,
                     [](const FileInfo &fInfo){
                         qDebug() << "MAIN THREAD: Received AudioSegmentationController Manifest Relay:" << fInfo.url.path();
                     });


    QObject::connect(videoSegController,
                     &VideoSegmentationController::manifestReady,
                     manifestsSynchronizer,
                     &ManifestSynchronizer::handleInput);

    QObject::connect(videoSegController,
                     &VideoSegmentationController::manifestReady,
                     &EventExtractorApp,
                     [](const FileInfo &fInfo){
                         qDebug() << "MAIN THREAD: Received VideoSegmentationController Manifest Relay:" << fInfo.url.path();
                     });



    // Pipeline
    QObject::connect(transcodeController,
                     &TranscodeController::finished,
                     transcribeAudioController,
                     &AudioTranscriptionController::handleInput);

    QObject::connect(transcodeController,
                     &TranscodeController::finished,
                     &EventExtractorApp,
                     [](){
                         qDebug() << "MAIN THREAD: TranscodeController reported FINISHED. Triggering AudioTranscriptionController...";
                     });


    QObject::connect(transcribeAudioController,
                     &AudioTranscriptionController::finished,
                     convertTranscriptionController,
                     &ConverterController::handleInput);

    QObject::connect(transcribeAudioController,
                     &AudioTranscriptionController::finished,
                     &EventExtractorApp,
                     [](){
                         qDebug() << "MAIN THREAD: AudioTranscriptionController reported FINISHED. Triggering ConverterController...";
                     });

    QObject::connect(convertTranscriptionController,
                     &ConverterController::finished,
                     &EventExtractorApp,
                     [](FileInfo transcriptionVTT){
                         qDebug() << "MAIN THREAD: ConverterController reported FINISHED." << transcriptionVTT.url.path();
                     });


    QObject::connect(transcodeController,
                     &TranscodeController::finished,
                     audioSegController,
                     &AudioSegmentationController::handleInput);

    QObject::connect(audioSegController,
                     &AudioSegmentationController::finished,
                     &EventExtractorApp,
                     [](){
                         qDebug() << "MAIN THREAD: AudioSegmentationController reported FINISHED. Triggering Video...";
                     });


    QObject::connect(transcodeController,
                     &TranscodeController::finished,
                     videoSegController,
                     [cmdOptions, videoSegController]() {
                         FileInfo videoFileInfo(cmdOptions->fileInfo.inVideoMediaUrl.path(), FileType_t::VideoFile);
                         videoSegController->handleInput(videoFileInfo);
                     });

    QObject::connect(videoSegController,
                     &VideoSegmentationController::finished,
                     &EventExtractorApp,
                     [](){
                         qDebug() << "MAIN THREAD: VideoSegmentationController reported FINISHED. Triggering ManifestSynchronizer...";
                     });

    QObject:: connect(manifestsSynchronizer,
                     &ManifestSynchronizer::finished,
                     evtExtractionController,
                     &EventExtractionController::handleInput);

    QObject::connect(manifestsSynchronizer,
                     &ManifestSynchronizer::finished,
                     &EventExtractorApp,
                     [](){
                         qDebug() << "MAIN THREAD: ManifestSynchronizer reported FINISHED. Triggering ManifestSynchronizer...";
                     });

    // Debugging the hang: Print when conversion specifically finishes

    // 3. Connect the finished signal so the app quits cleanly when done
    QObject::connect(evtExtractionController,
                     &EventExtractionController::finished,
                     &EventExtractorApp,
                     [transcodeControllerThread,
                      transcribeAudioControllerThread,
                      convertTranscriptionControllerThread,
                      audioSegControllerThread,
                      videoSegControllerThread,
                      manifestsSynchronizerThread,
                      evtExtractionControllerThread]() {
                         // Tell threads to gracefully exit their event loops
                         transcodeControllerThread->quit();
                         transcodeControllerThread->wait();
                         transcodeControllerThread->deleteLater();

                         transcribeAudioControllerThread->quit();
                         transcribeAudioControllerThread->wait();
                         transcribeAudioControllerThread->deleteLater();

                         convertTranscriptionControllerThread->quit();
                         convertTranscriptionControllerThread->wait();
                         convertTranscriptionControllerThread->deleteLater();

                         audioSegControllerThread->quit();
                         audioSegControllerThread->wait();
                         audioSegControllerThread->deleteLater();

                         videoSegControllerThread->quit();
                         videoSegControllerThread->wait();
                         videoSegControllerThread->deleteLater();

                         manifestsSynchronizerThread->quit();
                         manifestsSynchronizerThread->wait();
                         manifestsSynchronizerThread->deleteLater();

                         evtExtractionControllerThread->quit();
                         evtExtractionControllerThread->wait();
                         evtExtractionControllerThread->deleteLater();

                         // Now quit the main application
                         QApplication::quit();
                     });

    transcodeControllerThread->start();
    transcribeAudioControllerThread->start();
    convertTranscriptionControllerThread->start();
    audioSegControllerThread->start();
    videoSegControllerThread->start();
    manifestsSynchronizerThread->start();
    evtExtractionControllerThread->start();

    QTimer::singleShot(0, transcodeController, &TranscodeController::startTranscoding);

    qint32 result = EventExtractorApp.exec();

    qDebug() << "Clean exit.";

    return result;
}
/////////////////////////////////////////////////////////////////////////////////////

/////////// The Pipeline for transcoding and audio and video segmentation ///////////

// This main function is for transcoding (extract audio from video) and segmenting audio and video.
int mainAudioAndVideoSegTest(int argc, char *argv[])
// int main(int argc, char *argv[])
{
    QApplication EventExtractorApp(argc, argv);

    // FORCE Qt to keep the event loop running even with no windows open
    EventExtractorApp.setQuitOnLastWindowClosed(false);

    qRegisterMetaType<utils::UniqueId>("UniqueId");
    qRegisterMetaType<FileInfo>("FileInfo");
    qRegisterMetaType<ClipData>("ClipData");
    qRegisterMetaType<SegmentsWithPrompts>("SegmentsWithPrompts");

#ifdef QT_DEBUG
    // The LogUtils must be run before the application logic start.
    LogUtils::enableLog();
#endif

    if (argc <= 0 || argv == nullptr)
    {
        qCritical() << "Invalid command line argumtens.";
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractorApp::main(int, char**) - Invalid command line argumtens."),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        return -1;
    }

    QTranslator translator;

    const QStringList uiLanguages = QLocale::system().uiLanguages();

    for (const QString &locale : uiLanguages)
    {
        const QString baseName = "EventExtractorApp_" + QLocale(locale).name();

        if (translator.load(":/i18n/" + baseName))
        {
            EventExtractorApp.installTranslator(&translator);

            break;
        }
    }

    // 1. Argument Parsing
    // Note: Qt arguments include the program name at index 0, matching sys.argv
    QStringList args = EventExtractorApp.arguments();

    // The app arguments list must have at least seven items: the command, the input flag
    // ("-i" or "--input"), the input file (a video), the output audio format flag
    // ("-of" or "--audio_format"), the output audio format ("mp3", "wav", etc.),
    // target dir flag ("-td" of "--target_dir"), and the target dir.
    if (args.length() < 5)
    {
        qInfo().noquote() << "Please provide the <input file path>, <output file path> and target audio file format as command line input" << Qt::endl;

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractorApp::main(int, char**) - Usage: %1 -i <input video file> -af <output audio format> -td <destination dir>!")
                              .arg(args.at(0)),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        return -2; // sys.exit() equivalent
    }

    CmdArgumentsVec parsedOptions = ExtractorUtils::commandLineParser(args);

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractorApp::main(int, char**) - Arguments parsed: %1!")
                          .arg(args.at(0)),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    qint32 idx = -1;

    if ( (idx = ExtractorUtils::indexOfCmdArg(CmdFlags_t::Test, parsedOptions) ) >= 0 )
    {
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractorApp::main(int, char**) - Testing!"),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractorApp::main(int, char**) - Testing %1!")
                              .arg(parsedOptions.at(idx).value),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        if ( ! QString::compare(parsedOptions[idx].value, "preamble", Qt::CaseInsensitive) )
        {
            //            testPreamble();

        } else if ( ! QString::compare(parsedOptions[idx].value, "message", Qt::CaseInsensitive) )
        {
            //            testMessage();

        } else if ( ! QString::compare(parsedOptions[idx].value, "request", Qt::CaseInsensitive) )
        {
            //            testRequest();

        } else if ( ! QString::compare(parsedOptions[idx].value, "response", Qt::CaseInsensitive) )
        {
            //            testResponse();
        }

        return 0;
    }

    auto cmdOptions = QSharedPointer<CmdLineOptions>::create();

    if (ExtractorUtils::initCmdOptions(parsedOptions, cmdOptions.data()) < 0)
    {
        return -3;
    }

#ifdef QT_DEBUG
    if (cmdOptions->fileInfo.outAudioMediaUrl.isLocalFile())
    {
        LogUtils::safeLog(QString("EventExtractorApp::main(ing, char**) - Output file (%1): it is local file!")
                              .arg(cmdOptions->fileInfo.outAudioMediaUrl.path()),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
    } else {
        LogUtils::safeLog(QString("EventExtractorApp::main(ing, char**) - Output file (%1): it is not local file!")
                              .arg(cmdOptions->fileInfo.outAudioMediaUrl.path()),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
    }

    LogUtils::safeLog(QString("EventExtractor::main(ing, char**) - Transcode instantiation!"),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    // 4. Centralized Error Handling
    auto handleError = [&EventExtractorApp](const QString &msg) {
        qCritical() << "EventExtractorApp::main() -- Fatal Pipeline Error:" << msg;
        // Exit with error code 1
        EventExtractorApp.exit(1);
    };

    // 2. Create Controllers
    TranscodeController * transcodeController = new TranscodeController; // generator

    transcodeController->setupAudioTranscoding(cmdOptions->fileInfo.inVideoMediaUrl,
                                               cmdOptions->fileInfo.outAudioMediaUrl);

    QThread * transcodeControllerThread = new QThread(&EventExtractorApp);
    transcodeController->moveToThread(transcodeControllerThread);

    QObject::connect(transcodeControllerThread,
                     &QThread::finished,
                     transcodeController,
                     &QObject::deleteLater);

    QObject::connect(transcodeController,
                     &TranscodeController::error,
                     &EventExtractorApp,
                     handleError);

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractorApp::main(ing, char**) - Input file: %1!")
                          .arg(cmdOptions->fileInfo.inVideoMediaUrl.path()),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    // 2.4. Audio segmentation worker creation.
    AudioSegmentationController * audioSegController = new AudioSegmentationController;
    audioSegController->setClipSec(*Constants::Q_QGS_DEFAULT_CLIP_SEC); // 5 min clips, 0.0 is no slowdown
    // audioSegController->setClipSec(*Constants::Q_QGS_DEFAULT_CLIP_SEC/4.0); // 5 min clips, 4x slowdown
    // audioSegController->setClipSec(*Constants::Q_QGS_DEFAULT_CLIP_SEC/6.0); // 5 min clips, 6x slowdown
    // audioSegController->setClipSec(*Constants::Q_QGS_DEFAULT_CLIP_SEC/8.0); // 5 min clips, 8x slowdown
    audioSegController->setOutputDir(cmdOptions->dirInfo.audioClipsDir.absoluteFilePath());

    QThread * audioSegControllerThread = new QThread(&EventExtractorApp);
    audioSegController->moveToThread(audioSegControllerThread);

    QObject::connect(audioSegControllerThread,
                     &QThread::finished,
                     audioSegController,
                     &QObject::deleteLater);

    QObject::connect(audioSegController,
                     &AudioSegmentationController::error,
                     &EventExtractorApp,
                     handleError);

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractorApp::main(ing, char**) - Output MP3 audio file: %1!")
                          .arg(cmdOptions->fileInfo.outAudioMediaUrl.path()),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    // 2.5. Video segmentation worker creation.
    // The VideoProcessor constructor will internally initialize
    // VideoProcessorData and the VideoSegmentModel.
    // Process video and split it in shots of 5 min and no audio (audio is silence).
    VideoSegmentationController * videoSegController  = new VideoSegmentationController; // consumer
    videoSegController->setInputVideo(cmdOptions->fileInfo.inVideoMediaUrl.path());

    QString targetVideoClipsDir = QDir(cmdOptions->dirInfo.videoClipsDir.absoluteFilePath()).filePath("segments-Gemini25_Flash5fps21crf");
    videoSegController->setupVideoCutter(ModelCode_t::Gemini25_Flash5fps21crf,
                                         targetVideoClipsDir,
                                         *Constants::Q_QGS_DEFAULT_CLIP_SEC,
                                         0.0,                   // Slowdown Factor: 0.0 is no slowdown
                                         // 2.0,                   // Slowdown Factor: 2.0 is no slowdown
                                         // 4.0,                   // Slowdown Factor: 4.0 is no slowdown
                                         // 6.0,                   // Slowdown Factor: 6.0 is no slowdown
                                         // 8.0,                   // Slowdown Factor: 8.0 is no slowdown
                                         SegmentationMode_t::UseLibrary);

    QThread * videoSegControllerThread = new QThread(&EventExtractorApp);
    videoSegController->moveToThread(videoSegControllerThread);

    QObject::connect(videoSegControllerThread,
                     &QThread::finished,
                     videoSegController,
                     &QObject::deleteLater);

    QObject::connect(videoSegController,
                     &VideoSegmentationController::error,
                     &EventExtractorApp,
                     handleError);


    // Pipeline: Audio extraction; Audio segmentation; Video segmentation
    QObject::connect(transcodeController,
                     &TranscodeController::finished,
                     audioSegController,
                     &AudioSegmentationController::handleInput);

    QObject::connect(audioSegController,
                     &AudioSegmentationController::finished,
                     &EventExtractorApp,
                     [](){
                         qDebug() << "MAIN THREAD: AudioSegmentationController reported FINISHED. Triggering Video...";
                     });


    QObject::connect(transcodeController,
                     &TranscodeController::finished,
                     videoSegController,
                     [cmdOptions, videoSegController]() {
                         FileInfo videoFileInfo(cmdOptions->fileInfo.inVideoMediaUrl.path(), FileType_t::VideoFile);
                         videoSegController->handleInput(videoFileInfo);
                     });

    QObject::connect(videoSegController,
                     &VideoSegmentationController::finished,
                     &EventExtractorApp,
                     [](){
                         qDebug() << "MAIN THREAD: VideoSegmentationController reported FINISHED. Triggering ManifestSynchronizer...";
                     });


    // 3. Connect the finished signal so the app quits cleanly when done
    QObject::connect(videoSegController,
                     &EventExtractionController::finished,
                     &EventExtractorApp,
                     [transcodeControllerThread,
                      audioSegControllerThread,
                      videoSegControllerThread]() {
                         // Tell threads to gracefully exit their event loops
                         transcodeControllerThread->quit();
                         transcodeControllerThread->wait();
                         transcodeControllerThread->deleteLater();

                         audioSegControllerThread->quit();
                         audioSegControllerThread->wait();
                         audioSegControllerThread->deleteLater();

                         videoSegControllerThread->quit();
                         videoSegControllerThread->wait();
                         videoSegControllerThread->deleteLater();

                         // Now quit the main application
                         QApplication::quit();
                     });

    transcodeControllerThread->start();
    audioSegControllerThread->start();
    videoSegControllerThread->start();

    QTimer::singleShot(0, transcodeController, &TranscodeController::startTranscoding);

    qint32 result = EventExtractorApp.exec();

    qDebug() << "Clean exit.";

    return result;
}

/////////////////////////////////////////////////////////////////////////////////////

////////////// The Pipeline for transcribing and converting audio file //////////////

int mainTranscriberAndConverter(int argc, char *argv[])
// int main(int argc, char *argv[])
{
    QApplication EventExtractorApp(argc, argv);

    // FORCE Qt to keep the event loop running even with no windows open
    EventExtractorApp.setQuitOnLastWindowClosed(false);

    qRegisterMetaType<utils::UniqueId>("UniqueId");
    qRegisterMetaType<FileInfo>("FileInfo");
    qRegisterMetaType<ClipData>("ClipData");
    qRegisterMetaType<SegmentsWithPrompts>("SegmentsWithPrompts");

#ifdef QT_DEBUG
    // The LogUtils must be run before the application logic start.
    LogUtils::enableLog();
#endif

    if (argc <= 0 || argv == nullptr)
    {
        qCritical() << "Invalid command line argumtens.";
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractorApp::main(int, char**) - Invalid command line argumtens."),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        return -1;
    }

    QTranslator translator;

    const QStringList uiLanguages = QLocale::system().uiLanguages();

    for (const QString &locale : uiLanguages)
    {
        const QString baseName = "EventExtractorApp_" + QLocale(locale).name();

        if (translator.load(":/i18n/" + baseName))
        {
            EventExtractorApp.installTranslator(&translator);

            break;
        }
    }

    // 1. Argument Parsing
    // Note: Qt arguments include the program name at index 0, matching sys.argv
    QStringList args = EventExtractorApp.arguments();

    // The app arguments list must have at least seven items: the command, the input flag
    // ("-i" or "--input"), the input file (a video), the output audio format flag
    // ("-of" or "--audio_format"), the output audio format ("mp3", "wav", etc.),
    // target dir flag ("-td" of "--target_dir"), and the target dir.
    if (args.length() < 5)
    {
        qInfo().noquote() << "Please provide the <input file path>, <output file path> and target audio file format as command line input" << Qt::endl;

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractorApp::main(int, char**) - Usage: %1 -i <input video file> -af <output audio format> -td <destination dir>!")
                              .arg(args.at(0)),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        return -2; // sys.exit() equivalent
    }

    CmdArgumentsVec parsedOptions = ExtractorUtils::commandLineParser(args);

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractorApp::main(int, char**) - Arguments parsed: %1!")
                          .arg(args.at(0)),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    qint32 idx = -1;

    if ( (idx = ExtractorUtils::indexOfCmdArg(CmdFlags_t::Test, parsedOptions) ) >= 0 )
    {
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractorApp::main(int, char**) - Testing!"),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractorApp::main(int, char**) - Testing %1!")
                              .arg(parsedOptions.at(idx).value),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        if ( ! QString::compare(parsedOptions[idx].value, "preamble", Qt::CaseInsensitive) )
        {
            //            testPreamble();

        } else if ( ! QString::compare(parsedOptions[idx].value, "message", Qt::CaseInsensitive) )
        {
            //            testMessage();

        } else if ( ! QString::compare(parsedOptions[idx].value, "request", Qt::CaseInsensitive) )
        {
            //            testRequest();

        } else if ( ! QString::compare(parsedOptions[idx].value, "response", Qt::CaseInsensitive) )
        {
            //            testResponse();
        }

        return 0;
    }

    auto cmdOptions = QSharedPointer<CmdLineOptions>::create();

    if (ExtractorUtils::initCmdOptions(parsedOptions, cmdOptions.data()) < 0)
    {
        return -3;
    }

#ifdef QT_DEBUG
    if (cmdOptions->fileInfo.outAudioMediaUrl.isLocalFile())
    {
        LogUtils::safeLog(QString("EventExtractorApp::main(ing, char**) - Output file (%1): it is local file!")
                              .arg(cmdOptions->fileInfo.outAudioMediaUrl.path()),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
    } else {
        LogUtils::safeLog(QString("EventExtractorApp::main(ing, char**) - Output file (%1): it is not local file!")
                              .arg(cmdOptions->fileInfo.outAudioMediaUrl.path()),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
    }

    LogUtils::safeLog(QString("EventExtractor::main(ing, char**) - Transcode instantiation!"),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    // 4. Centralized Error Handling
    auto handleError = [&EventExtractorApp](const QString &msg) {
        qCritical() << "EventExtractorApp::main() -- Fatal Pipeline Error:" << msg;
        // Exit with error code 1
        EventExtractorApp.exit(1);
    };

    AudioTranscriptionController * transcribeAudioController = new AudioTranscriptionController;
    transcribeAudioController->setModel(Deepgram_AIS2TT2SModels_t::DeepgramS2TNova2GeneralPTBR);
    transcribeAudioController->setAnnotationDir(cmdOptions->dirInfo.annotation_and_transcriptsDir.absoluteFilePath());
    transcribeAudioController->setAnnotationClipsOutputDir(cmdOptions->dirInfo.annotationClipsDir.absoluteFilePath());
    transcribeAudioController->setClipSec(*Constants::Q_QGS_DEFAULT_CLIP_SEC); // 5 min / 0.0x is no slowdown factor
    // transcribeAudioController->setClipSec(*Constants::Q_QGS_DEFAULT_CLIP_SEC/2.0); // 5 min / 2.0x slowdown factor
    // transcribeAudioController->setClipSec(*Constants::Q_QGS_DEFAULT_CLIP_SEC/4.0); // 5 min / 4.0x slowdown factor
    // transcribeAudioController->setClipSec(*Constants::Q_QGS_DEFAULT_CLIP_SEC/6.0); // 5 min / 6.0x slowdown factor
    // transcribeAudioController->setClipSec(*Constants::Q_QGS_DEFAULT_CLIP_SEC/8.0); // 5 min / 8.0x slowdown factor

    QThread * transcribeAudioControllerThread = new QThread(&EventExtractorApp);
    transcribeAudioController->moveToThread(transcribeAudioControllerThread);

    QObject::connect(transcribeAudioControllerThread,
                     &QThread::finished,
                     transcribeAudioController,
                     &QObject::deleteLater);

    QObject::connect(transcribeAudioController,
                     &AudioTranscriptionController::error,
                     &EventExtractorApp,
                     handleError);

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractorApp::main(ing, char**) - Transcription directory: %1 --- %2!")
                          .arg(cmdOptions->dirInfo.annotation_and_transcriptsDir.absoluteDir().path(),
                               cmdOptions->dirInfo.annotation_and_transcriptsDir.fileName()),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    // 2.3. Transcription conversion worker creation.
    ConverterController * convertTranscriptionController = new ConverterController; // convert transcription to SRT/WebVTT

    convertTranscriptionController->setLineLength(200);
    convertTranscriptionController->setConverterType(ConverterType_t::DeepgramWebVttConverter);
    convertTranscriptionController->setClipSec(*Constants::Q_QGS_DEFAULT_CLIP_SEC/6.0); // 5 min / 6.0x slowdown factor
    convertTranscriptionController->setTranscriptionClipsOutputDir(cmdOptions->dirInfo.transcriptClipsDir.absoluteFilePath());

    QThread * convertTranscriptionControllerThread = new QThread(&EventExtractorApp);
    convertTranscriptionController->moveToThread(convertTranscriptionControllerThread);

    QObject::connect(convertTranscriptionControllerThread,
                     &QThread::finished,
                     convertTranscriptionController,
                     &QObject::deleteLater);

    QObject::connect(convertTranscriptionController,
                     &ConverterController::conversionError,
                     &EventExtractorApp,
                     handleError);



    // Pipeline
    QObject::connect(transcribeAudioController,
                     &AudioTranscriptionController::finished,
                     convertTranscriptionController,
                     &ConverterController::handleInput);

    QObject::connect(transcribeAudioController,
                     &AudioTranscriptionController::finished,
                     &EventExtractorApp,
                     [](){
                         qDebug() << "MAIN THREAD: AudioTranscriptionController reported FINISHED. Triggering ConverterController...";
                     });

    QObject::connect(convertTranscriptionController,
                     &ConverterController::finished,
                     &EventExtractorApp,
                     [](FileInfo transcriptionVTT){
                         qDebug() << "MAIN THREAD: ConverterController reported FINISHED." << transcriptionVTT.url.path();
                     });


    // Debugging the hang: Print when conversion specifically finishes

    // 3. Connect the finished signal so the app quits cleanly when done
    QObject::connect(convertTranscriptionController,
                     &EventExtractionController::finished,
                     &EventExtractorApp,
                     [transcribeAudioControllerThread,
                      convertTranscriptionControllerThread]() {
                         // Tell threads to gracefully exit their event loops
                         transcribeAudioControllerThread->quit();
                         transcribeAudioControllerThread->wait();
                         transcribeAudioControllerThread->deleteLater();

                         convertTranscriptionControllerThread->quit();
                         convertTranscriptionControllerThread->wait();
                         convertTranscriptionControllerThread->deleteLater();

                         // Now quit the main application
                         QApplication::quit();
                     });

    transcribeAudioControllerThread->start();
    convertTranscriptionControllerThread->start();

    FileInfo audioFile(Url("/Users/rrios/Downloads/EventExtractionDB/Pipeline-Test/224a7870-01dd-4211-8a4c-fe5b8a38331f/audio/224a7870-01dd-4211-8a4c-fe5b8a38331f.mp3"), FileType_t::AudioFile);

    transcribeAudioController->handleInput(audioFile);

    qint32 result = EventExtractorApp.exec();

    qDebug() << "Clean exit.";

    return result;
}

/////////////////////////////////////////////////////////////////////////////////////

/////////// The Pipeline for synchronazing manifests and event extraction ///////////

int mainManifestsSynchronizationAndEvtExtraction(int argc, char *argv[])
// int main(int argc, char *argv[])
{
    QApplication EventExtractorApp(argc, argv);

    // FORCE Qt to keep the event loop running even with no windows open
    EventExtractorApp.setQuitOnLastWindowClosed(false);

    qRegisterMetaType<utils::UniqueId>("UniqueId");
    qRegisterMetaType<FileInfo>("FileInfo");
    qRegisterMetaType<ClipData>("ClipData");
    qRegisterMetaType<SegmentsWithPrompts>("SegmentsWithPrompts");

#ifdef QT_DEBUG
    // The LogUtils must be run before the application logic start.
    LogUtils::enableLog();
#endif

    if (argc <= 0 || argv == nullptr)
    {
        qCritical() << "Invalid command line argumtens.";
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractorApp::main(int, char**) - Invalid command line argumtens."),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        return -1;
    }

    QTranslator translator;

    const QStringList uiLanguages = QLocale::system().uiLanguages();

    for (const QString &locale : uiLanguages)
    {
        const QString baseName = "EventExtractorApp_" + QLocale(locale).name();

        if (translator.load(":/i18n/" + baseName))
        {
            EventExtractorApp.installTranslator(&translator);

            break;
        }
    }

    // 1. Argument Parsing
    // Note: Qt arguments include the program name at index 0, matching sys.argv
    QStringList args = EventExtractorApp.arguments();

    // The app arguments list must have at least seven items: the command, the input flag
    // ("-i" or "--input"), the input file (a video), the output audio format flag
    // ("-of" or "--audio_format"), the output audio format ("mp3", "wav", etc.),
    // target dir flag ("-td" of "--target_dir"), and the target dir.
    if (args.length() < 5)
    {
        qInfo().noquote() << "Please provide the <input file path>, <output file path> and target audio file format as command line input" << Qt::endl;

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractorApp::main(int, char**) - Usage: %1 -i <input video file> -af <output audio format> -td <destination dir>!")
                              .arg(args.at(0)),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        return -2; // sys.exit() equivalent
    }

    CmdArgumentsVec parsedOptions = ExtractorUtils::commandLineParser(args);

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractorApp::main(int, char**) - Arguments parsed: %1!")
                          .arg(args.at(0)),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    qint32 idx = -1;

    if ( (idx = ExtractorUtils::indexOfCmdArg(CmdFlags_t::Test, parsedOptions) ) >= 0 )
    {
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractorApp::main(int, char**) - Testing!"),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractorApp::main(int, char**) - Testing %1!")
                              .arg(parsedOptions.at(idx).value),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        if ( ! QString::compare(parsedOptions[idx].value, "preamble", Qt::CaseInsensitive) )
        {
            //            testPreamble();

        } else if ( ! QString::compare(parsedOptions[idx].value, "message", Qt::CaseInsensitive) )
        {
            //            testMessage();

        } else if ( ! QString::compare(parsedOptions[idx].value, "request", Qt::CaseInsensitive) )
        {
            //            testRequest();

        } else if ( ! QString::compare(parsedOptions[idx].value, "response", Qt::CaseInsensitive) )
        {
            //            testResponse();
        }

        return 0;
    }

    auto cmdOptions = QSharedPointer<CmdLineOptions>::create();

    if (ExtractorUtils::initCmdOptions(parsedOptions, cmdOptions.data()) < 0)
    {
        return -3;
    }

#ifdef QT_DEBUG
    if (cmdOptions->fileInfo.outAudioMediaUrl.isLocalFile())
    {
        LogUtils::safeLog(QString("EventExtractorApp::main(ing, char**) - Output file (%1): it is local file!")
                              .arg(cmdOptions->fileInfo.outAudioMediaUrl.path()),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
    } else {
        LogUtils::safeLog(QString("EventExtractorApp::main(ing, char**) - Output file (%1): it is not local file!")
                              .arg(cmdOptions->fileInfo.outAudioMediaUrl.path()),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
    }

    LogUtils::safeLog(QString("EventExtractor::main(ing, char**) - Transcode instantiation!"),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    // 4. Centralized Error Handling
    auto handleError = [&EventExtractorApp](const QString &msg) {
        qCritical() << "EventExtractorApp::main() -- Fatal Pipeline Error:" << msg;
        // Exit with error code 1
        EventExtractorApp.exit(1);
    };


    ManifestSynchronizer * manifestsSynchronizer = new ManifestSynchronizer;
    manifestsSynchronizer->setOutputPath(QDir(cmdOptions->dirInfo.defaultRootDir.absoluteFilePath()).filePath("synchronized_manifests.json"));

    QThread * manifestsSynchronizerThread = new QThread(&EventExtractorApp);
    manifestsSynchronizer->moveToThread(manifestsSynchronizerThread);

    QObject::connect(manifestsSynchronizerThread,
                     &QThread::finished,
                     manifestsSynchronizer,
                     &QObject::deleteLater);

    QObject::connect(manifestsSynchronizer,
                     &ManifestSynchronizer::error,
                     &EventExtractorApp,
                     handleError);

    QObject::connect(manifestsSynchronizer,
                     &ManifestSynchronizer::collectionComplete,
                     manifestsSynchronizer,
                     &ManifestSynchronizer::synchronizeManifests,
                     Qt::QueuedConnection);

    QObject::connect(manifestsSynchronizer,
                     &ManifestSynchronizer::finished,
                     [manifestsSynchronizer](FileInfo fInfo) {

                         if (fInfo.success)
                         {
                             qDebug() << "✅ Synchronization completed successfully!";
                             qDebug() << "  Synchronized segments:" << manifestsSynchronizer->getSynchronizedSegmentCount();
                             qDebug() << "  Output file:" << fInfo.url.path();
                         } else {
                             qDebug() << "❌ Synchronization failed!";
                             qDebug() << "  Error:" << manifestsSynchronizer->lastError();
                             qCritical() << "Sync failed!";

                             // Optional: Stop the event loop once finished
                             QApplication::quit();
                         }
                     });



    // 2.6 Create the controller on the HEAP so it survives the scope of main()
    // QSharedPointer<EventExtractionController> evtExtractionController = new EventExtractionController(evtExtractorApp);
    EventExtractionController * evtExtractionController = new EventExtractionController;

    // Set up the controller (Replace with your actual testModelSubmission logic)
    evtExtractionController->setupModel(AIModels_t::Google_Gemini_API_25Flash);

    evtExtractionController->setOutputFile(cmdOptions->dirInfo.defaultRootDir.absoluteFilePath()+"/Google_Gemini_API_25Flash-20260626-00h35min.md");

    // QStringList searchTerms = {"goal", "foul", "corner kick", "penalty"};
    QStringList searchTerms = {"gol", "falta", "escanteio", "penalti", "cartao vermelho", "cartao amarelo"};

    evtExtractionController->addTermsForSearching(searchTerms);
    evtExtractionController->setSearchType(KeywordMatchType_t::Any);
    evtExtractionController->setMinConfidenceThreshold(0.80);

    evtExtractionController->setTaskTemplateType(TaskTemplate::Video_Analysis_Task);
    QString sysRefereePrompt = PromptManager::instance().getSystemProfile(Persona::FIFA_Referee);
    evtExtractionController->setSystemPrompt(sysRefereePrompt);

    QThread * evtExtractionControllerThread = new QThread(&EventExtractorApp);
    evtExtractionController->moveToThread(evtExtractionControllerThread);

    QObject::connect(evtExtractionControllerThread,
                     &QThread::finished,
                     evtExtractionController,
                     &QObject::deleteLater);

    QObject::connect(evtExtractionController,
                     &EventExtractionController::error,
                     &EventExtractorApp,
                     handleError);

    qDebug() << "main() ------- System profile: " << sysRefereePrompt;

    FileInfo audioSegmentationManifest(Url("/Users/rrios/Downloads/EventExtractionDB/Pipeline-Test/224a7870-01dd-4211-8a4c-fe5b8a38331f/audio/audioClips/audio_segmentation_manifest.json"), FileType_t::AudioManifest);

    FileInfo audioAnnotationManifest(Url("/Users/rrios/Downloads/EventExtractionDB/Pipeline-Test/224a7870-01dd-4211-8a4c-fe5b8a38331f/annotation_and_transcripts/annotationClips/deepgram_segmentation_manifest.json"), FileType_t::AnnotationManifest);

    FileInfo convertedTranscriptionManifest(Url("/Users/rrios/Downloads/EventExtractionDB/Pipeline-Test/224a7870-01dd-4211-8a4c-fe5b8a38331f/annotation_and_transcripts/transcriptClips/vtt_segmentation_manifest.json"), FileType_t::VTTManifest);

    FileInfo videoSegmentationManifest(Url("/Users/rrios/Downloads/EventExtractionDB/Pipeline-Test/224a7870-01dd-4211-8a4c-fe5b8a38331f/video/videoClips/segments-Gemini25_Flash5fps-5minCLip-6.0Slowdown/video_segmentation_manifest.json"), FileType_t::VideoManifest);


    QObject:: connect(manifestsSynchronizer,
                     &ManifestSynchronizer::finished,
                     evtExtractionController,
                     &EventExtractionController::handleInput);

    QObject::connect(manifestsSynchronizer,
                     &ManifestSynchronizer::finished,
                     &EventExtractorApp,
                     [](){
                         qDebug() << "MAIN THREAD: ManifestSynchronizer reported FINISHED. Triggering ManifestSynchronizer...";
                     });

    // 3. Connect the finished signal so the app quits cleanly when done
    QObject::connect(evtExtractionController,
                     &EventExtractionController::finished,
                     &EventExtractorApp,
                     [manifestsSynchronizerThread,
                      evtExtractionControllerThread]() {
                         // Tell threads to gracefully exit their event loops
                         manifestsSynchronizerThread->quit();
                         manifestsSynchronizerThread->wait();
                         manifestsSynchronizerThread->deleteLater();

                         evtExtractionControllerThread->quit();
                         evtExtractionControllerThread->wait();
                         evtExtractionControllerThread->deleteLater();

                         // Now quit the main application
                         QApplication::quit();
                     });

    manifestsSynchronizerThread->start();
    evtExtractionControllerThread->start();

    manifestsSynchronizer->handleInput(audioSegmentationManifest);
    manifestsSynchronizer->handleInput(audioAnnotationManifest);
    manifestsSynchronizer->handleInput(convertedTranscriptionManifest);
    manifestsSynchronizer->handleInput(videoSegmentationManifest);

    qint32 result = EventExtractorApp.exec();

    qDebug() << "Clean exit.";

    return result;
}

/////////////////////////////////////////////////////////////////////////////////////


int mainParseGeminiAnswer(int argc, char *argv[])
// int main(int argc, char *argv[])
{
    QApplication mainParseGeminiAnswer(argc, argv);

    if (argc <= 0 || argv == nullptr)
    {
        qCritical() << "Invalid command line argumtens.";

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("mainParseGeminiAnswer(int, char**) - Invalid command line argumtens."),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        return -1;
    }

    QStringList args = mainParseGeminiAnswer.arguments();

    if (args.length() < 2)
    {
        qInfo().noquote() << "Please provide the <input MD Gemini answer file path> and <output JSON file path> as command line input" << Qt::endl;

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("mainParseGeminiAnswer(int, char**) - Usage: %1 <input MD Gemini answer file path> <output JSON file path>!")
                              .arg(args.at(0)),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        return -2;
    }

    QString mdGeminiAnswerFile = QString::fromUtf8(argv[1]);
    qDebug() << "MD Gemini file path: " << mdGeminiAnswerFile;

    QString outputJsonFile = QString::fromUtf8(argv[2]);
    qDebug() << "Output JSON file path: " << outputJsonFile;

    // Diagnostic Block: Generate a batch of test files if directory is missing
    QFileInfo mdFilePath(mdGeminiAnswerFile);

    if (!mdFilePath.exists())
    {
        qDebug() << "No MD file exists!";

        return -3;
    }

    // Run batch parsing sequence and generate the persisted report file
    ExtractorUtils::parseCompleteGeminiMarkdownAnswer(mdGeminiAnswerFile, outputJsonFile);

    return 0;
}

// int main(int argc, char *argv[])
// {
//     QApplication EventExtractorApp(argc, argv);

//     // FORCE Qt to keep the event loop running even with no windows open
//     EventExtractorApp.setQuitOnLastWindowClosed(false);

//     qRegisterMetaType<utils::UniqueId>("UniqueId");
//     qRegisterMetaType<FileInfo>("FileInfo");
//     qRegisterMetaType<ClipData>("ClipData");
//     qRegisterMetaType<SegmentsWithPrompts>("SegmentsWithPrompts");
//     // qRegisterMetaType<Audio>("Actor");
//     // qRegisterMetaType<Audio>("Audio");
//     // qRegisterMetaType<Audio::AudioList_t>("Audio::AudioList_t");
//     // qRegisterMetaType<VideoSegmentInfo>("VideoSegmentInfo");
//     // qRegisterMetaType<VideoSegmentInfo::VideoSegmentInfoList_t>("VideoSegmentInfo::VideoSegmentInfoList_t");

// #ifdef QT_DEBUG
//     // The LogUtils must be run before the application logic start.
//     LogUtils::enableLog();
// #endif

//     if (argc <= 0 || argv == nullptr)
//     {
//         qCritical() << "Invalid command line argumtens.";
// #ifdef QT_DEBUG
//         LogUtils::safeLog(QString("EventExtractorApp::main(int, char**) - Invalid command line argumtens."),
//                           __FILE__,
//                           __LINE__,
//                           Q_FUNC_INFO);
// #endif
//         return -1;
//     }

//     QTranslator translator;

//     const QStringList uiLanguages = QLocale::system().uiLanguages();

//     for (const QString &locale : uiLanguages)
//     {
//         const QString baseName = "EventExtractorApp_" + QLocale(locale).name();

//         if (translator.load(":/i18n/" + baseName))
//         {
//             EventExtractorApp.installTranslator(&translator);

//             break;
//         }
//     }

//     // 1. Argument Parsing
//     // Note: Qt arguments include the program name at index 0, matching sys.argv
//     QStringList args = EventExtractorApp.arguments();
//     CmdArgumentsVec parsedOptions = Utils::commandLineParser(args);

// #ifdef QT_DEBUG
//     LogUtils::safeLog(QString("EventExtractorApp::main(int, char**) - Arguments parsed: %1!")
//                           .arg(args.at(0)),
//                       __FILE__,
//                       __LINE__,
//                       Q_FUNC_INFO);
// #endif

//     auto cmdOptions = QSharedPointer<CmdLineOptions>::create();

//     if (Utils::initCmdOptions(parsedOptions, cmdOptions.data()) < 0)
//     {
//         qCritical() << "Initialization Error: Failed to initialize pipeline configuration options.";
//         return -2;
//     }

//     if (cmdOptions->fileInfo.inVideoMediaUrl.isEmpty() || cmdOptions->dirInfo.defaultRootDir.absoluteFilePath().isEmpty())
//     {
//         qCritical().noquote() << "Usage Error: Missing mandatory fields.\n"
//                               << "Required: -i <input_video> -td <target_dir> -af <audio_format>";
//         return -3;
//     }

// // #ifdef QT_DEBUG
// //     if (cmdOptions->fileInfo.outAudioMediaUrl.isLocalFile())
// //     {
// //         LogUtils::safeLog(QString("EventExtractorApp::main(ing, char**) - Output file (%1): it is local file!")
// //                               .arg(cmdOptions->fileInfo.outAudioMediaUrl.path()),
// //                               __FILE__,
// //                               __LINE__,
// //                               Q_FUNC_INFO);
// //     } else {
// //         LogUtils::safeLog(QString("EventExtractorApp::main(ing, char**) - Output file (%1): it is not local file!")
// //                               .arg(cmdOptions->fileInfo.outAudioMediaUrl.path()),
// //                               __FILE__,
// //                               __LINE__,
// //                               Q_FUNC_INFO);
// //     }

// //     LogUtils::safeLog(QString("EventExtractor::main(ing, char**) - Transcode instantiation!"),
// //                       __FILE__,
// //                       __LINE__,
// //                       Q_FUNC_INFO);
// // #endif

//     qint32 testIdx = Utils::indexOfCmdArg(CmdFlags_t::Test, parsedOptions);

//     if ( testIdx >= 0 )
//     {
// #ifdef QT_DEBUG
//         LogUtils::safeLog(QString("EventExtractorApp::main(int, char**) - Testing!"),
//                           __FILE__,
//                           __LINE__,
//                           Q_FUNC_INFO);
// #endif

// #ifdef QT_DEBUG
//         LogUtils::safeLog(QString("EventExtractorApp::main(int, char**) - Testing %1!")
//                               .arg(parsedOptions.at(testIdx).value),
//                           __FILE__,
//                           __LINE__,
//                           Q_FUNC_INFO);
// #endif
//         QString testValue = parsedOptions.at(testIdx).value;
//         if ( ! QString::compare(testValue, "preamble", Qt::CaseInsensitive) ){ /*testPreamble();*/ }
//         else if ( ! QString::compare(testValue, "message", Qt::CaseInsensitive) ) { /*testMessage();*/ }
//         else if ( ! QString::compare(testValue, "request", Qt::CaseInsensitive) ) { /*testRequest();*/ }
//         else if ( ! QString::compare(testValue, "response", Qt::CaseInsensitive) ) { /*testResponse();*/ }
//         return 0;
//     }

//     // 4. Centralized Error Handling
//     auto handleError = [&EventExtractorApp](const QString &msg) {

//         qCritical() << "EventExtractorApp::main() -- Fatal Pipeline Error:" << msg;

//         // Exit with error code 1
//         EventExtractorApp.exit(1);
//     };

//     // // Centralized Error and Lifecycle Pipeline Closures
//     // qint32 activePipelinesCount = cmdOptions->slowdownFactors.size();

//     // // Track active jobs to prevent early exit conditions
//     // auto checkPipelineCompletion = [&activePipelinesCount]() {

//     //     activePipelinesCount--;

//     //     if (activePipelinesCount <= 0)
//     //     {
//     //         qDebug() << "All active execution tasks completed successfully. Shutting down application loop safely.";
//     //         QApplication::quit();
//     //     }
//     // };

//     // --- Sequential Queue Orchestration Controller ---
//     int currentSlowdownIdx = 0;
//     FileInfo capturedAnnotationManifest; // Holds the Deepgram transcription manifest across rounds
//     std::function<void()> runNextPipeline;

//     // for (QVector<double>::const_iterator slowdownFactorCit = cmdOptions->slowdownFactors.constBegin();
//     //      slowdownFactorCit != cmdOptions->slowdownFactors.constEnd();
//     //      slowdownFactorCit++)
//     // {
//     //     double currentFactor = *slowdownFactorCit;

//     // --- Connecting Thread Management and Memory Lifecycles ---
//     auto registerThreadCleanup = [](QThread *thread, QObject *worker) {
//         QObject::connect(thread, &QThread::finished, worker, &QObject::deleteLater);
//         QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);
//     };

//     runNextPipeline = [&]() {
//         // Stop Condition: Check if all requested factors have completed
//         if (currentSlowdownIdx >= cmdOptions->slowdownFactors.size())
//         {
//             qDebug() << "===========================================================================";
//             qDebug() << "✅ All sequential processing rounds completed successfully. Exiting clean.";
//             qDebug() << "===========================================================================";

//             QApplication::quit();

//             return;
//         }

//         double currentFactor = cmdOptions->slowdownFactors.at(currentSlowdownIdx);

//         qDebug() << "======================================================================";
//         qDebug() << "🚀 Launching Pipeline Round:" << (currentSlowdownIdx + 1)
//                  << "of" << cmdOptions->slowdownFactors.size()
//                  << "| Slowdown Factor Context Target:" << currentFactor << "x";
//         qDebug() << "======================================================================";

//         if (qFuzzyIsNull(cmdOptions->clipDuration))
//         {
//             cmdOptions->clipDuration = *Constants::Q_QGS_DEFAULT_CLIP_SEC;
//         }

//         qreal transcribeTranscriptionAndAudioClip_sec = cmdOptions->clipDuration;

//         if (currentFactor > 0)
//         {
//             transcribeTranscriptionAndAudioClip_sec = cmdOptions->clipDuration/currentFactor;
//         }

//         // 2.3. Transcription conversion worker creation.
//         ConverterController * convertTranscriptionController = new ConverterController; // convert transcription to SRT/WebVTT

//         convertTranscriptionController->setLineLength(200);
//         convertTranscriptionController->setConverterType(ConverterType_t::DeepgramWebVttConverter);
//         convertTranscriptionController->setClipSec(transcribeTranscriptionAndAudioClip_sec); // X min / slowdown factor of Y.YY*
//         convertTranscriptionController->setTranscriptionClipsOutputDir(cmdOptions->dirInfo.transcriptClipsDir.absoluteFilePath());

//         QThread * convertTranscriptionControllerThread = new QThread(&EventExtractorApp);
//         convertTranscriptionController->moveToThread(convertTranscriptionControllerThread);

//         // 2.4. Audio segmentation worker creation.
//         AudioSegmentationController * audioSegController = new AudioSegmentationController;
//         audioSegController->setClipSec(transcribeTranscriptionAndAudioClip_sec); // X min / slowdown factor of Y.YY*
//         audioSegController->setOutputDir(cmdOptions->dirInfo.audioClipsDir.absoluteFilePath());

//         QThread * audioSegControllerThread = new QThread(&EventExtractorApp);
//         audioSegController->moveToThread(audioSegControllerThread);

// #ifdef QT_DEBUG
//         LogUtils::safeLog(QString("EventExtractorApp::main(ing, char**) - Output MP3 audio file: %1!")
//                               .arg(cmdOptions->fileInfo.outAudioMediaUrl.path()),
//                           __FILE__,
//                           __LINE__,
//                           Q_FUNC_INFO);
// #endif

//         // 2.5. Video segmentation worker creation.
//         // The VideoProcessor constructor will internally initialize
//         // VideoProcessorData and the VideoSegmentModel.
//         // Process video and split it in shots of 5 min and no audio (audio is silence).
//         VideoSegmentationController * videoSegController  = new VideoSegmentationController; // consumer
//         videoSegController->setInputVideo(cmdOptions->fileInfo.inVideoMediaUrl.path());

//         QString targetVideoClipsDir = QDir(cmdOptions->dirInfo.videoClipsDir.absoluteFilePath())
//                                           .filePath(QString("segments-Gemini25_Flash5fps-%1minCLip-%2Slowdown")
//                                                         .arg(QString::number(cmdOptions->clipDuration / 60.0, 'f', 1),
//                                                              QString::number(currentFactor, 'f', 1)));

//         videoSegController->setupVideoCutter(ModelCode_t::Gemini25_Flash5fps21crf,
//                                              targetVideoClipsDir,
//                                              cmdOptions->clipDuration,
//                                              currentFactor, // Slowdown Factor
//                                              SegmentationMode_t::UseLibrary);

//         QThread * videoSegControllerThread = new QThread(&EventExtractorApp);
//         videoSegController->moveToThread(videoSegControllerThread);

//         // 2.6 Create the controller on the HEAP so it survives the scope of main()
//         ManifestSynchronizer * manifestsSynchronizer = new ManifestSynchronizer;
//         QString manifestPath = QDir(cmdOptions->dirInfo.defaultRootDir.absoluteFilePath())
//                                    .filePath(QString("synchronized_manifests_%1x.json")
//                                                  .arg(QString::number(currentFactor, 'f', 1)));

//         manifestsSynchronizer->setOutputPath(manifestPath);

//         QThread * manifestsSynchronizerThread = new QThread(&EventExtractorApp);
//         manifestsSynchronizer->moveToThread(manifestsSynchronizerThread);

//         QObject::connect(manifestsSynchronizer,
//                          &ManifestSynchronizer::collectionComplete,
//                          manifestsSynchronizer,
//                          &ManifestSynchronizer::synchronizeManifests,
//                          Qt::QueuedConnection);

//         QObject::connect(manifestsSynchronizer,
//                          &ManifestSynchronizer::finished,
//                          [manifestsSynchronizer](FileInfo fInfo) {

//                              if (fInfo.success)
//                              {
//                                  qDebug() << "✅ Synchronization completed successfully!";
//                                  qDebug() << "  Synchronized segments:" << manifestsSynchronizer->getSynchronizedSegmentCount();
//                                  qDebug() << "  Output file:" << fInfo.url.path();
//                              } else {
//                                  qDebug() << "❌ Synchronization failed!";
//                                  qDebug() << "  Error:" << manifestsSynchronizer->lastError();
//                                  qCritical() << "Sync failed!";

//                                  // Optional: Stop the event loop once finished
//                                  QApplication::quit();
//                              }
//                          });

//         // 2.6 Create the controller on the HEAP so it survives the scope of main()
//         EventExtractionController * evtExtractionController = new EventExtractionController;

//         // Set up the controller (Replace with your actual testModelSubmission logic)
//         evtExtractionController->setupModel(AIModels_t::Google_Gemini_API_25Flash);

//         QString markdownReportPath = QDir(cmdOptions->dirInfo.defaultRootDir.absoluteFilePath())
//                                          .filePath(QString("Google_Gemini_API_25-%1x_Slowdown.md")
//                                                        .arg(QString::number(currentFactor, 'f', 1)));

//         evtExtractionController->setOutputFile(markdownReportPath);

//         // QStringList searchTerms = {"goal", "foul", "corner kick", "penalty"};
//         QStringList searchTerms = cmdOptions->searchedTerms; //{"gol", "falta", "escanteio", "penalti", "cartao vermelho", "cartao amarelo"};

//         evtExtractionController->addTermsForSearching(searchTerms);
//         evtExtractionController->setSearchType(KeywordSearchEngine::MatchType::Any);
//         evtExtractionController->setMinConfidenceThreshold(cmdOptions->confidenceThreshould);

//         evtExtractionController->setTaskTemplateType(PromptManager::TaskTemplate::Video_Analysis_Task);
//         QString sysRefereePrompt = PromptManager::instance().getSystemProfile(PromptManager::Persona::FIFA_Referee);
//         evtExtractionController->setSystemPrompt(sysRefereePrompt);

//         QThread * evtExtractionControllerThread = new QThread(&EventExtractorApp);
//         evtExtractionController->moveToThread(evtExtractionControllerThread);


//         registerThreadCleanup(convertTranscriptionControllerThread, convertTranscriptionController);
//         registerThreadCleanup(audioSegControllerThread, audioSegController);
//         registerThreadCleanup(videoSegControllerThread, videoSegController);
//         registerThreadCleanup(manifestsSynchronizerThread, manifestsSynchronizer);
//         registerThreadCleanup(evtExtractionControllerThread, evtExtractionController);


//         QObject::connect(convertTranscriptionController,
//                          &ConverterController::conversionError,
//                          &EventExtractorApp,
//                          handleError);

//         QObject::connect(audioSegController,
//                          &AudioSegmentationController::error,
//                          &EventExtractorApp,
//                          handleError);

//         QObject::connect(videoSegController,
//                          &VideoSegmentationController::error,
//                          &EventExtractorApp,
//                          handleError);

//         QObject::connect(manifestsSynchronizer,
//                          &ManifestSynchronizer::error,
//                          &EventExtractorApp,
//                          handleError);

//         QObject::connect(evtExtractionController,
//                          &EventExtractionController::error,
//                          &EventExtractorApp,
//                          handleError);


//         qDebug() << "main() ------- System profile: " << sysRefereePrompt;


//         // Pipeline
//         QObject::connect(transcribeAudioController,
//                          &AudioTranscriptionController::manifestReady,
//                          manifestsSynchronizer,
//                          &ManifestSynchronizer::handleInput);

//         QObject::connect(transcribeAudioController,
//                          &AudioTranscriptionController::manifestReady,
//                          &EventExtractorApp,
//                          [](const FileInfo &fInfo){
//                              qDebug() << "MAIN THREAD: Emitted AudioTranscriptionController Manifest Relay:" << fInfo.url.path();
//                          });

//         QObject::connect(convertTranscriptionController,
//                          &ConverterController::manifestReady,
//                          manifestsSynchronizer,
//                          &ManifestSynchronizer::handleInput);

//         QObject::connect(convertTranscriptionController,
//                          &ConverterController::manifestReady,
//                          &EventExtractorApp,
//                          [](const FileInfo &fInfo){
//                              qDebug() << "MAIN THREAD: Emitted ConverterController Manifest Relay:" << fInfo.url.path();
//                          });

//         QObject::connect(convertTranscriptionController,
//                          &ConverterController::finished,
//                          &EventExtractorApp,
//                          [](FileInfo transcriptionVTT){
//                              qDebug() << "MAIN THREAD: ConverterController reported FINISHED." << transcriptionVTT.url.path();
//                          });

//         QObject::connect(audioSegController,
//                          &AudioSegmentationController::manifestReady,
//                          manifestsSynchronizer,
//                          &ManifestSynchronizer::handleInput);

//         QObject::connect(audioSegController,
//                          &AudioSegmentationController::manifestReady,
//                          &EventExtractorApp,
//                          [](const FileInfo &fInfo){
//                              qDebug() << "MAIN THREAD: Emitted AudioSegmentationController Manifest Relay:" << fInfo.url.path();
//                          });

//         QObject::connect(videoSegController,
//                          &VideoSegmentationController::manifestReady,
//                          manifestsSynchronizer,
//                          &ManifestSynchronizer::handleInput);

//         QObject::connect(videoSegController,
//                          &VideoSegmentationController::manifestReady,
//                          &EventExtractorApp,
//                          [](const FileInfo &fInfo){
//                              qDebug() << "MAIN THREAD: Emitted VideoSegmentationController Manifest Relay:" << fInfo.url.path();
//                          });

//         QObject::connect(manifestsSynchronizer,
//                          &ManifestSynchronizer::finished,
//                          &EventExtractorApp,
//                          [](){
//                              qDebug() << "MAIN THREAD: ManifestSynchronizer reported FINISHED. Triggering ManifestSynchronizer...";
//                          });

//         QObject::connect(manifestsSynchronizer,
//                          &ManifestSynchronizer::finished,
//                          evtExtractionController,
//                          &EventExtractionController::handleInput);

//         // Debugging the hang: Print when conversion specifically finishes

//         // 3. Connect the finished signal so the app quits cleanly when done
//         QObject::connect(evtExtractionController,
//                          &EventExtractionController::finished,
//                          [=, &currentSlowdownIdx, &runNextPipeline]() {
//                              // Tell threads to gracefully exit their event loops

//                              convertTranscriptionControllerThread->quit();
//                              convertTranscriptionControllerThread->wait();
//                              // convertTranscriptionControllerThread->deleteLater();

//                              audioSegControllerThread->quit();
//                              audioSegControllerThread->wait();
//                              // audioSegControllerThread->deleteLater();

//                              videoSegControllerThread->quit();
//                              videoSegControllerThread->wait();
//                              // videoSegControllerThread->deleteLater();

//                              manifestsSynchronizerThread->quit();
//                              manifestsSynchronizerThread->wait();
//                              // manifestsSynchronizerThread->deleteLater();

//                              evtExtractionControllerThread->quit();
//                              evtExtractionControllerThread->wait();
//                              // evtExtractionControllerThread->deleteLater();

//                              qDebug() << "Round processing complete.";

//                              // Increment sequence and invoke state-loop context recursively
//                              currentSlowdownIdx++;
//                              runNextPipeline();
//                          });

//         convertTranscriptionControllerThread->start();
//         audioSegControllerThread->start();
//         videoSegControllerThread->start();
//         manifestsSynchronizerThread->start();
//         evtExtractionControllerThread->start();

//         // Kickstart the workers with their inputs directly (skipping obsolete Transcode dependencies)
//         FileInfo audioFileInfo(cmdOptions->fileInfo.outAudioMediaUrl.path(), FileType_t::AudioFile);
//         FileInfo videoFileInfo(cmdOptions->fileInfo.inVideoMediaUrl.path(), FileType_t::VideoFile);

//         QTimer::singleShot(0, convertTranscriptionController, [=]() { convertTranscriptionController->handleInput(capturedAnnotationManifest); });

//         QTimer::singleShot(0, audioSegController, [=]() { audioSegController->handleInput(audioFileInfo); });
//         QTimer::singleShot(0, videoSegController, [=]() { videoSegController->handleInput(videoFileInfo); });
//     };

//     // --- PHASE 1: Run Transcoding and Transcription Once Upfront ---
//     qDebug() << "======================================================================";
//     qDebug() << "🌀 Launching Phase 1: Global Audio Extraction & Text Transcription";
//     qDebug() << "======================================================================";


//     // 2. Create Controllers
//     TranscodeController * transcodeController = new TranscodeController; // generator
//     transcodeController->setupAudioTranscoding(cmdOptions->fileInfo.inVideoMediaUrl,
//                                                cmdOptions->fileInfo.outAudioMediaUrl);

//     QThread * transcodeControllerThread = new QThread(&EventExtractorApp);
//     transcodeController->moveToThread(transcodeControllerThread);

// #ifdef QT_DEBUG
//     LogUtils::safeLog(QString("EventExtractorApp::main(ing, char**) - Input file: %1!")
//                           .arg(cmdOptions->fileInfo.inVideoMediaUrl.path()),
//                       __FILE__,
//                       __LINE__,
//                       Q_FUNC_INFO);
// #endif
//     /////////////
//     // TODO In the part output transcription file will be generated according to slowdown factors number!!! It must be generated only once.
//     AudioTranscriptionController * transcribeAudioController = new AudioTranscriptionController;
//     transcribeAudioController->setModel(Deepgram_AIS2TT2SModels_t::DeepgramS2TNova2General);
//     transcribeAudioController->setAnnotationDir(cmdOptions->dirInfo.annotation_and_transcriptsDir.absoluteFilePath());
//     transcribeAudioController->setAnnotationClipsOutputDir(cmdOptions->dirInfo.annotationClipsDir.absoluteFilePath());

//     transcribeAudioController->setClipSec(transcribeTranscriptionAndAudioClip_sec); // X min / slowdown factor of Y.YY*

//     QThread * transcribeAudioControllerThread = new QThread(&EventExtractorApp);
//     transcribeAudioController->moveToThread(transcribeAudioControllerThread);

// #ifdef QT_DEBUG
//     LogUtils::safeLog(QString("EventExtractorApp::main(ing, char**) - Transcription directory: %1 --- %2!")
//                           .arg(cmdOptions->dirInfo.annotation_and_transcriptsDir.absoluteDir().path(),
//                                cmdOptions->dirInfo.annotation_and_transcriptsDir.fileName()),
//                       __FILE__,
//                       __LINE__,
//                       Q_FUNC_INFO);
// #endif


//     registerThreadCleanup(transcodeControllerThread, transcodeController);
//     registerThreadCleanup(transcodeControllerThread, transcodeControllerThread);
//     registerThreadCleanup(transcribeAudioControllerThread, transcribeAudioController);
//     registerThreadCleanup(transcribeAudioControllerThread, transcribeAudioControllerThread);


//     QObject::connect(transcodeController,
//                      &TranscodeController::error,
//                      &EventExtractorApp,
//                      handleError);

//     QObject::connect(transcribeAudioController,
//                      &AudioTranscriptionController::error,
//                      &EventExtractorApp,
//                      handleError);


//     QObject::connect(transcodeController,
//                      &TranscodeController::finished,
//                      transcribeAudioController,
//                      &AudioTranscriptionController::handleInput);

//     QObject::connect(transcodeController,
//                      &TranscodeController::finished,
//                      &EventExtractorApp,
//                      [](){
//                          qDebug() << "MAIN THREAD: TranscodeController reported FINISHED. Triggering AudioTranscriptionController...";
//                      });

//     // Once Phase 1 transcription is complete, cache the manifest and bridge into Phase 2
//     QObject::connect(transcribeAudioController,
//                      &AudioTranscriptionController::finished,
//                      [=, &capturedAnnotationManifest, &runNextPipeline](FileInfo transcriptionManifest) {

//                          qDebug() << "✨ Master audio transcription complete! Manifest saved to:" << transcriptionManifest.url.path();
//                          capturedAnnotationManifest = transcriptionManifest;

//                          // Safely tear down Phase 1 background threads
//                          transcodeControllerThread->quit();
//                          transcodeControllerThread->wait();

//                          transcribeAudioControllerThread->quit();
//                          transcribeAudioControllerThread->wait();

//                          // Hand off control to sequential slowdown execution phase
//                          runNextPipeline();
//                      });

//     QObject::connect(transcribeAudioController,
//                      &AudioTranscriptionController::finished,
//                      &EventExtractorApp,
//                      [](){
//                          qDebug() << "MAIN THREAD: AudioTranscriptionController reported FINISHED. Triggering ConverterController...";
//                      });




//     QObject::connect(transcodeController,
//                      &TranscodeController::finished,
//                      audioSegController,
//                      &AudioSegmentationController::handleInput);

//     QObject::connect(audioSegController,
//                      &AudioSegmentationController::finished,
//                      &EventExtractorApp,
//                      [](){
//                          qDebug() << "MAIN THREAD: AudioSegmentationController reported FINISHED. Triggering Video...";
//                      });

//     QObject::connect(transcodeController,
//                      &TranscodeController::finished,
//                      videoSegController,
//                      [cmdOptions, videoSegController]() {
//                          FileInfo videoFileInfo(cmdOptions->fileInfo.inVideoMediaUrl.path(), FileType_t::VideoFile);
//                          videoSegController->handleInput(videoFileInfo);
//                      });

//     QObject::connect(videoSegController,
//                      &VideoSegmentationController::finished,
//                      &EventExtractorApp,
//                      [](){
//                          qDebug() << "MAIN THREAD: VideoSegmentationController reported FINISHED. Triggering ManifestSynchronizer...";
//                      });



//     transcodeControllerThread->start();
//     transcribeAudioControllerThread->start();

//     QTimer::singleShot(0, transcodeController, &TranscodeController::startTranscoding);

//     qint32 result = EventExtractorApp.exec();

//     qDebug() << "System runtime lifecycle loop closed smoothly.";

//     return result;
// }


// Latest working main. This main defines the whole Pipeline: transcoding, transcribing and convertin run
// once and the rest of the pipeline runs as many times while there are slowdonw factors. There is a bug
// that makes this main running iteratively.
int main_20260730_14h17min(int argc, char *argv[])
{
    QApplication EventExtractorApp(argc, argv);

    // Force Qt to keep the event loop running even with no windows open
    EventExtractorApp.setQuitOnLastWindowClosed(false);

    // Register custom MetaTypes for safe cross-thread Signal/Slot routing
    qRegisterMetaType<utils::UniqueId>("UniqueId");
    qRegisterMetaType<FileInfo>("FileInfo");
    qRegisterMetaType<ClipData>("ClipData");
    qRegisterMetaType<SegmentsWithPrompts>("SegmentsWithPrompts");

#ifdef QT_DEBUG
    LogUtils::enableLog();
#endif

    if (argc <= 0 || argv == nullptr)
    {
        qCritical() << "Error: Invalid low-level runtime command line arguments context system.";
        return -1;
    }

    // Internationalization / Localization Setup
    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();

    for (const QString &locale : uiLanguages)
    {
        const QString baseName = "EventExtractorApp_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName))
        {
            EventExtractorApp.installTranslator(&translator);
            break;
        }
    }

    // 1. Argument Parsing Phase
    // Note: Qt arguments include the program name at index 0, matching sys.argv
    QStringList args = EventExtractorApp.arguments();
    CmdArgumentsVec parsedOptions = ExtractorUtils::commandLineParser(args);

    auto cmdOptions = QSharedPointer<CmdLineOptions>::create();

    if (ExtractorUtils::initCmdOptions(parsedOptions, cmdOptions.data()) < 0)
    {
        qCritical() << "Initialization Error: Failed to initialize pipeline configuration options.";
        return -2;
    }

    // Semantic Field Validation Guard
    if (cmdOptions->fileInfo.inVideoMediaUrl.isEmpty() || cmdOptions->dirInfo.defaultRootDir.absoluteFilePath().isEmpty())
    {
        qCritical().noquote() << "Usage Error: Missing mandatory fields.\n"
                              << "Required: -i <input_video> -td <target_dir> -af <audio_format>...";
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractorApp::main(ing, char**) - Missing mandatory files!"),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        return -3;
    }

#ifdef QT_DEBUG
    if (cmdOptions->fileInfo.outAudioMediaUrl.isLocalFile())
    {
        LogUtils::safeLog(QString("EventExtractorApp::main(ing, char**) - Output file (%1): it is local file!")
                              .arg(cmdOptions->fileInfo.outAudioMediaUrl.path()),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
    } else {
        LogUtils::safeLog(QString("EventExtractorApp::main(ing, char**) - Output file (%1): it is not local file!")
                              .arg(cmdOptions->fileInfo.outAudioMediaUrl.path()),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
    }

    LogUtils::safeLog(QString("EventExtractor::main(ing, char**) - Transcode instantiation!"),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    // Handle early testing switches gracefully
    qint32 testIdx = ExtractorUtils::indexOfCmdArg(CmdFlags_t::Test, parsedOptions);
    if (testIdx >= 0)
    {
        QString testValue = parsedOptions.at(testIdx).value;

        if ( ! QString::compare(parsedOptions[testIdx].value, "preamble", Qt::CaseInsensitive) ) { /*testPreamble()*/; }
        else if ( ! QString::compare(parsedOptions[testIdx].value, "message", Qt::CaseInsensitive) ) { /*testMessage();*/ }
        else if ( ! QString::compare(parsedOptions[testIdx].value, "request", Qt::CaseInsensitive) ) { /*testRequest();*/ }
        else if ( ! QString::compare(parsedOptions[testIdx].value, "response", Qt::CaseInsensitive) ) { /*testResponse();*/ }

        return 0;
    }

    // Centralized Pipeline Error Routing Callback
    auto handleError = [&EventExtractorApp](const QString &msg) {
        qCritical() << "Fatal Processing Pipeline Exception Encountered:" << msg;
        EventExtractorApp.exit();
    };

    // --- State variables for the two-phase pipeline execution ---
    int currentSlowdownIdx = 0;
    FileInfo capturedAnnotationResult; // Holds the audio annotation across rounds
    FileInfo capturedTranscriptionResult; // Holds the audio transcription across rounds
    std::function<void()> runNextPipeline;

    // // --- Memory Lifecycle Cleanup Registry ---
    // auto registerThreadCleanup = [](QThread *thread, QObject *worker) {
    //     QObject::connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    //     QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    // };

    // --- PHASE 2: Sequential Orchestration Loop ---
    runNextPipeline = [&]() {
        if (currentSlowdownIdx >= cmdOptions->slowdownFactors.size())
        {
            qDebug() << "==========================================================================";
            qDebug() << "✅ All sequential processing rounds completed successfully. Exiting clean.";
            qDebug() << "==========================================================================";
            QApplication::quit();
            return;
        }

        double currentFactor = cmdOptions->slowdownFactors.at(currentSlowdownIdx);

        qDebug() << "===============================================";
        qDebug() << " 🚀 Launching Speed-Dependent Pipeline Round: " << (currentSlowdownIdx + 1)
                 << " of " << cmdOptions->slowdownFactors.size()
                 << " | Slowdown Factor Context Target:" << currentFactor << "x";
        qDebug() << "===============================================";

        if (qFuzzyIsNull(cmdOptions->clipDuration))
        {
            cmdOptions->clipDuration = *Constants::Q_QGS_DEFAULT_CLIP_SEC;
        }

        qreal localClipDuration = cmdOptions->clipDuration;
        if (currentFactor > 0)
        {
            localClipDuration = cmdOptions->clipDuration / currentFactor;
        }

        // 1. Audio Annotation Segmentation Controller Worker (Runs per round due to variable clip durations)
        AudioAnnotationSegmentationController * audioAnnotationSegmentationController = new AudioAnnotationSegmentationController;
        audioAnnotationSegmentationController->setClipSec(localClipDuration);

        QString targetAnnotationClipsDir = QDir(cmdOptions->dirInfo.annotationClipsDir.absoluteFilePath())
                                               .filePath(QString("segments-%1_minClip-%2_Slowdown")
                                                             .arg(QString::number(cmdOptions->clipDuration / 60.0, 'f', 2),
                                                                  QString::number(currentFactor, 'f', 2)));

        audioAnnotationSegmentationController->setAnnotationClipsOutputDir(targetAnnotationClipsDir);
        // audioAnnotationSegmentationController->setAnnotationManifest();
        QThread * audioAnnotationSegmentationControllerThread = new QThread;
        audioAnnotationSegmentationController->moveToThread(audioAnnotationSegmentationControllerThread);

        // 2. Audio Transcription Segmentation Controller Worker (Runs per round due to variable clip durations)
        AudioTranscriptionSegmentationController * audioTranscriptionSegmentationController = new AudioTranscriptionSegmentationController;
        audioTranscriptionSegmentationController->setClipSec(localClipDuration);

        QString targetTranscriptionClipsDir = QDir(cmdOptions->dirInfo.transcriptClipsDir.absoluteFilePath())
                                                  .filePath(QString("segments-%1_minClip-%2_Slowdown")
                                                                .arg(QString::number(cmdOptions->clipDuration / 60.0, 'f', 2),
                                                                     QString::number(currentFactor, 'f', 2)));

        audioTranscriptionSegmentationController->setTranscriptionClipsOutputDir(targetTranscriptionClipsDir);
        audioTranscriptionSegmentationController->setLineLength(200);
        // audioTranscriptionSegmentationController->setTranscriptionManifest();
        QThread * audioTranscriptionSegmentationControllerThread = new QThread;
        audioTranscriptionSegmentationController->moveToThread(audioTranscriptionSegmentationControllerThread);

        // 3. Audio Segmentation Controller Worker
        AudioSegmentationController *audioSegmentationController = new AudioSegmentationController;
        audioSegmentationController->setClipSec(localClipDuration);

        QString targetAudioClipsDir = QDir(cmdOptions->dirInfo.audioClipsDir.absoluteFilePath())
                                          .filePath(QString("segments-%1_minClip-%2_Slowdown")
                                                        .arg(QString::number(cmdOptions->clipDuration / 60.0, 'f', 2),
                                                             QString::number(currentFactor, 'f', 2)));

        audioSegmentationController->setOutputDir(targetAudioClipsDir);
        QThread *audioSegmentationControllerThread = new QThread;
        audioSegmentationController->moveToThread(audioSegmentationControllerThread);

        // 4. Video Segmentation Controller Worker
        VideoSegmentationController *videoSegmentationController = new VideoSegmentationController;
        videoSegmentationController->setInputVideo(cmdOptions->fileInfo.inVideoMediaUrl.path());

        QString targetVideoClipsDir = QDir(cmdOptions->dirInfo.videoClipsDir.absoluteFilePath())
                                          .filePath(QString("segments-Gemini25_Flash5fps21crf-%1minClip-%2-Slowdown")
                                                        .arg(QString::number(cmdOptions->clipDuration / 60.0, 'f', 2),
                                                             QString::number(currentFactor, 'f', 2)));

        videoSegmentationController->setupVideoCutter(ModelCode_t::Gemini25_Flash5fps21crf,
                                                      targetVideoClipsDir,
                                                      cmdOptions->clipDuration,
                                                      currentFactor,
                                                      SegmentationMode_t::UseLibrary);
        QThread *videoSegmentationControllerThread = new QThread;
        videoSegmentationController->moveToThread(videoSegmentationControllerThread);

        // 5. Manifest Synchronizer Controller Worker
        ManifestSynchronizer *manifestsSynchronizer = new ManifestSynchronizer;
        QString manifestPath = QDir(cmdOptions->dirInfo.defaultRootDir.absoluteFilePath())
                                   .filePath(QString("synchronized_manifests_%1_minClip_%2xSlowdownFactor.json")
                                                 .arg(QString::number(cmdOptions->clipDuration / 60.0, 'f', 2),
                                                      QString::number(currentFactor, 'f', 2)));
        manifestsSynchronizer->setOutputPath(manifestPath);

        QThread *manifestsSynchronizerThread = new QThread;
        manifestsSynchronizer->moveToThread(manifestsSynchronizerThread);

        QObject::connect(manifestsSynchronizer,
                         &ManifestSynchronizer::collectionComplete,
                         manifestsSynchronizer,
                         &ManifestSynchronizer::synchronizeManifests,
                         Qt::QueuedConnection);

        // 6. Event Extraction Controller Worker
        EventExtractionController *evtExtractionController = new EventExtractionController;
        evtExtractionController->setupModel(AIModels_t::Google_Gemini_API_25Flash);

        QString markdownReportPath = QDir(cmdOptions->dirInfo.defaultRootDir.absoluteFilePath())
                                         .filePath(QString("Google_Gemini_API_25-%1x_Slowdown.md")
                                                       .arg(QString::number(currentFactor, 'f', 2)));
        evtExtractionController->setOutputFile(markdownReportPath);
        evtExtractionController->addTermsForSearching(cmdOptions->searchedTerms);
        evtExtractionController->setSearchType(KeywordMatchType_t::Any);
        evtExtractionController->setMinConfidenceThreshold(cmdOptions->confidenceThreshold);
        evtExtractionController->setTaskTemplateType(TaskTemplate::Video_Analysis_Task);

        QString sysRefereePrompt = PromptManager::instance().getSystemProfile(Persona::FIFA_Referee);
        evtExtractionController->setSystemPrompt(sysRefereePrompt);
        QThread *evtExtractionControllerThread = new QThread;
        evtExtractionController->moveToThread(evtExtractionControllerThread);


        // Cleanup
        registerThreadCleanup(audioAnnotationSegmentationControllerThread, audioAnnotationSegmentationController);
        registerThreadCleanup(audioTranscriptionSegmentationControllerThread, audioTranscriptionSegmentationController);
        registerThreadCleanup(audioSegmentationControllerThread, audioSegmentationController);
        registerThreadCleanup(videoSegmentationControllerThread, videoSegmentationController);
        registerThreadCleanup(manifestsSynchronizerThread, manifestsSynchronizer);
        registerThreadCleanup(evtExtractionControllerThread, evtExtractionController);

        // Error Pipeline Routing
        QObject::connect(audioAnnotationSegmentationController,
                         &AudioAnnotationSegmentationController::error,
                         &EventExtractorApp,
                         handleError);

        QObject::connect(audioTranscriptionSegmentationController,
                         &AudioTranscriptionSegmentationController::error,
                         &EventExtractorApp,
                         handleError);

        QObject::connect(audioSegmentationController,
                         &AudioSegmentationController::error,
                         &EventExtractorApp,
                         handleError);

        QObject::connect(videoSegmentationController,
                         &VideoSegmentationController::error,
                         &EventExtractorApp,
                         handleError);

        QObject::connect(manifestsSynchronizer,
                         &ManifestSynchronizer::error,
                         &EventExtractorApp,
                         handleError);

        QObject::connect(evtExtractionController,
                         &EventExtractionController::error,
                         &EventExtractorApp,
                         handleError);

        // --- Asynchronous Pipeline Chains ---
        QObject::connect(audioAnnotationSegmentationController,
                         &AudioAnnotationSegmentationController::manifestReady,
                         manifestsSynchronizer,
                         &ManifestSynchronizer::handleInput);

        QObject::connect(audioAnnotationSegmentationController,
                         &AudioAnnotationSegmentationController::manifestReady,
                         &EventExtractorApp,
                         [](const FileInfo &fInfo){
                             qDebug() << "MAIN THREAD: Received AudioAnnotationSegmentationController Manifest Relay:" << fInfo.url.path();
                         });

        QObject::connect(audioTranscriptionSegmentationController,
                         &AudioTranscriptionSegmentationController::manifestReady,
                         manifestsSynchronizer,
                         &ManifestSynchronizer::handleInput);

        QObject::connect(audioTranscriptionSegmentationController,
                         &AudioTranscriptionSegmentationController::manifestReady,
                         &EventExtractorApp,
                         [](const FileInfo &fInfo){
                             qDebug() << "MAIN THREAD: Received AudioTranscriptionSegmentationController Manifest Relay:" << fInfo.url.path();
                         });

        QObject::connect(audioSegmentationController,
                         &AudioSegmentationController::manifestReady,
                         manifestsSynchronizer,
                         &ManifestSynchronizer::handleInput);

        QObject::connect(audioSegmentationController,
                         &AudioSegmentationController::manifestReady,
                         &EventExtractorApp,
                         [](const FileInfo &fInfo){
                             qDebug() << "MAIN THREAD: Received AudioSegmentationController Manifest Relay:" << fInfo.url.path();
                         });

        QObject::connect(videoSegmentationController,
                         &VideoSegmentationController::manifestReady,
                         manifestsSynchronizer,
                         &ManifestSynchronizer::handleInput);

        QObject::connect(videoSegmentationController,
                         &VideoSegmentationController::manifestReady,
                         &EventExtractorApp,
                         [](const FileInfo &fInfo){
                             qDebug() << "MAIN THREAD: Received VideoSegmentationController Manifest Relay:" << fInfo.url.path();
                         });

        // QObject::connect(videoSegController,
        //                  &VideoSegmentationController::finished,
        //                  manifestsSynchronizer,
        //                  &ManifestSynchronizer::manifestReady,
        //                  Qt::QueuedConnection);

        QObject::connect(manifestsSynchronizer,
                         &ManifestSynchronizer::finished,
                         &EventExtractorApp,
                         [&EventExtractorApp,
                          manifestsSynchronizer,
                          evtExtractionController,
                          audioAnnotationSegmentationControllerThread,
                          audioTranscriptionSegmentationControllerThread,
                          audioSegmentationControllerThread,
                          videoSegmentationControllerThread,
                          manifestsSynchronizerThread,
                          evtExtractionControllerThread](FileInfo fInfo) {

                             if (fInfo.success)
                             {
                                 qDebug() << "✅ Synchronization completed successfully!";
                                 qDebug() << "  Synchronized segments:" << manifestsSynchronizer->getSynchronizedSegmentCount();
                                 qDebug() << "  Output file:" << fInfo.url.path();

                                 // Route the success data to the next stage safely across threads
                                 QTimer::singleShot(0,
                                                    evtExtractionController,
                                                    [evtExtractionController,
                                                    fInfo]() {
                                     evtExtractionController->handleInput(fInfo);
                                 });

                             } else {
                                 qDebug() << "❌ Synchronization failed!";
                                 qDebug() << "  Error:" << manifestsSynchronizer->lastError();
                                 qCritical() << "Sync failed!";

                                 // 1. Manually trigger the safe shutdown sequence for ALL running threads
                                 audioAnnotationSegmentationControllerThread->quit();
                                 audioTranscriptionSegmentationControllerThread->quit();
                                 audioSegmentationControllerThread->quit();
                                 videoSegmentationControllerThread->quit();
                                 manifestsSynchronizerThread->quit();
                                 evtExtractionControllerThread->quit();

                                 // 2. Shut down the main event loop ONLY after thread cleanup is queued
                                 QMetaObject::invokeMethod(&EventExtractorApp, "quit", Qt::QueuedConnection);
                             }
                         });

        QApplication *appPtr = &EventExtractorApp;

        // 1. Map the finished signal to cleanly quit all threads.
        // Qt's signal/slot mechanism guarantees that if a thread has already
        // finished and deleted itself early, the signal is safely ignored (zero segfaults).
        QObject::connect(evtExtractionController,
                         &EventExtractionController::finished,
                         audioAnnotationSegmentationControllerThread,
                         &QThread::quit,
                         Qt::QueuedConnection);

        QObject::connect(evtExtractionController,
                         &EventExtractionController::finished,
                         audioTranscriptionSegmentationControllerThread,
                         &QThread::quit,
                         Qt::QueuedConnection);

        QObject::connect(evtExtractionController,
                         &EventExtractionController::finished,
                         audioSegmentationControllerThread,
                         &QThread::quit,
                         Qt::QueuedConnection);

        QObject::connect(evtExtractionController,
                         &EventExtractionController::finished,
                         videoSegmentationControllerThread,
                         &QThread::quit,
                         Qt::QueuedConnection);

        QObject::connect(evtExtractionController,
                         &EventExtractionController::finished,
                         manifestsSynchronizerThread,
                         &QThread::quit,
                         Qt::QueuedConnection);

        QObject::connect(evtExtractionController,
                         &EventExtractionController::finished,
                         evtExtractionControllerThread,
                         &QThread::quit, Qt::QueuedConnection);

        // 2. Schedule the next round
        QObject::connect(evtExtractionController,
                         &EventExtractionController::finished,
                         appPtr,
                         [appPtr,
                          &currentSlowdownIdx,
                          &runNextPipeline]() {
            qDebug() << "Round processing complete. Asynchronously launching next round...";
            currentSlowdownIdx++;

            // Allow a brief moment for the main event loop to process the quit() signals
            // and trigger the background deleteLater() cleanups before spinning up the next round.
            QTimer::singleShot(250,
                               appPtr,
                               [&runNextPipeline]() {
                                   runNextPipeline();
            });
        },
                         Qt::QueuedConnection);

        // Fire Execution Threads
        audioAnnotationSegmentationControllerThread->start();
        audioTranscriptionSegmentationControllerThread->start();
        audioSegmentationControllerThread->start();
        videoSegmentationControllerThread->start();
        manifestsSynchronizerThread->start();
        evtExtractionControllerThread->start();

        // Kickstart the workers with their inputs directly (skipping obsolete Transcode dependencies)
        FileInfo audioFileInfo(cmdOptions->fileInfo.outAudioMediaUrl.path(), FileType_t::AudioFile);
        FileInfo videoFileInfo(cmdOptions->fileInfo.inVideoMediaUrl.path(), FileType_t::VideoFile);

        QTimer::singleShot(0,
                           audioAnnotationSegmentationController,
                           [audioAnnotationSegmentationController,
                            capturedAnnotationResult]() {
                               audioAnnotationSegmentationController->handleInput(capturedAnnotationResult);
                           });

        QTimer::singleShot(0,
                           audioTranscriptionSegmentationController,
                           [audioTranscriptionSegmentationController,
                            capturedTranscriptionResult]() {
                               audioTranscriptionSegmentationController->handleInput(capturedTranscriptionResult);
                           });

        QTimer::singleShot(0,
                           audioSegmentationController,
                           [audioSegmentationController,
                            audioFileInfo]() {
                               audioSegmentationController->handleInput(audioFileInfo);
                           });

        QTimer::singleShot(0,
                           videoSegmentationController,
                           [videoSegmentationController,
                            videoFileInfo]() {
                               videoSegmentationController->handleInput(videoFileInfo);
                           });
    };

    // --- PHASE 1: Run Transcoding and Transcription Once Upfront ---
    qDebug() << "======================================================================";
    qDebug() << "🌀 Launching Phase 1: Global Audio Extraction & Text Transcription";
    qDebug() << "======================================================================";

    TranscodeController * transcodeController = new TranscodeController;
    transcodeController->setupAudioTranscoding(cmdOptions->fileInfo.inVideoMediaUrl,
                                               cmdOptions->fileInfo.outAudioMediaUrl);
    QThread *transcodeControllerThread = new QThread;
    transcodeController->moveToThread(transcodeControllerThread);

    AudioTranscriptionController * transcribeAudioController = new AudioTranscriptionController;
    transcribeAudioController->setModel(Deepgram_AIS2TT2SModels_t::DeepgramS2TNova2GeneralEN);
    transcribeAudioController->setAnnotationDir(cmdOptions->dirInfo.annotation_and_transcriptsDir.absoluteFilePath());

    QThread *transcribeAudioControllerThread = new QThread;
    transcribeAudioController->moveToThread(transcribeAudioControllerThread);

    ConverterController *convertController = new ConverterController;
    convertController->setLineLength(200);
    convertController->setConverterType(ConverterType_t::DeepgramWebVttConverter);
    QThread *convertControllerThread = new QThread;
    convertController->moveToThread(convertControllerThread);

    // Initial Preparation Lifecycle management
    registerThreadCleanup(transcodeControllerThread, transcodeController);
    registerThreadCleanup(transcribeAudioControllerThread, transcribeAudioController);
    registerThreadCleanup(convertControllerThread, convertController);

    QObject::connect(transcodeController,
                     &TranscodeController::error,
                     &EventExtractorApp,
                     handleError);

    QObject::connect(transcribeAudioController,
                     &AudioTranscriptionController::error,
                     &EventExtractorApp,
                     handleError);

    QObject::connect(convertController,
                     &ConverterController::conversionError,
                     &EventExtractorApp,
                     handleError);

    // Route Transcode -> Transcription
    QObject::connect(transcodeController,
                     &TranscodeController::finished,
                     transcribeAudioController,
                     &AudioTranscriptionController::handleInput);

    // Route Transcription -> Converter
    QObject::connect(transcribeAudioController,
                     &AudioTranscriptionController::finished,
                     convertController,
                     &ConverterController::handleInput);

    // Once Phase 1 conversion is complete, cache the annotation and transcription files and bridge into Phase 2
    QObject::connect(convertController,
                     &ConverterController::finished,
                     &EventExtractorApp,
                     [&capturedAnnotationResult,
                      &capturedTranscriptionResult,
                      &runNextPipeline,
                      transcribeAudioController,
                      transcribeAudioControllerThread,
                      transcodeControllerThread,
                      convertControllerThread](FileInfo audioTranscriptionResult) {

                         qDebug() << "✨ Master audio transcription complete! Annotation saved to:" << audioTranscriptionResult.url.path();

                         FileInfo audioAnnotationResult(transcribeAudioController->audioAnnotationPath(), FileType_t::AnnotationFile);
                         capturedAnnotationResult = audioAnnotationResult;
                         capturedTranscriptionResult = audioTranscriptionResult;

                         // Safely tear down Phase 1 background threads
                         transcodeControllerThread->quit();
                         transcodeControllerThread->wait();

                         transcribeAudioControllerThread->quit();
                         transcribeAudioControllerThread->wait();

                         convertControllerThread->quit();
                         convertControllerThread->wait();

                         // Hand off control to sequential slowdown execution phase
                         runNextPipeline();
                     });

    // Start Phase 1 threads
    transcodeControllerThread->start();
    transcribeAudioControllerThread->start();
    convertControllerThread->start();

    // Fire the initial transcoding routine
    QTimer::singleShot(0, transcodeController, &TranscodeController::startTranscoding);

    qint32 result = EventExtractorApp.exec();
    qDebug() << "System runtime lifecycle loop closed smoothly.";
    return result;
}


namespace ModuleRunners {

    /**
     * @brief runTranscode
     * @param evtExtractionApp
     * @param cmdOptions
     * @param errorHandler
     */
    void runTranscode(QApplication * evtExtractionApp,
                      QSharedPointer<CmdLineOptions> cmdOptions,
                      const std::function<void(const QString&)>& errorHandler)
    {
        qDebug() << "======================================================================";
        qDebug() << "🎙️ Launching Isolated Module: Transcoding Only";
        qDebug() << "======================================================================";

        // 1. Instantiate and Configure the Controller
        TranscodeController * transcodeController = new TranscodeController;
        transcodeController->setupAudioTranscoding(cmdOptions->fileInfo.inVideoMediaUrl,
                                                   cmdOptions->fileInfo.outAudioMediaUrl);

        QThread *transcodeControllerThread = new QThread;
        transcodeController->moveToThread(transcodeControllerThread);

        // 2. Memory Lifecycle Management
        registerThreadCleanup(transcodeControllerThread, transcodeController);

        // 3. Signal/Slot Routing
        QObject::connect(transcodeController,
                         &TranscodeController::error,
                         evtExtractionApp,
                         errorHandler);

        // 4. Tell the worker thread to safely exit its event loop first
        QObject::connect(transcodeController,
                         &TranscodeController::finished,
                         evtExtractionApp,
                         [transcodeControllerThread,
                         cmdOptions]() {
                             qDebug() << "✅ Isolated transcoding complete! Output saved to:"
                                      << cmdOptions->fileInfo.outAudioMediaUrl.path();

                             // Tell the worker thread to safely exit its event loop first
                             transcodeControllerThread->quit();
                         });

        // 5. Tell the main application to quit ONLY after the thread has fully spun down
        QObject::connect(transcodeControllerThread,
                         &QThread::finished,
                         evtExtractionApp,
                         &QApplication::quit,
                         Qt::QueuedConnection);

        // 6. Start Thread
        transcodeControllerThread->start();

        // 7. Trigger Execution
        // Because transcodeController was moved to transcodeControllerThread, QTimer::singleShot
        // with a context object guarantees startTranscoding runs safely inside the worker thread's
        // event loop.
        QTimer::singleShot(0, transcodeController, &TranscodeController::startTranscoding);
    }

    /**
     * @brief runTranscribeOnly
     * @param evtExtractionApp
     * @param cmdOptions
     * @param errorHandler
     */
    void runTranscribeOnly(QApplication * evtExtractionApp,
                           QSharedPointer<CmdLineOptions> cmdOptions,
                           const std::function<void(const QString&)>& errorHandler)
    {
        qDebug() << "======================================================================";
        qDebug() << "🎙️ Launching Isolated Module: Transcribing Only";
        qDebug() << "======================================================================";

        // 1. Instantiate and Configure the Controller
        AudioTranscriptionController* transcribeAudioController = new AudioTranscriptionController;
        transcribeAudioController->setModel(cmdOptions->speech2TextModel); // Deepgram_AIS2TT2SModels_t::DeepgramS2TNova2GeneralEN
        transcribeAudioController->setAnnotationDir(cmdOptions->dirInfo.annotation_and_transcriptsDir.absoluteFilePath());
        transcribeAudioController->setAnnotationClipsOutputDir(cmdOptions->dirInfo.annotationClipsDir.absoluteFilePath());

        QThread* transcribeAudioControllerThread = new QThread;
        transcribeAudioController->moveToThread(transcribeAudioControllerThread);

        // 2. Memory Lifecycle Management
        registerThreadCleanup(transcribeAudioControllerThread, transcribeAudioController);

        // 3. Signal/Slot Routing
        QObject::connect(transcribeAudioController,
                         &AudioTranscriptionController::error,
                         evtExtractionApp,
                         errorHandler);

        // 4. Tell the worker thread to safely exit its event loop first
        QObject::connect(transcribeAudioController,
                         &AudioTranscriptionController::finished,
                         evtExtractionApp,
                         [transcribeAudioControllerThread](const FileInfo& result) {
                             qDebug() << "✅ Isolated transcription complete! Output:" << result.url.path();

                             // Use queued connection to ensure thread cleans up safely before app shutdown
                             transcribeAudioControllerThread->quit();
                         });

        // 5. Tell the main application to quit ONLY after the thread has fully spun down
        QObject::connect(transcribeAudioControllerThread,
                         &QThread::finished,
                         evtExtractionApp,
                         &QApplication::quit,
                         Qt::QueuedConnection);

        // 6. Start Thread
        transcribeAudioControllerThread->start();

        // 7. Trigger Execution
        // Since we skipped transcoding, we must feed the input directly from cmdOptions.
        FileInfo audioFilePath(cmdOptions->audioFile, FileType_t::AudioFile);

        QTimer::singleShot(0,
                           transcribeAudioController,
                           [transcribeAudioController,
                            audioFilePath]() {
                               transcribeAudioController->handleInput(audioFilePath);
                           });
    }

    /**
     * @brief runConvertOnly
     * @param evtExtractionApp
     * @param cmdOptions
     * @param errorHandler
     */
    void runConvertOnly(QApplication * evtExtractionApp,
                        QSharedPointer<CmdLineOptions> cmdOptions,
                        const std::function<void(const QString&)>& errorHandler)
    {
        qDebug() << "======================================================================";
        qDebug() << "📝 Launching Isolated Module: Conversion Only";
        qDebug() << "======================================================================";

        // 1. Instantiate and Configure the Controller
        ConverterController *convertController = new ConverterController;
        convertController->setLineLength(200);
        convertController->setConverterType(cmdOptions->transcriptionConverterType); //ConverterType_t::DeepgramWebVttConverter

        QThread* convertControllerThread = new QThread;
        convertController->moveToThread(convertControllerThread);

        // 2. Memory Lifecycle Management
        registerThreadCleanup(convertControllerThread, convertController);

        // 3. Signal/Slot Routing
        // Note: ConverterController uses 'conversionError' instead of the standard 'error' signal
        QObject::connect(convertController,
                         &ConverterController::conversionError,
                         evtExtractionApp,
                         errorHandler);

        // 4. Tell the worker thread to safely exit its event loop first
        QObject::connect(convertController,
                         &ConverterController::finished,
                         evtExtractionApp,
                         [convertControllerThread](const FileInfo& result) {
                             qDebug() << "✅ Isolated conversion complete! VTT Output saved to:" << result.url.path();

                             convertControllerThread->quit();
                         });

        // 5. Tell the main application to quit ONLY after the thread has fully spun down
        QObject::connect(convertControllerThread,
                         &QThread::finished,
                         evtExtractionApp,
                         &QApplication::quit,
                         Qt::QueuedConnection);

        // 6. Start Thread
        convertControllerThread->start();

        // 7. Trigger Execution
        // Since we skipped the Transcribe phase, we must provide the raw transcript file manually.
        // Ensure you have a flag in your commandLineParser (e.g., -an or -tr) to populate this path.

        FileInfo inputAnnotationPath(cmdOptions->annotationFile, FileType_t::AnnotationFile);

        QTimer::singleShot(0,
                           convertController,
                           [convertController,
                            inputAnnotationPath]() {
                               convertController->handleInput(inputAnnotationPath);
                           });
    }

    /**
     * @brief runSegmentOnly
     * @param evtExtractionApp
     * @param cmdOptions
     * @param errorHandler
     */
    void runSegmentOnly(QApplication * evtExtractionApp,
                        QSharedPointer<CmdLineOptions> cmdOptions,
                        const std::function<void(const QString&)>& errorHandler)
    {
        qDebug() << "======================================================================";
        qDebug() << "📝 Launching Isolated Module: Segment Only";
        qDebug() << "======================================================================";

        if (qFuzzyIsNull(cmdOptions->clipDuration))
        {
            cmdOptions->clipDuration = *Constants::Q_QGS_DEFAULT_CLIP_SEC;
        }

        qint32 currentSlowdownIdx = 0;

        double currentFactor = cmdOptions->slowdownFactors.at(currentSlowdownIdx);

        qreal localClipDuration = cmdOptions->clipDuration;
        if (currentFactor > 0)
        {
            localClipDuration = cmdOptions->clipDuration / currentFactor;
        }

        // 1. Audio Annotation Segmentation Controller Worker
        AudioAnnotationSegmentationController * audioAnnotationSegmentationController = new AudioAnnotationSegmentationController;
        audioAnnotationSegmentationController->setClipSec(localClipDuration);

        QString targetAnnotationClipsDir = QDir(cmdOptions->dirInfo.annotationClipsDir.absoluteFilePath())
                                               .filePath(QString("segments-%1_minClip-%2_Slowdown")
                                                             .arg(QString::number(cmdOptions->clipDuration / 60.0, 'f', 2),
                                                                  QString::number(currentFactor, 'f', 2)));

        audioAnnotationSegmentationController->setAnnotationClipsOutputDir(targetAnnotationClipsDir);
        // audioAnnotationSegmentationController->setAnnotationManifest();

        QThread * audioAnnotationSegmentationControllerThread = new QThread;
        audioAnnotationSegmentationController->moveToThread(audioAnnotationSegmentationControllerThread);

        // 2. Audio Transcription Segmentation Controller Worker
        AudioTranscriptionSegmentationController * audioTranscriptionSegmentationController = new AudioTranscriptionSegmentationController;
        audioTranscriptionSegmentationController->setClipSec(localClipDuration);

        QString targetTranscriptionClipsDir = QDir(cmdOptions->dirInfo.transcriptClipsDir.absoluteFilePath())
                                                  .filePath(QString("segments-%1_minClip-%2_Slowdown")
                                                                .arg(QString::number(cmdOptions->clipDuration / 60.0, 'f', 2),
                                                                     QString::number(currentFactor, 'f', 2)));

        audioTranscriptionSegmentationController->setTranscriptionClipsOutputDir(targetTranscriptionClipsDir);
        audioTranscriptionSegmentationController->setLineLength(200);
        // audioTranscriptionSegmentationController->setTranscriptionManifest(...);

        QThread * audioTranscriptionSegmentationControllerThread = new QThread;
        audioTranscriptionSegmentationController->moveToThread(audioTranscriptionSegmentationControllerThread);

        // 3. Audio Segmentation Controller Worker
        AudioSegmentationController *audioSegmentationController = new AudioSegmentationController;
        audioSegmentationController->setClipSec(localClipDuration);

        QString targetAudioClipsDir = QDir(cmdOptions->dirInfo.audioClipsDir.absoluteFilePath())
                                          .filePath(QString("segments-%1_minClip-%2_Slowdown")
                                                        .arg(QString::number(cmdOptions->clipDuration / 60.0, 'f', 2),
                                                             QString::number(currentFactor, 'f', 2)));

        audioSegmentationController->setOutputDir(targetAudioClipsDir);

        QThread *audioSegmentationControllerThread = new QThread;
        audioSegmentationController->moveToThread(audioSegmentationControllerThread);

        // 4. Video Segmentation Controller Worker
        VideoSegmentationController *videoSegmentationController = new VideoSegmentationController;

        QString targetVideoClipsDir = QDir(cmdOptions->dirInfo.videoClipsDir.absoluteFilePath())
                                          .filePath(QString("segments-Gemini25_Flash5fps21crf-%1minClip-%2-Slowdown")
                                                        .arg(QString::number(cmdOptions->clipDuration / 60.0, 'f', 2),
                                                             QString::number(currentFactor, 'f', 2)));

        videoSegmentationController->setupVideoCutter(cmdOptions->videoSegmentationModel, //ModelCode_t::Gemini25_Flash5fps21crf
                                                      targetVideoClipsDir,
                                                      cmdOptions->clipDuration,
                                                      currentFactor,
                                                      SegmentationMode_t::UseLibrary);

        QThread *videoSegmentationControllerThread = new QThread;
        videoSegmentationController->moveToThread(videoSegmentationControllerThread);

        // 5. Manifest Synchronizer Controller Worker
        ManifestSynchronizer *manifestsSynchronizer = new ManifestSynchronizer;
        QString manifestPath = QDir(cmdOptions->dirInfo.defaultRootDir.absoluteFilePath())
                                   .filePath(QString("synchronized_manifests_%1_minClip_%2xSlowdownFactor.json")
                                                 .arg(QString::number(cmdOptions->clipDuration / 60.0, 'f', 2),
                                                      QString::number(currentFactor, 'f', 2)));
        manifestsSynchronizer->setOutputPath(manifestPath);

        QThread *manifestsSynchronizerThread = new QThread;
        manifestsSynchronizer->moveToThread(manifestsSynchronizerThread);

        QObject::connect(manifestsSynchronizer,
                         &ManifestSynchronizer::collectionComplete,
                         manifestsSynchronizer,
                         &ManifestSynchronizer::synchronizeManifests,
                         Qt::QueuedConnection);

        // 6. Memory Lifecycle Management
        registerThreadCleanup(audioAnnotationSegmentationControllerThread,      audioAnnotationSegmentationController);
        registerThreadCleanup(audioTranscriptionSegmentationControllerThread,   audioTranscriptionSegmentationController);
        registerThreadCleanup(audioSegmentationControllerThread,                audioSegmentationController);
        registerThreadCleanup(videoSegmentationControllerThread,                videoSegmentationController);
        registerThreadCleanup(manifestsSynchronizerThread,                      manifestsSynchronizer);

        // 7. Signal/Slot Routing
        QObject::connect(audioAnnotationSegmentationController,
                         &AudioAnnotationSegmentationController::error,
                         evtExtractionApp,
                         errorHandler);

        QObject::connect(audioAnnotationSegmentationController,
                         &AudioAnnotationSegmentationController::manifestReady,
                         manifestsSynchronizer,
                         &ManifestSynchronizer::handleInput);

        QObject::connect(audioTranscriptionSegmentationController,
                         &AudioTranscriptionSegmentationController::error,
                         evtExtractionApp,
                         errorHandler);

        QObject::connect(audioTranscriptionSegmentationController,
                         &AudioTranscriptionSegmentationController::manifestReady,
                         manifestsSynchronizer,
                         &ManifestSynchronizer::handleInput);

        QObject::connect(audioSegmentationController,
                         &AudioSegmentationController::error,
                         evtExtractionApp,
                         errorHandler);

        QObject::connect(audioSegmentationController,
                         &AudioSegmentationController::manifestReady,
                         manifestsSynchronizer,
                         &ManifestSynchronizer::handleInput);

        QObject::connect(videoSegmentationController,
                         &VideoSegmentationController::error,
                         evtExtractionApp,
                         errorHandler);

        QObject::connect(videoSegmentationController,
                         &VideoSegmentationController::manifestReady,
                         manifestsSynchronizer,
                         &ManifestSynchronizer::handleInput);

        QObject::connect(manifestsSynchronizer,
                         &ManifestSynchronizer::error,
                         evtExtractionApp,
                         errorHandler);

        // 8. Tell the worker thread to safely exit its event loop first
        QObject::connect(manifestsSynchronizer,
                         &ManifestSynchronizer::finished,
                         [audioAnnotationSegmentationControllerThread,
                          audioTranscriptionSegmentationControllerThread,
                          audioSegmentationControllerThread,
                          videoSegmentationControllerThread,
                          manifestsSynchronizerThread,
                          manifestsSynchronizer](FileInfo fInfo) {

                             if (fInfo.success)
                             {
                                 qDebug() << "✅ Synchronization completed successfully!";
                                 qDebug() << "  Synchronized segments:" << manifestsSynchronizer->getSynchronizedSegmentCount();
                                 qDebug() << "  Output file:" << fInfo.url.path();
                             } else {
                                 qDebug() << "❌ Synchronization failed!";
                                 qDebug() << "  Error:" << manifestsSynchronizer->lastError();
                                 qCritical() << "Sync failed!";
                             }

                             // Stop ALL background threads
                             audioAnnotationSegmentationControllerThread->quit();
                             audioTranscriptionSegmentationControllerThread->quit();
                             audioSegmentationControllerThread->quit();
                             videoSegmentationControllerThread->quit();
                             manifestsSynchronizerThread->quit();
                         });

        // 9. Shut down the application once the final pipeline thread fully exits
        QObject::connect(manifestsSynchronizerThread,
                         &QThread::finished,
                         evtExtractionApp,
                         &QApplication::quit,
                         Qt::QueuedConnection);

        // 10. Start Thread
        audioAnnotationSegmentationControllerThread->start();
        audioTranscriptionSegmentationControllerThread->start();
        audioSegmentationControllerThread->start();
        videoSegmentationControllerThread->start();
        manifestsSynchronizerThread->start();

        // 11. Trigger Execution
        // Audio Annotation Segmentation
        FileInfo inputAnnotationPath(cmdOptions->annotationFile, FileType_t::AnnotationFile);
        QTimer::singleShot(0,
                           audioAnnotationSegmentationController,
                           [audioAnnotationSegmentationController,
                            inputAnnotationPath]() {
                               audioAnnotationSegmentationController->handleInput(inputAnnotationPath);
                           });

        // Audio VTT Transcription Segmentation
        FileInfo inputVTTTranscriptionPath(cmdOptions->transcriptionFile, FileType_t::VTTTranscriptionFile);
        QTimer::singleShot(0,
                           audioTranscriptionSegmentationController,
                           [audioTranscriptionSegmentationController,
                            inputVTTTranscriptionPath]() {
                               audioTranscriptionSegmentationController->handleInput(inputVTTTranscriptionPath);
                           });

        // Audio Segmentation
        FileInfo inputAudioPath(cmdOptions->audioFile, FileType_t::AudioFile);
        QTimer::singleShot(0,
                           audioSegmentationController,
                           [audioSegmentationController,
                            inputAudioPath]() {
                               audioSegmentationController->handleInput(inputAudioPath);
                           });

        // Video Segmentation
        FileInfo inputVideoPath(cmdOptions->videoFile, FileType_t::VideoFile);
        QTimer::singleShot(0,
                           videoSegmentationController,
                           [videoSegmentationController,
                            inputVideoPath]() {
                               videoSegmentationController->handleInput(inputVideoPath);
                           });
    }

    // Define a state context to survive the asynchronous lifecycle
    struct EvtExtractionContext {
        qint32 currentSlowdownIdx = 0;
    };

    /**
     * @brief executeEvtExtractionRound  Stateless recursive helper to process multiple slowdown factors safely
     * @param evtExtractionApp
     * @param cmdOptions
     * @param errorHandler
     * @param state
     */
    void executeEvtExtractionRound(QApplication * evtExtractionApp,
                                   QSharedPointer<CmdLineOptions> cmdOptions,
                                   const std::function<void(const QString&)>& errorHandler,
                                   QSharedPointer<EvtExtractionContext> state)
    {
        if (state->currentSlowdownIdx >= cmdOptions->slowdownFactors.size())
        {
            qDebug() << "=================================================================================";
            qDebug() << "✅ All sequential event extraction rounds completed successfully. Exiting clean.";
            qDebug() << "=================================================================================";

            QMetaObject::invokeMethod(evtExtractionApp, "quit", Qt::QueuedConnection);

            return;
        }

        double currentFactor = cmdOptions->slowdownFactors.at(state->currentSlowdownIdx);

        qDebug() << "=================================================";
        qDebug() << " 🚀 Launching Isolated Extraction Round: " << (state->currentSlowdownIdx + 1)
                 << " of " << cmdOptions->slowdownFactors.size()
                 << " | Slowdown Factor Target:" << currentFactor << "x";
        qDebug() << "=================================================";

        // Permanently fix the struct if it's 0 so downstream formatting works
        if (qFuzzyIsNull(cmdOptions->clipDuration))
        {
            cmdOptions->clipDuration = *Constants::Q_QGS_DEFAULT_CLIP_SEC;
        }

        // --- 1. Controller Worker Setup ---
        EventExtractionController *evtExtractionController = new EventExtractionController;
        evtExtractionController->setupModel(cmdOptions->aiModelType); //AIModels_t::Google_Gemini_API_25Flash

        const auto &cfg = AIModelMap[cmdOptions->aiModelType];

        QString markdownReportPath = QDir(cmdOptions->dirInfo.defaultRootDir.absoluteFilePath())
                                         .filePath(QString("%1-%2x_Slowdown.md")
                                                   .arg(cfg.name,
                                                        QString::number(currentFactor, 'f', 2)));

        evtExtractionController->setOutputFile(markdownReportPath);
        evtExtractionController->addTermsForSearching(cmdOptions->searchedTerms);
        evtExtractionController->setSearchType(cmdOptions->searchType); //KeywordMatchType_t::Any
        evtExtractionController->setMinConfidenceThreshold(cmdOptions->confidenceThreshold);
        evtExtractionController->setTaskTemplateType(cmdOptions->taskAnalysis); //TaskTemplate::Video_Analysis_Task

        QString sysRefereePrompt = PromptManager::instance().getSystemProfile(cmdOptions->systemProfile); //Persona::FIFA_Referee
        evtExtractionController->setSystemPrompt(sysRefereePrompt);

        // --- 2. Safe Thread Instantiation (No Parents) ---
        QThread *evtExtractionControllerThread = new QThread();
        evtExtractionController->moveToThread(evtExtractionControllerThread);

        // --- 3. Cleanup & Signal Routing ---
        registerThreadCleanup(evtExtractionControllerThread, evtExtractionController);

        QObject::connect(evtExtractionController,
                         &EventExtractionController::error,
                         evtExtractionApp,
                         errorHandler);

        // Clean lambda capture for the isolated logging relay
        QObject::connect(evtExtractionController,
                         &EventExtractionController::finished,
                         evtExtractionApp,
                         [](const FileInfo& result) {
                             qDebug() << "✅ Extraction round complete! Output saved to:" << result.url.path();
                         });

        // 1. Thread Teardown Routing
        QObject::connect(evtExtractionController,
                         &EventExtractionController::finished,
                         evtExtractionControllerThread,
                         &QThread::quit,
                         Qt::QueuedConnection);

        // 2. ✨ THE "FRESH EXECUTION" SPARK ✨
        // Trigger the next round safely only after the current thread has completely died.
        QObject::connect(evtExtractionControllerThread,
                         &QThread::finished,
                         evtExtractionApp,
                         [evtExtractionApp, cmdOptions, errorHandler, state]() {
                             qDebug() << "Round thread teardown complete. Asynchronously launching next round...";
                             state->currentSlowdownIdx++;
                             executeEvtExtractionRound(evtExtractionApp,
                                                       cmdOptions,
                                                       errorHandler,
                                                       state);
                         },
                         Qt::QueuedConnection);

        // Start Thread
        evtExtractionControllerThread->start();

        // --- 4. Input Path Dynamic Resolution ---
        // If running multiple rounds, the manifest filename changes per round.
        // We dynamically construct the standard pipeline name to ensure we grab the right file.
        QString dynamicManifestName = QString("synchronized_manifests_%1_minClip_%2xSlowdownFactor.json")
                                          .arg(QString::number(cmdOptions->clipDuration / 60.0, 'f', 2),
                                               QString::number(currentFactor, 'f', 2));

        QString manifestPathToUse = QDir(cmdOptions->dirInfo.defaultRootDir.absoluteFilePath()).filePath(dynamicManifestName);

        // Fallback: If the dynamic file doesn't exist but the user provided an explicit file for a single run, use theirs.
        if (!QFile::exists(manifestPathToUse) && cmdOptions->slowdownFactors.size() == 1)
        {
            manifestPathToUse = cmdOptions->syncedManifestsJsonFile;
        }

        FileInfo inputSyncedManifestsPath(manifestPathToUse, FileType_t::SynchronizedManifest);

        // Kickstart worker
        QTimer::singleShot(0,
                           evtExtractionController,
                           [evtExtractionController,
                           inputSyncedManifestsPath]() {
                               evtExtractionController->handleInput(inputSyncedManifestsPath);
                           });
    }

    /**
     * @brief runEventsExtractionOnly
     * @param evtExtractionApp
     * @param cmdOptions
     * @param errorHandler
     */
    void runEventsExtractionOnly(QApplication * evtExtractionApp,
                                 QSharedPointer<CmdLineOptions> cmdOptions,
                                 const std::function<void(const QString&)>& errorHandler)
    {
        qDebug() << "======================================================";
        qDebug() << "📝 Launching Isolated Module: Events Extraction Only";
        qDebug() << "======================================================";

        // // Here I have to pass the only the factor that the sebments were slowdowned as the first of the list.
        // double currentFactor = cmdOptions->slowdownFactors.at(0);

        // EventExtractionController *evtExtractionController = new EventExtractionController;
        // evtExtractionController->setupModel(AIModels_t::Google_Gemini_API_25Flash);

        // QString markdownReportPath = QDir(cmdOptions->dirInfo.defaultRootDir.absoluteFilePath())
        //                                  .filePath(QString("Google_Gemini_API_25-%1x_Slowdown.md")
        //                                                .arg(QString::number(currentFactor, 'f', 2)));
        // evtExtractionController->setOutputFile(markdownReportPath);
        // evtExtractionController->addTermsForSearching(cmdOptions->searchedTerms);
        // evtExtractionController->setSearchType(KeywordSearchEngine::MatchType::Any);
        // evtExtractionController->setMinConfidenceThreshold(cmdOptions->confidenceThreshould);
        // evtExtractionController->setTaskTemplateType(PromptManager::TaskTemplate::Video_Analysis_Task);

        // QString sysRefereePrompt = PromptManager::instance().getSystemProfile(PromptManager::Persona::FIFA_Referee);
        // evtExtractionController->setSystemPrompt(sysRefereePrompt);

        // QThread *evtExtractionControllerThread = new QThread;
        // evtExtractionController->moveToThread(evtExtractionControllerThread);

        // registerThreadCleanup(evtExtractionControllerThread, evtExtractionController);

        // QObject::connect(evtExtractionController,
        //                  &EventExtractionController::error,
        //                  evtExtractionApp,
        //                  errorHandler);

        // // Clean lambda capture (only 'app') to prevent -Werror unused capture warnings
        // QObject::connect(evtExtractionController,
        //                  &EventExtractionController::finished,
        //                  evtExtractionApp,
        //                  [evtExtractionApp](const FileInfo& result) {
        //                      qDebug() << "✅ Isolated conversion complete! Events Extraction Output saved to:" << result.url.path();

        //                      QMetaObject::invokeMethod(evtExtractionApp, "quit", Qt::QueuedConnection);
        //                  });

        // // 4. Start Thread
        // evtExtractionControllerThread->start();

        // FileInfo inputSyncedManifestsPath(cmdOptions->syncedManifestsJsonFile, FileType_t::SynchronizedManifest);

        // QTimer::singleShot(0,
        //                    evtExtractionController,
        //                    [evtExtractionController,
        //                     inputSyncedManifestsPath]() {
        //                        evtExtractionController->handleInput(inputSyncedManifestsPath);
        //                    });

        // Allocate persistent state context on the heap
        auto state = QSharedPointer<EvtExtractionContext>::create();

        // Kick off the stateless recursive loop
        executeEvtExtractionRound(evtExtractionApp, cmdOptions, errorHandler, state);
    }

    // Define a state context to survive the asynchronous lifecycle
    struct PipelineContext {
        int currentSlowdownIdx = 0;
        FileInfo capturedAnnotationResult; // Holds the audio annotation across rounds
        FileInfo capturedTranscriptionResult; // Holds the audio transcription across rounds
    };

    // Forward declaration if necessary, or just place it above runPipeline
    void runPipelinePhase2Round(QApplication * evtExtractionApp,
                                QSharedPointer<CmdLineOptions> cmdOptions,
                                const std::function<void(const QString&)>& errorHandler,
                                QSharedPointer<PipelineContext> state)
    {
        if (state->currentSlowdownIdx >= cmdOptions->slowdownFactors.size())
        {
            qDebug() << "==========================================================================";
            qDebug() << "✅ All sequential processing rounds completed successfully. Exiting clean.";
            qDebug() << "==========================================================================";

            QMetaObject::invokeMethod(evtExtractionApp, "quit", Qt::QueuedConnection);

            return;
        }

        double currentFactor = cmdOptions->slowdownFactors.at(state->currentSlowdownIdx);

        qDebug() << "=================================================";
        qDebug() << " 🚀 Launching Speed-Dependent Pipeline Round: " << (state->currentSlowdownIdx + 1)
                 << " of " << cmdOptions->slowdownFactors.size()
                 << " | Slowdown Factor Target:" << currentFactor << "x";
        qDebug() << "=================================================";

        if (qFuzzyIsNull(cmdOptions->clipDuration))
        {
            cmdOptions->clipDuration = *Constants::Q_QGS_DEFAULT_CLIP_SEC;
        }

        double localClipDuration = cmdOptions->clipDuration;

        if (currentFactor > 0)
        {
            localClipDuration = cmdOptions->clipDuration / currentFactor;
        }

        // --- 1. Controller Worker Setup ---
        AudioAnnotationSegmentationController * audioAnnotationSegmentationController = new AudioAnnotationSegmentationController;
        audioAnnotationSegmentationController->setClipSec(localClipDuration);
        QString targetAnnotationClipsDir = QDir(cmdOptions->dirInfo.annotationClipsDir.absoluteFilePath())
                                           .filePath(QString("segments-%1_minClip-%2_Slowdown")
                                                     .arg(QString::number(cmdOptions->clipDuration / 60.0, 'f', 2),
                                                          QString::number(currentFactor, 'f', 2)));

        audioAnnotationSegmentationController->setAnnotationClipsOutputDir(targetAnnotationClipsDir);

        AudioTranscriptionSegmentationController * audioTranscriptionSegmentationController = new AudioTranscriptionSegmentationController;
        audioTranscriptionSegmentationController->setClipSec(localClipDuration);

        QString targetTranscriptionClipsDir = QDir(cmdOptions->dirInfo.transcriptClipsDir.absoluteFilePath())
                                              .filePath(QString("segments-%1_minClip-%2_Slowdown")
                                                        .arg(QString::number(cmdOptions->clipDuration / 60.0, 'f', 2),
                                                             QString::number(currentFactor, 'f', 2)));

        audioTranscriptionSegmentationController->setTranscriptionClipsOutputDir(targetTranscriptionClipsDir);
        audioTranscriptionSegmentationController->setLineLength(200);

        AudioSegmentationController *audioSegmentationController = new AudioSegmentationController;
        audioSegmentationController->setClipSec(localClipDuration);
        QString targetAudioClipsDir = QDir(cmdOptions->dirInfo.audioClipsDir.absoluteFilePath())
                                      .filePath(QString("segments-%1_minClip-%2_Slowdown")
                                                .arg(QString::number(cmdOptions->clipDuration / 60.0, 'f', 2),
                                                     QString::number(currentFactor, 'f', 2)));
        audioSegmentationController->setOutputDir(targetAudioClipsDir);

        VideoSegmentationController *videoSegmentationController = new VideoSegmentationController;
        videoSegmentationController->setInputVideo(cmdOptions->fileInfo.inVideoMediaUrl.path());

        QString modelCodeStr = ModelCodeMap.value(cmdOptions->videoSegmentationModel);

        QString targetVideoClipsDir = QDir(cmdOptions->dirInfo.videoClipsDir.absoluteFilePath())
                                      .filePath(QString("%1-%2minClip-%3-Slowdown")
                                                .arg(modelCodeStr,
                                                     QString::number(cmdOptions->clipDuration / 60.0, 'f', 2),
                                                     QString::number(currentFactor, 'f', 2)));

        videoSegmentationController->setupVideoCutter(cmdOptions->videoSegmentationModel, //ModelCode_t::Gemini25_Flash5fps21crf
                                                      targetVideoClipsDir,
                                                      cmdOptions->clipDuration,
                                                      currentFactor,
                                                      SegmentationMode_t::UseLibrary);

        ManifestSynchronizer *manifestsSynchronizer = new ManifestSynchronizer;
        QString manifestPath = QDir(cmdOptions->dirInfo.defaultRootDir.absoluteFilePath())
                               .filePath(QString("synchronized_manifests_%1_minClip_%2xSlowdownFactor.json")
                                         .arg(QString::number(cmdOptions->clipDuration / 60.0, 'f', 2),
                                              QString::number(currentFactor, 'f', 2)));
        manifestsSynchronizer->setOutputPath(manifestPath);

        const auto &cfg = AIModelMap[cmdOptions->aiModelType];

        EventExtractionController *evtExtractionController = new EventExtractionController;
        evtExtractionController->setupModel(cmdOptions->aiModelType); // AIModels_t::Google_Gemini_API_25Flash
        QString markdownReportPath = QDir(cmdOptions->dirInfo.defaultRootDir.absoluteFilePath())
                                     .filePath(QString("%1-%2x_Slowdown.md")
                                               .arg(cfg.name,
                                                    QString::number(currentFactor, 'f', 2)));

        evtExtractionController->setOutputFile(markdownReportPath);
        evtExtractionController->addTermsForSearching(cmdOptions->searchedTerms);
        evtExtractionController->setSearchType(cmdOptions->searchType); //KeywordMatchType_t::Any
        evtExtractionController->setMinConfidenceThreshold(cmdOptions->confidenceThreshold);
        evtExtractionController->setTaskTemplateType(cmdOptions->taskAnalysis); //TaskTemplate::Video_Analysis_Task
        evtExtractionController->setSystemPrompt(PromptManager::instance().getSystemProfile(cmdOptions->systemProfile)); //Persona::FIFA_Referee

        // --- 2. Safe Thread Instantiation (No Parents) ---
        QThread * audioAnnotationSegmentationControllerThread = new QThread();
        QThread * audioTranscriptionSegmentationControllerThread = new QThread();
        QThread * audioSegmentationControllerThread = new QThread();
        QThread * videoSegmentationControllerThread = new QThread();
        QThread * manifestsSynchronizerThread = new QThread();
        QThread * evtExtractionControllerThread = new QThread();

        audioAnnotationSegmentationController->moveToThread(audioAnnotationSegmentationControllerThread);
        audioTranscriptionSegmentationController->moveToThread(audioTranscriptionSegmentationControllerThread);
        audioSegmentationController->moveToThread(audioSegmentationControllerThread);
        videoSegmentationController->moveToThread(videoSegmentationControllerThread);
        manifestsSynchronizer->moveToThread(manifestsSynchronizerThread);
        evtExtractionController->moveToThread(evtExtractionControllerThread);

        // --- 3. Cleanup & Signal Routing ---
        registerThreadCleanup(audioAnnotationSegmentationControllerThread, audioAnnotationSegmentationController);
        registerThreadCleanup(audioTranscriptionSegmentationControllerThread, audioTranscriptionSegmentationController);
        registerThreadCleanup(audioSegmentationControllerThread, audioSegmentationController);
        registerThreadCleanup(videoSegmentationControllerThread, videoSegmentationController);
        registerThreadCleanup(manifestsSynchronizerThread, manifestsSynchronizer);
        registerThreadCleanup(evtExtractionControllerThread, evtExtractionController);

        QObject::connect(audioAnnotationSegmentationController,
                         &AudioAnnotationSegmentationController::error,
                         evtExtractionApp,
                         errorHandler);

        QObject::connect(audioTranscriptionSegmentationController,
                         &AudioTranscriptionSegmentationController::error,
                         evtExtractionApp,
                         errorHandler);

        QObject::connect(audioSegmentationController,
                         &AudioSegmentationController::error,
                         evtExtractionApp,
                         errorHandler);

        QObject::connect(videoSegmentationController,
                         &VideoSegmentationController::error,
                         evtExtractionApp,
                         errorHandler);

        QObject::connect(manifestsSynchronizer,
                         &ManifestSynchronizer::error,
                         evtExtractionApp,
                         errorHandler);

        QObject::connect(evtExtractionController,
                         &EventExtractionController::error,
                         evtExtractionApp,
                         errorHandler);

        // Manifest Relays
        QObject::connect(audioAnnotationSegmentationController,
                         &AudioAnnotationSegmentationController::manifestReady,
                         manifestsSynchronizer,
                         &ManifestSynchronizer::handleInput);

        QObject::connect(audioAnnotationSegmentationController,
                         &AudioAnnotationSegmentationController::manifestReady,
                         evtExtractionApp,
                         [](const FileInfo &fInfo){
                             qDebug() << "MAIN THREAD: Received AudioAnnotationSegmentationController Manifest Relay:" << fInfo.url.path();
                         });

        QObject::connect(audioTranscriptionSegmentationController,
                         &AudioTranscriptionSegmentationController::manifestReady,
                         manifestsSynchronizer,
                         &ManifestSynchronizer::handleInput);

        QObject::connect(audioTranscriptionSegmentationController,
                         &AudioTranscriptionSegmentationController::manifestReady,
                         evtExtractionApp,
                         [](const FileInfo &fInfo){
                             qDebug() << "MAIN THREAD: Received AudioTranscriptionSegmentationController Manifest Relay:" << fInfo.url.path();
                         });

        QObject::connect(audioSegmentationController,
                         &AudioSegmentationController::manifestReady,
                         manifestsSynchronizer,
                         &ManifestSynchronizer::handleInput);

        QObject::connect(audioSegmentationController,
                         &AudioSegmentationController::manifestReady,
                         evtExtractionApp,
                         [](const FileInfo &fInfo){
                             qDebug() << "MAIN THREAD: Received AudioSegmentationController Manifest Relay:" << fInfo.url.path();
                         });

        QObject::connect(videoSegmentationController,
                         &VideoSegmentationController::manifestReady,
                         manifestsSynchronizer,
                         &ManifestSynchronizer::handleInput);

        QObject::connect(videoSegmentationController,
                         &VideoSegmentationController::manifestReady,
                         evtExtractionApp,
                         [](const FileInfo &fInfo){
                             qDebug() << "MAIN THREAD: Received VideoSegmentationController Manifest Relay:" << fInfo.url.path();
                         });

        QObject::connect(manifestsSynchronizer,
                         &ManifestSynchronizer::collectionComplete,
                         manifestsSynchronizer,
                         &ManifestSynchronizer::synchronizeManifests,
                         Qt::QueuedConnection);

        // Smart Pipe Routing for Manifest -> Extractor
        QObject::connect(manifestsSynchronizer,
                         &ManifestSynchronizer::finished,
                         evtExtractionApp,
                         [evtExtractionApp,
                         manifestsSynchronizer,
                         evtExtractionController,
                         audioAnnotationSegmentationControllerThread,
                         audioTranscriptionSegmentationControllerThread,
                         audioSegmentationControllerThread,
                         videoSegmentationControllerThread,
                         manifestsSynchronizerThread,
                         evtExtractionControllerThread](FileInfo fInfo) {

            if (fInfo.success)
            {
                qDebug() << "✅ Synchronization completed successfully!";
                qDebug() << "  Synchronized segments:" << manifestsSynchronizer->getSynchronizedSegmentCount();
                qDebug() << "  Output file:" << fInfo.url.path();

                QMetaObject::invokeMethod(evtExtractionController,
                                          [evtExtractionController,
                                          fInfo]() {
                    evtExtractionController->handleInput(fInfo);
                },
                Qt::QueuedConnection);

            } else {

                qDebug() << "  Error:" << manifestsSynchronizer->lastError();
                qCritical() << "❌ Sync failed! Aborting pipeline safely.";
                audioAnnotationSegmentationControllerThread->quit();
                audioTranscriptionSegmentationControllerThread->quit();
                audioSegmentationControllerThread->quit();
                videoSegmentationControllerThread->quit();
                manifestsSynchronizerThread->quit();
                evtExtractionControllerThread->quit();

                QMetaObject::invokeMethod(evtExtractionApp, "quit", Qt::QueuedConnection);
            }
        });

        // Thread Teardown Routing
        QObject::connect(evtExtractionController,
                         &EventExtractionController::finished,
                         audioAnnotationSegmentationControllerThread,
                         &QThread::quit,
                         Qt::QueuedConnection);

        QObject::connect(evtExtractionController,
                         &EventExtractionController::finished,
                         audioTranscriptionSegmentationControllerThread,
                         &QThread::quit,
                         Qt::QueuedConnection);

        QObject::connect(evtExtractionController,
                         &EventExtractionController::finished,
                         audioSegmentationControllerThread,
                         &QThread::quit,
                         Qt::QueuedConnection);

        QObject::connect(evtExtractionController,
                         &EventExtractionController::finished,
                         videoSegmentationControllerThread,
                         &QThread::quit,
                         Qt::QueuedConnection);

        QObject::connect(evtExtractionController,
                         &EventExtractionController::finished,
                         manifestsSynchronizerThread,
                         &QThread::quit,
                         Qt::QueuedConnection);
        QObject::connect(evtExtractionController,
                         &EventExtractionController::finished,
                         evtExtractionControllerThread,
                         &QThread::quit,
                         Qt::QueuedConnection);

        // =========================================================================================
        // ✨ THE "FRESH EXECUTION" SPARK ✨
        // When the final thread dies, we trigger the next round. The compiler captures 'state'
        // and 'cmdOptions' perfectly. No cyclical pointers are required.
        // =========================================================================================
        QObject::connect(evtExtractionControllerThread,
                         &QThread::finished,
                         evtExtractionApp,
                         [evtExtractionApp,
                         cmdOptions,
                         errorHandler,
                         state]() {
            qDebug() << "Round processing complete. Asynchronously launching next round...";
            state->currentSlowdownIdx++;

            // This creates a 100% fresh stack frame and new controller pointers!
            runPipelinePhase2Round(evtExtractionApp, cmdOptions, errorHandler, state);
        },
        Qt::QueuedConnection);

        // Start Threads
        audioAnnotationSegmentationControllerThread->start();
        audioTranscriptionSegmentationControllerThread->start();
        audioSegmentationControllerThread->start();
        videoSegmentationControllerThread->start();
        manifestsSynchronizerThread->start();
        evtExtractionControllerThread->start();

        // Kickstart workers
        FileInfo audioFileInfo(cmdOptions->fileInfo.outAudioMediaUrl.path(), FileType_t::AudioFile);
        FileInfo videoFileInfo(cmdOptions->fileInfo.inVideoMediaUrl.path(), FileType_t::VideoFile);

        QTimer::singleShot(0,
                           audioAnnotationSegmentationController,
                           [audioAnnotationSegmentationController,
                           state]() {
            audioAnnotationSegmentationController->handleInput(state->capturedAnnotationResult);
        });

        QTimer::singleShot(0,
                           audioTranscriptionSegmentationController,
                           [audioTranscriptionSegmentationController,
                           state]() {
            audioTranscriptionSegmentationController->handleInput(state->capturedTranscriptionResult);
        });

        QTimer::singleShot(0,
                           audioSegmentationController,
                           [audioSegmentationController,
                           audioFileInfo]() {
            audioSegmentationController->handleInput(audioFileInfo);
        });

        QTimer::singleShot(0,
                           videoSegmentationController,
                           [videoSegmentationController,
                           videoFileInfo]() {
            videoSegmentationController->handleInput(videoFileInfo);
        });
    }

    /**
     * @brief runPipeline
     * @param evtExtractionApp
     * @param cmdOptions
     * @param errorHandler
     */
    void runPipeline(QApplication * evtExtractionApp,
                     QSharedPointer<CmdLineOptions> cmdOptions,
                     const std::function<void(const QString&)>& errorHandler)
    {
        // 1. Allocate state on the heap so it survives after runPipeline returns to main()
        auto state = QSharedPointer<PipelineContext>::create();

        // // 2. Use a SharedPointer for the recursive lambda to prevent scoping destruction
        // auto runNextPipeline = QSharedPointer<std::function<void()>>::create();

        // // --- PHASE 2: Sequential Orchestration Loop ---
        // *runNextPipeline = [evtExtractionApp,
        //                    cmdOptions,
        //                    errorHandler,
        //                    state,
        //                    runNextPipeline]() {
        //     if (state->currentSlowdownIdx >= cmdOptions->slowdownFactors.size())
        //     {
        //         qDebug() << "==========================================================================";
        //         qDebug() << "✅ All sequential processing rounds completed successfully. Exiting clean.";
        //         qDebug() << "==========================================================================";
        //         QApplication::quit();
        //         return;
        //     }

        //     double currentFactor = cmdOptions->slowdownFactors.at(state->currentSlowdownIdx);

        //     qDebug() << "=================================================";
        //     qDebug() << " 🚀 Launching Speed-Dependent Pipeline Round: " << (state->currentSlowdownIdx + 1)
        //              << " of " << cmdOptions->slowdownFactors.size()
        //              << " | Slowdown Factor Context Target:" << currentFactor << "x";
        //     qDebug() << "=================================================";

        //     if (qFuzzyIsNull(cmdOptions->clipDuration))
        //     {
        //         cmdOptions->clipDuration = *Constants::Q_QGS_DEFAULT_CLIP_SEC;
        //     }

        //     qreal localClipDuration = cmdOptions->clipDuration;
        //     if (currentFactor > 0)
        //     {
        //         localClipDuration = cmdOptions->clipDuration / currentFactor;
        //     }

        //     // 1. Audio Annotation Segmentation Controller Worker (Runs per round due to variable clip durations)
        //     AudioAnnotationSegmentationController * audioAnnotationSegmentationController = new AudioAnnotationSegmentationController;
        //     audioAnnotationSegmentationController->setClipSec(localClipDuration);

        //     QString targetAnnotationClipsDir = QDir(cmdOptions->dirInfo.annotationClipsDir.absoluteFilePath())
        //                                            .filePath(QString("segments-%1_minClip-%2_Slowdown")
        //                                                          .arg(QString::number(cmdOptions->clipDuration / 60.0, 'f', 2),
        //                                                               QString::number(currentFactor, 'f', 2)));

        //     audioAnnotationSegmentationController->setAnnotationClipsOutputDir(targetAnnotationClipsDir);
        //     // audioAnnotationSegmentationController->setAnnotationManifest(...);
        //     QThread * audioAnnotationSegmentationControllerThread = new QThread;
        //     audioAnnotationSegmentationController->moveToThread(audioAnnotationSegmentationControllerThread);

        //     // 2. Audio Transcription Segmentation Controller Worker (Runs per round due to variable clip durations)
        //     AudioTranscriptionSegmentationController * audioTranscriptionSegmentationController = new AudioTranscriptionSegmentationController;
        //     audioTranscriptionSegmentationController->setClipSec(localClipDuration);

        //     QString targetTranscriptionClipsDir = QDir(cmdOptions->dirInfo.transcriptClipsDir.absoluteFilePath())
        //                                               .filePath(QString("segments-%1_minClip-%2_Slowdown")
        //                                                             .arg(QString::number(cmdOptions->clipDuration / 60.0, 'f', 2),
        //                                                                  QString::number(currentFactor, 'f', 2)));

        //     audioTranscriptionSegmentationController->setTranscriptionClipsOutputDir(targetTranscriptionClipsDir);
        //     audioTranscriptionSegmentationController->setLineLength(200);
        //     // audioTranscriptionSegmentationController->setTranscriptionManifest(...);
        //     QThread * audioTranscriptionSegmentationControllerThread = new QThread;
        //     audioTranscriptionSegmentationController->moveToThread(audioTranscriptionSegmentationControllerThread);

        //     // 3. Audio Segmentation Controller Worker
        //     AudioSegmentationController *audioSegmentationController = new AudioSegmentationController;
        //     audioSegmentationController->setClipSec(localClipDuration);

        //     QString targetAudioClipsDir = QDir(cmdOptions->dirInfo.audioClipsDir.absoluteFilePath())
        //                                       .filePath(QString("segments-%1_minClip-%2_Slowdown")
        //                                                     .arg(QString::number(cmdOptions->clipDuration / 60.0, 'f', 2),
        //                                                          QString::number(currentFactor, 'f', 2)));

        //     audioSegmentationController->setOutputDir(targetAudioClipsDir);
        //     QThread *audioSegmentationControllerThread = new QThread();
        //     audioSegmentationController->moveToThread(audioSegmentationControllerThread);

        //     // 4. Video Segmentation Controller Worker
        //     VideoSegmentationController *videoSegmentationController = new VideoSegmentationController;
        //     videoSegmentationController->setInputVideo(cmdOptions->fileInfo.inVideoMediaUrl.path());

        //     QString targetVideoClipsDir = QDir(cmdOptions->dirInfo.videoClipsDir.absoluteFilePath())
        //                                       .filePath(QString("segments-Gemini25_Flash5fps21crf-%1minClip-%2-Slowdown")
        //                                                     .arg(QString::number(cmdOptions->clipDuration / 60.0, 'f', 2),
        //                                                          QString::number(currentFactor, 'f', 2)));

        //     videoSegmentationController->setupVideoCutter(ModelCode_t::Gemini25_Flash5fps21crf,
        //                                                   targetVideoClipsDir,
        //                                                   cmdOptions->clipDuration,
        //                                                   currentFactor,
        //                                                   SegmentationMode_t::UseLibrary);

        //     QThread *videoSegmentationControllerThread = new QThread;
        //     videoSegmentationController->moveToThread(videoSegmentationControllerThread);

        //     // 5. Manifest Synchronizer Controller Worker
        //     ManifestSynchronizer *manifestsSynchronizer = new ManifestSynchronizer;
        //     QString manifestPath = QDir(cmdOptions->dirInfo.defaultRootDir.absoluteFilePath())
        //                                .filePath(QString("synchronized_manifests_%1_minClip_%2xSlowdownFactor.json")
        //                                              .arg(QString::number(cmdOptions->clipDuration / 60.0, 'f', 2),
        //                                                   QString::number(currentFactor, 'f', 2)));
        //     manifestsSynchronizer->setOutputPath(manifestPath);

        //     QThread *manifestsSynchronizerThread = new QThread;
        //     manifestsSynchronizer->moveToThread(manifestsSynchronizerThread);

        //     QObject::connect(manifestsSynchronizer,
        //                      &ManifestSynchronizer::collectionComplete,
        //                      manifestsSynchronizer,
        //                      &ManifestSynchronizer::synchronizeManifests,
        //                      Qt::QueuedConnection);

        //     // 6. Event Extraction Controller Worker
        //     EventExtractionController *evtExtractionController = new EventExtractionController;
        //     evtExtractionController->setupModel(AIModels_t::Google_Gemini_API_25Flash);

        //     QString markdownReportPath = QDir(cmdOptions->dirInfo.defaultRootDir.absoluteFilePath())
        //                                      .filePath(QString("Google_Gemini_API_25-%1x_Slowdown.md")
        //                                                    .arg(QString::number(currentFactor, 'f', 2)));
        //     evtExtractionController->setOutputFile(markdownReportPath);
        //     evtExtractionController->addTermsForSearching(cmdOptions->searchedTerms);
        //     evtExtractionController->setSearchType(KeywordSearchEngine::MatchType::Any);
        //     evtExtractionController->setMinConfidenceThreshold(cmdOptions->confidenceThreshould);
        //     evtExtractionController->setTaskTemplateType(PromptManager::TaskTemplate::Video_Analysis_Task);

        //     QString sysRefereePrompt = PromptManager::instance().getSystemProfile(PromptManager::Persona::FIFA_Referee);
        //     evtExtractionController->setSystemPrompt(sysRefereePrompt);

        //     QThread *evtExtractionControllerThread = new QThread;
        //     evtExtractionController->moveToThread(evtExtractionControllerThread);


        //     // Cleanup
        //     registerThreadCleanup(audioAnnotationSegmentationControllerThread, audioAnnotationSegmentationController);
        //     registerThreadCleanup(audioTranscriptionSegmentationControllerThread, audioTranscriptionSegmentationController);
        //     registerThreadCleanup(audioSegmentationControllerThread, audioSegmentationController);
        //     registerThreadCleanup(videoSegmentationControllerThread, videoSegmentationController);
        //     registerThreadCleanup(manifestsSynchronizerThread, manifestsSynchronizer);
        //     registerThreadCleanup(evtExtractionControllerThread, evtExtractionController);

        //     // Error Pipeline Routing
        //     QObject::connect(audioAnnotationSegmentationController,
        //                      &AudioAnnotationSegmentationController::error,
        //                      evtExtractionApp,
        //                      errorHandler);

        //     QObject::connect(audioTranscriptionSegmentationController,
        //                      &AudioTranscriptionSegmentationController::error,
        //                      evtExtractionApp,
        //                      errorHandler);

        //     QObject::connect(audioSegmentationController,
        //                      &AudioSegmentationController::error,
        //                      evtExtractionApp,
        //                      errorHandler);

        //     QObject::connect(videoSegmentationController,
        //                      &VideoSegmentationController::error,
        //                      evtExtractionApp,
        //                      errorHandler);

        //     QObject::connect(manifestsSynchronizer,
        //                      &ManifestSynchronizer::error,
        //                      evtExtractionApp,
        //                      errorHandler);

        //     QObject::connect(evtExtractionController,
        //                      &EventExtractionController::error,
        //                      evtExtractionApp,
        //                      errorHandler);

        //     // --- Asynchronous Pipeline Chains ---
        //     QObject::connect(audioAnnotationSegmentationController,
        //                      &AudioAnnotationSegmentationController::manifestReady,
        //                      manifestsSynchronizer,
        //                      &ManifestSynchronizer::handleInput);

        //     QObject::connect(audioAnnotationSegmentationController,
        //                      &AudioAnnotationSegmentationController::manifestReady,
        //                      evtExtractionApp,
        //                      [](const FileInfo &fInfo){
        //                          qDebug() << "MAIN THREAD: Received AudioAnnotationSegmentationController Manifest Relay:" << fInfo.url.path();
        //                      });

        //     QObject::connect(audioTranscriptionSegmentationController,
        //                      &AudioTranscriptionSegmentationController::manifestReady,
        //                      manifestsSynchronizer,
        //                      &ManifestSynchronizer::handleInput);

        //     QObject::connect(audioTranscriptionSegmentationController,
        //                      &AudioTranscriptionSegmentationController::manifestReady,
        //                      evtExtractionApp,
        //                      [](const FileInfo &fInfo){
        //                          qDebug() << "MAIN THREAD: Received AudioTranscriptionSegmentationController Manifest Relay:" << fInfo.url.path();
        //                      });

        //     QObject::connect(audioSegmentationController,
        //                      &AudioSegmentationController::manifestReady,
        //                      manifestsSynchronizer,
        //                      &ManifestSynchronizer::handleInput);

        //     QObject::connect(audioSegmentationController,
        //                      &AudioSegmentationController::manifestReady,
        //                      evtExtractionApp,
        //                      [](const FileInfo &fInfo){
        //                          qDebug() << "MAIN THREAD: Received AudioSegmentationController Manifest Relay:" << fInfo.url.path();
        //                      });

        //     QObject::connect(videoSegmentationController,
        //                      &VideoSegmentationController::manifestReady,
        //                      manifestsSynchronizer,
        //                      &ManifestSynchronizer::handleInput);

        //     QObject::connect(videoSegmentationController,
        //                      &VideoSegmentationController::manifestReady,
        //                      evtExtractionApp,
        //                      [](const FileInfo &fInfo){
        //                          qDebug() << "MAIN THREAD: Received VideoSegmentationController Manifest Relay:" << fInfo.url.path();
        //                      });

        //     QObject::connect(manifestsSynchronizer,
        //                      &ManifestSynchronizer::finished,
        //                      evtExtractionApp,
        //                      [evtExtractionApp,
        //                      manifestsSynchronizer,
        //                      evtExtractionController,
        //                      audioAnnotationSegmentationControllerThread,
        //                      audioTranscriptionSegmentationControllerThread,
        //                      audioSegmentationControllerThread,
        //                      videoSegmentationControllerThread,
        //                      manifestsSynchronizerThread,
        //                      evtExtractionControllerThread](FileInfo fInfo) {

        //                          if (fInfo.success)
        //                          {
        //                              qDebug() << "✅ Synchronization completed successfully!";
        //                              qDebug() << "  Synchronized segments:" << manifestsSynchronizer->getSynchronizedSegmentCount();
        //                              qDebug() << "  Output file:" << fInfo.url.path();

        //                              // Route the success data to the next stage safely across threads
        //                              QTimer::singleShot(0,
        //                                                 evtExtractionController,
        //                                                 [evtExtractionController,
        //                                                  fInfo]() {
        //                                  evtExtractionController->handleInput(fInfo);
        //                              });

        //                          } else {
        //                              qDebug() << "❌ Synchronization failed!";
        //                              qDebug() << "  Error:" << manifestsSynchronizer->lastError();
        //                              qCritical() << "Sync failed!";

        //                              // 1. Manually trigger the safe shutdown sequence for ALL running threads
        //                              audioAnnotationSegmentationControllerThread->quit();
        //                              audioTranscriptionSegmentationControllerThread->quit();
        //                              audioSegmentationControllerThread->quit();
        //                              videoSegmentationControllerThread->quit();
        //                              manifestsSynchronizerThread->quit();
        //                              evtExtractionControllerThread->quit();

        //                              // 2. Shut down the main event loop ONLY after thread cleanup is queued
        //                              QMetaObject::invokeMethod(evtExtractionApp, "quit", Qt::QueuedConnection);
        //                          }
        //                      });

        //     // 1. Map the finished signal to cleanly quit all threads.
        //     // Qt's signal/slot mechanism guarantees that if a thread has already
        //     // finished and deleted itself early, the signal is safely ignored (zero segfaults).
        //     QObject::connect(evtExtractionController,
        //                      &EventExtractionController::finished,
        //                      audioAnnotationSegmentationControllerThread,
        //                      &QThread::quit,
        //                      Qt::QueuedConnection);

        //     QObject::connect(evtExtractionController,
        //                      &EventExtractionController::finished,
        //                      audioTranscriptionSegmentationControllerThread,
        //                      &QThread::quit,
        //                      Qt::QueuedConnection);

        //     QObject::connect(evtExtractionController,
        //                      &EventExtractionController::finished,
        //                      audioSegmentationControllerThread,
        //                      &QThread::quit,
        //                      Qt::QueuedConnection);

        //     QObject::connect(evtExtractionController,
        //                      &EventExtractionController::finished,
        //                      videoSegmentationControllerThread,
        //                      &QThread::quit,
        //                      Qt::QueuedConnection);

        //     QObject::connect(evtExtractionController,
        //                      &EventExtractionController::finished,
        //                      manifestsSynchronizerThread,
        //                      &QThread::quit,
        //                      Qt::QueuedConnection);

        //     QObject::connect(evtExtractionController,
        //                      &EventExtractionController::finished,
        //                      evtExtractionControllerThread,
        //                      &QThread::quit,
        //                      Qt::QueuedConnection);

        //     // 2. Schedule the next round
        //     QObject::connect(evtExtractionControllerThread,
        //                      &QThread::finished,
        //                      evtExtractionApp,
        //                      [state,
        //                       runNextPipeline]() {
        //                          qDebug() << "Round processing complete. Asynchronously launching next round...";
        //                          state->currentSlowdownIdx++;

        //                          (*runNextPipeline)();
        //                      },
        //                      Qt::QueuedConnection);

        //     // Fire Execution Threads
        //     audioAnnotationSegmentationControllerThread->start();
        //     audioTranscriptionSegmentationControllerThread->start();
        //     audioSegmentationControllerThread->start();
        //     videoSegmentationControllerThread->start();
        //     manifestsSynchronizerThread->start();
        //     evtExtractionControllerThread->start();

        //     // Kickstart the workers with their inputs directly (skipping obsolete Transcode dependencies)
        //     FileInfo audioFileInfo(cmdOptions->fileInfo.outAudioMediaUrl.path(), FileType_t::AudioFile);
        //     FileInfo videoFileInfo(cmdOptions->fileInfo.inVideoMediaUrl.path(), FileType_t::VideoFile);

        //     QTimer::singleShot(0,
        //                        audioAnnotationSegmentationController,
        //                        [audioAnnotationSegmentationController,
        //                         state]() {
        //         audioAnnotationSegmentationController->handleInput(state->capturedAnnotationResult);
        //     });

        //     QTimer::singleShot(0,
        //                        audioTranscriptionSegmentationController,
        //                        [audioTranscriptionSegmentationController,
        //                         state]() {
        //         audioTranscriptionSegmentationController->handleInput(state->capturedTranscriptionResult);
        //     });

        //     QTimer::singleShot(0,
        //                        audioSegmentationController,
        //                        [audioSegmentationController,
        //                         audioFileInfo]() {
        //         audioSegmentationController->handleInput(audioFileInfo);
        //     });

        //     QTimer::singleShot(0,
        //                        videoSegmentationController,
        //                        [videoSegmentationController,
        //                         videoFileInfo]() {
        //         videoSegmentationController->handleInput(videoFileInfo);
        //     });
        // };

        // --- PHASE 1: Run Transcoding and Transcription Once Upfront ---
        qDebug() << "======================================================================";
        qDebug() << "🌀 Launching Phase 1: Global Audio Extraction & Text Transcription";
        qDebug() << "======================================================================";

        TranscodeController * transcodeController = new TranscodeController;
        transcodeController->setupAudioTranscoding(cmdOptions->fileInfo.inVideoMediaUrl,
                                                   cmdOptions->fileInfo.outAudioMediaUrl);
        QThread *transcodeControllerThread = new QThread;
        transcodeController->moveToThread(transcodeControllerThread);

        AudioTranscriptionController * transcribeAudioController = new AudioTranscriptionController;
        transcribeAudioController->setModel(cmdOptions->speech2TextModel); //Deepgram_AIS2TT2SModels_t::DeepgramS2TNova2GeneralEN
        transcribeAudioController->setAnnotationDir(cmdOptions->dirInfo.annotation_and_transcriptsDir.absoluteFilePath());

        QThread *transcribeAudioControllerThread = new QThread;
        transcribeAudioController->moveToThread(transcribeAudioControllerThread);

        ConverterController *convertController = new ConverterController;
        convertController->setLineLength(200);
        convertController->setConverterType(cmdOptions->transcriptionConverterType); //ConverterType_t::DeepgramWebVttConverter
        QThread *convertControllerThread = new QThread;
        convertController->moveToThread(convertControllerThread);

        // Initial Preparation Lifecycle management
        registerThreadCleanup(transcodeControllerThread, transcodeController);
        registerThreadCleanup(transcribeAudioControllerThread, transcribeAudioController);
        registerThreadCleanup(convertControllerThread, convertController);

        QObject::connect(transcodeController,
                         &TranscodeController::error,
                         evtExtractionApp,
                         errorHandler);

        QObject::connect(transcribeAudioController,
                         &AudioTranscriptionController::error,
                         evtExtractionApp,
                         errorHandler);

        QObject::connect(convertController,
                         &ConverterController::conversionError,
                         evtExtractionApp,
                         errorHandler);

        // Route Transcode -> Transcription
        QObject::connect(transcodeController,
                         &TranscodeController::finished,
                         transcribeAudioController,
                         &AudioTranscriptionController::handleInput);

        // Route Transcription -> Converter
        QObject::connect(transcribeAudioController,
                         &AudioTranscriptionController::finished,
                         convertController,
                         &ConverterController::handleInput);

        // Once Phase 1 conversion is complete, cache the annotation and transcription files and bridge into Phase 2
        // QObject::connect(convertController,
        //                  &ConverterController::finished,
        //                  evtExtractionApp,
        //                  [state,
        //                   runNextPipeline,
        //                   transcribeAudioController,
        //                   transcribeAudioControllerThread,
        //                   transcodeControllerThread,
        //                   convertControllerThread](FileInfo audioTranscriptionResult) {

        //                      qDebug() << "✨ Master audio transcription complete! Annotation saved to:" << audioTranscriptionResult.url.path();

        //                      FileInfo audioAnnotationResult(transcribeAudioController->audioAnnotationPath(), FileType_t::AnnotationFile);
        //                      state->capturedAnnotationResult = audioAnnotationResult;
        //                      state->capturedTranscriptionResult = audioTranscriptionResult;

        //                      // Safely tear down Phase 1 background threads
        //                      transcodeControllerThread->quit();
        //                      transcodeControllerThread->wait();

        //                      transcribeAudioControllerThread->quit();
        //                      transcribeAudioControllerThread->wait();

        //                      convertControllerThread->quit();
        //                      convertControllerThread->wait();

        //                      // Hand off control to sequential slowdown execution phase
        //                      (*runNextPipeline)();
        //                  });

        QObject::connect(convertController,
                         &ConverterController::finished,
                         evtExtractionApp,
                         [state,
                         evtExtractionApp,
                         cmdOptions,
                         errorHandler,
                         transcribeAudioController,
                         transcribeAudioControllerThread,
                         transcodeControllerThread,
                         convertControllerThread](FileInfo audioTranscriptionResult) {

            qDebug() << "✨ Master audio transcription complete! Annotation saved to:" << audioTranscriptionResult.url.path();

            // Save Phase 1 results into our persistent state container
            state->capturedAnnotationResult = FileInfo(transcribeAudioController->audioAnnotationPath(), FileType_t::AnnotationFile);
            state->capturedTranscriptionResult = audioTranscriptionResult;

            // Tear down Phase 1
            transcodeControllerThread->quit();
            transcodeControllerThread->wait();

            transcribeAudioControllerThread->quit();
            transcribeAudioControllerThread->wait();

            convertControllerThread->quit();
            convertControllerThread->wait();

            // Kick off the first entirely fresh round of Phase 2
            runPipelinePhase2Round(evtExtractionApp, cmdOptions, errorHandler, state);
        });

        // Start Phase 1 threads
        transcodeControllerThread->start();
        transcribeAudioControllerThread->start();
        convertControllerThread->start();

        // Fire the initial transcoding routine
        QTimer::singleShot(0,
                           transcodeController,
                           &TranscodeController::startTranscoding);
    }
}

namespace Experiments {

    // Experiment 01: Validation of Token Consumption Optimization
    /**
     * @brief runBatchTokenEvaluator  Isolated module to validate token consumption optimization of WebVTT vs JSON.
     * @param evtExtractionApp
     * @param cmdOptions
     * @param errorHandler
     */
    void runBatchTokenEvaluator(QApplication * evtExtractionApp,
                                QSharedPointer<CmdLineOptions> cmdOptions,
                                const std::function<void(const QString&)>& errorHandler)
    {
        qDebug() << "======================================================================";
        qDebug() << "📊 Launching Isolated Module: Batch Token Evaluator";
        qDebug() << "======================================================================";

        // 1. Resolve Target Paths
        // This maps the baseline JSON and candidate WebVTT to the standard pipeline folders.
        QString jsonDatasetDirectory = cmdOptions->dirInfo.annotationClipsDir.absoluteFilePath();
        QString vttDatasetDirectory = cmdOptions->dirInfo.transcriptClipsDir.absoluteFilePath();
        QString outputFileReportPath = QDir(cmdOptions->dirInfo.defaultRootDir.absoluteFilePath()).filePath(*Constants::Q_QGS_BATCH_TOKEN_OPTIMIZAION_REPORT);

        qDebug() << "JSON dir path: " << jsonDatasetDirectory;
        qDebug() << "WebVTT dir path: " << vttDatasetDirectory;
        qDebug() << "Output report: " << outputFileReportPath;

        // 2. Validate Input Directories
        if (!QDir(jsonDatasetDirectory).exists() && !QDir(vttDatasetDirectory).exists())
        {
            errorHandler(QString("runBatchTokenEvaluator(...) - Operational Error: Input datasets directories do not exist.\n JSON: %1\n VTT: %2")
                                     .arg(jsonDatasetDirectory,
                                          vttDatasetDirectory));

            return;
        }

        // 3. Safely generate output directory if needed
        QFileInfo outputFileInfo(outputFileReportPath);
        QDir outputDir = outputFileInfo.absoluteDir();

        if (!outputDir.exists() && !outputDir.mkpath("."))
        {
            errorHandler(QString("runBatchTokenEvaluator(...) - Operational Error: Could not create target output directory: %1").arg(outputDir.absolutePath()));

            return;

        } else {
            qDebug() << "Output file dir path exists: " << outputDir.absolutePath();
        }

        // 4. Execute the experiment
        // Note: Because string parsing is computationally fast, this is run synchronously.
        // If the dataset grows to millions of files, this should be moved to a QThread worker.
        BatchTokenEvaluator::runBatchExperiment(jsonDatasetDirectory, vttDatasetDirectory, outputFileReportPath);

        qDebug() << "✅ Batch Token Evaluation complete!";

        // 5. Trigger clean application shutdown
        QMetaObject::invokeMethod(evtExtractionApp, "quit", Qt::QueuedConnection);
    }

    // Experimetn 02: Validation of Hallucination Mitigation & Constraint Enforcement

    /**
     * @brief The ExperimentContext class Define a state context to track the experiment phase
     */
    struct ExperimentContext {
        bool isBaselineRound = true;
        qint32 currentIteration = 1;
        const qint32 MAX_ITERATIONS = 5;
    };

    /**
     * @brief runExperimentRound  Stateless recursive helper to process the Baseline, followed by the Governed round.
     * @param evtExtractionApp
     * @param cmdOptions
     * @param errorHandler
     * @param state
     */
    void runExperimentRound(QApplication * evtExtractionApp,
                            QSharedPointer<CmdLineOptions> cmdOptions,
                            const std::function<void(const QString&)>& errorHandler,
                            QSharedPointer<ExperimentContext> state)
    {
        state->isBaselineRound = false;
        QString runType = state->isBaselineRound ? "Baseline" : "Governed";

        qDebug() << "============================================";
        qDebug() << "🧪 Launching Experiment Phase:" << runType;
        qDebug() << "============================================";

        EventExtractionController * evtExtractionController = new EventExtractionController;
        evtExtractionController->setupModel(cmdOptions->aiModelType);
        evtExtractionController->addTermsForSearching(cmdOptions->searchedTerms);
        evtExtractionController->setSearchType(cmdOptions->searchType);
        evtExtractionController->setMinConfidenceThreshold(cmdOptions->confidenceThreshold);
        evtExtractionController->setCurrentIteration(state->currentIteration);
        evtExtractionController->setTotalIterations(state->MAX_ITERATIONS);

        // Dynamically name the output file based on the current round
        auto const &cfg = AIModelMap[cmdOptions->aiModelType];
        QString markdownReportPath = QDir(cmdOptions->dirInfo.defaultRootDir.absoluteFilePath())
                                         .filePath(QString("%1-%2_Analysis.md").arg(cfg.name, runType));
        evtExtractionController->setOutputFile(markdownReportPath);

        // Configure specifics based on the round type
        if (state->isBaselineRound)
        {
            evtExtractionController->setSystemPrompt("");
            evtExtractionController->setTaskTemplateType(TaskTemplate::Video_Baseline_Analysis_Task);
        }
        else
        {
            QString sysRefereePrompt = PromptManager::instance().getSystemProfile(Persona::FIFA_Referee);
            evtExtractionController->setSystemPrompt(sysRefereePrompt);
            evtExtractionController->setTaskTemplateType(TaskTemplate::Video_Analysis_Task);
        }

        // --- Safe Thread Instantiation (No Parents) ---
        QThread * evtExtractionControllerThread = new QThread;
        evtExtractionController->moveToThread(evtExtractionControllerThread);

        // --- Cleanup & Signal Routing ---
        registerThreadCleanup(evtExtractionControllerThread, evtExtractionController);

        QObject::connect(evtExtractionController,
                         &EventExtractionController::error,
                         evtExtractionApp,
                         errorHandler);

        const int safeIterationRecord = state->currentIteration;
        const int maxIters = state->MAX_ITERATIONS;

        QObject::connect(evtExtractionController,
                         &EventExtractionController::finished,
                         evtExtractionApp,
                         [runType,
                         safeIterationRecord,
                         maxIters](const FileInfo& result) {

                             qDebug() << "================================================================================";
                             qDebug() << "✅ " << runType << " experiment round " << safeIterationRecord << "/" << maxIters <<
                                         " completed successfully. Running next round.";
                             qDebug() << "✅ Output saved to:" << result.url.path();
                             qDebug() << "================================================================================";
                         });

        // 1. Thread Teardown Routing
        QObject::connect(evtExtractionController,
                         &EventExtractionController::finished,
                         evtExtractionControllerThread,
                         &QThread::quit,
                         Qt::QueuedConnection);

        // 2. ✨ THE SEQUENTIAL EXECUTION TRIGGER ✨
        QObject::connect(evtExtractionControllerThread,
                         &QThread::finished,
                         evtExtractionApp,
                         [evtExtractionApp,
                         cmdOptions,
                         errorHandler,
                         state,
                         runType]() {

            if (state->currentIteration < state->MAX_ITERATIONS)
            {
                state->currentIteration++;

                runExperimentRound(evtExtractionApp, cmdOptions, errorHandler, state);
            }
            else
            {
                if (state->isBaselineRound)
                {
                    state->currentIteration = 1;

                    qDebug() << "Baseline round thread teardown complete. Launching Governed round...";
                    state->isBaselineRound = false;

                    runExperimentRound(evtExtractionApp, cmdOptions, errorHandler, state);
                }
                else
                {
                    qDebug() << "=================================================================";
                    qDebug() << "✅ Both experiment rounds completed successfully. Exiting clean.";
                    qDebug() << "=================================================================";

                    QMetaObject::invokeMethod(evtExtractionApp, "quit", Qt::QueuedConnection);
                }
            }
        },
        Qt::QueuedConnection);

        evtExtractionControllerThread->start();

        // Feed input to kickstart worker
        FileInfo inputSyncedManifestsPath(cmdOptions->syncedManifestsJsonFile, FileType_t::SynchronizedManifest);

        QTimer::singleShot(0,
                           evtExtractionController,
                           [evtExtractionController,
                           inputSyncedManifestsPath]() {
                               evtExtractionController->handleInput(inputSyncedManifestsPath);
                           });
    }

    /**
     * @brief runGovernanceComplianceAudit Isolated module to evaluate Hallucination Mitigation & Constraint Enforcement (Experiment 2)
     *                                     by auditing Baseline vs Governed Markdown outputs.
     * @param evtExtractionApp
     * @param cmdOptions
     * @param errorHandler
     */
    void runGovernanceComplianceAudit(QApplication * evtExtractionApp,
                                      QSharedPointer<CmdLineOptions> cmdOptions,
                                      const std::function<void(const QString&)>& errorHandler)
    {
        qDebug() << "===========================================================================";
        qDebug() << "⚖️ Launching Isolated Module: Governance & Compliance Audit (Experiment 2) ";
        qDebug() << "===========================================================================";

        // 1. Resolve Target Paths dynamically based on the target directory workspace (-td)
        // These names strictly match the outputs generated by runBaselineAndGovernedExperiment.
        QDir targetDir(cmdOptions->dirInfo.defaultRootDir.absoluteFilePath());

        const auto &cfg = AIModelMap[cmdOptions->aiModelType];

        // // 1. Define wildcard search filters matching partial file names
        // QStringList controlFilters  = { QString("%1-Baseline_Analysis*.md").arg(cfg.name) };
        // QStringList governedFilters = { QString("%1-Governed_Analysis*.md").arg(cfg.name) };

        // // 2. Query targetDir for matching files
        // QStringList controlMatches  = targetDir.entryList(controlFilters, QDir::Files);
        // QStringList governedMatches = targetDir.entryList(governedFilters, QDir::Files);

        // 3. Resolve the full file paths (takes the first matching file if found)
        // QString controlFilePath  = !controlMatches.isEmpty()  ? targetDir.filePath(controlMatches.first())  : QString();
        // QString governedFilePath = !governedMatches.isEmpty() ? targetDir.filePath(governedMatches.first()) : QString();

        QString controlFilePath  = cmdOptions->baselineMarkdownFile;
        QString governedFilePath = cmdOptions->governedMarkdownFile;

        // // QString controlFilePath = targetDir.filePath(QString("%1-Baseline_Analysis.md").arg(cfg.name));
        // // QString governedFilePath = targetDir.filePath(QString("%1-Governed_Analysis.md").arg(cfg.name));
        // QString destinationReport = QString("%1-%2").arg(cfg.name, *Constants::Q_QGS_COMPLIANCE_AUDIT_REPORT);
        // QString destinationReportPath = targetDir.filePath(destinationReport);

        QString destinationReport = QString("%1-%2").arg(cfg.name,
                                                         *Constants::Q_QGS_COMPLIANCE_AUDIT_REPORT);
        QString destinationReportPath = targetDir.filePath(destinationReport);

        qDebug() << "Baseline Source: " << controlFilePath;
        qDebug() << "Governed Source: " << governedFilePath;
        qDebug() << "Target Report:   " << destinationReportPath;

        // 2. Validate Input Files
        if (!QFile::exists(controlFilePath) || !QFile::exists(governedFilePath))
        {
            errorHandler(QString("runGovernanceComplianceAudit(...) - Operational Error: Missing input markdown reports.\n Baseline: %1\n Governed: %2\n Ensure"
                                 " 'runBaselineAndGovernedExperiment' was successfully executed first in this target directory.")
                         .arg(controlFilePath, governedFilePath));

            return;
        }

        // 3. Extract payloads from the raw markdown files.
        qDebug() << "Parsing baseline payloads...";
        QVector<Payload> controlGroupPayloads = GovernanceAuditHarness::loadPayloadsFromFile(controlFilePath, false);

        qDebug() << "Parsing governed payloads...";
        QVector<Payload> governedGroupPayloads = GovernanceAuditHarness::loadPayloadsFromFile(governedFilePath, false);

        // Safety check to ensure files were found and parsed correctly
        if (controlGroupPayloads.isEmpty() || governedGroupPayloads.isEmpty())
        {
            errorHandler("runGovernanceComplianceAudit(...) - Data Error: One or both payload groups failed to load or returned 0 payloads. Check markdown"
                         " header formatting.");

            return;
        }

        // 4. Run Governance Compliance Evaluation Pipeline
        qDebug() << "Auditing baseline metrics...";
        AuditMetrics controlAudit = GovernanceAuditHarness::auditBaselineOutputs(controlGroupPayloads);
        qDebug() << "Auditing governed metrics...";
        AuditMetrics governedAudit = GovernanceAuditHarness::auditGovernedOutputs(governedGroupPayloads);

        // 5. Generate and Persist Report
        qDebug() << "Generating final compliance report...";
        GovernanceAuditHarness::generateAuditReport(controlAudit, governedAudit, destinationReportPath);

        qDebug() << "✅ Governance & Compliance Audit complete!";

        // 6. Trigger clean application shutdown
        QMetaObject::invokeMethod(evtExtractionApp, "quit", Qt::QueuedConnection);

    }

    // Experiment 3: Precise Event Spotting & Clock-Domain Alignment.

    /**
     * @brief runTemporalAlignmentAudit  Isolated module to validate Clock-Domain Alignment and Precise Event Spotting (Experiment 3)
     * @param evtExtractionApp
     * @param cmdOptions
     * @param errorHandler
     */
    void runTemporalAlignmentAudit(QApplication * evtExtractionApp,
                                   QSharedPointer<CmdLineOptions> cmdOptions,
                                   const std::function<void(const QString&)>& errorHandler)
    {
        // Get governed (model response) file
        QString governedFilePath = cmdOptions->governedMarkdownFile;

        // Get ground truth CSV file
        QString csvFilePath = cmdOptions->groundTruthFile;

        // We audit the governed analysis file generated by earlier pipeline runs
        const auto &cfg = AIModelMap[cmdOptions->aiModelType];

        QString destinationReport = QString("%1-%2").arg(cfg.name,
                                                         *Constants::Q_QGS_TEMPORAL_ALIGNMENT_AUDIT_REPORT);

        // Resolve Target Paths dynamically
        QDir targetDir(cmdOptions->dirInfo.defaultRootDir.absoluteFilePath());

        QString destinationReportPath = targetDir.filePath(destinationReport);

        qDebug() << "Ground Truth CSV: " << csvFilePath;
        qDebug() << "Governed Source: " << governedFilePath;
        qDebug() << "Target Report:    " << destinationReportPath;

        // 1. Pre-Execution Safety Checks
        if (!QFile::exists(governedFilePath))
        {
            errorHandler(QString("runTemporalAlignmentAudit(...) - Operational Error: Missing input markdown report.\n Path: %1\n Ensure the extraction pipeline was successfully executed first.").arg(governedFilePath));

            return;
        }

        if (!QFile::exists(csvFilePath))
        {
            errorHandler(QString("runTemporalAlignmentAudit(...) - Operational Error: Missing ground truth CSV.\n Path: %1").arg(csvFilePath));
            return;
        }


        // 2. Load Ground Truth Data (Multi-Event Supported)
        qDebug() << "Loading Ground Truth Data from CSV...";

        GroundTruthMap groundTruthMap = TemporalAlignmentHarness::loadGroundTruthData(csvFilePath);

        if (groundTruthMap.isEmpty())
        {
            errorHandler("runTemporalAlignmentAudit(...) - Data Error: Ground truth map failed to load or is empty.");
            return;
        }

        // --- 3. Load & Match Payloads ---
        qDebug() << "Parsing governed payloads and attaching ground truth...";
        QVector<TemporalPayload> governedPayloads = TemporalAlignmentHarness::loadPayloadsFromFile(governedFilePath, groundTruthMap);

        if (governedPayloads.isEmpty())
        {
            errorHandler("runTemporalAlignmentAudit(...) - Data Error: Payload group failed to load or returned 0 payloads.");

            return;
        }

        // 4. Execute Alignment Audit
        qDebug() << "Auditing temporal alignments...";
        QVector<AlignmentResult> auditResults = TemporalAlignmentHarness::evaluateClockDomainAlignment(governedPayloads, cmdOptions->temporalTolerance);

        // 5. Generate Report
        qDebug() << "Generating final temporal alignment report...";
        TemporalAlignmentHarness::generateAlignmentReport(auditResults, cmdOptions->temporalTolerance, destinationReportPath);

        qDebug() << "✅ Temporal Alignment Audit complete!";

        // Trigger clean application shutdown
        QMetaObject::invokeMethod(evtExtractionApp, "quit", Qt::QueuedConnection);
    }

    void runReportEventsFoundOnGoverned(QApplication * evtExtractionApp,
                                        QSharedPointer<CmdLineOptions> cmdOptions,
                                        const std::function<void(const QString&)>& errorHandler)
    {
        qDebug() << "===========================================================================";
        qDebug() << "⚖️ Launching Isolated Module: Events Found On Governed      (Experiment 2) ";
        qDebug() << "===========================================================================";

        // 1. Resolve Target Paths dynamically based on the target directory workspace (-td)
        // These names strictly match the outputs generated by runBaselineAndGovernedExperiment.
        QDir targetDir(cmdOptions->dirInfo.defaultRootDir.absoluteFilePath());

        // const auto &cfg = AIModelMap[cmdOptions->aiModelType];

        // // 1. Define wildcard search filters matching partial file names
        // QStringList governedFilters = { QString("%1-Governed_Analysis*.md").arg(cfg.name) };

        // // 2. Query targetDir for matching files
        // QStringList governedMatches = targetDir.entryList(governedFilters, QDir::Files);

        // // 3. Resolve the full file paths (takes the first matching file if found)
        // QString governedFilePath = !governedMatches.isEmpty() ? targetDir.filePath(governedMatches.first()) : QString();

        QString governedFilePath = cmdOptions->governedMarkdownFile;

        // 2. Validate Input Files
        if (!QFile::exists(governedFilePath))
        {
            errorHandler(QString("runReportEventsFoundOnGoverned(...) - Operational Error: Missing input markdown reports.\n Governed: %1.")
                         .arg(governedFilePath));

            return;
        }

        // 1. Load your payloads from dataset file
        QVector<Payload> payloads = GovernanceAuditHarness::loadPayloadsFromFile(governedFilePath, true);

        // 2. Generate and print the Markdown table for a specific segment
        if (payloads.isEmpty())
        {
            qWarning() << "[Report Generator] No payloads could be extracted from:" << governedFilePath;
        }
        else
        {
            // 4. Generate the Complete Batch Table
            QString markdownTable = GovernanceAuditHarness::generateEventsMarkdownTableFromGoverned(payloads);

            // 5. Print to Console
            QTextStream console(stdout);
            console << markdownTable;
            console.flush();

            QFileInfo governedFile(governedFilePath);

            QString destinationReport = QString("%1-%2").arg(governedFile.fileName().remove(governedFile.fileName().size()-3, 3),
                                                             "ReportEventsFoundInGoverned.txt");
            QString destinationReportPath = targetDir.filePath(destinationReport);

            qDebug() << "Governed Source: " << governedFilePath;
            qDebug() << "Target Report:   " << destinationReportPath;

            // 6. Persist to Destination File
            QFile outFile(destinationReportPath);
            if (outFile.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                QTextStream fileStream(&outFile);
                fileStream.setCodec("UTF-8");
                fileStream << markdownTable;
                outFile.close();

                qDebug() << "\n[System] Events report saved successfully to:" << destinationReportPath;
            }
            else
            {
                qCritical() << "\n[System Error] Failed to write report to:" << destinationReportPath;
            }
        }

        // 6. Trigger clean application shutdown
        QMetaObject::invokeMethod(evtExtractionApp, "quit", Qt::QueuedConnection);
    }

    void runReportEventsRejectedOnGoverned(QApplication * evtExtractionApp,
                                           QSharedPointer<CmdLineOptions> cmdOptions,
                                           const std::function<void(const QString&)>& errorHandler)
    {
        qDebug() << "===========================================================================";
        qDebug() << "⚖️ Launching Isolated Module: Events Rejected On Governed   (Experiment 2) ";
        qDebug() << "===========================================================================";

        // 1. Resolve Target Paths dynamically based on the target directory workspace (-td)
        // These names strictly match the outputs generated by runBaselineAndGovernedExperiment.
        QDir targetDir(cmdOptions->dirInfo.defaultRootDir.absoluteFilePath());

        // const auto &cfg = AIModelMap[cmdOptions->aiModelType];

        // // 1. Define wildcard search filters matching partial file names
        // QStringList governedFilters = { QString("%1-Governed_Analysis*.md").arg(cfg.name) };

        // // 2. Query targetDir for matching files
        // QStringList governedMatches = targetDir.entryList(governedFilters, QDir::Files);

        // 3. Resolve the full file paths (takes the first matching file if found)
        // QString governedFilePath = !governedMatches.isEmpty() ? targetDir.filePath(governedMatches.first()) : QString();

        QString governedFilePath = cmdOptions->governedMarkdownFile;

        // 2. Validate Input Files
        if (!QFile::exists(governedFilePath))
        {
            errorHandler(QString("runReportEventsFoundOnGoverned(...) - Operational Error: Missing input markdown reports.\n Governed: %1.")
                         .arg(governedFilePath));

            return;
        }

        // 1. Load your payloads from dataset file
        QVector<Payload> payloads = GovernanceAuditHarness::loadPayloadsFromFile(governedFilePath, true);

        // 2. Generate and print the Markdown table for a specific segment
        if (payloads.isEmpty())
        {
            qWarning() << "[Report Generator] No payloads could be extracted from:" << governedFilePath;
        }
        else
        {
            // 3. Generate the Complete Batch Table
            QString markdownTable = GovernanceAuditHarness::generateRejectedVttEventsTableFromGoverned(payloads);

            // 4. Print to Console
            QTextStream console(stdout);
            console << markdownTable;
            console.flush();

            QFileInfo governedFile(governedFilePath);

            QString destinationReport = QString("%1-%2").arg(governedFile.fileName().remove(governedFile.fileName().size()-3, 3),
                                                             "ReportEventsRejectedOnGoverned.txt");
            QString destinationReportPath = targetDir.filePath(destinationReport);

            qDebug() << "Governed Source: " << governedFilePath;
            qDebug() << "Target Report:   " << destinationReportPath;

            // 5. Persist to Destination File
            QFile outFile(destinationReportPath);


            if (outFile.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                QTextStream fileStream(&outFile);
                fileStream.setCodec("UTF-8");
                fileStream << markdownTable;
                outFile.close();

                qDebug() << "\n[System] Events report saved successfully to:" << destinationReportPath;
            }
            else
            {
                qCritical() << "\n[System Error] Failed to write report to:" << destinationReportPath;
            }
        }

        // 6. Trigger clean application shutdown
        QMetaObject::invokeMethod(evtExtractionApp, "quit", Qt::QueuedConnection);
    }

    void runPrintReportOnGoverned(QApplication * evtExtractionApp,
                                  QSharedPointer<CmdLineOptions> cmdOptions,
                                  const std::function<void(const QString&)>& errorHandler)
    {
        qDebug() << "===========================================================================";
        qDebug() << "⚖️ Launching Isolated Module: Print Report On Governed   (Experiment 2)    ";
        qDebug() << "===========================================================================";

        // 1. Resolve Target Paths dynamically based on the target directory workspace (-td)
        // These names strictly match the outputs generated by runBaselineAndGovernedExperiment.
        QDir targetDir(cmdOptions->dirInfo.defaultRootDir.absoluteFilePath());

        // const auto &cfg = AIModelMap[cmdOptions->aiModelType];

        // // 1. Define wildcard search filters matching partial file names
        // QStringList governedFilters = { QString("%1-Governed_Analysis*.md").arg(cfg.name) };

        // // 2. Query targetDir for matching files
        // QStringList governedMatches = targetDir.entryList(governedFilters, QDir::Files);

        // // 3. Resolve the full file paths (takes the first matching file if found)
        // QString governedFilePath = !governedMatches.isEmpty() ? targetDir.filePath(governedMatches.first()) : QString();

        QString governedFilePath = cmdOptions->governedMarkdownFile;

        // 2. Validate Input Files
        if (!QFile::exists(governedFilePath))
        {
            errorHandler(QString("runPrintReportOnGoverned(...) - Operational Error: Missing input markdown reports.\n Governed: %1.")
                         .arg(governedFilePath));

            return;
        }

        // 1. Load your payloads from dataset file
        QVector<Payload> payloads = GovernanceAuditHarness::loadPayloadsFromFile(governedFilePath, true);

        // 2. Generate and print the Markdown table for a specific segment
        if (payloads.isEmpty())
        {
            qWarning() << "[Report Generator] No payloads could be extracted from:" << governedFilePath;
        }
        else
        {
            // 3. Generate the Complete Report
            QString report = GovernanceAuditHarness::printReport(payloads);

            // 4. Print to Console
            QTextStream console(stdout);
            console << report;
            console.flush();

            QFileInfo governedFile(governedFilePath);

            QString destinationReport = QString("%1-%2").arg(governedFile.fileName().remove(governedFile.fileName().size()-3, 3),
                                                             "ReportEventsOnGoverned.txt");
            QString destinationReportPath = targetDir.filePath(destinationReport);

            qDebug() << "Governed Source: " << governedFilePath;
            qDebug() << "Target Report:   " << destinationReportPath;

            // 4. Persist to Destination File
            QFile outFile(destinationReportPath);
            if (outFile.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                QTextStream fileStream(&outFile);
                fileStream.setCodec("UTF-8");
                fileStream << report;
                outFile.close();

                qDebug() << "\n[System] Events report saved successfully to:" << destinationReportPath;
            }
            else
            {
                qCritical() << "\n[System Error] Failed to write report to:" << destinationReportPath;
            }
        }

        // 6. Trigger clean application shutdown
        QMetaObject::invokeMethod(evtExtractionApp, "quit", Qt::QueuedConnection);

    }
}

int main(int argc, char *argv[])
{
    QApplication EventExtractorApp(argc, argv);

#ifdef BUILD_TESTS
    if (argc > 1 && QString(argv[1]) == "--test") {
        int status = 0;
        UploadManagerTest uploadTest;
        status |= QTest::qExec(&uploadTest, argc, argv);

        VideoSegmentaInfoTest videoTest;
        status |= QTest::qExec(&videoTest, argc, argv);

        return status;
    }
#endif

    // Force Qt to keep the event loop running even with no windows open
    EventExtractorApp.setQuitOnLastWindowClosed(false);

    // Register custom MetaTypes for safe cross-thread Signal/Slot routing
    qRegisterMetaType<utils::UniqueId>("UniqueId");
    qRegisterMetaType<FileInfo>("FileInfo");
    qRegisterMetaType<ClipData>("ClipData");
    qRegisterMetaType<SegmentsWithPrompts>("SegmentsWithPrompts");

#ifdef QT_DEBUG
    LogUtils::enableLog();
#endif

    if (argc <= 0 || argv == nullptr)
    {
        qCritical() << "Error: Invalid low-level runtime command line arguments context system.";
        return -1;
    }

    // Internationalization / Localization Setup
    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();

    for (const QString &locale : uiLanguages)
    {
        const QString baseName = "EventExtractorApp_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName))
        {
            EventExtractorApp.installTranslator(&translator);
            break;
        }
    }

    // 1. Argument Parsing Phase
    // Note: Qt arguments include the program name at index 0, matching sys.argv
    QStringList args = EventExtractorApp.arguments();
    CmdArgumentsVec parsedOptions = ExtractorUtils::commandLineParser(args);

    auto cmdOptions = QSharedPointer<CmdLineOptions>::create();

    if (ExtractorUtils::initCmdOptions(parsedOptions, cmdOptions.data()) < 0)
    {
        qCritical() << "Initialization Error: Failed to initialize pipeline configuration options.";

        return -2;
    }

//     // Semantic Field Validation Guard
//     if (cmdOptions->fileInfo.inVideoMediaUrl.isEmpty() || cmdOptions->dirInfo.defaultRootDir.absoluteFilePath().isEmpty())
//     {
//         qCritical().noquote() << "Usage Error: Missing mandatory fields.\n"
//                               << "Required: -i <input_video> -td <target_dir> -af <audio_format>...";
// #ifdef QT_DEBUG
//         LogUtils::safeLog(QString("EventExtractorApp::main(ing, char**) - Missing mandatory files!"),
//                           __FILE__,
//                           __LINE__,
//                           Q_FUNC_INFO);
// #endif
//         return -3;
//     }

// #ifdef QT_DEBUG
//     if (cmdOptions->fileInfo.outAudioMediaUrl.isLocalFile())
//     {
//         LogUtils::safeLog(QString("EventExtractorApp::main(ing, char**) - Output file (%1): it is local file!")
//                               .arg(cmdOptions->fileInfo.outAudioMediaUrl.path()),
//                               __FILE__,
//                               __LINE__,
//                               Q_FUNC_INFO);
//     } else {
//         LogUtils::safeLog(QString("EventExtractorApp::main(ing, char**) - Output file (%1): it is not local file!")
//                               .arg(cmdOptions->fileInfo.outAudioMediaUrl.path()),
//                               __FILE__,
//                               __LINE__,
//                               Q_FUNC_INFO);
//     }
// #endif


    // Handle early testing switches gracefully
    qint32 testIdx = ExtractorUtils::indexOfCmdArg(CmdFlags_t::Test, parsedOptions);

    if (testIdx >= 0)
    {
        QString testValue = parsedOptions.at(testIdx).value;

        if      ( ! QString::compare(parsedOptions[testIdx].value, "preamble",  Qt::CaseInsensitive) ) { /*testPreamble()*/; }
        else if ( ! QString::compare(parsedOptions[testIdx].value, "message",   Qt::CaseInsensitive) ) { /*testMessage();*/ }
        else if ( ! QString::compare(parsedOptions[testIdx].value, "request",   Qt::CaseInsensitive) ) { /*testRequest();*/ }
        else if ( ! QString::compare(parsedOptions[testIdx].value, "response",  Qt::CaseInsensitive) ) { /*testResponse();*/ }

        return 0;
    }

    // Centralized Pipeline Error Routing Callback
    auto handleError = [&EventExtractorApp](const QString &msg) {
        qCritical() << "Fatal Processing Pipeline Exception Encountered:" << msg;
        EventExtractorApp.exit();
    };

    LogUtils::safeLog(QString("EventExtractor::main(ing, char**) - Option: %1!")
                      .arg(static_cast<double>(cmdOptions->pipelineMode)),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);

    switch (cmdOptions->pipelineMode)
    {
        case PipelineMode_t::TranscodeOnly: // Run only the transcode module
        {
            ModuleRunners::runTranscode(&EventExtractorApp, cmdOptions, handleError);

            break;
        }

        case PipelineMode_t::TranscribeOnly: // Run only the transcribe module
        {
            ModuleRunners::runTranscribeOnly(&EventExtractorApp, cmdOptions, handleError);

            break;
        }

        case PipelineMode_t::ConvertOnly: // Run only the convert module
        {
            ModuleRunners::runConvertOnly(&EventExtractorApp, cmdOptions, handleError);

            break;
        }

        case PipelineMode_t::SegmentOnly: // Run only the segmentation module
        {
            ModuleRunners::runSegmentOnly(&EventExtractorApp, cmdOptions, handleError);

            break;
        }

        case PipelineMode_t::EvtsExtractionOnly: // Run only the events extraction module
        {
            ModuleRunners::runEventsExtractionOnly(&EventExtractorApp, cmdOptions, handleError);

            break;
        }

        case PipelineMode_t::Pipepline: // Run the pipeline
        {
            ModuleRunners::runPipeline(&EventExtractorApp, cmdOptions, handleError);

            break;
        }

        case PipelineMode_t::Experiment:
        {
            // Refactor initCmdOptions(...)

            switch (cmdOptions->experimentOption)
            {
                case ExperimentMode_t::TokenValidation: // Run Validation of Token
                {
                    Experiments::runBatchTokenEvaluator(&EventExtractorApp, cmdOptions, handleError);

                    break;
                }

                case ExperimentMode_t::BaselineGoverned: // Run Generation of baseline and governed (with system profile) responses
                {
                    auto state = QSharedPointer<Experiments::ExperimentContext>::create();

                    Experiments::runExperimentRound(&EventExtractorApp, cmdOptions, handleError, state);

                    break;
                }

                case ExperimentMode_t::GovernanceAudit: // Run Validation of Hallucination Mitigation & Constraint Enforcement
                {
#ifdef QT_DEBUG
                    LogUtils::safeLog(QString("EventExtractorApp::main(ing, char**) - Domain Alignment!"),
                                          __FILE__,
                                          __LINE__,
                                          Q_FUNC_INFO);
#endif
                    Experiments::runGovernanceComplianceAudit(&EventExtractorApp, cmdOptions, handleError);

                    break;
                }

                case ExperimentMode_t::ReportEventsFound:
                {
#ifdef QT_DEBUG
                    LogUtils::safeLog(QString("EventExtractorApp::main(ing, char**) - Report Events Found!"),
                                          __FILE__,
                                          __LINE__,
                                          Q_FUNC_INFO);
#endif

                    Experiments::runReportEventsFoundOnGoverned(&EventExtractorApp, cmdOptions, handleError);

                    break;
                }

                case ExperimentMode_t::ReportEventsRejected:
                {
#ifdef QT_DEBUG
                    LogUtils::safeLog(QString("EventExtractorApp::main(ing, char**) - Report Events Rejected!"),
                                          __FILE__,
                                          __LINE__,
                                          Q_FUNC_INFO);
#endif

                    Experiments::runReportEventsRejectedOnGoverned(&EventExtractorApp, cmdOptions, handleError);

                    break;
                }

                case ExperimentMode_t::PrintReportGoverned:
                {
#ifdef QT_DEBUG
                    LogUtils::safeLog(QString("EventExtractorApp::main(ing, char**) - Print Report Governed!"),
                                          __FILE__,
                                          __LINE__,
                                          Q_FUNC_INFO);
#endif
                    Experiments::runPrintReportOnGoverned(&EventExtractorApp, cmdOptions, handleError);

                    break;
                }

                case ExperimentMode_t::EventSpottingDomainAlignment: // Run precise Event Spotting & Clock-Domain Alignment
                {
                    Experiments::runTemporalAlignmentAudit(&EventExtractorApp, cmdOptions, handleError);
                    break;
                }
            }

            break;
        }

        case PipelineMode_t::None:
        {
            break;
        }
    }

    qint32 result = EventExtractorApp.exec();
    qDebug() << "System runtime lifecycle closed smoothly.";

    return result;
}
