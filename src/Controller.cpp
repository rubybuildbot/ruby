#include "Controller.hpp"

Controller::Controller() : control(), joypadBaud(), mode() {

}

Controller::~Controller() {

}

void Controller::setControlRegister(uint16_t value) {
    control._value = value;
}

void Controller::setJoypadBaudRegister(uint16_t value) {
    joypadBaud = value;
}

void Controller::setModeRegister(uint16_t value) {
    mode._value = value;
}
