#-------------------------------------------------
#
# Project created by QtCreator 2025-08-13T18:59:38
#
#-------------------------------------------------

TEMPLATE = lib

TARGET = Utils

VERSION = 1.0.0

include(../common.pri)

DEFINES += UTILS_LIBRARY

# ----------------------------------------------------------------------
# Qt Modules & C++ Standard
# ----------------------------------------------------------------------
# Note: Keep only the modules actually required by Utils.
# (If webengine/opengl are required, QT -= gui must be removed)
QT += core network sql xml
lessThan(QT_MAJOR_VERSION, 6): QT += xmlpatterns
QT -= gui

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

# ----------------------------------------------------------------------
# Build Configuration (Debug / Release)
# ----------------------------------------------------------------------
## Defined in ../common.pri
DESTDIR = $$LIB_DESTDIR

CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,, -debug)
    LIBLOGGER = Logger-debug
} else {
    LIBLOGGER = Logger
    DEFINES += QT_NO_DEBUG_OUTPUT \
               QT_NO_INFO_OUTPUT \
               QT_NO_WARNING_OUTPUT #\
               # QT_MESSAGELOGCONTEXT
}

# ----------------------------------------------------------------------
# Internal Dependency: Logger
# ----------------------------------------------------------------------
INCLUDEPATH += $$PWD/../Logger/inc
DEPENDPATH  += $$PWD/../Logger/inc

LIBS += -L$$LIB_DESTDIR \
        -l$${LIBLOGGER}

# ----------------------------------------------------------------------
# Platform: macOS Configuration
# ----------------------------------------------------------------------
macx {
    # Ensure this library looks in its own directory and standard locations for Logger
    # QMAKE_LFLAGS += -Wl,-rpath,@loader_path/ \
    #                 -Wl,-rpath,@executable_path/../Frameworks \
    #                 -Wl,-rpath,$$LIB_DESTDIR

    # Use QMAKE_RPATHDIR to let qmake deduplicate rpaths cleanly
    QMAKE_RPATHDIR += @loader_path/ \
                      $$LIB_DESTDIR
}

# ----------------------------------------------------------------------
# Installation Targets (POSIX / Unix / macOS). Supports PREFIX="" and INSTALL_ROOT=stage-*
# ----------------------------------------------------------------------
unix {
    # ------------------------------------------------------------------
    # 1. Define Base Install Prefix (Override via command line if needed)
    #    Usage: qmake PREFIX=/opt/local Logger.pro
    # ------------------------------------------------------------------
    !defined(PREFIX, var) {
        # Default to /usr/local for macOS command-line/dev builds
        macx: PREFIX = /usr/local
        else: PREFIX = /usr
    }

    target.path  = $$PREFIX/lib
    headers.path = $$PREFIX/include/Utils

    # ------------------------------------------------------------------
    # 2. Public Header Files
    # ------------------------------------------------------------------
    headers.files = $$HEADERS

    # ------------------------------------------------------------------
    # 3. Main Library Target
    # ------------------------------------------------------------------
    INSTALLS += target headers

    # ------------------------------------------------------------------
    # 4. Optional: App Bundle Frameworks / Embedding
    #    Usage: qmake BUNDLE_PATH=/path/to/MyConsumerApp.app Logger.pro
    # ------------------------------------------------------------------
    !isEmpty(BUNDLE_PATH) {
        bundle_fw.path  = $$BUNDLE_PATH/Contents/Frameworks
        bundle_fw.files = $$DESTDIR/lib$${TARGET}*
        INSTALLS += bundle_fw
    }

    # ------------------------------------------------------------------
    # 5. pkg-config Metadata generation (Clean linking for other tools)
    # ------------------------------------------------------------------
    CONFIG += create_pc create_prl no_install_prl
    QMAKE_PKGCONFIG_NAME        = Utils
    QMAKE_PKGCONFIG_DESCRIPTION = Application Utils Dynamic Library
    QMAKE_PKGCONFIG_PREFIX      = $$PREFIX
    QMAKE_PKGCONFIG_LIBDIR      = $$PREFIX/lib
    QMAKE_PKGCONFIG_INCDIR      = $$PREFIX/include
    QMAKE_PKGCONFIG_DESTDIR     = pkgconfig

    pkgconfig.path = $$PREFIX/lib/pkgconfig
    pkgconfig.files = $$OUT_PWD/pkgconfig/Utils.pc
    INSTALLS += pkgconfig
}
