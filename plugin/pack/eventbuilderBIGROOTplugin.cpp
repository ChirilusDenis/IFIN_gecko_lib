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

#include "eventbuilderBIGROOTplugin.h"

static PluginRegistrar registrar ("eventbuilderBIGROOT", EventBuilderBIGROOTPlugin::create, AbstractPlugin::GroupPack, EventBuilderBIGROOTPlugin::getEventBuilderAttributeMap());

EventBuilderBIGROOTPlugin::EventBuilderBIGROOTPlugin(int _id, QString _name, const Attributes &_attrs)
            : BasePlugin(_id, _name)
            , attribs_ (_attrs)
            , filePrefix("Run")
            , total_bytes_written(0)
            , current_bytes_written(0)
            , current_file_number(1)
            , number_of_mb(200)
            , hoursToReset(2)
            , reset (false)
            , writePath("/tmp")
            , outputValue(1)
{
    createSettings(settingsLayout);

    //Get the number of inputs from the attributes. Check for validity
    bool ok;
    int _nofInputs = _attrs.value ("nofInputs", QVariant (4)).toInt (&ok);
    if (!ok || _nofInputs <= 0 || _nofInputs > 512) {
        std::cout << _name.toStdString () <<" "<< _nofInputs<<": nofInputs invalid. Setting to 128." << std::endl;
        _nofInputs = 128;
    }

    //Create the input connectors
    for(int n = 0; n < _nofInputs; n++)
    {
        addConnector(new PluginConnectorQVUint(this,ScopeCommon::in,QString("in %1").arg(n)));
    }

    // Only needs one input to have data for writing the event
    setNumberOfMandatoryInputs(1);

    //Change Run Name and write the last cache on Run Stop
    connect(RunManager::ptr(),SIGNAL(runStopped()),this,SLOT(updateRunName()));
    connect(RunManager::ptr(),SIGNAL(runStopped()),this,SLOT(throwLastCache()));
    connect(RunManager::ptr(),SIGNAL(emitBeamStatus(bool)),this,SLOT(receiveInterfaceSignal(bool)));

    //Define the timer that changes the run and connect it
    resetTimer = new QTimer();
    connect(resetTimer,SIGNAL(timeout()),this,SLOT(setTimeReset()));

    //Define the timer that writes the trigger average rate to the logbook and connect it
    triggerToLogbook = new QTimer();
    connect(triggerToLogbook,SIGNAL(timeout()),this,SLOT(writeTrigger()));

    //Define the timer that sends the trigger average rate to the run manager and connect it
    triggerToRunManager = new QTimer();
    connect(triggerToRunManager,SIGNAL(timeout()),this,SLOT(writeRunTrigger()));

    //Define the timer that opens up the information pop-up box once every two hours
    infoPopUpBoxTimer = new QTimer();
   // connect(infoPopUpBoxTimer,SIGNAL(timeout()),this,SLOT(showInfoBox()));

    //Define the timer that opens up the information pop-up box once every two hours
    updateTimer = new QTimer();
    connect(updateTimer,SIGNAL(timeout()),this,SLOT(updateByteCounters()));

    connect(this, SIGNAL(startUpdateTimer(int)),this,SLOT(startingUpdateTimer(int)));
    connect(this, SIGNAL(stopUpdateTimer()),this,SLOT(stoppingUpdateTimer()));

    connect(this, SIGNAL(startResetTimer(int)),this,SLOT(startingResetTimer(int)));
    connect(this, SIGNAL(stopResetTimer()),this,SLOT(stoppingResetTimer()));

    connect(this, SIGNAL(startTriggerToLogbook(int)),this,SLOT(startingTriggerToLogbook(int)));
    connect(this, SIGNAL(stopTriggerToLogbook()),this,SLOT(stoppingTriggerToLogbook()));

    connect(this, SIGNAL(startTriggerToRunManager(int)),this,SLOT(startingTriggerToRunManager(int)));
    connect(this, SIGNAL(stopTriggerToRunManager()),this,SLOT(stoppingTriggerToRunManager()));

    //start the Time to check if you write the Pulsing status or not
    pulsingTime.start();

    std::cout << "Instantiated EventBuilderBIGROOTPlugin" << std::endl;
}

EventBuilderBIGROOTPlugin::~EventBuilderBIGROOTPlugin()
{
    
    //Writes the last cache on plugin destruction
    // if(outFile.isOpen()) {
    //     writeCache();
    //     outFile.close();
    // }

    // DEBUG
    printf("Strating deconstructor...\n");

    // Write last data from the ttree and free used memory
    if((rootfile != NULL) && rootfile->IsOpen()) {
        if (roottree != NULL) {
            rootfile->cd();
            roottree->Write();
            delete roottree;
            // roottree = NULL;
        }
        rootfile->Flush();
        rootfile->Close();
        delete rootfile;
        // rootfile = NULL;
    }

    if (read_idx != NULL) delete[] read_idx;
    if (write_idx != NULL) delete[] write_idx;

    if (data_tree != NULL) {
        for(uint16_t type_idx = 0; type_idx < typeNo; type_idx++) {
            for(uint16_t param = 0; param < typeParam[type_idx] + 1; param++) {
                delete[] data_tree[type_idx][param];     
            }
            delete[] data_tree[type_idx];
        }
        delete[] data_tree;
    }

    // delete crt_time;
    delete time;

    // DEBUG
    printf("Deconstructor done.\n");
}

AbstractPlugin::AttributeMap EventBuilderBIGROOTPlugin::getEventBuilderAttributeMap() {
    AbstractPlugin::AttributeMap attrs;
    //Creates the attributes that are read on plugin creation
    attrs.insert ("nofInputs", QVariant::Int);
    return attrs;
}

AbstractPlugin::AttributeMap EventBuilderBIGROOTPlugin::getAttributeMap () const {
    return getEventBuilderAttributeMap();
}

AbstractPlugin::Attributes EventBuilderBIGROOTPlugin::getAttributes () const {
    return attribs_;
}

void EventBuilderBIGROOTPlugin::startingUpdateTimer(int time)
{
    updateTimer->start(time);
}

void EventBuilderBIGROOTPlugin::stoppingUpdateTimer()
{
    updateTimer->stop();
}

void EventBuilderBIGROOTPlugin::startingResetTimer(int time)
{
    resetTimer->start(time);
}

void EventBuilderBIGROOTPlugin::stoppingResetTimer()
{
    resetTimer->stop();
}

void EventBuilderBIGROOTPlugin::startingTriggerToLogbook(int time)
{
    triggerToLogbook->start(time);
}

void EventBuilderBIGROOTPlugin::stoppingTriggerToLogbook()
{
    triggerToLogbook->stop();
}

void EventBuilderBIGROOTPlugin::startingTriggerToRunManager(int time)
{
    triggerToRunManager->start(time);
}

void EventBuilderBIGROOTPlugin::stoppingTriggerToRunManager()
{
    triggerToRunManager->stop();
}

void EventBuilderBIGROOTPlugin::createSettings(QGridLayout * l)
{
    //Constructing the interface
    QWidget* container = new QWidget();
    {
        QGridLayout* cl = new QGridLayout;

        //Creating, connecting and placing the Logbook note button
        addNote = new QPushButton(tr("Add a note to the logbook"));
        connect(addNote,SIGNAL(clicked()),this,SLOT(addNoteClicked()));
        QGroupBox* gg = new QGroupBox("");
        {
                QGridLayout* cg = new QGridLayout();
                cg->addWidget(addNote,                          0,0,1,1);
                gg->setLayout(cg);
        }
        cl->addWidget(gg,0,0,1,1);

        //Creating the Configuration button, connecting it and placing labels
        QLabel* confLabel = new QLabel(tr("Configuration file name:"));
        confNameLabel = new QLabel();
        confNameButton = new QPushButton(tr("Choose the configuration file"));
        connect(confNameButton,SIGNAL(clicked()),this,SLOT(confNameButtonClicked()));
        //Various labels and information
        currentFileNameLabel = new QLabel(makeFileName());
        currentBytesWrittenLabel = new QLabel(tr("%1 MBytes").arg(current_bytes_written/1024./1024.));
        timeElapsedLabel= new QLabel(tr("%1:%2:%3").arg(0,2,10,QChar('0')).arg(0,2,10,QChar('0')).arg(0,2,10,QChar('0')));
        totalBytesWrittenLabel = new QLabel(tr("%1 MBytes").arg(total_bytes_written/1024./1024.));

        //Check how much free space is on the disk
        runPath = writePath.toStdString().c_str();
        boost::uintmax_t freeBytes = boost::filesystem::space(runPath).available;
        bytesFreeOnDiskLabel = new QLabel(tr("%1 GBytes").arg((double)(freeBytes/1024./1024./1024.)));

        //Creating the Button to choose the write folder and displaying it
        QLabel* writeLabel = new QLabel(tr("Write folder:"));
        writeLabel2 = new QLabel();
        writeButton = new QPushButton(tr("Choose the write folder"));
        connect(writeButton,SIGNAL(clicked()),this,SLOT(writeButtonClicked()));

        //Set the run name
        prefEdit = new QLineEdit();
        prefEdit->setText(filePrefix);
        connect(prefEdit,SIGNAL(editingFinished()),this,SLOT(prefEditInput()));
        //Placing all the above
        QGroupBox* gf = new QGroupBox("Write settings and stats");
        {
            QGridLayout* cl = new QGridLayout();
            cl->addWidget(confLabel,                            1,0,1,1);
            cl->addWidget(confNameLabel,                        1,1,1,1);
            cl->addWidget(confNameButton,                       1,2,1,1);
            cl->addWidget(new QLabel("File:"),                  2,0,1,1);
            cl->addWidget(currentFileNameLabel,                 2,1,1,1);
            cl->addWidget(new QLabel("Data written:"),          3,0,1,1);
            cl->addWidget(currentBytesWrittenLabel,             3,1,1,1);
            cl->addWidget(new QLabel("Time elapsed:"),          4,0,1,1);
            cl->addWidget(timeElapsedLabel,                     4,1,1,1);
            cl->addWidget(new QLabel("Total Data Written:"),    5,0,1,1);
            cl->addWidget(totalBytesWrittenLabel,               5,1,1,1);
            cl->addWidget(new QLabel("Disk free:"),             6,0,1,1);
            cl->addWidget(bytesFreeOnDiskLabel,                 6,1,1,1);
            cl->addWidget(writeLabel,                           7,0,1,1);
            cl->addWidget(writeLabel2,                          7,1,1,1);
            cl->addWidget(writeButton,                          7,2,1,1);
            cl->addWidget(new QLabel("Run Name:"),              8,0,1,1);
            cl->addWidget(prefEdit,                             8,1,1,1);
            gf->setLayout(cl);
        }
        cl->addWidget(gf,1,0,1,2);

        //Run change conditions
        writtenReset = new QSpinBox();
        writtenReset->setMinimum(200);
        writtenReset->setMaximum(4000);
        writtenReset->setSingleStep(100);
        connect(writtenReset,SIGNAL(valueChanged(int)),this,SLOT(mbSizeChanged()));
        timeReset = new QSpinBox();
        timeReset->setMinimum(1);
        timeReset->setMaximum(10);
        connect(timeReset,SIGNAL(valueChanged(int)),this,SLOT(timeResetInput()));
        //Placing the above
        QGroupBox* gr = new QGroupBox("Reset parameters");
        {
            QGridLayout* cl = new QGridLayout();
            cl->addWidget(new QLabel("Maximum file size in Mb:"),                  0,0,1,1);
            cl->addWidget(writtenReset,                 0,1,1,1);
            cl->addWidget(new QLabel("Hours until reset"),          1,0,1,1);
            cl->addWidget(timeReset,                    1,1,1,1);
            gr->setLayout(cl);
        }
        cl->addWidget(gr,2,0,1,2);

        //Set the coincidence interval for the data coupling
        setCoincInterval = new QSpinBox();
        setCoincInterval->setMinimum(10);
        setCoincInterval->setMaximum(460);
        setCoincInterval->setSingleStep(10);
        connect(setCoincInterval,SIGNAL(valueChanged(int)),this,SLOT(uiInput()));

        QLabel* scriptLabel = new QLabel(tr("Script file name:"));
        scriptNameLabel = new QLabel();
        scriptNameButton = new QPushButton(tr("Choose the script file"));
        connect(scriptNameButton,SIGNAL(clicked()),this,SLOT(scriptNameButtonClicked()));
        //Place all of the above
        QGroupBox* gc = new QGroupBox("Coincidence interval");
        {
            QGridLayout* cl = new QGridLayout();
            cl->addWidget(new QLabel("Coincidence interval:"),       14,0,1,1);
            cl->addWidget(setCoincInterval,                          14,1,1,1);
            cl->addWidget(scriptLabel,                                 15,0,1,1);
            cl->addWidget(scriptNameLabel,                             15,1,1,2);
            cl->addWidget(scriptNameButton,                            15,4,1,1);
            gc->setLayout(cl);
        }
        cl->addWidget(gc,3,0,1,2);
        container->setLayout(cl);
    }

    //Placing everything in the plugin window
    l->addWidget(container,0,0,1,1);

    //Setting up the Info pop-up Window
    popUpBox = new QWidget();
    popUpBox->setWindowModality(Qt::ApplicationModal);

    targetEdit = new QLineEdit();
    targetEdit->setText(target);
    projectileEdit = new QLineEdit();
    projectileEdit->setText(projectile);
    HPGerateEdit = new QLineEdit();
    HPGerateEdit->setText(HPGerate);
    targetCurrentEdit = new QLineEdit();
    targetCurrentEdit->setText(targetCurrent);

    //If okButton is pressed, plugin checks if all the values have been filled
    okButton = new QPushButton("&OK", this);
    connect(okButton,SIGNAL(clicked()),this,SLOT(checkInfo()));
    notOKLabel = new QLabel();

        QGridLayout* cl = new QGridLayout();
        cl->addWidget(new QLabel("Target:"),            0,0,1,1);
        cl->addWidget(targetEdit,                            0,1,1,1);
        cl->addWidget(new QLabel("Projectile:"),            1,0,1,1);
        cl->addWidget(projectileEdit,                            1,1,1,1);
        cl->addWidget(new QLabel("HPGe rate:"),            2,0,1,1);
        cl->addWidget(HPGerateEdit,                            2,1,1,1);
        cl->addWidget(new QLabel("Target current:"),            3,0,1,1);
        cl->addWidget(targetCurrentEdit,                            3,1,1,1);
        cl->addWidget(okButton,                             4,0,1,1);
        cl->addWidget(notOKLabel,                            4,1,1,1);
        popUpBox->setLayout(cl);

    //Making sure it doesn't have a close button
    popUpBox->setWindowFlags(Qt::WindowMaximizeButtonHint);
}

void EventBuilderBIGROOTPlugin::updateRunName() {
    //Writing the file name and address in the plugin window
    QString Qfilename;
    Qfilename=tr("%1/%2%3")
            .arg(writePath)
            .arg(filePrefix)
            .arg(current_file_number,3,10,QChar('0'));
    currentFileNameLabel->setText(Qfilename);
}

void EventBuilderBIGROOTPlugin::throwLastCache()
{
    //Write to logbook that the Run was stopped by the user
    QDateTime time=QDateTime::currentDateTime();
    logbook<<time.date().toString("dddd dd:MM:yyyy").toStdString()<<" ";
    logbook<<time.time().toString("HH:mm:ss").toStdString()<<"\t\t";
    logbook<<"Stopping "<<(tr("%1%2").arg(filePrefix).arg(current_file_number,3,10,QChar('0'))).toStdString()<<std::endl;
    logbook<<"User stopped "<<(tr("%1%2").arg(filePrefix).arg(current_file_number,3,10,QChar('0'))).toStdString()<<"!"<<std::endl;

    if(!scriptName.isEmpty())
        if (!QProcess::startDetached("/bin/sh", QStringList{scriptName}))
            std::cout << "Failed to run";

    // Writing the last data from the root tree and freeing used memory
    if((rootfile != NULL) && rootfile->IsOpen()) {
        if(roottree != NULL) {
            rootfile->cd();
            roottree->Write();
            delete roottree;
            roottree = NULL;
        }
        rootfile->Flush();
        rootfile->Close();
        delete rootfile;
        rootfile = NULL;
    }

    //Stop the timers
    stopTriggerToLogbook();
    stopTriggerToRunManager();

    stopUpdateTimer();
    //infoPopUpBoxTimer->stop();
}


void EventBuilderBIGROOTPlugin::setTimeReset(){
    //When set to true, the file will be changed. Linked to the time reset
    reset=true;
}

void EventBuilderBIGROOTPlugin::receiveInterfaceSignal(bool receivedBeamStatus)
    {
    outputValue=receivedBeamStatus;

    if(pulsingTime.restart()>10*60*1000)
    {
        //Write to logbook and change Manual Pulsing button text
        QDateTime time=QDateTime::currentDateTime();
        logbook<<time.date().toString("dddd dd:MM:yyyy").toStdString()<<" ";
        logbook<<time.time().toString("HH:mm:ss").toStdString()<<"\t\t";
        if(!outputValue)
        {
            logbook<<"Beam off"<<std::endl;
        }
        else
        {
            logbook<<"Beam on"<<std::endl;
        }

        if(!scriptName.isEmpty())
            if (!QProcess::startDetached("/bin/sh", QStringList{scriptName}))
                std::cout << "Failed to run";
    }

    if(outputValue)
    {
        beamOnTime.start();
    }
}

void EventBuilderBIGROOTPlugin::writeTrigger()
{
    //Write the average trigger rate to logbook
    QDateTime time=QDateTime::currentDateTime();
    logbook<<time.date().toString("dddd dd:MM:yyyy").toStdString()<<" ";
    logbook<<time.time().toString("HH:mm:ss").toStdString()<<"\t\t";
    logbook<<1000*(nofTriggers-lastNofTriggers)/triggerToLogbook->interval()<<" average triggers/second in the last hour"<<std::endl;
    lastNofTriggers=nofTriggers;

    if(!scriptName.isEmpty())
        if (!QProcess::startDetached("/bin/sh", QStringList{scriptName}))
            std::cout << "Failed to run";
}

void EventBuilderBIGROOTPlugin::writeRunTrigger()
{
    //Send the average number of triggers to the Run manager, where it is available to other plugins
    RunManager::ptr()->transmitTriggerRate(nofTriggers);
}

void EventBuilderBIGROOTPlugin::showInfoBox()
{
    //Shows info pop-up box
    if(popUpBox->isHidden())
    {
         popUpBox->show();
    }

    //Starts timer for the window to reappear every 2 hours
   // infoPopUpBoxTimer->setSingleShot(false);
   // infoPopUpBoxTimer->start(2*3600*1000);
}

void EventBuilderBIGROOTPlugin::addNoteClicked()
{
    //Add a note to the logbook
    if(!logbook.is_open())
    {
        logbook.open ((tr("%1/%2").arg(writePath).arg("Logbook.txt")).toLatin1().data(),std::ofstream::out | std::ofstream::app);
    }
    bool ok;
    QString text = QInputDialog::getText(this, tr("Add a note to the logbook"),tr("To be noted"), QLineEdit::Normal,tr("What would you like to note?"), &ok);
    logbook<<text.toStdString()<<std::endl;

    if(!scriptName.isEmpty())
        if (!QProcess::startDetached("/bin/sh", QStringList{scriptName}))
            std::cout << "Failed to run";
}

void EventBuilderBIGROOTPlugin::confNameButtonClicked()
{
     //Get configuration file
     setConfName(QFileDialog::getOpenFileName(this,tr("Choose configuration file"), "/home",tr("Text (*)")));
}

void EventBuilderBIGROOTPlugin::scriptNameButtonClicked()
{
     //Get configuration file
     setScriptName(QFileDialog::getOpenFileName(this,tr("Choose script"), "/home",tr("Text (*.sh)")));
}

void EventBuilderBIGROOTPlugin::writeButtonClicked()
{
    //Change folder the data is written to
     setWriteFolder(QFileDialog::getExistingDirectory(this,tr("Choose folder to write the data to"), "/home",QFileDialog::ShowDirsOnly));
}

void EventBuilderBIGROOTPlugin::prefEditInput()
{
    //Change file prefix
   filePrefix=prefEdit->text();
}

void EventBuilderBIGROOTPlugin::infoInput()
{
    bool change=0;
    //Check if known info has changed and remember it if it has
    if(target!=targetEdit->text())
    {
        target=targetEdit->text();
        change=1;
    }
    if(projectile!=projectileEdit->text())
    {
        projectile=projectileEdit->text();
        change=1;
    }
    if(HPGerate!=HPGerateEdit->text())
    {
        HPGerate=HPGerateEdit->text();
        change=1;
    }
    if(targetCurrent!=targetCurrentEdit->text())
    {
        targetCurrent=targetCurrentEdit->text();
        change=1;
    }

    //if the known info changed, write it to logbook.
    if(change)
    {
        if(!logbook.is_open())
        {
            logbook.open ((tr("%1/%2").arg(writePath).arg("Logbook.txt")).toLatin1().data(),std::ofstream::out | std::ofstream::app);
        }
        QDateTime time=QDateTime::currentDateTime();
        logbook<<time.date().toString("dddd dd:MM:yyyy").toStdString()<<" ";
        logbook<<time.time().toString("HH:mm:ss").toStdString()<<"\t\t"<<std::endl;
        logbook<<"Target is: "<<target.toStdString()<<std::endl;
        logbook<<"Projectile is: "<<projectile.toStdString()<<std::endl;
        logbook<<"HPGe rate is: "<<HPGerate.toStdString()<<std::endl;
        logbook<<"Target Current is: "<<targetCurrent.toStdString()<<std::endl;

        if(!scriptName.isEmpty())
            if (!QProcess::startDetached("/bin/sh", QStringList{scriptName}))
                std::cout << "Failed to run";
    }
}

void EventBuilderBIGROOTPlugin::checkInfo()
{
    //Check if all values have been filled. Else, display a message on screen.
    if((!target.isEmpty())&&(!projectile.isEmpty())&&(!HPGerate.isEmpty())&&(!targetCurrent.isEmpty()))
    {
        popUpBox->close();
        infoInput();
    }
    else
    {
        notOKLabel->setText(tr("All values must be filled!"));
        notOKLabel->setStyleSheet("QLabel {color : red;}");
    }
}

void EventBuilderBIGROOTPlugin::mbSizeChanged()
{
    //Set the number of MegaBytes at which the file is changed
    number_of_mb = writtenReset->value();
    // Update the max ttree size, so it won't dump to a file on its own
    if(roottree != NULL)
        roottree->SetMaxTreeSize(2 * number_of_mb * 1024 * 1024);
}


void EventBuilderBIGROOTPlugin::timeResetInput()
{
    //Set the number of hours at which the file is changed
   hoursToReset = timeReset->value();
}

void EventBuilderBIGROOTPlugin::uiInput()
{
    //Set the coincidence interval for defining two signals as part of the same event
    offset = setCoincInterval->value();
}

void EventBuilderBIGROOTPlugin::setConfName(QString _confPath)
{
    //Set the label text in the plugin and configure the detectors
    confNameLabel->setText(_confPath);
    confName=_confPath;
    configureDetectors(confName);
}

void EventBuilderBIGROOTPlugin::setScriptName(QString _scriptPath)
{
    scriptNameLabel->setText(_scriptPath);
    scriptName=_scriptPath;
}

void EventBuilderBIGROOTPlugin::setWriteFolder(QString _confPath)
{
    //Get the path to the folder in which to write the data and open the logbook
    writeLabel2->setText(_confPath);
    writePath=_confPath;
    if(logbook.is_open())
    {
        logbook.close();
        logbook.open ((tr("%1/%2").arg(writePath).arg("Logbook.txt")).toLatin1().data(),std::ofstream::out | std::ofstream::app);
    }
}

void EventBuilderBIGROOTPlugin::configureDetectors(QString setName) {
    printf("Det configured\n");

    typeNo=0;
    int word, nextValue;
    numberOfDet=0;
    typeParam.clear();
    detchan.clear();

    //Open configuration file
    std::ifstream chanconfig;
    QByteArray ba = setName.toLatin1();
    const char *c_str2 = ba.data();
    chanconfig.open (c_str2);

    if(chanconfig.is_open())
    {
        //Read the number of parameters for each detector type
        do{
           typeNo++;
           chanconfig>>word;
           typeParam.push_back(word);
        }while(chanconfig.peek()!='\n');

        //Initialize vectors
        noDetType.resize(typeNo);
        totalNoDet.resize(typeNo+1);
        for(int j=1;j<=typeNo;j++)
            totalNoDet[j]=0;

        //Read the channels for each detector
        while(!chanconfig.eof()){
            detchan.resize(numberOfDet+1);
            do{
                chanconfig>> nextValue;
                if(chanconfig.eof()) break;
                detchan[numberOfDet].push_back(nextValue);
              }while((chanconfig.peek()!='\n')&&(!chanconfig.eof()));
            if(chanconfig.eof()) break;
            totalNoDet[detchan[numberOfDet].back()]++;
            numberOfDet++;
        }

        detchan.resize(numberOfDet);
   }

   chanconfig.close();
}

void EventBuilderBIGROOTPlugin::applySettings(QSettings* settings)
{
    //Read the settings
    QString set;
    settings->beginGroup(getName());
        set = "filePrefix"; if(settings->contains(set)) filePrefix = settings->value(set).toString();
        set = "target";      if(settings->contains(set)) target = settings->value(set).toString();
        set = "projectile";      if(settings->contains(set)) projectile = settings->value(set).toString();
        set = "number_of_mb"; if(settings->contains(set)) number_of_mb = settings->value(set).toInt();
        set = "offset";     if(settings->contains(set)) offset = settings->value(set).toInt();
        set = "hoursToReset"; if(settings->contains(set)) hoursToReset = settings->value(set).toInt();
        set = "confName";   if(settings->contains(set)) confName = settings->value(set).toString();
        set = "scriptName";   if(settings->contains(set)) scriptName = settings->value(set).toString();
        set = "writePath";   if(settings->contains(set)) writePath =settings->value(set).toString();
    settings->endGroup();

    //Apply the settings
    confNameLabel->setText(confName);
    scriptNameLabel->setText(scriptName);
    configureDetectors(confName);
    writeLabel2->setText(writePath);
    writtenReset->setValue(number_of_mb);
    setCoincInterval->setValue(offset);
    timeReset->setValue(hoursToReset);
    prefEdit->setText(filePrefix);
}

void EventBuilderBIGROOTPlugin::saveSettings(QSettings* settings)
{
    //Save the settings
    if(settings == NULL)
    {
        std::cout << getName().toStdString() << ": no settings file" << std::endl;
        return;
    }
    else
    {
        std::cout << getName().toStdString() << " saving settings...";
        settings->beginGroup(getName());
            settings->setValue("filePrefix",filePrefix);
            settings->setValue("target",target);
            settings->setValue("projectile",projectile);
            settings->setValue("number_of_mb",number_of_mb);
            settings->setValue("offset",offset);
            settings->setValue("hoursToReset",hoursToReset);
            settings->setValue("confName",confName);
            settings->setValue("scriptName",scriptName);
            settings->setValue("writePath",writePath);
        settings->endGroup();
        std::cout << " done" << std::endl;
    }
}

void EventBuilderBIGROOTPlugin::runStartingEvent(){
    // Reset timers
    startUpdateTimer(1000);
    startTriggerToLogbook(60*60*1000);
    startTriggerToRunManager(5*1000);

    // Get number of inputs
    nofInputs = inputs->size();

    // Resize vectors
    data.resize(nofInputs);
    dataTemp.resize(nofInputs);
    resetPosition.resize(nofInputs);

    // Reset counters
    current_bytes_written = 0;
    current_file_number = 1;
    total_bytes_written = 0;
    nofTriggers=0;
    lastNofTriggers=0;

    // Update UI
    updateByteCounters();

    //Construct the name of the file the data will be written to
    makeFileName();

    //Open the logbook if it is not yet opened, and write that the Run was started
    if(!logbook.is_open())
    {
        logbook.open ((tr("%1/%2").arg(writePath).arg("Logbook.txt")).toLatin1().data(),std::ofstream::out | std::ofstream::app);
        logbook<<"User started "<<(tr("%1%2").arg(filePrefix).arg(current_file_number,3,10,QChar('0'))).toStdString()<<"!"<<std::endl;
    }

    if(!scriptName.isEmpty())
        if (!QProcess::startDetached("/bin/sh", QStringList{scriptName}))
            std::cout << "Failed to run";

    // NEW

    //Open new root file
    openNewFile();

   // infoPopUpBoxTimer->setSingleShot(true);
   //infoPopUpBoxTimer->start(1000);
}

void EventBuilderBIGROOTPlugin::updateByteCounters() {
    //Update the values shown on the UI of the plugin
    //Get the ammount of free space on the drive
    boost::uintmax_t freeBytes = boost::filesystem::space(runPath).available;
    bytesFreeOnDiskLabel->setText(tr("%1 GBytes").arg((double)(freeBytes/1024./1024./1024.),2,'f',3));
    //Get the ammount of data written
    currentBytesWrittenLabel->setText(tr("%1 MBytes").arg(current_bytes_written/1024./1024.,2,'f',3));
    totalBytesWrittenLabel->setText(tr("%1 MBytes").arg(total_bytes_written/1024./1024.,2,'f',3));
    //Get how much time passed since the new file was opened
    int passed=elapsedTime.elapsed();
    timeElapsedLabel->setText(tr("%1:%2:%3").arg(passed/1000/60/60,2,10,QChar('0')).arg(passed/1000/60%60,2,10,QChar('0')).arg(passed/1000%60,2,10,QChar('0')));
}

QString EventBuilderBIGROOTPlugin::makeFileName() {
    QString Qfilename;
    bool check=0;
    current_file_number=1;

    //Construct the filename from the write path, the prefix, and the run number
    do{
        Qfilename=tr("%1/%2%3.root")
                .arg(writePath)
                .arg(filePrefix)
                .arg(current_file_number,3,10,QChar('0'));
        check = QFile::exists(Qfilename);
        //If said file exists, go to the next one
        if (check){
            current_file_number++;
        }
    }while(check);

    return Qfilename;
}

void EventBuilderBIGROOTPlugin::openNewFile() {
    //DEBUG
    printf("Creating new file...\n");

    //Restart the reset timer
    stopResetTimer();
    reset=false;
    startResetTimer(hoursToReset*1000*60*60);

    //Start the counter for the elapsed time
    elapsedTime.start();

    beamOnTime.start();

    // Close and clean after old file
    if ((rootfile != NULL) && rootfile->IsOpen()) {
        if (roottree != NULL) {
            rootfile->cd();
            roottree->Write();
        }

        rootfile->Flush();
        rootfile->Close();
        delete rootfile;
        rootfile = NULL;

        QDateTime time=QDateTime::currentDateTime();
        logbook<<time.date().toString("dddd dd:MM:yyyy").toStdString()<<" ";
        logbook<<time.time().toString("HH:mm:ss").toStdString()<<"\t\t";
        logbook<<"Stopping "<<(tr("%1%2").arg(filePrefix).arg(current_file_number,3,10,QChar('0'))).toStdString()<<std::endl;

        if(!scriptName.isEmpty())
            if (!QProcess::startDetached("/bin/sh", QStringList{scriptName}))
                std::cout << "Failed to run";
    }

    // Create output directory if it doesn't exist
    outDir = QDir(writePath);
    if(!outDir.exists()) {
        outDir.mkdir(writePath);
    }

    // Check if the directory was opened or created 
    if (outDir.exists()) {

        // Update name on the UI
        updateRunName();

        //Write to logbook
        QDateTime time=QDateTime::currentDateTime();
        logbook<<time.date().toString("dddd dd:MM:yyyy").toStdString()<<" ";
        logbook<<time.time().toString("HH:mm:ss").toStdString()<<"\t\t";
        logbook<<"Starting "<<(tr("%1%2").arg(filePrefix).arg(current_file_number,3,10,QChar('0'))).toStdString()<<std::endl;\

        if(!scriptName.isEmpty())
            if (!QProcess::startDetached("/bin/sh", QStringList{scriptName}))
                std::cout << "Failed to run";

        current_bytes_written = 0;

        rootfile = new TFile(makeFileName().toStdString().c_str(), "recreate");
        
        if ((rootfile == NULL) || !rootfile->IsOpen()) {
            printf("EventBuilderBIGROOT: ROOT file can not be created!");
            return;
        }

        // "Emptying" the root tree by copying it with 0 entries copied

        // DEBUG
        // printf("(Empty roottree...)");
        
        if (roottree != NULL) {
            TTree * temp = roottree->CloneTree(0);
            delete roottree;
            roottree = temp;
            roottree->SetDirectory(rootfile);
        } else {
            if (data_tree == NULL) makeTreeBuffer();
            makeTTree();
        }

    } else {
        //If the folder does not exist and could not be created, write it to the terminal
       printf("EventBuilderBIGROOT: The output directory does not exist and could not be created! (%s)\n",outDir.absolutePath().toStdString().c_str());
   }
   // DEBUG
   printf("File done\n");
}

void EventBuilderBIGROOTPlugin::userProcess() {
    // DEBUG
    printf("User process...\n");
    
    //If the configuration file is not read, write to prompt that there is a problem
    if(typeNo==0) std::cout<<"WRITING PROBLEM!! No detector configuration detected!!"<<std::endl;

    //Initialize values and vectors. Largecheck and smallcheck are relevant to looking for events that are split by a timer reset
    bool hasReseted=0;
    uint32_t largeCheck=1030000000;
    uint32_t smallCheck=30000000;
    resetPosition.fill(-1);
    for(int i=0;i<nofInputs;i++)
        dataTemp[i].clear();

    // Get the data from each input
    for (int i=0; i<nofInputs; ++i) {
        data[i] = inputs->at(i)->getData().value< QVector<uint32_t> >();
    }

    // File switch at a certain number of mb written or after a certain time passed
    if ((current_bytes_written >= number_of_mb * 1000 * 1000) || (reset)) {
        openNewFile();
    }

    //Check to see if the timer has reseted in the event block
    for(int i=0;i<nofInputs;i++)
    {
        bool large=0;
        bool small=0;
        for(int j=1;j<data[i].size();j+=2)
        {
            if(!large)
            {
                if(data[i][j]>largeCheck)
                {
                    large=1;
                }
            }
            else if(!small)
                    if(data[i][j]<smallCheck)
                    {
                        small=1;
                        resetPosition[i]=j;
                        hasReseted=1;
                        break;
                    }
        }
    }

    // If the timer has reset, split the data into 2 blocks, one before the reset and one after the reset
    if(hasReseted)
    {
        for(int i=0;i<nofInputs;i++)
        {
            if(resetPosition[i]!=-1)
                for(int j=resetPosition[i]-1;j<data[i].size();j++)
                    dataTemp[i].push_back(data[i][j]);
            int toBeRemoved=data[i].size()-resetPosition[i]+1;
            data[i].remove(resetPosition[i]-1,toBeRemoved);
        }
    }

    // Start reconstructing the events
    writeToTree();

    //If the timer has reset, start reconstructing the events after the reset
    if(hasReseted)
    {
        for(int i=0;i<nofInputs;i++)
        {
            dataTemp[i].swap(data[i]);
        }
        writeToTree();
    }
    // DEBUG
    printf("User done\n");
}

// Moving data from data to roottree buffers
int EventBuilderBIGROOTPlugin::writeToTree() {\
    // DEBUG
    printf("Writting...\n");

    uint16_t read_channel, det_type_idx;
    uint32_t **big_branch;
    int local_bytes;

    // Emptying data tree
    for (uint16_t type_idx = 0; type_idx < typeNo; type_idx++) {
        for (uint16_t param = 0; param < typeParam[type_idx] + 1; param++) {
            memset(data_tree[type_idx][param], 0 , totalNoDet[type_idx + 1] * sizeof(uint32_t));
        }
    }
    
    // Reseting reading and writing indexes
    memset(read_idx, 0, nofInputs * sizeof(uint16_t));
    

    // Filling up the data_tree with values from data
    do {

        // Update current time
        // crt_time->Set();
        time->Set();
        memset(write_idx, 0, typeNo * sizeof(uint16_t));

        hasData = false;
        leastTime = 0;

        for (uint16_t conf_entry = 0; conf_entry < numberOfDet; conf_entry++) {

            for (uint16_t param = 1; param < detchan[conf_entry].size() - 1; param++) {
                read_channel = detchan[conf_entry][param];

                // Make sure the read channel actually exists
                if (read_channel >= nofInputs) {
                    printf("EventbuilderBIGROOT: Channel value over number of inputs");
                    break;
                }

                // Check if there is any remaining data to read on any channel
                if (data[read_channel].size() >= read_idx[read_channel] + 2) {
                    hasData = true;

                    // Find the lowest unused timestamp
                    if ((leastTime == 0) || (leastTime > data[read_channel][read_idx[read_channel] + 1]))
                        leastTime = data[read_channel][read_idx[read_channel] + 1];
                }
            }
        }
            

        // Writing the actual data to the data_tree
        for (uint16_t conf_entry = 0; conf_entry < numberOfDet; conf_entry++) {
            det_type_idx = detchan[conf_entry].back() - 1;
            big_branch = data_tree[det_type_idx];
            
            // Writing index
            big_branch[0][write_idx[det_type_idx]] = detchan[conf_entry][0];

            for (uint16_t param = 1; param < detchan[conf_entry].size() - 1; param++) {

                read_channel = detchan[conf_entry][param];

                // Make sure the read channel actually exists
                if (read_channel >= nofInputs) {
                    printf("EventbuilderBIGROOT: Channel value over number of inputs");
                    break;
                }

                if (data[read_channel].size() >= read_idx[read_channel] + 2) {
                    if (data[read_channel][read_idx[read_channel] + 1] < (leastTime + offset)) {
                        // Get data from positions to read and jump to the next value to next to_read

                        // Write only the val, skip timestamp
                        big_branch[param][write_idx[det_type_idx]] = data[read_channel][read_idx[read_channel]];

                        read_idx[read_channel] += 2;
                    }
                }
            }
            write_idx[det_type_idx]++;
        }

        if(hasData)
           nofTriggers++;

        local_bytes = roottree->Fill(); // Fill() returns the number of bytes committed to memory
        // Some filling error ocurred
        if (local_bytes < 0) {
            printf("EventBuilderBIGROOT: Error while writing to roottree!\n");
            break;
        }

        total_bytes_written += local_bytes;
        current_bytes_written += local_bytes;


    } while (hasData);

    // DEBUG
    printf("Write done\n");

    return local_bytes;
}

// Create buffers for roottree
// Should be called before makeTTree()
void EventBuilderBIGROOTPlugin::makeTreeBuffer() {
    // DEBUG
    printf("Making buffers...\n");

    // Make timestamp structure
    // crt_time = new TDatime();
    time = new TTimeStamp();

    uint32_t **big_branch = NULL;

    read_idx = (uint16_t *)calloc(nofInputs, sizeof(uint16_t));
    write_idx = (uint16_t *)calloc(typeNo, sizeof(uint16_t));

    data_tree = (uint32_t ***)calloc(typeNo, sizeof(uint32_t **));
    // The first division is based on the detector type
    // Those nodes divide further based on the number of parameters(channels to be read) + index for each deterctor type
    // The leaves are arrays
    // The leaves are used as buffers for the root tree branches

    // Making big branches
    for (uint16_t type_idx = 0; type_idx < typeNo; type_idx++) {

        big_branch = (uint32_t **)calloc(typeParam[type_idx] + 1, sizeof(uint32_t *));

        // Make 'sub' branches for each 'big' branch, sub branches = det index + channel data
        for (uint16_t param = 0; param < typeParam[type_idx] + 1; param++) {

            // Make space for future data
            big_branch[param] = (uint32_t *)calloc(totalNoDet[type_idx + 1], sizeof(uint32_t));
        }
        data_tree[type_idx] = big_branch;
    }

    // DEBUG
    printf("Buffers done\n");
}

// Create actual roottree
// Should be called only after makeTreeBuffer()
void EventBuilderBIGROOTPlugin::makeTTree() {
    // DEBUG
    printf("Making TTree...\n");

    std::string branch_name, leaf_name;

    roottree = new TTree("Gecko","Gecko");

    if(rootfile != NULL)
        roottree->SetDirectory(rootfile);

    // Setting limit so the ttree won't autosave to a file on its own
    roottree->SetMaxTreeSize(2 * number_of_mb * 1024 * 1024);

    // Making time and date branch
    // roottree->Branch("Timestamp", crt_time);
    roottree->Branch("Timestamp", time);

    // Making branches and linking their buffers
    for (uint16_t type_idx = 0; type_idx < typeNo; type_idx++) {
        for (uint16_t param = 0; param < typeParam[type_idx] + 1; param++) {

            // TODO name

            if (param == 0) branch_name = "Box" + std::to_string(type_idx + 1) + "_Index";
            else branch_name = "Box" + std::to_string(type_idx + 1) + "_Param_" + std::to_string(param);
            leaf_name = branch_name + "[" + std::to_string(totalNoDet[type_idx + 1]) + "]/I";

            // Make one branch for each detector type and (index & parameters) combination
            roottree->Branch(branch_name.c_str(), data_tree[type_idx][param], leaf_name.c_str());
        }
    }

    // DEBUG
    // roottree->Fill();

    // DEBUG
    printf("TTree done\n");
}