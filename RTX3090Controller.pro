QT += core widgets gui

TARGET = RTX3090Controller
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    nvidiacontroller.cpp

HEADERS += \
    mainwindow.h \
    nvidiacontroller.h

RESOURCES += \
    resources/resources.qrc

# Include paths for NVAPI
INCLUDEPATH += \
    "C:/Windows/System32" \
    "C:/Windows/SysWOW64"

# Suppress specific warnings for function pointer casting
QMAKE_CXXFLAGS += -Wno-cast-function-type
