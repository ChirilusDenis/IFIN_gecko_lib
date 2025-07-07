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

#ifndef PULSING_H
#define PULSING_H

#include "baseplugin.h"
#include "pluginmanager.h"
#include "runmanager.h"
#include "abstractinterface.h"
#include "abstractmodule.h"

#include <iostream>
#include <string>
#include <QGridLayout>
#include <QSpinBox>
#include <QPushButton>
#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QTimer>
#include <QTime>
#include <QPalette>

class Pulsing : public BasePlugin
{
Q_OBJECT
public:
    Pulsing(int id, QString name, const Attributes &attrs);
    static AbstractPlugin *create (int id, const QString &name, const Attributes &attrs) {
        return new Pulsing (id, name, attrs);
    }

    AttributeMap getAttributeMap () const;
    Attributes getAttributes () const;
    static AttributeMap getPulsingAttributeMap ();
    virtual void process();
    virtual void userProcess () {}

    virtual void applySettings(QSettings*);
    virtual void saveSettings(QSettings*);

public slots:
    void pulsingInputOn();
    void pulsingInputOff();
    void stopBeam();
    void pulsingButtonPressed();
    void sendInterfaceSignal();
    void runStartingEvent();
    void updateInterface();
    void stopStartPressed();
    void changeRecordBeamOn(int);
    void changeRecordBeamOff(int);

private:
    Attributes attrs_;
    void createSettings (QGridLayout *);

protected:
    QSpinBox* setPulsingOnH;
    QSpinBox* setPulsingOnM;
    QSpinBox* setPulsingOnS;
    QSpinBox* setPulsingOnMs;
    QSpinBox* setPulsingOffH;
    QSpinBox* setPulsingOffM;
    QSpinBox* setPulsingOffS;
    QSpinBox* setPulsingOffMs;
    QPushButton* pulsingManual;
    QPushButton* stopStartPulsing;
    QCheckBox* recordBeamOn;
    QCheckBox* recordBeamOff;
    QPalette* stopStartPalette;
    QTimer* setInterfaceOutput;
    QTimer* updateInterfaceTimer;
    QTime elapsedTime;
    QLabel* timeElapsedLabel;

    uint64_t pulsingTimeOn;
    uint64_t pulsingTimeOff;
    bool beamStatus;
    bool pulsingActive;
    QList<AbstractModule*> modules;

    int beamOnRecord;
    int beamOffRecord;

};

#endif // PULSING_H
