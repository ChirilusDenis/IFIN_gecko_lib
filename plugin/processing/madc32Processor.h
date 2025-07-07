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

#ifndef MADC32PROCESSOR_H
#define MADC32PROCESSOR_H

#include <QGridLayout>
#include <QString>
#include <QVector>
#include <iostream>
#include <algorithm>
#include "abstractmodule.h"
#include "outputplugin.h"
#include "modulemanager.h"
#include "module/mesytec_madc_32_v2.h"
#include "baseplugin.h"

class BasePlugin;

class MADC32Processor : public BasePlugin
{
    Q_OBJECT

protected:
    virtual void createSettings(QGridLayout*);
    void readSettings(QSettings*,QString);

    Attributes attribs_;

public:
    MADC32Processor(int _id, QString _name, const Attributes &_attrs);

    static AbstractPlugin *create (int id, const QString &name, const Attributes &attrs) {
        return new MADC32Processor (id, name, attrs);
    }

    AttributeMap getAttributeMap () const;
    Attributes getAttributes () const;
    static AttributeMap getEventBuilderAttributeMap ();

    virtual void userProcess();
    virtual void applySettings(QSettings*){}
    virtual void saveSettings(QSettings*){}

public slots:
    void runStartingEvent();

private:
    bool inEvent;
    QList<AbstractModule*> modules;
    uint32_t* it;
    QVector <QVector<uint32_t> > v;
    std::map<uint8_t, uint16_t> chData;
    madc32_header_t header;
    madc32_end_of_event_t trailer;
    uint32_t eventCounter;
    int bits;
    int parentModule;

    void startNewEvent();
    void continueEvent();
    void finishEvent();
    void finishBlock();

};

#endif // MADC32PROCESSOR_H
