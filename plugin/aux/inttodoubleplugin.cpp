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

#include "inttodoubleplugin.h"
#include "pluginmanager.h"
#include "pluginconnectorqueued.h"

#include <iostream>
#include <string>

#include <QGridLayout>

static PluginRegistrar reg ("int->double", &IntToDoublePlugin::create, AbstractPlugin::GroupAux, IntToDoublePlugin::getIntToDoubleAttributeMap ());

IntToDoublePlugin::IntToDoublePlugin(int id, QString name, const Attributes &attrs)
    : BasePlugin(id, name)
    , attrs_ (attrs)
{
    nofChannels_ = attrs_.value ("nofChannels", 1).toInt ();

    //Checking the nofChannels value
    if (nofChannels_ <= 0) {
        std::cout << "Invalid number of channels: " << nofChannels_ << "! Setting to 1" << std::endl;
        nofChannels_ = 1;
    }

    //Number of inputs that have to have data in order for the plugin to run
    setNumberOfMandatoryInputs(1);

    //Creating an equal number of input and output connectors
    for (int i = 0; i < nofChannels_; ++i) {
        addConnector (new PluginConnectorQVUint (this, ScopeCommon::in, QString ("in %1").arg (i)));
        addConnector (new PluginConnectorQVDouble (this, ScopeCommon::out, QString ("out %1").arg (i)));
    }
}

void IntToDoublePlugin::createSettings (QGridLayout*) {}

AbstractPlugin::AttributeMap IntToDoublePlugin::getIntToDoubleAttributeMap () {
    AbstractPlugin::AttributeMap attrs;
    attrs.insert ("nofChannels", QVariant::Int);
    return attrs;
}

AbstractPlugin::AttributeMap IntToDoublePlugin::getAttributeMap () const {
    return getIntToDoubleAttributeMap ();
}

AbstractPlugin::Attributes IntToDoublePlugin::getAttributes () const {
    return attrs_;
}

void IntToDoublePlugin::process () {
    //Going through all the channels
    for (int i = 0; i < nofChannels_; ++i) {
        //If the input has data
        if (inputs->at (i)->dataAvailable ()) {
            //Get the int format data from the input
            QVector<uint32_t> idata = inputs->at (i)->getData ().value< QVector<uint32_t> > ();
            QVector<double> odata;

            //Making the double format vector as large as the int one
            odata.reserve (idata.size ());
            //Copying the int format vector to the double one
            for (int j = 0; j < idata.size (); ++j)
                odata << idata.at (j);
            //The double format vector is sent to the output
            outputs->at (i)->setData (QVariant::fromValue (odata));
            inputs->at (i)->useData ();
        }
    }
}

/*!
\page inttodoubleplg IntToDouble Plugin
\li <b>Plugin names:</b> \c inttodouble
\li <b>Group:</b> Aux

\section pdesc Plugin Description
The IntToDouble plugin is an auxiliary plugin.
It takes a uint32 input value on each of its inputs, converts it to a double and outputs the converted value to the respective output connector.

Due to program limitations, it is not currently possible to create/delete connectors after the plugin has been created.
Therefore the number of input/output connectors has to be set in the Add Plugin dialog.

\section attrs Attributes
\c nofChannels: Integer value describing the number of input/output pairs to create.

\section conf Configuration
None necessary.

\section inputs Input Connectors
\li \c in [0..n] \c &lt;uint32_t>: Input for the uint32 data to be converted

\section outputs Output Connectors
\li \c out [0..n] \c &lt;double>: Outputs for the double-converted data
*/
