#-------------------------------------------------
#
# Project created by QtCreator 2025-03-07T17:29:47
#
#-------------------------------------------------

# Read the document at https://doc.qt.io/qt-5/debug.html

# macx {
#     QMAKE_MAC_SDK = macosx
#     # QMAKE_MAC_SDK = macosx10.15
#     # QMAKE_MAC_SDK = macosx14.5
#     # CONFIG += sdk_no_version_check

#     # 1. Deployment Target (OS Compatibility)
#     QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.15

#     # 2. Relax the compiler (Compiler Compatibility)
#     # This disables the specific warning about strict flags that broke your QIODevice code
#     # QMAKE_CXXFLAGS += -Wno-deprecated-declarations -Wno-ambiguous-member-template
# }

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
} else {
    # Suppress console logging in release builds
    DEFINES += QT_NO_DEBUG_OUTPUT \
               QT_NO_INFO_OUTPUT \
               QT_NO_WARNING_OUTPUT # \
               # QT_MESSAGELOGCONTEXT
}

# ----------------------------------------------------------------------
# macOS Specific Toolchain & Runtime Settings
# ----------------------------------------------------------------------
# macx {
#     # Optional: Log the exact SDK path and version to Qt Creator compile output
#     _SDK_PATH = $$system(/usr/bin/xcrun --sdk macosx --show-sdk-path 2>/dev/null)
#     _SDK_VER  = $$system(/usr/bin/xcrun --sdk macosx --show-sdk-version 2>/dev/null)

#     !isEmpty(_SDK_VER) {
#         message("Building with macOS SDK: $$_SDK_VER at $$_SDK_PATH")
#     }

#     # Optional: Resolve the exact SDK filesystem sysroot path
#     QMAKE_MAC_SDK_PATH = $$system(/usr/bin/xcrun --sdk macosx --show-sdk-path 2>/dev/null)

#     !isEmpty(QMAKE_MAC_SDK_PATH) {
#         message("SDK Path: $$QMAKE_MAC_SDK_PATH")
#     }
# }

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
