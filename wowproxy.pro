#-------------------------------------------------
#
# Project created by QtCreator 2014-03-17T12:25:57
#
#-------------------------------------------------

QT       += core
QT       += network

QT       -= gui

QMAKE_CXXFLAGS += -std=c++11

TARGET = wowproxy
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp \
    wowproxy.cpp

HEADERS += \
    wowproxy.h
