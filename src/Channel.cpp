#include "Channel.hpp"

using namespace std;

Channel::Channel() : logger(NoLog) {}

Channel::Channel(LogLevel logLevel, DMAPort port) : control(), baseAddress(), blockControl(), logger(logLevel, "  DMA: "), port(port) {

}

Channel::~Channel() {

}

uint32_t Channel::controlRegister() const {
    logger.logMessage("D%d_CHCR [R]: %#x", port, control.value);
    return control.value;
}

void Channel::setControlRegister(uint32_t value) {
    logger.logMessage("D%d_CHCR [W]: %#x", port, value);
    value &= ~(1UL << 2);
    value &= ~(1UL << 3);
    value &= ~(1UL << 4);
    value &= ~(1UL << 5);
    value &= ~(1UL << 6);
    value &= ~(1UL << 7);
    value &= ~(1UL << 11);
    value &= ~(1UL << 12);
    value &= ~(1UL << 13);
    value &= ~(1UL << 14);
    value &= ~(1UL << 15);
    value &= ~(1UL << 19);
    value &= ~(1UL << 23);
    value &= ~(1UL << 25);
    value &= ~(1UL << 26);
    value &= ~(1UL << 27);
    value &= ~(1UL << 31);
    control.value = value;
}

uint32_t Channel::baseAddressRegister() const {
    logger.logMessage("D%d_MADR [R]: %#x", port, baseAddress.value);
    return baseAddress.value;
}

void Channel::setBaseAddressRegister(uint32_t value) {
    logger.logMessage("D%d_MADR [W]: %#x", port, value);
    value &= ~(1UL << 24);
    value &= ~(1UL << 25);
    value &= ~(1UL << 26);
    value &= ~(1UL << 27);
    value &= ~(1UL << 28);
    value &= ~(1UL << 29);
    value &= ~(1UL << 30);
    value &= ~(1UL << 31);
    baseAddress.value = value;
}

uint32_t Channel::blockControlRegister() const {
    logger.logMessage("D%d_BCR [R]: %#x", port, blockControl.value);
    return blockControl.value;
}

void Channel::setBlockControlRegister(uint32_t value) {
    logger.logMessage("D%d_BCR [W]: %#x", port, value);
    blockControl.value = value;
}

bool Channel::isActive() const {
    if (control.sync() == Sync::Manual) {
        return control.enable && control.trigger;
    }
    return control.enable;
}

Sync Channel::sync() const {
    return control.sync();
}

Direction Channel::direction() const {
    return control.direction();
}

Step Channel::step() const {
    return control.step();
}

optional<uint32_t> Channel::transferSize() const {
    switch (control.sync()) {
        case Sync::LinkedList: {
            return nullopt;
        }
        case Sync::Manual: {
            return { blockControl.blockSize };
        }
        case Sync::Request: {
            return { blockControl.blockSize * blockControl.blockCount };
        }
        default: {
            logger.logError("Unknown DMA sync mode");
            return {0};
        }
    }
}

void Channel::done() {
    control.enable = false;
    control.trigger = false;
}
