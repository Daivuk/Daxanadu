#pragma once

#include "CPUPeripheral.h"
#include "PPUPeripheral.h"

#include <onut/Color.h>
#include <onut/ForwardDeclaration.h>


OForwardDeclare(Texture);
class CPU;


class PPU final : public CPUPeripheral, public PPUPeripheral
{
public:
    static const int SCREEN_W = 256;
    static const int SCREEN_H = 240;

    PPU(CPU* cpu);
    ~PPU();

    void serialize(FILE* f, int version) const;
    void deserialize(FILE* f, int version);

    bool cpu_write(uint16_t addr, uint8_t data) override;
    bool cpu_read(uint16_t addr, uint8_t* out_data) override;
    bool ppu_write(uint16_t addr, uint8_t data) override;
    bool ppu_read(uint16_t addr, uint8_t* out_data) override;

    Color get_color(int idx);

    void reset();
    void tick();
    void render();

private:
    void load_colors();
    void update_screen();
    void update_pattern_table(int idx);
    void update_nametable(int idx);
    void update_sprites(int idx);

    uint8_t m_PPUCTRL_register = 0;
    uint8_t m_PPUMASK_register = 0;
    uint8_t m_PPUSTATUS_register = 0;
    uint8_t m_OAMADDR_register = 0;
    uint16_t m_PPUSCROLL_register = 0;
    uint16_t m_PPUADDR_register = 0;

    // Ram
    uint8_t m_nametables[2][1024];
    uint8_t m_sprites[256] = { 0 };
    uint8_t m_palettes[32] = { 0 };

    CPU* m_cpu = nullptr;
    uint8_t m_colors[192] = { 0 };
    uint8_t* m_sprite_pixels = nullptr;
    uint8_t* m_pattern_pixels = nullptr;
    uint8_t* m_nametable_pixels = nullptr;
    OTextureRef m_screen_texture;
    OTextureRef m_sprite_textures[2]; // Background then foreground
    OTextureRef m_chr_textures[2];
    OTextureRef m_nametable_textures[2];
    int m_row = 261;
    int m_col = 0;
    int m_frames = 0;
    int m_scroll_h = 0;
    int m_scroll_v = 0;
    int m_display_scroll_h = 0;
};
