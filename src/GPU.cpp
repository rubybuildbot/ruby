#include "GPU.hpp"
#include <iostream>
#include "Point.hpp"
#include "Color.hpp"

using namespace std;

uint8_t horizontalResolutionFromValues(uint8_t value1, uint8_t value2) {
    return ((value2 & 1) | (value1 & 3 << 1));
}

TexturePageColors texturePageColorsWithValue(uint32_t value) {
    if (value > TexturePageColors::T15Bit) {
        cout << "Attempting to create Texture Page Colors out-of-bounds value: " << dec << value << endl;
        exit(1);
    }
    return TexturePageColors(value);
}

GPU::GPU() : texturePageBaseX(0),
             texturePageBaseY(0),
             semiTransparency(0),
             texturePageColors(TexturePageColors::T4Bit),
             ditheringEnable(false),
             allowDrawToDisplayArea(false),
             shouldSetMaskBit(false),
             shouldPreserveMaskedPixels(false),
             interlaceField(Field::Top),
             textureDisable(false),
             horizontalResolution(horizontalResolutionFromValues(0, 0)),
             verticalResolution(VerticalResolution::Y240),
             videoMode(VideoMode::NTSC),
             displayAreaColorDepth(DisplayAreaColorDepth::D15Bits),
             verticalInterlaceEnable(false),
             displayDisable(true),
             interruptRequestEnable(false),
             dmaDirection(GPUDMADirection::Off),
             rectangleTextureFlipX(false),
             rectangleTextureFlipY(false),
             textureWindowMaskX(0),
             textureWindowMaskY(0),
             textureWindowOffsetX(0),
             textureWindowOffsetY(0),
             drawingAreaTop(0),
             drawingAreaLeft(0),
             drawingAreaBottom(0),
             drawingAreaRight(0),
             drawingOffsetX(0),
             drawingOffsetY(0),
             displayVRAMStartX(0),
             displayVRAMStartY(0),
             displayHorizontalStart(0),
             displayHorizontalEnd(0),
             displayLineStart(0),
             displayLineEnd(0),
             gp0InstructionBuffer(GPUInstructionBuffer()),
             gp0WordsRemaining(0),
             gp0InstructionMethod(nullptr),
             gp0Mode(GP0Mode::Command),
             renderer(Renderer())
{
}

GPU::~GPU() {
}

uint32_t GPU::statusRegister() const {
    uint32_t value = 0;
    value |= ((uint32_t)texturePageBaseX) << 0;
    value |= ((uint32_t)texturePageBaseY) << 4;
    value |= ((uint32_t)semiTransparency) << 5;
    value |= ((uint32_t)texturePageColors) << 7;
    value |= ((uint32_t)ditheringEnable) << 9;
    value |= ((uint32_t)allowDrawToDisplayArea) << 10;
    value |= ((uint32_t)shouldSetMaskBit) << 11;
    value |= ((uint32_t)shouldPreserveMaskedPixels) << 12;
    value |= ((uint32_t)interlaceField) << 13;
    // Nobody knows what bit 14 really does
    value |= ((uint32_t)textureDisable) << 15;
    value |= ((uint32_t)horizontalResolution) << 16;
    // TODO: remove this commented out line
    // value |= ((uint32_t)verticalResolution) << 19;
    value |= ((uint32_t)videoMode) << 20;
    value |= ((uint32_t)displayAreaColorDepth) << 21;
    value |= ((uint32_t)verticalInterlaceEnable) << 22;
    value |= ((uint32_t)displayDisable) << 23;
    value |= ((uint32_t)interruptRequestEnable) << 24;
    value |= ((uint32_t)1) << 26; // Ready to receive command
    value |= ((uint32_t)1) << 27; // Ready to send VRAM to CPU
    value |= ((uint32_t)1) << 28; // Ready to receive DMA
    value |= ((uint32_t)dmaDirection) << 29;
    value |= ((uint32_t)0) << 31; // current drawn line?
    uint32_t dmaRequest;
    switch (dmaDirection) {
        case GPUDMADirection::Off: {
            dmaRequest = 0;
            break;
        }
        case GPUDMADirection::Fifo: {
            dmaRequest = 1;
            break;
        }
        case GPUDMADirection::CPUToGp0: {
            dmaRequest = (value >> 28) & 1;
            break;
        }
        case GPUDMADirection::VRAMToCPU: {
            dmaRequest = (value >> 27) & 1;
            break;
        }
    }
    value |= (dmaRequest << 25);
    return value;
}

void GPU::executeGp0(uint32_t value) {
    if (gp0WordsRemaining == 0) {
        uint32_t opCode = (value >> 24) & 0xff;
        switch (opCode) {
            case 0x00: {
                gp0WordsRemaining = 1;
                gp0InstructionMethod = [&]() {
                    this->operationGp0Nop();
                };
                break;
            }
            case 0x01: {
                gp0WordsRemaining = 1;
                gp0InstructionMethod = [&]() {
                    this->operationGp0ClearCache();
                };
                break;
            }
            case 0x28: {
                gp0WordsRemaining = 5;
                gp0InstructionMethod = [&]() {
                    this->operationGp0MonochromeQuadOpaque();
                };
                break;
            }
            case 0x2c: {
                gp0WordsRemaining = 9;
                gp0InstructionMethod = [&]() {
                    this->operationGp0TexturedQuadOpaqueTextureBlending();
                };
                break;
            }
            case 0x30: {
                gp0WordsRemaining = 6;
                gp0InstructionMethod = [&]() {
                    this->operationGp0ShadedTriangleOpaque();
                };
                break;
            }
            case 0x38: {
                gp0WordsRemaining = 8;
                gp0InstructionMethod = [&]() {
                    this->operationGp0ShadedQuadOpaque();
                };
                break;
            }
            case 0xa0: {
                gp0WordsRemaining = 3;
                gp0InstructionMethod = [&]() {
                    this->operationGp0CopyRectangleCPUToVRAM();
                };
                break;
            }
            case 0xc0: {
                gp0WordsRemaining = 3;
                gp0InstructionMethod = [&]() {
                    this->operationGp0CopyRectangleVRAMToCPU();
                };
                break;
            }
            case 0xe1: {
                gp0WordsRemaining = 1;
                gp0InstructionMethod = [&]() {
                    this->operationGp0DrawMode();
                };
                break;
            }
            case 0xe2: {
                gp0WordsRemaining = 1;
                gp0InstructionMethod = [&]() {
                    this->operationGp0TextureWindowSetting();
                };
                break;
            }
            case 0xe3: {
                gp0WordsRemaining = 1;
                gp0InstructionMethod = [&]() {
                    this->operationGp0SetDrawingAreaTopLeft();
                };
                break;
            }
            case 0xe4: {
                gp0WordsRemaining = 1;
                gp0InstructionMethod = [&]() {
                    this->operationGp0SetDrawingAreaBottomRight();
                };
                break;
            }
            case 0xe5: {
                gp0WordsRemaining = 1;
                gp0InstructionMethod = [&]() {
                    this->operationGp0SetDrawingOffset();
                };
                break;
            }
            case 0xe6: {
                gp0WordsRemaining = 1;
                gp0InstructionMethod = [&]() {
                    this->operationGp0MaskBitSetting();
                };
                break;
            }
            default: {
                cout << "Unhandled gp0 instruction 0x" << hex << opCode << endl;
                exit(1);
            }
        }
        gp0InstructionBuffer.clear();
    }
    gp0WordsRemaining -= 1;

    if (gp0Mode == GP0Mode::Command) {
        gp0InstructionBuffer.pushWord(value);
        if (gp0WordsRemaining == 0) {
            gp0InstructionMethod();
        }
    } else if (gp0Mode == GP0Mode::ImageLoad) {
        // TODO: copy pixel data to VRAM
        if (gp0WordsRemaining == 0) {
            gp0Mode = GP0Mode::Command;
        }
    }
}

void GPU::executeGp1(uint32_t value) {
    uint32_t opCode = (value >> 24) & 0xff;
    switch (opCode) {
        case 0x00: {
            operationGp1Reset(value);
            break;
        }
        case 0x01: {
            operationGp1ResetCommandBuffer(value);
            break;
        }
        case 0x02: {
            operationGp1AcknowledgeGPUInterrupt(value);
            break;
        }
        case 0x03: {
            operationGp1DisplayEnable(value);
            break;
        }
        case 0x04: {
            operationGp1DMADirection(value);
            break;
        }
        case 0x05: {
            operationGp1StartOfDisplayArea(value);
            break;
        }
        case 0x06: {
            operationGp1HorizontalDisplayRange(value);
            break;
        }
        case 0x07: {
            operationGp1VerticalDisplayRange(value);
            break;
        }
        case 0x08: {
            operationGp1DisplayMode(value);
            break;
        }
        default: {
            cout << "Unhandled gp1 instruction 0x" << hex << opCode << endl;
            exit(1);
        }
    }
}

void GPU::operationGp0Nop() {
    return;
}

/*
GP0(E1h) - Draw Mode setting (aka "Texpage")
0-3   Texture page X Base   (N*64) (ie. in 64-halfword steps)    ;GPUSTAT.0-3
4     Texture page Y Base   (N*256) (ie. 0 or 256)               ;GPUSTAT.4
5-6   Semi Transparency     (0=B/2+F/2, 1=B+F, 2=B-F, 3=B+F/4)   ;GPUSTAT.5-6
7-8   Texture page colors   (0=4bit, 1=8bit, 2=15bit, 3=Reserved);GPUSTAT.7-8
9     Dither 24bit to 15bit (0=Off/strip LSBs, 1=Dither Enabled) ;GPUSTAT.9
10    Drawing to display area (0=Prohibited, 1=Allowed)          ;GPUSTAT.10
11    Texture Disable (0=Normal, 1=Disable if GP1(09h).Bit0=1)   ;GPUSTAT.15
        (Above might be chipselect for (absent) second VRAM chip?)
12    Textured Rectangle X-Flip   (BIOS does set this bit on power-up...?)
13    Textured Rectangle Y-Flip   (BIOS does set it equal to GPUSTAT.13...?)
14-23 Not used (should be 0)
24-31 Command  (E1h)
*/
void GPU::operationGp0DrawMode() {
    uint32_t value = gp0InstructionBuffer[0];
    texturePageBaseX = value & 0xf;
    texturePageBaseY = (value >> 4) & 1;
    semiTransparency = (value >> 5) & 3;
    texturePageColors = texturePageColorsWithValue((value >> 7) & 3);
    ditheringEnable = ((value >> 9) & 1) != 0;
    allowDrawToDisplayArea = ((value >> 10) & 1) != 0;
    textureDisable = ((value >> 11) & 1) != 0;
    rectangleTextureFlipX = ((value >> 12) & 1) != 0;
    rectangleTextureFlipY = ((value >> 13) & 1) != 0;
}

/*
GP1(00h) - Reset GPU
0-23  Not used (zero)
Resets the GPU to the following values:
GP1(01h)      ;clear fifo
GP1(02h)      ;ack irq (0)
GP1(03h)      ;display off (1)
GP1(04h)      ;dma off (0)
GP1(05h)      ;display address (0)
GP1(06h)      ;display x1,x2 (x1=200h, x2=200h+256*10)
GP1(07h)      ;display y1,y2 (y1=010h, y2=010h+240)
GP1(08h)      ;display mode 320x200 NTSC (0)
GP0(E1h..E6h) ;rendering attributes (0)
*/
void GPU::operationGp1Reset(uint32_t value) {
    interruptRequestEnable = false;

    texturePageBaseX = 0;
    texturePageBaseY = 0;
    semiTransparency = 0;
    texturePageColors = TexturePageColors::T4Bit;
    textureWindowMaskX = 0;
    textureWindowMaskY = 0;
    textureWindowOffsetX = 0;
    textureWindowOffsetY = 0;
    ditheringEnable = false;
    allowDrawToDisplayArea = false;
    textureDisable = false;
    rectangleTextureFlipX = false;
    rectangleTextureFlipY = false;
    drawingAreaLeft = 0;
    drawingAreaTop = 0;
    drawingAreaRight = 0;
    drawingAreaBottom = 0;
    drawingOffsetX = 0;
    drawingOffsetY = 0;
    shouldSetMaskBit = false;
    shouldPreserveMaskedPixels = false;

    dmaDirection = GPUDMADirection::Off;

    displayDisable = true;
    displayVRAMStartX = 0;
    displayVRAMStartY = 0;
    horizontalResolution = horizontalResolutionFromValues(0, 0);
    verticalResolution = VerticalResolution::Y240;

    videoMode = VideoMode::NTSC;
    verticalInterlaceEnable = false;
    displayHorizontalStart = 0x200;
    displayHorizontalEnd = 0xc00;
    displayLineStart = 0x10;
    displayLineEnd = 0x100;
    displayAreaColorDepth = DisplayAreaColorDepth::D15Bits;

    // TODO: clear the command FIFO
    // TODO: invalidate GPU cache
}

uint32_t GPU::readRegister() const {
    return 0;
}

/*
GP1(08h) - Display mode
0-1   Horizontal Resolution 1     (0=256, 1=320, 2=512, 3=640) ;GPUSTAT.17-18
2     Vertical Resolution         (0=240, 1=480, when Bit5=1)  ;GPUSTAT.19
3     Video Mode                  (0=NTSC/60Hz, 1=PAL/50Hz)    ;GPUSTAT.20
4     Display Area Color Depth    (0=15bit, 1=24bit)           ;GPUSTAT.21
5     Vertical Interlace          (0=Off, 1=On)                ;GPUSTAT.22
6     Horizontal Resolution 2     (0=256/320/512/640, 1=368)   ;GPUSTAT.16
7     "Reverseflag"               (0=Normal, 1=Distorted)      ;GPUSTAT.14
8-23  Not used (zero)
*/
void GPU::operationGp1DisplayMode(uint32_t value) {
    uint8_t horizontalResolutionValue1 = (value & 3);
    uint8_t horizontalResolutionValue2 = (value >> 6) & 1;
    horizontalResolution = horizontalResolutionFromValues(horizontalResolutionValue1, horizontalResolutionValue2);
    verticalResolution = VerticalResolution((value >> 2) & 0x1);
    videoMode = VideoMode((value >> 3) & 0x1);
    displayAreaColorDepth = DisplayAreaColorDepth((value >> 4) & 0x1);
    verticalInterlaceEnable = (value >> 5) & 0x1;
    if ((value & 0x80) != 0) {
        // This is supposed to be bit 14 on GPUSTAT
        cout << "Unsupported display mode: distorted" << endl;
        exit(1);
    }
}

/*
GP1(04h) - DMA Direction / Data Request
0-1  DMA Direction (0=Off, 1=FIFO, 2=CPUtoGP0, 3=GPUREADtoCPU) ;GPUSTAT.29-30
2-23 Not used (zero)
*/
void GPU::operationGp1DMADirection(uint32_t value) {
    dmaDirection = GPUDMADirection(value & 3);
}

/*
GP0(E3h) - Set Drawing Area top left (X1,Y1)
GP0(E4h) - Set Drawing Area bottom right (X2,Y2)
0-9    X-coordinate (0..1023)
10-18  Y-coordinate (0..511)   ;\on Old 160pin GPU (max 1MB VRAM)
19-23  Not used (zero)         ;/
10-19  Y-coordinate (0..1023)  ;\on New 208pin GPU (max 2MB VRAM)
20-23  Not used (zero)         ;/(retail consoles have only 1MB though)
24-31  Command  (Exh)
*/
void GPU::operationGp0SetDrawingAreaTopLeft() {
    uint32_t value = gp0InstructionBuffer[0];
    drawingAreaTop = ((value >> 10) & 0x3ff);
    drawingAreaLeft = (value & 0x3ff);
}

void GPU::operationGp0SetDrawingAreaBottomRight() {
    uint32_t value = gp0InstructionBuffer[0];
    drawingAreaBottom = ((value >> 10) & 0x3ff);
    drawingAreaRight = (value & 0x3ff);
}

/*
GP0(E5h) - Set Drawing Offset (X,Y)
0-10   X-offset (-1024..+1023) (usually within X1,X2 of Drawing Area)
11-21  Y-offset (-1024..+1023) (usually within Y1,Y2 of Drawing Area)
22-23  Not used (zero)
24-31  Command  (E5h)
*/
void GPU::operationGp0SetDrawingOffset() {
    uint32_t value = gp0InstructionBuffer[0];
    uint16_t x = (value & 0x7ff);
    uint16_t y = ((value >> 11) & 0x7ff);

    drawingOffsetX = ((int16_t)(x << 5)) >> 5;
    drawingOffsetY = ((int16_t)(y << 5)) >> 5;

    // TODO: Remove. Temporary workaround to update the window.
    renderer.display();
}

/*
GP0(E2h) - Texture Window setting
0-4    Texture window Mask X   (in 8 pixel steps)
5-9    Texture window Mask Y   (in 8 pixel steps)
10-14  Texture window Offset X (in 8 pixel steps)
15-19  Texture window Offset Y (in 8 pixel steps)
20-23  Not used (zero)
24-31  Command  (E2h)
*/
void GPU::operationGp0TextureWindowSetting() {
    uint32_t value = gp0InstructionBuffer[0];
    textureWindowMaskX = (value & 0x1f);
    textureWindowMaskY = ((value >> 5) & 0x1f);
    textureWindowOffsetX = ((value >> 10) & 0x1f);
    textureWindowOffsetY = ((value >> 15) & 0x1f);
}

/*
GP0(E6h) - Mask Bit Setting
0     Set mask while drawing (0=TextureBit15, 1=ForceBit15=1)   ;GPUSTAT.11
1     Check mask before draw (0=Draw Always, 1=Draw if Bit15=0) ;GPUSTAT.12
2-23  Not used (zero)
24-31 Command  (E6h)
*/
void GPU::operationGp0MaskBitSetting() {
    uint32_t value = gp0InstructionBuffer[0];
    shouldSetMaskBit = (value & 1) != 0;
    shouldPreserveMaskedPixels = (value & 2) != 0;
}

/*
GP1(05h) - Start of Display area (in VRAM)
0-9   X (0-1023)    (halfword address in VRAM)  (relative to begin of VRAM)
10-18 Y (0-511)     (scanline number in VRAM)   (relative to begin of VRAM)
19-23 Not used (zero)
*/
void GPU::operationGp1StartOfDisplayArea(uint32_t value) {
    displayVRAMStartX = (value & 0x3fe);
    displayVRAMStartY = ((value >> 10) & 0x1ff);
}

/*
GP1(06h) - Horizontal Display range (on Screen)
0-11   X1 (260h+0)       ;12bit       ;\counted in 53.222400MHz units,
12-23  X2 (260h+320*8)   ;12bit       ;/relative to HSYNC
*/
void GPU::operationGp1HorizontalDisplayRange(uint32_t value) {
    displayHorizontalStart = (value & 0xfff);
    displayHorizontalEnd = ((value >> 12) & 0xfff);
}

/*
GP1(07h) - Vertical Display range (on Screen)
0-9   Y1 (NTSC=88h-(224/2), (PAL=A3h-(264/2))  ;\scanline numbers on screen,
10-19 Y2 (NTSC=88h+(224/2), (PAL=A3h+(264/2))  ;/relative to VSYNC
20-23 Not used (zero)
*/
void GPU::operationGp1VerticalDisplayRange(uint32_t value) {
    displayLineStart = (value & 0x3ff);
    displayLineEnd = ((value >> 10) & 0x3ff);
}

void GPU::operationGp0MonochromeQuadOpaque() {
    // TODO:
    return;
}

void GPU::operationGp0ClearCache() {
    return;
}

/*
GP0(A0h) - Copy Rectangle (CPU to VRAM)
1st  Command           (Cc000000h)
2nd  Destination Coord (YyyyXxxxh)  ;Xpos counted in halfwords
3rd  Width+Height      (YsizXsizh)  ;Xsiz counted in halfwords
...  Data              (...)      <--- usually transferred via DMA
*/
void GPU::operationGp0CopyRectangleCPUToVRAM() {
    uint32_t resolution = gp0InstructionBuffer[2];
    uint32_t width = resolution & 0xffff;
    uint32_t height = resolution >> 16;
    uint32_t imageSize = width * height;
    // Pixels are 16 bit wide and transactions are 32 bit wide
    // If the resolution is odd, add a unit to get the right
    // number of transactions
    if (imageSize % 2 != 0) {
        imageSize++;
    }
    gp0WordsRemaining = imageSize / 2;
    gp0Mode = GP0Mode::ImageLoad;
    return;
}

/*
GP1(03h) - Display Enable
0     Display On/Off   (0=On, 1=Off)                         ;GPUSTAT.23
1-23  Not used (zero)
*/
void GPU::operationGp1DisplayEnable(uint32_t value) {
    displayDisable = (value & 1) != 0;
}

/*
GP0(C0h) - Copy Rectangle (VRAM to CPU)
1st  Command           (Cc000000h) ;\
2nd  Source Coord      (YyyyXxxxh) ; write to GP0 port (as usually)
3rd  Width+Height      (YsizXsizh) ;/
...  Data              (...)       ;<--- read from GPUREAD port (or via DMA)
*/
void GPU::operationGp0CopyRectangleVRAMToCPU() {
    uint32_t resolution = gp0InstructionBuffer[2];
    uint32_t width = resolution & 0xffff;
    uint32_t height = resolution >> 16;

    cout << "Unhandled GP0 Copy Rectangle VRAM to CPU with with: " << dec << width << "x" << dec << height << endl;
}

void GPU::operationGp0ShadedQuadOpaque() {
    // TODO:
    cout << "Draw Shaded Quad Opaque" << endl;
    return;
}

void GPU::operationGp0ShadedTriangleOpaque() {
    Point points[3] = { Point(gp0InstructionBuffer[1]), Point(gp0InstructionBuffer[3]), Point(gp0InstructionBuffer[5])};
    Color colors[3] = { Color(gp0InstructionBuffer[0]), Color(gp0InstructionBuffer[2]), Color(gp0InstructionBuffer[4])};

    renderer.pushTriangle(points, colors);
    return;
}

void GPU::operationGp0TexturedQuadOpaqueTextureBlending() {
    // TODO:
    cout << "Draw Textured Quad Opaque with Texture Blending" << endl;
    return;
}

void GPU::operationGp1AcknowledgeGPUInterrupt(uint32_t value) {
    interruptRequestEnable = false;
    return;
}

void GPU::operationGp1ResetCommandBuffer(uint32_t value) {
    gp0InstructionBuffer.clear();
    gp0WordsRemaining = 0;
    gp0Mode = GP0Mode::Command;
    // TODO: clear the command FIFO
}
