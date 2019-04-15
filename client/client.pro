QT += core
QT -= gui

CONFIG += c++11

TARGET = client
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

QMAKE_CXXFLAGS += -fpermissive

DEFINES += ICE_CPP11_MAPPING


INCLUDEPATH += \
    ../ice

LIBS += \
    -lIce++11

HEADERS += \
    ../ice/Printer.h \
    helper.h

SOURCES += \
    main.cpp \
    ../ice/Printer.cpp
