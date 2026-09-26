
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
#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QThread>
#include <QTime>
#include <QTimer>

#include "UniqueId.h"
using namespace utils;

#include "inc/TranscodeController.h"
#include "inc/AudioTranscriptionController.h"
#include "inc/ConverterController.h"
#include "inc/AudioAnnotationSegmentationController.h"
#include "inc/AudioTranscriptionSegmentationController.h"
#include "inc/AudioSegmentationController.h"
#include "inc/VideoSegmentationController.h"
#include "inc/ManifestSynchronizer.h"

#include "inc/EventExtractionController.h"

#include "inc/ExtractorUtils.h"
#include "inc/LogUtils.h"
#include "inc/BatchTokenEvaluator.h"
#include "inc/GovernanceAuditHarness.h"
#include "inc/TemporalAlignmentHarness.h"

// #include "tests/inc/AIModelsTest.h"

// AI Prompts
#include "inc/PromptManager.h"

#include <functional>

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

                case ExperimentMode_t::None:
                {
                    //
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
