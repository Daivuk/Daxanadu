#include "APTracker.h"
#include "APItems.h"
#include "PPU.h"
#include "RAM.h"
#include "TileDrawer.h"

#include <onut/Dialogs.h>
#include <onut/onut.h>
#include <onut/Input.h>
#include <onut/Texture.h>
#include <onut/Renderer.h>
#include <onut/SpriteBatch.h>


static const char* ITEM_NAMES[] = {
    "HAND DAGGER",
    "SMALL SHIELD",
    "KEY JACK",
    "DELUGE",
    "LONG SWORD",
    "LARGE SHIELD",
    "KEY QUEEN",
    "THUNDER",
    "GIANT BLADE",
    "MAGIC SHIELD",
    "KEY KING",
    "FIRE",
    "DRAGON SLAYER",
    "BATTLE HELMET",
    "KEY ACE",
    "DEATH",
    "LEATHER ARMOR",
    "RING OF ELF",
    "KEY JOKER",
    "TILTE",
    "STUDDED MAIL",
    "RING OF RUBY",
    "MATTOCK",
    "BLACK ONYX",
    "FULL PLATE",
    "RING OF DWORF",
    "WINGBOOTS",
    "MAGICAL ROD",
    "BATTLE SUIT",
    "DEMONS RING",
    "SPRING ELIXIR",
    "PENDANT",

    "SKY SPRING",
    "TOWER OF FORTRESS SPRING",
    "JOKER SPRING"
};


APTracker::APTracker(uint8_t* rom, RAM* ram, TileDrawer* tile_drawer)
	: m_rom(rom)
	, m_ram(ram)
    , m_tile_drawer(tile_drawer)
{
    for (int i = 0; i < (int)item_t::COUNT; ++i)
    {
        m_states[i] = false;
    }

    // Load palettes from the ROM file
    uint8_t palette[4 * 3];
    uint8_t entity_palette[4 * 3];
    uint8_t colors[192];
    load_colors(colors);

    {
        uint8_t indexed_palette[4];
        memcpy(indexed_palette, &rom[0x0002C000 + 12], 4);
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 3; ++j)
                palette[i * 3 + j] = colors[indexed_palette[i] * 3 + j];
    }
    {
        uint8_t indexed_palette[4];
        memcpy(indexed_palette, &rom[0x0002C000 + 28 * 16 + 2 * 4], 4);
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 3; ++j)
                entity_palette[i * 3 + j] = colors[indexed_palette[i] * 3 + j];
    }

    // Create tracker spritesheet
    uint8_t* image_data = new uint8_t[(4 * 8) * (16 * 16 * 4)];
    memset(image_data, 0xFF, sizeof(image_data));

    bake((int)item_t::hand_dagger, image_data, palette, 0x4A, 0x4B, 0x8F, 0x4C);
    bake((int)item_t::small_shield, image_data, palette, 0x61, 0x62, 0x63, 0x64);
    bake((int)item_t::key_jack, image_data, palette, 0x73, 0x27, 0x74, 0x29);
    bake((int)item_t::deluge, image_data, palette, 0x7F, 0x80, 0x81, 0x82);
    bake((int)item_t::long_sword, image_data, palette, 0x47, 0x48, 0x8F, 0x49);
    bake((int)item_t::large_shield, image_data, palette, 0x65, 0x66, 0x67, 0x68);
    bake((int)item_t::key_queen, image_data, palette, 0x71, 0x27, 0x72, 0x29);
    bake((int)item_t::thunder, image_data, palette, 0x83, 0x84, 0x85, 0x86);
    bake((int)item_t::giant_blade, image_data, palette, 0x44, 0x45, 0x8F, 0x46);
    bake((int)item_t::magic_shield, image_data, palette, 0x69, 0x6A, 0x6B, 0x6C);
    bake((int)item_t::key_king, image_data, palette, 0x2A, 0x27, 0x2B, 0x29);
    bake((int)item_t::fire, image_data, palette, 0x87, 0x88, 0x89, 0x8A);
    bake((int)item_t::dragon_slayer, image_data, palette, 0x4D, 0x4E, 0x4F, 0x50);
    bake((int)item_t::battle_helmet, image_data, palette, 0x6D, 0x6E, 0x6F, 0x70);
    bake((int)item_t::key_ace, image_data, palette, 0x26, 0x27, 0x28, 0x29);
    bake((int)item_t::death, image_data, palette, 0x7B, 0x7C, 0x7D, 0x7E);
    bake((int)item_t::leather_armor, image_data, palette, 0x51, 0x52, 0x53, 0x54);
    bake((int)item_t::ring_of_elf, image_data, palette, 0x06, 0x07, 0x0C, 0x0D);
    bake((int)item_t::key_joker, image_data, palette, 0x75, 0x27, 0x76, 0x29);
    bake((int)item_t::tilte, image_data, palette, 0x77, 0x78, 0x79, 0x7A);
    bake((int)item_t::studded_mail, image_data, palette, 0x55, 0x56, 0x57, 0x58);
    bake((int)item_t::ring_of_ruby, image_data, palette, 0x08, 0x09, 0x0C, 0x0D);
    bake((int)item_t::mattock, image_data, palette, 0x00, 0x01, 0x02, 0x03);
    bake((int)item_t::black_onyx, image_data, palette, 0x3C, 0x3D, 0x3E, 0x3F);
    bake((int)item_t::full_plate, image_data, palette, 0x59, 0x5A, 0x5B, 0x5C);
    bake((int)item_t::ring_of_dworf, image_data, palette, 0x0A, 0x0B, 0x0C, 0x0D);
    bake((int)item_t::wingboots, image_data, palette, 0x22, 0x23, 0x24, 0x25);
    bake((int)item_t::magical_rod, image_data, palette, 0x0E, 0x0F, 0x10, 0x11);
    bake((int)item_t::battle_suit, image_data, palette, 0x5D, 0x5E, 0x5F, 0x60);
    bake((int)item_t::demons_ring, image_data, palette, 0x04, 0x05, 0x0C, 0x0D);
    bake((int)item_t::spring_elixir, image_data, palette, 0x34, 0x35, 0x36, 0x37, true);
    bake((int)item_t::pendant, image_data, palette, 0x38, 0x39, 0x3A, 0x3B);
    m_sprite_sheet = OTexture::createFromData(image_data, { 4 * 16, 8 * 16 }, false);

    bake_spring(0, image_data, entity_palette, 0x00, false);
    bake_spring(1, image_data, entity_palette, 0x01, false);
    bake_spring(2, image_data, entity_palette, 0x01, true);
    bake_spring(3, image_data, entity_palette, 0x00, true);
    bake_spring(4, image_data, entity_palette, 0x02, false);
    bake_spring(5, image_data, entity_palette, 0x03, false);
    bake_spring(6, image_data, entity_palette, 0x03, true);
    bake_spring(7, image_data, entity_palette, 0x02, true);
    bake_spring(8, image_data, entity_palette, 0x04, false);
    bake_spring(9, image_data, entity_palette, 0x05, false);
    bake_spring(10, image_data, entity_palette, 0x05, true);
    bake_spring(11, image_data, entity_palette, 0x04, true);
    m_spring_texture = OTexture::createFromData(image_data, { 4 * 8, 3 * 8 }, false);

    delete[] image_data;
}


void APTracker::bake(int dst_id, uint8_t* dst_image, uint8_t* palette, uint8_t tile0, uint8_t tile1, uint8_t tile2, uint8_t tile3, bool flip_green_red)
{
    int dst_x = (dst_id % 4) * 16;
    int dst_y = (dst_id / 4) * 16;

    const uint8_t* srcs[] = {
        m_rom + 0x00028500 + tile0 * 16,
        m_rom + 0x00028500 + tile1 * 16,
        m_rom + 0x00028500 + tile2 * 16,
        m_rom + 0x00028500 + tile3 * 16
    };

    for (int i = 0; i < 4; ++i)
    {
        auto src = srcs[i];
        int dst2_x = dst_x + (i % 2) * 8;
        int dst2_y = dst_y + (i / 2) * 8;

        for (int y = 0; y < 8; ++y)
        {
            int dst_y_k = (dst2_y + y) * (4 * 16 * 4);
            for (int x = 0; x < 8; ++x)
            {
                int inv_x = 7 - x;
                int b1 = (src[y] >> inv_x) & 1;
                int b2 = (src[y + 8] >> inv_x) & 1;
                int col = (b1) | (b2 << 1);

                if (flip_green_red)
                {
                    if (col == 1) col = 2;
                    else if (col == 2) col = 1;
                }

                int dst_k = dst_y_k + ((dst2_x + x) * 4);

                dst_image[dst_k + 0] = palette[col * 3 + 0];
                dst_image[dst_k + 1] = palette[col * 3 + 1];
                dst_image[dst_k + 2] = palette[col * 3 + 2];
                dst_image[dst_k + 3] = 255;
            }
        }
    }
}


void APTracker::bake_spring(int dst_id, uint8_t* dst_image, uint8_t* palette, uint8_t tile, bool flip_h)
{
    int dst_x = (dst_id % 4) * 8;
    int dst_y = (dst_id / 4) * 8;

    const uint8_t* src = m_rom + 0x0001CE36 + tile * 16;

    for (int y = 0; y < 8; ++y)
    {
        int dst_y_k = (dst_y + y) * (4 * 8 * 4);

        for (int x = 0; x < 8; ++x)
        {
            int inv_x = 7 - x;
            if (flip_h) inv_x = x;

            int b1 = (src[y] >> inv_x) & 1;
            int b2 = (src[y + 8] >> inv_x) & 1;
            int col = (b1) | (b2 << 1);

            int dst_k = dst_y_k + ((dst_x + x) * 4);

            dst_image[dst_k + 0] = palette[col * 3 + 0];
            dst_image[dst_k + 1] = palette[col * 3 + 1];
            dst_image[dst_k + 2] = palette[col * 3 + 2];
            dst_image[dst_k + 3] = 255;
        }
    }
}


void APTracker::load_colors(uint8_t* colors)
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


static int get_progressive_sword_level(const RAM* in_ram)
{
	const auto& ram = *in_ram;

	int level = 0;
	level += (int)ram[0x03C2]; // inv count
	if (ram[0x03BD] != 0xFF) level++; // Equipped

	return level;
}


static int get_progressive_armor_level(const RAM* in_ram)
{
	const auto& ram = *in_ram;

	int level = 0;
	level += (int)ram[0x03C3]; // inv count
	if (ram[0x03BE] != 0xFF) level++; // Equipped

	return level;
}


static int get_progressive_shield_level(const RAM* in_ram)
{
	const auto& ram = *in_ram;

	int level = 0;
	level += (int)ram[0x03C4]; // inv count
	if (ram[0x03BF] != 0xFF) level++; // Equipped

	return level;
}


void APTracker::set_item_tracked(uint8_t item_id)
{
    if (item_id == 0xFF || item_id == 0x00) return;

    switch (item_id)
    {
        case 0x60: m_states[(int)item_t::deluge] = true; break;
        case 0x61: m_states[(int)item_t::thunder] = true; break;
        case 0x62: m_states[(int)item_t::fire] = true; break;
        case 0x63: m_states[(int)item_t::death] = true; break;
        case 0x64: m_states[(int)item_t::tilte] = true; break;

        case 0x87: m_states[(int)item_t::key_jack] = true; break;
        case 0x86: m_states[(int)item_t::key_queen] = true; break;
        case 0x85: m_states[(int)item_t::key_king] = true; break;
        case 0x84: m_states[(int)item_t::key_ace] = true; break;
        case 0x88: m_states[(int)item_t::key_joker] = true; break;
        case 0x89: m_states[(int)item_t::mattock] = true; break;
        case AP_ITEM_SPRING_ELIXIR: m_states[(int)item_t::spring_elixir] = true; break;
    }
}


void APTracker::update(float dt)
{
    memset(m_states, 0, sizeof(m_states));

    const auto& ram = *m_ram;

	int sword_level = get_progressive_sword_level(m_ram);
	int armor_level = get_progressive_armor_level(m_ram);
	int shield_level = get_progressive_shield_level(m_ram);

    m_states[(int)item_t::hand_dagger] = sword_level >= 1;
    m_states[(int)item_t::long_sword] = sword_level >= 2;
    m_states[(int)item_t::giant_blade] = sword_level >= 3;
    m_states[(int)item_t::dragon_slayer] = sword_level >= 4;

    m_states[(int)item_t::leather_armor] = armor_level >= 1;
    m_states[(int)item_t::studded_mail] = armor_level >= 2;
    m_states[(int)item_t::full_plate] = armor_level >= 3;
    m_states[(int)item_t::battle_suit] = armor_level >= 4;

    m_states[(int)item_t::small_shield] = shield_level >= 1;
    m_states[(int)item_t::large_shield] = shield_level >= 2;
    m_states[(int)item_t::magic_shield] = shield_level >= 3;
    m_states[(int)item_t::battle_helmet] = shield_level >= 4;

    // Magic
    for (int i = 0, len = (int)ram[0x03C5]; i < len; ++i)
    {
        auto item_id = ram[0x03A9 + i] + 96;
        set_item_tracked(item_id);
    }
    if (ram[0x03C0] != 0xFF)
        set_item_tracked(ram[0x03C0] + 96);

    // Items
    for (int i = 0, len = (int)ram[0x03C6]; i < len; ++i)
    {
        auto item_id = ram[0x03AD + i] | 0x80;
        set_item_tracked(item_id);
    }
    set_item_tracked(ram[0x03C1] | 0x80);

    // Extras and rings
    m_states[(int)item_t::black_onyx] = ram[0x042C] & 0x01;
    m_states[(int)item_t::pendant] = ram[0x042C] & 0x02;
    m_states[(int)item_t::magical_rod] = ram[0x042C] & 0x04;
    m_states[(int)item_t::spring_elixir] |= m_ram->get(0x042D) & 0x01; // Check if spring is activated
    m_states[(int)item_t::demons_ring] = ram[0x042C] & 0x10;
    m_states[(int)item_t::ring_of_dworf] = ram[0x042C] & 0x20;
    m_states[(int)item_t::ring_of_ruby] = ram[0x042C] & 0x40;
    m_states[(int)item_t::ring_of_elf] = ram[0x042C] & 0x80;

    // Wingboots shop unlock
    m_states[(int)item_t::wingboots] = ram[0x819];
}


int APTracker::owns(int in_item_id) const
{
    const auto& ram = *m_ram;

    switch (in_item_id)
    {
        case AP_ITEM_PROGRESSIVE_SWORD:
            return get_progressive_sword_level(m_ram);
        case AP_ITEM_PROGRESSIVE_ARMOR:
            return get_progressive_armor_level(m_ram);
        case AP_ITEM_PROGRESSIVE_SHIELD:
            return get_progressive_shield_level(m_ram);

        // Magic
        case 0x60: // Deluge
        case 0x61: // Thunder
        case 0x62: // Fire
        case 0x63: // Death
        case 0x64: // Tilte
            for (int i = 0, len = (int)ram[0x03C5]; i < len; ++i)
            {
                auto item_id = ram[0x03A9 + i] + 96;
                if (item_id == 0xFF || item_id == 0x00) continue;
                if (item_id == in_item_id) return 1;
            }
            // Equipped
            if (ram[0x03C0] != 0xFF)
            {
                auto item_id = ram[0x03C0] + 96;
                if (item_id == 0xFF || item_id == 0x00) return 0;
                if (item_id == in_item_id) return 1;
            }
            return 0;

        // Unique items
        case 0x89: // Mattock
        case 0x87: // AP_ENTITY_KEY_JACK
        case 0x86: // AP_ENTITY_KEY_QUEEN
        case 0x85: // AP_ENTITY_KEY_KING
        case 0x88: // AP_ENTITY_KEY_JOKER
        case 0x84: // AP_ENTITY_KEY_ACE
            for (int i = 0, len = (int)ram[0x03C6]; i < len; ++i)
            {
                auto item_id = ram[0x03AD + i] | 0x80;
                if (item_id == 0xFF || item_id == 0x00) continue;
                if (item_id == in_item_id) return 1;
            }
            // Equipped
            {
                auto item_id = ram[0x03C1] | 0x80;
                if (item_id == 0xFF || item_id == 0x00) return 0;
                if (item_id == in_item_id) return 1;
            }
            return 0;

        // Extra and rings
        case 0x94: return ram[0x042C] & 0x01; // Black Onyx
        case 0x93: return ram[0x042C] & 0x02; // Pendant
        case 0x8A: return ram[0x042C] & 0x04; // Magical Rod
        case AP_ITEM_SPRING_ELIXIR: return m_ram->get(0x042D) & 0x01;
        case 0x83: return ram[0x042C] & 0x10; // Demons Ring
        case 0x82: return ram[0x042C] & 0x20; // Ring of Dworf
        case 0x81: return ram[0x042C] & 0x40; // Ring of Ruby
        case 0x80: return ram[0x042C] & 0x80; // Ring of Elf

        // Wingboots shop unlock
        case AP_ITEM_AP_WINGBOOTS: return ram[0x819];

        default:
            return 1; // We don't want to trigger it for any other type of items
    }
}


void APTracker::render()
{
    auto res = OScreenf;
    float scale = std::floor(res.y / (float)PPU::SCREEN_H);
    float scale2 = std::ceil(scale * 0.5f);

#if !SHOW_RAM
    int item_mouse_hovered = -1;
    Vector2 tooltip_item_pos;

    oSpriteBatch->begin();
    {
        float tracker_x = res.x * 0.5f + (float)PPU::SCREEN_W * 0.5f * scale + scale * 16.0f;
        float tracker_y = res.y * 0.5f - (float)(PPU::SCREEN_H - 16) * 0.5f * scale;
        const auto& screen_mouse_pos = OGetMousePos();

        // Background, grayed out
        oSpriteBatch->drawRect(m_sprite_sheet,
                               Rect(tracker_x, tracker_y, 4.0f * 16.0f * scale, 16.0f * 8.0f * scale + 24.0f * scale2),
                               Color::Black);
        oSpriteBatch->drawRect(m_sprite_sheet,
                               Rect(tracker_x, tracker_y, 4.0f * 16.0f * scale, 8.0f * 16.0f * scale),
                               Color(0.25f));

        // Light up owned items
        for (int i = 0; i < (int)item_t::COUNT; ++i)
        {
            int x = i % 4;
            int y = i / 4;
            float screen_x = tracker_x + (float)x * 16.0f * scale;
            float screen_y = tracker_y + (float)y * 16.0f * scale;
            if (Rect(screen_x, screen_y, 16.0f * scale, 16.0f * scale).Contains(screen_mouse_pos))
            {
                item_mouse_hovered = i;
                tooltip_item_pos.x = screen_x;
                tooltip_item_pos.y = screen_y;
            }

            if (!m_states[i]) continue;

            oSpriteBatch->drawRectWithUVs(m_sprite_sheet, 
                Rect(tracker_x + (float)x * 16.0f * scale, tracker_y + (float)y * 16.0f * scale,
                     16.0f * scale, 16.0f * scale),
                Vector4((float)x / 4.0f, (float)y / 8.0f,
                        (float)(x + 1) / 4.0f, (float)(y + 1) / 8.0f));
        }

        // Springs
        uint8_t bits = m_ram->get(0x042D);
        float spring_spacing = std::floor(((4.0f * 16.0f * scale) - (3.0f * 4.0f * 8.0f * scale2)) / 3.0f);
        for (int i = 0; i < 3; ++i)
        {
            float screen_x = tracker_x + (float)i * (4 * 8 * scale2 + spring_spacing);
            float screen_y = tracker_y + 8 * 16 * scale;
            if (Rect(screen_x, screen_y, 4 * 8 * scale2, 3 * 8 * scale2).Contains(screen_mouse_pos))
            {
                item_mouse_hovered = (int)item_t::COUNT + i;
                tooltip_item_pos.x = screen_x;
                tooltip_item_pos.y = screen_y;
            }
            oSpriteBatch->drawRect(m_spring_texture,
                                   Rect(tracker_x + (float)i * (4 * 8 * scale2 + spring_spacing), tracker_y + 8 * 16 * scale, 4 * 8 * scale2, 3 * 8 * scale2),
                                   (bits & (1 << i)) ? Color::White : Color(0.25f));
        }
    }
    oSpriteBatch->end();

    // Tool tip
    if (item_mouse_hovered != -1)
    {
        int len = (int)strlen(ITEM_NAMES[item_mouse_hovered]);
        int x = (int)(tooltip_item_pos.x / scale / 8) - (len + 5);
        int y = (int)(tooltip_item_pos.y / scale / 8);
        oSpriteBatch->begin(Matrix::CreateScale(scale));
        m_tile_drawer->draw_ui_frame(x, y, len + 4, 3);
        m_tile_drawer->draw_text(x + 2, y + 1, ITEM_NAMES[item_mouse_hovered]);
        oSpriteBatch->end();
    }
#endif

    // Titles and springs
    {
        float tracker_x = res.x * 0.5f - (float)PPU::SCREEN_W * 0.5f * scale;
        float tracker_y = res.y * 0.5f - (float)(PPU::SCREEN_H - 16) * 0.5f * scale;
        scale = std::ceil(scale * 0.5f);

        oSpriteBatch->begin(Matrix::CreateScale(scale) *
                            Matrix::CreateTranslation(tracker_x, tracker_y, 0.0f));

        // Black frame behind
        oSpriteBatch->drawRect(m_sprite_sheet,
                               Rect(-18 * 8, 0, 15 * 8, 33 * 8),
                               Color::Black);

        int current_title = (int)m_ram->get(0x0437);
        int x = -15;
        int y = 1;
        Color disabled_tint(0.25f, 0.25f, 0.25f, 1.0f);
        for (int i = 0; i <= 15; ++i)
        {
            char text[20];
            int len = (int)m_rom[0x00030943 + i * 16];
            text[len] = '\0';
            memcpy(text, m_rom + 0x00030944 + i * 16, len);

            m_tile_drawer->draw_text(x, y, text, (i <= current_title) ? Color::White : disabled_tint);

            y += 2;
        }

        m_tile_drawer->draw_cursor(-17, 1 + current_title * 2);
        oSpriteBatch->end();
    }
}
