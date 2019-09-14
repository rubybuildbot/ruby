#pragma once
#include <cstdint>
#include <queue>
#include <memory>
#include "InterruptController.hpp"



/*
1F801800h - Index/Status Register (Bit0-1 R/W) (Bit2-7 Read Only)
0-1 Index   Port 1F801801h-1F801803h index (0..3 = Index0..Index3)   (R/W)
2   ADPBUSY XA-ADPCM fifo empty  (0=Empty) ;set when playing XA-ADPCM sound
3   PRMEMPT Parameter fifo empty (1=Empty) ;triggered before writing 1st byte
4   PRMWRDY Parameter fifo full  (0=Full)  ;triggered after writing 16 bytes
5   RSLRRDY Response fifo empty  (0=Empty) ;triggered after reading LAST byte
6   DRQSTS  Data fifo empty      (0=Empty) ;triggered after reading LAST byte
7   BUSYSTS Command/parameter transmission busy  (1=Busy)
*/
union CDROMStatus {
    struct {
        uint8_t index : 2;
        uint8_t XAADCPMFifoEmpty : 1;
        uint8_t parameterFifoEmpty : 1;
        uint8_t parameterFifoFull : 1;
        uint8_t responseFifoEmpty : 1;
        uint8_t dataFifoEmpty : 1;
        uint8_t transmissionBusy : 1;
    };

    uint8_t _value;

    CDROMStatus() : _value(0x18) {}
};

/*
1F801803h.Index0 - Interrupt Enable Register (R)
1F801803h.Index2 - Interrupt Enable Register (R) (Mirror)
0-4  Interrupt Enable Bits (usually all set, ie. 1Fh=Enable All IRQs)
5-7  Unknown/unused (write: should be zero) (read: usually all bits set)
*/
union CDROMInterrupt {
    struct {
        uint8_t enable : 4;
        uint8_t unknown : 4;
    };

    uint8_t _value;

    CDROMInterrupt() : _value(0) {}
};

enum CDROMState : uint8_t {
    Unknown = 0,
    Reading = 5,
    Seeking = 6,
    Playing = 7,
};

/*
Status code (stat)
0  Error         Invalid Command/parameters (followed by Error Byte)
1  Spindle Motor (0=Motor off, or in spin-up phase, 1=Motor on)
2  SeekError     (0=Okay, 1=Seek error)     (followed by Error Byte)
3  IdError       (0=Okay, 1=GetID denied) (also set when Setmode.Bit4=1)
4  ShellOpen     Once shell open (0=Closed, 1=Is/was Open)
5  Read          Reading data sectors  ;/set until after Seek completion)
6  Seek          Seeking               ; at a time (ie. Read/Play won't get
7  Play          Playing CD-DA         ;\only ONE of these bits can be set
*/
union CDROMStatusCode {
    struct {
        uint8_t error : 1;
        uint8_t spindleMotor : 1;
        uint8_t seekError : 1;
        uint8_t getIdError : 1;
        uint8_t shellOpen : 1;
        uint8_t read : 1;
        uint8_t seek : 1;
        uint8_t play : 1;
    };

    uint8_t _value;

    CDROMStatusCode() : _value(0x10) {}

    void setState(CDROMState state) {
        error = false;
        spindleMotor = true;
        seekError = false;
        getIdError = false;
        if (state == Unknown) {
            return;
        }
        uint8_t mask = 1 << state;
        _value |= mask;
    }

    void setShellOpen(bool open) {
        shellOpen = open;
        if (!shellOpen) {
            setState(CDROMState::Unknown);
        }
    }
};

class CDROM {
    std::unique_ptr<InterruptController> &interruptController;
    bool logActivity;

    CDROMStatus status;
    CDROMInterrupt interrupt;
    CDROMStatusCode statusCode;

    std::queue<uint8_t> parameters;
    std::queue<uint8_t> response;
/*
INT0   No response received (no interrupt request)
INT1   Received SECOND (or further) response to ReadS/ReadN (and Play+Report)
INT2   Received SECOND response (to various commands)
INT3   Received FIRST response (to any command)
INT4   DataEnd (when Play/Forward reaches end of disk) (maybe also for Read?)
INT5   Received error-code (in FIRST or SECOND response)
        INT5 also occurs on SECOND GetID response, on unlicensed disks
        INT5 also occurs when opening the drive door (even if no command
        was sent, ie. even if no read-command or other command is active)
INT6   N/A
INT7   N/A
*/
    std::queue<uint8_t> interruptQueue;
    uint32_t seekSector;
    uint32_t readSector;

    void setStatusRegister(uint8_t value);
    void setInterruptRegister(uint8_t value);
    void setInterruptFlagRegister(uint8_t value);
    void execute(uint8_t value);

    uint8_t getStatusRegister() const;
    uint8_t getInterruptFlagRegister() const;
    uint8_t getReponse();

    void clearParameters();
    void clearInterruptQueue();
    void clearResponse();
    void pushParameter(uint8_t value);
    void pushResponse(uint8_t value);
    uint8_t popParameter();

    void updateStatusRegister();

/*
Command          Parameters      Response(s)
00h -            -               INT5(11h,40h)  ;reportedly "Sync" uh?
01h Getstat      -               INT3(stat)
02h Setloc     E amm,ass,asect   INT3(stat)
03h Play       E (track)         INT3(stat), optional INT1(report bytes)
04h Forward    E -               INT3(stat), optional INT1(report bytes)
05h Backward   E -               INT3(stat), optional INT1(report bytes)
06h ReadN      E -               INT3(stat), INT1(stat), datablock
07h MotorOn    E -               INT3(stat), INT2(stat)
08h Stop       E -               INT3(stat), INT2(stat)
09h Pause      E -               INT3(stat), INT2(stat)
0Ah Init         -               INT3(late-stat), INT2(stat)
0Bh Mute       E -               INT3(stat)
0Ch Demute     E -               INT3(stat)
0Dh Setfilter  E file,channel    INT3(stat)
0Eh Setmode      mode            INT3(stat)
0Fh Getparam     -               INT3(stat,mode,null,file,channel)
10h GetlocL    E -               INT3(amm,ass,asect,mode,file,channel,sm,ci)
11h GetlocP    E -               INT3(track,index,mm,ss,sect,amm,ass,asect)
12h SetSession E session         INT3(stat), INT2(stat)
13h GetTN      E -               INT3(stat,first,last)  ;BCD
14h GetTD      E track (BCD)     INT3(stat,mm,ss)       ;BCD
15h SeekL      E -               INT3(stat), INT2(stat)  ;\use prior Setloc
16h SeekP      E -               INT3(stat), INT2(stat)  ;/to set target
17h -            -               INT5(11h,40h)  ;reportedly "SetClock" uh?
18h -            -               INT5(11h,40h)  ;reportedly "GetClock" uh?
19h Test         sub_function    depends on sub_function (see below)
1Ah GetID      E -               INT3(stat), INT2/5(stat,flg,typ,atip,"SCEx")
1Bh ReadS      E?-               INT3(stat), INT1(stat), datablock
1Ch Reset        -               INT3(stat), Delay
1Dh GetQ       E adr,point       INT3(stat), INT2(10bytesSubQ,peak_lo) ;\not
1Eh ReadTOC      -               INT3(late-stat), INT2(stat)           ;/vC0
1Fh VideoCD      sub,a,b,c,d,e   INT3(stat,a,b,c,d,e)   ;<-- SCPH-5903 only
1Fh..4Fh -       -               INT5(11h,40h)  ;-Unused/invalid
50h Secret 1     -               INT5(11h,40h)  ;\
51h Secret 2     "Licensed by"   INT5(11h,40h)  ;
52h Secret 3     "Sony"          INT5(11h,40h)  ; Secret Unlock Commands
53h Secret 4     "Computer"      INT5(11h,40h)  ; (not in version vC0, and,
54h Secret 5     "Entertainment" INT5(11h,40h)  ; nonfunctional in japan)
55h Secret 6     "<region>"      INT5(11h,40h)  ;
56h Secret 7     -               INT5(11h,40h)  ;/
57h SecretLock   -               INT5(11h,40h)  ;-Secret Lock Command
58h..5Fh Crash   -               Crashes the HC05 (jumps into a data area)
6Fh..FFh -       -               INT5(11h,40h)  ;-Unused/invalid
*/
    void operationTest();

    void operationGetstat();
    void operationGetID();
    void operationSetloc();
    void operationSeekL();
    void logMessage(std::string message) const;
public:
    CDROM(std::unique_ptr<InterruptController> &interruptController, bool logActivity);
    ~CDROM();

    void step();

    template <typename T>
    inline T load(uint32_t offset);
    template <typename T>
    inline void store(uint32_t offset, T value);

    void loadCDROMImageFile(std::string message);
};
