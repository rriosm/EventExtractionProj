#-------------------------------------------------
#
# Project created by QtCreator 2025-03-07T17:29:47
#
#-------------------------------------------------

TEMPLATE = app

TARGET = EventExtractor

include(../common.pri)

# ----------------------------------------------------------------------
# Qt Modules & C++ Standard
# ----------------------------------------------------------------------
# Headless console service: uses core, concurrent, sql, and network
QT += core concurrent sql widgets gui

# If automated unit tests in tests/ are compiled into this binary:
# QT += testlib

CONFIG += console
CONFIG -= app_bundle

DESTDIR = $$BIN_DESTDIR

# # ----------------------------------------------------------------------
# # Conditional GUI Configuration
# # ----------------------------------------------------------------------
# CONFIG(build_gui) {
#     message("Building EventExtractor with GUI (QtWidgets)")
#     QT += widgets gui
#     DEFINES += ENABLE_GUI

#     # Build as a standard macOS .app bundle
#     CONFIG += app_bundle
#     CONFIG -= console
# } else {
#     message("Building EventExtractor as headless CLI")
#     QT -= widgets gui
#     DEFINES += CLI_MODE

#     # Headless macOS command-line executable
#     CONFIG += console
#     CONFIG -= app_bundle
# }

# ----------------------------------------------------------------------
# Conditional Compilation: testlib & Unit Tests
# ----------------------------------------------------------------------
# CONFIG += build_tests

CONFIG(build_tests) {
    message("Unit tests enabled: including QtTest and test suites")
    QT += testlib
    DEFINES += BUILD_TESTS

    INCLUDEPATH += $$PWD/tests/inc

    HEADERS += \
        tests/inc/AIModelsTest.h \
        tests/inc/ChatHistory-Test.h \
        tests/inc/VideoSegmentaInfoTest.h

    SOURCES += \
        tests/src/UploadManagerTest.cpp \
        tests/src/VideoSegmentaInfoTest.cpp
} else {
    message("Unit tests disabled: building core application only")
}

# ----------------------------------------------------------------------
# Build Configuration (Debug / Release)
# ----------------------------------------------------------------------
CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,, -debug)
    LIBLOGGER = Logger-debug
    LIBUTILS  = Utils-debug
    DEFINES  += QT_DEBUG DEBUG_MODE USE_LIBCURL
} else {
    LIBLOGGER = Logger
    LIBUTILS  = Utils
    DEFINES  += QT_NO_DEBUG_OUTPUT \
                QT_NO_INFO_OUTPUT \
                QT_NO_WARNING_OUTPUT #\
                # QT_LOGMESSAGELOGCONTEXT
}

# ----------------------------------------------------------------------
# Compiler & Platform Settings (macOS)
# ----------------------------------------------------------------------
macx {
    # Allow command line override: qmake FFMPEG_PREFIX=/custom/path
    isEmpty(FFMPEG_PREFIX) {
        # MacPorts Paths for FFmpeg 7
        exists(/opt/local/libexec/ffmpeg7/include) {
            FFMPEG_PREFIX = /opt/local/libexec/ffmpeg7
        } else:exists(/opt/homebrew) {
            FFMPEG_PREFIX = /opt/homebrew
        } else {
            FFMPEG_PREFIX = /usr/local
        }
    }

    # Put FFmpeg 7 headers and libs FIRST
    INCLUDEPATH  = $${FFMPEG_PREFIX}/include $$INCLUDEPATH

    ## ld may fail to resolve Ffmpeg7 symbols due to Apple's Clang linkger resolves
    ## -L paths from left to right. To fix this, is to pass the absolute library
    ## file paths directly instead of using -L... -l...
    # LIBS         += -L$${FFMPEG_PREFIX}/lib \
    #                 -lavcodec -lavformat -lavfilter -lavutil -lswresample \
    #                 $$LIBS

    ## The fix!
    LIBS += -L$${FFMPEG_PREFIX}/lib \
            -lavcodec -lavformat -lavfilter -lavutil -lswresample
            # $${FFMPEG_PREFIX}/lib/libavcodec.dylib \
            # $${FFMPEG_PREFIX}/lib/libavformat.dylib \
            # $${FFMPEG_PREFIX}/lib/libavfilter.dylib \
            # $${FFMPEG_PREFIX}/lib/libavutil.dylib \
            # $${FFMPEG_PREFIX}/lib/libswresample.dylib
} else:unix {
    CONFIG += link_pkgconfig
    PKGCONFIG += libavcodec libavformat libavfilter libavutil libswresample
} else:win32 {
    LIBS += -lavcodec -lavformat -lavfilter -lavutil -lswresample
}

# ======================================================================
# HTTP Backend Configuration
# Uncomment to compile using libcurl; comment out to use QNetworkAccessManager
# ======================================================================
CONFIG += use_libcurl

# ----------------------------------------------------------------------
# Conditional Compilation & Module Setup
# ----------------------------------------------------------------------
CONFIG(use_libcurl) {
    message("HTTP backend: libcurl")
    DEFINES += USE_LIBCURL

    macx {
        # Add general MacPorts paths ONLY for curl, appended at the end
        exists(/opt/local/include/curl) {
            INCLUDEPATH += /opt/local/include
            LIBS        += -L/opt/local/lib -lcurl
        } else:exists(/opt/homebrew/include/curl) {
            INCLUDEPATH += /opt/homebrew/include
            LIBS        += -L/opt/homebrew/lib -lcurl
        } else {
            LIBS += -lcurl
        }
    } else:unix {
        # Linux / standard POSIX
        CONFIG += link_pkgconfig
        PKGCONFIG += libcurl
    } else:win32 {
        LIBS += -lcurl
    }
} else {
    message("HTTP backend: Qt QNetworkAccessManager")
    QT += network
}

# ----------------------------------------------------------------------
# Internal Dependencies: Logger & Utils (Resolves against Staging)
# ----------------------------------------------------------------------
INCLUDEPATH += $$PWD/inc \
               $$PWD/../Logger/inc \
               $$PWD/../Utils/inc

LIBS += -L$$LIB_DESTDIR -l$${LIBLOGGER} -l$${LIBUTILS}

# Embed relative rpaths so the binary resolves libraries placed next to it or in ../lib
macx {
    QMAKE_RPATHDIR += $$LIB_DESTDIR \
                      @executable_path/ \
                      @executable_path/../lib \
                      @loader_path/
} else:unix {
    QMAKE_RPATHDIR += $$LIB_DESTDIR \
                      \$\$ORIGIN \
                      \$\$ORIGIN/../lib
}

# ----------------------------------------------------------------------
# Project Files & Directories
# ----------------------------------------------------------------------
INCLUDEPATH += $$PWD/inc

HEADERS += \
    inc/Actor.h \
    inc/Audio.h \
    inc/AudioAnnotationSegmentationController.h \
    inc/AudioAnnotationSegmentationWorker.h \
    inc/AudioModel.h \
    inc/AudioSegmentationController.h \
    inc/AudioSegmentationWorker.h \
    inc/AudioTranscriptionController.h \
    inc/AudioTranscriptionWorker.h \
    inc/AudioTranscriptionSegmentationController.h \
    inc/AudioTranscriptionSegmentationWorker.h \
    inc/CommandLineDefs.h \
    inc/ConverterController.h \
    inc/ConverterWorker.h \
    inc/Defs.h \
    inc/EventDatabaseManager.h \
    inc/EventExtractionController.h \
    inc/EventExtractionWorker.h \
    inc/EventModel.h \
    inc/ExtractorUtils.h \
    inc/FullVideo.h \
    inc/IAudioSegmentationWorker.h \
    inc/IVideoSegmentationWorker.h \
    inc/KeywordSearchEngine.h \
    inc/Location.h \
    inc/LogUtils.h \
    inc/ManifestSynchronizer.h \
    inc/Message.h \
    inc/MessageHistory.h \
    inc/PipelineWorker.h \
    inc/PromptManager.h \
    inc/SettingsManager.h \
    inc/TranscodeController.h \
    inc/TranscodeWorker.h \
    inc/UploadManager.h \
    inc/VideoModelConfig.h \
    inc/VideoSegmentInfo.h \
    inc/VideoSegmentModel.h \
    inc/VideoSegmentationController.h \
    inc/VideoSegmentationWorker.h \
    inc/VideoSegmentationWorkerCLI.h \
    inc/VideoSegmentationWorkerFactory.h \
    inc/ffmpeg_gen.h \
    inc/BatchTokenEvaluator.h \
    inc/GovernanceAuditHarness.h \
    inc/TemporalAlignmentHarness.h
    # \
    # tests/inc/AIModelsTest.h \
    # tests/inc/ChatHistory-Test.h \
    # tests/inc/VideoSegmentaInfoTest.h \
#        \
#        inc/AudioEtiandRe.h

SOURCES += \
    src/ExtractorUtils.cpp \
    src/main.cpp \
    src/Actor.cpp \
    src/Audio.cpp \
    src/AudioAnnotationSegmentationController.cpp \
    src/AudioAnnotationSegmentationWorker.cpp \
    src/AudioModel.cpp \
    src/AudioSegmentationControler.cpp \
    src/AudioSegmentationWorker.cpp \
    src/AudioTranscriptionController.cpp \
    src/AudioTranscriptionWorker.cpp \
    src/AudioTranscriptionSegmentationController.cpp \
    src/AudioTranscriptionSegmentationWorker.cpp \
    src/ConverterController.cpp \
    src/ConverterWorker.cpp \
    src/EventDatabaseManager.cpp \
    src/EventExtractionController.cpp \
    src/EventExtractionWorker.cpp \
    src/EventModel.cpp \
    src/FullVideo.cpp \
    src/KeywordSearchEngine.cpp \
    src/Location.cpp \
    src/LogUtils.cpp \
    src/ManifestSynchronizer.cpp \
    src/Message.cpp \
    src/MessageHistory.cpp \
    src/PipelineWorker.cpp \
    src/PromptManager.cpp \
    src/SettingsManager.cpp \
    src/TranscodeController.cpp \
    src/TranscodeWorker.cpp \
    src/UploadManager.cpp \
    src/VideoModelConfig.cpp \
    src/VideoSegmentInfo.cpp \
    src/VideoSegmentModel.cpp \
    src/VideoSegmentationController.cpp \
    src/VideoSegmentationWorker.cpp \
    src/VideoSegmentationWorkerCLI.cpp \
    src/VideoSegmentationWorkerFactory.cpp \
    src/BatchTokenEvaluator.cpp \
    src/GovernanceAuditHarness.cpp \
    src/TemporalAlignmentHarness.cpp
    # \
    # tests/src/UploadManagerTest.cpp \
    # tests/src/VideoSegmentaInfoTest.cpp
    # \
    # src/AudioEtiandRe.cpp

RESOURCES += \
    EventExtractorResources.qrc

DISTFILES += \
    GeminiRequest.py \
    GeminiUtils.py \
    PythonFiles/WebVTTConverter.py \
    PythonFiles/mock_server.py \
    TestFiles/ai-query.json \
    TestFiles/response-test.json \
    TestFiles/brasil-whispersmall.json \
    TestFiles/brasil.json

# ----------------------------------------------------------------------
# Installation Target
# ----------------------------------------------------------------------
macx {
    !defined(PREFIX, var) {
        PREFIX = /usr/local
    }

    !isEmpty(PREFIX) {
        target.path = $$PREFIX/bin
    } else {
        target.path = /bin
    }

    INSTALLS += target
}
