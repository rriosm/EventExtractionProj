#-------------------------------------------------
#
# Project created by QtCreator 2013-03-07T17:29:47
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

# Export macro required by inc/Logger_global.h
DEFINES += LOGGER_LIBRARY

# ----------------------------------------------------------------------
# Qt Modules & C++ Standard
# ----------------------------------------------------------------------
QT += core
QT -= gui

# Modern C++ standard (use c++11 if your toolchain allows)
CONFIG += c++11

# ----------------------------------------------------------------------
# Sources & Headers
# ----------------------------------------------------------------------
INCLUDEPATH += $$PWD/inc

HEADERS += \
    inc/Logger_global.h \
    inc/Logger.h

SOURCES += \
    src/Logger.cpp

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
# Build Configurations (Debug / Release)
# ----------------------------------------------------------------------
CONFIG(debug, debug|release) {
    # Distinguish debug binary name (e.g., libLogger-debug.dylib / libLogger-debug.so)
    TARGET = $$join(TARGET,,, -debug)
    DESTDIR = $$OUT_PWD/../lib/Debug
} else {
    DESTDIR = $$OUT_PWD/../lib/Release
    # Suppress console logging in release builds
    DEFINES += QT_NO_DEBUG_OUTPUT \
               QT_NO_INFO_OUTPUT \
               QT_NO_WARNING_OUTPUT
               # \
               # QT_MESSAGELOGCONTEXT
}

# ----------------------------------------------------------------------
# macOS Specific Toolchain & Runtime Settings
# ----------------------------------------------------------------------
macx {
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

    # Compiler warnings / flags
    QMAKE_CXXFLAGS += -Wno-deprecated-declarations -Wno-ambiguous-member-template

    # Optional: Log the exact SDK path and version to Qt Creator compile output
    _SDK_PATH = $$system(/usr/bin/xcrun --sdk macosx --show-sdk-path 2>/dev/null)
    _SDK_VER  = $$system(/usr/bin/xcrun --sdk macosx --show-sdk-version 2>/dev/null)

    !isEmpty(_SDK_VER) {
        message("Building with macOS SDK: $$_SDK_VER at $$_SDK_PATH")
    }

    # Optional: Resolve the exact SDK filesystem sysroot path
    QMAKE_MAC_SDK_PATH = $$system(/usr/bin/xcrun --sdk macosx --show-sdk-path 2>/dev/null)

    !isEmpty(QMAKE_MAC_SDK_PATH) {
        message("SDK Path: $$QMAKE_MAC_SDK_PATH")
    }
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

    # Ensure clean path concatenation without trailing slashes
    !isEmpty(PREFIX) {
        target.path = $$PREFIX/lib
        headers.path = $$PREFIX/include/Logger
    } else {
        # Flat staging: install directly to /lib and /include
        target.path = /lib
        headers.path = /include/Logger
    }

    # ------------------------------------------------------------------
    # 3. Public Header Files
    # ------------------------------------------------------------------
    headers.files = $$HEADERS

    # ------------------------------------------------------------------
    # 2. Main Library Target
    # ------------------------------------------------------------------
    INSTALLS += target headers

    # # ------------------------------------------------------------------
    # # 4. Optional: Staging target for local workspace distribution
    # #    Install build artifacts into a centralized project build folder
    # # ------------------------------------------------------------------
    # stage_lib.path  = $$PWD/../dist/lib
    # stage_lib.files = $$DESTDIR/lib$${TARGET}*

    # stage_headers.path  = $$PWD/../dist/include/Logger
    # stage_headers.files = $$HEADERS

    # # Group local staging into an explicit custom target
    # # Usage: make stage
    # stage.commands = $(MAKE) -f $(MAKEFILE) INSTALL_ROOT=$$PWD/../dist install
    # # Or export specific file sets:
    # # INSTALLS += stage_lib stage_headers

    # ------------------------------------------------------------------
    # 5. Optional: App Bundle Frameworks / Embedding
    #    Usage: qmake BUNDLE_PATH=/path/to/MyConsumerApp.app Logger.pro
    # ------------------------------------------------------------------
    !isEmpty(BUNDLE_PATH) {
        bundle_fw.path = $$BUNDLE_PATH/Contents/Frameworks
        bundle_fw.files = $$DESTDIR/lib$${TARGET}*
        INSTALLS += bundle_fw
    }

    # ------------------------------------------------------------------
    # 6. pkg-config Metadata generation (Clean linking for other tools)
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
