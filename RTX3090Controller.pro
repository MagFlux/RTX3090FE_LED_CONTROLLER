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

# Libraries - handle platform-specific linking
win32 {
    # On Windows, we don't link directly to nvapi library since we load it dynamically
    # We only need the include path for NVAPI headers
    # The actual loading is done in code using LoadLibrary
} else:unix {
    # On Unix-like systems (Linux), link against nvidia-ml
    LIBS += -lnvidia-ml
}