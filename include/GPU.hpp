#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include "GPUInstructionBuffer.hpp"
#include "Renderer.hpp"
#include "GPUImageBuffer.hpp"
#include "Window.hpp"
#include "Logger.hpp"

enum TexturePageColors {
    T4Bit = 0,
    T8Bit = 1,
    T15Bit = 2,
};

enum Field {
    Bottom = 0,
    Top = 1
};

enum VerticalResolution {
    Y240 = 0,
    Y480 = 1
};

enum VideoMode {
    NTSC = 0,
    PAL = 1
};

enum DisplayAreaColorDepth {
    D15Bits = 0,
    D24Bits = 1
};

enum GPUDMADirection {
    Off = 0,
    Fifo = 1,
    CPUToGp0 = 2,
    VRAMToCPU = 3
};

enum GP0Mode {
    Command = 0,
    ImageLoad = 1
};

/*
1F801814h - GPUSTAT - GPU Status Register (R)
0-3   Texture page X Base   (N*64)                              ;GP0(E1h).0-3
4     Texture page Y Base   (N*256) (ie. 0 or 256)              ;GP0(E1h).4
5-6   Semi Transparency     (0=B/2+F/2, 1=B+F, 2=B-F, 3=B+F/4)  ;GP0(E1h).5-6
7-8   Texture page colors   (0=4bit, 1=8bit, 2=15bit, 3=Reserved)GP0(E1h).7-8
9     Dither 24bit to 15bit (0=Off/strip LSBs, 1=Dither Enabled);GP0(E1h).9
10    Drawing to display area (0=Prohibited, 1=Allowed)         ;GP0(E1h).10
11    Set Mask-bit when drawing pixels (0=No, 1=Yes/Mask)       ;GP0(E6h).0
12    Draw Pixels           (0=Always, 1=Not to Masked areas)   ;GP0(E6h).1
13    Interlace Field       (or, always 1 when GP1(08h).5=0)
14    "Reverseflag"         (0=Normal, 1=Distorted)             ;GP1(08h).7
15    Texture Disable       (0=Normal, 1=Disable Textures)      ;GP0(E1h).11
16    Horizontal Resolution 2     (0=256/320/512/640, 1=368)    ;GP1(08h).6
17-18 Horizontal Resolution 1     (0=256, 1=320, 2=512, 3=640)  ;GP1(08h).0-1
19    Vertical Resolution         (0=240, 1=480, when Bit22=1)  ;GP1(08h).2
20    Video Mode                  (0=NTSC/60Hz, 1=PAL/50Hz)     ;GP1(08h).3
21    Display Area Color Depth    (0=15bit, 1=24bit)            ;GP1(08h).4
22    Vertical Interlace          (0=Off, 1=On)                 ;GP1(08h).5
23    Display Enable              (0=Enabled, 1=Disabled)       ;GP1(03h).0
24    Interrupt Request (IRQ1)    (0=Off, 1=IRQ)       ;GP0(1Fh)/GP1(02h)
25    DMA / Data Request, meaning depends on GP1(04h) DMA Direction:
        When GP1(04h)=0 ---> Always zero (0)
        When GP1(04h)=1 ---> FIFO State  (0=Full, 1=Not Full)
        When GP1(04h)=2 ---> Same as GPUSTAT.28
        When GP1(04h)=3 ---> Same as GPUSTAT.27
26    Ready to receive Cmd Word   (0=No, 1=Ready)  ;GP0(...) ;via GP0
27    Ready to send VRAM to CPU   (0=No, 1=Ready)  ;GP0(C0h) ;via GPUREAD
28    Ready to receive DMA Block  (0=No, 1=Ready)  ;GP0(...) ;via GP0
29-30 DMA Direction (0=Off, 1=?, 2=CPUtoGP0, 3=GPUREADtoCPU)    ;GP1(04h).0-1
31    Drawing even/odd lines in interlace mode (0=Even or Vblank, 1=Odd)
*/
class GPU {
    Logger logger;
    uint8_t texturePageBaseX;
    uint8_t texturePageBaseY;
    uint8_t semiTransparency;
    TexturePageColors texturePageColors;
    bool ditheringEnable;
    bool allowDrawToDisplayArea;
    bool shouldSetMaskBit;
    bool shouldPreserveMaskedPixels;
    Field interlaceField;
    bool textureDisable;
    uint8_t horizontalResolution;
    VerticalResolution verticalResolution;
    VideoMode videoMode;
    DisplayAreaColorDepth displayAreaColorDepth;
    bool verticalInterlaceEnable;
    bool displayDisable;
    bool interruptRequestEnable;
    GPUDMADirection dmaDirection;
/*
GP0(E1h) - Draw Mode setting (aka "Texpage")
*/
    bool rectangleTextureFlipX;
    bool rectangleTextureFlipY;
/*
GP0(E2h) - Texture Window setting
*/
    uint8_t textureWindowMaskX;
    uint8_t textureWindowMaskY;
    uint8_t textureWindowOffsetX;
    uint8_t textureWindowOffsetY;
/*
GP0(E3h) - Set Drawing Area top left (X1,Y1)
*/
    uint16_t drawingAreaTop;
    uint16_t drawingAreaLeft;
/*
GP0(E4h) - Set Drawing Area bottom right (X2,Y2)
*/
    uint16_t drawingAreaBottom;
    uint16_t drawingAreaRight;
/*
GP0(E5h) - Set Drawing Offset (X,Y)
*/
    int16_t drawingOffsetX;
    int16_t drawingOffsetY;
/*
GP1(05h) - Start of Display area (in VRAM)
*/
    uint16_t displayVRAMStartX;
    uint16_t displayVRAMStartY;
/*
GP1(06h) - Horizontal Display range (on Screen)
*/
    uint16_t displayHorizontalStart;
    uint16_t displayHorizontalEnd;
/*
GP1(07h) - Vertical Display range (on Screen)
*/
    uint16_t displayLineStart;
    uint16_t displayLineEnd;

    GPUInstructionBuffer gp0InstructionBuffer;
    int32_t gp0WordsRemaining;
    uint32_t gp0WordsRead;
    std::function<void(void)> gp0InstructionMethod;

    uint32_t gpuRead;

    GP0Mode gp0Mode;

    std::unique_ptr<Renderer> renderer;

    std::unique_ptr<GPUImageBuffer> imageBuffer;

    void operationGp0Nop();
    void operationGp0DrawMode();
    void operationGp0SetDrawingAreaTopLeft();
    void operationGp0SetDrawingAreaBottomRight();
    void operationGp0SetDrawingOffset();
    void operationGp0TextureWindowSetting();
    void operationGp0MaskBitSetting();
    void operationGp0ClearCache();
    void operationGp0CopyRectangleCPUToVRAM();
    void operationGp0CopyRectangleVRAMToCPU();

    void operationGp0MonochromeThreePointOpaque();
    void operationGp0MonochromeThreePointSemiTransparent();
    void operationGp0MonochromeFourPointOpaque();
    void operationGp0MonochromeFourPointSemiTransparent();

    void operationGp0TexturedThreePointOpaqueTextureBlending();
    void operationGp0TexturedThreePointOpaqueRawTexture();
    void operationGp0TexturedThreePointSemiTransparentTextureBlending();
    void operationGp0TexturedThreePointSemiTransparentRawTexture();
    void operationGp0TexturedFourPointOpaqueTextureBlending();
    void operationGp0TexturedFourPointOpaqueRawTexture();
    void operationGp0TexturedFourPointSemiTransparentTextureBlending();
    void operationGp0TexturedFourPointSemiTransparentRawTexture();

    void operationGp0ShadedThreePointOpaque();
    void operationGp0ShadedThreePointSemiTransparent();
    void operationGp0ShadedFourPointOpaque();
    void operationGp0ShadedFourPointSemiTransparent();

    void operationGp0TexturedShadedThreePointOpaqueTextureBlending();
    void operationGp0TexturedShadedThreePointSemiTransparentTextureBlending();
    void operationGp0TexturedShadedFourPointOpaqueTextureBlending();
    void operationGp0TexturedShadedFourPointSemiTransparentTextureBlending();

    void operationGp0FillRectagleInVRAM();

    void operationGp0MonochromeLineOpaque();
    void operationGp0MonochromeLineSemiTransparent();
    void operationGp0MonochromePolylineOpaque();
    void operationGp0MonochromePolylineSemiTransparent();

    void operationGp0ShadedLineOpaque();
    void operationGp0ShadedLineSemiTransparent();
    void operationGp0ShadedPolylineOpaque();
    void operationGp0ShadedPolylineSemiTransparent();

    void operationGp0MonochromeQuadOpaque();
    void operationGp0MonochromeQuadSemiTransparent();
    void operationGp0MonochromeQuad1x1Opaque();
    void operationGp0MonochromeQuad1x1SemiTransparent();
    void operationGp0MonochromeQuad8x8Opaque();
    void operationGp0MonochromeQuad8x8SemiTransparent();
    void operationGp0MonochromeQuad16x16Opaque();
    void operationGp0MonochromeQuad16x16SemiTransparent();

    void operationGp0TexturedQuadOpaqueTextureBlending();
    void operationGp0TexturedQuadOpaqueRawTexture();
    void operationGp0TexturedSemiTransparentOpaqueTextureBlending();
    void operationGp0TexturedSemiTransparentOpaqueRawTexture();
    void operationGp0TexturedQuad1x1OpaqueTextureBlending();
    void operationGp0TexturedQuad1x1OpaqueRawTexture();
    void operationGp0TexturedQuad1x1SemiTransparentTextureBlending();
    void operationGp0TexturedQuad1x1SemiTransparentRawTexture();
    void operationGp0TexturedQuad8x8OpaqueTextureBlending();
    void operationGp0TexturedQuad8x8OpaqueRawTexture();
    void operationGp0TexturedQuad8x8SemiTransparentTextureBlending();
    void operationGp0TexturedQuad8x8SemiTransparentRawTexture();
    void operationGp0TexturedQuad16x16OpaqueTextureBlending();
    void operationGp0TexturedQuad16x16OpaqueRawTexture();
    void operationGp0TexturedQuad16x16SemiTransparentTextureBlending();
    void operationGp0TexturedQuad16x16SemiTransparentRawTexture();

    void operationGp1Reset(uint32_t value);
    void operationGp1DisplayMode(uint32_t value);
    void operationGp1DMADirection(uint32_t value);
    void operationGp1StartOfDisplayArea(uint32_t value);
    void operationGp1HorizontalDisplayRange(uint32_t value);
    void operationGp1VerticalDisplayRange(uint32_t value);
    void operationGp1DisplayEnable(uint32_t value);
    void operationGp1AcknowledgeGPUInterrupt(uint32_t value);
    void operationGp1ResetCommandBuffer(uint32_t value);
    void operationGp1GetGPUInfo(uint32_t value);

    uint32_t statusRegister() const;
    uint32_t readRegister() const;

    void texturedQuad(Dimensions dimensions, bool opaque, TextureBlendMode textureBlendMode);
    void quad(Dimensions dimensions, bool opaque);
    void monochromePolygon(unsigned int numberOfPoints, bool opaque);
    void shadedPolygon(unsigned int numberOfPoints, bool opaque);
    void texturedPolygon(unsigned int numberOfPoints, bool opaque, TextureBlendMode textureBlendMode);
    void shadedTexturedPolygon(unsigned int numberOfPoints, bool opaque, TextureBlendMode textureBlendMode);
    void monochromeLine(unsigned int numberOfPoints, bool opaque);
    void shadedLine(unsigned int numberOfPoints, bool opaque);

    void executeGp1(uint32_t value);
    TexturePageColors texturePageColorsWithValue(uint32_t value) const;
    uint8_t horizontalResolutionFromValues(uint8_t value1, uint8_t value2) const;
public:
    GPU(LogLevel logLevel, std::unique_ptr<Window> &mainWindow);
    ~GPU();
    template <typename T>
    inline T load(uint32_t offset) const;
    template <typename T>
    inline void store(uint32_t offset, T value);

    // TODO: should be private
    void executeGp0(uint32_t value);
    void render();
    Dimensions getResolution();
    Point getDisplayAreaStart();
};
