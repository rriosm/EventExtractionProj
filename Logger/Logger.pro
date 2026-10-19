#-------------------------------------------------
#
# Project created by QtCreator 2025-03-07T17:29:47
#
#-------------------------------------------------

#----------------------------------------------------------------------
# Project Configuration
#----------------------------------------------------------------------
TEMPLATE = lib

TARGET = Logger

VERSION = 1.0.0

include(../common.pri)

# Export macro required by inc/Logger_global.h
DEFINES += LOGGER_LIBRARY

# ----------------------------------------------------------------------
# Qt Modules & C++ Standard
# ----------------------------------------------------------------------
QT += core
QT -= gui

# ----------------------------------------------------------------------
# Sources & Headers
# ----------------------------------------------------------------------
INCLUDEPATH += $$PWD/inc

HEADERS += \
    inc/Logger_global.h \
    inc/Logger.h

SOURCES += \
    src/Logger.cpp

# ----------------------------------------------------------------------
# Build Configurations (Debug / Release)
# ----------------------------------------------------------------------
## Defined in ../common.pri
DESTDIR = $$LIB_DESTDIR

CONFIG(debug, debug|release) {
    # Distinguish debug binary name (e.g., libLogger-debug.dylib / libLogger-debug.so)
    TARGET = $$join(TARGET,,, -debug)
    DEFINES += DEBUG_MODE
} else {
    # Suppress console logging in release builds
    DEFINES += QT_NO_DEBUG_OUTPUT \
               QT_NO_INFO_OUTPUT \
               QT_NO_WARNING_OUTPUT # \
               # QT_MESSAGELOGCONTEXT
}

HDR_DEST = $$INC_DESTDIR/Logger
!exists($$HDR_DEST): mkpath($$HDR_DEST)

# Ensure the directory exists at make-time, then copy headers
QMAKE_POST_BUILD += $$QMAKE_MKDIR $$shell_path($$HDR_DEST) $$escape_expand(\n\t)

for(header, HEADERS) {
    QMAKE_POST_BUILD += $$QMAKE_COPY $$shell_path($$PWD/$$header) $$shell_path($$HDR_DEST) $$escape_expand(\n\t)
}

#----------------------------------------------------------------------
# Installation & Distribution Rules (POSIX / Unix / macOS)
#----------------------------------------------------------------------
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

    target.path = $$PREFIX/lib
    headers.path = $$PREFIX/include/Logger

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
        bundle_fw.path = $$BUNDLE_PATH/Contents/Frameworks
        bundle_fw.files = $$DESTDIR/lib$${TARGET}*
        INSTALLS += bundle_fw
    }

    # ------------------------------------------------------------------
    # 5. pkg-config Metadata generation (Clean linking for other tools)
    # ------------------------------------------------------------------
    CONFIG += create_pc create_prl no_install_prl
    QMAKE_PKGCONFIG_NAME        = Logger
    QMAKE_PKGCONFIG_DESCRIPTION = Application Logger Dynamic Library
    QMAKE_PKGCONFIG_PREFIX      = $$PREFIX
    QMAKE_PKGCONFIG_LIBDIR      = $$PREFIX/lib
    QMAKE_PKGCONFIG_INCDIR      = $$PREFIX/include
    QMAKE_PKGCONFIG_DESTDIR     = pkgconfig

    pkgconfig.path = $$PREFIX/lib/pkgconfig
    pkgconfig.files = $$OUT_PWD/pkgconfig/Logger.pc
    INSTALLS += pkgconfig
}
