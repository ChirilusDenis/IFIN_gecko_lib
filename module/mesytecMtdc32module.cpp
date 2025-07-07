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

#include "mesytecMtdc32module.h"
#include "mesytecMtdc32ui.h"
#include "modulemanager.h"
#include "runmanager.h"
#include "confmap.h"

#include <cstdio>
#include <cstring>
#include <unistd.h> // usleep()

using namespace std;
static ModuleRegistrar reg1 ("mesytecMtdc32", MesytecMtdc32Module::create);

MesytecMtdc32Module::MesytecMtdc32Module (int i, const QString &n)
    : BaseModule (i, n)
    , firmware(0)
    , event_counter(0)
    , timestamp_counter(0)
    , time_counter(0)
    , buffer_data_length(0)
    , dmx_ (evslots_, this)
{
    setChannels ();
    createOutputPlugin();
    setUI (new MesytecMtdc32UI (this));
        std::cout << "Instantiated MesytecMtdc32 module" << std::endl;
}

void MesytecMtdc32Module::setChannels () {
    EventBuffer *evbuf = RunManager::ref ().getEventBuffer ();

    // Per channel outputs
    //for(int i = 0; i < MTDC32V2_NUM_CHANNELS; i++)
       //evslots_ << evbuf->registerSlot (this, tr("out %1").arg(i,1,10), PluginConnector::VectorUint32);

    // Output for raw data -> to event builder
    evslots_ << evbuf->registerSlot(this, "raw out", PluginConnector::VectorUint32);
}

int MesytecMtdc32Module::configure () {
    AbstractInterface *iface = getInterface ();

    uint32_t baddr = conf_.base_addr;
    uint16_t data;
    int ret = 0;

    if(!iface) return 2;
    if(!iface->isOpen()) return 1;

    // set address sources
    ret = iface->writeA32D16 (baddr + MTDC32V2_ADDR_SOURCE, conf_.addr_source);
    ret = iface->writeA32D16 (baddr + MTDC32V2_ADDR_SOURCE, conf_.addr_source);
    if (ret) printf ("Error %d at MTDC32V2_ADDR_SOURCE \n", ret);

    // set address register
    ret = iface->writeA32D16(baddr + MTDC32V2_ADDR_REGISTER, conf_.base_addr_register);
    if (ret) printf ("Error %d at MTDC32V2_ADDR_REGISTER", ret);

    // set module id
    ret = iface->writeA32D16(baddr + MTDC32V2_MODULE_ID, conf_.module_id);
    if (ret) printf ("Error %d at MTDC32V2_MODULE_ID", ret);

    // set irq level
    ret = iface->writeA32D16(baddr + MTDC32V2_IRQ_LEVEL, conf_.irq_level);
    if (ret) printf ("Error %d at MTDC32V2_", ret);

    // set irq vector
    ret = iface->writeA32D16(baddr + MTDC32V2_IRQ_VECTOR, conf_.irq_vector);
    if (ret) printf ("Error %d at MTDC32V2_IRQ_VECTOR", ret);

    // set irq threshold
    ret = iface->writeA32D16(baddr + MTDC32V2_IRQ_THRESHOLD, conf_.irq_threshold);
    if (ret) printf ("Error %d at MTDC32V2_IRQ_THRESHOLD", ret);

    // set max transfer data
    ret = iface->writeA32D16(baddr + MTDC32V2_MAX_TRANSFER_DATA, conf_.max_transfer_data);
    if (ret) printf ("Error %d at MTDC32V2_MAX_TRANSFER_DATA", ret);

    // set cblt mcst ctrl
    ret = iface->writeA32D16(baddr + MTDC32V2_CBLT_MCST_CTRL, conf_.cblt_mcst_ctrl);
    if (ret) printf ("Error %d at MADC32V2_CBLT_MCST_CTRL", ret);

    // set cblt address
    ret = iface->writeA32D16(baddr + MTDC32V2_CBLT_ADDRESS,
                             conf_.cblt_addr);
    if (ret) printf ("Error %d at MTDC32V2_CBLT_ADDRESS", ret);

    // set mcst address
    ret = iface->writeA32D16(baddr + MTDC32V2_MCST_ADDRESS,
                             conf_.mcst_addr);
    if (ret) printf ("Error %d at MTDC32V2_MCST_ADDRESS", ret);

    // set data length format
    ret = iface->writeA32D16(baddr + MTDC32V2_DATA_LENGTH_FORMAT, conf_.data_length_format);
    if (ret) printf ("Error %d at MTDC32V2_DATA_LENGTH_FORMAT", ret);

    // set multi event mode
    data = conf_.multi_event_mode;
    if(conf_.enable_compare_with_max)
        data |= MTDC32V2_VAL_MULTIEVENT_MODE_MAX_DATA;
    if(conf_.enable_different_eob_marker)
        data |= MTDC32V2_VAL_MULTIEVENT_MODE_EOB_BERR;
    ret = iface->writeA32D16(baddr + MTDC32V2_MULTIEVENT_MODE, data);
    if (ret) printf ("Error %d at MTDC32V2_MULTIEVENT_MODE", ret);

    // set marking type
    ret = iface->writeA32D16(baddr + MTDC32V2_MARKING_TYPE, conf_.marking_type);
    if (ret) printf ("Error %d at MTDC32V2_MARKING_TYPE", ret);

    // set bank mode
    ret = iface->writeA32D16(baddr + MTDC32V2_BANK_MODE, conf_.bank_operation);
    if (ret) printf ("Error %d at MTDC32V2_BANK_MODE", ret);

    // set adc resolution
    ret = iface->writeA32D16(baddr + MTDC32V2_TDC_RESOLUTION, 2+conf_.tdc_resolution);
    if (ret) printf ("Error %d at MTDC32V2_TDC_RESOLUTION", ret);

    // set output format (not necessary at the moment)
    ret = iface->writeA32D16(baddr + MTDC32V2_OUTPUT_FORMAT, conf_.output_format);
    if (ret) printf ("Error %d at MTDC32V2_OUTPUT_FORMAT", ret);

    // set bank 0 window start
    ret = iface->writeA32D16(baddr + MTDC32V2_BANK0_WIN_START, conf_.bank0_win_start);
    if (ret) printf ("Error %d at MTDC32V2_BANK0_WIN_START", ret);

    // set bank 1 window start
    ret = iface->writeA32D16(baddr + MTDC32V2_BANK1_WIN_START, conf_.bank1_win_start);
    if (ret) printf ("Error %d at MTDC32V2_BANK1_WIN_START", ret);

    // set bank 0 window width
    ret = iface->writeA32D16(baddr + MTDC32V2_BANK0_WIN_WIDTH, conf_.bank0_win_width);
    if (ret) printf ("Error %d at MTDC32V2_BANK0_WIN_WIDTH", ret);

    // set bank 1 window width
    ret = iface->writeA32D16(baddr + MTDC32V2_BANK1_WIN_WIDTH, conf_.bank1_win_width);
    if (ret) printf ("Error %d at MTDC32V2_BANK1_WIN_WIDTH", ret);

    // set bank 0 trigger source
    ret = iface->writeA32D16(baddr + MTDC32V2_BANK0_TRIG_SOURCE, conf_.bank0_trig_source);
    if (ret) printf ("Error %d at MTDC32V2_BANK0_TRIG_SOURCE", ret);

    // set bank 1 trigger source
    ret = iface->writeA32D16(baddr + MTDC32V2_BANK1_TRIG_SOURCE, conf_.bank1_trig_source);
    if (ret) printf ("Error %d at MTDC32V2_BANK1_TRIG_SOURCE", ret);

    // set  first hit
    ret = iface->writeA32D16(baddr + MTDC32V2_FIRST_HIT, conf_.only_first_hit);
    if (ret) printf ("Error %d at MTDC32V2_USE_FIRST_HIT", ret);

    // set negative edge
    ret = iface->writeA32D16(baddr + MTDC32V2_NEGATIVE_EDGE, conf_.negative_edge);
    if (ret) printf ("Error %d at MTDC32V2_NEGATIVE_EDGE", ret);

    // set ecl termination
    data = 0;
    if(conf_.enable_termination_input_trig0) data |= (1 << MTDC32V2_OFF_ECL_TERMINATION_TRIG_0);
    if(conf_.enable_termination_input_trig1) data |= (1 << MTDC32V2_OFF_ECL_TERMINATION_TRIG_1);
    if(conf_.enable_termination_input_res) data |= (1 << MTDC32V2_OFF_ECL_TERMINATION_RES);
    ret = iface->writeA32D16(baddr + MTDC32V2_ECL_TERMINATED, data);
    if (ret) printf ("Error %d at MTDC32V2_ECL_TERMINATED", ret);

    // set ecl trig 1 mode
    ret = iface->writeA32D16(baddr + MTDC32V2_ECL_TRIG1_OSC, conf_.ecl_trig1_mode);
    if (ret) printf ("Error %d at MTDC32V2_ECL_trig1_OSC", ret);

    // set ecl busy mode
    ret = iface->writeA32D16(baddr + MTDC32V2_TRIG_SELECT, conf_.trig_select_mode);
    if (ret) printf ("Error %d at MTDC32V2_TRIG_SELECT", ret);

    // set ecl out mode
    ret = iface->writeA32D16(baddr + MTDC32V2_ECL_OUT_CONFIG, conf_.ecl_out_mode);
    if (ret) printf ("Error %d at MTDC32V2_OUT_CONFIG", ret);

    // set nim trig 1 mode
    ret = iface->writeA32D16(baddr + MTDC32V2_NIM_TRIG1_OSC, conf_.nim_trig1_mode);
    if (ret) printf ("Error %d at MTDC32V2_NIM_TRIG1_OSC", ret);

    // set nim busy mode
    ret = iface->writeA32D16(baddr + MTDC32V2_NIM_BUSY, conf_.nim_busy_mode);
    if (ret) printf ("Error %d at MTDC32V2_NIM_BUSY", ret);

    // set pulser status
    ret = iface->writeA32D16(baddr + MTDC32V2_PULSER_STATUS, conf_.test_pulser_mode);
    if (ret) printf ("Error %d at MTDC32V2_PULSER_STATUS", ret);

    ret = iface->writeA32D16(baddr + MTDC32V2_PULSER_PATTERN, conf_.pulser_pattern);
    if (ret) printf ("Error %d at MTDC32V2_PULSER_PATTERN", ret);

    ret = iface->writeA32D16(baddr + MTDC32V2_BANK0_INPUT_THR, conf_.bank0_input_thr);
    if (ret) printf ("Error %d at MTDC32V2_BANK0_INPUT_THR", ret);

    ret = iface->writeA32D16(baddr + MTDC32V2_BANK1_INPUT_THR, conf_.bank1_input_thr);
    if (ret) printf ("Error %d at MTDC32V2_BANK1_INPUT_THR", ret);

    data=0;
    if(conf_.time_stamp_source) data |= (1 << 0);
    if(conf_.enable_ext_ts_reset) data |= (1 << 1);
    ret = iface->writeA32D16(baddr + MTDC32V2_TIMESTAMP_SOURCE, data);
    if (ret) printf ("Error %d at MTDC32V2_TIMESTAMP_SOURCE", ret);

    ret = iface->writeA32D16(baddr + MTDC32V2_HIGH_LIMIT_0, conf_.high_limit0);
    if (ret) printf ("Error %d at MTDC32V2_HIGH_LIMIT_0", ret);

    ret = iface->writeA32D16(baddr + MTDC32V2_LOW_LIMIT_0, conf_.low_limit0);
    if (ret) printf ("Error %d at MTDC32V2_LOW_LIMIT_0", ret);

    ret = iface->writeA32D16(baddr + MTDC32V2_HIGH_LIMIT_1, conf_.high_limit1);
    if (ret) printf ("Error %d at MTDC32V2_HIGH_LIMIT_1", ret);

    ret = iface->writeA32D16(baddr + MTDC32V2_LOW_LIMIT_1, conf_.low_limit1);
    if (ret) printf ("Error %d at MTDC32V2_LOW_LIMIT_1", ret);

    // set template
    //ret = iface->writeA32D16(baddr + MTDC32V2_, conf_.);
    //if (ret) printf ("Error %d at MTDC32V2_", ret);

    //REG_DUMP();

    ret = counterResetAll ();
    return ret;
}

void MesytecMtdc32Module::REG_DUMP() {
    int nof_reg_groups = 2;
    uint32_t start_addr[] = {0x00004000, 0x00006000};
    uint32_t end_addr[]   = {0x0000403e, 0x000060ae};
    uint16_t data;

    printf("MesytecMtdc32Module::REG_DUMP:\n");
    for(int i = 0; i < nof_reg_groups; ++i) {
        for(uint32_t addr = conf_.base_addr + start_addr[i]; addr < conf_.base_addr + end_addr[i]; addr += 2) {
            if(getInterface()->readA32D16(addr,&data) == 0) {
                printf("0x%08x: 0x%04x\n",addr,data);
            }
        }
        printf("*\n");
    }
    fflush(stdout);
}

int MesytecMtdc32Module::counterResetAll () {
    return counterResetAB(MTDC32V2_VAL_RST_COUNTER_AB_ALL);
}

int MesytecMtdc32Module::counterResetAB(uint8_t counter) {
    return getInterface()->writeA32D16(conf_.base_addr + MTDC32V2_RESET_COUNTER_AB,counter);
}

int MesytecMtdc32Module::irqReset() {
    return getInterface()->writeA32D16(conf_.base_addr + MTDC32V2_IRQ_RESET, 1);
}

int MesytecMtdc32Module::irqTest() {
    return getInterface()->writeA32D16(conf_.base_addr + MTDC32V2_IRQ_TEST, 1);
}

int MesytecMtdc32Module::readoutReset () {
    int ret=getInterface()->writeA32D16(conf_.base_addr + MTDC32V2_READOUT_RESET, 1);
    return ret;
}

int MesytecMtdc32Module::startAcquisition() {
    return getInterface()->writeA32D16(conf_.base_addr + MTDC32V2_START_ACQUISITION, 1);
}

int MesytecMtdc32Module::stopAcquisition() {
    return getInterface()->writeA32D16(conf_.base_addr + MTDC32V2_START_ACQUISITION, 0);
}

int MesytecMtdc32Module::fifoReset () {
    return getInterface()->writeA32D16(conf_.base_addr + MTDC32V2_FIFO_RESET, 1);
}

int MesytecMtdc32Module::stopCounter (uint8_t counter) {
    return getInterface()->writeA32D16(conf_.base_addr + MTDC32V2_STOP_COUNTER, counter);
}

// Warning: This reset also resets parameters! Do not issue after configuration!
int MesytecMtdc32Module::softReset () {
    return getInterface()->writeA32D16(conf_.base_addr + MTDC32V2_SOFT_RESET, 1);
}

int MesytecMtdc32Module::reset () {
    irqReset();
    fifoReset();
    readoutReset();
    counterResetAll();
    return softReset ();
}

int MesytecMtdc32Module::panicReset()
{
    return readoutReset();
}

uint16_t MesytecMtdc32Module::getBufferDataLength() const {
        uint16_t data = 0;
        getInterface ()->readA32D16 (conf_.base_addr + MTDC32V2_BUFFER_DATA_LENGTH, &data);
        return data;
}

uint16_t MesytecMtdc32Module::getFirmwareRevision() {
        getInterface ()->readA32D16 (conf_.base_addr + MTDC32V2_FIRMWARE_REVISION, &firmware);
        conf_.firmware_revision_minor = firmware & 0xff;
        conf_.firmware_revision_major = (firmware >> 8) & 0xff;
        return firmware;
}

uint16_t MesytecMtdc32Module::getModuleIdConfigured() {
        if (!module_id)
        getInterface ()->readA32D16 (conf_.base_addr + MTDC32V2_MODULE_ID, &module_id);
        return module_id;
}

bool MesytecMtdc32Module::checkFirmware()  {
    if(!firmware) getFirmwareRevision();
    if(firmware != conf_.firmware_expected) {
        printf("MesytecMtdc32Module::checkFirmware : Firmware mismatch (0x%04x, expected: 0x%04x)\n",
               firmware,conf_.firmware_expected);
        return false;
    }
    return true;
}

int MesytecMtdc32Module::getAllCounters() {
    event_counter = getEventCounter();
    timestamp_counter = getTimestampCounter();
    time_counter = getTime();
    return 0;
}

uint32_t MesytecMtdc32Module::getEventCounter() {
    int ret = 0;
    uint32_t data32;
    uint16_t data;

    AbstractInterface *iface = getInterface ();

    ret = iface->readA32D16 (conf_.base_addr + MTDC32V2_EVENT_COUNTER_LOW, &data);
    if (ret) printf ("Error %d at MTDC32V2_EVENT_COUNTER_LOW\n", ret);
    data32 = data;

    ret = iface->readA32D16 (conf_.base_addr + MTDC32V2_EVENT_COUNTER_HIGH, &data);
    if (ret) printf ("Error %d at MTDC32V2_EVENT_COUNTER_HIGH\n", ret);
    data32 |= (((uint32_t)(data) & 0x0000ffff) << 16);

    printf("Event counter: %d\n",data32);

    return data32;
}

uint32_t MesytecMtdc32Module::getTimestampCounter() {
    int ret = 0;
    uint32_t data32;
    uint16_t data;

    AbstractInterface *iface = getInterface ();

    ret = iface->readA32D16 (conf_.base_addr + MTDC32V2_TIMESTAMP_CNT_L, &data);
    if (ret) printf ("Error %d at MTDC32V2_TIMESTAMP_CNT_L", ret);
    data32 = data;

    ret = iface->readA32D16 (conf_.base_addr + MTDC32V2_TIMESTAMP_CNT_H, &data);
    if (ret) printf ("Error %d at MTDC32V2_TIMESTAMP_CNT_H", ret);
    data32 |= (((uint32_t)(data) & 0x0000ffff) << 16);

    return data32;
}

uint64_t MesytecMtdc32Module::getTime() {
    int ret = 0;
    uint32_t data64 = 0LL;
    uint16_t data;

    AbstractInterface *iface = getInterface ();

    ret = iface->readA32D16 (conf_.base_addr + MTDC32V2_TIME_0, &data);
    if (ret) printf ("Error %d at MTDC32V2_TIME_0", ret);
    data64 = data;

    ret = iface->readA32D16 (conf_.base_addr + MTDC32V2_TIME_1, &data);
    if (ret) printf ("Error %d at MTDC32V2_TIME_1", ret);
    data64 |= (((uint64_t)(data) & 0xffffLL) << 16);

    ret = iface->readA32D16 (conf_.base_addr + MTDC32V2_TIME_2, &data);
    if (ret) printf ("Error %d at MTDC32V2_TIME_2", ret);
    data64 |= (((uint64_t)(data) & 0xffffLL) << 32);

    return data64;
}

bool MesytecMtdc32Module::getDataReady() {
    int ret;
    uint16_t data;
    ret = getInterface ()->readA32D16 (conf_.base_addr + MTDC32V2_DATA_READY, &data);
    if (ret) printf ("Error %d at MTDC32V2_DATA_READY", ret);

    return (data == 1);
}

bool MesytecMtdc32Module::dataReady () {
    //return getDataReady();
    return (getBufferDataLength() > 0);
}

int MesytecMtdc32Module::acquire (Event* ev) {
    int ret;

    ret = acquireSingle (data, &buffer_data_length);
    if (ret == 0) writeToBuffer(ev);
    else printf("MesytecMtdc32Module::Error at acquireSingle\n");

    return buffer_data_length;
}

void MesytecMtdc32Module::writeToBuffer(Event *ev)
{
    bool go_on = dmx_.processData (ev, data, buffer_data_length);
    if (!go_on) {
        // Do what has to be done to finish this acquisition cycle
         //fifoReset();
    }
}

int MesytecMtdc32Module::acquireSingle (uint32_t *data, uint32_t *rd) {
    *rd = 0;

    // Get buffer data length
    uint32_t words_to_read = 0;
    buffer_data_length = getBufferDataLength();
    //printf("mtdc32: Event length (buffer_data_length): %d\n",buffer_data_length);

    // Translate buffer data length to number of words to read
    switch(conf_.data_length_format) {
    case MesytecMtdc32ModuleConfig::dl8bit:
        words_to_read = buffer_data_length / 4;
        break;
    case MesytecMtdc32ModuleConfig::dl16bit:
        words_to_read = buffer_data_length / 2;
        break;
    case MesytecMtdc32ModuleConfig::dl64bit:
        words_to_read = buffer_data_length * 2;
        break;
    case MesytecMtdc32ModuleConfig::dl32bit:
    default:
        words_to_read = buffer_data_length;
    }

    ++words_to_read;
    //printf("mtdc32: Words to read: %d\n",words_to_read);

    // Read the data fifo
    uint32_t addr = conf_.base_addr + MTDC32V2_DATA_FIFO;
    //printf("mtdc32: acquireSingle from addr = 0x%08x\n",addr);

    switch(conf_.vme_mode) {

    case MesytecMtdc32ModuleConfig::vmFIFO:
    {
        //printf("MesytecMtdc32ModuleConfig::vmFIFO\n");
        int ret = getInterface ()->readA32FIFO(addr, data, words_to_read, rd);
        if (!*rd && ret && !getInterface ()->isBusError (ret)) {
            printf ("Error %d at MTDC32V2_DATA_FIFO with FIFO read\n", ret);
            return ret;
        }
        break;
    }
    case MesytecMtdc32ModuleConfig::vmDMA32:
    {
        //printf("MesytecMtdc32ModuleConfig::vmDMA32\n");
        int ret = getInterface ()->readA32DMA32(addr, data, words_to_read, rd);
        if (!*rd && ret && !getInterface ()->isBusError (ret)) {
            printf ("Error %d at MTDC32V2_DATA_FIFO with DMA32\n", ret);
            return ret;
        }
        break;
    }
    case MesytecMtdc32ModuleConfig::vmBLT32:
    {
        //printf("MesytecMtdc32ModuleConfig::vmBLT32\n");
        int ret = getInterface ()->readA32BLT32(addr, data, words_to_read, rd);
        if (!*rd && ret && !getInterface ()->isBusError (ret)) {
            printf ("Error %d at MTDC32V2_DATA_FIFO with BLT32\n", ret);
            return ret;
        }
        break;
    }
    case MesytecMtdc32ModuleConfig::vmBLT64:
    {
        //printf("MesytecMtdc32ModuleConfig::vmBLT64\n");
        int ret = getInterface ()->readA32MBLT64(addr, data, words_to_read, rd);
        if (!*rd && ret && !getInterface ()->isBusError (ret)) {
            printf ("Error %d at MTDC32V2_DATA_FIFO with MBLT64\n", ret);
            return ret;
        }
        break;
    }
    case MesytecMtdc32ModuleConfig::vm2ESST: // Not handled by the module
    case MesytecMtdc32ModuleConfig::vmSingle:
    {
        //printf("MesytecMtdc32ModuleConfig::vmSingle\n");

        int idx = 0;
        for(uint i = 0; i < words_to_read-1; ++i) {
              int ret = getInterface()->readA32D32(addr,&(data[idx]));
              if(ret) {
                  printf ("Error %d at MTDC32V2_DATA_FIFO with D32\n", ret);
                  return ret;
                  }
              ++idx;
              }

        (*rd) = words_to_read-1;
        break;
    }
    }


    // Reset readout logic
//    int ret = fifoReset();
//    if(ret) {
//        printf ("Error %d at MTDC32V2_FIFO_RESET with D32\n", ret);
//    }
    //usleep(1000);
    //int ret = readoutReset();
    //usleep(1000);
    int ret = readoutReset();
    if(ret)
        printf ("Error %d at MTDC32V2_READOUT_RESET with D32\n", ret);

    // Dump the data
//    printf("\nEvent dump:\n");
//    for(int i = 0; i < (*rd); ++i) {
//        printf("<%d> 0x%08x\n",i,data[i]);
//    }
//    printf("\n"); fflush(stdout);

    return 0;
}

void MesytecMtdc32Module::counterResetSync()
{
    counterResetAll();
}

void MesytecMtdc32Module::singleShot (uint32_t *data, uint32_t *rd) {
    bool triggered = false;

    if(!getInterface()->isOpen()) {
        getInterface()->open();
    }

    reset();
    configure();

    usleep(500);

    fifoReset();
    startAcquisition();
    readoutReset();

    uint tmp_poll_cnt = conf_.pollcount;
    conf_.pollcount = 100000;

    for (unsigned int i = 0; i < conf_.pollcount; ++i) {
        if (dataReady ()) {
            triggered = true;
            break;
        }
    }

    if (!triggered)
        std::cout << "MTDC32: No data after " << conf_.pollcount
                  << " trigger loops" << std::endl << std::flush;
    else
        acquireSingle (data, rd);

    conf_.pollcount = tmp_poll_cnt;

    stopAcquisition();

    // Unpack the data

    mtdc32_header_t header;
    int idx = 0;
    header.data = data[idx++];

    if(header.bits.signature != MTDC32V2_SIG_HEADER) {
        printf("Invalid header word signature: %d\n",header.bits.signature);
    }

//    printf("Header info:\t(0x%08x)\n",header.data);
//    printf("ADC resolution: %d\n",header.bits.adc_resolution);
//    printf("Module ID: %d\n",header.bits.module_id);
//    printf("Data length: %d\n",header.bits.data_length);

//    printf("###############\n");

    if(header.bits.data_length != (*rd)-1) {
        printf("Data length mismatch: (header.bits.data_length = %d) (read-1 = %d)\n",
               header.bits.data_length,(*rd)-1);
        return;
    }

    for(int i = 0; i < MTDC32V2_NUM_CHANNELS; ++i) {
        current_energy[i] = 0;
    }

    for(int i = 0; i < header.bits.data_length-1; ++i) {
        mtdc32_data_t datum;
        datum.data = data[idx++];

        if(datum.bits.signature != MTDC32V2_SIG_DATA) {
            printf("Invalid data word signature: %d\n",datum.bits.signature);
        }

        if(datum.bits.sub_signature == MTDC32V2_SIG_DATA_EVENT) {
//            printf("Energy (%d): %d\t(0x%08x)\n",datum.bits.channel,datum.bits.value,datum.data);
        } else {
            //printf("Found word with sub-signature: %d\n",datum.bits.sub_signature);
        }
        current_energy[datum.bits.channel] = datum.bits.value;
    }

    mtdc32_end_of_event_t trailer;
    trailer.data = data[idx++];

    if(trailer.bits.signature != MTDC32V2_SIG_END) {
        printf("Invalid trailer word signature: %d\n",trailer.bits.signature);
    }

//    printf("###############\n");

//    printf("Trailer info:\t(0x%08x)\n",trailer.data);
//    printf("Trigger counter / Time stamp: %d\n",trailer.bits.trigger_counter);

//    printf("###############\n\n");

    current_module_id = header.bits.module_id;
    current_resolution = header.bits.tdc_resolution;
    current_time_stamp = trailer.bits.trigger_counter;

    fflush(stdout);
}

int MesytecMtdc32Module::updateModuleInfo() {
    uint16_t firmware_from_module = getFirmwareRevision();
    uint16_t module_id_from_module = getModuleIdConfigured();

    Q_UNUSED (firmware_from_module)
    Q_UNUSED (module_id_from_module)

    return 0;
}

typedef ConfMap::confmap_t<MesytecMtdc32ModuleConfig> confmap_t;
static const confmap_t confmap [] = {
    confmap_t ("addr_source", (uint16_t MesytecMtdc32ModuleConfig::*)
                &MesytecMtdc32ModuleConfig::addr_source),
    confmap_t ("base_addr", &MesytecMtdc32ModuleConfig::base_addr),
    confmap_t ("base_addr_register", &MesytecMtdc32ModuleConfig::base_addr_register),
    confmap_t ("module_id", &MesytecMtdc32ModuleConfig::module_id),
    confmap_t ("firmware_expected", &MesytecMtdc32ModuleConfig::firmware_expected),
    confmap_t ("irq_level", &MesytecMtdc32ModuleConfig::irq_level),
    confmap_t ("irq_vector", &MesytecMtdc32ModuleConfig::irq_vector),
    confmap_t ("irq_threshold", &MesytecMtdc32ModuleConfig::irq_threshold),
    confmap_t ("max_transfer_data", &MesytecMtdc32ModuleConfig::max_transfer_data),
    confmap_t ("cblt_mcst_ctrl", &MesytecMtdc32ModuleConfig::cblt_mcst_ctrl),
    confmap_t ("cblt_active", &MesytecMtdc32ModuleConfig::cblt_active),
    confmap_t ("mcst_active", &MesytecMtdc32ModuleConfig::mcst_active),
    confmap_t ("enable_cblt_first", &MesytecMtdc32ModuleConfig::enable_cblt_first),
    confmap_t ("enable_cblt_last", &MesytecMtdc32ModuleConfig::enable_cblt_last),
    confmap_t ("enable_cblt_middle", &MesytecMtdc32ModuleConfig::enable_cblt_middle),
    confmap_t ("cblt_addr", &MesytecMtdc32ModuleConfig::cblt_addr),
    confmap_t ("mcst_addr", &MesytecMtdc32ModuleConfig::mcst_addr),
    confmap_t ("data_length_format", (uint16_t MesytecMtdc32ModuleConfig::*) &MesytecMtdc32ModuleConfig::data_length_format),
    confmap_t ("multi_event_mode", (uint16_t MesytecMtdc32ModuleConfig::*) &MesytecMtdc32ModuleConfig::multi_event_mode),
    confmap_t ("enable_different_eob_marker", &MesytecMtdc32ModuleConfig::enable_different_eob_marker),
    confmap_t ("enable_compare_with_max", &MesytecMtdc32ModuleConfig::enable_compare_with_max),
    confmap_t ("marking_type", (uint16_t MesytecMtdc32ModuleConfig::*) &MesytecMtdc32ModuleConfig::marking_type),
    confmap_t ("bank_operation", (uint16_t MesytecMtdc32ModuleConfig::*) &MesytecMtdc32ModuleConfig::bank_operation),
    confmap_t ("tdc_resolution", (uint16_t MesytecMtdc32ModuleConfig::*) &MesytecMtdc32ModuleConfig::tdc_resolution),
    confmap_t ("output_format", (uint16_t MesytecMtdc32ModuleConfig::*) &MesytecMtdc32ModuleConfig::output_format),
    confmap_t ("bank0_win_start", (uint16_t MesytecMtdc32ModuleConfig::*) &MesytecMtdc32ModuleConfig::bank0_win_start),
    confmap_t ("bank1_win_start", (uint16_t MesytecMtdc32ModuleConfig::*) &MesytecMtdc32ModuleConfig::bank1_win_start),
    confmap_t ("bank0_win_width", (uint16_t MesytecMtdc32ModuleConfig::*) &MesytecMtdc32ModuleConfig::bank0_win_width),
    confmap_t ("bank1_win_width", (uint16_t MesytecMtdc32ModuleConfig::*) &MesytecMtdc32ModuleConfig::bank1_win_width),
    confmap_t ("bank0_trig_source", (uint16_t MesytecMtdc32ModuleConfig::*) &MesytecMtdc32ModuleConfig::bank0_trig_source),
    confmap_t ("bank1_trig_source", (uint16_t MesytecMtdc32ModuleConfig::*) &MesytecMtdc32ModuleConfig::bank1_trig_source),
    confmap_t ("only_first_hit", &MesytecMtdc32ModuleConfig::only_first_hit),
    confmap_t ("negative_edge", (uint16_t MesytecMtdc32ModuleConfig::*) &MesytecMtdc32ModuleConfig::negative_edge),
    confmap_t ("enable_termination_input_trig0", &MesytecMtdc32ModuleConfig::enable_termination_input_trig0),
    confmap_t ("enable_termination_input_trig1", &MesytecMtdc32ModuleConfig::enable_termination_input_trig1),
    confmap_t ("enable_termination_input_res", &MesytecMtdc32ModuleConfig::enable_termination_input_res),
    confmap_t ("ecl_trig1_mode", (uint16_t MesytecMtdc32ModuleConfig::*) &MesytecMtdc32ModuleConfig::ecl_trig1_mode),
    confmap_t ("ecl_out_mode", (uint16_t MesytecMtdc32ModuleConfig::*) &MesytecMtdc32ModuleConfig::ecl_out_mode),
    confmap_t ("trig_select_mode", (uint16_t MesytecMtdc32ModuleConfig::*) &MesytecMtdc32ModuleConfig::trig_select_mode),
    confmap_t ("nim_trig1_mode", (uint16_t MesytecMtdc32ModuleConfig::*) &MesytecMtdc32ModuleConfig::nim_trig1_mode),
    confmap_t ("nim_busy_mode", (uint16_t MesytecMtdc32ModuleConfig::*) &MesytecMtdc32ModuleConfig::nim_busy_mode),
    confmap_t ("test_pulser_mode", (uint16_t MesytecMtdc32ModuleConfig::*) &MesytecMtdc32ModuleConfig::test_pulser_mode),
    confmap_t ("pulser_pattern", (uint16_t MesytecMtdc32ModuleConfig::*) &MesytecMtdc32ModuleConfig::pulser_pattern),
    confmap_t ("bank0_input_thr", (uint16_t MesytecMtdc32ModuleConfig::*) &MesytecMtdc32ModuleConfig::bank0_input_thr),
    confmap_t ("bank1_input_thr", (uint16_t MesytecMtdc32ModuleConfig::*) &MesytecMtdc32ModuleConfig::bank1_input_thr),
    confmap_t ("time_stamp_source", (uint16_t MesytecMtdc32ModuleConfig::*) &MesytecMtdc32ModuleConfig::time_stamp_source),
    confmap_t ("enable_ext_ts_reset", &MesytecMtdc32ModuleConfig::enable_ext_ts_reset),

    confmap_t ("time_stamp_divisor", &MesytecMtdc32ModuleConfig::time_stamp_divisor),
    confmap_t ("pollcount", &MesytecMtdc32ModuleConfig::pollcount),
    confmap_t ("vme_mode", (uint16_t MesytecMtdc32ModuleConfig::*)
                &MesytecMtdc32ModuleConfig::vme_mode),

    confmap_t ("high_limit0", (uint16_t MesytecMtdc32ModuleConfig::*)
                &MesytecMtdc32ModuleConfig::high_limit0),
    confmap_t ("low_limit0", (uint16_t MesytecMtdc32ModuleConfig::*)
                &MesytecMtdc32ModuleConfig::low_limit0),
    confmap_t ("high_limit1", (uint16_t MesytecMtdc32ModuleConfig::*)
                &MesytecMtdc32ModuleConfig::high_limit1),
    confmap_t ("low_limit1", (uint16_t MesytecMtdc32ModuleConfig::*)
                &MesytecMtdc32ModuleConfig::low_limit1),
};

void MesytecMtdc32Module::applySettings (QSettings *settings) {
    std::cout << "Applying settings for " << getName ().toStdString () << "... ";
    rememberedSettings=settings;
    settings->beginGroup (getName ());

    ConfMap::apply (settings, &conf_, confmap);

    for (int i = 0; i < MTDC32V2_NUM_CHANNELS; ++i) {
        QString key = QString ("enable_channel%1").arg (i);
        if (settings->contains (key))
            conf_.enable_channel[i] = settings->value (key).toBool ();
    }
    settings->endGroup ();
    std::cout << "done" << std::endl;

    if(getUI()) getUI ()->applySettings ();
}

void MesytecMtdc32Module::saveSettings (QSettings *settings) {
    std::cout << "Saving settings for " << getName ().toStdString () << "... ";
    settings->beginGroup (getName ());
    ConfMap::save (settings, &conf_, confmap);
    for (int i = 0; i < MTDC32V2_NUM_CHANNELS; ++i) {
        QString key = QString ("enable_channel%1").arg (i);
        settings->setValue (key, conf_.enable_channel [i]);
    }

    settings->endGroup ();
    std::cout << "done" << std::endl;
}

int MesytecMtdc32Module::getSettings () {
    switch(conf_.data_length_format) {
    case MesytecMtdc32ModuleConfig::dl8bit:
        return 8;
        break;
    case MesytecMtdc32ModuleConfig::dl16bit:
        return 16;
        break;
    case MesytecMtdc32ModuleConfig::dl64bit:
        return 64;
        break;
    case MesytecMtdc32ModuleConfig::dl32bit:
    default:
        return 32;
    }
}

void MesytecMtdc32Module::setBaseAddress (uint32_t baddr) {
    conf_.base_addr = baddr;
    if(getUI()) getUI ()->applySettings ();
}

uint32_t MesytecMtdc32Module::getBaseAddress () const {
    return conf_.base_addr;
}
/*!
\page Mesytec MTDC-32 module
<b>Module name:</b> \c mesytecMtdc32

\section desc Module Description
The Mesytec MTDC-32 module is a 32 channel ADC.

\section cpanel Configuration Panel

\subsection settings Settings

\subsection irq IRQ
The IRQ panel controls the conditions on which VME interrupt requests are generated. IRQs are not yet used in data readout.

\subsection info Info
The Info panel provides some basic information about the firmware that runs on the module.
This page is not fully implemented yet.

\section outs Outputs

*/

