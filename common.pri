# ----------------------------------------------------------------------
# Common Workspace Configuration
# ----------------------------------------------------------------------

# Modern C++ standard (use c++11 if your toolchain allows)
CONFIG += c++11

# Keep generated files organized outside the source tree
OBJECTS_DIR = $$OUT_PWD/.obj
MOC_DIR     = $$OUT_PWD/.moc
RCC_DIR     = $$OUT_PWD/.rcc
UI_DIR      = $$OUT_PWD/.ui

# Unified output destinations for the entire workspace
CONFIG(debug, debug|release) {
    BUILD_MODE = debug
    LIB_DESTDIR = $$OUT_PWD/../lib/Debug
    BIN_DESTDIR = $$OUT_PWD/../bin/Debug
} else {
    BUILD_MODE = release
    LIB_DESTDIR = $$OUT_PWD/../lib/Release
    BIN_DESTDIR = $$OUT_PWD/../bin/Release
}

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
# macOS Specific Toolchain & Runtime Settings
# ----------------------------------------------------------------------
# macOS Portability Defaults
macx {
    QMAKE_MAC_SDK = macosx

    # Set library ID to @rpath so consumers can relocate it cleanly
    QMAKE_SONAME_PREFIX = @rpath

    CONFIG += lib_version_first

    # Suppress version mismatch warnings between older Qt mkspecs and newer Xcode SDKs
    # When using Qt 5 with newer Xcode installations, Qt's internal mkspec feature tests
    # (mac/sdk.prf) often throw errors or warnings if the detected SDK version is higher
    # than what that Qt version officially recognized. Enabling CONFIG += sdk_no_version_check
    # bypasses this check.
    CONFIG += sdk_no_version_check

    # Let Xcode resolve the installed SDK dynamically via the unversioned alias.
    # Apple's clang & xcrun map 'macosx' to whatever active macOS SDK is installed
    # (in your case, Xcode 11.7's macOS 10.15 SDK).
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.15

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

    # Compiler warnings / flags
    QMAKE_CXXFLAGS += -Wno-deprecated-declarations \
                      -Wno-ambiguous-member-template
}
