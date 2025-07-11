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

#include "eventbuilderBIGplugin.h"

static PluginRegistrar registrar ("eventbuilderBIG", EventBuilderBIGPlugin::create, AbstractPlugin::GroupPack, EventBuilderBIGPlugin::getEventBuilderAttributeMap());

EventBuilderBIGPlugin::EventBuilderBIGPlugin(int _id, QString _name, const Attributes &_attrs)
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
            , rawWrite(0)
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

    std::cout << "Instantiated EventBuilderBIGPlugin" << std::endl;
}

EventBuilderBIGPlugin::~EventBuilderBIGPlugin()
{
    //Writes the last cache on plugin destruction
    if(outFile.isOpen()) {
        writeCache();
        outFile.close();
    }
}

AbstractPlugin::AttributeMap EventBuilderBIGPlugin::getEventBuilderAttributeMap() {
    AbstractPlugin::AttributeMap attrs;
    //Creates the attributes that are read on plugin creation
    attrs.insert ("nofInputs", QVariant::Int);
    return attrs;
}

AbstractPlugin::AttributeMap EventBuilderBIGPlugin::getAttributeMap () const {
    return getEventBuilderAttributeMap();
}

AbstractPlugin::Attributes EventBuilderBIGPlugin::getAttributes () const {
    return attribs_;
}

void EventBuilderBIGPlugin::startingUpdateTimer(int time)
{
    updateTimer->start(time);
}

void EventBuilderBIGPlugin::stoppingUpdateTimer()
{
    updateTimer->stop();
}

void EventBuilderBIGPlugin::startingResetTimer(int time)
{
    resetTimer->start(time);
}

void EventBuilderBIGPlugin::stoppingResetTimer()
{
    resetTimer->stop();
}

void EventBuilderBIGPlugin::startingTriggerToLogbook(int time)
{
    triggerToLogbook->start(time);
}

void EventBuilderBIGPlugin::stoppingTriggerToLogbook()
{
    triggerToLogbook->stop();
}

void EventBuilderBIGPlugin::startingTriggerToRunManager(int time)
{
    triggerToRunManager->start(time);
}

void EventBuilderBIGPlugin::stoppingTriggerToRunManager()
{
    triggerToRunManager->stop();
}

void EventBuilderBIGPlugin::createSettings(QGridLayout * l)
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
        //Choose if raw data should be written
        rawWriteBox = new QCheckBox();    
        connect(rawWriteBox,SIGNAL(stateChanged(int)),this,SLOT(rawWriteChanged()));

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
            cl->addWidget(new QLabel("Write raw data"),              14,2,1,1);
            cl->addWidget(rawWriteBox,                               14,4,1,1);
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

void EventBuilderBIGPlugin::updateRunName() {
    //Writing the file name and address in the plugin window
    QString Qfilename;
    Qfilename=tr("%1/%2%3")
            .arg(writePath)
            .arg(filePrefix)
            .arg(current_file_number,3,10,QChar('0'));
    currentFileNameLabel->setText(Qfilename);
}

void EventBuilderBIGPlugin::throwLastCache()
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

    //Write the last cache to file
    if(outFile.isOpen()) {
        writeCache();
        outFile.close();
    }

    //If applicable, write the last cache to the raw file
    if(rawFile.isOpen())
        rawFile.close();

    //Stop the timers
    stopTriggerToLogbook();
    stopTriggerToRunManager();

    stopUpdateTimer();
    //infoPopUpBoxTimer->stop();
}


void EventBuilderBIGPlugin::setTimeReset(){
    //When set to true, the file will be changed. Linked to the time reset
    reset=true;
}

void EventBuilderBIGPlugin::receiveInterfaceSignal(bool receivedBeamStatus)
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

void EventBuilderBIGPlugin::writeTrigger()
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

void EventBuilderBIGPlugin::writeRunTrigger()
{
    //Send the average number of triggers to the Run manager, where it is available to other plugins
    RunManager::ptr()->transmitTriggerRate(nofTriggers);
}

void EventBuilderBIGPlugin::showInfoBox()
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

void EventBuilderBIGPlugin::addNoteClicked()
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

void EventBuilderBIGPlugin::confNameButtonClicked()
{
     //Get configuration file
     setConfName(QFileDialog::getOpenFileName(this,tr("Choose configuration file"), "/home",tr("Text (*)")));
}

void EventBuilderBIGPlugin::scriptNameButtonClicked()
{
     //Get configuration file
     setScriptName(QFileDialog::getOpenFileName(this,tr("Choose script"), "/home",tr("Text (*.sh)")));
}

void EventBuilderBIGPlugin::writeButtonClicked()
{
    //Change folder the data is written to
     setWriteFolder(QFileDialog::getExistingDirectory(this,tr("Choose folder to write the data to"), "/home",QFileDialog::ShowDirsOnly));
}

void EventBuilderBIGPlugin::prefEditInput()
{
    //Change file prefix
   filePrefix=prefEdit->text();
}

void EventBuilderBIGPlugin::infoInput()
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

void EventBuilderBIGPlugin::checkInfo()
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

void EventBuilderBIGPlugin::mbSizeChanged()
{
    //Set the number of MegaBytes at which the file is changed
    number_of_mb = writtenReset->value();
}


void EventBuilderBIGPlugin::timeResetInput()
{
    //Set the number of hours at which the file is changed
   hoursToReset = timeReset->value();
}

void EventBuilderBIGPlugin::uiInput()
{
    //Set the coincidence interval for defining two signals as part of the same event
    offset = setCoincInterval->value();
}

void EventBuilderBIGPlugin::rawWriteChanged()
{
    //Set if raw data will be written or not
    rawWrite = rawWriteBox->isChecked();
}

void EventBuilderBIGPlugin::setConfName(QString _confPath)
{
    //Set the label text in the plugin and configure the detectors
    confNameLabel->setText(_confPath);
    confName=_confPath;
    configureDetectors(confName);
}

void EventBuilderBIGPlugin::setScriptName(QString _scriptPath)
{
    scriptNameLabel->setText(_scriptPath);
    scriptName=_scriptPath;
}

void EventBuilderBIGPlugin::setWriteFolder(QString _confPath)
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

void EventBuilderBIGPlugin::configureDetectors(QString setName) {
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

void EventBuilderBIGPlugin::applySettings(QSettings* settings)
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
        set = "rawWrite";   if(settings->contains(set)) rawWrite=settings->value(set).toBool();
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
    rawWriteBox->setChecked(rawWrite);
}

void EventBuilderBIGPlugin::saveSettings(QSettings* settings)
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
            settings->setValue("rawWrite",rawWrite);
        settings->endGroup();
        std::cout << " done" << std::endl;
    }
}

void EventBuilderBIGPlugin::runStartingEvent(){
    // Reset timers
    startUpdateTimer(1000);
    startTriggerToLogbook(60*60*1000);
    startTriggerToRunManager(5*1000);

    // Get number of inputs
    nofInputs = inputs->size();

    // Resize vectors
    data.resize(nofInputs);
    dataTemp.resize(nofInputs);
    toBeRead.resize(nofInputs);
    readPointer.resize(nofInputs);
    readIt.resize(numberOfDet);
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

    //Open that file
    openNewFile();

   // infoPopUpBoxTimer->setSingleShot(true);
   //infoPopUpBoxTimer->start(1000);
}

void EventBuilderBIGPlugin::updateByteCounters() {
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

QString EventBuilderBIGPlugin::makeFileName() {
    QString Qfilename;
    bool check=0;
    current_file_number=1;

    //Construct the filename from the write path, the prefix, and the run number
    do{
        Qfilename=tr("%1/%2%3")
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

QString EventBuilderBIGPlugin::makeRawName() {
    QString Qfilename;
    //Construct the name of the raw file. Its number will be identical to that of the non-raw file. The file will be in a special Raw folder
        Qfilename=tr("%1/%2%3")
                .arg(writePath)
                .arg("RAW/Raw")
                .arg(current_file_number,3,10,QChar('0'));
    return Qfilename;
}

void EventBuilderBIGPlugin::openNewFile(){
    //Restart the reset timer
    stopResetTimer();
    reset=false;
    startResetTimer(hoursToReset*1000*60*60);

    //Start the counter for the elapsed time
    elapsedTime.start();

    beamOnTime.start();

    // If necessary, close old file and write to logbook
    if(outFile.isOpen()) {
        writeCache();
        outFile.close();
        QDateTime time=QDateTime::currentDateTime();
        logbook<<time.date().toString("dddd dd:MM:yyyy").toStdString()<<" ";
        logbook<<time.time().toString("HH:mm:ss").toStdString()<<"\t\t";
        logbook<<"Stopping "<<(tr("%1%2").arg(filePrefix).arg(current_file_number,3,10,QChar('0'))).toStdString()<<std::endl;

        if(!scriptName.isEmpty())
            if (!QProcess::startDetached("/bin/sh", QStringList{scriptName}))
                std::cout << "Failed to run";
    }

    //If necessary, close the old raw file
    if(rawWrite)
        if(rawFile.isOpen()) {
            writeCache();
            rawFile.close();
        }


    //If the output directory does not exist, try to create it
    outDir = QDir(writePath);
    if(!outDir.exists())
    {
        outDir.mkdir(writePath);
    }

    //Check again if the output directory exists or was created
    if (outDir.exists()) {
        outFile.setFileName(makeFileName());
        outFile.open(QIODevice::WriteOnly);
        //Update the name on the UI
        updateRunName();
        //Reset or increase the byte counters with the addition from the file header
        current_bytes_written = 16384;
        total_bytes_written += (16384);

        //If enabled, create the raw file and raw folder
         if(rawWrite)
         {
            QString outPath=writePath;
            outPath.append("/RAW");
            if(!QDir(outPath).exists())
            {
                outDir.mkdir(outPath);
            }
            rawFile.setFileName(makeRawName());
            rawFile.open(QIODevice::WriteOnly);
            raw.setDevice(&rawFile);
            raw.setByteOrder(QDataStream::LittleEndian);
         }

     //Write to logbook
     QDateTime time=QDateTime::currentDateTime();
     logbook<<time.date().toString("dddd dd:MM:yyyy").toStdString()<<" ";
     logbook<<time.time().toString("HH:mm:ss").toStdString()<<"\t\t";
     logbook<<"Starting "<<(tr("%1%2").arg(filePrefix).arg(current_file_number,3,10,QChar('0'))).toStdString()<<std::endl;\

     if(!scriptName.isEmpty())
         if (!QProcess::startDetached("/bin/sh", QStringList{scriptName}))
             std::cout << "Failed to run";

     //Open the output file
     out.setDevice(&outFile);
     out.setByteOrder(QDataStream::LittleEndian);

     //Initialize the values for the file header
     uint16_t runnum=current_file_number;
        uint16_t *fhead;
        QString aux, aux1, aux2, aux3, aux4, comments;
        int l,w=0;
        fhead=( u_int16_t *) calloc(16, sizeof(u_int16_t *));
        comments.resize(16352);

        //Each block, including the file header, must be 16k bytes long. Comments is the file header without the block header
        for(int k=0;k<16352;k++)
            comments[k]=0;

        //Create the block header of the file header
        fhead[0]= 16;
        fhead[1]= 0;
        fhead[2]= runnum;
        fhead[3]= 18248;
        fhead[4]= 16;

        //Write the block header of the file header
        for(int k=0;k<16;k++)
            out<<fhead[k];

        //Write the rest of the file header
        l=filePrefix.size();

        for(int k=w;k<w+l;k++)
            comments[k]=filePrefix[k];
        w+=l;

        comments[w]= current_file_number;
        w+=2;

        aux="| Header (1 param) |";
        l=aux.size();
        for(int k=w;k<w+l;k++)
            comments[k]=aux[k-w];;
        w+=l;

        aux1="DetType";
        aux2=" det , ";
        aux4=" param)";
        aux3="(";
        for(int j=1;j<=typeNo;j++)
        {
            l=aux1.size();
            for(int k=w;k<w+l;k++)
               comments[k]=aux1[k-w];
            w+=l;
            comments[w]= j;
            comments[w+1]=aux3[0];
            comments[w+2]= totalNoDet[j];
            w+=3;
            l=aux2.size();
            for(int k=w;k<w+l;k++)
               comments[k]=aux2[k-w];
            w+=l;
            comments[w]=typeParam[j-1];
            w++;
            l=aux4.size();
            for(int k=w;k<w+l;k++)
               comments[k]=aux4[k-w];
            w+=l;
        }

        out.writeRawData( comments.toLatin1(), 16352);
}
    else {
        //If the folder does not exist and could not be created, write it to the terminal
       printf("EventBuilderBIG: The output directory does not exist and could not be created! (%s)\n",outDir.absolutePath().toStdString().c_str());
   }

    //Initialize the cache and written values
    for(int k=0;k<8176;k++)
        cache[k]=0;
    written=0;
}

void EventBuilderBIGPlugin::userProcess()
{
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
    for(int i=0; i<nofInputs; ++i) {
        data[i] = inputs->at(i)->getData().value< QVector<uint32_t> >();
    }

    //If raw writing is enabled, write the raw data to the raw files
    if(rawWrite)
        for(uint16_t i=0; i<nofInputs; ++i)
        {
            raw<< 61440;
            raw<<i;
            raw<<data[i].size();
            for(int j=0;j<data[i].size();j++)
                raw<<data[i][j];
        }

    // File switch at a certain number of mb written or after a certain time passed
    if((current_bytes_written >= 1000*1000*number_of_mb)||(reset)) {
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

    //If the timer has reset, split the data into 2 blocks, one before the reset and one after the reset
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

    //Start reconstructing the events
    writeToCache();

    //If the timer has reset, start reconstructing the events after the reset
    if(hasReseted)
    {
        for(int i=0;i<nofInputs;i++)
        {
            dataTemp[i].swap(data[i]);
        }
        writeToCache();
    }
}

void EventBuilderBIGPlugin::writeToCache()
{
    //Initialize some values
    uint32_t m;
    int j;
    readPointer.fill(0);

    //While there is still unprocessed data, reconstruct events
    do
    {
        //Reset some values on every processing pass
        readIt.fill(0);
        hasData=0;
        leastTime=0;
        toBeRead.fill(0);
        for(j=1;j<=typeNo;j++)
            noDetType[j]=0;

        for(int k=0;k<numberOfDet;k++)
                {
                    for(uint16_t z=1;z<detchan[k].size()-1;z++)
                    {
                        m=detchan[k][z];
                        if(data[m].size()>readPointer[m]+1)
                            hasData=1;
                    }
                }

        //Find the minimum timestamp not yet processed
        for(int k=0;k<numberOfDet;k++)
        {
            for(uint16_t z=1;z<detchan[k].size()-1;z++)
            {
                m=detchan[k][z];
                if(data[m].size()>=readPointer[m]+2)
                {
                    if(leastTime==0) leastTime=data[m][readPointer[m]+1];
                    else if(leastTime>data[m][readPointer[m]+1]) leastTime=data[m][readPointer[m]+1];
                }
            }
        }

        //Find the signals which belong to the current event
        for(int k=0;k<numberOfDet;k++)
        {
            for(uint16_t z=1;z<detchan[k].size()-1;z++)
            {
                m=detchan[k][z];

                if(data[m].size()>=2+readPointer[m])
                    if(data[m][readPointer[m]+1]<(leastTime+offset))
                    {
                        toBeRead[m]=1;
                        readIt[k]=1;
                    }
            }

            if(readIt[k])
            {
            noDetType[detchan[k].back()]++;
            }
        }

        //Reset some more values on every processing pass
        int evlength=0;
         uint16_t zero, one, moduleValue;
         uint16_t ch;
         zero=0;
         one=1;

        // Create the header
        uint16_t header = 0xF003+typeNo;
        for(j=1;j<=typeNo;j++) {
            header+=(typeParam[j-1]+1)*noDetType[j];
            evlength+=(typeParam[j-1]+1)*noDetType[j];
        }

        //If more than 8176 words are in the cache, write the cache to file
        if(written+3+typeNo+evlength>8176)
            while(writeCache()){};

        //write the header to the cache
        cache[written++] = header;
        //Write the state of the beam to the cache
        if(outputValue)
            cache[written++] = one;
        else cache[written++] = zero;
        //write time stamp
        cache[written++] = current_file_number;
        cache[written++] = (beamOnTime.elapsed()/1000)%65536;
        //Write the number of detectors that fired from each type to the cache
        for(j=1;j<=typeNo;j++)
             cache[written++] = noDetType[j];


        // Write the parameteres of each detector that fired
        for(j=0; j < numberOfDet; j++)
        {
            if(readIt[j])
            {
                ch=detchan[j][0]-1;
                cache[written++]=ch;
                for(uint16_t z=1;z<detchan[j].size()-1;z++)
                {
                    m=detchan[j][z];
                    if(toBeRead[m]==1)
                    moduleValue=data[m][readPointer[m]];
                    else moduleValue=0;
                    cache[written++] =moduleValue;
                }
            }
        }

        //Count the number of triggers
     if(hasData)
           nofTriggers++;

     //Take note of which signals were used for the reconstruction, ignore them from further processing passes
        for(j=0;j<nofInputs;j++)
            if(toBeRead[j])
                readPointer[j]+=2;
    //reiterate
    }while(hasData);
}

int EventBuilderBIGPlugin::writeCache(){
    //Check if the writing file is open
    if(outFile.isOpen()) {
        //Create the block header
            uint16_t *shead;
            shead=( u_int16_t *) calloc(16, sizeof(u_int16_t *));
            uint16_t runnum=current_file_number;
            shead[0]= 16;
            shead[1]= 0;
            shead[2]= runnum;
            shead[3]= 18264;
            shead[4]= 16;
            //Write the block header
            out.writeRawData( (char *) shead, 32);
            //Write the cache
            out.writeRawData( (char *) cache, 16352);

            //Reinitialize the cache
            for(int k=0;k<8176;k++)
                cache[k]=0;

            written=0;

            //Add up the ammount of data written
            current_bytes_written += (16384);
            total_bytes_written += (16384);

            return 0;
    }
    else  {
        return 1;
    }
}
