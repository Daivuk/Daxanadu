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


static void copy_sprite(uint8_t* src, uint8_t* dst, int flip /*1=h,2=v*/, bool invert_green_red, int shift = 0)
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

		if (flip & 2)
		{
			src_plane0 = src[7 - y];
			src_plane1 = src[7 - y + 8];
		}

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
			if (flip & 1)
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
#define BANK_OFFSET(bank, offset) (bank * 0x4000 + offset)
#define ROM_OFFSET_LO(bank, offset) (m_info.rom + BANK_ADDR_LO(bank, (offset) + 0x8000))
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

	auto patcher = m_info.patcher;

	// Replace Crystal with spring elixir
	copy_sprite(TILE_ADDR(0x34), TILE_ADDR(0x12), 0, true);
	copy_sprite(TILE_ADDR(0x35), TILE_ADDR(0x13), 0, true);
	copy_sprite(TILE_ADDR(0x36), TILE_ADDR(0x14), 0, true);
	copy_sprite(TILE_ADDR(0x37), TILE_ADDR(0x15), 0, true);

	// Replace Lamp with glove
	copy_sprite(SPRITE_ADDR(0x0001CD26, 0x00), TILE_ADDR(0x16), 0, false, 4);
	copy_sprite(SPRITE_ADDR(0x0001CD26, 0x00), TILE_ADDR(0x17), 0, false, -4);
	copy_sprite(SPRITE_ADDR(0x0001CD26, 0x01), TILE_ADDR(0x18), 0, false, 4);
	copy_sprite(SPRITE_ADDR(0x0001CD26, 0x01), TILE_ADDR(0x19), 0, false, -4);

	// Replace fire crystal tiles with ointment
	copy_sprite(SPRITE_ADDR(0x0001CE06, 0x00), TILE_ADDR(0x40), 0, false);
	copy_sprite(SPRITE_ADDR(0x0001CE06, 0x00), TILE_ADDR(0x41), 1, false);
	copy_sprite(SPRITE_ADDR(0x0001CE06, 0x01), TILE_ADDR(0x42), 0, false);
	copy_sprite(SPRITE_ADDR(0x0001CE06, 0x01), TILE_ADDR(0x43), 1, false);

	// Replace battle helmet sprites for Magic Shield tiles
	copy_sprite(TILE_ADDR(0x69), SPRITE_ADDR(0x0001CFC6, 0x00), 0, false);
	copy_sprite(TILE_ADDR(0x6A), SPRITE_ADDR(0x0001CFC6, 0x01), 0, false);
	copy_sprite(TILE_ADDR(0x6B), SPRITE_ADDR(0x0001CFC6, 0x02), 0, false);
	copy_sprite(TILE_ADDR(0x6C), SPRITE_ADDR(0x0001CFC6, 0x03), 0, false);

	// Rename battle helmet to progressive shield in the popup dialog
	memcpy(ROM_LO(13, 0xB243), "I've""\xfd""got""\xfe""Progressive""\xfe""Shield", 27);
	memcpy(ROM_LO(13, 0xB25F), "I've""\xfd""got""\xfe""Progressive""\xfe""Sword.", 27);
	memcpy(ROM_LO(13, 0xB229), "Ive""\xfd""got""\xfe""Progressive""\xfe""Armor", 25);

	// World items
#if 1
	{
		// Load sprite id and ignore 0xFF
		{
			auto load_sprite_id_addr = patcher->patch_new_code(14, {
				OP_LDA_ABSX(0x02CC),
				OP_CMP_IMM(0xFF),
				OP_BEQ(3),
				OP_JMP_ABS(0x8019),
				OP_JMP_ABS(0x8043),
			});

			patcher->patch(14, 0x8014, 0, {
				OP_JMP_ABS(load_sprite_id_addr)
			});

			auto update_sprite_no_sprite = patcher->patch_new_code(14, {
				OP_LDY_ABSX(0x02CC),
				OP_CPY_IMM(0xFF),
				OP_BEQ(3),
				OP_JMP_ABS(0x8BDF),
				OP_RTS(),
			});

			patcher->patch(14, 0x8BDA, 0, {
				OP_JMP_ABS(update_sprite_no_sprite)
			});
		}

		// Progressive sword (We're lucky this code fits perfectly to replace previous one)
		patcher->patch(15, 0xC737, 0, {
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
		patcher->patch(15, 0xC717, 0, {
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
		patcher->patch(15, 0xC6F7, 0, {
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

		// Dialogs, Start at ID 0x98
		auto elf_ring_dialog = patcher->patch_new_code(12, { 0x00, 0x01, 0xC4, 0x00 }); // 0x98
		auto ruby_ring_dialog = patcher->patch_new_code(12, { 0x00, 0x01, 0xC5, 0x00 }); // 0x99
		auto dworf_ring_dialog = patcher->patch_new_code(12, { 0x00, 0x01, 0xC6, 0x00 }); // 0x9A
		auto demons_ring_dialog = patcher->patch_new_code(12, { 0x00, 0x01, 0xC7, 0x00 }); // 0x9B

		auto key_jack_dialog = patcher->patch_new_code(12, { 0x00, 0x01, 0xC8, 0x00 }); // 0x9C
		auto key_queen_dialog = patcher->patch_new_code(12, { 0x00, 0x01, 0xC9, 0x00 }); // 0x9D
		auto key_king_dialog = patcher->patch_new_code(12, { 0x00, 0x01, 0xCA, 0x00 }); // 0x9E
		auto key_ace_dialog = patcher->patch_new_code(12, { 0x00, 0x01, 0xCB, 0x00 }); // 0x9F
		auto key_joker_dialog = patcher->patch_new_code(12, { 0x00, 0x01, 0xCC, 0x00 }); // 0xA0

		auto deluge_dialog = patcher->patch_new_code(12, { 0x00, 0x01, 0xCD, 0x00 }); // 0xA1
		auto thunder_dialog = patcher->patch_new_code(12, { 0x00, 0x01, 0xCE, 0x00 }); // 0xA2
		auto fire_dialog = patcher->patch_new_code(12, { 0x00, 0x01, 0xCF, 0x00 }); // 0xA3
		auto death_dialog = patcher->patch_new_code(12, { 0x00, 0x01, 0xD0, 0x00 }); // 0xA4
		auto tilte_dialog = patcher->patch_new_code(12, { 0x00, 0x01, 0xD1, 0x00 }); // 0xA5

		auto spring_elixir_dialog = patcher->patch_new_code(12, { 0x00, 0x01, 0xD2, 0x00 }); // 0xA6

		auto ap_dialog = patcher->patch_new_code(12, { 0x00, 0x01, 0xD3, 0x00 }); // 0xA6

		// Table continue
		auto dialog_lo = patcher->patch_new_code(12, {
			LO(elf_ring_dialog),
			LO(ruby_ring_dialog),
			LO(dworf_ring_dialog),
			LO(demons_ring_dialog),
			LO(key_jack_dialog),
			LO(key_queen_dialog),
			LO(key_king_dialog),
			LO(key_ace_dialog),
			LO(key_joker_dialog),
			LO(deluge_dialog),
			LO(thunder_dialog),
			LO(fire_dialog),
			LO(death_dialog),
			LO(tilte_dialog),
			LO(spring_elixir_dialog),
			LO(ap_dialog),
			LO(ap_dialog),
		});
		auto dialog_hi = patcher->patch_new_code(12, {
			HI(elf_ring_dialog),
			HI(ruby_ring_dialog),
			HI(dworf_ring_dialog),
			HI(demons_ring_dialog),
			HI(key_jack_dialog),
			HI(key_queen_dialog),
			HI(key_king_dialog),
			HI(key_ace_dialog),
			HI(key_joker_dialog),
			HI(deluge_dialog),
			HI(thunder_dialog),
			HI(fire_dialog),
			HI(death_dialog),
			HI(tilte_dialog),
			HI(spring_elixir_dialog),
			HI(ap_dialog),
			HI(ap_dialog),
		});

		// We want to add dialogs for the new items. They are tightly packed into the
		// bank 12. Modify the code to jump ahead further if message ID is too big
		auto get_dialog_addr_addr = patcher->patch_new_code(12, {
			OP_SEC(),
			OP_SBC_IMM(0x98),
			OP_BCC(12),

			// Greater or equal to 0x98, we go somewhere else
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

		patcher->patch(12, 0x824C, 0, {
			OP_JSR(get_dialog_addr_addr),
			OP_NOP(), OP_NOP(),
			OP_NOP(), OP_NOP(), OP_NOP(),
			OP_NOP(), OP_NOP(),
		});

		// The arrays for entity properties are tightly packed.
		// We have to insert our own code a bit everywhere to return offsets to new tables.
		// The good news is, we return same value for most of these.
		// So only new functions are required that return constant if entity ID is more than $65 (101).

		// Entity size in Pixels
		{
			auto lookup_lo_y_addr = patcher->patch_new_code(14, {
				OP_LDA_ABSX(0x2CC),
				OP_BPL(3),
				OP_LDA_IMM(0x10), // 16 pixels
				OP_RTS(),
				OP_LDA_ABSY(0xB407), // Original table
				OP_RTS(),
			});

			auto lookup_hi_y_addr = patcher->patch_new_code(14, {
				OP_LDA_ABSX(0x2CC),
				OP_BPL(3),
				OP_LDA_IMM(0x10), // 16 pixels
				OP_RTS(),
				OP_LDA_ABSY(0xB407 + 1), // Original table
				OP_RTS(),
			});

			patcher->patch(14, 0x88D0, 0, { OP_JSR(lookup_lo_y_addr) });
			patcher->patch(14, 0x88E5, 0, { OP_JSR(lookup_hi_y_addr) });
		}

		// Entity size in 8x8 blocks
		{
			auto lookup_x_addr = patcher->patch_new_code(14, {
				OP_TXA(),
				OP_BPL(3),
				OP_LDA_IMM(0x00), // 2 blocks
				OP_RTS(),
				OP_LDA_ABSX(0xB4DF), // Original table
				OP_RTS(),
			});

			auto lookup_y_addr = patcher->patch_new_code(14, {
				OP_TYA(),
				OP_BPL(3),
				OP_LDA_IMM(0x00), // 2 blocks
				OP_RTS(),
				OP_LDA_ABSY(0xB4DF), // Original table
				OP_RTS(),
			});

			patcher->patch(14, 0xA220, 0, { OP_JSR(lookup_x_addr) });
			patcher->patch(14, 0xAC96, 0, { OP_JSR(lookup_y_addr) });
			patcher->patch(14, 0xACC6, 0, { OP_JSR(lookup_y_addr) });

			auto lookup15_y_addr = patcher->patch_new_code(15, {
				OP_BPL(3),
				OP_LDA_IMM(0x00), // 2 blocks
				OP_RTS(),
				OP_LDA_ABSY(0xB4DF), // Original table
				OP_RTS(),
			});

			patcher->patch(15, 0xC22F, 0, { OP_JSR(lookup15_y_addr) });
		}

		// Entity type
		{
			auto lookup_x_addr = patcher->patch_new_code(14, {
				OP_TXA(),
				OP_BPL(3),
				OP_LDA_IMM(0x05), // 5 = item
				OP_RTS(),
				OP_LDA_ABSX(0xB544), // Original table
				OP_RTS(),
			});

			auto lookup_y_addr = patcher->patch_new_code(14, {
				OP_TYA(),
				OP_BPL(3),
				OP_LDA_IMM(0x05), // 5 = item
				OP_RTS(),
				OP_LDA_ABSY(0xB544), // Original table
				OP_RTS(),
			});
			
			patcher->patch(14, 0x8220, 0, { OP_JSR(lookup_y_addr) });
			patcher->patch(14, 0x8797, 0, { OP_JSR(lookup_y_addr) });
			patcher->patch(14, 0x881F, 0, { OP_JSR(lookup_y_addr) });
			patcher->patch(14, 0x88B9, 0, { OP_JSR(lookup_y_addr) });
			patcher->patch(14, 0x8946, 0, { OP_JSR(lookup_y_addr) });
			patcher->patch(14, 0x8AF3, 0, { OP_JSR(lookup_y_addr) });
			patcher->patch(14, 0xA546, 0, { OP_JSR(lookup_x_addr) });
			patcher->patch(14, 0xA673, 0, { OP_JSR(lookup_y_addr) });

			auto lookup15_y_addr = patcher->patch_new_code(15, {
				OP_TYA(),
				OP_BPL(3),
				OP_LDA_IMM(0x05), // 5 = item
				OP_RTS(),
				OP_LDA_ABSY(0xB544), // Original table
				OP_RTS(),
			});

			patcher->patch(15, 0xEFFD, 0, { OP_JSR(lookup15_y_addr) });
		}

		// Entity health
		{
			auto lookup_x_addr = patcher->patch_new_code(14, {
				OP_TXA(),
				OP_BPL(3),
				OP_LDA_IMM(0x00), // 0 health
				OP_RTS(),
				OP_LDA_ABSX(0xB5A9), // Original table
				OP_RTS(),
			});

			patcher->patch(14, 0x9D6C, 0, { OP_JSR(lookup_x_addr) });

			auto lookup15_y_addr = patcher->patch_new_code(15, {
				OP_TYA(),
				OP_BPL(3),
				OP_LDA_IMM(0x00), // 0 health
				OP_RTS(),
				OP_LDA_ABSY(0xB5A9), // Original table
				OP_RTS(),
			});

			patcher->patch(15, 0xC235, 0, { OP_JSR(lookup15_y_addr) });
		}

		// Entity XP reward
		{
			auto lookup_y_addr = patcher->patch_new_code(14, {
				OP_TYA(),
				OP_BPL(3),
				OP_LDA_IMM(0x00), // No XP reward
				OP_RTS(),
				OP_LDA_ABSY(0xB60E), // Original table
				OP_RTS(),
			});

			patcher->patch(14, 0x8B8A, 0, { OP_JSR(lookup_y_addr) });
		}

		// Entity reward type
		{
			auto lookup_y_addr = patcher->patch_new_code(14, {
				OP_TYA(),
				OP_BPL(3),
				OP_LDA_IMM(0xFF), // No reward
				OP_RTS(),
				OP_LDA_ABSY(0xB672), // Original table
				OP_RTS(),
			});

			patcher->patch(14, 0xAC32, 0, { OP_JSR(lookup_y_addr) });
		}

		// Entity damage
		{
			auto lookup_y_addr = patcher->patch_new_code(14, {
				OP_TYA(),
				OP_BPL(3),
				OP_LDA_IMM(0x00), // No damage
				OP_RTS(),
				OP_LDA_ABSY(0xB6D7), // Original table
				OP_RTS(),
			});

			patcher->patch(14, 0x87E2, 0, { OP_JSR(lookup_y_addr) });
			patcher->patch(14, 0x8A7C, 0, { OP_JSR(lookup_y_addr) });
		}

		// Entity magic resistance
		{
			auto lookup_y_addr = patcher->patch_new_code(14, {
				OP_TYA(),
				OP_BPL(3),
				OP_LDA_IMM(0xFC), // This is probably a mask
				OP_RTS(),
				OP_LDA_ABSY(0xB73B), // Original table
				OP_RTS(),
			});

			patcher->patch(14, 0x81DC, 0, { OP_JSR(lookup_y_addr) });
		}

		// Entity behaviour
		{
			std::vector<uint8_t> entity_type_lo = {
				0x17, // Red Potion stays
				0x63, // Hidden Red Potion
				0x5B, // Boss Mattock
			};
			std::vector<uint8_t> entity_type_hi = {
				0xB2, // Red Potion
				0xB2, // Hidden Red Potion
				0xB2, // Boss Mattock
			};

			auto entity_type_lookup_lo_addr = patcher->patch_new_code(14, entity_type_lo);
			auto entity_type_lookup_hi_addr = patcher->patch_new_code(14, entity_type_hi);

			auto lookup_x_addr = patcher->patch_new_code(14, {
				OP_BPL(19),
				
				OP_AND_IMM(0x7F),
				OP_JSR(0xF785), // Divide by 32
				OP_TAX(),
				OP_LDA_ABSX(entity_type_lookup_lo_addr),
				OP_STA_ABSY(0x0354),
				OP_LDA_ABSX(entity_type_lookup_hi_addr),
				OP_STA_ABSY(0x035C),
				OP_RTS(),

				OP_ASL_A(),
				OP_TAX(),
				OP_LDA_ABSX(0xAD2D),
				OP_STA_ABSY(0x0354),
				OP_LDA_ABSX(0xAD2D + 1),
				OP_STA_ABSY(0x035C),
				OP_RTS(),
			});

			auto lookup_y_addr = patcher->patch_new_code(14, {
				OP_BPL(19),
				
				OP_AND_IMM(0x7F),
				OP_JSR(0xF785), // Divide by 32
				OP_TAY(),
				OP_LDA_ABSY(entity_type_lookup_lo_addr),
				OP_STA_ABSX(0x0354),
				OP_LDA_ABSY(entity_type_lookup_hi_addr),
				OP_STA_ABSX(0x035C),
				OP_RTS(),

				OP_ASL_A(),
				OP_TAY(),
				OP_LDA_ABSY(0xAD2D),
				OP_STA_ABSX(0x0354),
				OP_LDA_ABSY(0xAD2D + 1),
				OP_STA_ABSX(0x035C),
				OP_RTS(),
			});

			patcher->patch(14, 0xA207, 0, {
				OP_JSR(lookup_x_addr),
				OP_NOP(), OP_NOP(), OP_NOP(),
				OP_NOP(), OP_NOP(), OP_NOP(),
				OP_NOP(), OP_NOP(), OP_NOP(),
				OP_NOP(), OP_NOP(),
			});

			patcher->patch(14, 0xAC03, 0, {
				OP_JSR(lookup_y_addr),
				OP_NOP(), OP_NOP(), OP_NOP(),
				OP_NOP(), OP_NOP(), OP_NOP(),
				OP_NOP(), OP_NOP(), OP_NOP(),
				OP_NOP(), OP_NOP(),
			});

			patcher->patch(14, 0xAC81, 0, {
				OP_JSR(lookup_y_addr),
				OP_NOP(), OP_NOP(), OP_NOP(),
				OP_NOP(), OP_NOP(), OP_NOP(),
				OP_NOP(), OP_NOP(), OP_NOP(),
				OP_NOP(), OP_NOP(),
			});

			patcher->patch(14, 0xACB0, 0, {
				OP_JSR(lookup_y_addr),
				OP_NOP(), OP_NOP(), OP_NOP(),
				OP_NOP(), OP_NOP(), OP_NOP(),
				OP_NOP(), OP_NOP(), OP_NOP(),
				OP_NOP(), OP_NOP(),
			});

			auto entity_type_lookup15_lo_addr = patcher->patch_new_code(15, entity_type_lo);
			auto entity_type_lookup15_hi_addr = patcher->patch_new_code(15, entity_type_hi);

			auto lookup15_addr = patcher->patch_new_code(15, {
				OP_BPL(21),

				OP_AND_IMM(0x7F),
				OP_JSR(0xF785), // Divide by 32
				OP_TAY(),
				OP_LDA_ABSY(entity_type_lookup15_lo_addr),
				OP_STA_ABSX(0x0354),
				OP_LDA_ABSY(entity_type_lookup15_hi_addr),
				OP_STA_ABSX(0x035C),
				OP_JMP_ABS(0xC24A),

				OP_ASL_A(),
				OP_TAY(),
				OP_LDA_ABSY(0xAD2D),
				OP_STA_ABSX(0x0354),
				OP_LDA_ABSY(0xAD2E),
				OP_STA_ABSX(0x035C),
				OP_JMP_ABS(0xC24A),
			});

			patcher->patch(15, 0xC23C, 0, { OP_JMP_ABS(lookup15_addr) });
		}

		// Entity update function
		{
			std::vector<uint8_t> entity_update_lo = {
				0x09, // Red Potion stays
				0x09, // Hidden Red Potion
				0x09, // Boss Mattock
			};
			std::vector<uint8_t> entity_update_hi = {
				0xA3, // Red Potion
				0xA3, // Hidden Red Potion
				0xA3, // Boss Mattock
			};

			auto entity_update_lookup_lo_addr = patcher->patch_new_code(14, entity_update_lo);
			auto entity_update_lookup_hi_addr = patcher->patch_new_code(14, entity_update_hi);

			auto update_addr = patcher->patch_new_code(14, {
				OP_BPL(15),

				OP_AND_IMM(0x7F),
				OP_JSR(0xF785), // Divide by 32
				OP_TAY(),
				OP_LDA_ABSY(entity_update_lookup_hi_addr), // Notice we do HI first
				OP_PHA(),
				OP_LDA_ABSY(entity_update_lookup_lo_addr),
				OP_PHA(),
				OP_RTS(),

				OP_ASL_A(),
				OP_TAY(),
				OP_LDA_ABSY(0x8087 + 1),
				OP_PHA(),
				OP_LDA_ABSY(0x8087),
				OP_PHA(),
				OP_RTS(),
			});

			patcher->patch(14, 0x8C0D, 0, { OP_JMP_ABS(update_addr) });
		}

		// Function that gets the rectangle for the sprite. Let's redo it and hardcode our known values
		{
			auto get_sprite_rect_addr = patcher->patch_new_code(14, {
				OP_LDA_ABSX(0x02CC), // Load sprite ID
				OP_BPL(19),

				OP_LDA_ZPGX(0xBA),
				OP_STA_ABS(0x03E2), // Left
				OP_LDA_ZPGX(0xC2),
				OP_STA_ABS(0x03E3), // Top
				OP_LDA_IMM(0x10),
				OP_STA_ABS(0x03E4), // Width
				OP_STA_ABS(0x03E5), // Height
				OP_RTS(),

				OP_JMP_ABS(0x8A0F), // Continue normally
			});

			patcher->patch(14, 0x8A0C, 0, { OP_JMP_ABS(get_sprite_rect_addr) });
		}

		// Sprite sheets lookup
		{
			auto entity_id_lookup_addr = patcher->patch_new_code(15, {
				OP_LDA_ABS(0x038B), // Current entity we're working with
				OP_BPL(5),
				OP_AND_IMM(0x1F),
				OP_JMP_ABS(0xCD98),
				OP_JMP_ABS(0xCD92),
			});

			patcher->patch(15, 0xCD8F, 0, { OP_JMP_ABS(entity_id_lookup_addr) });

			auto get_sprite_bank_addr = patcher->patch_new_code(15, {
				OP_BMI(8),
				OP_CMP_IMM(0x37),
				OP_BCC(1), // First bank
				OP_INY(),
				OP_JMP_ABS(0xC286),
				OP_LDA_IMM(9), // New items are in bank 9
				OP_JMP_ABS(0xC289),
			});

			patcher->patch(15, 0xC281, 0, { OP_JMP_ABS(get_sprite_bank_addr) });

			// Number of tiles an entity needs
			auto lookup_y_addr = patcher->patch_new_code(15, {
				OP_BPL(3),

				OP_LDA_IMM(4),
				OP_RTS(),

				OP_LDA_ABSY(0xCE1B), // Original table
				OP_RTS(),
			});

			patcher->patch(15, 0xCDAA, 0, { OP_JSR(lookup_y_addr) });
		}

		// Phase table
		{
			auto lookup_y_addr = patcher->patch_new_code(14, {
				OP_BPL(13),

				OP_TYA(),
				OP_AND_IMM(0x1F),
				OP_TAY(),

				OP_LDA_ABS(0x0100), // Original code (Dont remove)
				OP_PHA(), // Original code (Dont remove)
				OP_LDX_IMM(9), // Where we keep our new frames
				OP_JMP_ABS(0xF05E),

				OP_ADC_ABSY(0x8C9F), // Original table
				OP_JMP_ABS(0xF057), // This will load bank 7
			});

			patcher->patch(14, 0x8C92, 0, { OP_JMP_ABS(lookup_y_addr) });
		}

		// Bank 9, the sprite definitions and tiles
		{
			const int HEADER_SIZE = 8;
			const int FRAME_OFFSETS_TABLE_OFFSET = HEADER_SIZE;
			const int FRAMES_OFFSET = FRAME_OFFSETS_TABLE_OFFSET + EXTRA_ITEMS_COUNT * 2;
			const int TILE_OFFSETS_TABLE_OFFSET = FRAMES_OFFSET + EXTRA_ITEMS_COUNT * 12;
			const int TILES_OFFSET = TILE_OFFSETS_TABLE_OFFSET + EXTRA_ITEMS_COUNT * 2;

			// Frame addr table at the start of the bank 9 (Bank 9 is empty, we put our new sprites in there)
			patcher->patch_new_code(9, {
				PATCH_ADDR(TILE_OFFSETS_TABLE_OFFSET),
				0, 0, 0, 0,
				PATCH_ADDR(FRAME_OFFSETS_TABLE_OFFSET)
			});

			//--- Frames
			for (int i = 0; i < EXTRA_ITEMS_COUNT; ++i)
				patcher->patch_new_code(9, { PATCH_ADDR(FRAMES_OFFSET + i * 12) });

			const uint8_t pal_lookup[] = {
				0, 0, 2, 1, // Rings
				1, 1, 1, 1, 1, // Keys
				1, 1, 1, 1, 1, // Magics
				2, // Spring Elixir
				1, 1, // AP
			};

			// Only difference between different sprites is the palette
			for (int i = 0; i < EXTRA_ITEMS_COUNT; ++i)
			{
				uint8_t pal = pal_lookup[i];
				patcher->patch_new_code(9, {
					0x11, /* 2x2 */
					0x00, /* x offset */
					0x00, /* y offset */
					0x08, /* unknown, other items use 8 */
					0, /* Sprite ID */
					pal, /* Palette */
					1, /* Sprite ID */
					pal, /* Palette */
					2, /* Sprite ID */
					pal, /* Palette */
					3, /* Sprite ID */
					pal, /* Palette */
				});
			}

			//--- Tiles
			for (int i = 0; i < EXTRA_ITEMS_COUNT; ++i)
			{
				patcher->patch_new_code(9, { PATCH_ADDR(TILES_OFFSET + i * 64) }); // Offset into the bank to the tiles
			}

#define DST_TILE(idx) ROM_OFFSET_LO(9, TILES_OFFSET + idx * 16)

			// Ring of elf
			copy_sprite(TILE_ADDR(0x06), DST_TILE(0), 0, true);
			copy_sprite(TILE_ADDR(0x07), DST_TILE(1), 0, true);
			copy_sprite(TILE_ADDR(0x0C), DST_TILE(2), 0, false);
			copy_sprite(TILE_ADDR(0x0D), DST_TILE(3), 0, false);

			// Ruby
			copy_sprite(TILE_ADDR(0x08), DST_TILE(4), 0, true);
			copy_sprite(TILE_ADDR(0x09), DST_TILE(5), 0, true);
			copy_sprite(TILE_ADDR(0x0C), DST_TILE(6), 0, false);
			copy_sprite(TILE_ADDR(0x0D), DST_TILE(7), 0, false);

			// Dworf
			copy_sprite(TILE_ADDR(0x0A), DST_TILE(8), 0, false);
			copy_sprite(TILE_ADDR(0x0B), DST_TILE(9), 0, false);
			copy_sprite(TILE_ADDR(0x0C), DST_TILE(10), 0, false);
			copy_sprite(TILE_ADDR(0x0D), DST_TILE(11), 0, false);

			// Demons
			copy_sprite(TILE_ADDR(0x04), DST_TILE(12), 0, false);
			copy_sprite(TILE_ADDR(0x05), DST_TILE(13), 0, false);
			copy_sprite(TILE_ADDR(0x0C), DST_TILE(14), 0, false);
			copy_sprite(TILE_ADDR(0x0D), DST_TILE(15), 0, false);

			// Jack
			copy_sprite(TILE_ADDR(0x73), DST_TILE(16), 0, true);
			copy_sprite(TILE_ADDR(0x27), DST_TILE(17), 0, true);
			copy_sprite(TILE_ADDR(0x74), DST_TILE(18), 0, true);
			copy_sprite(TILE_ADDR(0x29), DST_TILE(19), 0, true);

			// Queen
			copy_sprite(TILE_ADDR(0x71), DST_TILE(20), 0, true);
			copy_sprite(TILE_ADDR(0x27), DST_TILE(21), 0, true);
			copy_sprite(TILE_ADDR(0x72), DST_TILE(22), 0, true);
			copy_sprite(TILE_ADDR(0x29), DST_TILE(23), 0, true);
			
			// King
			copy_sprite(TILE_ADDR(0x2A), DST_TILE(24), 0, true);
			copy_sprite(TILE_ADDR(0x27), DST_TILE(25), 0, true);
			copy_sprite(TILE_ADDR(0x2B), DST_TILE(26), 0, true);
			copy_sprite(TILE_ADDR(0x29), DST_TILE(27), 0, true);
			
			// Ace
			copy_sprite(TILE_ADDR(0x26), DST_TILE(28), 0, true);
			copy_sprite(TILE_ADDR(0x27), DST_TILE(29), 0, true);
			copy_sprite(TILE_ADDR(0x28), DST_TILE(30), 0, true);
			copy_sprite(TILE_ADDR(0x29), DST_TILE(31), 0, true);
			
			// Joker
			copy_sprite(TILE_ADDR(0x75), DST_TILE(32), 0, true);
			copy_sprite(TILE_ADDR(0x27), DST_TILE(33), 0, true);
			copy_sprite(TILE_ADDR(0x76), DST_TILE(34), 0, true);
			copy_sprite(TILE_ADDR(0x29), DST_TILE(35), 0, true);
			
			// Deluge
			copy_sprite(TILE_ADDR(0x7F), DST_TILE(36), 0, true);
			copy_sprite(TILE_ADDR(0x80), DST_TILE(37), 0, true);
			copy_sprite(TILE_ADDR(0x7F), DST_TILE(38), 2, true);
			copy_sprite(TILE_ADDR(0x80), DST_TILE(39), 2, true);
			
			// Thunder
			copy_sprite(TILE_ADDR(0x83), DST_TILE(40), 0, true);
			copy_sprite(TILE_ADDR(0x84), DST_TILE(41), 0, true);
			copy_sprite(TILE_ADDR(0x83), DST_TILE(42), 2, true);
			copy_sprite(TILE_ADDR(0x84), DST_TILE(43), 2, true);
			
			// Fire
			copy_sprite(TILE_ADDR(0x87), DST_TILE(44), 0, true);
			copy_sprite(TILE_ADDR(0x88), DST_TILE(45), 0, true);
			copy_sprite(TILE_ADDR(0x87), DST_TILE(46), 2, true);
			copy_sprite(TILE_ADDR(0x88), DST_TILE(47), 2, true);
			
			// Death
			copy_sprite(TILE_ADDR(0x7B), DST_TILE(48), 0, true);
			copy_sprite(TILE_ADDR(0x7C), DST_TILE(49), 0, true);
			copy_sprite(TILE_ADDR(0x7D), DST_TILE(50), 0, true);
			copy_sprite(TILE_ADDR(0x7E), DST_TILE(51), 0, true);
			
			// Tilte
			copy_sprite(TILE_ADDR(0x77), DST_TILE(52), 0, true);
			copy_sprite(TILE_ADDR(0x78), DST_TILE(53), 0, true);
			copy_sprite(TILE_ADDR(0x79), DST_TILE(54), 0, true);
			copy_sprite(TILE_ADDR(0x7A), DST_TILE(55), 0, true);
			
			// Spring Elixir
			copy_sprite(TILE_ADDR(0x34), DST_TILE(56), 0, true);
			copy_sprite(TILE_ADDR(0x35), DST_TILE(57), 0, true);
			copy_sprite(TILE_ADDR(0x36), DST_TILE(58), 0, true);
			copy_sprite(TILE_ADDR(0x37), DST_TILE(59), 0, true);
		}

		// Touching an item entity
		{
			auto entity_to_item_table_addr = patcher->patch_new_code(15, {
				0x80, 0x81, 0x82, 0x83, // Rings
				0x87, 0x86, 0x85, 0x84, 0x88, // Keys
				0x60, 0x61, 0x62, 0x63, 0x64, // Magics
				AP_ITEM_SPRING_ELIXIR,
			});

			auto touched_new_item_addr = patcher->patch_new_code(15, {
				// Show dialog
				OP_AND_IMM(0x1F),
				OP_CLC(),
				OP_ADC_IMM(0x98),
				OP_JSR(0xF859),
				0x0C, 0x41, 0x82, // I have no idea why this is needed after a dialog

				// Play sound
				OP_LDA_IMM(0x08),
				OP_JSR(0xD0E4),

				// Give item
				OP_LDX_IMM(12), OP_JSR(0xCC1A), // Switch bank 12
				OP_LDX_ABS(0x0378),
				OP_LDA_ABSX(0x02CC),
				OP_TAX(),
				OP_LDA_ABSX(entity_to_item_table_addr - 0x80),
				OP_JSR(0x9AF7), // Give item
				OP_LDX_IMM(14), OP_JSR(0xCC1A), // Switch bank 14

				OP_RTS(),
			});

			auto touched_item_addr = patcher->patch_new_code(15, {
				OP_CMP_IMM(0x57), // Magical Rod
				OP_BNE(3),
				OP_JMP_ABS(0xC810), // Magical rod pickup code

				OP_TAY(), // Do we care about integrity of Y here? We do for X that I know
				OP_BPL(3),
				OP_JMP_ABS(touched_new_item_addr),
			
				OP_JMP_ABS(0xC76F), // Return where we were
			});

			patcher->patch(15, 0xC768, 0, {
				OP_JMP_ABS(touched_item_addr),
			});
		}
	}
#endif

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
		auto addr = patcher->patch_new_code(12, {
			OP_BCC(4),

			OP_LDY_IMM(0xAE),
			OP_STY_ZPG(0xED),

			OP_TAY(),
			OP_JMP_ABS(0x8E9B),
		});

		patcher->patch(12, 0x8C50, 0, {
			OP_JMP_ABS(addr),
		});
	}

	// 15:C8CD ; Description: Stores an item in the next free slot in the item directory

	// Create a function that checks what inventory item is being added.
	// So we can change it to something else, like if we are adding progressive sword, add
	// the next sword instead.
	{
		auto choose_prog_sword = patcher->patch_new_code(12, {
			OP_LDA_IMM(0), // Initialize A to zero
			OP_LDY_ABS(0x03BD), // Active weapon
			OP_BMI(2), // If active weapon is FF, skip
			OP_LDA_IMM(1), // Active weapon is set, so set A to 1
			OP_CLC(),
			OP_ADC_ABS(0x03C2), // Add number of weapons to A
			OP_RTS(), // A is now the item id to add
		});

		auto choose_prog_shield = patcher->patch_new_code(12, {
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

		auto choose_prog_armor = patcher->patch_new_code(12, {
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

		auto give_item = patcher->patch_new_code(12, {
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

		patcher->patch(12, 0x9B01, 0, {
			OP_JSR(give_item),
			OP_NOP(),
			OP_NOP(),
		});
	}

	// Spring elixir quest
	{
		patcher->patch(12, 0xA1B3, 0, { AP_ITEM_SPRING_ELIXIR }); // Check if has
		patcher->patch(12, 0xA1BC, 0, { AP_ITEM_SPRING_ELIXIR }); // Give

		// Remove item, but checks first if it's not selected, and remove it from selected
		auto remove_item = patcher->patch_new_code(12, {
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
		patcher->patch(12, 0x865A, 1, { PATCH_ADDR(remove_item) });
	}

	// Don't remove keys when used
	patcher->patch(15, 0xEBD9, 0, {
		OP_NOP(), OP_NOP(),
		OP_NOP(), OP_NOP(), OP_NOP(),
		OP_NOP(), OP_NOP(), OP_NOP(),
	});

	// Don't remove mattock when used
	patcher->patch(15, 0xC64C, 0, {
		OP_NOP(), OP_NOP(), OP_NOP(),
	});

	// Wingboots
	{
		// Don't remove wing boots when used,
		// but remove them from selected item and put them back into inventory.
		patcher->patch(15, 0xC581, 0, {
			OP_NOP(), OP_NOP(), OP_NOP(),
		});

		// Trigger a reuse timeout of 2mins
	}

	// Item inventory limit to 12
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
		//   glove
		//   ointment

		// Max number of item
		patcher->patch(12, 0x847D, 4, { 12 });

		// Check at store
		//patcher->patch(12, 0x846C, 1, { 6 }); // Where it displays "you cant carry more"
		patcher->patch(12, 0x8405, 0, { OP_JMP_ABS(0x8439) });

		// Add to inventory
		patcher->patch(15, 0xC8D0, 1, { 12 });

		// Make item inventory window bigger
		//patcher->patch(12, 0x8AA2, 1, { 0x08 }); // Inventory screen y position
		//patcher->patch(12, 0x8AFF, 1, { 0x04 }); // y position of items dialog
		//patcher->patch(12, 0x8B09, 1, { 0x20 }); // height of items dialog

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
	//return;
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
				if (ap_item->entity_id != 0xFF)
				{
					m_info.rom[scout.loc->addr] = ap_item->entity_id;
				}
				break;
			case ap_location_type_t::hidden:
				if (ap_item->hidden_entity_id != 0xFF)
				{
					m_info.rom[scout.loc->addr] = ap_item->hidden_entity_id;
				}
				break;
			case ap_location_type_t::boss_reward:
				if (ap_item->boss_entity_id != 0xFF)
				{
					m_info.rom[scout.loc->addr] = ap_item->boss_entity_id;
				}
				break;
		}
	}
}


void AP::render()
{
}
