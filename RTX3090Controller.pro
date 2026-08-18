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

# Include paths for NVAPI
INCLUDEPATH += \
    "C:/Windows/System32" \
    "C:/Windows/SysWOW64"

# Suppress specific warnings for function pointer casting
win32 {
    QMAKE_CXXFLAGS += -Wno-cast-function-type
}

# NVAPI is always loaded dynamically at runtime (LoadLibrary) and resolved
# through nvapi_QueryInterface, so there is nothing to link against. The old
# `-lnvidia-ml` link pointed at the wrong library (the Management API), which does
# not provide the illumination interfaces we use.
win32 {
    # Loaded dynamically via LoadLibrary + GetProcAddress at runtime.
}