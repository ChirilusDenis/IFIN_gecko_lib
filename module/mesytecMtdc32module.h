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

#ifndef MESYTECMTDC32_H
#define MESYTECMTDC32_H

#include "basemodule.h"
#include "baseplugin.h"
#include "mesytecMtdc32dmx.h"
#include "pluginmanager.h"
#include "mesytec_mtdc_32_v2.h"
#include <fstream>

struct MesytecMtdc32ModuleConfig {
    enum AddressSource{asBoard,asRegister};
    enum DataLengthFormat{dl8bit,dl16bit,dl32bit,dl64bit};
    enum MultiEventMode{meSingle,meMulti1,meMulti2,meMulti3};
    enum MarkingType{mtEventCounter,mtTimestamp,mtReserved2,mtExtendedTs};
    enum BankOperation{boConnected,boIndependent};
    enum OutputFormat{standard,full_time_stamp};
    enum EclTrig1Mode{egTrig,egOscillator};
    enum TrigSelectMode{ebNIM,ebECL};
    enum NimTrig1Mode{ngTrig,ngOscillator};
    enum TimeStampSource{tsVme,tsExternal};
    enum VmeMode{vmSingle,vmDMA32,vmFIFO,vmBLT32,vmBLT64,vm2ESST};

    AddressSource addr_source;
    uint32_t base_addr;
    uint16_t base_addr_register;
    uint8_t module_id;
    uint16_t firmware_expected;
    uint8_t firmware_revision_major;
    uint8_t firmware_revision_minor;

    uint8_t irq_level;
    uint8_t irq_vector;
    uint16_t irq_threshold;
    uint16_t max_transfer_data;

    uint8_t cblt_mcst_ctrl;
    bool cblt_active;
    bool mcst_active;
    bool enable_cblt_first;
    bool enable_cblt_last;
    bool enable_cblt_middle;
    uint8_t cblt_addr;
    uint8_t mcst_addr;

    DataLengthFormat data_length_format;
    
    MultiEventMode multi_event_mode;
    bool enable_different_eob_marker;
    bool enable_compare_with_max;
    MarkingType marking_type;

    BankOperation bank_operation;
    uint8_t tdc_resolution;
    int output_format;

    uint16_t bank0_win_start;
    uint16_t bank1_win_start;
    uint16_t bank0_win_width;
    uint16_t bank1_win_width;
    uint16_t bank0_trig_source;
    uint16_t bank1_trig_source;
    uint16_t only_first_hit;

    uint16_t negative_edge;
    bool enable_termination_input_trig0;
    bool enable_termination_input_trig1;
    bool enable_termination_input_res;
    EclTrig1Mode ecl_trig1_mode;
    uint16_t ecl_out_mode;
    TrigSelectMode trig_select_mode;
    NimTrig1Mode nim_trig1_mode;
    uint16_t nim_busy_mode;

    bool test_pulser_mode;
    uint8_t pulser_pattern;
    uint8_t bank0_input_thr;
    uint8_t bank1_input_thr;

    TimeStampSource time_stamp_source;    
    bool enable_ext_ts_reset;
    uint16_t time_stamp_divisor;



    VmeMode vme_mode;

    unsigned int pollcount;

    uint8_t high_limit0;
    uint8_t low_limit0;
    uint8_t high_limit1;
    uint8_t low_limit1;

bool enable_channel[32];

    MesytecMtdc32ModuleConfig ()
        : addr_source(asBoard), base_addr(0),
          base_addr_register(0),module_id(0xFF),
          firmware_expected(MTDC32V2_2_EXPECTED_FIRMWARE),
          irq_level(0),irq_vector(0),irq_threshold(1),
          max_transfer_data(1),cblt_mcst_ctrl(0),
	  cblt_active(0),mcst_active(0),
          cblt_addr(0xAA),mcst_addr(0xBB),
          data_length_format(dl32bit),
          multi_event_mode(meSingle),
          enable_different_eob_marker(false),
          enable_compare_with_max(false),
          marking_type(mtEventCounter),
          bank_operation(boConnected),
          tdc_resolution(0),
          output_format(standard),
	  bank0_win_start(16384),bank1_win_start(16384),
	  bank0_win_width(32), bank1_win_width(32),
	  bank0_trig_source(1), bank1_trig_source(2),
          only_first_hit(3),
          negative_edge(0),
          enable_termination_input_trig0(false),
          enable_termination_input_trig1(false),
          enable_termination_input_res(false),
          ecl_trig1_mode(egTrig),
          ecl_out_mode(0),
          trig_select_mode(ebNIM),
          nim_trig1_mode(ngTrig),
          nim_busy_mode(0),
          test_pulser_mode(0),
          pulser_pattern(0),
	  bank0_input_thr(105),bank1_input_thr(105),
          time_stamp_source(tsVme), enable_ext_ts_reset(0),
          time_stamp_divisor(1),
          vme_mode(vmSingle),
          pollcount(100000),
          high_limit0(255),low_limit0(0),
          high_limit1(255),low_limit1(0)
    {
        for (int i = 0; i < MTDC32V2_NUM_CHANNELS; ++i) 
            enable_channel[i] = true;
     }
};

class MesytecMtdc32Module : public BaseModule {
	Q_OBJECT
public:
    // Factory method
    static AbstractModule *create (int id, const QString &name) {
        return new MesytecMtdc32Module (id, name);
    }

    // Settings
    virtual void saveSettings (QSettings*);
    virtual void applySettings (QSettings*);
    virtual int getSettings();

    // Functions
    int softReset();
    QSettings* rememberedSettings;

    int irqTest();
    int irqReset();

    int readoutReset();
    int startAcquisition();
    int stopAcquisition();
    int fifoReset();
 
    inline int counterResetAll();
    inline int counterResetAB(uint8_t counter);
    inline int stopCounter(uint8_t counter);

    // getters
    uint16_t getFirmwareRevision();
    uint16_t getModuleIdConfigured();
    uint16_t getBufferDataLength() const; // Units are as set in data_length_format
    bool getDataReady();
    int getAllCounters();
    uint32_t getEventCounter();
    uint32_t getTimestampCounter();
    uint64_t getTime();

    int updateModuleInfo();

    // checks
    bool checkFirmware();

    // Remote control bus control
    int writeRcBus(uint8_t addr, uint16_t data);
    uint16_t readRcBus(uint8_t addr);
    int setRcBusNumber(uint8_t number); // makes no sense here
    int setRcModuleNumber(uint8_t number);
    inline uint8_t getRcReturnStatus();
    uint8_t getRcBusNumber();
    uint8_t getRcModuleNumber();
    bool rcBusReady();

    // Mandatory virtual functions
    virtual void setChannels ();
    virtual int acquire (Event* ev);
    virtual bool dataReady ();
    virtual int reset ();
    virtual void counterResetSync();
    virtual int panicReset ();
    virtual int configure ();

    virtual uint32_t getBaseAddress () const;
    virtual void setBaseAddress (uint32_t baddr);
    virtual void runStartingEvent(){};

    MesytecMtdc32ModuleConfig *getConfig () { return &conf_; }

    int acquireSingle (uint32_t *data, uint32_t *rd);

private:
    MesytecMtdc32Module (int _id, const QString &);
    void writeToBuffer(Event *ev);

public slots:
    virtual void prepareForNextAcquisition () {}
    void singleShot (uint32_t *data, uint32_t *rd);
    void REG_DUMP();

public:
    MesytecMtdc32ModuleConfig conf_;
    uint32_t current_module_id;
    uint32_t current_energy[MTDC32V2_NUM_CHANNELS];
    uint32_t current_time_stamp;
    uint32_t current_resolution;

private:
    uint16_t firmware;
    uint16_t module_id;
    uint32_t event_counter;
    uint32_t timestamp_counter;
    uint32_t time_counter;
    uint32_t buffer_data_length; // unit depends of conf_.data_length_format
    uint32_t data [48640];


    MesytecMtdc32Demux dmx_;
    QVector<EventSlot*> evslots_;
};

#endif // MESYTECMTDC32_H
