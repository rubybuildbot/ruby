#pragma once
#include <cstdint>

/*
1F80104Ah JOY_CTRL (R/W) (usually 1003h,3003h,0000h)
0     TX Enable (TXEN)  (0=Disable, 1=Enable)
1     /JOYn Output      (0=High, 1=Low/Select) (/JOYn as defined in Bit13)
2     RX Enable (RXEN)  (0=Normal, when /JOYn=Low, 1=Force Enable Once)
3     Unknown? (read/write-able) (for SIO, this would be TX Output Level)
4     Acknowledge       (0=No change, 1=Reset JOY_STAT.Bits 3,9)          (W)
5     Unknown? (read/write-able) (for SIO, this would be RTS Output Level)
6     Reset             (0=No change, 1=Reset most JOY_registers to zero) (W)
7     Not used             (always zero) (unlike SIO, no matter of FACTOR)
8-9   RX Interrupt Mode    (0..3 = IRQ when RX FIFO contains 1,2,4,8 bytes)
10    TX Interrupt Enable  (0=Disable, 1=Enable) ;when JOY_STAT.0-or-2 ;Ready
11    RX Interrupt Enable  (0=Disable, 1=Enable) ;when N bytes in RX FIFO
12    ACK Interrupt Enable (0=Disable, 1=Enable) ;when JOY_STAT.7  ;/ACK=LOW
13    Desired Slot Number  (0=/JOY1, 1=/JOY2) (set to LOW when Bit1=1)
14-15 Not used             (always zero)
*/
union JoypadControl {
    struct {
        uint16_t txEnable : 1;
        uint16_t joyOutput : 1;
        uint16_t rxEnable : 1;
        uint16_t unknown : 1;
        uint16_t acknowledge : 1;
        uint16_t unknown2 : 1;
        uint16_t reset : 1;
        uint16_t unused : 1;
        uint16_t rxInterruptMode : 2;
        uint16_t txInterruptEnable : 1;
        uint16_t rxInterruptEnable : 1;
        uint16_t ackInterruptEnable : 1;
        uint16_t desiredSlotNumber : 1;
        uint16_t unused2 : 2;
    };

    uint16_t _value;

    JoypadControl() : _value(0x0) {}
};

/*
1F801048h JOY_MODE (R/W) (usually 000Dh, ie. 8bit, no parity, MUL1)
0-1   Baudrate Reload Factor (1=MUL1, 2=MUL16, 3=MUL64) (or 0=MUL1, too)
2-3   Character Length       (0=5bits, 1=6bits, 2=7bits, 3=8bits)
4     Parity Enable          (0=No, 1=Enable)
5     Parity Type            (0=Even, 1=Odd) (seems to be vice-versa...?)
6-7   Unknown (always zero)
8     CLK Output Polarity    (0=Normal:High=Idle, 1=Inverse:Low=Idle)
9-15  Unknown (always zero)
*/
union JoypadMode {
    struct {
        uint16_t baudRateReloadFactor : 2;
        uint16_t characterLength : 2;
        uint16_t parityEnable : 1;
        uint16_t parityType : 1;
        uint16_t unknown : 2;
        uint16_t CLKOutputPolarity : 1;
        uint16_t unknown2 : 7;
    };

    uint16_t _value;

    JoypadMode() : _value(0x0) {}
};

class Controller {

public:
    Controller();
    ~Controller();

    JoypadControl control;
    uint16_t joypadBaud;
    JoypadMode mode;

    void setControlRegister(uint16_t value);
    void setJoypadBaudRegister(uint16_t value);
    void setModeRegister(uint16_t value);

    template <typename T>
    inline T load(uint32_t offset) const;
    template <typename T>
    inline void store(uint32_t offset, T value);
};
