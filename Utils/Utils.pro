#-------------------------------------------------
#
# Project created by QtCreator 2013-08-13T18:59:38
#
#-------------------------------------------------

TEMPLATE = lib

TARGET = Utils

VERSION = 1.0.0

DEFINES += UTILS_LIBRARY

# ----------------------------------------------------------------------
# Qt Modules & C++ Standard
# ----------------------------------------------------------------------
# Note: Keep only the modules actually required by Utils.
# (If webengine/opengl are required, QT -= gui must be removed)
QT += core network sql xml xmlpatterns
QT -= gui

CONFIG += c++11

# ----------------------------------------------------------------------
# Sources & Headers
# ----------------------------------------------------------------------
INCLUDEPATH += $$PWD/inc

HEADERS += \
    inc/MimeTypes.h \
    inc/NamespaceDefs.h \
    inc/UniqueId.h \
    inc/Utils.h \
    inc/Utils_global.h \
    inc/UrlList.h \
    inc/Url.h \
    inc/DownloadManager.h

SOURCES += \
    src/MimeTypes.cpp \
    src/UniqueId.cpp \
    src/UrlList.cpp \
    src/Utils.cpp \
    src/Url.cpp \
    src/DownloadManager.cpp

# Keep generated files organized outside the source tree
OBJECTS_DIR = $$OUT_PWD/.obj
MOC_DIR     = $$OUT_PWD/.moc
RCC_DIR     = $$OUT_PWD/.rcc
UI_DIR      = $$OUT_PWD/.ui

# ----------------------------------------------------------------------
# Warnings & Deprecations
# ----------------------------------------------------------------------
# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# ----------------------------------------------------------------------
# Build Configuration (Debug / Release)
# ----------------------------------------------------------------------
CONFIG(debug, debug|release) {
    message("debug mode")
    TARGET = $$join(TARGET,,, -debug)
    DESTDIR = $$OUT_PWD/../lib/Debug
    LIBLOGGER = Logger-debug
} else {
    DESTDIR = $$OUT_PWD/../lib/Release
    LIBLOGGER = Logger
    DEFINES += QT_NO_DEBUG_OUTPUT \
               QT_NO_INFO_OUTPUT \
               QT_NO_WARNING_OUTPUT \
               QT_MESSAGELOGCONTEXT
}

# ----------------------------------------------------------------------
# Dependency: Logger Resolution
# ----------------------------------------------------------------------
# Priority order:
# 1. User-specified LOGGER_DIR (qmake LOGGER_DIR=/path/to/stage)
# 2. Local sibling build staging directory
# 3. System prefix (/usr/local or /opt/local)

isEmpty(LOGGER_DIR) {
    CONFIG(debug, debug|release) {
        LOGGER_DIR = $$PWD/../Logger/build/Qt_$$QT_VERSION/Debug/stage-debug
    } else {
        LOGGER_DIR = $$PWD/../Logger/build/Qt_$$QT_VERSION/Release/stage-release
    }
}

exists($$LOGGER_DIR/include/Logger/Logger.h) {
    INCLUDEPATH += $$LOGGER_DIR/include \
                   $$LOGGER_DIR/include/Logger
    DEPENDPATH  += $$LOGGER_DIR/include/Logger

    LIBS += -L$$LOGGER_DIR/lib -l$${LIBLOGGER}
} else {
    # Fallback to direct sibling source tree or system directories
    INCLUDEPATH += $$PWD/../Logger/inc
    DEPENDPATH  += $$PWD/../Logger/inc

    CONFIG(debug, debug|release) {
        LIBS += -L$$PWD/../Logger/build/Qt_$$QT_VERSION/Debug/stage-debug/lib \
                -L$$PWD/../Logger/build/Qt_$$QT_VERSION/Debug/lib/Debug \
                -l$${LIBLOGGER}
    } else {
        LIBS += -L$$PWD/../Logger/build/Qt_$$QT_VERSION/Release/stage-release/lib \
                -L$$PWD/../Logger/build/Qt_$$QT_VERSION/Release/lib/Release \
                -l$${LIBLOGGER}
    }
}

# ----------------------------------------------------------------------
# Platform: macOS Configuration
# ----------------------------------------------------------------------
macx {
    # Set library ID to @rpath so consumers can relocate it cleanly
    QMAKE_SONAME_PREFIX = @rpath
    CONFIG += lib_version_first

    # Let Xcode resolve the installed SDK dynamically via the unversioned alias.
    # Apple's clang & xcrun map 'macosx' to whatever active macOS SDK is installed
    # (in your case, Xcode 11.7's macOS 10.15 SDK).
    QMAKE_MAC_SDK = macosx

    # Suppress version mismatch warnings between older Qt mkspecs and newer Xcode SDKs
    # When using Qt 5 with newer Xcode installations, Qt's internal mkspec feature tests
    # (mac/sdk.prf) often throw errors or warnings if the detected SDK version is higher
    # than what that Qt version officially recognized. Enabling CONFIG += sdk_no_version_check
    # bypasses this check.
    CONFIG += sdk_no_version_check

    # Deployment Target (Minimum OS version required at runtime)
    # Kept independent of the build SDK
    # Leaving QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.15 ensures that even when compiling with a
    # newer SDK (like macOS 14/15 SDK), Clang targets the 10.15 ABI and links against weak
    # symbols for newer APIs.
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.15

    QMAKE_CXXFLAGS += -Wno-deprecated-declarations \
                      -Wno-ambiguous-member-template

    # Ensure this library looks in its own directory and standard locations for Logger
    QMAKE_LFLAGS += -Wl,-rpath,@loader_path/ -Wl,-rpath,@executable_path/../Frameworks
}

# ----------------------------------------------------------------------
# Installation Targets (POSIX / Unix / macOS). Supports PREFIX="" and INSTALL_ROOT=stage-*
# ----------------------------------------------------------------------
macx {
    !defined(PREFIX, var) {
        macx: PREFIX = /usr/local
        else: PREFIX = /usr
    }

    !isEmpty(PREFIX) {
        target.path  = $$PREFIX/lib
        headers.path = $$PREFIX/include/Utils
    } else {
        target.path  = /lib
        headers.path = /include/Utils
    }

    headers.files = $$HEADERS
    INSTALLS += target headers

    !isEmpty(BUNDLE_PATH) {
        bundle_fw.path  = $$BUNDLE_PATH/Contents/Frameworks
        bundle_fw.files = $$DESTDIR/lib$${TARGET}*
        INSTALLS += bundle_fw
    }
}
