#include "PPU.h"
#include "CPU.h"
#include "CPUBUS.h"
#include "PPUBUS.h"

#include <onut/Dialogs.h>
#include <onut/onut.h>
#include <onut/Random.h>
#include <onut/Renderer.h>
#include <onut/SpriteBatch.h>
#include <onut/Texture.h>

#include <stdio.h>


static const int PPU_COL_COUNT = 341;
static const int PPU_ROW_COUNT = 262;


PPU::PPU(CPU* cpu)
    : m_cpu(cpu)
{
    memset(m_nametables, 0, sizeof(m_nametables));
    memset(m_sprites, 0, sizeof(m_sprites));

    load_colors();

    m_sprite_pixels = new uint8_t[SCREEN_W * SCREEN_H * 4];
    memset(m_sprite_pixels, 0, SCREEN_W * SCREEN_H * 4);

    m_pattern_pixels = new uint8_t[128 * 128 * 4];
    memset(m_pattern_pixels, 0, 128 * 128 * 4);

    m_nametable_pixels = new uint8_t[SCREEN_W * SCREEN_H * 4];
    memset(m_nametable_pixels, 0, SCREEN_W * SCREEN_H * 4);

    m_screen_texture = OTexture::createRenderTarget({ SCREEN_W, SCREEN_H });
    m_sprite_textures[0] = OTexture::createDynamic({ SCREEN_W, SCREEN_H });
    m_sprite_textures[0]->setData(m_sprite_pixels);
    m_sprite_textures[1] = OTexture::createDynamic({ SCREEN_W, SCREEN_H });
    m_sprite_textures[1]->setData(m_sprite_pixels);
    m_chr_textures[0] = OTexture::createDynamic({ 128, 128 });
    m_chr_textures[0]->setData(m_sprite_pixels);
    m_chr_textures[1] = OTexture::createDynamic({ 128, 128 });
    m_chr_textures[1]->setData(m_sprite_pixels);
    m_nametable_textures[0] = OTexture::createDynamic({ SCREEN_W, SCREEN_H });
    m_nametable_textures[0]->setData(m_sprite_pixels);
    m_nametable_textures[1] = OTexture::createDynamic({ SCREEN_W, SCREEN_H });
    m_nametable_textures[1]->setData(m_sprite_pixels);
}


PPU::~PPU()
{
    delete[] m_nametable_pixels;
    delete[] m_pattern_pixels;
    delete[] m_sprite_pixels;
}


void PPU::load_colors()
{
    const char* palette_filename = "FCEUX.pal";

    FILE* f = fopen(palette_filename, "rb");
    if (!f)
    {
        onut::showMessageBox("ERROR", "Missing palette file: " + std::string(palette_filename));
        OQuit();
        return;
    }
    if (fread(m_colors, 1, 192, f) != 192)
    {
        onut::showMessageBox("ERROR", "Corrupted palette file: " + std::string(palette_filename));
        OQuit();
        fclose(f);
        return;
    }
    fclose(f);
}


void PPU::serialize(FILE* f, int version) const
{
    fwrite(&m_PPUCTRL_register, sizeof(m_PPUCTRL_register), 1, f);
    fwrite(&m_PPUMASK_register, sizeof(m_PPUMASK_register), 1, f);
    fwrite(&m_PPUSTATUS_register, sizeof(m_PPUSTATUS_register), 1, f);
    fwrite(&m_OAMADDR_register, sizeof(m_OAMADDR_register), 1, f);
    fwrite(&m_PPUSCROLL_register, sizeof(m_PPUSCROLL_register), 1, f);
    fwrite(&m_PPUADDR_register, sizeof(m_PPUADDR_register), 1, f);
    
    fwrite(m_nametables, 1, sizeof(m_nametables), f);
    fwrite(m_sprites, 1, sizeof(m_sprites), f);
    fwrite(m_palettes, 1, sizeof(m_palettes), f);

    fwrite(&m_row, sizeof(m_row), 1, f);
    fwrite(&m_col, sizeof(m_col), 1, f);
    fwrite(&m_frames, sizeof(m_frames), 1, f);
    fwrite(&m_scroll_h, sizeof(m_scroll_h), 1, f);
    fwrite(&m_scroll_v, sizeof(m_scroll_v), 1, f);
    fwrite(&m_display_scroll_h, sizeof(m_display_scroll_h), 1, f);
}


void PPU::deserialize(FILE* f, int version)
{
    fread(&m_PPUCTRL_register, sizeof(m_PPUCTRL_register), 1, f);
    fread(&m_PPUMASK_register, sizeof(m_PPUMASK_register), 1, f);
    fread(&m_PPUSTATUS_register, sizeof(m_PPUSTATUS_register), 1, f);
    fread(&m_OAMADDR_register, sizeof(m_OAMADDR_register), 1, f);
    fread(&m_PPUSCROLL_register, sizeof(m_PPUSCROLL_register), 1, f);
    fread(&m_PPUADDR_register, sizeof(m_PPUADDR_register), 1, f);
    
    fread(m_nametables, 1, sizeof(m_nametables), f);
    fread(m_sprites, 1, sizeof(m_sprites), f);
    fread(m_palettes, 1, sizeof(m_palettes), f);

    fread(&m_row, sizeof(m_row), 1, f);
    fread(&m_col, sizeof(m_col), 1, f);
    fread(&m_frames, sizeof(m_frames), 1, f);
    fread(&m_scroll_h, sizeof(m_scroll_h), 1, f);
    fread(&m_scroll_v, sizeof(m_scroll_v), 1, f);
    fread(&m_display_scroll_h, sizeof(m_display_scroll_h), 1, f);
}


bool PPU::cpu_write(uint16_t addr, uint8_t data)
{
    if (addr == 0x4014)
    {
        uint16_t ram_addr = data << 8;
        uint8_t byte;
        for (int i = 0; i < 256; ++i)
        {
            get_cpu_bus()->read(ram_addr + static_cast<uint16_t>(i), &byte);
            m_sprites[i] = byte;
        }
        m_cpu->halt(513);
        return true;
    }

    if (addr < 0x2000 || addr > 0x3FFF) return false;

    // Mirrors
    addr = ((addr - 0x2000) % 8) + 0x2000;

    switch (addr)
    {
        case 0x2000:
            m_PPUCTRL_register = data;
            return true;
        case 0x2001:
            m_PPUMASK_register = data;
            return true;
        case 0x2002:
            return false;
        case 0x2003:
            m_OAMADDR_register = data;
            return false;
        case 0x2004:
            return false;
        case 0x2005:
            m_PPUSCROLL_register = (m_PPUSCROLL_register << 8) | data;
            //m_scroll_h = static_cast<int>((m_PPUSCROLL_register >> 8) & 0xFF);
            m_scroll_h = static_cast<int>((m_PPUSCROLL_register >> 8) & 0xFF) + (static_cast<int>(m_PPUCTRL_register & 0b1) << 8);
            return true;
        case 0x2006:
            m_PPUADDR_register = ((m_PPUADDR_register << 8) | data) & 0x3FFF;
            return true;
        case 0x2007:
            get_ppu_bus()->write(m_PPUADDR_register, data);
            m_PPUADDR_register += (m_PPUCTRL_register & 0b100) ? 32 : 1;
            return true;
        default:
            return false;
    }
}


bool PPU::cpu_read(uint16_t addr, uint8_t* out_data)
{
    if (addr < 0x2000 || addr > 0x2007) return false;

    // Mirrors
    addr = ((addr - 0x2000) % 8) + 0x2000;

    *out_data = 0;

    switch (addr)
    {
        case 0x2000:
            return false;
        case 0x2001:
            return false;
        case 0x2002:
            *out_data = m_PPUSTATUS_register & 0b11100000;
            m_PPUSTATUS_register &= 0b01111111;
            m_PPUADDR_register = 0;
            return true;
        case 0x2003:
            return false;
        case 0x2004:
            return false;
        case 0x2005:
            return false;
        case 0x2006:
            return false;
        case 0x2007:
            get_ppu_bus()->read(m_PPUADDR_register, out_data);
            return true;
        default:
            return false;
    }
}


bool PPU::ppu_write(uint16_t addr, uint8_t data)
{
    if (addr < 0x2000 || addr >= 0x4000) return false; // Pattern tables are stored in the cart

    if (addr >= 0x2000 && addr <= 0x3EFF)
    {
        addr = (addr - 0x2000) % 0x0800;
        if (addr >= 0x0000 && addr <= 0x03FF)
        {
            m_nametables[0][addr] = data;
            return true;
        }
        else if (addr >= 0x0400 && addr <= 0x07FF)
        {
            m_nametables[1][addr - 0x400] = data;
        }
    }
    else if (addr >= 0x3F00 && addr <= 0x3FFF)
    {
        m_palettes[addr & 0b11111] = data;
        return true;
    }

    return false;
}


bool PPU::ppu_read(uint16_t addr, uint8_t* out_data)
{
    if (addr < 0x2000 || addr >= 0x4000) return false; // Pattern tables are stored in the cart

    if (addr >= 0x2000 && addr <= 0x3EFF)
    {
        addr = (addr - 0x2000) % 0x0800;
        if (addr >= 0x0000 && addr <= 0x03FF)
        {
            *out_data = m_nametables[0][addr];
            return true;
        }
        else if (addr >= 0x0400 && addr <= 0x07FF)
        {
            *out_data = m_nametables[1][addr - 0x400];
        }
    }
    else if (addr >= 0x3F00 && addr <= 0x3FFF)
    {
        *out_data = m_palettes[addr & 0b11111];
        return true;
    }

    return false;
}


void PPU::reset()
{
    // Usually none of these are reset. The PPU keeps going.
    // But I like to start each games with a clean slate.
    m_PPUCTRL_register = 0;
    m_PPUMASK_register = 0;
    m_PPUSTATUS_register = 0;
    m_OAMADDR_register = 0;
    m_PPUSCROLL_register = 0;
    m_PPUADDR_register = 0;

    m_row = 261;
    m_col = 0;
    m_frames = 0;
    m_scroll_h = 0;
    m_scroll_v = 0;
}


void PPU::update_pattern_table(int idx)
{
    int addr = idx * 0x1000;
    auto ppu_bus = get_ppu_bus();

    for (int ty = 0; ty < 16; ++ty)
    {
        int dst_y = ty * 8 * 16 * 8 * 4;
        int src_y = ty * 16 * 16;
        for (int tx = 0; tx < 16; ++tx)
        {
            int dst_x = tx * 8 * 4;
            int dst_k = dst_y + dst_x;
            int src_x = tx * 16;
            int src_k = addr + src_y + src_x;
            for (int y = 0; y < 8; ++y, dst_k += 16 * 8 * 4 - 8 * 4, ++src_k)
            {
                uint8_t plane0 = 0x55;
                uint8_t plane1 = 0x55;
                
                ppu_bus->read(src_k, &plane0);
                ppu_bus->read(src_k + 8, &plane1);

                for (int x = 0; x < 8; ++x, dst_k += 4)
                {
                    int b0 = (plane0 >> (7 - x)) & 1;
                    int b1 = (plane1 >> (7 - x)) & 1;
                    int col = (b0) | (b1 << 1);
                    m_pattern_pixels[dst_k + 0] = col * 85;
                    m_pattern_pixels[dst_k + 1] = col * 85;
                    m_pattern_pixels[dst_k + 2] = col * 85;
                    m_pattern_pixels[dst_k + 3] = 255;
                }
            }
        }
    }

    m_chr_textures[idx]->setData(m_pattern_pixels);
}


Color PPU::get_color(int idx)
{
    return Color(
        (float)m_colors[idx * 3 + 0] / 255.0f,
        (float)m_colors[idx * 3 + 1] / 255.0f,
        (float)m_colors[idx * 3 + 2] / 255.0f,
        1.0f);
}


void PPU::update_nametable(int idx)
{
    memset(m_nametable_pixels, 0, SCREEN_W * SCREEN_H * 4);

    if (!(m_PPUMASK_register & 0b00001000))
    {
        // nametables off
        m_nametable_textures[idx]->setData(m_nametable_pixels);
        return;
    }

    auto ppu_bus = get_ppu_bus();
    int chr_offset = 0x1000;

    // There are faster ways to blit that data... There's a reason the data is layed out the way it is
    for (int ty = 0; ty < 30; ++ty)
    {
        int dst_y = ty * 8 * 32 * 8 * 4;
        int src_y = ty * 32;
        for (int tx = 0; tx < 32; ++tx)
        {
            int dst_x = tx * 8 * 4;
            int dst_k = dst_y + dst_x;
            int src_x = tx;
            int src_k = src_y + src_x;

            int attrib_k = (ty / 4) * 8 + (tx / 4);
            uint8_t attrib = m_nametables[idx][960 + attrib_k];

            int quadran = ((tx / 2) & 1) + (((ty / 2) & 1) * 2);
            int pal_idx = (attrib >> (quadran * 2)) & 0b11;

            uint8_t* pal = m_palettes + pal_idx * 4; // Should do a ppu read here
            uint8_t tile_id = m_nametables[idx][src_k];

            int chr_src_k = tile_id * 16 + chr_offset;
            for (int y = 0; y < 8; ++y, dst_k += 32 * 8 * 4 - 8 * 4, chr_src_k++)
            {
                uint8_t plane0 = 0x55;
                uint8_t plane1 = 0x55;
                
                ppu_bus->read(chr_src_k, &plane0);
                ppu_bus->read(chr_src_k + 8, &plane1);

                for (int x = 0; x < 8; ++x, dst_k += 4)
                {
                    int b0 = (plane0 >> (7 - x)) & 1;
                    int b1 = (plane1 >> (7 - x)) & 1;
                    int col = (b0) | (b1 << 1);

                    if (col)
                    {
                        m_nametable_pixels[dst_k + 0] = m_colors[pal[col] * 3 + 0];
                        m_nametable_pixels[dst_k + 1] = m_colors[pal[col] * 3 + 1];
                        m_nametable_pixels[dst_k + 2] = m_colors[pal[col] * 3 + 2];
                        m_nametable_pixels[dst_k + 3] = 255;
                    }
                }
            }
        }
    }

    m_nametable_textures[idx]->setData(m_nametable_pixels);
}


void PPU::update_sprites(int idx)
{
    memset(m_sprite_pixels, 0, SCREEN_W * SCREEN_H * 4);
    if (!(m_PPUMASK_register & 0b00010000))
    {
        m_sprite_textures[idx]->setData(m_sprite_pixels);
        return;
    }

    auto ppu_bus = get_ppu_bus();
    for (int i = 0; i < 256; i += 4)
    {
        auto sprite = m_sprites + i;

        if (sprite[2] & 0b00100000)
        {
            if (idx) continue;
        }
        else
        {
            if (!idx) continue;
        }

        int pal_idx = sprite[2] & 0b11;
        uint8_t* pal = m_palettes + 16 + pal_idx * 4; // Should do a ppu read here

        int dst_k = ((int)sprite[0] + 1) * SCREEN_W * 4 + (int)sprite[3] * 4;
        int dst_inc_y = SCREEN_W * 4 - 8 * 4;
        int dst_inc_x = 4;
        int src_k = (int)sprite[1] * 16;
        int src_inc_y = 1;
        bool flipped_h = (sprite[2] & 0b01000000) ? true : false;

        if (sprite[2] & 0b10000000)
        {
            // y-flipped
            src_k += 7;
            src_inc_y = -src_inc_y;
        }

        for (int y = 0; y < 8; ++y, src_k += src_inc_y, dst_k += dst_inc_y)
        {
            if (((int)sprite[0] + 1) + y >= SCREEN_H) break;

            uint8_t plane0 = 0x55;
            uint8_t plane1 = 0x55;
            
            ppu_bus->read(src_k, &plane0);
            ppu_bus->read(src_k + 8, &plane1);

            if (flipped_h)
            {
                for (int x = 0; x < 8; ++x, dst_k += dst_inc_x)
                {
                    if ((int)sprite[3] + x < SCREEN_W)
                    {
                        int b0 = (plane0 >> x) & 1;
                        int b1 = (plane1 >> x) & 1;
                        int col = (b0) | (b1 << 1);

                        if (col)
                        {
                            m_sprite_pixels[dst_k + 0] = m_colors[pal[col] * 3 + 0];
                            m_sprite_pixels[dst_k + 1] = m_colors[pal[col] * 3 + 1];
                            m_sprite_pixels[dst_k + 2] = m_colors[pal[col] * 3 + 2];
                            m_sprite_pixels[dst_k + 3] = 255;
                        }
                    }
                }
            }
            else
            {
                for (int x = 0; x < 8; ++x, dst_k += dst_inc_x)
                {
                    if ((int)sprite[3] + x < SCREEN_W)
                    {
                        int b0 = (plane0 >> (7 - x)) & 1;
                        int b1 = (plane1 >> (7 - x)) & 1;
                        int col = (b0) | (b1 << 1);

                        if (col)
                        {
                            m_sprite_pixels[dst_k + 0] = m_colors[pal[col] * 3 + 0];
                            m_sprite_pixels[dst_k + 1] = m_colors[pal[col] * 3 + 1];
                            m_sprite_pixels[dst_k + 2] = m_colors[pal[col] * 3 + 2];
                            m_sprite_pixels[dst_k + 3] = 255;
                        }
                    }
                }
            }
        }
    }

    m_sprite_textures[idx]->setData(m_sprite_pixels);
}


void PPU::update_screen()
{
    // Animate mist in ROM directly


    //
    update_pattern_table(0);
    update_pattern_table(1);
    update_nametable(0);
    update_nametable(1);
    update_sprites(0);
    update_sprites(1);
}


void PPU::tick()
{
    if (m_row == 261)
    {
        if (m_col == 1)
        {
            // Clear sprite 0 hit
            m_PPUSTATUS_register &= 0b00111111;
        }
        else if (m_col == 257 || m_col == 320)
        {
            m_OAMADDR_register = 0;
        }
#if 0
        // Reload vertical scroll register
        else if (m_col == 280)
        {
            m_scroll_v = static_cast<int>(m_PPUSCROLL_register & 0xFF) + (static_cast<int>((m_PPUCTRL_register >> 1) & 0b1) << 8);
        }
#endif
    }
    else if (m_row >= 0 && m_row <= 239)
    {
        // Visible scanlines
        if (m_row == 25 && m_col == 1)
        {
            // Sprite 0 hit
            m_PPUSTATUS_register |= 0b01000000;
        }

        // First DOT is idle
        if (m_col >= 1 && m_col <= 256)
        {
#if 0
            int dot = m_col - 1;

            // This is not optimized, but it'll do for now
            uint16_t pattern_addr = ((m_scroll_h + dot) >> 3);
            uint8_t tile = 0;
            ppu_read(


            int dst_k = (m_row * 256 * 4) + dot * 4;
            uint8_t pixel = onut::randb() ? 255 : 0;
            m_screen_pixels[dst_k + 0] = pixel;
            m_screen_pixels[dst_k + 1] = pixel;
            m_screen_pixels[dst_k + 2] = pixel;
            m_screen_pixels[dst_k + 3] = 255;
#endif
        }
        else if (m_col == 257)
        {
            // Fetch sprites for next frame
            m_OAMADDR_register = 0;
        }
    }
    else if (m_row == 241)
    {
        if (m_col == 1)
        {
            // v-blank, update our screen texture
            m_PPUSTATUS_register |= 0b10000000;
            m_display_scroll_h = m_scroll_h;
            if (m_PPUCTRL_register | 0b10000000)
                m_cpu->NMI();
            update_screen();
        }
    }

    // Pass to the next dot
    m_col++;

    // Odd frames, the first scanline has 1 less cycle
    int end_line = PPU_COL_COUNT;
    if (m_row == 261 && (m_frames & 0b1))
    {
        end_line--;
    }

    // End of scanline
    if (m_col >= end_line)
    {
        m_col = 0;
        m_row++;
    }

    // End of frame
    if (m_row >= PPU_ROW_COUNT)
    {
        m_row = 0;
    }
}


void PPU::render()
{
    auto res = OScreenf;
    float scale = std::floor(res.y / (float)SCREEN_H);

    oRenderer->renderStates.renderTargets[0].push(m_screen_texture);
    oRenderer->clear(OColorRGB(m_colors[m_palettes[0] * 3 + 0], m_colors[m_palettes[0] * 3 + 1], m_colors[m_palettes[0] * 3 + 2]));
    oSpriteBatch->begin();
    
    // Draw background sprites
    oSpriteBatch->drawSprite(m_sprite_textures[0], Vector2::Zero, Color::White, OTopLeft);

    // Render the nametables to the render target.
    oSpriteBatch->drawRectWithUVs(m_nametable_textures[0], 
                                    Rect(-(float)m_display_scroll_h, 32.0f, (float)SCREEN_W, (float)SCREEN_H - 32.0f),
                                    Vector4(0, 32.0f / (float)SCREEN_H, 1, 1), Color::White);
    oSpriteBatch->drawRectWithUVs(m_nametable_textures[1], 
                                    Rect((float)SCREEN_W - (float)m_display_scroll_h, 32.0f, (float)SCREEN_W, (float)SCREEN_H - 32.0f),
                                    Vector4(0, 32.0f / (float)SCREEN_H, 1, 1), Color::White);
    oSpriteBatch->drawRectWithUVs(m_nametable_textures[0], 
                                    Rect(-(float)m_display_scroll_h + (float)SCREEN_W * 2, 32.0f, (float)SCREEN_W, (float)SCREEN_H - 32.0f),
                                    Vector4(0, 32.0f / (float)SCREEN_H, 1, 1), Color::White);

    // The top part is always at scroll 0
    oSpriteBatch->drawRectWithUVs(m_nametable_textures[0], Rect(0, 0, (float)SCREEN_W, 32.0f), Vector4(0, 0, 1, 32.0f / (float)SCREEN_H), Color::White);

    // Draw foreground sprites
    oSpriteBatch->drawSprite(m_sprite_textures[1], Vector2::Zero, Color::White, OTopLeft);

    oSpriteBatch->end();
    oRenderer->renderStates.renderTargets[0].pop();
    
    // Draw final image
    oSpriteBatch->begin();
    oSpriteBatch->drawSpriteWithUVs(m_screen_texture, OScreenCenterf, 
                                    Vector4(0, 8.0f / (float)SCREEN_H, 1.0f, ((float)SCREEN_H - 8.0f) / (float)SCREEN_H),
                                    Color::White, 0.0f, scale, OCenter);
#if defined(_DEBUG) // To be safe
#if 0
    oSpriteBatch->drawSprite(m_chr_textures[0], Vector2(0, 16), Color::White, 0.0f, 2.0f, OTopLeft);
    oSpriteBatch->drawSprite(m_chr_textures[1], Vector2(0, 16 + 128 * 2 + 2), Color::White, 0.0f, 2.0f, OTopLeft);
    oSpriteBatch->drawSprite(m_nametable_textures[0], Vector2(res.x - SCREEN_W, 16), Color::White, OTopLeft);
    oSpriteBatch->drawSprite(m_nametable_textures[1], Vector2(res.x - SCREEN_W, 16 + SCREEN_H + 2), Color::White, OTopLeft);
#endif
#endif
    oSpriteBatch->end();
}
