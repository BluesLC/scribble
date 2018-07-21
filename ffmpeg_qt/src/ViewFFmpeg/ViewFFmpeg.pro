TEMPLATE = app
CONFIG += console c++11
SOURCES += main.cpp

INCLUDEPATH += $$PWD/../../include/win32
LIBS += -L$$PWD/../../lib/win32 -lavcodec
