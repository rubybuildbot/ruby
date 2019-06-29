#include "DMA.hpp"
#include "Output.hpp"
#include "RAM.tcc"

using namespace std;

Port portWithIndex(uint32_t index) {
    if (index > Port::OTC) {
        printError("Attempting to get port with out-of-bounds index: %d", index);
    }
    return Port(index);
}

DMA::DMA(unique_ptr<RAM> &ram, unique_ptr<GPU> &gpu) : ram(ram), gpu(gpu) {
    for (int i = 0; i < 7; i++) {
        channels[i] = Channel();
    }
}

DMA::~DMA() {

}

uint32_t DMA::controlRegister() const {
    return control.value;
}

void DMA::setControlRegister(uint32_t value) {
    control.value = value;
}

bool DMA::interruptRequestStatus() const {
    uint8_t channelStatus = interrupt.IRQFlagsStatus().value & interrupt.IRQEnableStatus().value;
    return interrupt.forceIRQ || (interrupt.IRQEnableStatus().value && channelStatus != 0);
}

uint32_t DMA::interruptRegister() const {
    return interrupt.value;
}

void DMA::setInterruptRegister(uint32_t value) {
    value &= ~(1UL << 6);
    value &= ~(1UL << 7);
    value &= ~(1UL << 8);
    value &= ~(1UL << 9);
    value &= ~(1UL << 10);
    value &= ~(1UL << 11);
    value &= ~(1UL << 12);
    value &= ~(1UL << 13);
    value &= ~(1UL << 14);
    interrupt.value = value;
}

Channel& DMA::channelForPort(Port port) {
    return channels[port];
}

void DMA::execute(Port port) {
    Channel& channel = channels[port];
    if (channel.sync() == Sync::LinkedList) {
        executeLinkedList(port, channel);
    } else {
        executeBlock(port, channel);
    }
    return;
}

void DMA::executeLinkedList(Port port, Channel& channel) {
    uint32_t address = channel.baseAddressRegister() & 0x1ffffc;
    if (channel.direction() == Direction::ToRam) {
        printError("Unhandled DMA direction");
    }
    if (port != Port::GPUP) {
        printError("Unhandled DMA port");
    }
    while (true) {
        uint32_t header = ram->load<uint32_t>(address);
        uint32_t remainingTransferSize = header >> 24;
        while (remainingTransferSize > 0) {
            address = (address + 4) & 0x1ffffc;
            uint32_t command = ram->load<uint32_t>(address);
            gpu->executeGp0(command);
            remainingTransferSize -= 1;
        }
        if ((header & 0x800000) != 0) {
            break;
        }
        address = header & 0x1ffffc;
    }
    channel.done();
    return;
}

void DMA::executeBlock(Port port, Channel& channel) {
    int8_t step = 4;
    if (channel.step() == Step::Decrement) {
        step *= -1;
    }
    uint32_t address = channel.baseAddressRegister();
    optional<uint32_t> transferSize = channel.transferSize();
    if (!transferSize) {
        printError("Unknown DMA transfer size");
    }
    uint32_t remainingTransferSize = *transferSize;
    while (remainingTransferSize > 0) {
        uint32_t currentAddress = address & 0x1ffffc;
        switch (channel.direction()) {
            case Direction::FromRam: {
                uint32_t source = ram->load<uint32_t>(currentAddress);
                switch (port) {
                    case Port::GPUP: {
                        gpu->executeGp0(source);
                        break;
                    }
                    default: {
                        printError("Unhandled DMA source port");
                        break;
                    }
                }
                break;
            }
            case Direction::ToRam: {
                uint32_t source;
                switch (port) {
                    case Port::OTC: {
                        switch (remainingTransferSize) {
                            case 1: {
                                source = 0xffffff;
                                break;
                            }
                            default: {
                                source = (address - 4) & 0x1fffff;
                                break;
                            }
                        }
                        break;
                    }
                    default: {
                        printError("Unhandled DMA source port");
                        break;
                    }
                }
                ram->store<uint32_t>(currentAddress, source);
                break;
            }
        }
        address += step;
        remainingTransferSize -= 1;
    }
    channel.done();
    return;
}
