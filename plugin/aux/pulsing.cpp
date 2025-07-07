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

#include "pulsing.h"

static PluginRegistrar reg ("pulsing", &Pulsing::create, AbstractPlugin::GroupAux, Pulsing::getPulsingAttributeMap ());

Pulsing::Pulsing(int id, QString name, const Attributes &attrs)
    : BasePlugin(id, name)
    , attrs_ (attrs)
    , beamStatus (0)
    , pulsingActive (1)
    , beamOnRecord (1)
    , beamOffRecord (1)
{
    createSettings(settingsLayout);

    //Number of inputs that have to have data in order for the plugin to run
    setNumberOfMandatoryInputs(0);

    connect(RunManager::ptr(),SIGNAL(runStopped()),this,SLOT(stopBeam()));

    //Define the timer that changes the pulser status and connect it
    setInterfaceOutput = new QTimer();
    connect(setInterfaceOutput,SIGNAL(timeout()),this,SLOT(sendInterfaceSignal()));

    updateInterfaceTimer = new QTimer();
    updateInterfaceTimer->start(1000);
    connect(updateInterfaceTimer,SIGNAL(timeout()),this,SLOT(updateInterface()));

    RunManager::ptr()->changeBeamStatus(beamStatus);
}

void Pulsing::createSettings (QGridLayout * l) {
    QWidget* container = new QWidget();
    {
        QGridLayout* cl = new QGridLayout;
        //Setting the pulsing interval
        setPulsingOnH = new QSpinBox();
        setPulsingOnH->setMinimum(0);
        setPulsingOnH->setMaximum(5);
        setPulsingOnH->setSingleStep(1);
        setPulsingOnH->setSuffix(" h");

        setPulsingOnM = new QSpinBox();
        setPulsingOnM->setMinimum(0);
        setPulsingOnM->setMaximum(60);
        setPulsingOnM->setSingleStep(1);
        setPulsingOnM->setSuffix(" min");

        setPulsingOnS = new QSpinBox();
        setPulsingOnS->setMinimum(0);
        setPulsingOnS->setMaximum(60);
        setPulsingOnS->setSingleStep(1);
        setPulsingOnS->setSuffix(" sec");
        setPulsingOnS->setValue(5);

        setPulsingOnMs = new QSpinBox();
        setPulsingOnMs->setMinimum(0);
        setPulsingOnMs->setMaximum(1000);
        setPulsingOnMs->setSingleStep(1);
        setPulsingOnMs->setSuffix(" msec");

        //Connecting the pulsing interval Spin boxes
        connect(setPulsingOnH,SIGNAL(valueChanged(int)),this,SLOT(pulsingInputOn()));
        connect(setPulsingOnM,SIGNAL(valueChanged(int)),this,SLOT(pulsingInputOn()));
        connect(setPulsingOnS,SIGNAL(valueChanged(int)),this,SLOT(pulsingInputOn()));
        connect(setPulsingOnMs,SIGNAL(valueChanged(int)),this,SLOT(pulsingInputOn()));

        setPulsingOffH = new QSpinBox();
        setPulsingOffH->setMinimum(0);
        setPulsingOffH->setMaximum(5);
        setPulsingOffH->setSingleStep(1);
        setPulsingOffH->setSuffix(" h");

        setPulsingOffM = new QSpinBox();
        setPulsingOffM->setMinimum(0);
        setPulsingOffM->setMaximum(60);
        setPulsingOffM->setSingleStep(1);
        setPulsingOffM->setSuffix(" min");

        setPulsingOffS = new QSpinBox();
        setPulsingOffS->setMinimum(0);
        setPulsingOffS->setMaximum(60);
        setPulsingOffS->setSingleStep(1);
        setPulsingOffS->setSuffix(" sec");
        setPulsingOffS->setValue(5);

        setPulsingOffMs = new QSpinBox();
        setPulsingOffMs->setMinimum(0);
        setPulsingOffMs->setMaximum(1000);
        setPulsingOffMs->setSingleStep(1);
        setPulsingOffMs->setSuffix(" msec");

        //Connecting the pulsing interval Spin boxes
        connect(setPulsingOffH,SIGNAL(valueChanged(int)),this,SLOT(pulsingInputOff()));
        connect(setPulsingOffM,SIGNAL(valueChanged(int)),this,SLOT(pulsingInputOff()));
        connect(setPulsingOffS,SIGNAL(valueChanged(int)),this,SLOT(pulsingInputOff()));
        connect(setPulsingOffMs,SIGNAL(valueChanged(int)),this,SLOT(pulsingInputOff()));

        //Manual control of the pulsing
        pulsingManual = new QPushButton(tr(""));
        if(beamStatus)
            pulsingManual->setText(tr("Beam is now ON: Set Beam Off"));
        else
            pulsingManual->setText(tr("Beam is now OFF: Set Beam On"));
        connect(pulsingManual,SIGNAL(clicked()),this,SLOT(pulsingButtonPressed()));

        timeElapsedLabel= new QLabel(tr("%1:%2:%3").arg(0,2,10,QChar('0')).arg(0,2,10,QChar('0')).arg(0,2,10,QChar('0')));

        //Stop or start the pulsing
        stopStartPulsing = new QPushButton(tr(""));
        stopStartPalette = new QPalette(stopStartPulsing->palette());
        if(pulsingActive)
        {
            stopStartPulsing->setText(tr("Pulsing is now ON: Set Pulsing OFF"));
            stopStartPulsing->setStyleSheet("background-color: #ccffcc");
        }
        else
        {
            stopStartPulsing->setText(tr("Pulsing in now OFF: Set Pulsing ON"));
            stopStartPulsing->setStyleSheet("background-color: #ffb2b2");
        }
        stopStartPulsing->setPalette(*stopStartPalette);
        connect(stopStartPulsing,SIGNAL(clicked()),this,SLOT(stopStartPressed()));

        recordBeamOn = new QCheckBox();
        recordBeamOn->setChecked(1);
        connect(recordBeamOn,SIGNAL(stateChanged(int)),this,SLOT(changeRecordBeamOn(int)));

        recordBeamOff = new QCheckBox();
        recordBeamOff->setChecked(1);
        connect(recordBeamOff,SIGNAL(stateChanged(int)),this,SLOT(changeRecordBeamOff(int)));

        //Placing all of the above
        cl->addWidget(new QLabel("Beam On Interval"),               0,0,1,4);
        cl->addWidget(setPulsingOnH,                                1,0,1,1);
        cl->addWidget(setPulsingOnM,                                1,1,1,1);
        cl->addWidget(setPulsingOnS,                                1,2,1,1);
        cl->addWidget(setPulsingOnMs,                               1,3,1,1);
        cl->addWidget(new QLabel("Beam Off Interval"),              2,0,1,4);
        cl->addWidget(setPulsingOffH,                                3,0,1,1);
        cl->addWidget(setPulsingOffM,                                3,1,1,1);
        cl->addWidget(setPulsingOffS,                                3,2,1,1);
        cl->addWidget(setPulsingOffMs,                               3,3,1,1);
        cl->addWidget(pulsingManual,                                4,0,1,2);
        cl->addWidget(new QLabel("Time until pulsing change"),      4,2,1,1);
        cl->addWidget(timeElapsedLabel,                             4,3,1,1);
        cl->addWidget(stopStartPulsing,                             5,0,1,4);
        cl->addWidget(new QLabel("Record data when the beam is ON"),6,0,1,1);
        cl->addWidget(recordBeamOn,                                 6,1,1,1);
        cl->addWidget(new QLabel("Record data when the beam is OFF"),6,2,1,1);
        cl->addWidget(recordBeamOff,                                6,3,1,1);

        container->setLayout(cl);
    }

    //Placing everything in the plugin window
    l->addWidget(container,0,0,1,1);
}

AbstractPlugin::AttributeMap Pulsing::getPulsingAttributeMap () {
    AbstractPlugin::AttributeMap attrs;
    return attrs;
}

AbstractPlugin::AttributeMap Pulsing::getAttributeMap () const {
    return getPulsingAttributeMap ();
}

AbstractPlugin::Attributes Pulsing::getAttributes () const {
    return attrs_;
}

void Pulsing::pulsingInputOn()
{
    //Set the time at which the beam is set on or off
    pulsingTimeOn=(setPulsingOnH->value())*3600*1000+(setPulsingOnM->value())*60*1000+(setPulsingOnS->value())*1000+setPulsingOnMs->value();
}

void Pulsing::pulsingInputOff()
{
    //Set the time at which the beam is set on or off
    pulsingTimeOff=(setPulsingOffH->value())*3600*1000+(setPulsingOffM->value())*60*1000+(setPulsingOffS->value())*1000+setPulsingOffMs->value();
}

void Pulsing::stopBeam()
{
    //Stop the beam
    modules = *ModuleManager::ref ().list ();
    AbstractInterface *iface = modules[0]->getInterface ();
    iface->setOutput3(1);

    beamStatus=0;
    pulsingManual->setText(tr("Beam is now OFF: Set Beam On"));
    RunManager::ptr()->changeBeamStatus(beamStatus);
    setInterfaceOutput->stop();
}

void Pulsing::sendInterfaceSignal()
{
    //Stop or start the beam
    modules = *ModuleManager::ref ().list ();
    AbstractInterface *iface = modules[0]->getInterface ();
    iface->setOutput3(beamStatus);

    //Change Manual Pulsing button text
    if(beamStatus)
    {
        beamStatus=0;
        pulsingManual->setText("Beam is now OFF: Set Beam On");
        setInterfaceOutput->start(pulsingTimeOff);
        RunManager::ptr()->changeRecordStatus(beamOffRecord);

    }
    else
    {
        beamStatus=1;
        pulsingManual->setText("Beam is now ON: Set Beam Off");
        setInterfaceOutput->start(pulsingTimeOn);
        RunManager::ptr()->changeRecordStatus(beamOnRecord);
    }

    RunManager::ptr()->changeBeamStatus(beamStatus);

    //Restart timer
    elapsedTime.restart();

}

void Pulsing::runStartingEvent()
{
    beamStatus=0;
    sendInterfaceSignal();
    elapsedTime.start();
    setInterfaceOutput->start(pulsingTimeOn);
    RunManager::ptr()->changeBeamStatus(beamStatus);
}

void Pulsing::pulsingButtonPressed()
{
    if(pulsingActive)
    {
        //Change the beam status
        modules = *ModuleManager::ref ().list ();
        AbstractInterface *iface = modules[0]->getInterface ();
        iface->setOutput3(beamStatus);

        //Change the button text
        if(beamStatus)
        {
            beamStatus=0;
            pulsingManual->setText("Beam is now OFF: Set Beam On");
            setInterfaceOutput->start(pulsingTimeOff);

        }
        else
        {
            beamStatus=1;
            pulsingManual->setText("Beam is now ON: Set Beam Off");
            setInterfaceOutput->start(pulsingTimeOn);
        }

        RunManager::ptr()->changeBeamStatus(beamStatus);

        //Restart timer
        elapsedTime.restart();
    }
}

void Pulsing::stopStartPressed()
{
    //Change the beam status
    modules = *ModuleManager::ref ().list ();
    AbstractInterface *iface = modules[0]->getInterface ();
    iface->setOutput3(1);

    //Change the button text
    if(pulsingActive)
    {
        pulsingActive=0;
        stopStartPulsing->setText(tr("Pulsing in now OFF: Set Pulsing ON"));
        stopStartPulsing->setStyleSheet("background-color: #ffb2b2");
        pulsingManual->setText("Pulsing is STOPPED");
        setInterfaceOutput->stop();
    }
    else
    {
        pulsingActive=1;
        stopStartPulsing->setText(tr("Pulsing is now ON: Set Pulsing OFF"));
        pulsingManual->setText("Beam is now ON: Set Beam Off");
        stopStartPulsing->setStyleSheet("background-color: #ccffcc");
        setInterfaceOutput->start(pulsingTimeOn);
        elapsedTime.restart();
    }

    RunManager::ptr()->changeBeamStatus(1);
}

void Pulsing::applySettings(QSettings *settings)
{
    QString set;
    settings->beginGroup(getName());
    set = "pulsingOnH";   if(settings->contains(set)) setPulsingOnH->setValue(settings->value(set).toInt());
    set = "pulsingOnM";   if(settings->contains(set)) setPulsingOnM->setValue(settings->value(set).toInt());
    set = "pulsingOnS";   if(settings->contains(set)) setPulsingOnS->setValue(settings->value(set).toInt());
    set = "pulsingOnMs";   if(settings->contains(set)) setPulsingOnMs->setValue(settings->value(set).toInt());
    set = "pulsingOffH";   if(settings->contains(set)) setPulsingOffH->setValue(settings->value(set).toInt());
    set = "pulsingOffM";   if(settings->contains(set)) setPulsingOffM->setValue(settings->value(set).toInt());
    set = "pulsingOffS";   if(settings->contains(set)) setPulsingOffS->setValue(settings->value(set).toInt());
    set = "pulsingOffMs";   if(settings->contains(set)) setPulsingOffMs->setValue(settings->value(set).toInt());
    set = "beamOnRecord"; if(settings->contains(set)) recordBeamOn->setChecked(settings->value(set).toInt());
    set = "beamOffRecord"; if(settings->contains(set)) recordBeamOff->setChecked(settings->value(set).toInt());
    settings->endGroup();
    pulsingInputOff();
    pulsingInputOn();

}

void Pulsing::saveSettings(QSettings* settings)
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
            settings->setValue("pulsingOnH",setPulsingOnH->value());
            settings->setValue("pulsingOnM",setPulsingOnM->value());
            settings->setValue("pulsingOnS",setPulsingOnS->value());
            settings->setValue("pulsingOnMs",setPulsingOnMs->value());
            settings->setValue("pulsingOffH",setPulsingOffH->value());
            settings->setValue("pulsingOffM",setPulsingOffM->value());
            settings->setValue("pulsingOffS",setPulsingOffS->value());
            settings->setValue("pulsingOffMs",setPulsingOffMs->value());
            settings->setValue("beamOnRecord",recordBeamOn->checkState());
            settings->setValue("beamOffRecord",recordBeamOff->checkState());
        settings->endGroup();
        std::cout << " done" << std::endl;
    }
}

void Pulsing::updateInterface()
{
    int passed=elapsedTime.elapsed();
    int interval=setInterfaceOutput->interval();
    int diff=interval-passed;
    if(diff>0)
        timeElapsedLabel->setText(tr("%1:%2:%3").arg(diff/1000/60/60,2,10,QChar('0')).arg(diff/1000/60%60,2,10,QChar('0')).arg(diff/1000%60,2,10,QChar('0')));
    else
        timeElapsedLabel->setText(tr("00:00:00"));
}

void Pulsing::changeRecordBeamOn(int state)
{
    beamOnRecord=state/2;
}

void Pulsing::changeRecordBeamOff(int state)
{
    beamOffRecord=state/2;
}

void Pulsing::process () {

}
