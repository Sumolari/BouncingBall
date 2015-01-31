// Copyright 2015 Lluís Ulzurrun de Asanza Sàez

#include <nds.h>
#include <nds/debug.h>
#include <sstream>

#include "./debug.h"

//------------------------------------------------------------------------------
// Graphic references
//------------------------------------------------------------------------------

#include "./gfx_ball.h"
#include "./gfx_brick.h"
#include "./gfx_gradient.h"

//------------------------------------------------------------------------------
// Tile entries
//------------------------------------------------------------------------------

#define TILE_EMPTY     0  // Tile 0 = empty
#define TILE_BRICK     1  // Tile 1 = brick
#define TILE_GRADIENT  2  // Tile 2 = gradient

// Macro for calculating BG VRAM memory address with tile index.
#define tile2bgram(t)  (BG_GFX + (t) * 16)

//------------------------------------------------------------------------------
// Palette entries
//------------------------------------------------------------------------------

#define PAL_BRICKS     0  // Brick palette (entry 0->15).
#define PAL_GRADIENT   1  // Gradient palette (entry 16->31).

#define BACKDROP_COLOR RGB8(190, 255, 255)

// Macro for calculating BG VRAM memory address with palette index.
#define pal2bgram(p)   (BG_PALETTE + (p) * 16)

//------------------------------------------------------------------------------
// BG Screen Base Blocks pointed
//------------------------------------------------------------------------------
#define bg0map    (reinterpret_cast<u16*>BG_MAP_RAM(1))
#define bg1map    (reinterpret_cast<u16*>BG_MAP_RAM(2))

//------------------------------------------------------------------------------
// Main code section
//------------------------------------------------------------------------------

/**
 * Sets up graphics.
 */
void setupGraphics(void) {
    vramSetBankE(VRAM_E_MAIN_BG);
    vramSetBankF(VRAM_F_MAIN_SPRITE);

    // Generate the first blank tile by clearing it to zero.
    for ( int n = 0; n < 16; n++ )
        BG_GFX[n] = 0;

    // Copy BG graphics.
    dmaCopyHalfWords(3, gfx_brickTiles, tile2bgram(TILE_BRICK),
                     gfx_brickTilesLen);
    dmaCopyHalfWords(3, gfx_gradientTiles, tile2bgram(TILE_GRADIENT),
                     gfx_gradientTilesLen);

    // Palettes go to palette memory.
    dmaCopyHalfWords(3, gfx_brickPal, pal2bgram(PAL_BRICKS), gfx_brickPalLen);
    dmaCopyHalfWords(3, gfx_gradientPal, pal2bgram(PAL_GRADIENT),
                     gfx_gradientPalLen);

    // Set backdrop color.
    BG_PALETTE[0] = BACKDROP_COLOR;

    // libnds prefixes the register names with REG_
    REG_BG0CNT = BG_MAP_BASE(1);
    REG_BG1CNT = BG_MAP_BASE(2);
}

void update_logic() {
}

void update_graphics() {
    // Clear entire bricks' tilemap to zero
    for ( int n = 0; n < 1024; n++ )
        bg0map[n] = 0;

    // Unsigned int16 has 16 bit size, the same as our register.
    uint16 pal_bricks_bit = PAL_BRICKS << 12;

    // Set tilemap entries for 6 first rows.
    for ( int y = 0; y < 6; y++ ) {
        int y32 = y * 32;

        for ( int x = 0; x < 32; x++ ) {
            // Magical formula to calculate if the tile needs to be flipped.
            // Basically: x & 1 -> AND operation between both numbers, that is:
            //            if last bit is 1, 1, if not, 0. This allows to check
            //            if a number is odd or even in a very fast way.
            //            y & 1 -> Works in the very same way as x & 1.
            //            ^ is the xor operator.
            int hflip = (x & 1) ^ (y & 1);

            // Set the tilemap entry
            bg0map[x + y32] = TILE_BRICK | (hflip << 10) | pal_bricks_bit;
        }
    }

    videoSetMode(MODE_0_2D | DISPLAY_BG0_ACTIVE);
}

int main(void) {
    irqInit();               // Initialize interrups.
    irqEnable(IRQ_VBLANK);   // Enable vblank interrupt.
    setupGraphics();
    while (1) {
        // Rendering period:
        // Update game objects.
        update_logic();

        // Wait for the vblank period.
        swiWaitForVBlank();

        // VBlank period: (safe yo modify graphics)
        // Move the graphics around.
        update_graphics();
    }
}
