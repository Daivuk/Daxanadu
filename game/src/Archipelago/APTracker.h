#pragma once

#include <onut/ForwardDeclaration.h>


OForwardDeclare(Texture);
class RAM;
class TileDrawer;


class APTracker
{
public:
    enum class item_t
    {
        hand_dagger,
        small_shield,
        key_jack,
        deluge,
        long_sword,
        large_shield,
        key_queen,
        thunder,
        giant_blade,
        magic_shield,
        key_king,
        fire,
        dragon_slayer,
        battle_helmet,
        key_ace,
        death,
        leather_armor,
        ring_of_elf,
        key_joker,
        tilte,
        studded_mail,
        ring_of_ruby,
        mattock,
        black_onyx,
        full_plate,
        ring_of_dworf,
        wingboots,
        magical_rod,
        battle_suit,
        demons_ring,
        spring_elixir,
        pendant,

        COUNT
    };

    APTracker(uint8_t* rom, RAM* ram, TileDrawer* tile_drawer);

    void update(float dt);
    void render();

private:
    void load_colors(uint8_t* colors);
    void bake(int dst_id, uint8_t* dst_image, uint8_t* palette, uint8_t tile0, uint8_t tile1, uint8_t tile2, uint8_t tile3, bool flip_green_red = false);
    void set_item_tracked(uint8_t item_id);

    uint8_t* m_rom = nullptr;
    RAM* m_ram = nullptr;
    OTextureRef m_sprite_sheet;
    bool m_states[(int)item_t::COUNT] = {false};
    TileDrawer* m_tile_drawer = nullptr;
};
