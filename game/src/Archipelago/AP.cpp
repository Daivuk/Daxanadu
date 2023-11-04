#include "AP.h"
#include "APItems.h"
#include "APLocations.h"
#include "ExternalInterface.h"
#include "Patcher.h"
#include "version.h"

#include "Archipelago.h"

#include <onut/Files.h>
#include <onut/Maths.h>


static AP* g_ap = nullptr;


static std::string string_to_hex(const char* str)
{
    static const char hex_digits[] = "0123456789ABCDEF";

	std::string out;
	std::string in = str;

    out.reserve(in.length() * 2);
    for (unsigned char c : in)
    {
        out.push_back(hex_digits[c >> 4]);
        out.push_back(hex_digits[c & 15]);
    }

    return out;
}


static void f_itemclr()
{
	if (g_ap) g_ap->on_item_clear();
}


static void f_itemrecv(int64_t item_id, bool notify_player)
{
	if (g_ap) g_ap->on_item_received(item_id, notify_player);
}


static void f_locrecv(int64_t loc_id)
{
	if (g_ap) g_ap->on_location_received(loc_id);
}


static void f_locinfo(std::vector<AP_NetworkItem> loc_infos)
{
	if (g_ap) g_ap->on_location_info(loc_infos);
}


AP::AP(const ap_info_t& info)
    : m_info(info)
{
	g_ap = this;
}


AP::~AP()
{
	connection_success_delegate = nullptr;
	connection_failed_delegate = nullptr;
	AP_Shutdown();
	g_ap = nullptr;
}


void AP::connect()
{
	m_state = state_t::connecting;
	patch_items();

	AP_NetworkVersion version = {0, 4, 3};
	AP_SetClientVersion(&version);
    AP_Init(m_info.address.c_str(), "Faxanadu", m_info.slot_name.c_str(), m_info.password.c_str());
	AP_SetDeathLinkSupported(true);
	AP_SetItemClearCallback(f_itemclr);
	AP_SetItemRecvCallback(f_itemrecv);
	AP_SetLocationCheckedCallback(f_locrecv);
	AP_SetLocationInfoCallback(f_locinfo);
    AP_Start();
}


static void copy_sprite(uint8_t* src, uint8_t* dst, bool flip, bool invert_green_red, int shift = 0)
{
	if (!flip && !invert_green_red && shift == 0)
	{
		memcpy(dst, src, 16);
		return;
	}

	for (int y = 0; y < 8; ++y)
	{
		uint8_t src_plane0 = src[y];
		uint8_t src_plane1 = src[y + 8];

		uint8_t dst_plane0 = 0;
		uint8_t dst_plane1 = 0;

		for (int x = 0; x < 8; ++x)
		{
			auto b0 = (src_plane0 >> x) & 1;
			auto b1 = (src_plane1 >> x) & 1;
			if (invert_green_red)
			{
				auto tmp = b0;
				b0 = b1;
				b1 = tmp;
			}
			if (flip)
			{
				dst_plane0 |= b0 << (7 - x);
				dst_plane1 |= b1 << (7 - x);
			}
			else
			{
				dst_plane0 |= b0 << x;
				dst_plane1 |= b1 << x;
			}
		}

		if (shift >= 0)
		{
			dst[y] = (uint8_t)(((int)dst_plane0 >> shift) & 0xFF);
			dst[y + 8] = (uint8_t)(((int)dst_plane1 >> shift) & 0xFF);
		}
		else
		{
			dst[y] = (uint8_t)(((int)dst_plane0 << -shift) & 0xFF);
			dst[y + 8] = (uint8_t)(((int)dst_plane1 << -shift) & 0xFF);
		}
	}
}


void AP::patch_items()
{
#define BANK_ADDR_LO(bank, addr) (bank * 0x4000 + (addr - 0x8000))
#define ROM_LO(bank, addr) (m_info.rom + BANK_ADDR_LO(bank, addr))
#define ADD_ITEM(name, id, tile0, tile1, tile2, tile3) \
	memcpy(ROM_LO(12, 0xAE4D) + (id - 0x90) * 16 + 1, name, strlen(name)); \
	m_info.rom[0x0002B53C - 6 * 4 + (id - 0x90) * 4 + 0] = tile0; \
	m_info.rom[0x0002B53C - 6 * 4 + (id - 0x90) * 4 + 1] = tile1; \
	m_info.rom[0x0002B53C - 6 * 4 + (id - 0x90) * 4 + 2] = tile2; \
	m_info.rom[0x0002B53C - 6 * 4 + (id - 0x90) * 4 + 3] = tile3
#define TILE_ADDR(index) (m_info.rom + 0x00028500 + index * 16)
#define SPRITE_ADDR(rom_addr, index) (m_info.rom + rom_addr - 0x10 + index * 16)
#define LO(addr) (uint8_t)(addr & 0xFF)
#define HI(addr) (uint8_t)((addr >> 8) & 0xFF)

	// Replace Crystal with spring elixir
	copy_sprite(TILE_ADDR(0x34), TILE_ADDR(0x12), false, true);
	copy_sprite(TILE_ADDR(0x35), TILE_ADDR(0x13), false, true);
	copy_sprite(TILE_ADDR(0x36), TILE_ADDR(0x14), false, true);
	copy_sprite(TILE_ADDR(0x37), TILE_ADDR(0x15), false, true);

	// Replace Lamp with glove
	copy_sprite(SPRITE_ADDR(0x0001CD26, 0), TILE_ADDR(0x16), false, false, 4);
	copy_sprite(SPRITE_ADDR(0x0001CD26, 0), TILE_ADDR(0x17), false, false, -4);
	copy_sprite(SPRITE_ADDR(0x0001CD26, 1), TILE_ADDR(0x18), false, false, 4);
	copy_sprite(SPRITE_ADDR(0x0001CD26, 1), TILE_ADDR(0x19), false, false, -4);

	// Replace fire crystal tiles with ointment
	copy_sprite(SPRITE_ADDR(0x0001CE06, 0), TILE_ADDR(0x40), false, false);
	copy_sprite(SPRITE_ADDR(0x0001CE06, 0), TILE_ADDR(0x41), true, false);
	copy_sprite(SPRITE_ADDR(0x0001CE06, 1), TILE_ADDR(0x42), false, false);
	copy_sprite(SPRITE_ADDR(0x0001CE06, 1), TILE_ADDR(0x43), true, false);

	// Replace battle helmet sprites for Magic Shield tiles
	copy_sprite(TILE_ADDR(0x69), SPRITE_ADDR(0x0001CFC6, 0), false, false);
	copy_sprite(TILE_ADDR(0x6A), SPRITE_ADDR(0x0001CFC6, 1), false, false);
	copy_sprite(TILE_ADDR(0x6B), SPRITE_ADDR(0x0001CFC6, 2), false, false);
	copy_sprite(TILE_ADDR(0x6C), SPRITE_ADDR(0x0001CFC6, 3), false, false);

	// Replace snake monster (unused) with ring tiles
	copy_sprite(TILE_ADDR(0x06), SPRITE_ADDR(0x00018C32, 0), false, true); // Ring of elf cap
	copy_sprite(TILE_ADDR(0x07), SPRITE_ADDR(0x00018C32, 1), false, true);
	copy_sprite(TILE_ADDR(0x08), SPRITE_ADDR(0x00018C32, 2), false, true); // Ruby ring cap
	copy_sprite(TILE_ADDR(0x09), SPRITE_ADDR(0x00018C32, 3), false, true);
	copy_sprite(TILE_ADDR(0x0A), SPRITE_ADDR(0x00018C32, 4), false, false); // Ring of dworf cap
	copy_sprite(TILE_ADDR(0x0B), SPRITE_ADDR(0x00018C32, 5), false, false);
	copy_sprite(TILE_ADDR(0x04), SPRITE_ADDR(0x00018C32, 6), false, false); // Demons ring cap
	copy_sprite(TILE_ADDR(0x05), SPRITE_ADDR(0x00018C32, 7), false, false);
	copy_sprite(TILE_ADDR(0x0C), SPRITE_ADDR(0x00018C32, 8), false, false); // Loop
	copy_sprite(TILE_ADDR(0x0D), SPRITE_ADDR(0x00018C32, 9), false, false);

	// Replace unused night monster sprites with key tiles
	copy_sprite(TILE_ADDR(0x26), SPRITE_ADDR(0x00019442, 0), false, false); // A top
	copy_sprite(TILE_ADDR(0x27), SPRITE_ADDR(0x00019442, 1), false, false); // Dents top
	copy_sprite(TILE_ADDR(0x28), SPRITE_ADDR(0x00019442, 2), false, false); // A bottom
	copy_sprite(TILE_ADDR(0x29), SPRITE_ADDR(0x00019442, 3), false, false); // Dents bottom
	copy_sprite(TILE_ADDR(0x2A), SPRITE_ADDR(0x00019442, 4), false, false); // K bottom
	copy_sprite(TILE_ADDR(0x2B), SPRITE_ADDR(0x00019442, 5), false, false); // K top
	copy_sprite(TILE_ADDR(0x71), SPRITE_ADDR(0x00019442, 6), false, false); // Q top
	copy_sprite(TILE_ADDR(0x72), SPRITE_ADDR(0x00019442, 7), false, false); // Q bottom
	copy_sprite(TILE_ADDR(0x73), SPRITE_ADDR(0x00019442, 8), false, false); // J top
	copy_sprite(TILE_ADDR(0x74), SPRITE_ADDR(0x00019442, 9), false, false); // J bottom
	copy_sprite(TILE_ADDR(0x75), SPRITE_ADDR(0x00019442, 10), false, false); // Jo top
	copy_sprite(TILE_ADDR(0x76), SPRITE_ADDR(0x00019442, 11), false, false); // Jo bottom

	// Replace rest of unused night monster sprites with spring elixir tiles
	copy_sprite(TILE_ADDR(0x34), SPRITE_ADDR(0x00019442, 12), false, true);
	copy_sprite(TILE_ADDR(0x35), SPRITE_ADDR(0x00019442, 13), false, true);
	copy_sprite(TILE_ADDR(0x36), SPRITE_ADDR(0x00019442, 14), false, true);
	copy_sprite(TILE_ADDR(0x37), SPRITE_ADDR(0x00019442, 15), false, true);

	// Rename battle helmet to progressive shield in the popup dialog
	memcpy(ROM_LO(13, 0xB243), "I've""\xfd""got""\xfe""Progressive""\xfe""Shield", 27);
	memcpy(ROM_LO(13, 0xB25F), "I've""\xfd""got""\xfe""Progressive""\xfe""Sword.", 27);
	memcpy(ROM_LO(13, 0xB229), "Ive""\xfd""got""\xfe""Progressive""\xfe""Armor", 25);

	// World items
	{
		// Progressive sword (We're lucky this code fits perfectly to replace previous one)
		m_info.patcher->patch(15, 0xC737, 0, {
			OP_LDA_ABS(0x100),
			OP_PHA(),
			OP_LDX_IMM(12),
			OP_JSR(0xCC1A), // Switch bank
			OP_LDA_IMM(AP_ITEM_PROGRESSIVE_SWORD),
			OP_JSR(0x9AF7), // Give item (Usually called by dialogs)
			OP_PLA(),
			OP_TAX(),
			OP_JMP_ABS(0xCC1A), // Switch bank
		});

		// Progressive shield (We're lucky this code fits perfectly to replace previous one)
		m_info.patcher->patch(15, 0xC717, 0, {
			OP_LDA_ABS(0x100),
			OP_PHA(),
			OP_LDX_IMM(12),
			OP_JSR(0xCC1A), // Switch bank
			OP_LDA_IMM(AP_ITEM_PROGRESSIVE_SHIELD),
			OP_JSR(0x9AF7), // Give item (Usually called by dialogs)
			OP_PLA(),
			OP_TAX(),
			OP_JMP_ABS(0xCC1A), // Switch bank
		});

		// Progressive armor (We're lucky this code fits perfectly to replace previous one)
		m_info.patcher->patch(15, 0xC6F7, 0, {
			OP_LDA_ABS(0x100),
			OP_PHA(),
			OP_LDX_IMM(12),
			OP_JSR(0xCC1A), // Switch bank
			OP_LDA_IMM(AP_ITEM_PROGRESSIVE_ARMOR),
			OP_JSR(0x9AF7), // Give item (Usually called by dialogs)
			OP_PLA(),
			OP_TAX(),
			OP_JMP_ABS(0xCC1A), // Switch bank
		});

		// Start at ID 0x98
		auto key_jack_dialog = m_info.patcher->patch_new_code(12, { 0x00, 0x01, 0xC4, 0x00 });
		auto key_queen_dialog = m_info.patcher->patch_new_code(12, { 0x00, 0x01, 0xC5, 0x00 });
		auto key_king_dialog = m_info.patcher->patch_new_code(12, { 0x00, 0x01, 0xC6, 0x00 });
		auto key_ace_dialog = m_info.patcher->patch_new_code(12, { 0x00, 0x01, 0xC8, 0x00 });
		auto key_joker_dialog = m_info.patcher->patch_new_code(12, { 0x00, 0x01, 0xC7, 0x00 });
		auto ruby_ring_dialog = m_info.patcher->patch_new_code(12, { 0x00, 0x01, 0xC9, 0x00 });
		auto dworf_ring_dialog = m_info.patcher->patch_new_code(12, { 0x00, 0x01, 0xCA, 0x00 });
		auto demons_ring_dialog = m_info.patcher->patch_new_code(12, { 0x00, 0x01, 0xCB, 0x00 });
		auto elf_ring_dialog = m_info.patcher->patch_new_code(12, { 0x00, 0x01, 0xCC, 0x00 });
		auto deluge_dialog = m_info.patcher->patch_new_code(12, { 0x00, 0x01, 0xCD, 0x00 });
		auto thunder_dialog = m_info.patcher->patch_new_code(12, { 0x00, 0x01, 0xCE, 0x00 });
		auto fire_dialog = m_info.patcher->patch_new_code(12, { 0x00, 0x01, 0xCF, 0x00 });
		auto death_dialog = m_info.patcher->patch_new_code(12, { 0x00, 0x01, 0xD0, 0x00 });
		auto tilte_dialog = m_info.patcher->patch_new_code(12, { 0x00, 0x01, 0xD1, 0x00 });
		auto spring_elixir_dialog = m_info.patcher->patch_new_code(12, { 0x00, 0x01, 0xD2, 0x00 });

		// Table continue
		auto dialog_lo = m_info.patcher->patch_new_code(12, {
			LO(key_jack_dialog),
			LO(key_queen_dialog),
			LO(key_king_dialog),
			LO(key_joker_dialog),
			LO(key_ace_dialog),
			LO(ruby_ring_dialog),
			LO(dworf_ring_dialog),
			LO(demons_ring_dialog),
			LO(elf_ring_dialog),
			LO(deluge_dialog),
			LO(thunder_dialog),
			LO(fire_dialog),
			LO(death_dialog),
			LO(tilte_dialog),
			LO(spring_elixir_dialog),
		});
		auto dialog_hi = m_info.patcher->patch_new_code(12, {
			HI(key_jack_dialog),
			HI(key_queen_dialog),
			HI(key_king_dialog),
			HI(key_joker_dialog),
			HI(key_ace_dialog),
			HI(ruby_ring_dialog),
			HI(dworf_ring_dialog),
			HI(demons_ring_dialog),
			HI(elf_ring_dialog),
			HI(deluge_dialog),
			HI(thunder_dialog),
			HI(fire_dialog),
			HI(death_dialog),
			HI(tilte_dialog),
			HI(spring_elixir_dialog),
		});

		// We want to add dialogs for the new items. They are tightly packed into the
		// bank 12. Modify the code to jump ahead further if message ID is too big
		auto get_dialog_addr_addr = m_info.patcher->patch_new_code(12, {
			OP_SEC(),
			OP_SBC_IMM(0x98),
			OP_BMI(12),

			// Greater or equal to 152, we go somewhere else
			OP_TAX(),
			OP_LDA_ABSX(dialog_lo), // lo
			OP_STA_ZPG(0xDB),
			OP_LDA_ABSX(dialog_hi), // hi
			OP_STA_ZPG(0xDC),
			OP_RTS(),

			// Normal code
			OP_LDA_ABSX(0x9F6B), // lo
			OP_STA_ZPG(0xDB),
			OP_LDA_ABSX(0xA003), // hi
			OP_STA_ZPG(0xDC),
			OP_RTS(),
		});

		m_info.patcher->patch(12, 0x824C, 0, {
			OP_JSR(get_dialog_addr_addr),
			OP_NOP(), OP_NOP(),
			OP_NOP(), OP_NOP(), OP_NOP(),
			OP_NOP(), OP_NOP(),
		});

		// Ring of Ruby (Replaces unused snake monster)

#define REPLACE_ENTITY_WITH_ITEM(entity_id, sprite_sheet, sprite0, sprite1, sprite2, sprite3, pal) \
		{ \
			ROM_LO(14, 0xB407)[entity_id * 2 + 0] = 0x10; /* 16x16 pixels */ \
			ROM_LO(14, 0xB407)[entity_id * 2 + 1] = 0x10; \
			ROM_LO(14, 0xB4DF)[entity_id] = 0; /* 16x16 pixels */ \
			ROM_LO(14, 0xB544)[entity_id] = 5; /* Item type */ \
			ROM_LO(14, 0xB5A9)[entity_id] = 0; /* No hit points */ \
			ROM_LO(14, 0xB60E)[entity_id] = 0; /* No xp */ \
			ROM_LO(14, 0xB672)[entity_id] = 0xFF; /* No reward */ \
			ROM_LO(14, 0xB6D7)[entity_id] = 0; /* No damage */ \
			ROM_LO(14, 0xB73B)[entity_id] = 0xFC; /* This is probably a mask */ \
			ROM_LO(14, 0xAD2D)[entity_id * 2 + 0] = 0x4F; /* Behaviour (copy from Magical Rod) */ \
			ROM_LO(14, 0xAD2D)[entity_id * 2 + 1] = 0xB2; \
			ROM_LO(14, 0x8087)[entity_id * 2 + 0] = 0x4A; /* Entity update function (copy from Magical Rod) */ \
			ROM_LO(14, 0x8087)[entity_id * 2 + 1] = 0xA3; \
			ROM_LO(6, 0x8002)[entity_id * 2 + 0] = LO(sprite_sheet); \
			ROM_LO(6, 0x8002)[entity_id * 2 + 1] = HI(sprite_sheet); \
			int phase_index = ROM_LO(14, 0x8C9F)[entity_id]; \
			int frames_addr = ROM_LO(7, 0x9036)[phase_index * 2 + 0]; \
			frames_addr |= ROM_LO(7, 0x9036)[phase_index * 2 + 1] << 8; \
			frames_addr += 0x8000; \
			ROM_LO(7, frames_addr)[0] = 0x11; /* 2x2 */ \
			ROM_LO(7, frames_addr)[1] = 0x00; /* x offset */ \
			ROM_LO(7, frames_addr)[2] = 0x00; /* y offset */ \
			ROM_LO(7, frames_addr)[3] = 0x08; /* unknown, other items use 8 */ \
			ROM_LO(7, frames_addr)[4] = sprite0; /* Sprite ID */ \
			ROM_LO(7, frames_addr)[5] = pal; /* Palette */ \
			ROM_LO(7, frames_addr)[6] = sprite1; /* Sprite ID */ \
			ROM_LO(7, frames_addr)[7] = pal; /* Palette */ \
			ROM_LO(7, frames_addr)[8] = sprite2; /* Sprite ID */ \
			ROM_LO(7, frames_addr)[9] = pal; /* Palette */ \
			ROM_LO(7, frames_addr)[10] = sprite3; /* Sprite ID */ \
			ROM_LO(7, frames_addr)[11] = pal; /* Palette */ \
		}

		REPLACE_ENTITY_WITH_ITEM(AP_ENTITY_RING_OF_ELF, 0x0C22, 0, 1, 8, 9, 0);
		REPLACE_ENTITY_WITH_ITEM(AP_ENTITY_RING_OF_RUBY, 0x0C22, 2, 3, 8, 9, 0);
		REPLACE_ENTITY_WITH_ITEM(AP_ENTITY_RING_OF_DWORF, 0x0C22, 4, 5, 8, 9, 2);
		REPLACE_ENTITY_WITH_ITEM(AP_ENTITY_DEMONS_RING, 0x0C22, 6, 7, 8, 9, 1);

		//REPLACE_ENTITY_WITH_ITEM(AP_ENTITY_KEY_JACK, 0x0C22, 8, 1, 9, 3, 1); // Ignore for now
		REPLACE_ENTITY_WITH_ITEM(AP_ENTITY_KEY_QUEEN, 0x1432, 6, 1, 7, 3, 1);
		REPLACE_ENTITY_WITH_ITEM(AP_ENTITY_KEY_KING, 0x1432, 4, 1, 5, 3, 1);
		REPLACE_ENTITY_WITH_ITEM(AP_ENTITY_KEY_ACE, 0x1432, 0, 1, 2, 3, 1);
		REPLACE_ENTITY_WITH_ITEM(AP_ENTITY_KEY_JOKER, 0x1432, 10, 1, 11, 3, 1);

		REPLACE_ENTITY_WITH_ITEM(AP_ENTITY_SPRING_ELIXIR, 0x1432, 12, 13, 14, 15, 2);

#define TOUCHED_RING(dialog_id, item_mask) m_info.patcher->patch_new_code(15, { \
			OP_LDA_IMM(dialog_id), \
			OP_JSR(0xF859), \
			0x0C, 0x41, 0x82, \
			OP_LDA_IMM(0x08), \
			OP_JSR(0xD0E4), \
			OP_LDA_ABS(0x042C), \
			OP_ORA_IMM(item_mask), \
			OP_STA_ABS(0x042C), \
			OP_RTS(), \
		})

		// We're wasting a lot of precious space here...
#define TOUCHED_ITEM(dialog_id, item_id) m_info.patcher->patch_new_code(15, { \
			OP_TXA(), \
			OP_PHA(), \
			OP_LDA_IMM(dialog_id), \
			OP_JSR(0xF859), \
			0x0C, 0x41, 0x82, \
			OP_LDA_IMM(0x08), /* Play Sound */ \
			OP_JSR(0xD0E4), \
			OP_LDA_ABS(0x100), /* current bank */ \
			OP_PHA(), \
			OP_LDX_IMM(12), \
			OP_JSR(0xCC1A), /* Switch bank */ \
			OP_LDA_IMM(item_id), \
			OP_JSR(0x9AF7), /* Give item */ \
			OP_PLA(), \
			OP_TAX(), \
			OP_JSR(0xCC1A), /* Switch bank */ \
			OP_PLA(), \
			OP_TAX(), \
			OP_RTS(), \
		})

		auto touched_elf_ring_addr = TOUCHED_RING(0xA0, 0x80);
		auto touched_ruby_ring_addr = TOUCHED_RING(0x9D, 0x40);
		auto touched_dworf_ring_addr = TOUCHED_RING(0x9E, 0x20);
		auto touched_demons_ring_addr = TOUCHED_RING(0x9F, 0x10);

		auto touched_key_jack_addr = TOUCHED_ITEM(0x98, 0x87);
		auto touched_key_queen_addr = TOUCHED_ITEM(0x99, 0x86);
		auto touched_key_king_addr = TOUCHED_ITEM(0x9A, 0x85);
		auto touched_key_ace_addr = TOUCHED_ITEM(0x9B, 0x84);
		auto touched_key_joker_addr = TOUCHED_ITEM(0x9C, 0x88);

		auto touched_spring_elixir_addr = TOUCHED_ITEM(0xA6, AP_ITEM_SPRING_ELIXIR);

		auto touched_item_addr = m_info.patcher->patch_new_code(15, {
			OP_CMP_IMM(0x57), // Magical Rod
			OP_BNE(3),
			OP_JMP_ABS(0xC810), // Magical rod pickup code

			// Rings
			OP_CMP_IMM(AP_ENTITY_RING_OF_ELF), OP_BNE(3), OP_JMP_ABS(touched_elf_ring_addr),
			OP_CMP_IMM(AP_ENTITY_RING_OF_RUBY), OP_BNE(3), OP_JMP_ABS(touched_ruby_ring_addr),
			OP_CMP_IMM(AP_ENTITY_RING_OF_DWORF), OP_BNE(3), OP_JMP_ABS(touched_dworf_ring_addr),
			OP_CMP_IMM(AP_ENTITY_DEMONS_RING), OP_BNE(3), OP_JMP_ABS(touched_demons_ring_addr),

			// Keys
			OP_CMP_IMM(AP_ENTITY_KEY_JACK), OP_BNE(3), OP_JMP_ABS(touched_key_jack_addr),
			OP_CMP_IMM(AP_ENTITY_KEY_QUEEN), OP_BNE(3), OP_JMP_ABS(touched_key_queen_addr),
			OP_CMP_IMM(AP_ENTITY_KEY_KING), OP_BNE(3), OP_JMP_ABS(touched_key_king_addr),
			OP_CMP_IMM(AP_ENTITY_KEY_ACE), OP_BNE(3), OP_JMP_ABS(touched_key_ace_addr),
			OP_CMP_IMM(AP_ENTITY_KEY_JOKER), OP_BNE(3), OP_JMP_ABS(touched_key_joker_addr),

			// Magics

			// Spring Elixir
			OP_CMP_IMM(AP_ENTITY_SPRING_ELIXIR), OP_BNE(3), OP_JMP_ABS(touched_spring_elixir_addr),
			
			OP_JMP_ABS(0xC76F), // Go back
		});

		m_info.patcher->patch(15, 0xC768, 0, {
			OP_JMP_ABS(touched_item_addr),
		});
	}

	// Prepare space in unused area for all the new item texts.
	// Each location will have item name hardcoded + slot name.
	for (int i = 0; i < 16 + (sizeof(AP_LOCATIONS) / sizeof(ap_location_t) * 2); ++i)
	{
		ROM_LO(12, 0xAE4D)[i * 16] = 0x0D;
		for (int j = 1; j < 16; ++j)
		{
			ROM_LO(12, 0xAE4D)[i * 16 + j] = 0x20;
		}
	}

	// Move old texts from second page of items into a new addr with empty space so we can jump higher up and use this space instead
	memcpy(ROM_LO(12, 0xAE4D), ROM_LO(12, 0x9D4D), 16 * 6);

	// Add new items (There are unused items in the game, but we will avoid them incase they might trigger buffs/nerfs we don't know about)
	ADD_ITEM("PROG SWORD", AP_ITEM_PROGRESSIVE_SWORD, 0x4D, 0x4E, 0x4F, 0x50);
	ADD_ITEM("PROG ARMOR", AP_ITEM_PROGRESSIVE_ARMOR, 0x5D, 0x5E, 0x5F, 0x60);
	ADD_ITEM("PROG SHIELD", AP_ITEM_PROGRESSIVE_SHIELD, 0x69, 0x6A, 0x6B, 0x6C);
	ADD_ITEM("POISON", AP_ITEM_POISON, 0x30, 0x31, 0x32, 0x33);
	ADD_ITEM("OINTMENT", AP_ITEM_OINTMENT, 0x40, 0x41, 0x42, 0x43);
	ADD_ITEM("GLOVE", AP_ITEM_GLOVE, 0x16, 0x17, 0x18, 0x19);
	ADD_ITEM("SPRING ELIXIR", AP_ITEM_SPRING_ELIXIR, 0x12, 0x13, 0x14, 0x15);

	// Write new code in bank12 that allows to jump further to index the new text
	{
		auto addr = m_info.patcher->patch_new_code(12, {
			OP_BCC(4),

			OP_LDY_IMM(0xAE),
			OP_STY_ZPG(0xED),

			OP_TAY(),
			OP_JMP_ABS(0x8E9B),
		});

		m_info.patcher->patch(12, 0x8C50, 0, {
			OP_JMP_ABS(addr),
		});
	}

	// 15:C8CD ; Description: Stores an item in the next free slot in the item directory

	// Create a function that checks what inventory item is being added.
	// So we can change it to something else, like if we are adding progressive sword, add
	// the next sword instead.
	{
		auto choose_prog_sword = m_info.patcher->patch_new_code(12, {
			OP_LDA_IMM(0), // Initialize A to zero
			OP_LDY_ABS(0x03BD), // Active weapon
			OP_BMI(2), // If active weapon is FF, skip
			OP_LDA_IMM(1), // Active weapon is set, so set A to 1
			OP_CLC(),
			OP_ADC_ABS(0x03C2), // Add number of weapons to A
			OP_RTS(), // A is now the item id to add
		});

		auto choose_prog_shield = m_info.patcher->patch_new_code(12, {
			OP_LDA_IMM(0), // Initialize A to zero
			OP_LDY_ABS(0x03BF), // Active shield
			OP_BMI(2), // If active shield is FF, skip
			OP_LDA_IMM(1), // Active shield is set, so set A to 1
			OP_CLC(),
			OP_ADC_ABS(0x03C4), // Add number of shields to A
			OP_CLC(),
			OP_ADC_IMM(0x40), // shields IDs start at 64
			OP_RTS(), // A is now the item id to add
		});

		auto choose_prog_armor = m_info.patcher->patch_new_code(12, {
			OP_LDA_IMM(0), // Initialize A to zero
			OP_LDY_ABS(0x03BE), // Active armor
			OP_BMI(2), // If active armor is FF, skip
			OP_LDA_IMM(1), // Active armor is set, so set A to 1
			OP_CLC(),
			OP_ADC_ABS(0x03C3), // Add number of armors to A
			OP_CLC(),
			OP_ADC_IMM(0x20), // armors IDs start at 32
			OP_RTS(), // A is now the item id to add
		});

		auto give_item = m_info.patcher->patch_new_code(12, {
			// Check if progression sword
			OP_CMP_IMM(AP_ITEM_PROGRESSIVE_SWORD),
			OP_BNE(3),
			OP_JSR(choose_prog_sword),

			// Check if progressive armor
			OP_CMP_IMM(AP_ITEM_PROGRESSIVE_ARMOR),
			OP_BNE(3),
			OP_JSR(choose_prog_armor),

			// Check if progressive shield
			OP_CMP_IMM(AP_ITEM_PROGRESSIVE_SHIELD),
			OP_BNE(3),
			OP_JSR(choose_prog_shield),

			// Store final result
			OP_STA_ZPG(0xEE),
			OP_JSR(0xF785),
			OP_TAX(),
			OP_RTS(),
		});

		m_info.patcher->patch(12, 0x9B01, 0, {
			OP_JSR(give_item),
			OP_NOP(),
			OP_NOP(),
		});
	}

	// Spring elixir quest
	{
		m_info.patcher->patch(12, 0xA1B3, 0, { AP_ITEM_SPRING_ELIXIR }); // Check if has
		m_info.patcher->patch(12, 0xA1BC, 0, { AP_ITEM_SPRING_ELIXIR }); // Give

		// Remove item, but checks first if it's not selected, and remove it from selected
		auto remove_item = m_info.patcher->patch_new_code(12, {
			// Check if the item we remove is selected item
			OP_PHA(),
			OP_AND_IMM(0x1F),
			OP_CMP_ABS(0x03C1),
			OP_BEQ(4),

			// Normal remove item function
			OP_PLA(),
			OP_JMP_ABS(0x9A6A),

			// Clears selected item
			OP_PLA(),
			OP_JMP_ABS(0xC4BF), // Removes equip item
		});
		m_info.patcher->patch(12, 0x865A, 1, { PATCH_ADDR(remove_item) });
	}

	// Don't remove keys when used
	m_info.patcher->patch(15, 0xEBD9, 0, {
		OP_NOP(), OP_NOP(),
		OP_NOP(), OP_NOP(), OP_NOP(),
		OP_NOP(), OP_NOP(), OP_NOP(),
	});

	// Don't remove mattock when used
	m_info.patcher->patch(15, 0xC64C, 0, {
		OP_NOP(), OP_NOP(), OP_NOP(),
	});

	// Wingboots
	{
		// Don't remove wing boots when used,
		// but remove them from selected item and put them back into inventory.
		m_info.patcher->patch(15, 0xC581, 0, {
			OP_NOP(), OP_NOP(), OP_NOP(),
		});

		// Trigger a reuse timeout of 2mins
	}

	// Item inventory limit to 10
	{
		// Persistent inventory items
		//   Key J
		//   Key Jo
		//   Key Q
		//   Key K
		//   Key A
		//   Mattock
		//   Spring Elixir
		//   Wingboots

		// Non persistent
		//   Potion
		//   Hourglass

		// Max number of item
		m_info.patcher->patch(12, 0x847D, 4, { 10 });

		// Check at store
		//m_info.patcher->patch(12, 0x846C, 1, { 6 }); // Where it displays "you cant carry more"
		m_info.patcher->patch(12, 0x8405, 0, { OP_JMP_ABS(0x8439) });

		// Add to inventory
		m_info.patcher->patch(15, 0xC8D0, 1, { 10 });

		// Make item inventory window bigger
		m_info.patcher->patch(12, 0x8AA2, 1, { 0x08 }); // Inventory screen y position
		m_info.patcher->patch(12, 0x8AFF, 1, { 0x06 }); // y position of items dialog
		m_info.patcher->patch(12, 0x8B09, 1, { 0x16 }); // height of items dialog

		// Change the redraw area after it's closed
	}

	// Potion and Hourglass can stack (No limit. well... 255)
}


void AP::on_item_clear()
{
}


void AP::on_item_received(int64_t item_id, bool notify_player)
{
}


void AP::on_location_received(int64_t loc_id)
{
}


void AP::on_location_info(const std::vector<AP_NetworkItem>& loc_infos)
{
	for (const auto& loc_info : loc_infos)
	{
		ap_location_scout_t scout_loc;
		scout_loc.flags = loc_info.flags;
		scout_loc.item = loc_info.item;
		scout_loc.item_name = loc_info.item_name;
		scout_loc.player = loc_info.player;
		scout_loc.player_name = loc_info.player_name;
		for (auto& ap_location : AP_LOCATIONS)
		{
			if (ap_location.id == loc_info.location)
			{
				scout_loc.loc = &ap_location;
				break;
			}
		}
		if (!scout_loc.loc)
		{
			// Show some error or something
			__debugbreak();
		}
		m_location_scouts.push_back(scout_loc);
	}
}


void AP::load_state()
{
}


void AP::save_state()
{
}


void AP::update(float dt)
{
	switch (m_state)
	{
		case state_t::connecting:
		{
			switch (AP_GetConnectionStatus())
			{
				case AP_ConnectionStatus::Authenticated:
				{
					m_state = state_t::connected;
					
					// Log room info
					AP_RoomInfo ap_room_info;
					AP_GetRoomInfo(&ap_room_info);

					printf("Room Info:\n");
					printf("  Network Version: %i.%i.%i\n", ap_room_info.version.major, ap_room_info.version.minor, ap_room_info.version.build);
					printf("  Tags:\n");
					for (const auto& tag : ap_room_info.tags)
						printf("    %s\n", tag.c_str());
					printf("  Password required: %s\n", ap_room_info.password_required ? "true" : "false");
					printf("  Permissions:\n");
					for (const auto& permission : ap_room_info.permissions)
						printf("    %s = %i:\n", permission.first.c_str(), permission.second);
					printf("  Hint cost: %i\n", ap_room_info.hint_cost);
					printf("  Location check points: %i\n", ap_room_info.location_check_points);
					printf("  Data package version: %i\n", ap_room_info.datapackage_version);
					printf("  Data package versions: %i\n", ap_room_info.datapackage_version);
					for (const auto& datapackage_version : ap_room_info.datapackage_versions)
						printf("    %s = %i:\n", datapackage_version.first.c_str(), datapackage_version.second);
					printf("  Seed name: %s\n", ap_room_info.seed_name.c_str());
					printf("  Time: %f\n", ap_room_info.time);

					// Create a directory where saves will go for this AP seed.
					m_save_dir_name = "AP_" + ap_room_info.seed_name + "_" + string_to_hex(m_info.slot_name.c_str());
					printf("Save directory: %s\n", m_save_dir_name.c_str());
					if (!onut::fileExists(m_save_dir_name))
					{
						printf("  Doesn't exist, creating...\n");
						onut::createFolder(m_save_dir_name);
					}

					load_state();

					// Scout locations
					m_state = state_t::scouting;
					std::vector<int64_t> location_scouts;
					for (const auto& location : AP_LOCATIONS)
					{
						location_scouts.push_back(location.id);
					}
					printf("APDOOM: Scouting for %i locations...\n", (int)location_scouts.size());
					AP_SendLocationScouts(location_scouts, 0);
					break;
				}
				case AP_ConnectionStatus::ConnectionRefused:
				{
					m_state = state_t::idle;
					if (connection_failed_delegate) connection_failed_delegate();
					break;
				}
			}
			break;
		}
		case state_t::scouting:
		{
			if (m_location_scouts.size() == sizeof(AP_LOCATIONS) / sizeof(ap_location_t))
			{
				m_state = state_t::connected;
				patch_locations();
				if (connection_success_delegate) connection_success_delegate();
			}
			break;
		}
		case state_t::connected:
		{
			switch (AP_GetConnectionStatus())
			{
				case AP_ConnectionStatus::Disconnected:
				{
					if (m_state == state_t::connected)
					{
						__debugbreak();
					}
					break;
				}
			}

			while (AP_IsMessagePending())
			{
				AP_Message* msg = AP_GetLatestMessage();
				
				std::string colored_msg;

				switch (msg->type)
				{
					case AP_MessageType::ItemSend:
					{
						AP_ItemSendMessage* o_msg = static_cast<AP_ItemSendMessage*>(msg);
						colored_msg = "^2" + o_msg->item + "^3 was sent to ^1" + o_msg->recvPlayer;
						break;
					}
					case AP_MessageType::ItemRecv:
					{
						AP_ItemRecvMessage* o_msg = static_cast<AP_ItemRecvMessage*>(msg);
						colored_msg = "^3Received ^2" + o_msg->item + "^3 from ^1" + o_msg->sendPlayer;
						break;
					}
					case AP_MessageType::Hint:
					{
						AP_HintMessage* o_msg = static_cast<AP_HintMessage*>(msg);
						colored_msg = "^2" + o_msg->item + "^3 from ^1" + o_msg->sendPlayer + "^3 to ^1" + o_msg->recvPlayer + "^3 at ^2" + o_msg->location + (o_msg->checked ? " (Checked)" : " (Unchecked)");
						break;
					}
					default:
					{
						colored_msg = "^3" + msg->text;
						break;
					}
				}

				printf("AP Message: %s\n", msg->text.c_str());

				AP_ClearLatestMessage();
			}
			break;
		}
	}
}


void AP::patch_locations()
{
	for (const auto& scout : m_location_scouts)
	{
		//if (scout.loc->id == 400192)
		//{
		//	__debugbreak();
		//}

		ap_item_t* ap_item = nullptr;
		for (auto& item : AP_ITEMS)
		{
			if (item.id == scout.item)
			{
				ap_item = &item;
				break;
			}
		}
		if (!ap_item)
		{
			// Show AP item
			continue;
		}

		switch (scout.loc->type)
		{
			case ap_location_type_t::shop:
				if (ap_item->item_id != 0x00)
				{
					m_info.rom[scout.loc->addr] = ap_item->item_id;
				}
				break;
			case ap_location_type_t::give:
				if (ap_item->item_id != 0x00)
				{
					m_info.rom[scout.loc->addr] = ap_item->item_id;
					m_info.rom[scout.loc->giver_cond_addr] = ap_item->item_id;
				}
				break;
			case ap_location_type_t::world:
				if (ap_item->entity_id != 0x00)
				{
					m_info.rom[scout.loc->addr] = ap_item->entity_id;
				}
				break;
			case ap_location_type_t::boss_reward:
				if (ap_item->entity_id != 0x00)
				{
					m_info.rom[scout.loc->addr] = ap_item->entity_id;
				}
				break;
			case ap_location_type_t::hidden:
				if (ap_item->entity_id != 0x00)
				{
					m_info.rom[scout.loc->addr] = ap_item->entity_id;
				}
				break;
		}
	}
}


void AP::render()
{
}
