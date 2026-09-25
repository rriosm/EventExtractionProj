#ifndef COMMANDLINEDEFS_H
#define COMMANDLINEDEFS_H

#include "inc/Defs.h"
// #include "inc/KeywordSearchEngine.h"

#include "Url.h"
using namespace utils;

#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QVector>

#ifndef CMD_FLAGS_DEFINED
#define CMD_FLAGS_DEFINED
enum class CmdFlags_e
{
    None = 0X4002000,       //

    PipelineMode,           // transcode_only; transcribe_only; segment_only; events_extraction_only; pipeline
    OutputFormat,           //
    SlowdownFactors,        //
    ClipDuration,           //
    ConfidenceThreshould,   //
    TranscriptionConverter, //
    AIModelType,            // AI model type
    S2TModelType,           // Speech2Text model type
    SystemProfile,          // AI System Profile
    TaskAnalysis,           // AI Task Analysis
    VideoSegmentationModel,  // Video Segmentation Model

    SearchType,             //
    SearchedTerms,          //

    // Dirs
    TargetDir,              // target dir where the program must save the pipeline's results

    // Files
    AnnotationFile,         // annotation with the audio transcription from AI model trancription
    AudioFile,              // audio file only
    InputFile,              // video+audio file
    SyncedManifestsFile,    // JSON synced manifests file
    TranscriptionFile,      // transcription file
    VideoFile,              // video file only, no audio

    ExperimentMode,         // experiment

    BaselineMarkdown,
    GovernedMarkdown,
    GroundTruth,

    TemporalTolerance,      // temporal tolerance for experiment clock-domain alignment

    Test                    //
};

typedef CmdFlags_e CmdFlags_t;

Q_DECLARE_METATYPE(CmdFlags_e)

inline bool operator==(const CmdFlags_t type1, const CmdFlags_t type2)
{
    return (static_cast<quint64> (type1) == static_cast<quint64> (type2));
}

inline uint qHash(const CmdFlags_t type, uint seed = 0)
{
    return (::qHash(static_cast<quint64> (type), seed));
}
#endif

using CmdArgumentOld = QPair<CmdFlags_t, QString>;

struct CmdArgument_st {
    CmdFlags_t flag;
    QString value;

    // Default values for ALL arguments make this a default constructor
    CmdArgument_st(CmdFlags_t f = CmdFlags_t::None, QString v = "")
        : flag(f), value(v) {}
};

Q_DECLARE_METATYPE(CmdArgument_st)

using CmdArgumentsVec = QVector<CmdArgument_st>;

struct CmdLineOptions
{
    struct FileInfo {
        Url inVideoMediaUrl;
        Url outAudioMediaUrl;
        QString outputAudioFormat;

    } fileInfo;

    struct WebVTTSRT {
        QString srtConverterCmd;
        QStringList srtConverterParams;

    } webVTTSRT;

    struct DirInfo {
        QFileInfo defaultRootDir;
        QFileInfo annotationClipsDir;
        QFileInfo annotation_and_transcriptsDir;
        QFileInfo transcriptClipsDir;
        QFileInfo audioDir;
        QFileInfo audioClipsDir;
        QFileInfo videoDir;
        QFileInfo videoClipsDir;

        DirInfo() {
            defaultRootDir = QFileInfo(*Constants::Q_QGS_DEFAULT_ROOT_DIRECTORY);
            QString root = defaultRootDir.absoluteFilePath();

            // transcriptsDir = QFileInfo(defaultRootDir.absoluteFilePath() + QDir::separator() + "transcription");
            annotation_and_transcriptsDir = QFileInfo(QDir(root).filePath("annotation_and_transcripts"));
            transcriptClipsDir = QFileInfo(QDir(annotation_and_transcriptsDir.absoluteFilePath()).filePath("transcriptClips"));
            annotationClipsDir = QFileInfo(QDir(annotation_and_transcriptsDir.absoluteFilePath()).filePath("annotationClips"));

            // audioDir = QFileInfo(*DEFAULT_ROOT_DIRECTORY + QDir::separator()+"audio");
            audioDir = QFileInfo(QDir(root).filePath("audio"));
            // audioClipsDir = QFileInfo(*DEFAULT_ROOT_DIRECTORY + QDir::separator()+"audio/audioClips");
            audioClipsDir = QFileInfo(QDir(audioDir.absoluteFilePath()).filePath("audioClips"));

            // videoDir = QFileInfo(*DEFAULT_ROOT_DIRECTORY + QDir::separator() + "video");
            videoDir = QFileInfo(QDir(root).filePath("video"));
            // videoClipsDir = QFileInfo(*DEFAULT_ROOT_DIRECTORY + QDir::separator()+"videoClips");
            videoClipsDir = QFileInfo(QDir(videoDir.absoluteFilePath()).filePath("videoClips"));
        };

    } dirInfo;

    PipelineMode_t pipelineMode = PipelineMode_t::None;                             // The pipeline module option: transcode_only; transcribe_only; segment_only; events_extraction_only; pipeline

    QVector<double> slowdownFactors;                                                // Slowdown Factor value

    qreal clipDuration = *Constants::Q_QGS_DEFAULT_CLIP_SEC;                        // Clip Duration in seconds

    qreal confidenceThreshold = *Constants::Q_QGS_CONFIDENCE_THRESHOLD;             // Confidence Threshold value

    ConverterType_t transcriptionConverterType = ConverterType_t::None;             // Transcription Converstion type

    AIModels_t aiModelType = AIModels_t::None;                                      // AI Model type

    Deepgram_AIS2TT2SModels_t speech2TextModel = Deepgram_AIS2TT2SModels_t::NONE;   // Speech-2-Text AI model type

    Persona systemProfile = Persona::None;                                          // System Profile type

    TaskTemplate taskAnalysis = TaskTemplate::None;                                 // Task Analysis type

    ModelCode_t videoSegmentationModel = ModelCode_t::None;                         // Video Segmentation model

    KeywordMatchType_t searchType = KeywordMatchType_t::None;                       // Keyword search type

    QStringList searchedTerms;                                                      // Searched terms

    QString annotationFile;                                                         // JSON transcription file

    QString audioFile;                                                              // Audio only file

    QString inputVideoFile;                                                         // Video+Audio file

    QString transcriptionFile;                                                      // WebVTT file

    QString syncedManifestsJsonFile;                                                // Synchronized Manifest JSON file

    QString videoFile;                                                              // Video only file

    QString baselineMarkdownFile;                                                   // Baseline Markdown file. No System Profile and no elaborated user prompt.

    QString governedMarkdownFile;                                                   // Governed Markdown file. Elaborated System Profile and User Prompt.

    QString groundTruthFile;                                                        // Ground Truth file. Selected video IDs, real timestamps, dilated timestamps, and event categories.

    ExperimentMode_t experimentOption = ExperimentMode_t::None;                     // The pipeline experiment option: transcode_only; transcribe_only; segment_only; events_extraction_only; pipeline

    qint32 temporalTolerance = 5;                                                   // Temporal tolerance int value in seconds for verify clock-domain alignment.
};

Q_DECLARE_METATYPE(CmdLineOptions)

#endif // COMMANDLINEDEFS_H
