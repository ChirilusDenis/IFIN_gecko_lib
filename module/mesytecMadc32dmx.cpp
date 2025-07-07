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

#include "mesytecMadc32dmx.h"
#include "eventbuffer.h"
#include "abstractmodule.h"
#include "outputplugin.h"
#include <iostream>

MesytecMadc32Demux::MesytecMadc32Demux(const QVector<EventSlot*>& _evslots,
                           const AbstractModule* own,
                           uint chans, uint bits)
    : cnt (0)
    , nofChannels (chans)
    , nofBits (bits)
    , evslots (_evslots)
    , owner (own)
{
    if (nofBits == 0) {
        nofBits = MADC32V2_NUM_BITS;
        std::cout << "MesytecMtdc32Demux: nofBits invalid. Setting to 16" << std::endl;
    }

    std::cout << "Instantiated MesytecMtdc32Demux" << std::endl;
}

bool MesytecMadc32Demux::processData (Event* ev, uint32_t *data, uint32_t len)
{
    //std::cout << "DemuxMesytecMtdc32Plugin Processing" << std::endl;
    it = data;

    rawData.resize(len);
    rawData.fill(0);
    rawCnt = 0;

    while(it != (data+len))
    {

        rawData[rawCnt++] = (*it);
        ev->put(evslots.last(), QVariant::fromValue(rawData));

        it++;
    }
    return true;
}
