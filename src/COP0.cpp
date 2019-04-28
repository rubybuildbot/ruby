#include "COP0.hpp"

COP0::COP0() : statusRegister(), causeRegister(), returnAddressFromTrap() {

}

COP0::~COP0() {

}

bool COP0::isCacheIsolated() {
    return (statusRegister & 0x10000) != 0;
}

uint32_t COP0::getStatusRegister() {
    return statusRegister;
}

void COP0::setStatusRegister(uint32_t value) {
    statusRegister = value;
}

uint32_t COP0::getReturnAddressFromTrap() {
    return returnAddressFromTrap;
}

void COP0::setReturnAddressFromTrap(uint32_t value) {
    returnAddressFromTrap = value;
}

uint32_t COP0::getCauseRegister() {
    return causeRegister;
}

void COP0::setCauseRegister(uint32_t value) {
    causeRegister = value;
}

uint32_t COP0::updateRegistersWithException(ExceptionType exceptionType, uint32_t programCounter, bool isDelaySlot) {
    // Grab the last 6 bits of SR
    // Shift the result two spaces to the left, padded with zeroes
    // This is the Interrupt Enable/User Mode mask
    uint32_t mode = statusRegister & 0x3f;

    statusRegister &= ~0x3f;
    statusRegister |= ((mode << 2) & 0x3f);

    causeRegister &= ~0x7c;
    causeRegister |= exceptionType << 2;

    if (isDelaySlot) {
        returnAddressFromTrap = returnAddressFromTrap - 4;
        causeRegister |= (1 << 31);
    } else {
        returnAddressFromTrap = programCounter;
        causeRegister &= ~(1 << 31);
    }

    if ((statusRegister & (1 << 2)) != 0) {
        return 0xbfc00180;
    } else {
        return 0x80000080;
    }
}

void COP0::updateRegistersWithReturnFromException() {
    uint32_t mode = statusRegister & 0x3f;
    statusRegister &= ~0xf;
    statusRegister |= mode >> 2;
}
