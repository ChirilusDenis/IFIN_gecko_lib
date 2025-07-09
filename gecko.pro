# -------------------------------------------------
# Project created by QtCreator 2009-07-14T17:06:02
# -------------------------------------------------
TARGET = gecko
TEMPLATE = app
CONFIG += qt
QT += widgets concurrent network
QMAKE_CC = gcc
QMAKE_CXX = g++
CONFIG += thread \
    c++13
QMAKE_CXXFLAGS_RELEASE += -g -march=native \
    -ggdb
QMAKE_CXXFLAGS += $$system(root-config --cflags)
LIBS += -g \
    -L$$PWD/lib/sis3100_calls \
    -l_sis3100 \
    -lgsl \
    -lgslcblas \
    -lusb \
    -lboost_filesystem \
    -lboost_system \
    $$system(root-config --libs)
INCLUDEPATH += include \
    lib/sis3100_calls \
    $$system(root-config --incdir)
SOURCES +=     core/2Ddisplay.cpp \
    core/baseplugin.cpp \
    core/eventbuffer.cpp \
    core/geckoremote.cpp \
    core/interfacemanager.cpp \
    core/main.cpp \
    core/modulemanager.cpp \
    core/outputplugin.cpp \
    core/plot2d.cpp \
    core/pluginconnector.cpp \
    core/pluginmanager.cpp \
    core/pluginthread.cpp \
    core/remotecontrolpanel.cpp \
    core/runmanager.cpp \
    core/runthread.cpp \
    core/scopemainwindow.cpp \
    core/threadbuffer.cpp \
    core/viewport.cpp \
    interface/sis3100module.cpp \
    interface/sis3100ui.cpp \
    module/caen792module.cpp \
    module/caen792ui.cpp \
    module/caenadcdmx.cpp \
    plugin/aux/fanoutplugin.cpp \
    plugin/aux/inttodoubleplugin.cpp \
    plugin/aux/pulsing.cpp \
    plugin/cache/multiplecachehistogramplugin.cpp \
    plugin/pack/eventbuilderBIGplugin.cpp \
    plugin/processing/mtdc32Processor.cpp \
    plugin/processing/madc32Processor.cpp \
    module/mesytecMadc32ui.cpp \
    module/mesytecMadc32module.cpp \
    module/mesytecMadc32dmx.cpp \
    module/mesytecMtdc32ui.cpp \
    module/mesytecMtdc32module.cpp \
    module/mesytecMtdc32dmx.cpp \
    plugin/cache/MatrixPlotWalk.cpp
HEADERS +=     include/2Ddisplay.h \
    include/addeditdlgs.h \
    include/geckoremote.h \
    include/pluginthread.h \
    include/remotecontrolpanel.h \
    include/runthread.h \
    include/scopemainwindow.h \
    include/systeminfo.h \
    include/threadbuffer.h \
    include/abstractinterface.h \
    include/abstractmodule.h \
    include/abstractplugin.h \
    include/baseinterface.h \
    include/basemodule.h \
    include/baseplugin.h \
    include/baseui.h \
    include/confmap.h \
    include/eventbuffer.h \
    include/geckoui.h \
    include/hexspinbox.h \
    include/interfacemanager.h \
    include/modulemanager.h \
    include/outputplugin.h \
    include/plot2d.h \
    include/pluginconnector.h \
    include/pluginconnectorplain.h \
    include/pluginconnectorqueued.h \
    include/pluginmanager.h \
    include/runmanager.h \
    include/samdsp.h \
    include/samqvector.h \
    include/viewport.h \
    interface/sis3100module.h \
    interface/sis3100ui.h \
    module/caen792module.h \
    module/caen792ui.h \
    module/caenadcdmx.h \
    module/caen_v785.h \
    module/caen_v792.h \
    plugin/aux/fanoutplugin.h \
    plugin/aux/inttodoubleplugin.h \
    plugin/aux/pulsing.h \
    plugin/cache/multiplecachehistogramplugin.h \
    plugin/pack/eventbuilderBIGplugin.h \
    plugin/processing/mtdc32Processor.h \
    plugin/processing/madc32Processor.h \
    module/mesytec_madc_32_v2.h \
    module/mesytecMadc32module.h \
    module/mesytecMadc32dmx.h \
    module/mesytecMadc32ui.h \
    module/mesytec_mtdc_32_v2.h \
    module/mesytecMtdc32module.h \
    module/mesytecMtdc32dmx.h \
    module/mesytecMtdc32ui.h \
    plugin/cache/MatrixPlotWalk.h
#OTHER_FILES +=

