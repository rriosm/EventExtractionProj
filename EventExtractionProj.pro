
TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    Logger \
    Utils \
    EventExtractor

Utils.depends = Logger
EventExtractor.depends = Logger Utils

macx {
    # Force Clang compiler diagnostics to print absolute file paths
    QMAKE_CXXFLAGS += -fdiagnostics-absolute-paths
    QMAKE_CFLAGS   += -fdiagnostics-absolute-paths
}