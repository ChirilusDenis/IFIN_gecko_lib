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

#include "mtdc32Processor.h"
#include "pluginmanager.h"
#include "pluginconnectorqueued.h"
#include "runmanager.h"

static PluginRegistrar registrar ("MTDC32Processor", MTDC32Processor::create, AbstractPlugin::GroupProcessing, MTDC32Processor::getEventBuilderAttributeMap());

MTDC32Processor::MTDC32Processor(int _id, QString _name, const Attributes &_attrs)
            : BasePlugin(_id, _name)
            , attribs_ (_attrs)
            , inEvent(false)
{
    createSettings(settingsLayout);

    //Create input connector
    addConnector(new PluginConnectorQVUint(this,ScopeCommon::in,"in"));

    //Create 32 output connectors
    for(int n = 0; n < 32; n++)
    {
        addConnector(new PluginConnectorQVUint (this,ScopeCommon::out,QString("out %1").arg(n)));
    }

    setNumberOfMandatoryInputs(1); // Only needs one input to have data for writing the event

    std::cout << "Instantiated MTDC32Processor" << std::endl;
}

AbstractPlugin::AttributeMap MTDC32Processor::getEventBuilderAttributeMap() {
    AbstractPlugin::AttributeMap attrs;
    return attrs;
}
AbstractPlugin::AttributeMap MTDC32Processor::getAttributeMap () const { return getEventBuilderAttributeMap();}
AbstractPlugin::Attributes MTDC32Processor::getAttributes () const { return attribs_;}

void MTDC32Processor::createSettings(QGridLayout*){}

void MTDC32Processor::runStartingEvent(){
    int parentModule=-1;

    //Finding the plugin's connected module
    QString new1= inputs->first()->getConnectedPluginName();
    modules = *ModuleManager::ref ().list ();
    for(int i=0;i<modules.size();i++)
    {
        QString new2 = modules[i]->getOutputPlugin()->getName();
        if(new1==new2)
            {
            parentModule=i;
            }
    }


    //Getting the number of bits from the parent module
    if(parentModule==-1)
        std::cout<<"Input disconnected or module could not be found"<<std::endl;
    else
    {
        if(modules[parentModule]->getSettings()==32) bits=32;
        else if(modules[parentModule]->getSettings()==8) bits=8;
        else if(modules[parentModule]->getSettings()==16) bits=16;
        else if(modules[parentModule]->getSettings()==64) bits=64;

    }

}

void MTDC32Processor::userProcess()
{
    QVector<uint32_t> data;
    uint8_t sigID;
    v.clear();
    v.resize(32);

    //Get the data from the module
    data = inputs->first()->getData().value< QVector<uint32_t> >();


    //Go through the data
    for(int i=0;i<data.size();i++)
    {
        if(data[i]!=0)
        {
            it=&data[i];
            //Get the word signature
            sigID = 0x0 | (((*it) >> MTDC32V2_OFF_DATA_SIG) & MTDC32V2_MSK_DATA_SIG);

            //Check the word signature
            if(sigID == MTDC32V2_SIG_HEADER)
            {
                //If header, then start new event
                //There used to be an inEvent check here, but it would wrongly concatenated events if it found a problem
                startNewEvent();
            }
            else if(sigID == MTDC32V2_SIG_DATA)
            {
                //If data word, continue event
                if(inEvent) continueEvent();
                else std::cout << "Not in event!" << std::endl;
            }
            else if(sigID == MTDC32V2_SIG_END || sigID == MTDC32V2_SIG_END_BERR)
            {
                //If end of event word, finish event
                if(inEvent)
                    finishEvent();
                else std::cout << "Not in event!"<< std::endl;
            }
            else
                std::cout << "Unknown data word " << std::hex << (*it) << std::endl;
        }
    }

    //Send the data to the outputs
    finishBlock();
}

void MTDC32Processor::startNewEvent()
{
    //Start the event, register the header info, clear the data values
    inEvent = true;
    header.data = (*it);
    chData.clear ();
}

void MTDC32Processor::continueEvent()
{
    mtdc32_data_t dataWord;
    dataWord.data = (*it);

    //Check the subsignature
    if(dataWord.bits.sub_signature == MTDC32V2_SIG_DATA_EVENT) {
        if(dataWord.bits.channel < bits) {
                  if(dataWord.bits.trigger !=1){
                      //Insert the channel/data pair into chData
                      chData.insert(std::make_pair(+ dataWord.bits.channel,+ dataWord.bits.value));
                }
        }
        else std::cout << "DemuxMesytecMtdc32: got invalid channel number " << std::dec << (int)dataWord.bits.channel << std::endl;
    }

    else if (dataWord.bits.sub_signature == MTDC32V2_SIG_DATA_DUMMY) {
        // If the word is a dummy, do nothing
    }

    else if (dataWord.bits.sub_signature == MTDC32V2_SIG_DATA_TIME) {
        //The plugin is not prepared to handle extended timestamps
        mtdc32_extended_timestamp_t time;
        time.data = (*it);
        std::cout << "DemuxMesytecMtdc32: Found extended timestamp: "
                  << std::dec << (uint32_t) time.bits.timestamp << std::endl;
    }

    else {
        std::cout << "DemuxMesytecMtdc32: No valid sub signature in word: "
                  << std::hex << (uint32_t) dataWord.data << std::endl;
    }
}

void MTDC32Processor::finishEvent()
{
    //State that the event is over, get the trailer info, get the timestamp or event counter
    inEvent = false;
    trailer.data = (*it);
    eventCounter = trailer.bits.trigger_counter;

    //Store the data and timestamp in the vector corresponding to the channel they came from
    for (std::map<uint8_t,uint16_t>::const_iterator i = chData.begin (); i != chData.end (); ++i) {
        v[i->first]<<i->second;v[i->first]<<eventCounter;
    }
}

void MTDC32Processor::finishBlock()
{
    //Send the data to the respective output
    for(int i=0; i<32;i++)
        outputs->at (i)->setData (QVariant::fromValue(v[i]));
}
