#ifndef DEFS_H
#define DEFS_H

#include <QCoreApplication>
#include <QObject>
#include <QDir>
#include <QFile>
#include <QHash>
#include <QSettings>
#include <QStandardPaths>

#include <QTemporaryDir>
#include <QTemporaryFile>

#include <QTextStream>

#include "Url.h"
using namespace utils;

inline QString loadFromFileDefs(const QString &path)
{
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return "";

    return QTextStream(&file).readAll();
}

inline QString loadSecretKey(const QString &section, const QString &keyName, const QString &defaultValue = QString())
{
    const QString settingPath = section.isEmpty() ? keyName : (section + "/" + keyName);

    const QStringList searchPaths = {
        QDir::current().filePath("secrets.ini"),
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/secrets.ini",
        QDir::home().filePath(".config/EventExtractorApp/secrets.ini"),
        QCoreApplication::applicationDirPath() + "/secrets.ini"
    };

    for (const QString &path : searchPaths)
    {
        if (QFile::exists(path))
        {
            QSettings settings(path, QSettings::IniFormat);

            const QString val = settings.value(settingPath, defaultValue).toString().trimmed();

            if (!val.isEmpty() && !val.startsWith("YOUR_"))
            {
                return val;
            }
        }
    }

    return defaultValue;
}

/**
 * Supress unused variable warning
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"

namespace Constants {

    Q_GLOBAL_STATIC_WITH_ARGS(const Url,        Q_QGS_DeepgramT2SAura2ThaliaEN_Url,     ("https://api.deepgram.com/v1/speak?model=aura-2-thalia-en"));

    Q_GLOBAL_STATIC_WITH_ARGS(const qreal,      Q_QGS_DeepgramUtterance,                (0.8)); // seconds

    Q_GLOBAL_STATIC_WITH_ARGS(const Url,        Q_QGS_DeepgramS2TNova_Url,              ("https://api.deepgram.com/v1/listen?model=nova&smart_format=true&language=pt-BR&diarize=true&punctuate=true&utterances=true&utt_split="+QString::number(*Q_QGS_DeepgramUtterance)));
    Q_GLOBAL_STATIC_WITH_ARGS(const Url,        Q_QGS_DeepgramS2TNovaGeneral_Url,       ("https://api.deepgram.com/v1/listen?model=nova-general&smart_format=true&language=pt-BR&diarize=true&punctuate=true&utterances=true&utt_split="+QString::number(*Q_QGS_DeepgramUtterance)));
    Q_GLOBAL_STATIC_WITH_ARGS(const Url,        Q_QGS_DeepgramS2TNova2_Url,             ("https://api.deepgram.com/v1/listen?model=nova-2&smart_format=true&language=pt-BR&diarize=true&punctuate=true&utterances=true&utt_split="+QString::number(*Q_QGS_DeepgramUtterance)));

    Q_GLOBAL_STATIC_WITH_ARGS(const Url,        Q_QGS_DeepgramS2TNova2GeneralPTBR_Url,  ("https://api.deepgram.com/v1/listen?model=nova-2-general&smart_format=true&language=pt-BR&diarize=true&punctuate=true&utterances=true&utt_split="+QString::number(*Q_QGS_DeepgramUtterance)));

    Q_GLOBAL_STATIC_WITH_ARGS(const Url,        Q_QGS_DeepgramS2TNova2GeneralEN_Url,    ("https://api.deepgram.com/v1/listen?model=nova-2-general&smart_format=true&language=en&diarize=true&punctuate=true&utterances=true&utt_split="+QString::number(*Q_QGS_DeepgramUtterance)));

    Q_GLOBAL_STATIC_WITH_ARGS(const Url,        Q_QGS_DeepgramS2TNova3_Url,             ("https://api.deepgram.com/v1/listen?model=nova-3&smart_format=true&language=multi&diarize=true&punctuate=true&utterances=true&utt_split="+QString::number(*Q_QGS_DeepgramUtterance)));

    Q_GLOBAL_STATIC_WITH_ARGS(const Url,        Q_QGS_DeepgramS2TNova3General_Url,      ("https://api.deepgram.com/v1/listen?model=nova-3-general&smart_format=true&language=multi&diarize=true&punctuate=true&utterances=true&utt_split="+QString::number(*Q_QGS_DeepgramUtterance)));

    Q_GLOBAL_STATIC_WITH_ARGS(const Url,        Q_QGS_DeepgramS2TEnhanced_Url,          ("https://api.deepgram.com/v1/listen?model=enhanced&language=br&smart_format=true&diarize=true&punctuate=true&utterances=true&utt_split="+QString::number(*Q_QGS_DeepgramUtterance)));
    Q_GLOBAL_STATIC_WITH_ARGS(const Url,        Q_QGS_DeepgramS2TEnhancedGeneral_Url,   ("https://api.deepgram.com/v1/listen?model=enhanced-general&language=br&smart_format=true&diarize=true&punctuate=true&utterances=true&utt_split="+QString::number(*Q_QGS_DeepgramUtterance)));
    Q_GLOBAL_STATIC_WITH_ARGS(const Url,        Q_QGS_DeepgramS2TBase_Url,              ("https://api.deepgram.com/v1/listen?model=base&language=br&smart_format=true&diarize=true&punctuate=true&utterances=true&utt_split="+QString::number(*Q_QGS_DeepgramUtterance)));
    Q_GLOBAL_STATIC_WITH_ARGS(const Url,        Q_QGS_DeepgramS2TBaseGeneral_Url,       ("https://api.deepgram.com/v1/listen?model=base-general&language=br&smart_format=true&diarize=true&punctuate=true&utterances=true&utt_split="+QString::number(*Q_QGS_DeepgramUtterance)));
    Q_GLOBAL_STATIC_WITH_ARGS(const Url,        Q_QGS_DeepgramS2TWhisperTiny_Url,       ("https://api.deepgram.com/v1/listen?model=whisper-tiny&language=br&smart_format=true&diarize=true&punctuate=true&utterances=true&utt_split="+QString::number(*Q_QGS_DeepgramUtterance)));
    Q_GLOBAL_STATIC_WITH_ARGS(const Url,        Q_QGS_DeepgramS2TWhisperBase_Url,       ("https://api.deepgram.com/v1/listen?model=whisper-base&language=br&smart_format=true&diarize=true&punctuate=true&utterances=true&utt_split="+QString::number(*Q_QGS_DeepgramUtterance)));
    Q_GLOBAL_STATIC_WITH_ARGS(const Url,        Q_QGS_DeepgramS2TWhisperSmall_Url,      ("https://api.deepgram.com/v1/listen?model=whisper-small&language=br&smart_format=true&diarize=true&punctuate=true&utterances=true&utt_split="+QString::number(*Q_QGS_DeepgramUtterance)));
    Q_GLOBAL_STATIC_WITH_ARGS(const Url,        Q_QGS_DeepgramS2TWhisperMedium_Url,     ("https://api.deepgram.com/v1/listen?model=whisper-medium&language=br&smart_format=true&diarize=true&punctuate=true&utterances=true&utt_split="+QString::number(*Q_QGS_DeepgramUtterance)));
    Q_GLOBAL_STATIC_WITH_ARGS(const Url,        Q_QGS_DeepgramS2TWhisper_Url,           ("https://api.deepgram.com/v1/listen?model=whisper&language=br&smart_format=true&diarize=true&punctuate=true&utterances=true&utt_split="+QString::number(*Q_QGS_DeepgramUtterance)));
    Q_GLOBAL_STATIC_WITH_ARGS(const Url,        Q_QGS_DeepgramS2TWhisperLarge_Url,      ("https://api.deepgram.com/v1/listen?model=whisper-large&language=br&smart_format=true&diarize=true&punctuate=true&utterances=true&utt_split="+QString::number(*Q_QGS_DeepgramUtterance)));

    // Q_GLOBAL_STATIC_WITH_ARGS(QString, DEFAULT_ROOT_DIRECTORY, (QStandardPaths::writableLocation(QStandardPaths::HomeLocation)+QDir::separator()+"Downloads"+QDir::separator()));
    Q_GLOBAL_STATIC_WITH_ARGS(QString,          Q_QGS_DEFAULT_ROOT_DIRECTORY,           (QDir(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).filePath("Downloads")));

    Q_GLOBAL_STATIC_WITH_ARGS(qint64,           Q_QGS_DEFAULT_FILE_SIZE,                (15LL * 1024 * 1024));


    /**
     * Deepgram
     */
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_Deepgram_iKey_Str, (loadSecretKey("Deepgram", "ApiKey")));

    /**
     * Google
     */
    Q_GLOBAL_STATIC_WITH_ARGS(const QString,    Q_QGS_GoogleGemini_API_Key_Str,                 (loadSecretKey("Google", "GeminiApiKey")));

    Q_GLOBAL_STATIC_WITH_ARGS(const Url,        Q_QGS_BaseGoogleGeminiAPI_Url,                  ("https://generativelanguage.googleapis.com/upload/v1beta")); // Google API
    Q_GLOBAL_STATIC_WITH_ARGS(const Url,        Q_QGS_GoogleGeminiFileAPI_Url,                  ("https://generativelanguage.googleapis.com/upload/v1beta/files")); // Google API
    Q_GLOBAL_STATIC_WITH_ARGS(const Url,        Q_QGS_GoogleGeminiAI_Url,                       ("https://generativelanguage.googleapis.com/v1beta/models/%:generateContent")); // Google API

    // OpenAI API
    Q_GLOBAL_STATIC_WITH_ARGS(const QString,    Q_QGS_GoogleGemini20FlashLite_Str,              ("gemini-2.0-flash-lite"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString,    Q_QGS_GoogleGemini20FlashLite001_Str,           ("gemini-2.0-flash-lite-001"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString,    Q_QGS_GoogleGemini20Flash_Str,                  ("gemini-2.0-flash"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString,    Q_QGS_GoogleGemini20Flash001_Str,               ("gemini-2.0-flash-001"));

    Q_GLOBAL_STATIC_WITH_ARGS(const QString,    Q_QGS_GoogleGemini25Flash_Str,                  ("gemini-2.5-flash"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString,    Q_QGS_GoogleGemini25FlashLite_Str,              ("gemini-2.5-flash-lite"));

    Q_GLOBAL_STATIC_WITH_ARGS(const QString,    Q_QGS_GoogleGemini25Pro_Str,                    ("gemini-2.5-pro"));

    Q_GLOBAL_STATIC_WITH_ARGS(const QString,    Q_QGS_GoogleGemini3FlashPreview_Str,            ("gemini-3-flash-preview"));

    Q_GLOBAL_STATIC_WITH_ARGS(const QString,    Q_QGS_GoogleGemini31FlashLite_Str,              ("gemini-3.1-flash-lite"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString,    Q_QGS_GoogleGemini31FlashLitePreview_Str,       ("gemini-3.1-flash-lite-preview"));

    Q_GLOBAL_STATIC_WITH_ARGS(const QString,    Q_QGS_GoogleGemini35Flash_Str,                  ("gemini-3.5-flash"));

    Q_GLOBAL_STATIC_WITH_ARGS(const QString,    Q_QGS_GoogleGeminiFlashLiteLatest_Str,          ("gemini-flash-lite-latest"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString,    Q_QGS_GoogleGeminiFlashLatest_Str,              ("gemini-flash-latest"));

    Q_GLOBAL_STATIC_WITH_ARGS(const QString,    Q_QGS_GoogleGemini3ProPreview_Str,              ("gemini-3-pro-preview"));

    Q_GLOBAL_STATIC_WITH_ARGS(const QString,    Q_QGS_GoogleGemini31ProPreview_Str,             ("gemini-3.1-pro-preview"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString,    Q_QGS_GoogleGemini3ProPreviewCustomTools_Str,   ("gemini-3-pro-preview-customtools"));

    Q_GLOBAL_STATIC_WITH_ARGS(const QString,    Q_QGS_GoogleGeminiProLatest_Str,                ("gemini-pro-latest"));

    /********************************************************************************************************/

    Q_GLOBAL_STATIC_WITH_ARGS(const QString,    Q_QGS_GoogleHuggingFace_API_Key_Str,            (loadSecretKey("Google", "HuggingFaceApiKey")));

    Q_GLOBAL_STATIC_WITH_ARGS(const QString,    Q_QGS_GoogleOpenAPIEventProjKey_Str,            (loadSecretKey("Google", "OpenAPIEventProjKey")));

    Q_GLOBAL_STATIC_WITH_ARGS(const Url,        Q_QGS_OpenAIGoogleGeminiAI_Url,                 ("https://generativelanguage.googleapis.com/v1beta/openai/chat/completions/")); // OpenAI API
    /********************************************************************************************************/

    /**
     * GPT
     */
    Q_GLOBAL_STATIC_WITH_ARGS(const QString,    Q_QGS_GPT_4o_MiniKey_Str,                       (loadSecretKey("OpenAI", "GPT4oMiniApiKey")));

    Q_GLOBAL_STATIC_WITH_ARGS(const Url,        Q_QGS_GPT_4o_Mini_Url,                          ("https://api.openai.com/v1/chat/completions"));

    Q_GLOBAL_STATIC_WITH_ARGS(const QString,    Q_QGS_GPT_4o_MiniModel_Str,                     ("gpt-4o-mini"));
    /********************************************************************************************************/

    /**
     * OpenRouter
     */
    Q_GLOBAL_STATIC_WITH_ARGS(const Url,        Q_QGS_OpenRouterAI_Url,                                        ("https://openrouter.ai/api/v1/chat/completions"));

    Q_GLOBAL_STATIC_WITH_ARGS(const QString,    Q_QGS_OpenRouterKey_Str,                                       (loadSecretKey("OpenRouter", "ApiKey")));

    Q_GLOBAL_STATIC_WITH_ARGS(const QString,    Q_QGS_OpenRouterFreeModel_Str,                                 ("openrouter/free"))
    Q_GLOBAL_STATIC_WITH_ARGS(const QString,    Q_QGS_OpenRouter_Deepseekv2Model_Str,                          ("deepseek/deepseek-prover-v2:free"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString,    Q_QGS_OpenRouter_GoogleGemma3_27B_Str,                         ("deepseek/deepseek-prover-v2:free"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString,    Q_QGS_OpenRouter_Deepseekv3BaseModel_Str,                      ("deepseek/deepseek-v3-base:free"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString,    Q_QGS_OpenRouter_DeepSeekDeepSeekV3Model_Str,                  ("deepseek/deepseek-chat:free"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString,    Q_QGS_OpenRouter_Qwen3235BA22BModel_Str,                       ("qwen/qwen3-235b-a22b:free"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString,    Q_QGS_OpenRouter_MicrosoftMAIDSR1Model_Str,                    ("microsoft/mai-ds-r1:free"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString,    Q_QGS_OpenRouter_MetaLlama4Maverick2Model_Str,                 ("meta-llama/llama-4-maverick:free"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString,    Q_QGS_OpenRouter_MetaLlama4ScoutModel_Str,                     ("meta-llama/llama-4-scout:free"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString,    Q_QGS_OpenRouter_GoogleGemini2_5ProExperimentalModel_Str,      ("google/gemini-2.5-pro-exp-03-25"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString,    Q_QGS_OpenRouter_GoogleGemma3_12BModel_Str,                    ("google/gemma-3-12b-it:free"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString,    Q_QGS_OpenRouter_GoogleGemma3_27BModel_Str,                    ("google/gemma-3-27b-it:free"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString,    Q_QGS_OpenRouter_GoogleGemini2_0FlashExperimentalModel_Str,    ("google/gemini-2.0-flash-exp:free"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString,    Q_QGS_OpenRouter_MistralMistralNemoModel_Str,                  ("mistralai/mistral-nemo:free"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString,    Q_QGS_OpenRouter_GPT_4o_MiniModel_Str,                         ("openai/gpt-4o-mini"));
    /********************************************************************************************************/

    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_URLContentTypeUnknown_Str,           ("unknown"));

    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_URLContentTypeCBOR_Str,              ("application/cbor"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_URLContentTypeJason_Str,             ("application/json"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_URLContentTypeOctetStream_Str,       ("application/octet-stream"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_URLContentTypePDF_Str,               ("application/pdf"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_URLContentTypeZip_Str,               ("application/zip"));


    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_URLContentTypeAudioAAC_Str,          ("audio/aac"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_URLContentTypeAudioFLAC_Str,         ("audio/flac"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_URLContentTypeAudioM4A_Str,          ("audio/m4a"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_URLContentTypeAudioMP2_Str,          ("audio/mp2"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_URLContentTypeAudioMP3_Str,          ("audio/mp3"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_URLContentTypeAudioMP4_Str,          ("audio/mp4"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_URLContentTypeAudioOgg_Str,          ("audio/ogg"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_URLContentTypeAudioOpus_Str,         ("audio/opus"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_URLContentTypeAudioPCM_Str,          ("audio/pcm"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_URLContentTypeAudioWav_Str,          ("audio/wav"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_URLContentTypeAudioWebM_Str,         ("audio/webm"));

    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_URLContentTypeImageGif_Str,          ("image/gif"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_URLContentTypeImageJpg_Str,          ("image/jpg"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_URLContentTypeImagePng_Str,          ("image/png"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_URLContentTypeImageSVGXML_Str,       ("image/svg+xml"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_URLContentTypeImageWebP_Str,         ("image/webp"));

    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_URLContentTypeTextPlain_Str,         ("text/plain"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_URLContentTypeTextHtml_Str,          ("text/html"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_URLContentTypeTextCss_Str,           ("text/css"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_URLContentTypeTextCsv_Str,           ("text/csv"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_URLContentTypeTextXCPPSrc_Str,       ("text/x-c+src"));

    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_URLContentTypeVideoMP4_Str,          ("video/mp4"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_URLContentTypeVideoXMatroska_Str,    ("video/x-matroska"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_URLContentTypeVideoQuicktime_Str,    ("video/quicktime"));


    /*
     * Headers
     */
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_URLHeaderAuthorizationTemplate_Str,          ("Authorization: Bearer <KEY>"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_URLHeaderGoogleAuthorizationTemplate_Str,    ("x-goog-api-key: <KEY>"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_URLHeaderRefererTemplate_Str,                ("HTTP-Referer: <SITE_URL>"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_URLHeaderXTitleTemplate_Str,                 ("X-Title: <SITE_NAME>"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_URLHeaderContentTypeTemplate_Str,            ("Content-Type: <CONTENT_TYPE>"));


    const QString result01 = loadFromFileDefs(":/Resources/Result01");

    const QString result2 = loadFromFileDefs(":/Resources/Result02");

    const QString request01 = loadFromFileDefs(":/Resources/Request01");;

    const QString curlPost = loadFromFileDefs(":/Resources/CurlPost01");


    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_PipelineModeNone,                            ("none"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_PipelineModeTranscode,                       ("transcode_only"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_PipelineModeTranscribe,                      ("transcribe_only"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_PipelineModeConvert,                         ("convert_only"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_PipelineModeSegment,                         ("segment_only"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_PipelineModeEvtsExtraction,                  ("evtsExtraction_only"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_PipelineModePipeline,                        ("pipeline"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_PipelineModeExperiment,                      ("experiment"));

    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_ExperimentModeNone,                          ("none"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_ExperimentModeTokenValidation,               ("token_validation_experiment"));                   // Validation of Token Consumption Optimization
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_ExperimentBaselineGoverned,                  ("baseline_governed_experiment"));                  // Generate the baseline and governed (with system profile) responses
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_ExperimentGovernanceAudit,                   ("governance_audit"));                              // Validation of Hallucination Mitigation & Constraint Enforcement
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_ExperimentReportEventsFound,                 ("report_events_found"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_ExperimentReportEventsRejected,              ("report_events_rejected"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_ExperimentPrintReportGoverned,               ("print_report_governed"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_ExperimentModeEventSpottingDomainAlignment,  ("event_spotting_domain_alignment"));    // Precise Event Spotting & Clock-Domain Alignment

    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_CANDIDATES,                              ("candidates"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_CONTENT,                                 ("content"));

    // Events
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_EVENTS,                                  ("events"));

    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_EVENTS_BROADCASTERS,                     ("broadcasters"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_EVENTS_CATEGORY,                         ("category"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_EVENTS_CONFIDENCE,                       ("confidence"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_EVENTS_DESCRIPTION,                      ("description"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_EVENTS_EXPERT_ANALYSIS,                  ("expert_analysis"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_EVENTS_ID,                               ("id"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_EVENTS_PLAYSERS,                         ("players"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_EVENTS_PREV_CAUSAL,                      ("prev_causal"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_EVENTS_PREV_TEMPORAL,                    ("prev_temporal"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_EVENTS_PRIORITY_LEVEL,                   ("priority_level"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_EVENTS_REFEREE_ACTION,                   ("referee_action"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_EVENTS_TYPE,                             ("type"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_EVENTS_TIMESTAMP_REAL,                   ("timestamp_real"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_EVENTS_TIMESTAMP_VIDEO,                  ("timestamp_video"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_EVENTS_TIMESTAMP_VTT,                    ("timestamp_vtt"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_EVENTS_VERDICT,                          ("verdict"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_EVENTS_VTT_MATCH,                        ("vtt_match"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_EVENTS_VTT_SOURCE,                       ("vtt_source"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_EVENTS_VTT_STATUS,                       ("vtt_status"));

    Q_GLOBAL_STATIC_WITH_ARGS(const QStringList, Q_QGS_REQUIRED_EVENTS_KEYS,                    (QStringList{
                                                                                     *Constants::Q_QGS_KEY_EVENTS_BROADCASTERS,
                                                                                     *Constants::Q_QGS_KEY_EVENTS_CATEGORY,
                                                                                     *Constants::Q_QGS_KEY_EVENTS_CONFIDENCE,
                                                                                     *Constants::Q_QGS_KEY_EVENTS_DESCRIPTION,
                                                                                     *Constants::Q_QGS_KEY_EVENTS_EXPERT_ANALYSIS,
                                                                                     *Constants::Q_QGS_KEY_EVENTS_ID,
                                                                                     *Constants::Q_QGS_KEY_EVENTS_PLAYSERS,
                                                                                     *Constants::Q_QGS_KEY_EVENTS_PREV_CAUSAL,
                                                                                     *Constants::Q_QGS_KEY_EVENTS_PREV_TEMPORAL,
                                                                                     *Constants::Q_QGS_KEY_EVENTS_PRIORITY_LEVEL,
                                                                                     *Constants::Q_QGS_KEY_EVENTS_REFEREE_ACTION,
                                                                                     *Constants::Q_QGS_KEY_EVENTS_TYPE,
                                                                                     *Constants::Q_QGS_KEY_EVENTS_TIMESTAMP_REAL,
                                                                                     *Constants::Q_QGS_KEY_EVENTS_TIMESTAMP_VIDEO,
                                                                                     *Constants::Q_QGS_KEY_EVENTS_TIMESTAMP_VTT,
                                                                                     *Constants::Q_QGS_KEY_EVENTS_VERDICT,
                                                                                     *Constants::Q_QGS_KEY_EVENTS_VTT_MATCH,
                                                                                     *Constants::Q_QGS_KEY_EVENTS_VTT_SOURCE,
                                                                                     *Constants::Q_QGS_KEY_EVENTS_VTT_STATUS
                                                                        }));

    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_VTT_VERIFICATION,                        ("vtt_verification"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_VTT_VERIFICATION_CONFIRMED_EVENTS,       ("confirmed_events"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_VTT_VERIFICATION_REJECTED_EVENTS,        ("rejected_events"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_VTT_VERIFICATION_MISSED_EVENTS_BY_VTT,   ("missed_events_by_vtt"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_VTT_VERIFICATION_VTT_UNAVAILABLE,        ("vtt_unavailable"));

    Q_GLOBAL_STATIC_WITH_ARGS(const QStringList, Q_QGS_REQUIRED_VTT_VERIFICATION_KEYS,          (QStringList{
                                                                                                    *Constants::Q_QGS_KEY_VTT_VERIFICATION_CONFIRMED_EVENTS,
                                                                                                    *Constants::Q_QGS_KEY_VTT_VERIFICATION_REJECTED_EVENTS,
                                                                                                    *Constants::Q_QGS_KEY_VTT_VERIFICATION_MISSED_EVENTS_BY_VTT,
                                                                                                    *Constants::Q_QGS_KEY_VTT_VERIFICATION_VTT_UNAVAILABLE
                                                                                       }));


    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_STATUS_CONFIRMED,                            ("confirmed"));

    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_VIDEO_CLIP_INFO,                         ("video_clip_info"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_VIDEO_CLIP_ID,                           ("clip_video_id"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_VIDEO_CLIP_START,                        ("clip_start_video"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_VIDEO_CLIP_END,                          ("clip_end_video"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_VIDEO_CLIP_SLOWDOWN_FACTOR,              ("slowdown_factor"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_VIDEO_CLIP_PRIORITY_EVENTS_DETECTED,     ("priority_events_detected"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_VIDEO_CLIP_TOTAL_VTT_EVENTS,             ("total_vtt_events"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_VIDEO_CLIP_VTT_EVENTS_CONFIRMED,         ("vtt_events_confirmed"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_VIDEO_CLIP_VTT_EVENTS_REJECTED,          ("vtt_events_rejected"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_VIDEO_CLIP_VIDEO_ONLY_EVENTS,            ("video_only_events"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_VIDEO_CLIP_VTT_ACCURACY,                 ("vtt_accuracy"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_VIDEO_CLIP_TOTAL_EVENTS,                 ("total_events"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_VIDEO_CLIP_ANALYSIS_DATE,                ("analysis_date"));

    Q_GLOBAL_STATIC_WITH_ARGS(const QStringList, Q_QGS_KEY_VIDEO_CLIP_INFO_KEYS,                (QStringList{
                                                                                              *Constants::Q_QGS_KEY_VIDEO_CLIP_ID,
                                                                                              *Constants::Q_QGS_KEY_VIDEO_CLIP_START,
                                                                                              *Constants::Q_QGS_KEY_VIDEO_CLIP_END,
                                                                                              *Constants::Q_QGS_KEY_VIDEO_CLIP_SLOWDOWN_FACTOR,
                                                                                              *Constants::Q_QGS_KEY_VIDEO_CLIP_PRIORITY_EVENTS_DETECTED,
                                                                                              *Constants::Q_QGS_KEY_VIDEO_CLIP_TOTAL_VTT_EVENTS,
                                                                                              *Constants::Q_QGS_KEY_VIDEO_CLIP_VTT_EVENTS_CONFIRMED,
                                                                                              *Constants::Q_QGS_KEY_VIDEO_CLIP_VTT_EVENTS_REJECTED,
                                                                                              *Constants::Q_QGS_KEY_VIDEO_CLIP_VIDEO_ONLY_EVENTS,
                                                                                              *Constants::Q_QGS_KEY_VIDEO_CLIP_VTT_ACCURACY,
                                                                                              *Constants::Q_QGS_KEY_VIDEO_CLIP_TOTAL_EVENTS,
                                                                                              *Constants::Q_QGS_KEY_VIDEO_CLIP_ANALYSIS_DATE
                                                                                 }));

    // Root keys list
    Q_GLOBAL_STATIC_WITH_ARGS(const QStringList, Q_QGS_REQUIRED_ROOT_KEYS,                      (QStringList{
                                                                                        *Constants::Q_QGS_KEY_VIDEO_CLIP_INFO,
                                                                                        *Constants::Q_QGS_KEY_VTT_VERIFICATION,
                                                                                        *Constants::Q_QGS_KEY_EVENTS
                                                                           }));

    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_PARTS,                                   ("parts"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_TEXT,                                    ("text"));

    // This key is a non-structured key, i.e., it is not defined in the System Profile and User Prompt. For baseline analysis only.
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_UNSTRUCUTRED_SOCCER_EVENTS,              ("soccer_events"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_UNSTRUCUTRED_KEY_EVENTS,                 ("key_events"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_UNSTRUCUTRED_KEY_SOCCER_EVENTS,          ("key_soccer_events"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_UNSTRUCUTRED_EVENT_TYPE,                 ("event_type"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_UNSTRUCUTRED_DESCRIPTION,                ("description"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_UNSTRUCUTRED_DETAILS,                    ("details"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_UNSTRUCUTRED_COMMENTARY,                 ("commentary"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_UNSTRUCUTRED_NOTE,                       ("note"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_UNSTRUCUTRED_NOTES,                      ("notes"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_UNSTRUCUTRED_COMMENT,                    ("comment"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_UNSTRUCUTRED_DESYNC_NOTE,                ("desync_note"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_KEY_UNSTRUCUTRED_DESYNC_VTT,                 ("VTT"));

    // Known baseline event array keys
    Q_GLOBAL_STATIC_WITH_ARGS(const QStringList, Q_QGS_KNOWN_UNSTRUCTURED_EVENT_KEYS,           (QStringList{
                                                                                          *Constants::Q_QGS_KEY_UNSTRUCUTRED_SOCCER_EVENTS,
                                                                                          *Constants::Q_QGS_KEY_UNSTRUCUTRED_KEY_EVENTS,
                                                                                          *Constants::Q_QGS_KEY_UNSTRUCUTRED_KEY_SOCCER_EVENTS
                                                                                      }));

    // Unstructured VTT desync notes list
    Q_GLOBAL_STATIC_WITH_ARGS(const QStringList, Q_QGS_UNFORMATTED_NOTE_KEYS,                   (QStringList{
                                                                                  *Constants::Q_QGS_KEY_UNSTRUCUTRED_NOTE,
                                                                                  *Constants::Q_QGS_KEY_UNSTRUCUTRED_NOTES,
                                                                                  *Constants::Q_QGS_KEY_UNSTRUCUTRED_COMMENT,
                                                                                  *Constants::Q_QGS_KEY_UNSTRUCUTRED_COMMENTARY,
                                                                                  *Constants::Q_QGS_KEY_UNSTRUCUTRED_EVENT_TYPE,
                                                                                  *Constants::Q_QGS_KEY_UNSTRUCUTRED_DESYNC_NOTE
                                                                              }));


    // Unstructured desync notes. Include here notes of VTT found in the baseline pipeline response.
    Q_GLOBAL_STATIC_WITH_ARGS(const QStringList, QGS_UNSTRUCTURED_DSYNC_NOTES,                  (QStringList{
                                                                                   "not align",
                                                                                   "does not appear",
                                                                                   "discrepancy",
                                                                                   "does not match"
                                                                               }));

    // Soccer Events
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_CAT_GOAL,                                    ("GOAL"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_CAT_LAST_MINUTE_GOAL,                        ("LAST-MINUTE GOAL"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_CAT_FOUL,                                    ("FOUL"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_CAT_PENALTY,                                 ("PENALTY"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_CAT_PENALTY_KICK,                            ("PENALTY KICK"));

    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_CAT_CARD,                                    ("CARD"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_CAT_RED_CARD,                                ("RED CARD"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_CAT_RED_CARD_INCIDENT,                       ("RED CARD INCIDENT"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_CAT_YELLOW_CARD,                             ("YELLOW CARD"));

    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_CAT_PASS,                                    ("PASS"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_CAT_HIGH_PASS,                               ("HIGH PASS"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_CAT_SUBSTITUTION,                            ("SUBSTITUTION"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_CAT_SHOT,                                    ("SHOT"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_CAT_THROW_IN,                                ("THROW-IN"));

    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_CAT_FREE_KICK,                               ("FREE KICK"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_CAT_CORNER,                                  ("CORNER"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_CAT_CORNER_KICK,                             ("CORNER KICK"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_CAT_GOAL_KICK,                               ("GOAL KICK"));

    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_CAT_SAVE,                                    ("SAVE"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_CAT_DRAMATIC_SAVE,                           ("DRAMATIC SAVE"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_CAT_PLAYER_BLOCK,                            ("PLAYER BLOCK"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_CAT_PLAYER_SUCCESSFUL_TACKLE,                ("PLAYER SUCCESSFUL TACKLE"));

    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_CAT_OUT,                                     ("OUT"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_CAT_CROSS,                                   ("CROSS"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_CAT_BALL,                                    ("BALL"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_CAT_OFFSIDE_CALL,                            ("OFFSIDE CALL"));

    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_CAT_INJURY_TIMEOUT,                          ("INJURY TIMEOUT"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_CAT_DRIVE,                                   ("DRIVE"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_CAT_HEADER,                                  ("HEADER"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_CAT_HAT_TRICK,                               ("HAT TRICK"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_CAT_INJURY,                                  ("INJURY"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_CAT_CELEBRATION_MOMENT,                      ("CELEBRATION MOMENT"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_CAT_VIRAL_MOMENT,                            ("VIRAL MOMENT"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_CAT_TEAM_HUDDLE,                             ("TEAM HUDDLE"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_CAT_MANAGER_REACTION,                        ("MANAGER REACTION"));

    // Define the collection of critical category tokens
    Q_GLOBAL_STATIC_WITH_ARGS(const QStringList, Q_QGS_CRITICAL_CATEGORIES,                     (QStringList{
                                                                                            *Constants::Q_QGS_CAT_GOAL,
                                                                                            *Constants::Q_QGS_CAT_FOUL,
                                                                                            *Constants::Q_QGS_CAT_CARD,
                                                                                            *Constants::Q_QGS_CAT_RED_CARD,
                                                                                            *Constants::Q_QGS_CAT_YELLOW_CARD,
                                                                                            *Constants::Q_QGS_CAT_RED_CARD_INCIDENT,
                                                                                            *Constants::Q_QGS_CAT_PENALTY,
                                                                                            *Constants::Q_QGS_CAT_FREE_KICK,
                                                                                            *Constants::Q_QGS_CAT_CORNER,
                                                                                            *Constants::Q_QGS_CAT_SHOT
                                                                                }));

    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_BATCH_TOKEN_OPTIMIZAION_REPORT,              ("Batch_Token_Optimization_Report.txt"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_COMPLIANCE_AUDIT_REPORT,                     ("Compliance_Audit_Report.txt"));
    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_TEMPORAL_ALIGNMENT_AUDIT_REPORT,             ("Temporal_Alignment_Report.txt"));

    Q_GLOBAL_STATIC_WITH_ARGS(const qreal, Q_QGS_DEFAULT_CLIP_SEC,                              (300.00));

    Q_GLOBAL_STATIC_WITH_ARGS(const qreal, Q_QGS_DEFAULT_SLOWDOWN_FACTOR,                       (0.00));

    Q_GLOBAL_STATIC_WITH_ARGS(const double, Q_QGS_CONFIDENCE_THRESHOLD,                         (0.70));

    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_CLIP_TAG_STR,                                ("CLIP_"));

    Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_CLIP_ITERATON_TAG_PATTERN,                   ("## [CLIP_%1_ITERATION_%2]\n"));
}

#pragma GCC diagnostic pop
#pragma clang diagnostic pop

struct ClipData
{
    QMap<QString, QString> data;

    // Default Constructor
    ClipData() = default;

    // Convenience Constructor
    explicit ClipData(const QMap<QString, QString>& initialData)
        : data(initialData)
    {
        // TODO
    }

    // Equality Operator
    bool operator==(const ClipData& other) const
    {
        return data == other.data;
    }

    bool operator!=(const ClipData& other) const
    {
        return !(*this == other);
    }

    // Useful Functions
    bool isEmpty() const
    {
        return data.isEmpty();
    }

    bool isValid() const
    {
        return !data.isEmpty();
    }

    int count() const
    {
        return data.size();
    }

    int size() const
    {
        return data.size();
    }

    void clear()
    {
        data.clear();
    }

    // Helper to add or update metadata
    void add(const QString& key, const QString& value)
    {
        data.insert(key, value);
    }

    QString firstKey()
    {
        return data.firstKey();
    }

    QString take(QString key)
    {
        return data.take(key);
    }

    // Helper to get a list of all Keys
    QStringList clipDataKeys() const
    {
        return data.keys();
    }

    // Helper to get a prompt for a specific key
    QString valueFor(const QString& key) const
    {
        return data.value(key);
    }

    // Check if an filePath exists
    bool contains(const QString& key) const
    {
        return data.contains(key);
    }
};
// Registering the types
Q_DECLARE_METATYPE(ClipData)

struct SegmentsWithPrompts
{
    // Inner structure to hold all necessary metadata for a segment
    struct SegmentContext {
        QString segmentId;
        QString userPrompt;

        SegmentContext() = default;

        SegmentContext(QString id, QString prompt) :
            segmentId(std::move(id)),
            userPrompt(std::move(prompt))
        {
            // TODO
        }

        bool operator==(const SegmentContext& other) const noexcept {
            return segmentId == other.segmentId && userPrompt == other.userPrompt;
        }
        bool operator!=(const SegmentContext& other) const noexcept {
            return !(*this == other);
        }
    };

    // QMap<FilePath, UserPrompt>
    QMap<QString, SegmentContext> data;

    // Default Constructor
    SegmentsWithPrompts() = default;

    // Convenience Constructor
    explicit SegmentsWithPrompts(const QMap<QString, SegmentContext>& initialData) :
        data(std::move(initialData))
    {
        // TODO
    }

    // Equality Operator
    bool operator==(const SegmentsWithPrompts& other) const noexcept
    {
        return data == other.data;
    }

    bool operator!=(const SegmentsWithPrompts& other) const noexcept
    {
        return !(*this == other);
    }

    // Useful Functions
    [[nodiscard]] bool isEmpty() const noexcept
    {
        return data.isEmpty();
    }

    [[nodiscard]] bool isValid() const noexcept
    {
        return !data.isEmpty();
    }

    [[nodiscard]] qint32 count() const noexcept
    {
        return data.size();
    }

    [[nodiscard]] qint32 size() const noexcept
    {
        return data.size();
    }

    void clear()
    {
        data.clear();
    }

    // Check if an filePath exists
    [[nodiscard]] bool contains(const QString& filePath) const
    {
        return data.contains(filePath);
    }

    // =========================================================================
    // Mutation & Data Insertion
    // =========================================================================

    // Adds a segment-prompt pair
    // Copy-based insertion
    void add(const QString& filePath, const QString& segmentId, const QString& prompt)
    {
        data.insert(filePath, {segmentId, prompt});
    }

    // Move-based insertion (Zero-copy for rvalues)
    void add(QString&& filePath, QString&& segmentId, QString&& prompt)
    {
        data.insert(std::move(filePath), SegmentContext(std::move(segmentId), std::move(prompt)));
    }

    // Direct context insertion
    void add(const QString& filePath, SegmentContext context)
    {
        data.insert(filePath, std::move(context));
    }

    // Removes and returns the context for the given filePath
    SegmentContext take(const QString &filePath)
    {
        return data.take(filePath);
    }

    // =========================================================================
    // High-Performance Lookups (Zero Temporary Struct Copies)
    // =========================================================================

    // First filePath
    [[nodiscard]] QString firstKey()
    {
        return data.firstKey();
    }

    // Helper to get a list of all filePaths (Keys)
    [[nodiscard]] QStringList segmentfilePaths() const
    {
        return data.keys();
    }

    [[nodiscard]] const SegmentContext * contextFor(const QString &filePath) const
    {
        const auto it = data.constFind(filePath);

        return (it != data.constEnd()) ? &it.value() : nullptr;
    }

    // Fast helper to get a prompt for a specific filePath
    [[nodiscard]] QString promptFor(const QString& filePath, const QString& fallback = QString()) const
    {
        const auto it = data.constFind(filePath);
        return (it != data.constEnd()) ? it.value().userPrompt : fallback;
    }

    [[nodiscard]] QString segmentIdFor(const QString& filePath, const QString& fallback = QString()) const
    {
        const auto it = data.constFind(filePath);
        if (it != data.constEnd())
            return it.value().segmentId;

        return (it != data.constEnd()) ? it.value().segmentId : fallback;
    }

    // Type aliases for C++11 compatibility
    using iterator                 = QMap<QString, SegmentContext>::iterator;
    using const_iterator           = QMap<QString, SegmentContext>::const_iterator;
    using key_value_iterator       = QMap<QString, SegmentContext>::key_value_iterator;
    using const_key_value_iterator = QMap<QString, SegmentContext>::const_key_value_iterator;

    // =========================================================================
    // STL & Qt Iterator Passthrough (Enables range-based for loops)
    // =========================================================================
    iterator begin() noexcept { return data.begin(); }
    iterator end() noexcept { return data.end(); }
    const_iterator begin() const noexcept { return data.begin(); }
    const_iterator end() const noexcept { return data.end(); }
    const_iterator cbegin() const noexcept { return data.cbegin(); }
    const_iterator cend() const noexcept { return data.cend(); }
    const_iterator constBegin() const noexcept { return data.constBegin(); }
    const_iterator constEnd() const noexcept { return data.constEnd(); }
    const_key_value_iterator keyValueBegin() const noexcept { return data.keyValueBegin(); }
    const_key_value_iterator keyValueEnd() const noexcept { return data.keyValueEnd(); }
};
Q_DECLARE_METATYPE(SegmentsWithPrompts)


#ifndef WORKER_EXIT_STATUS_DEFINED
#define WORKER_EXIT_STATUS_DEFINED
enum class WorkerExitStatus {
    Success = 0X4002000,
    Failure,
    Crash,  // Used by CLI
    Abort
};
#endif

#ifndef FFMPEG_STATE_DEFINED
#define FFMPEG_STATE_DEFINED
enum class FFmpegState_e {
    Idle = 0X4003000,
    OpeningInput,
    ConfiguringCodecs,
    Filtering,
    Processing,
    Flushing,
    Finalizing,
    Finished,
    Error
};

Q_ENUMS(FFmpegState_e)

typedef FFmpegState_e FFmpegState_t;

inline bool operator==(const FFmpegState_t type1, const FFmpegState_t type2)
{
    return (static_cast<quint64> (type1) == static_cast<quint64> (type2));
}

inline uint qHash(const FFmpegState_t type, uint seed = 0)
{
    return (::qHash(static_cast<quint64> (type), seed));
}
#endif

#ifndef FFMPEG_PROGRESS_DEFINED
#define FFMPEG_PROGRESS_DEFINED
struct FFmpegProgress {
    qint64 timestamp;
    double timeInSeconds;
};
#endif

#ifndef MODEL_CODE_DEFINED
#define MODEL_CODE_DEFINED
enum class ModelCode_e {
    None = 0X4004000,
    Gemini20_FlashLite2fps23crf,                // 2FPS, 23CRF
    Gemini20_FlashLite001_2fps23crf,            // 2FPS, 23CRF
    Gemini20_Flash5fps21crf,                    // 5FPS, 21CRF
    Gemini20_Flash001_5fps21crf,                // 5FPS, 21CRF

    Gemini25_FlashLite2fps23crf,                // 2FPS, 23CRF
    Gemini25_Flash5fps21crf,                    // 5FPS, 21CRF

    Gemini25_Pro5fps20crf,                      // 5FPS, 20CRF
    Gemini25_Pro10fps20crf,                     // 10FPS, 20CRF

    Gemini3_FlashPreview5fps21crf,              // 5FPS, 21CRF
    Gemini3_Flash5fps21crf,                     // 5FPS, 21CRF

    Gemini31_FlashLitePreview1fps23crf,         // 1FPS, 23CRF
    Gemini31_FlashLitePreview2fps23crf,         // 2FPS, 23CRF
    Gemini31_FlashPreview5fps21crf,             // 5FPS, 21CRF

    Gemini35_Flash5fps21crf,                    // 5FPS, 21CRF

    Gemini_FlashLiteLatest1fps23crf,            // 1FPS, 23CRF
    Gemini_FlashLiteLatest2fps23crf,            // 2FPS, 23CRF
    Gemini_FlashLatest5fps21crf,                // 5FPS, 21CRF

    Gemini3_ProPreview5fps20crf,                // 5FPS, 20CRF
    Gemini3_ProPreview10fps20crf,               // 10FPS, 20CRF

    Gemini31_ProPreview5fps20crf,               // 5FPS, 20CRF
    Gemini31_ProPreview10fps20crf,              // 10FPS, 20CRF
    Gemini31_ProPreviewCustomTools5fps20crf,    // 5FPS, 20CRF
    Gemini31_ProPreviewCustomTools10fps20crf,   // 10FPS, 20CRF

    Gemini_ProLatest5fps20crf,                  // 5FPS, 20CRF
    Gemini_ProLatest10fps20crf,                 // 10FPS, 20CRF

    Grok3_5fps22crf,                            // 5FPS, 22CRF
    Grok3Mini_5fps22crf,                        // 5FPS, 22CRF
    Grok4_10fps22crf,                           // 10FPS, 22CRF
    GrokMini_5fps22crf,                         // 5FPS, 22CRF
    GPT4_10fps21crf,                            // 10FPS, 21CRF
    GPT45_10fps21crf,                           // 10FPS, 21CRF
    Llama30_5fps23crf,                          // 5FPS, 23CRF
    Llama31_5fps23crf,                          // 5FPS, 23CRF
    Llama4Scout_10fps23crf,                     // 10FPS, 23CRF
    Llama4Maverick_10fps28crf,                  // 10FPS, 28CRF
    Llama4Behemoth_10fps28crf,                  // 10FPS, 28CRF
    DeepSeekVL2_5fps28crf,                      // 5FPS, 28CRF
    Qwen3_5fps20crf                             // 5FPS, 20CRF
};

Q_ENUMS(ModelCode_e)

typedef ModelCode_e ModelCode_t;

inline bool operator==(const ModelCode_t type1, const ModelCode_t type2)
{
    return (static_cast<quint64> (type1) == static_cast<quint64> (type2));
}

inline uint qHash(const ModelCode_t type, uint seed = 0)
{
    return (::qHash(static_cast<quint64> (type), seed));
}

static const QHash<ModelCode_t, QString> ModelCodeMap = {
    {ModelCode_t::None, "none"},
    {ModelCode_t::Gemini20_FlashLite2fps23crf,              "Gemini20_FlashLite2fps23crf"},
    {ModelCode_t::Gemini20_FlashLite001_2fps23crf,          "Gemini20_FlashLite001_2fps23crf"},
    {ModelCode_t::Gemini20_Flash5fps21crf,                  "Gemini20_Flash5fps21crf"},
    {ModelCode_t::Gemini20_Flash001_5fps21crf,              "Gemini20_Flash001_5fps21crf"},

    {ModelCode_t::Gemini25_FlashLite2fps23crf,              "Gemini25_FlashLite2fps23crf"},
    {ModelCode_t::Gemini25_Flash5fps21crf,                  "Gemini25_Flash5fps21crf"},

    {ModelCode_t::Gemini25_Pro5fps20crf,                    "Gemini25_Pro5fps20crf"},
    {ModelCode_t::Gemini25_Pro10fps20crf,                   "Gemini25_Pro10fps20crf"},

    {ModelCode_t::Gemini3_FlashPreview5fps21crf,            "Gemini3_FlashPreview5fps21crf"},
    {ModelCode_t::Gemini3_Flash5fps21crf,                   "Gemini3_Flash5fps21crf"},

    {ModelCode_t::Gemini31_FlashLitePreview1fps23crf,       "Gemini31_FlashLitePreview1fps23crf"},
    {ModelCode_t::Gemini31_FlashLitePreview2fps23crf,       "Gemini31_FlashLitePreview2fps23crf"},
    {ModelCode_t::Gemini31_FlashPreview5fps21crf,           "Gemini31_FlashPreview5fps21crf"},

    {ModelCode_t::Gemini35_Flash5fps21crf,                  "Gemini35_Flash5fps21crf"},

    {ModelCode_t::Gemini_FlashLiteLatest1fps23crf,          "Gemini_FlashLiteLatest1fps23crf"},
    {ModelCode_t::Gemini_FlashLiteLatest2fps23crf,          "Gemini_FlashLiteLatest2fps23crf"},
    {ModelCode_t::Gemini_FlashLatest5fps21crf,              "Gemini_FlashLatest5fps21crf"},

    {ModelCode_t::Gemini3_ProPreview5fps20crf,              "Gemini3_ProPreview5fps20crf"},
    {ModelCode_t::Gemini3_ProPreview10fps20crf,             "Gemini3_ProPreview10fps20crf"},

    {ModelCode_t::Gemini31_ProPreview5fps20crf,             "Gemini31_ProPreview5fps20crf"},
    {ModelCode_t::Gemini31_ProPreview10fps20crf,            "Gemini31_ProPreview10fps20crf"},
    {ModelCode_t::Gemini31_ProPreviewCustomTools5fps20crf,  "Gemini31_ProPreviewCustomTools5fps20crf"},
    {ModelCode_t::Gemini31_ProPreviewCustomTools10fps20crf, "Gemini31_ProPreviewCustomTools10fps20crf"},

    {ModelCode_t::Gemini_ProLatest5fps20crf,                "Gemini_ProLatest5fps20crf"},
    {ModelCode_t::Gemini_ProLatest10fps20crf,               "Gemini_ProLatest10fps20crf"},

    {ModelCode_t::Grok3_5fps22crf,                          "Grok3_5fps22crf"},
    {ModelCode_t::Grok3Mini_5fps22crf,                      "Grok3Mini_5fps22crf"},
    {ModelCode_t::Grok4_10fps22crf,                         "Grok4_10fps22crf"},
    {ModelCode_t::GrokMini_5fps22crf,                       "GrokMini_5fps22crf"},
    {ModelCode_t::GPT4_10fps21crf,                          "GPT4_10fps21crf"},
    {ModelCode_t::GPT45_10fps21crf,                         "GPT45_10fps21crf"},
    {ModelCode_t::Llama30_5fps23crf,                        "Llama30_5fps23crf"},
    {ModelCode_t::Llama31_5fps23crf,                        "Llama31_5fps23crf"},
    {ModelCode_t::Llama4Scout_10fps23crf,                   "Llama4Scout_10fps23crf"},
    {ModelCode_t::Llama4Maverick_10fps28crf,                "Llama4Maverick_10fps28crf"},
    {ModelCode_t::Llama4Behemoth_10fps28crf,                "Llama4Behemoth_10fps28crf"},
    {ModelCode_t::DeepSeekVL2_5fps28crf,                    "DeepSeekVL2_5fps28crf"},
    {ModelCode_t::Qwen3_5fps20crf,                          "Qwen3_5fps20crf"}
};

#endif

#ifndef CONVERTER_TYPE_DEFINED
#define CONVERTER_TYPE_DEFINED
enum class ConverterType_e
{
    None = 0X4005000,
    DeepgramWebVttConverter,
    DeepgramSRTConverter
};

Q_ENUMS(ConverterType_e)

typedef ConverterType_e ConverterType_t;

inline bool operator==(const ConverterType_t type1, const ConverterType_t type2)
{
    return (static_cast<quint64> (type1) == static_cast<quint64> (type2));
}

inline uint qHash(const ConverterType_t type, uint seed = 0)
{
    return (::qHash(static_cast<quint64> (type), seed));
}

static const QHash<ConverterType_t, QString> TranscriptionConverterMap = {
    {ConverterType_t::None,                     "none"},
    {ConverterType_t::DeepgramWebVttConverter,  "DeepgramWebVttConverter"},
    {ConverterType_t::DeepgramSRTConverter,     "DeepgramWebVttConverter"}
};

#endif

#ifndef SEGMENTATION_MODE_DEFINED
#define SEGMENTATION_MODE_DEFINED
enum class SegmentationMode_e
{
    None = 0X4006000,
    UseLibrary,
    UseCLI
};

Q_ENUMS(SegmentationMode_e)

typedef SegmentationMode_e SegmentationMode_t;

inline bool operator==(const SegmentationMode_t type1, const SegmentationMode_t type2)
{
    return (static_cast<quint64> (type1) == static_cast<quint64> (type2));
}

inline uint qHash(const SegmentationMode_t type, uint seed = 0)
{
    return (::qHash(static_cast<quint64> (type), seed));
}
#endif

#ifndef DEEPGRAM_AIS2TT2S_MODELS_DEFINED
#define DEEPGRAM_AIS2TT2S_MODELS_DEFINED
enum class Deepgram_AIS2TT2SModels_e {
    NONE = 0X4007000,
    DeepgramT2SAura2ThaliaEN,
    DeepgramS2TNova,
    DeepgramS2TNovaGeneral,
    DeepgramS2TNova2,
    DeepgramS2TNova2GeneralPTBR,
    DeepgramS2TNova2GeneralEN,
    DeepgramS2TNova3,
    DeepgramS2TNova3General,
    DeepgramS2TEnhanced,
    DeepgramS2TEnhancedGeneral,
    DeepgramS2TBase,
    DeepgramS2TBaseGeneral,
    DeepgramS2TWhisperTiny,
    DeepgramS2TWhisperBase,
    DeepgramS2TWhisperSmall,
    DeepgramS2TWhisperMedium,
    DeepgramS2TWhisper,
    DeepgramS2TWhisperLarge
};

Q_ENUMS(Deepgram_AIS2TT2SModels_e)

typedef Deepgram_AIS2TT2SModels_e Deepgram_AIS2TT2SModels_t ;

inline bool operator==(const Deepgram_AIS2TT2SModels_t type1, const Deepgram_AIS2TT2SModels_t type2)
{
    return (static_cast<quint64> (type1) == static_cast<quint64> (type2));
}

inline uint qHash(const Deepgram_AIS2TT2SModels_t type, uint seed = 0)
{
    return (::qHash(static_cast<quint64> (type), seed));
}

static const QHash<Deepgram_AIS2TT2SModels_t, QString> DeepgramAIS2TT2SModelMap = {
    {Deepgram_AIS2TT2SModels_t::NONE,                           "nome"},
    {Deepgram_AIS2TT2SModels_t::DeepgramT2SAura2ThaliaEN,       "DeepgramT2SAura2ThaliaEN"},
    {Deepgram_AIS2TT2SModels_t::DeepgramS2TNova,                "DeepgramS2TNova"},
    {Deepgram_AIS2TT2SModels_t::DeepgramS2TNovaGeneral,         "DeepgramS2TNovaGeneral"},
    {Deepgram_AIS2TT2SModels_t::DeepgramS2TNova2,               "DeepgramS2TNova2"},
    {Deepgram_AIS2TT2SModels_t::DeepgramS2TNova2GeneralPTBR,    "DeepgramS2TNova2GeneralPTBR"},
    {Deepgram_AIS2TT2SModels_t::DeepgramS2TNova2GeneralEN,      "DeepgramS2TNova2GeneralEN"},
    {Deepgram_AIS2TT2SModels_t::DeepgramS2TNova3,               "DeepgramS2TNova3"},
    {Deepgram_AIS2TT2SModels_t::DeepgramS2TNova3General,        "DeepgramS2TNova3General"},
    {Deepgram_AIS2TT2SModels_t::DeepgramS2TEnhanced,            "DeepgramS2TEnhanced"},
    {Deepgram_AIS2TT2SModels_t::DeepgramS2TEnhancedGeneral,     "DeepgramS2TEnhancedGeneral"},
    {Deepgram_AIS2TT2SModels_t::DeepgramS2TBase,                "DeepgramS2TBase"},
    {Deepgram_AIS2TT2SModels_t::DeepgramS2TBaseGeneral,         "DeepgramS2TBaseGeneral"},
    {Deepgram_AIS2TT2SModels_t::DeepgramS2TWhisperTiny,         "DeepgramS2TWhisperTiny"},
    {Deepgram_AIS2TT2SModels_t::DeepgramS2TWhisperBase,         "DeepgramS2TWhisperBase"},
    {Deepgram_AIS2TT2SModels_t::DeepgramS2TWhisperSmall,        "DeepgramS2TWhisperSmall"},
    {Deepgram_AIS2TT2SModels_t::DeepgramS2TWhisperMedium,       "DeepgramS2TWhisperMedium"},
    {Deepgram_AIS2TT2SModels_t::DeepgramS2TWhisper,             "DeepgramS2TWhisper"},
    {Deepgram_AIS2TT2SModels_t::DeepgramS2TWhisperLarge,        "DeepgramS2TWhisperLarge"}
};
#endif

#ifndef TRANSFORMATION_DEFINED
#define TRANSFORMATION_DEFINED
enum class Transformation_e {
    NONE = 0X4008000,
    Speech2Text,
    Text2Speech
};

Q_ENUMS(Transformation_e)

typedef Transformation_e Transformation_t ;

inline bool operator==(const Transformation_t type1, const Transformation_t type2)
{
    return (static_cast<quint64> (type1) == static_cast<quint64> (type2));
}

inline uint qHash(const Transformation_t type, uint seed = 0)
{
    return (::qHash(static_cast<quint64> (type), seed));
}
#endif

#ifndef AI_MODELS_DEFINED
#define AI_MODELS_DEFINED
enum class AIModels_e {
    Deepgram_API = 0X4009000,
    Deepseek_API,

    Google_Gemini_API_20FlashLite,
    Google_Gemini_API_20FlashLite001,
    Google_Gemini_API_20Flash,
    Google_Gemini_API_20Flash001,

    Google_Gemini_API_25Flash,
    Google_Gemini_API_25FlashLite,

    Google_Gemini_API_25Pro,

    Google_Gemini_API_3FlashPreview,

    Google_Gemini_API_31FlashLite,
    Google_Gemini_API_31FlashLitePreview,

    Google_Gemini_API_35Flash,

    Google_Gemini_API_FlashLiteLatest,
    Google_Gemini_API_FlashLatest,

    Google_Gemini_API_3ProPreview,

    Google_Gemini_API_31ProPreview,
    Google_Gemini_API_31ProPreviewCustomTools,

    Google_Gemini_API_ProLatest,

    Google_HuggingFace_API,
    GPT_4o_Mini_API,
    OpenAI_Google_API,
    OpenRouter_API_FreeModel,
    OpenRouter_API_Deepseekv2,
    OpenRouter_API_Deepseekv3Base,
    OpenRouter_API_DeepSeekDeepSeekV3,
    OpenRouter_API_GoogleGemini2_0FlashExperimental,
    OpenRouter_API_GoogleGemini2_5ProExperimental,
    OpenRouter_API_GoogleGemma3_12B,
    OpenRouter_API_GoogleGemma3_27B,
    OpenRouter_API_GPT_4o_Mini,
    OpenRouter_API_MetaLlama4Maverick,
    OpenRouter_API_MetaLlama4Scout,
    OpenRouter_API_MicrosoftMAIDSR1,
    OpenRouter_API_MistralMistralNemo,
    OpenRouter_API_Qwen3235BA22B,
    UserDefined_API,
    None
};

Q_ENUMS(AIModels_e)

typedef AIModels_e AIModels_t;

inline bool operator==(const AIModels_t type1, const AIModels_t type2)
{
    return (static_cast<quint64> (type1) == static_cast<quint64> (type2));
}

inline uint qHash(const AIModels_t type, uint seed = 0)
{
    return (::qHash(static_cast<quint64> (type), seed));
}

// Mapping table to eliminate the massive switch
struct AIModelConfig { QString name; Url url; QString key; };

static const QHash<AIModels_t, AIModelConfig> AIModelMap = {
    { AIModels_t::Google_Gemini_API_20FlashLite,                    { *Constants::Q_QGS_GoogleGemini20FlashLite_Str, *Constants::Q_QGS_GoogleGeminiAI_Url, *Constants::Q_QGS_GoogleGemini_API_Key_Str }},
    { AIModels_t::Google_Gemini_API_20FlashLite001,                 { *Constants::Q_QGS_GoogleGemini20FlashLite001_Str, *Constants::Q_QGS_GoogleGeminiAI_Url, *Constants::Q_QGS_GoogleGemini_API_Key_Str }},
    { AIModels_t::Google_Gemini_API_20Flash,                        { *Constants::Q_QGS_GoogleGemini20Flash_Str, *Constants::Q_QGS_GoogleGeminiAI_Url, *Constants::Q_QGS_GoogleGemini_API_Key_Str }},
    { AIModels_t::Google_Gemini_API_20Flash001,                     { *Constants::Q_QGS_GoogleGemini20Flash001_Str, *Constants::Q_QGS_GoogleGeminiAI_Url, *Constants::Q_QGS_GoogleGemini_API_Key_Str }},

    { AIModels_t::Google_Gemini_API_25Flash,                        { *Constants::Q_QGS_GoogleGemini25Flash_Str, *Constants::Q_QGS_GoogleGeminiAI_Url, *Constants::Q_QGS_GoogleGemini_API_Key_Str }},
    { AIModels_t::Google_Gemini_API_25FlashLite,                    { *Constants::Q_QGS_GoogleGemini25FlashLite_Str, *Constants::Q_QGS_GoogleGeminiAI_Url, *Constants::Q_QGS_GoogleGemini_API_Key_Str }},

    { AIModels_t::Google_Gemini_API_25Pro,                          { *Constants::Q_QGS_GoogleGemini25Pro_Str, *Constants::Q_QGS_GoogleGeminiAI_Url, *Constants::Q_QGS_GoogleGemini_API_Key_Str }},

    { AIModels_t::Google_Gemini_API_3FlashPreview,                  { *Constants::Q_QGS_GoogleGemini3FlashPreview_Str, *Constants::Q_QGS_GoogleGeminiAI_Url, *Constants::Q_QGS_GoogleGemini_API_Key_Str }},

    { AIModels_t::Google_Gemini_API_31FlashLite,                    { *Constants::Q_QGS_GoogleGemini31FlashLite_Str, *Constants::Q_QGS_GoogleGeminiAI_Url, *Constants::Q_QGS_GoogleGemini_API_Key_Str }},
    { AIModels_t::Google_Gemini_API_31FlashLitePreview,             { *Constants::Q_QGS_GoogleGemini31FlashLitePreview_Str, *Constants::Q_QGS_GoogleGeminiAI_Url, *Constants::Q_QGS_GoogleGemini_API_Key_Str }},

    { AIModels_t::Google_Gemini_API_35Flash,                        { *Constants::Q_QGS_GoogleGemini35Flash_Str, *Constants::Q_QGS_GoogleGeminiAI_Url, *Constants::Q_QGS_GoogleGemini_API_Key_Str }},

    { AIModels_t::Google_Gemini_API_FlashLiteLatest,                { *Constants::Q_QGS_GoogleGeminiFlashLiteLatest_Str, *Constants::Q_QGS_GoogleGeminiAI_Url, *Constants::Q_QGS_GoogleGemini_API_Key_Str }},
    { AIModels_t::Google_Gemini_API_FlashLatest,                    { *Constants::Q_QGS_GoogleGeminiFlashLatest_Str, *Constants::Q_QGS_GoogleGeminiAI_Url, *Constants::Q_QGS_GoogleGemini_API_Key_Str }},

    { AIModels_t::Google_Gemini_API_3ProPreview,                    { *Constants::Q_QGS_GoogleGemini3ProPreview_Str, *Constants::Q_QGS_GoogleGeminiAI_Url, *Constants::Q_QGS_GoogleGemini_API_Key_Str }},

    { AIModels_t::Google_Gemini_API_31ProPreview,                   { *Constants::Q_QGS_GoogleGemini31ProPreview_Str, *Constants::Q_QGS_GoogleGeminiAI_Url, *Constants::Q_QGS_GoogleGemini_API_Key_Str }},
    { AIModels_t::Google_Gemini_API_31ProPreviewCustomTools,        { *Constants::Q_QGS_GoogleGemini3ProPreviewCustomTools_Str, *Constants::Q_QGS_GoogleGeminiAI_Url, *Constants::Q_QGS_GoogleGemini_API_Key_Str }},

    { AIModels_t::Google_Gemini_API_ProLatest,                      { *Constants::Q_QGS_GoogleGeminiProLatest_Str, *Constants::Q_QGS_GoogleGeminiAI_Url, *Constants::Q_QGS_GoogleGemini_API_Key_Str }},

    { AIModels_t::Google_HuggingFace_API,                           { *Constants::Q_QGS_GPT_4o_MiniModel_Str, *Constants::Q_QGS_OpenRouterAI_Url, *Constants::Q_QGS_GoogleHuggingFace_API_Key_Str }},
    { AIModels_t::GPT_4o_Mini_API,                                  { *Constants::Q_QGS_GPT_4o_MiniModel_Str, *Constants::Q_QGS_OpenRouterAI_Url, *Constants::Q_QGS_GPT_4o_MiniKey_Str }},
    { AIModels_t::OpenRouter_API_FreeModel,                         { *Constants::Q_QGS_OpenRouterFreeModel_Str, *Constants::Q_QGS_OpenRouterAI_Url, *Constants::Q_QGS_OpenRouterKey_Str }},
    { AIModels_t::OpenRouter_API_Deepseekv2,                        { *Constants::Q_QGS_OpenRouter_Deepseekv2Model_Str, *Constants::Q_QGS_OpenRouterAI_Url, *Constants::Q_QGS_OpenRouterKey_Str }},
    { AIModels_t::OpenRouter_API_Deepseekv3Base,                    { *Constants::Q_QGS_OpenRouter_Deepseekv3BaseModel_Str, *Constants::Q_QGS_OpenRouterAI_Url, *Constants::Q_QGS_OpenRouterKey_Str }},
    { AIModels_t::OpenRouter_API_DeepSeekDeepSeekV3,                { *Constants::Q_QGS_OpenRouter_DeepSeekDeepSeekV3Model_Str, *Constants::Q_QGS_OpenRouterAI_Url, *Constants::Q_QGS_OpenRouterKey_Str }},
    { AIModels_t::OpenRouter_API_GoogleGemini2_0FlashExperimental,  { *Constants::Q_QGS_OpenRouter_GoogleGemini2_0FlashExperimentalModel_Str, *Constants::Q_QGS_OpenRouterAI_Url, *Constants::Q_QGS_OpenRouterKey_Str }},
    { AIModels_t::OpenRouter_API_GoogleGemini2_5ProExperimental,    { *Constants::Q_QGS_OpenRouter_GoogleGemini2_5ProExperimentalModel_Str, *Constants::Q_QGS_OpenRouterAI_Url, *Constants::Q_QGS_OpenRouterKey_Str }},
    { AIModels_t::OpenRouter_API_GoogleGemma3_12B,                  { *Constants::Q_QGS_OpenRouter_GoogleGemma3_12BModel_Str, *Constants::Q_QGS_OpenRouterAI_Url, *Constants::Q_QGS_OpenRouterKey_Str }},
    { AIModels_t::OpenRouter_API_GoogleGemma3_27B,                  { *Constants::Q_QGS_OpenRouter_GoogleGemma3_27BModel_Str, *Constants::Q_QGS_OpenRouterAI_Url, *Constants::Q_QGS_OpenRouterKey_Str }},
    { AIModels_t::OpenRouter_API_GPT_4o_Mini,                       { *Constants::Q_QGS_OpenRouter_GPT_4o_MiniModel_Str, *Constants::Q_QGS_OpenRouterAI_Url, *Constants::Q_QGS_OpenRouterKey_Str }},
    { AIModels_t::OpenRouter_API_MetaLlama4Maverick,                { *Constants::Q_QGS_OpenRouter_MetaLlama4Maverick2Model_Str, *Constants::Q_QGS_OpenRouterAI_Url, *Constants::Q_QGS_OpenRouterKey_Str }},
    { AIModels_t::OpenRouter_API_MetaLlama4Scout,                   { *Constants::Q_QGS_OpenRouter_MetaLlama4ScoutModel_Str, *Constants::Q_QGS_OpenRouterAI_Url, *Constants::Q_QGS_OpenRouterKey_Str }},
    { AIModels_t::OpenRouter_API_MicrosoftMAIDSR1,                  { *Constants::Q_QGS_OpenRouter_MicrosoftMAIDSR1Model_Str, *Constants::Q_QGS_OpenRouterAI_Url, *Constants::Q_QGS_OpenRouterKey_Str }},
    { AIModels_t::OpenRouter_API_MistralMistralNemo,                { *Constants::Q_QGS_OpenRouter_MistralMistralNemoModel_Str, *Constants::Q_QGS_OpenRouterAI_Url, *Constants::Q_QGS_OpenRouterKey_Str }},
    { AIModels_t::OpenRouter_API_Qwen3235BA22B,                     { *Constants::Q_QGS_OpenRouter_Qwen3235BA22BModel_Str, *Constants::Q_QGS_OpenRouterAI_Url, *Constants::Q_QGS_OpenRouterKey_Str }}
};

/**
 * @brief FileAPIMap
 */
static const QHash<AIModels_t, Url> FileAPIMap = {
    { AIModels_t::Google_Gemini_API_20FlashLite,                    *Constants::Q_QGS_GoogleGeminiFileAPI_Url},
    { AIModels_t::Google_Gemini_API_20FlashLite001,                 *Constants::Q_QGS_GoogleGeminiFileAPI_Url},
    { AIModels_t::Google_Gemini_API_20Flash,                        *Constants::Q_QGS_GoogleGeminiFileAPI_Url},
    { AIModels_t::Google_Gemini_API_20Flash001,                     *Constants::Q_QGS_GoogleGeminiFileAPI_Url},

    { AIModels_t::Google_Gemini_API_25Flash,                        *Constants::Q_QGS_GoogleGeminiFileAPI_Url},
    { AIModels_t::Google_Gemini_API_25FlashLite,                    *Constants::Q_QGS_GoogleGeminiFileAPI_Url},

    { AIModels_t::Google_Gemini_API_31FlashLite,                    *Constants::Q_QGS_GoogleGeminiFileAPI_Url},
    { AIModels_t::Google_Gemini_API_31FlashLitePreview,             *Constants::Q_QGS_GoogleGeminiFileAPI_Url},

    { AIModels_t::Google_Gemini_API_35Flash,                        *Constants::Q_QGS_GoogleGeminiFileAPI_Url},

    { AIModels_t::Google_Gemini_API_FlashLiteLatest,                *Constants::Q_QGS_GoogleGeminiFileAPI_Url},
    { AIModels_t::Google_Gemini_API_FlashLatest,                    *Constants::Q_QGS_GoogleGeminiFileAPI_Url},

    { AIModels_t::Google_Gemini_API_25Pro,                          *Constants::Q_QGS_GoogleGeminiFileAPI_Url},

    { AIModels_t::Google_Gemini_API_3FlashPreview,                  *Constants::Q_QGS_GoogleGeminiFileAPI_Url},
    { AIModels_t::Google_Gemini_API_3ProPreview,                    *Constants::Q_QGS_GoogleGeminiFileAPI_Url},

    { AIModels_t::Google_Gemini_API_31ProPreview,                   *Constants::Q_QGS_GoogleGeminiFileAPI_Url},
    { AIModels_t::Google_Gemini_API_31ProPreviewCustomTools,        *Constants::Q_QGS_GoogleGeminiFileAPI_Url},

    { AIModels_t::Google_Gemini_API_ProLatest,                      *Constants::Q_QGS_GoogleGeminiFileAPI_Url},

    { AIModels_t::Google_HuggingFace_API,                           *Constants::Q_QGS_GoogleGeminiFileAPI_Url},
    { AIModels_t::GPT_4o_Mini_API,                                  Url()},
    { AIModels_t::OpenRouter_API_FreeModel,                         Url()},
    { AIModels_t::OpenRouter_API_Deepseekv2,                        Url()},
    { AIModels_t::OpenRouter_API_Deepseekv3Base,                    Url()},
    { AIModels_t::OpenRouter_API_DeepSeekDeepSeekV3,                Url()},
    { AIModels_t::OpenRouter_API_GoogleGemini2_0FlashExperimental,  Url()},
    { AIModels_t::OpenRouter_API_GoogleGemini2_5ProExperimental,    Url()},
    { AIModels_t::OpenRouter_API_GoogleGemma3_12B,                  Url()},
    { AIModels_t::OpenRouter_API_GoogleGemma3_27B,                  Url()},
    { AIModels_t::OpenRouter_API_GPT_4o_Mini,                       Url()},
    { AIModels_t::OpenRouter_API_MetaLlama4Maverick,                Url()},
    { AIModels_t::OpenRouter_API_MetaLlama4Scout,                   Url()},
    { AIModels_t::OpenRouter_API_MicrosoftMAIDSR1,                  Url()},
    { AIModels_t::OpenRouter_API_MistralMistralNemo,                Url()},
    { AIModels_t::OpenRouter_API_Qwen3235BA22B,                     Url()}
};
#endif

/**
 * Json stuff
 */
//Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_AudioMP3MimeType_Str,  ("audio/mp3"));
//Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_AudioOggMimeType_Str,  ("audio/ogg"));
//Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_AudioWavMimeType_Str,  ("audio/wav"));
//Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_JsonMimeType_Str,      ("application/json"));
//Q_GLOBAL_STATIC_WITH_ARGS(const QString, Q_QGS_JsonCBORMimeType_Str,  ("application/cbor"));

#ifndef CONTENT_MIME_TYPE_DEFINED
#define CONTENT_MIME_TYPE_DEFINED
/**
 * @brief The FileMimeType enum
 *          This is for Deepgram supported audio types.
 */
enum class ContentMimeType_e {
    Unknown = 0X400A000,
    Application,
    ApplicationCBor,            // application/zip (Binary)
    ApplicationJson,            // application/json
    ApplicationOctetStream,     // application/octet-stream
    ApplicationPdf,             // application/pdf
    ApplicationZip,             // application/zip

    Audio,
    AudioAAC,                   // audio/aac
    AudioFLAC,                  // audio/flac
    AudioM4A,                   // audio/m4q
    AudioMP2,                   // audio/mp2
    AudioMP3,                   // audio/mp3
    AudioMP4,                   // audio/mp4
    AudioOgg,                   // audio/ogg
    AudioOpus,                  // audio/opus
    AudioPCM,                   // audio/pcm
    AudioWav,                   // audio/wav
    AudioWebM,                  // audio/webm

    Image,
    ImageGif,                   // image/gif
    ImageJpeg,                  // image/jpg
    ImagePng,                   // image/png
    ImageSVGXML,                // image/srg+xml
    ImageWebP,                  // image/webp

    Text,
    TextCss,                    // text/cxx
    TextCsv,                    // text/csv
    TextHtml,                   // text/html
    TextPlain,                  // text/plain
    TextXCPPSrc,                // text/x-c+src

    Video,
    VideoMP4,                   // video/mp4
    VideoMKV,                   // video/x-matroska (MKV)
    VideoQuicktime              // video/quicktime (MOV)
};

Q_ENUMS(ContentMimeType_e)

typedef ContentMimeType_e ContentMimeType_t;

inline bool operator==(const ContentMimeType_t type1, const ContentMimeType_t type2)
{
    return (static_cast<quint64> (type1) == static_cast<quint64> (type2));
}

inline uint qHash(const ContentMimeType_t type, uint seed = 0)
{
    return (::qHash(static_cast<quint64> (type), seed));
}

static const QHash<ContentMimeType_t, QString> ContentTypeMap = {
    {ContentMimeType_t::ApplicationJson,        *Constants::Q_QGS_URLContentTypeJason_Str},
    {ContentMimeType_t::ApplicationCBor,        *Constants::Q_QGS_URLContentTypeCBOR_Str},
    {ContentMimeType_t::ApplicationOctetStream, *Constants::Q_QGS_URLContentTypeOctetStream_Str},
    {ContentMimeType_t::ApplicationPdf,         *Constants::Q_QGS_URLContentTypePDF_Str},
    {ContentMimeType_t::ApplicationZip,         *Constants::Q_QGS_URLContentTypeZip_Str},
    {ContentMimeType_t::AudioAAC,               *Constants::Q_QGS_URLContentTypeAudioAAC_Str},
    {ContentMimeType_t::AudioFLAC,              *Constants::Q_QGS_URLContentTypeAudioFLAC_Str},
    {ContentMimeType_t::AudioM4A,               *Constants::Q_QGS_URLContentTypeAudioM4A_Str},
    {ContentMimeType_t::AudioMP2,               *Constants::Q_QGS_URLContentTypeAudioMP2_Str},
    {ContentMimeType_t::AudioMP3,               *Constants::Q_QGS_URLContentTypeAudioMP3_Str},
    {ContentMimeType_t::AudioMP4,               *Constants::Q_QGS_URLContentTypeAudioMP4_Str},
    {ContentMimeType_t::AudioOgg,               *Constants::Q_QGS_URLContentTypeAudioOgg_Str},
    {ContentMimeType_t::AudioOpus,              *Constants::Q_QGS_URLContentTypeAudioOpus_Str},
    {ContentMimeType_t::AudioPCM,               *Constants::Q_QGS_URLContentTypeAudioPCM_Str},
    {ContentMimeType_t::AudioWav,               *Constants::Q_QGS_URLContentTypeAudioWav_Str},
    {ContentMimeType_t::AudioWebM,              *Constants::Q_QGS_URLContentTypeAudioWebM_Str},
    {ContentMimeType_t::ImageGif,               *Constants::Q_QGS_URLContentTypeImageGif_Str},
    {ContentMimeType_t::ImageJpeg,              *Constants::Q_QGS_URLContentTypeImageJpg_Str},
    {ContentMimeType_t::ImagePng,               *Constants::Q_QGS_URLContentTypeImagePng_Str},
    {ContentMimeType_t::ImageSVGXML,            *Constants::Q_QGS_URLContentTypeImageSVGXML_Str},
    {ContentMimeType_t::ImageWebP,              *Constants::Q_QGS_URLContentTypeImageWebP_Str},
    {ContentMimeType_t::TextCss,                *Constants::Q_QGS_URLContentTypeTextCss_Str},
    {ContentMimeType_t::TextCsv,                *Constants::Q_QGS_URLContentTypeTextCsv_Str},
    {ContentMimeType_t::TextHtml,               *Constants::Q_QGS_URLContentTypeTextHtml_Str},
    {ContentMimeType_t::TextPlain,              *Constants::Q_QGS_URLContentTypeTextPlain_Str},
    {ContentMimeType_t::TextXCPPSrc,            *Constants::Q_QGS_URLContentTypeTextXCPPSrc_Str},
    {ContentMimeType_t::VideoMP4,               *Constants::Q_QGS_URLContentTypeVideoMP4_Str},
    {ContentMimeType_t::VideoMKV,               *Constants::Q_QGS_URLContentTypeVideoXMatroska_Str},
    {ContentMimeType_t::VideoQuicktime,         *Constants::Q_QGS_URLContentTypeVideoQuicktime_Str},
    {ContentMimeType_t::Unknown,                *Constants::Q_QGS_URLContentTypeUnknown_Str}
};
#endif

#ifndef HEADER_URL_CONETNT_TYPE_DEFINED
#define HEADER_URL_CONETNT_TYPE_DEFINED
enum class HeaderUrlContentType_e {
    NONE = 0X400B000,
    URLContentTypeUser,
    URLContentTypeAudioMP3,
    URLContentTypeAudioOgg,
    URLContentTypeAudioWav,
    URLContentTypeJason
};

Q_ENUMS(HeaderUrlContentType_e);

typedef HeaderUrlContentType_e HeaderUrlContentType_t;

inline bool operator==(const HeaderUrlContentType_t type1, const HeaderUrlContentType_t type2)
{
    return (static_cast<quint64> (type1) == static_cast<quint64> (type2));
}

inline uint qHash(const HeaderUrlContentType_t type, uint seed = 0)
{
    return (::qHash(static_cast<quint64> (type), seed));
}
#endif

#ifndef HEADER_TYPE_DEFINED
#define HEADER_TYPE_DEFINED
enum class HeaderType_e {
    Authorization = 0X400C000,
    GoogleAuthorisation,
    HttpReferer,
    X_Title,
    Content_Type
};

Q_ENUMS(HeadersType_e);

typedef HeaderType_e HeaderType_t;

inline bool operator==(const HeaderType_t type1, const HeaderType_t type2)
{
    return (static_cast<quint64> (type1) == static_cast<quint64> (type2));
}

inline uint qHash(const HeaderType_t type, uint seed = 0)
{
    return (::qHash(static_cast<quint64> (type), seed));
}

typedef QPair<HeaderType_t, QString> HeaderTuple;

typedef QVector<HeaderTuple> Headers;

Q_DECLARE_METATYPE(Headers)
Q_DECLARE_METATYPE(Headers *)
#endif

#ifndef MESSAGE_CONTENT_PART_TYPE_DEFINED
#define MESSAGE_CONTENT_PART_TYPE_DEFINED
/**
 * @brief The ContentPartType enum
 */
enum class MessageContentPartType_e {
    ImageContentPart = 0X400D000,
    TextContent,
};

Q_ENUMS(MessageContentPartType_e);

typedef MessageContentPartType_e MessageContentPartType_t;

inline bool operator==(const MessageContentPartType_t type1, const MessageContentPartType_t type2)
{
    return (static_cast<quint64> (type1) == static_cast<quint64> (type2));
}

inline uint qHash(const MessageContentPartType_t type, uint seed = 0)
{
    return (::qHash(static_cast<quint64> (type), seed));
}
#endif

#ifndef MESSAGE_CONTENT_TYPE_DEFINED
#define MESSAGE_CONTENT_TYPE_DEFINED
/**
 * @brief The ContentType enum
 */
enum class MessageContentType_e {
    Unknown = 0X400E000,
    ContentParts,
    String // String - This relates to type string of OpenRouter API for request command.
};

Q_ENUMS(MessageContentType_e);

typedef MessageContentType_e MessageContentType_t;

inline bool operator==(const MessageContentType_t type1, const MessageContentType_t type2)
{
    return (static_cast<quint64> (type1) == static_cast<quint64> (type2));
}

inline uint qHash(const MessageContentType_t type, uint seed = 0)
{
    return (::qHash(static_cast<quint64> (type), seed));
}
#endif

#ifndef CONTROLLER_FILE_TYPE_DEFINED
#define CONTROLLER_FILE_TYPE_DEFINED

enum class FileType_e {
    AnnotationFile = 0X400F000,
    AnnotationManifest,
    //
    AudioFile,
    AudioManifest,
    AudioSegmentsDir,
    //
    EventExtractionManifest,
    //
    SRTManifest,
    SRTTranscriptionFile,
    //
    SynchronizedManifest,
    //
    VideoFile,
    VideoManifest,
    VideoSegmentsDir,
    //
    VTTManifest,
    VTTTranscriptionFile,
    //
    Unknown
};

Q_ENUMS(FileType_e)

typedef FileType_e FileType_t;

inline bool operator==(const FileType_t type1, const FileType_t type2)
{
    return (static_cast<quint64> (type1) == static_cast<quint64> (type2));
}

inline uint qHash(const FileType_t type, uint seed = 0)
{
    return (::qHash(static_cast<quint64> (type), seed));
}

#endif

#ifndef FILE_INFO_DEFINED
#define FILE_INFO_DEFINED
struct FileInfo {
    Url url;
    FileType_t type;
    bool success;

    /**
     * @brief FileInfo
     */
    FileInfo() = default;

    /**
     * @brief FileInfo
     * @param p
     * @param t
     * @param s
     */
    FileInfo(const QString & p, FileType_t t, bool s = false) :
        url(p),
        type(t),
        success(s)
    {
        // TODO
    }

    /**
     * @brief FileInfo
     * @param u
     * @param t
     * @param s
     */
    FileInfo(const Url & u, FileType_t t, bool s = false) :
        url(u),
        type(t),
        success(s)
    {
        // TODO
    }
};

Q_DECLARE_METATYPE(FileInfo)
#endif

#ifndef PIPELINE_MODE_DEFINED
#define PIPELINE_MODE_DEFINED
enum class PipelineMode_e
{
    None,
    TranscodeOnly,
    TranscribeOnly,
    ConvertOnly,
    SegmentOnly,
    EvtsExtractionOnly,
    Pipepline,
    Experiment
};

Q_DECLARE_METATYPE(PipelineMode_e)

typedef PipelineMode_e PipelineMode_t;

inline bool operator==(const PipelineMode_t type1, const PipelineMode_t type2)
{
    return (static_cast<quint64> (type1) == static_cast<quint64> (type2));
}

inline uint qHash(const PipelineMode_t type, uint seed = 0)
{
    return (::qHash(static_cast<quint64> (type), seed));
}

static const QHash<PipelineMode_t, QString> PipelineModeMap = {
    {PipelineMode_t::None,                *Constants::Q_QGS_PipelineModeNone},
    {PipelineMode_t::TranscodeOnly,       *Constants::Q_QGS_PipelineModeTranscode},
    {PipelineMode_t::TranscribeOnly,      *Constants::Q_QGS_PipelineModeTranscribe},
    {PipelineMode_t::ConvertOnly,         *Constants::Q_QGS_PipelineModeConvert},
    {PipelineMode_t::SegmentOnly,         *Constants::Q_QGS_PipelineModeSegment},
    {PipelineMode_t::EvtsExtractionOnly,  *Constants::Q_QGS_PipelineModeEvtsExtraction},
    {PipelineMode_t::Pipepline,           *Constants::Q_QGS_PipelineModePipeline},
    {PipelineMode_t::Experiment,          *Constants::Q_QGS_PipelineModeExperiment}
};
#endif


#ifndef EXPERIMENT_MODE_DEFINED
#define EXPERIMENT_MODE_DEFINED
enum class ExperimentMode_e
{
    None,
    TokenValidation,
    BaselineGoverned,
    GovernanceAudit,
    ReportEventsFound,
    ReportEventsRejected,
    PrintReportGoverned,
    EventSpottingDomainAlignment,
};

Q_DECLARE_METATYPE(ExperimentMode_e)

typedef ExperimentMode_e ExperimentMode_t;

inline bool operator==(const ExperimentMode_t type1, const ExperimentMode_t type2)
{
    return (static_cast<quint64> (type1) == static_cast<quint64> (type2));
}

inline uint qHash(const ExperimentMode_t type, uint seed = 0)
{
    return (::qHash(static_cast<quint64> (type), seed));
}

static const QHash<ExperimentMode_t, QString> ExperimentModeMap = {
    {ExperimentMode_t::None,                            *Constants::Q_QGS_ExperimentModeNone},
    {ExperimentMode_t::TokenValidation,                 *Constants::Q_QGS_ExperimentModeTokenValidation},
    {ExperimentMode_t::BaselineGoverned,                *Constants::Q_QGS_ExperimentBaselineGoverned},
    {ExperimentMode_t::GovernanceAudit,                 *Constants::Q_QGS_ExperimentGovernanceAudit},
    {ExperimentMode_t::ReportEventsFound,               *Constants::Q_QGS_ExperimentReportEventsFound},
    {ExperimentMode_t::ReportEventsRejected,            *Constants::Q_QGS_ExperimentReportEventsRejected},
    {ExperimentMode_t::PrintReportGoverned,             *Constants::Q_QGS_ExperimentPrintReportGoverned},
    {ExperimentMode_t::EventSpottingDomainAlignment,    *Constants::Q_QGS_ExperimentModeEventSpottingDomainAlignment}
};
#endif


#ifndef KEYWORD_MATCH_TYPE_DEFINED
#define KEYWORD_MATCH_TYPE_DEFINED
enum class KeywordMatchType_e {
    None,
    Any, // Returns true if ANY of the search terms are found
    All,  // Returns true only if ALL search terms are found
    Exact
};

Q_DECLARE_METATYPE(KeywordMatchType_e)

typedef KeywordMatchType_e KeywordMatchType_t;

inline bool operator==(const KeywordMatchType_t type1, const KeywordMatchType_t type2)
{
    return (static_cast<quint64> (type1) == static_cast<quint64> (type2));
}

inline uint qHash(const KeywordMatchType_t type, uint seed = 0)
{
    return (::qHash(static_cast<quint64> (type), seed));
}

static const QHash<KeywordMatchType_t, QString> KeywordMatchTypeMap = {
    {KeywordMatchType_t::Any, "any"},
    {KeywordMatchType_t::All, "all"},
    {KeywordMatchType_t::Exact, "exact"}
};
#endif

#ifndef AI_PROMPT_DEFINED
#define AI_PROMPT_DEFINED

/**
 * @brief The Persona enum. Enum to keep track of different
 *        AI personas
 */
enum class Persona {
    None = 0X400,
    FIFA_Referee,
    Technical_Analyst,
    Fan_Commentator
};

inline bool operator==(const Persona type1, const Persona type2)
{
    return (static_cast<quint64> (type1) == static_cast<quint64> (type2));
}

inline uint qHash(const Persona type, uint seed = 0)
{
    return (::qHash(static_cast<quint64> (type), seed));
}

static const QHash<Persona, QString> PersonaTypeMap = {
    {Persona::None, "none"},
    {Persona::FIFA_Referee, "fifa_referee"},
    {Persona::Technical_Analyst, "technical_analysis"},
    {Persona::Fan_Commentator, "fan_commentator"}
};

/**
 * @brief The TaskTemplate enum. Enum to keep track of different
 *        AI task/user prompts
 */
enum class TaskTemplate {
    None = 0X410,
    Video_Analysis_Task,
    Video_Baseline_Analysis_Task
};

inline bool operator==(const TaskTemplate type1, const TaskTemplate type2)
{
    return (static_cast<quint64> (type1) == static_cast<quint64> (type2));
}

inline uint qHash(const TaskTemplate type, uint seed = 0)
{
    return (::qHash(static_cast<quint64> (type), seed));
}

static const QHash<TaskTemplate, QString> TaskTemplateTypeMap = {
    {TaskTemplate::None, "none"},
    {TaskTemplate::Video_Analysis_Task, "video_analysis_task"},
    {TaskTemplate::Video_Baseline_Analysis_Task, "video_baseline_analysus_task"}
};

#endif


Q_DECLARE_METATYPE(QVector<QString>)
Q_DECLARE_METATYPE(QVector<QString> *)

Q_DECLARE_METATYPE(std::vector<std::string>)
Q_DECLARE_METATYPE(std::vector<std::string> *)

#endif // DEFS_H
