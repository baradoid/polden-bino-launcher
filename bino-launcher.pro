#-------------------------------------------------
#
# Project created by QtCreator 2018-07-11T12:05:38
#
#-------------------------------------------------

QT       += core gui network serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = bino-launcher
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        main.cpp \
        dialog.cpp \
    web.cpp \
    utils.cpp \
    com.cpp \
    unity.cpp \
    temp.c \
    demo.cpp

HEADERS += \
        dialog.h \
    web.h \
    utils.h \
    com.h \
    unity.h \
    temp.h \
    demo.h

FORMS += \
        dialog.ui

RESOURCES += \
    resources.qrc

LIBS += -lole32 -lWbemuuid -lOleAut32

QMAKE_LFLAGS_RELEASE += -static -static-libgcc
