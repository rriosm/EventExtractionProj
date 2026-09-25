
TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    Logger \
    Utils \
    EventExtractor

Utils.depends = Logger
EventExtractor.depends = Logger Utils

