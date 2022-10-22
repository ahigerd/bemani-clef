isEmpty(S2W_LDFLAGS) {
  error("S2W_LDFLAGS must be set")
}

isEmpty(BUILDPATH) {
  error("BUILDPATH must be set")
}

isEmpty(PLUGIN_NAME) {
  error("PLUGIN_NAME must be set")
}

BUILDPATH = $$absolute_path($$BUILDPATH)
PROJPATH = $$absolute_path($$BUILDPATH/..)
S2WPATH = $$absolute_path($$BUILDPATH/../seq2wav)

TEMPLATE = app

isEmpty(BUILD_DEBUG) {
  TARGET = ../$${PLUGIN_NAME}_gui
  OBJECTS_DIR = $$BUILDPATH/gui
  MOC_DIR = $$BUILDPATH/gui
  RCC_DIR = $$BUILDPATH/gui
  CONFIG -= debug debug_and_release
  CONFIG += release
  LIBS += -L../build -l$$PLUGIN_NAME
  PRE_TARGETDEPS += $$BUILDPATH/lib$${PLUGIN_NAME}.a $$S2WPATH/build/libseq2wav.a
} else {
  TARGET = ../$${PLUGIN_NAME}_gui_d
  OBJECTS_DIR = $$BUILDPATH/gui_d
  MOC_DIR = $$BUILDPATH/gui_d
  RCC_DIR = $$BUILDPATH/gui_d
  CONFIG -= release debug_and_release
  CONFIG += debug
  LIBS += -L../build -l$${PLUGIN_NAME}_d
  PRE_TARGETDEPS += $$BUILDPATH/lib$${PLUGIN_NAME}_d.a $$S2WPATH/build/libseq2wav_d.a
}
QT = core gui widgets multimedia
QMAKE_CXXFLAGS += -std=c++17 -Wno-multichar
DEFINES -= QT_DEPRECATED_WARNINGS
DEFINES += QT_NO_DEPRECATED_WARNINGS
INCLUDEPATH += $$S2WPATH/include $$PROJPATH/src
LIBS += $$S2W_LDFLAGS

HEADERS += mainwindow.h   tagview.h   playercontrols.h
SOURCES += mainwindow.cpp tagview.cpp playercontrols.cpp

SOURCES += main.cpp ../plugins/s2wplugin.cpp
