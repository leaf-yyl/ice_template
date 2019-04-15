QT += core
QT -= gui

CONFIG += c++11

TARGET = server
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

DEFINES += ICE_CPP11_MAPPING


INCLUDEPATH += \
    ../ice

LIBS += \
    -lIce++11

HEADERS += \
    ../ice/Printer.h

SOURCES += \
    main.cpp \
    ../ice/Printer.cpp
