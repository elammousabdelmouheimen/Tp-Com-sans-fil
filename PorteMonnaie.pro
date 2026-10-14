QT       += core gui widgets
CONFIG   += c++17
TARGET    = PorteMonnaie
TEMPLATE  = app

SOURCES += main.cpp \
           mainwindow.cpp \
           simbackend.cpp

FORMS   += mainwindow.ui

HEADERS += mainwindow.h \
           cardbackend.h \
           simbackend.h

win32:!sim {
    DEFINES     += USE_ODALID
    SOURCES     += odalidbackend.cpp
    HEADERS     += odalidbackend.h
    INCLUDEPATH += $$PWD/LIB
    LIBS        += -L$$PWD/LIB -lODALID_Education
    LIBS        += -lws2_32

    DESTDIR      = $$OUT_PWD/bin
    QMAKE_POST_LINK += $$QMAKE_COPY $$shell_quote($$shell_path($$PWD/LIB/ODALID_Education.dll)) $$shell_quote($$shell_path($$DESTDIR)) $$escape_expand(\\n\\t)
}
