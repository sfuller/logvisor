TEMPLATE = lib
CONFIG += staticlib c++11
TARGET = LogVisor
CONFIG -= Qt
QT      =

HEADERS += \
    include/LogVisor/LogVisor.hpp

SOURCES += \
    lib/LogVisor.cpp
