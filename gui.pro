TEMPLATE = app
TARGET = build/ipc_gui
INCLUDEPATH += .
QT += widgets x11extras
OBJECTS_DIR = build
LIBS += -lX11

# Input
FORMS += src/gui/ipc.ui src/gui/manager.ui
SOURCES += src/common/common.cpp
SOURCES += src/gui/main.cpp src/gui/ipc.cpp src/gui/manager.cpp
HEADERS += src/gui/ipc.hpp src/gui/manager.hpp
