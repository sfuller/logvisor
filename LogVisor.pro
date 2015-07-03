TEMPLATE = lib
CONFIG += staticlib c++11
TARGET = LogVisor
CONFIG -= Qt
QT      =

INCLUDEPATH += include

HEADERS += \
    include/LogVisor/LogVisor.hpp

SOURCES += \
    lib/LogVisor.cpp
