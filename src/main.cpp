#include <SDL2/SDL.h>
#include <memory>
#include <cstdint>
#include "CPU.hpp"
#include "Debugger.hpp"
#include "TestRunner.hpp"
#include "Logger.hpp"

int main(int argc, char* argv[]) {
    TestRunner *testRunner = TestRunner::getInstance();
    testRunner->configure(argc, argv);
    std::unique_ptr<CPU> cpu = std::make_unique<CPU>();
    testRunner->setCPU(cpu.get());
    Debugger *debugger = Debugger::getInstance();
    debugger->setCPU(cpu.get());
    Logger *logger = Logger::getInstance();
    logger->configure(testRunner->shouldRunTests());
    logger->setupTraceFile();
    bool quit = false;
    while (!quit) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_BACKSPACE: {
                        debugger->debug();
                        break;
                    }
                }
            }
        }
        if (debugger->isAttached() && debugger->shouldStep()) {
            debugger->doStep();
            continue;
        }
        if (debugger->isAttached() && debugger->isStopped()) {
            continue;
        }
        for (int i = 0; i < 0xFFFF; i++) {
            testRunner->checkTTY();
            if (!cpu->executeNextInstruction()) {
                testRunner->setup();
            }
        }
    }
}
