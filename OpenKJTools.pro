#-------------------------------------------------
#
# Project created by QtCreator 2014-07-02T12:09:02
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = OpenKJTools
TEMPLATE = app

# Populate version with version from git describe
VERSION = 0.11.6
message($$VERSION)
DEFINES += GIT_VERSION=\\"\"$$VERSION\\"\"
QMAKE_TARGET_COMPANY = OpenKJ.org
QMAKE_TARGET_PRODUCT = OpenKJ Tools
QMAKE_TARGET_DESCRIPTION = OpenKJ karaoke file managemnt tools

unix: BLDDATE = $$system(date -R)
win32: BLDDATE = $$system(date /t)
DEFINES += BUILD_DATE=__DATE__



SOURCES += main.cpp\
        mainwindow.cpp \
    processingthread.cpp \
    ziphandler.cpp \
    miniz.c \
    settings.cpp \
    dlgsettings.cpp

HEADERS  += mainwindow.h \
    processingthread.h \
    ziphandler.h \
    miniz.h \
    settings.h \
    dlgsettings.h

FORMS    += mainwindow.ui \
    dlgsettings.ui

unix:!macx {
    isEmpty(PREFIX) {
      PREFIX=/usr
    }
    iconfiles.files += resources/openkjtools.png
    iconfiles.path = $$PREFIX/share/pixmaps
    desktopfiles.files += resources/openkjtools.desktop
    desktopfiles.path = $$PREFIX/share/applications
    binaryfiles.files += OpenKJTools 
    binaryfiles.path = $$PREFIX/bin
    INSTALLS += binaryfiles iconfiles desktopfiles
}

RESOURCES += \
    resources.qrc
