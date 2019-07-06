#include "Vertex.hpp"
#include "GPU.hpp"

Vertex::Vertex(uint32_t position, uint32_t color) : texturePosition(), textureBlendMode(), texturePage(), textureDepthShift(), clut() {
    GLshort x = ((GLshort)(position & 0xffff));
    GLshort y = ((GLshort)((position >> 16) & 0xffff));
    this->position = {x, y};
    GLubyte red = ((GLubyte)(color & 0xff));
    GLubyte green = ((GLubyte)((color >> 8) & 0xff));
    GLubyte blue = ((GLubyte)((color >> 16) & 0xff));
    this->color = {red, green, blue};
}

/*
Texture page:
0-3   Texture page X Base   (N*64) (ie. in 64-halfword steps)    ;GPUSTAT.0-3
4     Texture page Y Base   (N*256) (ie. 0 or 256)               ;GPUSTAT.4
5-6   Semi Transparency     (0=B/2+F/2, 1=B+F, 2=B-F, 3=B+F/4)   ;GPUSTAT.5-6
7-8   Texture page colors   (0=4bit, 1=8bit, 2=15bit, 3=Reserved);GPUSTAT.7-8

CLUT:
0-5      X coordinate X/16  (ie. in 16-halfword steps)
6-14     Y coordinate 0-511 (ie. in 1-line steps)
15       Unknown/unused (should be 0)
*/
Vertex::Vertex(uint32_t position, uint32_t color, uint16_t texturePosition, TextureBlendMode textureBlendMode, uint32_t texturePage, uint16_t clutData) {
    GLshort x = ((GLshort)(position & 0xffff));
    GLshort y = ((GLshort)((position >> 16) & 0xffff));
    this->position = {x, y};
    GLubyte red = ((GLubyte)(color & 0xff));
    GLubyte green = ((GLubyte)((color >> 8) & 0xff));
    GLubyte blue = ((GLubyte)((color >> 16) & 0xff));
    this->color = {red, green, blue};
    GLshort textureX = ((GLshort)(texturePosition & 0xff));
    GLshort textureY = ((GLshort)((texturePosition >> 8) & 0xff));
    this->texturePosition = {textureX, textureY};
    this->textureBlendMode = textureBlendMode;
    GLshort texturePageX = ((GLshort)((texturePage & 0xf) << 6));
    GLshort texturePageY = ((GLshort)(((texturePage >> 4) & 0x1) << 8));
    this->texturePage = {texturePageX, texturePageY};
    TexturePageColors texturePageColors = texturePageColorsWithValue((texturePage >> 7) & 0x3);
    switch (texturePageColors) {
        case T4Bit: {
            this->textureDepthShift = 2;
            break;
        }
        case T8Bit: {
            this->textureDepthShift = 1;
            break;
        }
        case T15Bit: {
            this->textureDepthShift = 0;
            break;
        }
    }
    this->clut = { ((GLshort)((clutData & 0x3f) << 4)), ((GLshort)((clutData >> 6) & 0x1ff)) };
}

Vertex::~Vertex() {

}