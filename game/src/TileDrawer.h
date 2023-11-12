#pragma once

#include <onut/Color.h>
#include <onut/ForwardDeclaration.h>


OForwardDeclare(Texture);
class PPU;


// TileDrawer extract graphics data from the cart and 
// provide functions to draw stuff, like UI text and Boxes
class TileDrawer final
{
public:
    TileDrawer(const uint8_t* rom, PPU* ppu);

    void draw_ui_frame(int x, int y, int w, int h, bool only_background = false);
    void draw_ui_frame_fine(int x, int y, int w, int h, bool only_background = false);
    void draw_text(int x, int y, const char* text, const Color& tint = Color::White);
    void draw_cursor(int x, int y);

private:
    static void load_colors(uint8_t* colors);
    static void load_tileset_range(const uint8_t* rom,
                                   uint8_t* out,
                                   const uint8_t* palette,
                                   int addr, int count, int offset);

    void draw_tile(int tile_id, int x, int y, const Color& color = Color::White);
    void draw_tile_fine(int tile_id, int x, int y);

    OTextureRef m_texture;
    PPU* m_ppu = nullptr;
};
