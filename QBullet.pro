#-------------------------------------------------
#
# Project created by QtCreator 2013-02-25T19:41:21
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QBullet
TEMPLATE = app


SOURCES += main.cpp\
        settings.cpp \
    logindialog.cpp \
    bullet.cpp \
    prompt.cpp \
    qbulletproxyfactory.cpp \
    networkaccessmanager.cpp

HEADERS  += settings.h \
    logindialog.h \
    bullet.h \
    prompt.h \
    qbulletproxyfactory.h \
    networkaccessmanager.h

FORMS    += settings.ui \
    logindialog.ui \
    prompt.ui \
    passworddialog.ui \
    proxy.ui

OTHER_FILES += \
    QBullet.png \
    README.txt

RESOURCES += \
    resources.qrc
