#include "TileDrawer.h"
#include "PPU.h"

#include <onut/Dialogs.h>
#include <onut/onut.h>
#include <onut/Renderer.h>
#include <onut/SpriteBatch.h>
#include <onut/Texture.h>


static const int TILESET_W = 128;
static const int TILESET_H = 128;


TileDrawer::TileDrawer(const uint8_t* rom, PPU* ppu)
    : m_ppu(ppu)
{
    uint8_t* tileset_data = new uint8_t[TILESET_W * TILESET_H * 4];
    memset(tileset_data, 0, TILESET_W * TILESET_H * 4);

    // Load palette from the ROM file
    uint8_t colors[192];
    uint8_t indexed_palette[4];
    uint8_t palette[4 * 3];
    load_colors(colors);
    memcpy(indexed_palette, &rom[0x0002C000 + 12], 4);
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 3; ++j)
            palette[i * 3 + j] = colors[indexed_palette[i] * 3 + j];

    // Load tilesets
    load_tileset_range(rom, tileset_data, palette, 0x00028000, 80, 0);
    load_tileset_range(rom, tileset_data, palette, 0x00020C04 + 50 * 16, 8, 80);

    m_texture = OTexture::createFromData(tileset_data, { TILESET_W, TILESET_H }, false);
    delete[] tileset_data;
}


void TileDrawer::load_colors(uint8_t* colors)
{
    const char* palette_filename = "FCEUX.pal";

    FILE* f = fopen(palette_filename, "rb");
    if (!f)
    {
        onut::showMessageBox("ERROR", "Missing palette file: " + std::string(palette_filename));
        OQuit();
        return;
    }
    if (fread(colors, 1, 192, f) != 192)
    {
        onut::showMessageBox("ERROR", "Corrupted palette file: " + std::string(palette_filename));
        OQuit();
        fclose(f);
        return;
    }
    fclose(f);
}


void TileDrawer::load_tileset_range(const uint8_t* rom,
                                    uint8_t* out,
                                    const uint8_t* palette,
                                    int addr, int count, int offset)
{
    const int x_tile_count = TILESET_W / 8;
    const int dst_x_inc = 4;
    const int dst_y_inc = TILESET_W * 4 - 8 * 4;

    for (int i = 0; i < count; ++i)
    {
        int dst_tile = offset + i;
        int dst_tile_x = dst_tile % x_tile_count;
        int dst_tile_y = dst_tile / x_tile_count;
        int dst_k = dst_tile_y * TILESET_W * 4 * 8 + dst_tile_x * 8 * 4;

        auto src_data = &rom[addr + i * 16];
        auto dst_data = &out[dst_k];

        for (int y = 0; y < 8; ++y, dst_data += dst_y_inc)
        {
            for (int x = 0; x < 8; ++x, dst_data += dst_x_inc)
            {
                int b1 = (src_data[y] >> (7 - x)) & 1;
                int b2 = (src_data[y + 8] >> (7 - x)) & 1;
                int col = (b1) | (b2 << 1);

                dst_data[0] = palette[col * 3 + 0];
                dst_data[1] = palette[col * 3 + 1];
                dst_data[2] = palette[col * 3 + 2];
                dst_data[3] = 255;
            }
        }
    }
}


void TileDrawer::draw_tile_fine(int tile_id, int x, int y)
{
    const int x_tile_count = TILESET_W / 8;
    const float tile_uv_size = 8.0f / (float)TILESET_W;

    int tile_x = tile_id % x_tile_count;
    int tile_y = tile_id / x_tile_count;
    float u = (float)tile_x * tile_uv_size;
    float v = (float)tile_y * tile_uv_size;

    oSpriteBatch->drawRectWithUVs(m_texture,
                                  Rect((float)x, (float)y, 8.0f, 8.0f),
                                  Vector4(u, v, u + tile_uv_size, v + tile_uv_size));
}


void TileDrawer::draw_tile(int tile_id, int x, int y, const Color& color)
{
    const int x_tile_count = TILESET_W / 8;
    const float tile_uv_size = 8.0f / (float)TILESET_W;

    int tile_x = tile_id % x_tile_count;
    int tile_y = tile_id / x_tile_count;
    float u = (float)tile_x * tile_uv_size;
    float v = (float)tile_y * tile_uv_size;

    oSpriteBatch->drawRectWithUVs(m_texture,
                                  Rect((float)(x * 8), (float)(y * 8), 8.0f, 8.0f),
                                  Vector4(u, v, u + tile_uv_size, v + tile_uv_size),
                                  color);
}


void TileDrawer::draw_ui_frame(int x, int y, int w, int h, bool only_background)
{
    draw_ui_frame_fine(x * 8, y * 8, w, h, only_background);
}


void TileDrawer::draw_ui_frame_fine(int x, int y, int w, int h, bool only_background)
{
    // Fill middle
    for (int j = 1; j < h - 1; ++j)
    {
        for (int i = 1; i < w - 1; ++i)
        {
            draw_tile_fine(0x00, x + i * 8, y + j * 8);
        }
    }

    if (only_background) return;

    // Corners
    draw_tile_fine(0x15, x, y);
    draw_tile_fine(0x19, x, y + h * 8 - 8);
    draw_tile_fine(0x1A, x + w * 8 - 8, y + h * 8 - 8);
    draw_tile_fine(0x17, x + w * 8 - 8, y);

    // Edges
    for (int i = 1; i < w - 1; ++i)
    {
        draw_tile_fine(0x16, x + i * 8, y);
        draw_tile_fine(0x16, x + i * 8, y + h * 8 - 8);
    }
    for (int j = 1; j < h - 1; ++j)
    {
        draw_tile_fine(0x18, x, y + j * 8);
        draw_tile_fine(0x18, x + w * 8 - 8, y + j * 8);
    }
}


void TileDrawer::draw_text(int x, int y, const char* text, const Color& tint)
{
    Color color = tint;
    int start_x = x;
    while (char c = *text++)
    {
        int tile_id = 0;

        if (c >= 'A' && c <= 'Z') tile_id = c - 'A' + 0x24;
        else if (c >= '0' && c <= '9') tile_id = c - '0' + 0x44;
        else if (c == ' ') tile_id = 0x20;
        else if (c == '[') tile_id = 0x22;
        else if (c == ']') tile_id = 0x23;
        else if (c == '@') tile_id = 0x3E;
        else if (c == ':') tile_id = 0x4E;
        else if (c == '-') tile_id = 0x1B;
        else if (c == '=') tile_id = 0x1C;
        else if (c == '\n')
        {
            y++;
            x = start_x;
            continue;
        }
        else if (c == '^')
        {
            c = *text++;
            if (c == '\0') break;
            if (c == '0') color = m_ppu->get_color(0x0F);
            else if (c == '1') color = m_ppu->get_color(0x15);
            else if (c == '2') color = m_ppu->get_color(0x19);
            else if (c == '3') color = Color::White;
            color *= tint;
            continue;
        }
        else if (c == '.')
        {
            // Just draw a rectangle. No '.' in Menu font
            oSpriteBatch->drawRect(nullptr, { (float)(x++ * 8 + 2), (float)(y * 8 + 5), 2, 2 }, color);
            continue;
        }
        else if (c == '*')
        {
            // Just draw a rectangle. No '.' in Menu font
            oSpriteBatch->drawRect(nullptr, { (float)(x++ * 8 + 1), (float)(y * 8 + 3), 4, 4 }, color);
            continue;
        }

        draw_tile(tile_id, x++, y, color);
    }
}


void TileDrawer::draw_cursor(int x, int y)
{
    draw_tile(0x50, x, y);
}
