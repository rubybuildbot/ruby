#include "Interconnect.hpp"
#include "Range.hpp"

using namespace std;

const uint32_t regionMask[8] = {
    // KUSEG: 2048MB
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    // KSEG0: 512MB
    0x7fffffff,
    // KSEG1: 512MB
    0x1fffffff,
    // KSEG2: 1024MB
    0xffffffff, 0xffffffff,
};

Interconnect::Interconnect() {
    bios = make_unique<BIOS>();
    bios->loadBin("SCPH1001.BIN");
    ram = make_unique<RAM>();
    gpu = make_unique<GPU>();
    dma = make_unique<DMA>(ram, gpu);
    scratchpad = make_unique<Scratchpad>();
    cdrom = make_unique<CDROM>();
    interruptController = make_unique<InterruptController>();
}

Interconnect::~Interconnect() {
}

uint32_t Interconnect::maskRegion(uint32_t address) const {
    uint8_t index = address >> 29;
    return address & regionMask[index];
}

void Interconnect::transferToRAM(string path, uint32_t origin, uint32_t size, uint32_t destination) {
    uint32_t maskedDestination = maskRegion(destination);
    ram->receiveTransfer(path, origin, size, maskedDestination);
}

void Interconnect::dumpRAM() {
    ram->dump();
}

unique_ptr<InterruptController>& Interconnect::interruptControllerRef() {
    return interruptController;
}
