#pragma once
#include "CDROM.hpp"
#include "Output.hpp"

template <typename T>
inline T CDROM::load(uint32_t offset) const {
    static_assert(std::is_same<T, uint8_t>() || std::is_same<T, uint16_t>() || std::is_same<T, uint32_t>(), "Invalid type");
    if (sizeof(T) != 1) {
        printError("Unsupported CDROM read with size: %d", sizeof(T));
    }
    switch (offset) {
        case 0: {
            printWarning("Unhandled CDROM Status Register");
            return 0;
        }
        default: {
            printError("Unhandled CDROM read at offset: %#x", offset);
        }
    }
    return 0;
}

template <typename T>
inline void CDROM::store(uint32_t offset, T value) {
    static_assert(std::is_same<T, uint8_t>() || std::is_same<T, uint16_t>() || std::is_same<T, uint32_t>(), "Invalid type");
    if (sizeof(T) != 1) {
        printError("Unsupported CDROM write with size: %d", sizeof(T));
    }
    uint8_t param = value & 0xFF;
    switch (offset) {
        case 0: {
            setIndex(param);
            break;
        }
        case 2: {
            switch (index) {
                case 1: {
                    printWarning("Unhandled CDROM Interrupt Enable Register");
                    break;
                }
                default: {
                    printError("Unhandled CDROM write at offset: %#x, with index: %d", offset, index);
                    break;
                }
            }
            break;
        }
        case 3: {
            switch (index) {
                case 1: {
                    printWarning("Unhandled CDROM Interrupt Flag Register");
                    break;
                }
                default: {
                    printError("Unhandled CDROM write at offset: %#x, with index: %d", offset, index);
                    break;
                }
            }
            break;
        }
        default: {
            printError("Unhandled CDROM write at offset: %#x, with index: %d", offset, index);
            break;
        }
    }
    return;
}

