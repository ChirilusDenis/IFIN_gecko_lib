/*
Copyright 2011 Bastian Loeher, Roland Wirth

This file is part of GECKO.

GECKO is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

GECKO is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* this plugin is meant to replace the event plugin provided by gecko-gecko because of the limit of
  32 channels in the event. This limit is due to the output format which uses a 32 bit channel mask to
  inform which of the channels has data. At the moment of trying to implement this software, the RoSphere
  has 25 detectors, (both energy and time information being required) and a few more are envisaged with the
  addition of particle detectors. Gabi S.
*/

#ifndef EVENTBUILDERBIGROOTPLUGIN_H
#define EVENTBUILDERBIGROOTPLUGIN_H

#include "baseplugin.h"
#include "modulemanager.h"
#include "runmanager.h"
#include "pluginmanager.h"
#include "pluginconnectorqueued.h"
#include <iostream>
#include <QTimer>
#include <QGridLayout>
#include <QPushButton>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <boost/filesystem/convenience.hpp>
#include <QSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QByteArray>
#include <QDataStream>
#include <QDateTime>
#include <algorithm>
#include <vector>
#include <QTime>
#include <QCheckBox>
#include <fstream>
#include <QInputDialog>
#include <fstream>
#include <stdio.h>
#include <bitset>
#include <QProcess>

#include <TFile.h>
#include <TTree.h>
#include <string.h>

class BasePlugin;

class EventBuilderBIGROOTPlugin : public BasePlugin
{
    Q_OBJECT

protected:
    Attributes attribs_;
    QString filePrefix;
    uint64_t total_bytes_written;
    uint32_t current_bytes_written;
    uint32_t current_file_number;
    uint32_t number_of_mb;
    uint32_t hoursToReset;
    bool reset;
    QString writePath;
    bool outputValue;

    QTimer* resetTimer;
    QTimer* triggerToLogbook;
    QTimer* triggerToRunManager;
    QTimer* infoPopUpBoxTimer;

    QPushButton* addNote;

    QLabel* confNameLabel;
    QPushButton* confNameButton;

    QLabel* scriptNameLabel;
    QPushButton* scriptNameButton;

    QLabel* currentFileNameLabel;
    QLabel* currentBytesWrittenLabel;
    QLabel* timeElapsedLabel;
    QLabel* totalBytesWrittenLabel;
    QLabel* writeLabel2;
    QPushButton* writeButton;
    boost::filesystem::path runPath;
    QLineEdit* prefEdit;

    QSpinBox* writtenReset;
    QSpinBox* timeReset;

    QSpinBox* setCoincInterval;

    int offset;
    virtual void createSettings(QGridLayout*);
    QString makeFileName();

    QLabel* bytesFreeOnDiskLabel;

    QFile outFile;
    QDir outDir;
    QTimer* updateTimer;
    QTime elapsedTime;
    QTime pulsingTime;
    QTime beamOnTime;
    QString confName;
    QString scriptName;

    QWidget* popUpBox;

    QLineEdit* targetEdit;
    QString target;
    QLineEdit* projectileEdit;
    QString projectile;
    QLineEdit* HPGerateEdit;
    QString HPGerate;
    QLineEdit* targetCurrentEdit;
    QString targetCurrent;
    QPushButton* okButton;
    QLabel* notOKLabel;

public:
    EventBuilderBIGROOTPlugin(int _id, QString _name, const Attributes &_attrs);

    static AbstractPlugin *create (int id, const QString &name, const Attributes &attrs) {
        return new EventBuilderBIGROOTPlugin (id, name, attrs);
    }
    virtual ~EventBuilderBIGROOTPlugin();

    AttributeMap getAttributeMap () const;
    Attributes getAttributes () const;
    static AttributeMap getEventBuilderAttributeMap ();

    void setConfName(QString);
    void setScriptName(QString);
    void setWriteFolder(QString);
    void infoInput();

    virtual void userProcess();
    virtual void applySettings(QSettings*);
    virtual void saveSettings(QSettings*);
    void configureDetectors(QString);

    // NEW
    int writeToTree();
    void makeTreeBuffer();
    void makeTTree();


public slots:
    void updateRunName();
    void throwLastCache();

    void setTimeReset();
    void receiveInterfaceSignal(bool);
    void writeTrigger();
    void writeRunTrigger();
    void showInfoBox();

    void addNoteClicked();
    void confNameButtonClicked();
    void scriptNameButtonClicked();
    void writeButtonClicked();
    void prefEditInput();
    void checkInfo();

    void mbSizeChanged();
    void timeResetInput();

    void updateByteCounters();
    void runStartingEvent();
    void uiInput();

    void openNewFile();

    void startingUpdateTimer(int);
    void stoppingUpdateTimer();

    void startingResetTimer(int);
    void stoppingResetTimer();

    void startingTriggerToLogbook(int);
    void stoppingTriggerToLogbook();
    void startingTriggerToRunManager(int);
    void stoppingTriggerToRunManager();

signals:
    void startUpdateTimer(int);
    void stopUpdateTimer();

    void startResetTimer(int);
    void stopResetTimer();

    void startTriggerToLogbook(int);
    void stopTriggerToLogbook();
    void startTriggerToRunManager(int);
    void stopTriggerToRunManager();

private:
    std::ofstream logbook;
    QDataStream out;
    int nofInputs; // Number of lines in data or dataTemp

    int typeNo; // Number of types (boxes)
    int numberOfDet; // Number of detectors = number of lines in the config file without the header
    uint64_t nofTriggers;
    uint64_t lastNofTriggers;
    std::vector <uint16_t> noDetType; // Number of detectors that fired
    std::vector <uint32_t> totalNoDet; // Number of detectors from each type
    std::vector <std::vector<uint32_t> > detchan; // Config file lines without the header
    QVector <int> typeParam;
    QVector <int> resetPosition;
    QVector<QVector<uint32_t> > data; // Input data, for each channel: val timestamp val timestamp ...
    QVector<QVector<uint32_t> > dataTemp; // Post reset data if reset happened


    TFile *rootfile = nullptr; // TFile to be written to disk
    TTree *roottree = nullptr; // TTree containing data to be written in the root files

    uint32_t ***data_tree = nullptr; // Used to make a move data from the input to the roottree

    uint16_t *read_idx = nullptr; // Used to keep track of read entries from data
    uint16_t *write_idx = nullptr; // Used to keep track of the number of entries in each 'big' branch of the data_tree
    
    TDatime trigger_time;

    // TODO
    // std::vector<std::string> param_names = {"Index", "TrailingTime", "LeadingTime"};
};

#endif // EVENTBUILDERBIGROOTPLUGIN_H
