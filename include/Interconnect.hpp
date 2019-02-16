#pragma once
#include "BIOS.hpp"
#include "RAM.hpp"

/*
Memory Map
KUSEG     KSEG0     KSEG1
00000000h 80000000h A0000000h  2048K  Main RAM (first 64K reserved for BIOS)
1F000000h 9F000000h BF000000h  8192K  Expansion Region 1 (ROM/RAM)
1F800000h 9F800000h    --      1K     Scratchpad (D-Cache used as Fast RAM)
1F801000h 9F801000h BF801000h  8K     I/O Ports
1F802000h 9F802000h BF802000h  8K     Expansion Region 2 (I/O Ports)
1FA00000h 9FA00000h BFA00000h  2048K  Expansion Region 3 (whatever purpose)
1FC00000h 9FC00000h BFC00000h  512K   BIOS ROM (Kernel) (4096K max)
    FFFE0000h (KSEG2)        0.5K   I/O Ports (Cache Control)

See IOMap.md for I/O register mapping
*/
class Interconnect {
    const BIOS &bios;
    RAM &ram;
public:
    Interconnect(BIOS &bios, RAM &ram);
    ~Interconnect();

    uint32_t readWord(uint32_t address) const;
    void storeWord(uint32_t address, uint32_t value) const;
};
