#include "AP.h"
#include "APItems.h"
#include "APLocations.h"
#include "APTracker.h"
#include "Cart.h"
#include "CPU.h"
#include "CPUBUS.h"
#include "ExternalInterface.h"
#include "Patcher.h"
#include "RAM.h"
#include "version.h"

#include "Archipelago.h"

#include <onut/Files.h>
#include <onut/Images.h>
#include <onut/Maths.h>
#include <onut/onut.h>
#include <onut/Settings.h>
#include <onut/Strings.h>


#define BANK_ADDR_LO(bank, addr) (bank * 0x4000 + (addr - 0x8000))
#define BANK_ADDR_HI(bank, addr) (bank * 0x4000 + (addr - 0xC000))
#define ROM_LO(bank, addr) (m_info.rom + BANK_ADDR_LO(bank, addr))
#define ROM_HI(bank, addr) (m_info.rom + BANK_ADDR_HI(bank, addr))
#define BANK_OFFSET(bank, offset) (bank * 0x4000 + offset)
#define ROM_OFFSET_LO(bank, offset) (m_info.rom + BANK_ADDR_LO(bank, (offset) + 0x8000))
#define TILE_ADDR(index) (m_info.rom + 0x00028500 + index * 16)
#define SPRITE_ADDR(rom_addr, index) (m_info.rom + rom_addr - 0x10 + index * 16)
#define LO(addr) (uint8_t)(addr & 0xFF)
#define HI(addr) (uint8_t)((addr >> 8) & 0xFF)
#define SRC_TILE(addr, id) m_info.rom + (addr - 0x10) + id * 16
#define DST_TILE_ID(id) ROM_OFFSET_LO(9, TILES_OFFSET + (id) * 16)


#define COMMON_ITEM_COUNT_ADDR 0x810
#define WINGBOOTS_UNLOCK_ADDR 0x819

#define MIN_SUPPORTED_VERSION "0.1.0"


static const int HEADER_SIZE = 8;
static const int FRAME_OFFSETS_TABLE_OFFSET = HEADER_SIZE;
static const int FRAMES_OFFSET = FRAME_OFFSETS_TABLE_OFFSET + EXTRA_ITEMS_COUNT * 2;
static const int TILE_OFFSETS_TABLE_OFFSET = FRAMES_OFFSET + EXTRA_ITEMS_COUNT * 12;
static const int TILES_OFFSET = TILE_OFFSETS_TABLE_OFFSET + EXTRA_ITEMS_COUNT * 2;


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


static void f_itemrecv(int64_t item_id, int player_id, bool notify_player)
{
	if (g_ap) g_ap->on_item_received(item_id, player_id, notify_player);
}


static void f_locrecv(int64_t loc_id)
{
	if (g_ap) g_ap->on_location_received(loc_id);
}


static void f_locinfo(std::vector<AP_NetworkItem> loc_infos)
{
	if (g_ap) g_ap->on_location_info(loc_infos);
}


static void f_version(std::string version)
{
	if (g_ap) g_ap->check_ap_version(version);
}


static void f_option_random_musics(int value)
{
	if (g_ap) g_ap->option_random_musics(value);
}


static void f_option_random_sounds(int value)
{
	if (g_ap) g_ap->option_random_sounds(value);
}


static void f_option_random_npcs(int value)
{
	if (g_ap) g_ap->option_random_npcs(value);
}


static void f_option_random_monsters(int value)
{
	if (g_ap) g_ap->option_random_monsters(value);
}


static void f_option_random_rewards(int value)
{
	if (g_ap) g_ap->option_random_rewards(value);
}


// Found in Patcher.cpp
static void load_tiles_from_png(const char* filename, uint8_t* out_data)
{
    Point size;
    auto png_data = onut::loadPNG(filename, size);
    if (size.x != 16 || size.y != 16)
    {
        onut::showMessageBox("ERROR", "(0x80000008) Corrupted or missing tile file: " + std::string(filename));
        OQuit();
    }

#define LOAD_TILE(src_x, src_y) \
    for (int y = 0; y < 8; ++y) \
    { \
        uint8_t plane0 = dst_data[y]; \
        uint8_t plane1 = dst_data[y + 8]; \
        for (int x = 0; x < 8; ++x) \
        { \
            uint32_t col32 = *reinterpret_cast<uint32_t*>(&png_data[(src_y + y) * 16 * 4 + (src_x + x) * 4]); \
            int b0 = (col32 & 0x000000FF) ? 1 : 0; \
            int b1 = (col32 & 0x0000FF00) ? 1 : 0; \
            plane0 = (plane0 & ~(1 << (7 - x))) | (b0 << (7 - x)); \
            plane1 = (plane1 & ~(1 << (7 - x))) | (b1 << (7 - x)); \
        } \
        dst_data[y] = plane0; \
        dst_data[y + 8] = plane1; \
    }

	auto dst_data = out_data + 0;
	LOAD_TILE(0, 0);
	dst_data = out_data + 16;
	LOAD_TILE(8, 0);
	dst_data = out_data + 32;
	LOAD_TILE(0, 8);
	dst_data = out_data + 48;
	LOAD_TILE(8, 8);
}


AP::AP(const ap_info_t& info)
    : m_info(info)
{
	g_ap = this;
	m_tracker = new APTracker(m_info.rom, m_info.ram, m_info.tile_drawer);
}


AP::~AP()
{
	connection_success_delegate = nullptr;
	connection_failed_delegate = nullptr;
	AP_Shutdown();
	delete m_tracker;
	g_ap = nullptr;
}


void AP::connect()
{
	m_state = state_t::connecting;
	patch_items();
	patch_cpp_hooks();
	m_info.patcher->print_usage();

	AP_NetworkVersion version = {0, 4, 3};
	AP_SetClientVersion(&version);
    AP_Init(m_info.address.c_str(), "Faxanadu", m_info.slot_name.c_str(), m_info.password.c_str());
	AP_SetDeathLinkSupported(true);
	AP_SetItemClearCallback(f_itemclr);
	AP_SetItemRecvCallback(f_itemrecv);
	AP_RegisterSlotDataRawCallback("daxanadu_version", f_version);
	AP_RegisterSlotDataIntCallback("random_musics", f_option_random_musics);
	AP_RegisterSlotDataIntCallback("random_sounds", f_option_random_sounds);
	AP_RegisterSlotDataIntCallback("random_npcs", f_option_random_npcs);
	AP_RegisterSlotDataIntCallback("random_monsters", f_option_random_monsters);
	AP_RegisterSlotDataIntCallback("random_rewards", f_option_random_rewards);
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


int AP::get_progressive_sword_level()
{
	const auto& ram = *m_info.ram;

	int level = 0;
	level += (int)ram[0x03C2]; // inv count
	if (ram[0x03BD] != 0xFF) level++; // Equipped

	return level;
}


int AP::get_progressive_armor_level()
{
	const auto& ram = *m_info.ram;

	int level = 0;
	level += (int)ram[0x03C3]; // inv count
	if (ram[0x03BE] != 0xFF) level++; // Equipped

	return level;
}


int AP::get_progressive_shield_level()
{
	const auto& ram = *m_info.ram;

	int level = 0;
	level += (int)ram[0x03C4]; // inv count
	if (ram[0x03BF] != 0xFF) level++; // Equipped

	return level;
}
		

#define SET_ITEM_TILES(id, tile0, tile1, tile2, tile3) \
{ \
	m_info.rom[0x0002B53C - 6 * 4 + (id - 0x90) * 4 + 0] = tile0; \
	m_info.rom[0x0002B53C - 6 * 4 + (id - 0x90) * 4 + 1] = tile1; \
	m_info.rom[0x0002B53C - 6 * 4 + (id - 0x90) * 4 + 2] = tile2; \
	m_info.rom[0x0002B53C - 6 * 4 + (id - 0x90) * 4 + 3] = tile3; \
}


void AP::update_progressive_sword_sprites()
{
	switch (get_progressive_sword_level())
	{
		case 0: // Hand Dagger
			copy_sprite(TILE_ADDR(0x4A), DST_TILE_ID(56), 0, false);
			copy_sprite(TILE_ADDR(0x4B), DST_TILE_ID(57), 0, false);
			copy_sprite(TILE_ADDR(0x8F), DST_TILE_ID(58), 0, false);
			copy_sprite(TILE_ADDR(0x4C), DST_TILE_ID(59), 0, false);
			SET_ITEM_TILES(AP_ITEM_PROGRESSIVE_SWORD, 0x4A, 0x4B, 0x8F, 0x4C);
			break;
		case 1: // Long Sword
			copy_sprite(TILE_ADDR(0x47), DST_TILE_ID(56), 0, false);
			copy_sprite(TILE_ADDR(0x48), DST_TILE_ID(57), 0, false);
			copy_sprite(TILE_ADDR(0x8F), DST_TILE_ID(58), 0, false);
			copy_sprite(TILE_ADDR(0x49), DST_TILE_ID(59), 0, false);
			SET_ITEM_TILES(AP_ITEM_PROGRESSIVE_SWORD, 0x47, 0x48, 0x8F, 0x49);
			break;
		case 2: // Giant Sword
			copy_sprite(TILE_ADDR(0x44), DST_TILE_ID(56), 0, false);
			copy_sprite(TILE_ADDR(0x45), DST_TILE_ID(57), 0, false);
			copy_sprite(TILE_ADDR(0x8F), DST_TILE_ID(58), 0, false);
			copy_sprite(TILE_ADDR(0x46), DST_TILE_ID(59), 0, false);
			SET_ITEM_TILES(AP_ITEM_PROGRESSIVE_SWORD, 0x44, 0x45, 0x8F, 0x46);
			break;
		case 3: // Dragon Slayer
			copy_sprite(SRC_TILE(0x0001D006, 0), DST_TILE_ID(56), 0, false);
			copy_sprite(SRC_TILE(0x0001D006, 1), DST_TILE_ID(57), 0, false);
			copy_sprite(SRC_TILE(0x0001D006, 2), DST_TILE_ID(58), 0, false);
			copy_sprite(SRC_TILE(0x0001D006, 3), DST_TILE_ID(59), 0, false);
			SET_ITEM_TILES(AP_ITEM_PROGRESSIVE_SWORD, 0x4D, 0x4E, 0x4F, 0x50);
			break;
	}
}

void AP::update_progressive_armor_sprites()
{
	switch (get_progressive_armor_level())
	{
		case 1: // Studded Mail
			copy_sprite(TILE_ADDR(0x55), DST_TILE_ID(60), 0, false);
			copy_sprite(TILE_ADDR(0x56), DST_TILE_ID(61), 0, false);
			copy_sprite(TILE_ADDR(0x57), DST_TILE_ID(62), 0, false);
			copy_sprite(TILE_ADDR(0x58), DST_TILE_ID(63), 0, false);
			SET_ITEM_TILES(AP_ITEM_PROGRESSIVE_ARMOR, 0x55, 0x56, 0x57, 0x58);
			break;
		case 2: // Full Plate
			copy_sprite(TILE_ADDR(0x59), DST_TILE_ID(60), 0, false);
			copy_sprite(TILE_ADDR(0x5A), DST_TILE_ID(61), 0, false);
			copy_sprite(TILE_ADDR(0x5B), DST_TILE_ID(62), 0, false);
			copy_sprite(TILE_ADDR(0x5C), DST_TILE_ID(63), 0, false);
			SET_ITEM_TILES(AP_ITEM_PROGRESSIVE_ARMOR, 0x59, 0x5A, 0x5B, 0x5C);
			break;
		case 3: // Battle Suit
			copy_sprite(SRC_TILE(0x0001CFA6, 0), DST_TILE_ID(60), 0, false);
			copy_sprite(SRC_TILE(0x0001CFA6, 0), DST_TILE_ID(61), 1, false);
			copy_sprite(SRC_TILE(0x0001CFA6, 1), DST_TILE_ID(62), 0, false);
			copy_sprite(SRC_TILE(0x0001CFA6, 1), DST_TILE_ID(63), 1, false);
			SET_ITEM_TILES(AP_ITEM_PROGRESSIVE_ARMOR, 0x5D, 0x5E, 0x5F, 0x60);
			break;
	}
}


void AP::update_progressive_shield_sprites()
{
	switch (get_progressive_shield_level())
	{
		case 0: // Small Shield
			copy_sprite(TILE_ADDR(0x61), DST_TILE_ID(64), 0, true);
			copy_sprite(TILE_ADDR(0x62), DST_TILE_ID(65), 0, true);
			copy_sprite(TILE_ADDR(0x63), DST_TILE_ID(66), 0, true);
			copy_sprite(TILE_ADDR(0x64), DST_TILE_ID(67), 0, true);
			SET_ITEM_TILES(AP_ITEM_PROGRESSIVE_SHIELD, 0x61, 0x62, 0x63, 0x64);
			break;
		case 1: // Large Shield
			copy_sprite(TILE_ADDR(0x65), DST_TILE_ID(64), 0, true);
			copy_sprite(TILE_ADDR(0x66), DST_TILE_ID(65), 0, true);
			copy_sprite(TILE_ADDR(0x67), DST_TILE_ID(66), 0, true);
			copy_sprite(TILE_ADDR(0x68), DST_TILE_ID(67), 0, true);
			SET_ITEM_TILES(AP_ITEM_PROGRESSIVE_SHIELD, 0x65, 0x66, 0x67, 0x68);
			break;
		case 2: // Magic Shield
			copy_sprite(TILE_ADDR(0x69), DST_TILE_ID(64), 0, true);
			copy_sprite(TILE_ADDR(0x6A), DST_TILE_ID(65), 0, true);
			copy_sprite(TILE_ADDR(0x6B), DST_TILE_ID(66), 0, true);
			copy_sprite(TILE_ADDR(0x6C), DST_TILE_ID(67), 0, true);
			SET_ITEM_TILES(AP_ITEM_PROGRESSIVE_SHIELD, 0x69, 0x6A, 0x6B, 0x6C);
			break;
		case 3: // Battle Helmet
			copy_sprite(SRC_TILE(0x0001CFC6, 0), DST_TILE_ID(64), 0, false);
			copy_sprite(SRC_TILE(0x0001CFC6, 1), DST_TILE_ID(65), 0, false);
			copy_sprite(SRC_TILE(0x0001CFC6, 2), DST_TILE_ID(66), 0, false);
			copy_sprite(SRC_TILE(0x0001CFC6, 3), DST_TILE_ID(67), 0, false);
			SET_ITEM_TILES(AP_ITEM_PROGRESSIVE_SHIELD, 0x69, 0x6A, 0x6B, 0x6C);
			break;
	}
}


void AP::patch_items()
{
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
	
	uint8_t ap_tile_data[4 * 16];
			
	// Replace Book with AP
	load_tiles_from_png("assets/images/ap_inv.png", ap_tile_data);
	copy_sprite(ap_tile_data + 0 * 16, TILE_ADDR(0x1E), 0, false);
	copy_sprite(ap_tile_data + 1 * 16, TILE_ADDR(0x1F), 0, false);
	copy_sprite(ap_tile_data + 2 * 16, TILE_ADDR(0x20), 0, false);
	copy_sprite(ap_tile_data + 3 * 16, TILE_ADDR(0x21), 0, false);
			
	// Replace empty tiles at the end of tileset with AP Progression (Hopefully not used)
	load_tiles_from_png("assets/images/ap_prog_inv.png", ap_tile_data);
	copy_sprite(ap_tile_data + 0 * 16, TILE_ADDR(0x8B), 0, false);
	copy_sprite(ap_tile_data + 1 * 16, TILE_ADDR(0x8C), 0, false);
	copy_sprite(ap_tile_data + 2 * 16, TILE_ADDR(0x8D), 0, false);
	copy_sprite(ap_tile_data + 3 * 16, TILE_ADDR(0x8E), 0, false);

	std::vector<uint8_t> entity_to_item_table = {
		0x80, 0x81, 0x82, 0x83, // Rings
		0x87, 0x86, 0x85, 0x84, 0x88, // Keys
		0x60, 0x61, 0x62, 0x63, 0x64, // Magics

		// Progressives
		AP_ITEM_PROGRESSIVE_SWORD,
		AP_ITEM_PROGRESSIVE_ARMOR,
		AP_ITEM_PROGRESSIVE_SHIELD,

		// Tools
		AP_ITEM_SPRING_ELIXIR,
		0x89, // Mattock
		AP_ITEM_AP_WINGBOOTS, // Wing boots
		0x94, // Black Onyx
		0x8A, // Magical Rod
		0x93, // Pendant

		// Consumables
		0x90, // Red Potion
		0x92, // Elixir
		AP_ITEM_POISON,
		AP_ITEM_OINTMENT,
		AP_ITEM_GLOVE,
		0x8D, // Hour Glass

		0xFF, 0xFF, // AP
		0xFF, // NULL
	};

	auto entity_to_item_table_addr = patcher->patch_new_code(15, entity_to_item_table);

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
				OP_JSR(0xF785), // Divide by 320
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
				1, 1, 1, // Progressive items
				2, 0, 2, 2, 0, 1, // Tools
				1, 0, 0, 0, 0, 3, // Consumables
				3, 3, // AP
				0, // NULL
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

			int tile_id = 0;
#define DST_TILE() DST_TILE_ID(tile_id++)

			// Ring of elf
			copy_sprite(TILE_ADDR(0x06), DST_TILE(), 0, true);
			copy_sprite(TILE_ADDR(0x07), DST_TILE(), 0, true);
			copy_sprite(TILE_ADDR(0x0C), DST_TILE(), 0, false);
			copy_sprite(TILE_ADDR(0x0D), DST_TILE(), 0, false);

			// Ruby
			copy_sprite(TILE_ADDR(0x08), DST_TILE(), 0, true);
			copy_sprite(TILE_ADDR(0x09), DST_TILE(), 0, true);
			copy_sprite(TILE_ADDR(0x0C), DST_TILE(), 0, false);
			copy_sprite(TILE_ADDR(0x0D), DST_TILE(), 0, false);

			// Dworf
			copy_sprite(TILE_ADDR(0x0A), DST_TILE(), 0, false);
			copy_sprite(TILE_ADDR(0x0B), DST_TILE(), 0, false);
			copy_sprite(TILE_ADDR(0x0C), DST_TILE(), 0, false);
			copy_sprite(TILE_ADDR(0x0D), DST_TILE(), 0, false);

			// Demons
			copy_sprite(TILE_ADDR(0x04), DST_TILE(), 0, false);
			copy_sprite(TILE_ADDR(0x05), DST_TILE(), 0, false);
			copy_sprite(TILE_ADDR(0x0C), DST_TILE(), 0, false);
			copy_sprite(TILE_ADDR(0x0D), DST_TILE(), 0, false);

			// Jack
			copy_sprite(TILE_ADDR(0x73), DST_TILE(), 0, true);
			copy_sprite(TILE_ADDR(0x27), DST_TILE(), 0, true);
			copy_sprite(TILE_ADDR(0x74), DST_TILE(), 0, true);
			copy_sprite(TILE_ADDR(0x29), DST_TILE(), 0, true);

			// Queen
			copy_sprite(TILE_ADDR(0x71), DST_TILE(), 0, true);
			copy_sprite(TILE_ADDR(0x27), DST_TILE(), 0, true);
			copy_sprite(TILE_ADDR(0x72), DST_TILE(), 0, true);
			copy_sprite(TILE_ADDR(0x29), DST_TILE(), 0, true);
			
			// King
			copy_sprite(TILE_ADDR(0x2A), DST_TILE(), 0, true);
			copy_sprite(TILE_ADDR(0x27), DST_TILE(), 0, true);
			copy_sprite(TILE_ADDR(0x2B), DST_TILE(), 0, true);
			copy_sprite(TILE_ADDR(0x29), DST_TILE(), 0, true);
			
			// Ace
			copy_sprite(TILE_ADDR(0x26), DST_TILE(), 0, true);
			copy_sprite(TILE_ADDR(0x27), DST_TILE(), 0, true);
			copy_sprite(TILE_ADDR(0x28), DST_TILE(), 0, true);
			copy_sprite(TILE_ADDR(0x29), DST_TILE(), 0, true);
			
			// Joker
			copy_sprite(TILE_ADDR(0x75), DST_TILE(), 0, true);
			copy_sprite(TILE_ADDR(0x27), DST_TILE(), 0, true);
			copy_sprite(TILE_ADDR(0x76), DST_TILE(), 0, true);
			copy_sprite(TILE_ADDR(0x29), DST_TILE(), 0, true);
			
			// Deluge
			copy_sprite(TILE_ADDR(0x7F), DST_TILE(), 0, true);
			copy_sprite(TILE_ADDR(0x80), DST_TILE(), 0, true);
			copy_sprite(TILE_ADDR(0x7F), DST_TILE(), 2, true);
			copy_sprite(TILE_ADDR(0x80), DST_TILE(), 2, true);
			
			// Thunder
			copy_sprite(TILE_ADDR(0x83), DST_TILE(), 0, true);
			copy_sprite(TILE_ADDR(0x84), DST_TILE(), 0, true);
			copy_sprite(TILE_ADDR(0x83), DST_TILE(), 2, true);
			copy_sprite(TILE_ADDR(0x84), DST_TILE(), 2, true);
			
			// Fire
			copy_sprite(TILE_ADDR(0x87), DST_TILE(), 0, true);
			copy_sprite(TILE_ADDR(0x88), DST_TILE(), 0, true);
			copy_sprite(TILE_ADDR(0x87), DST_TILE(), 2, true);
			copy_sprite(TILE_ADDR(0x88), DST_TILE(), 2, true);
			
			// Death
			copy_sprite(TILE_ADDR(0x7B), DST_TILE(), 0, true);
			copy_sprite(TILE_ADDR(0x7C), DST_TILE(), 0, true);
			copy_sprite(TILE_ADDR(0x7D), DST_TILE(), 0, true);
			copy_sprite(TILE_ADDR(0x7E), DST_TILE(), 0, true);
			
			// Tilte
			copy_sprite(TILE_ADDR(0x77), DST_TILE(), 0, true);
			copy_sprite(TILE_ADDR(0x78), DST_TILE(), 0, true);
			copy_sprite(TILE_ADDR(0x79), DST_TILE(), 0, true);
			copy_sprite(TILE_ADDR(0x7A), DST_TILE(), 0, true);
			
			// Progressive Sword
			copy_sprite(SRC_TILE(0x0001D006, 0), DST_TILE(), 0, false);
			copy_sprite(SRC_TILE(0x0001D006, 1), DST_TILE(), 0, false);
			copy_sprite(SRC_TILE(0x0001D006, 2), DST_TILE(), 0, false);
			copy_sprite(SRC_TILE(0x0001D006, 3), DST_TILE(), 0, false);
			
			// Progressive Armor
			copy_sprite(SRC_TILE(0x0001CFA6, 0), DST_TILE(), 0, false);
			copy_sprite(SRC_TILE(0x0001CFA6, 0), DST_TILE(), 1, false);
			copy_sprite(SRC_TILE(0x0001CFA6, 1), DST_TILE(), 0, false);
			copy_sprite(SRC_TILE(0x0001CFA6, 1), DST_TILE(), 1, false);
			
			// Progressive Shield
			copy_sprite(TILE_ADDR(0x69), DST_TILE(), 0, true);
			copy_sprite(TILE_ADDR(0x6A), DST_TILE(), 0, true);
			copy_sprite(TILE_ADDR(0x6B), DST_TILE(), 0, true);
			copy_sprite(TILE_ADDR(0x6C), DST_TILE(), 0, true);
			
			// Spring Elixir
			copy_sprite(SRC_TILE(0x0001CDC6, 0), DST_TILE(), 0, true);
			copy_sprite(SRC_TILE(0x0001CDC6, 1), DST_TILE(), 0, true);
			copy_sprite(SRC_TILE(0x0001CDC6, 2), DST_TILE(), 0, true);
			copy_sprite(SRC_TILE(0x0001CDC6, 3), DST_TILE(), 0, true);
			
			// Mattock
			copy_sprite(SRC_TILE(0x0001CE26, 0), DST_TILE(), 0, false);
			copy_sprite(SRC_TILE(0x0001CE26, 0), DST_TILE(), 1, false);
			copy_sprite(SRC_TILE(0x0001CE26, 1), DST_TILE(), 0, false);
			copy_sprite(SRC_TILE(0x0001CE26, 1), DST_TILE(), 1, false);
			
			// Wingboots
			copy_sprite(SRC_TILE(0x0001CF06, 0), DST_TILE(), 0, false);
			copy_sprite(SRC_TILE(0x0001CF06, 1), DST_TILE(), 0, false);
			copy_sprite(SRC_TILE(0x0001CF06, 2), DST_TILE(), 0, false);
			copy_sprite(SRC_TILE(0x0001CF06, 3), DST_TILE(), 0, false);
			
			// Black Onyx
			copy_sprite(SRC_TILE(0x0001CD46, 0), DST_TILE(), 0, false);
			copy_sprite(SRC_TILE(0x0001CD46, 0), DST_TILE(), 1, false);
			copy_sprite(SRC_TILE(0x0001CD46, 1), DST_TILE(), 0, false);
			copy_sprite(SRC_TILE(0x0001CD46, 1), DST_TILE(), 1, false);
			
			// Magical Rod
			copy_sprite(SRC_TILE(0x0001CF66, 0), DST_TILE(), 0, false);
			copy_sprite(SRC_TILE(0x0001CF66, 1), DST_TILE(), 0, false);
			copy_sprite(SRC_TILE(0x0001CF66, 2), DST_TILE(), 0, false);
			copy_sprite(SRC_TILE(0x0001CF66, 3), DST_TILE(), 0, false);
			
			// Pendant
			copy_sprite(SRC_TILE(0x0001CD66, 0), DST_TILE(), 0, false);
			copy_sprite(SRC_TILE(0x0001CD66, 1), DST_TILE(), 0, false);
			copy_sprite(SRC_TILE(0x0001CD66, 2), DST_TILE(), 0, false);
			copy_sprite(SRC_TILE(0x0001CD66, 3), DST_TILE(), 0, false);
			
			// Red Potion
			copy_sprite(SRC_TILE(0x0001CDA6, 0), DST_TILE(), 0, false, 4);
			copy_sprite(SRC_TILE(0x0001CDA6, 0), DST_TILE(), 0, false, -4);
			copy_sprite(SRC_TILE(0x0001CDA6, 1), DST_TILE(), 0, false, 4);
			copy_sprite(SRC_TILE(0x0001CDA6, 1), DST_TILE(), 0, false, -4);
			
			// Elixir
			copy_sprite(SRC_TILE(0x0001CDC6, 0), DST_TILE(), 0, false);
			copy_sprite(SRC_TILE(0x0001CDC6, 1), DST_TILE(), 0, false);
			copy_sprite(SRC_TILE(0x0001CDC6, 2), DST_TILE(), 0, false);
			copy_sprite(SRC_TILE(0x0001CDC6, 3), DST_TILE(), 0, false);
			
			// Poison
			copy_sprite(SRC_TILE(0x0001CDA6, 0), DST_TILE(), 0, false, 4);
			copy_sprite(SRC_TILE(0x0001CDA6, 0), DST_TILE(), 0, false, -4);
			copy_sprite(SRC_TILE(0x0001CDA6, 1), DST_TILE(), 0, false, 4);
			copy_sprite(SRC_TILE(0x0001CDA6, 1), DST_TILE(), 0, false, -4);
			
			// Ointment
			copy_sprite(SRC_TILE(0x0001CE06, 0), DST_TILE(), 0, false);
			copy_sprite(SRC_TILE(0x0001CE06, 0), DST_TILE(), 1, false);
			copy_sprite(SRC_TILE(0x0001CE06, 1), DST_TILE(), 0, false);
			copy_sprite(SRC_TILE(0x0001CE06, 1), DST_TILE(), 1, false);
			
			// Glove
			copy_sprite(SRC_TILE(0x0001CD26, 0), DST_TILE(), 0, false, 4);
			copy_sprite(SRC_TILE(0x0001CD26, 0), DST_TILE(), 0, false, -4);
			copy_sprite(SRC_TILE(0x0001CD26, 1), DST_TILE(), 0, false, 4);
			copy_sprite(SRC_TILE(0x0001CD26, 1), DST_TILE(), 0, false, -4);
			
			// Hour Glass
			copy_sprite(SRC_TILE(0x0001CF46, 0), DST_TILE(), 0, false);
			copy_sprite(SRC_TILE(0x0001CF46, 0), DST_TILE(), 1, false);
			copy_sprite(SRC_TILE(0x0001CF46, 1), DST_TILE(), 0, false);
			copy_sprite(SRC_TILE(0x0001CF46, 1), DST_TILE(), 1, false);

			uint8_t ap_tile_data[4 * 16];
			
			// AP
			load_tiles_from_png("assets/images/ap_world.png", ap_tile_data);
			copy_sprite(ap_tile_data + 0 * 16, DST_TILE(), 0, false);
			copy_sprite(ap_tile_data + 1 * 16, DST_TILE(), 0, false);
			copy_sprite(ap_tile_data + 2 * 16, DST_TILE(), 0, false);
			copy_sprite(ap_tile_data + 3 * 16, DST_TILE(), 0, false);
			
			// AP Progression
			load_tiles_from_png("assets/images/ap_prog_world.png", ap_tile_data);
			copy_sprite(ap_tile_data + 0 * 16, DST_TILE(), 0, false);
			copy_sprite(ap_tile_data + 1 * 16, DST_TILE(), 0, false);
			copy_sprite(ap_tile_data + 2 * 16, DST_TILE(), 0, false);
			copy_sprite(ap_tile_data + 3 * 16, DST_TILE(), 0, false);
			
			// Null
			uint8_t NULL_TILE[16] = {0};
			copy_sprite(NULL_TILE, DST_TILE(), 0, false);
			copy_sprite(NULL_TILE, DST_TILE(), 0, false);
			copy_sprite(NULL_TILE, DST_TILE(), 0, false);
			copy_sprite(NULL_TILE, DST_TILE(), 0, false);

			patcher->advance_new_code(9, (16 + 13) * 4 * 16);
		}

		// Touching an item entity
		{
			auto touched_new_item_addr = patcher->patch_new_code(15, {
				OP_CMP_IMM(0x9F),
				OP_BNE(1),
				OP_RTS(),

				// Don't show dialog or play sound if it's poison. It will have it's own dialog later
				OP_AND_IMM(0x1F),
				OP_CMP_IMM(AP_ENTITY_POISON & 0x7F),
				OP_BEQ(38 + 9 + 5),

				// Change item id for dialog if AP
				OP_CMP_IMM(AP_ENTITY_AP & 0x7F),
				OP_BEQ(4),
				OP_CMP_IMM(AP_ENTITY_AP_PROGRESSION & 0x7F),
				OP_BNE(30),
				OP_LDA_IMM(0x85),
				OP_STA_ABS(0x6000),
				OP_LDA_ZPG(0x24), // World id
				OP_STA_ABS(0x6000),
				OP_LDA_ZPG(0x63), // Screen id
				OP_STA_ABS(0x6000),
				OP_LDX_ABS(0x0378),
				OP_LDA_ZPGX(0xBA), // X
				OP_STA_ABS(0x6000),
				OP_LDA_ZPGX(0xC2), // Y
				OP_STA_ABS(0x6000),
				OP_LDA_IMM(EXTRA_ITEMS_COUNT),

				// Show dialog
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
				OP_AND_IMM(0x1F),
				OP_TAX(),
				OP_LDA_ABSX(entity_to_item_table_addr),
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

			m_info.external_interface->register_callback(0x85, [patcher, this](uint8_t world, uint8_t screen, uint8_t x, uint8_t y) -> uint8_t
			{
				auto found_scout = get_scout_location((int)world, (int)screen, (int)x, (int)y);
				if (!found_scout)
				{
					// Not found
					printf("Cannot find location. World %i, Screen %i\n", (int)world, (int)screen);
					return 0;
				}

				patcher->patch_ap_message(found_scout->dialog);
				return 0;
			}, 4);
		}
	}
#endif

	// New items text and tiles
	{
		// Text lo addr byte needs to be 0x4D. We're wasting a few bytes (26)
		auto text_addr = patcher->get_new_code_addr(12);
		if ((text_addr & 0xFF) < 0x4D)
		{
			auto new_code_addr = (text_addr & 0xFF00) | 0x4D;
			patcher->advance_new_code(12, new_code_addr - text_addr);
			text_addr = new_code_addr;
		}
		else if ((text_addr & 0xFF) > 0x4D)
		{
			auto new_code_addr = ((text_addr & 0xFF00) + 0x0100) | 0x4D;
			patcher->advance_new_code(12, new_code_addr - text_addr);
			text_addr = new_code_addr;
		}

		// Move old texts from second page of items into a new addr with empty space so we can jump higher up and use this space instead
		memcpy(ROM_LO(12, text_addr), ROM_LO(12, 0x9D4D), 16 * 6);
		patcher->advance_new_code(12, 6 * 16);

		// Add new items (There are unused items in the game, but we will avoid them incase they might trigger buffs/nerfs we don't know about)
	#define ADD_ITEM(name, id, tile0, tile1, tile2, tile3) \
		{ \
			auto item_text_addr = patcher->patch_new_code(12, { \
				(uint8_t)strlen(name), 0x20, 0x20, 0x20, \
				0x20, 0x20, 0x20, 0x20, \
				0x20, 0x20, 0x20, 0x20, \
				0x20, 0x20, 0x20, 0x20 \
			}); \
			memcpy(ROM_LO(12, item_text_addr + 1), name, strlen(name)); \
			m_info.rom[0x0002B53C - 6 * 4 + (id - 0x90) * 4 + 0] = tile0; \
			m_info.rom[0x0002B53C - 6 * 4 + (id - 0x90) * 4 + 1] = tile1; \
			m_info.rom[0x0002B53C - 6 * 4 + (id - 0x90) * 4 + 2] = tile2; \
			m_info.rom[0x0002B53C - 6 * 4 + (id - 0x90) * 4 + 3] = tile3; \
		}

		ADD_ITEM("PROG SWORD", AP_ITEM_PROGRESSIVE_SWORD, 0x4D, 0x4E, 0x4F, 0x50);
		ADD_ITEM("PROG ARMOR", AP_ITEM_PROGRESSIVE_ARMOR, 0x5D, 0x5E, 0x5F, 0x60);
		ADD_ITEM("PROG SHIELD", AP_ITEM_PROGRESSIVE_SHIELD, 0x69, 0x6A, 0x6B, 0x6C);
		ADD_ITEM("POISON", AP_ITEM_POISON, 0x30, 0x31, 0x32, 0x33);
		ADD_ITEM("OINTMENT", AP_ITEM_OINTMENT, 0x40, 0x41, 0x42, 0x43);
		ADD_ITEM("GLOVE", AP_ITEM_GLOVE, 0x16, 0x17, 0x18, 0x19);
		ADD_ITEM("SPRING ELIXIR", AP_ITEM_SPRING_ELIXIR, 0x12, 0x13, 0x14, 0x15);
		ADD_ITEM("UNLOCK BOOTS", AP_ITEM_AP_WINGBOOTS, 0x22, 0x23, 0x24, 0x25);
		auto ap_text_addr = patcher->get_new_code_addr(12);
		ADD_ITEM("AP", AP_ITEM_AP, 0x1E, 0x1F, 0x20, 0x21);
		auto ap_prog_text_addr = patcher->get_new_code_addr(12);
		ADD_ITEM("AP PROG", AP_ITEM_AP_PROGRESSION, 0x8B, 0x8C, 0x8D, 0x8E);

		// Null item replaces fire crystal
		memcpy(ROM_LO(12, text_addr + 5 * 16 + 1), "SOLD OUT       ", 15);
		m_info.rom[0x0002B53C - 6 * 4 + (AP_ITEM_NULL - 0x90) * 4 + 0] = 0x8F;
		m_info.rom[0x0002B53C - 6 * 4 + (AP_ITEM_NULL - 0x90) * 4 + 1] = 0x8F;
		m_info.rom[0x0002B53C - 6 * 4 + (AP_ITEM_NULL - 0x90) * 4 + 2] = 0x8F;
		m_info.rom[0x0002B53C - 6 * 4 + (AP_ITEM_NULL - 0x90) * 4 + 3] = 0x8F;

		auto read_callback = [this](int addr)
		{
			// Get current world / screen that will help us find the location
			auto world_id = m_info.ram->get(0x24);
			auto screen_id = m_info.ram->get(0x63);
			auto shop_index = m_info.ram->get(0x1F4); // Find the shop index in the stack... very hacky!

			// Find location
			const ap_location_t* location = nullptr;
			for (const auto& loc : AP_LOCATIONS)
			{
				if (loc.world == world_id &&
					loc.screen == screen_id)
				{
					if (loc.type == ap_location_type_t::shop)
					{
						if (loc.shop_index == shop_index)
						{
							location = &loc;
							break;
						}
					}
					else if (loc.type == ap_location_type_t::give)
					{
						location = &loc;
						break;
					}
				}
			}
			if (!location)
			{
				printf("Location not found");
				return;
			}

			// Find associated scout
			const ap_location_scout_t* scout = nullptr;
			for (const auto& s : m_location_scouts)
			{
				if (s.loc == location)
				{
					scout = &s;
					break;
				}
			}
			if (!scout)
			{
				printf("Scout not found");
				return;
			}

			memset(m_info.rom + addr, 0x20, 16);
			m_info.rom[addr] = (uint8_t)scout->item_player_name.size();
			memcpy(m_info.rom + addr + 1, scout->item_player_name.c_str(), scout->item_player_name.size());
		};

		m_info.cart->register_read_callback(read_callback, BANK_ADDR_LO(12, ap_text_addr));
		m_info.cart->register_read_callback(read_callback, BANK_ADDR_LO(12, ap_prog_text_addr));

		// Write new code in bank12 that allows to jump further to index the new text
		auto addr = patcher->patch_new_code(12, {
			OP_BCC(4),

			OP_LDY_IMM(HI(text_addr)),
			OP_STY_ZPG(0xED),

			OP_TAY(),
			OP_JMP_ABS(0x8E9B),
		});

		patcher->patch(12, 0x8C50, 0, {
			OP_JMP_ABS(addr),
		});
	}
	

	// Trying to buy boots shouldn't work until we have unlocked them
	{
		auto addr = patcher->patch_new_code(12, {
			OP_LDA_ZPG(0x19),
			OP_BMI(3), // Is A pressed?
			OP_JMP_ABS(0x84F7), // Go back after the bmi check in the original code

			// Check if null item
			OP_LDX_ABS(0x021E),
			OP_LDA_ABSX(0x0220), // Selected item
			OP_CMP_IMM(AP_ITEM_NULL),
			OP_BEQ(15),

			// Check if wingboots
			OP_CMP_IMM(0x8F),
			OP_BEQ(3),
			OP_JMP_ABS(0x84FD), // not wing boots, proceed with buy

			// Check if we have unlocked wingboots
			OP_LDA_ABS(0x819),
			OP_BEQ(3),
			OP_JMP_ABS(0x84FD), // We can buy

			// Play denied sound, loop back
			OP_LDA_IMM(0x0D),
			OP_JSR(0xD0E4),
			OP_JMP_ABS(0x84ED),
		});
		patcher->patch(12, 0x84F3, 0, { OP_JMP_ABS(addr) });
	}

	
	// New item dialogs
	{
		// They need to be aligned on page, so we will waste some bytes here
		auto dialogs_addr = patcher->get_new_code_addr(12);
		auto aligned_addr = LO(dialogs_addr) ? ((dialogs_addr & 0xFF00) + 0x100) : dialogs_addr;
		patcher->advance_new_code(12, aligned_addr - dialogs_addr);
		dialogs_addr = aligned_addr;

		for (int i = 0; i < EXTRA_ITEMS_COUNT; ++i)
			patcher->patch_new_code(12, { 0x00, 0x01, (uint8_t)(0xC4 + i), 0x00 }); // 0x98
		patcher->patch_new_code(12, { 0x00, 0x01, 0xE4, 0x00 }); // 0xB8, AP entity dialog

		// We want to add dialogs for the new items. They are tightly packed into the
		// bank 12. Modify the code to jump ahead further if message ID is too big
		auto get_dialog_addr_addr = patcher->patch_new_code(12, {
			OP_SEC(),
			OP_SBC_IMM(0x98),
			OP_BCC(9),

			// Greater or equal to 0x98, we go somewhere else
			OP_ASL_A(), OP_ASL_A(), // lo
			OP_STA_ZPG(0xDB),
			OP_LDA_IMM(HI(dialogs_addr)),
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
	}


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

		auto add_new_common = patcher->patch_new_code(12, {
			OP_LDX_IMM(0xFF),
			OP_INX(),
			OP_LDY_ABSX(0x03B5),
			OP_BNE(0xFA),
			OP_STA_ABSX(0x03B5),
			OP_INC_ABSX(COMMON_ITEM_COUNT_ADDR),
			OP_RTS(),
		});

		auto add_common = patcher->patch_new_code(12, {
			OP_AND_IMM(0x1F),
			OP_LDX_IMM(7), // We can hold 8 common inventory items
			OP_CMP_ABSX(0x03B5),
			OP_BEQ(6),
			OP_DEX(),
			OP_BPL(0xF8), // -8
			OP_JMP_ABS(add_new_common),
			OP_STA_ABSX(0x03B5),
			OP_INC_ABSX(COMMON_ITEM_COUNT_ADDR),
			OP_RTS(),
		});

		auto add_poison = patcher->patch_new_code(12, {
			OP_LDX_ABS(0x0800), // Input context. 0 = gameplay
			OP_BEQ(9),

			// Queue it C++ side. Then it will trigger when no dialog are active
			OP_LDX_IMM(0x83), // C++ Queue item
			OP_STX_ABS(0x6000),
			OP_STA_ABS(0x6000),
			OP_RTS(),

			OP_JSR(0xC83C), // Touched poison subroutine
			OP_RTS(),
		});

		auto add_null = patcher->patch_new_code(12, {
			OP_RTS(), // NULL item do nothing
		});

		auto add_ap = patcher->patch_new_code(12, {
			OP_RTS(), // AP item do nothing, the check is already done separately
		});

		auto add_wingboots_unlock = patcher->patch_new_code(12, {
			// Write it into ram
			OP_LDA_IMM(1),
			OP_STA_ABS(WINGBOOTS_UNLOCK_ADDR),
			OP_RTS(),
		});

		auto give_item = patcher->patch_new_code(12, {
			// Check if it's poison. We hurt player, we don't add to inventory
			OP_CMP_IMM(AP_ITEM_POISON),
			OP_BNE(3),
			OP_JMP_ABS(add_poison),

			// Check if it's NULL. We ignore and do nothing
			OP_CMP_IMM(AP_ITEM_NULL),
			OP_BNE(3),
			OP_JMP_ABS(add_null),

			// Check if it's the Wingboots unlock
			OP_CMP_IMM(AP_ITEM_AP_WINGBOOTS),
			OP_BNE(3),
			OP_JMP_ABS(add_wingboots_unlock),

			// Touched ap item
			OP_CMP_IMM(AP_ITEM_AP),
			OP_BNE(3),
			OP_JMP_ABS(add_ap),
			OP_CMP_IMM(AP_ITEM_AP_PROGRESSION),
			OP_BNE(3),
			OP_JMP_ABS(add_ap),

			// Check if our item is common
			OP_CMP_IMM(0x90), // Red Potion
			OP_BEQ(16),
			OP_CMP_IMM(AP_ITEM_OINTMENT),
			OP_BEQ(12),
			OP_CMP_IMM(AP_ITEM_GLOVE),
			OP_BEQ(8),
			OP_CMP_IMM(0x8F), // Wingboots
			OP_BEQ(4),
			OP_CMP_IMM(0x8D), // Hour Glass
			OP_BNE(3),
			OP_JMP_ABS(add_common),

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
			OP_JMP_ABS(0x9B06),
		});

		patcher->patch(12, 0x9B01, 0, { OP_JMP_ABS(give_item) });
	}

	// Spring elixir quest
	{
		patcher->patch(12, 0xA1B3, 0, { AP_ITEM_SPRING_ELIXIR }); // Check if has
		patcher->patch(12, 0xA1BC, 0, { AP_ITEM_SPRING_ELIXIR }); // Give

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
#if 0 // We do now
	{
		// Don't remove wing boots when used,
		// but remove them from selected item and put them back into inventory.
		patcher->patch(15, 0xC581, 0, {
			OP_NOP(), OP_NOP(), OP_NOP(),
		});

		// Trigger a reuse timeout of 2mins
	}
#endif

	// New inventory category
	{
		// Goes into ITEM (unique/persistent)
		//   Key Jack
		//   Key Queen
		//   Key King
		//   Key Ace
		//   Key Joker
		//   Spring Elixir
		//   Mattock

		// Goes into COMMON (Consumables)
		//   Red Potion
		//   Ointment
		//   Glove
		//   Hour Glass
		//   Wingboots

		// Add a row to the menu screen
		patcher->patch(12, 0x8AAC, 1, { 0x10 });

		// Copy existing menu text further in a free area, and add COMMON in it
		auto text_addr = patcher->get_new_code_addr(12);
		memcpy(ROM_LO(12, text_addr), ROM_LO(12, 0x8873), 5 * 16);
		memcpy(ROM_LO(12, text_addr) + 5 * 16, "\x6""COMMON""\x20\x20\x20\x20\x20\x20\x20\x20\x20", 16);
		memcpy(ROM_LO(12, text_addr) + 6 * 16, ROM_LO(12, 0x88C3), 16);
		patcher->advance_new_code(12, 7 * 16);

		// Replace the addresses to point to this new text
		patcher->patch(12, 0x8AB9, 1, { LO(text_addr) });
		patcher->patch(12, 0x8ABD, 1, { HI(text_addr) });

		// Same thing for the "no item/no weapon" text, but just we repeat "no item" for the common
		auto no_item_addr = patcher->patch_new_code(12, {
			OP_CPY_IMM(5 << 4),
			OP_BNE(3),
			OP_LDY_IMM(4 << 4),
			OP_TYA(),
			OP_JMP_ABS(0x8E9B),
		});

		patcher->patch(12, 0x8B3E, 0, { OP_JSR(no_item_addr)} );

		// We have 7 submenus now
		patcher->patch(12, 0x8A93, 1, { 0x07 });

		auto addr = patcher->patch_new_code(12, {
			OP_STX_ABS(0x0225),
			OP_INX(),
			OP_STX_ABS(0x0226),
			OP_RTS(),
		});
		patcher->patch(12, 0x8AD7, 0, { OP_JSR(addr) });

		// Player screen is now index 6
		patcher->patch(12, 0x8AF3, 1, { 0x06 });
	}

	// Patch all the places we're looking up the number of item in a sub inventory
	{
		auto inv_size_table_addr = patcher->patch_new_code(12, {
			4, 4, 4, 4, 127, 127
		});
		
		patcher->patch(12, 0x8434, 1, { PATCH_ADDR(inv_size_table_addr) });
		patcher->patch(12, 0x9B14, 1, { PATCH_ADDR(inv_size_table_addr) });

		auto load_ax_addr = patcher->patch_new_code(12, {
			OP_CPX_IMM(0x05),
			OP_BEQ(4),
			OP_LDA_ABSX(0x03C2),
			OP_RTS(),

			OP_LDY_IMM(0),
			OP_LDA_ABSY(0x03B5),
			OP_BEQ(5),
			OP_INY(),
			OP_CPY_IMM(8),
			OP_BNE(0xF6),

			OP_TYA(),
			OP_RTS(),
		});

		auto load_yx_addr = patcher->patch_new_code(12, {
			OP_CPX_IMM(0x05),
			OP_BEQ(4),

			OP_LDY_ABSX(0x03C2),
			OP_RTS(),

			OP_PHA(),

			OP_LDY_IMM(0),
			OP_LDA_ABSY(0x03B5),
			OP_BEQ(5),
			OP_INY(),
			OP_CPY_IMM(8),
			OP_BNE(0xF6),

			OP_PLA(),
			OP_RTS(),
		});

		auto load_xy_addr = patcher->patch_new_code(12, {
			OP_CPY_IMM(0x05),
			OP_BEQ(4),

			OP_LDX_ABSY(0x03C2),
			OP_RTS(),

			OP_PHA(),

			OP_LDX_IMM(0),
			OP_LDA_ABSX(0x03B5),
			OP_BEQ(5),
			OP_INX(),
			OP_CPX_IMM(8),
			OP_BNE(0xF6),
			
			OP_PLA(),
			OP_RTS(),
		});

		patcher->patch(12, 0x8431, 0, { OP_JSR(load_ax_addr) }); // Check if should show "no items" (hum no? That's shop)
		patcher->patch(12, 0x84D6, 0, { OP_JSR(load_yx_addr) });
		patcher->patch(12, 0x8671, 0, { OP_JSR(load_yx_addr) });
		patcher->patch(12, 0x8B17, 0, { OP_JSR(load_ax_addr) }); // How many items to draw
		patcher->patch(12, 0x8D37, 0, { OP_JSR(load_xy_addr) });
		patcher->patch(12, 0x9A99, 0, { OP_JSR(load_ax_addr) }); // Remove from inventory?
	}

	// Inventory offsets
	{
		auto los_addr = patcher->patch_new_code(12, {
			0x9D, 0xA1, 0xA5, 0xA9, 0xAD, 0xB5
		});
		auto his_addr = patcher->patch_new_code(12, {
			3, 3, 3, 3, 3, 3
		});

		patcher->patch(12, 0x84C4, 1, { PATCH_ADDR(los_addr) }); // This one is useles
		patcher->patch(12, 0x84C9, 1, { PATCH_ADDR(his_addr) });

		patcher->patch(12, 0x867E, 1, { PATCH_ADDR(los_addr) });
		patcher->patch(12, 0x8683, 1, { PATCH_ADDR(his_addr) });

		patcher->patch(12, 0x8B52, 1, { PATCH_ADDR(los_addr) });
		patcher->patch(12, 0x8B57, 1, { PATCH_ADDR(his_addr) });

		patcher->patch(12, 0x8D25, 1, { PATCH_ADDR(los_addr) });
		patcher->patch(12, 0x8D2A, 1, { PATCH_ADDR(his_addr) });

		patcher->patch(12, 0x9AA0, 1, { PATCH_ADDR(los_addr) });
		patcher->patch(12, 0x9AA5, 1, { PATCH_ADDR(his_addr) });

		patcher->patch(12, 0x9B07, 1, { PATCH_ADDR(los_addr) });
		patcher->patch(12, 0x9B0C, 1, { PATCH_ADDR(his_addr) });
	}

	// Load item from common inventory
	{
		auto fix_item_id_addr = patcher->patch_new_code(12, {
			OP_CMP_IMM(5),
			OP_BNE(2),
			OP_LDA_IMM(4),
			OP_JMP_ABS(0xF78B), // Multiply by 32
		});

		patcher->patch(12, 0x8C19, 0, { OP_JSR(fix_item_id_addr) }); // When displaying in inventory
		patcher->patch(12, 0x8BD9, 0, { OP_JSR(fix_item_id_addr) }); // When equipping
	}

	// Equip item
	{
		auto remove_common_item_addr = patcher->patch_new_code(12, {
			OP_PHA(),
			OP_AND_IMM(0x1F),
			OP_LDX_IMM(0),

			OP_CMP_ABSX(0x03B5),
			OP_BEQ(5),
			OP_INX(),
			OP_CPX_IMM(8), // Shouldn't reach this
			OP_BNE(0xF6), // -10

			OP_DEC_ABSX(COMMON_ITEM_COUNT_ADDR),
			OP_BNE(9 + 27),
			OP_LDA_IMM(0),
			OP_STA_ABSX(0x03B5),

			// Shift following items into place
			OP_CPX_IMM(7),
			OP_BEQ(27),

			OP_LDA_ABSX(0x03B5 + 1),
			OP_STA_ABSX(0x03B5),
			OP_LDA_ABSX(COMMON_ITEM_COUNT_ADDR + 1),
			OP_STA_ABSX(COMMON_ITEM_COUNT_ADDR),
			OP_INX(),
			OP_CPX_IMM(7),
			OP_BNE(0xEF), // -17

			// Put 0 in last item
			OP_LDA_IMM(0x00),
			OP_STA_ABS(0x03B5 + 7),
			OP_LDA_IMM(0),
			OP_STA_ABS(COMMON_ITEM_COUNT_ADDR + 7),

			OP_PLA(),
			OP_RTS(), // Equip item
		});

		auto addr = patcher->patch_new_code(12, {
			OP_PHA(),
			OP_LDA_ABS(0x020E), // Contains inventory index (5 = common)
			OP_CMP_IMM(5),
			OP_BNE(4),
			OP_PLA(),
			OP_JMP_ABS(remove_common_item_addr),
			OP_PLA(),
			OP_JMP_ABS(0x9A6A), // Original code to remove items
		});

		// 0x020E contains 5 for common
		patcher->patch(12, 0x8CB9, 0, { OP_JSR(addr) });
	}

	// Display count in common inventory
	{
		// 12:8C36 Display text
		// 15:F804 Display letter
		// 12:99F8 Draw shop items

		// RAM:
		//   21E = 5
		//   x = list index
		//   a = item id
		//   $EA = x position
		//   $EB = y position
		//   $EE = 0 ?
		//   y = 5 ?
		// 
		//  1600:
		//   $EC = 40
		//   $ED = 06

		auto draw_item_text_addr = patcher->patch_new_code(12, {
			OP_STX_ABS(0x020A), // Cache list index

			OP_LDY_ABS(0x021E),
			OP_CPY_IMM(5),
			OP_BEQ(3),
			OP_JMP_ABS(0x8C36), // Original code that draws the text

			// Common inventory, draw the count
			OP_JSR(0x8C36), // Draw item name
			OP_LDX_ABS(0x020A), // List index
			OP_LDY_ABSX(COMMON_ITEM_COUNT_ADDR), // Count
			OP_CPY_IMM(1),
			OP_BNE(1),
			OP_RTS(), // Don't display count if 1

			OP_STY_ZPG(0xEC),
			OP_LDY_IMM(0x00), // Number hi
			OP_STY_ZPG(0xED),
			OP_LDY_IMM(0x00), // Dunno, it has to be 0
			OP_STY_ZPG(0xEE),
			OP_LDY_IMM(0x13), // X position
			OP_STY_ZPG(0xEA),
			OP_LDY_IMM(3), // Right-align spacing
			OP_JSR(0xFA26), // Draw count

			OP_RTS(),
		});

		patcher->patch(12, 0x8C2A, 0, { OP_JSR(draw_item_text_addr) });
	}

	// Use common item
	{
		// Use ointment function
		auto use_ointment_addr = patcher->patch_new_code(15, {
			OP_JSR(0xC87A), // Touched Ointment
			OP_JSR(0xC4BF), // Remove Selected Item
			OP_RTS(),
		});

		// Use glove function
		auto use_glove_addr = patcher->patch_new_code(15, {
			OP_JSR(0xC7CF), // Touched Glove
			OP_JSR(0xC4BF), // Remove Selected Item
			OP_RTS(),
		});

		// Copy item callback table and add our new item handlers
		auto addr = patcher->get_new_code_addr(15);
		memcpy(ROM_HI(15, addr), ROM_HI(15, 0xC49D), 17 * 2);
		patcher->advance_new_code(15, 17 * 2);

		// 0xC49B means they don't do anything
		patcher->patch_new_code(15, { PATCH_ADDR(0xC49B) }); // Black potion
		patcher->patch_new_code(15, { PATCH_ADDR(0xC49B) }); // Elixir
		patcher->patch_new_code(15, { PATCH_ADDR(0xC49B) }); // Pendant
		patcher->patch_new_code(15, { PATCH_ADDR(0xC49B) }); // Black Onyx
		patcher->patch_new_code(15, { PATCH_ADDR(0xC49B) }); // Fire Crystal
		patcher->patch_new_code(15, { PATCH_ADDR(0xC49B) }); // Progressive Sword
		patcher->patch_new_code(15, { PATCH_ADDR(0xC49B) }); // Progressive Armor
		patcher->patch_new_code(15, { PATCH_ADDR(0xC49B) }); // Progressive Shield
		patcher->patch_new_code(15, { PATCH_ADDR(0xC49B) }); // Poison
		patcher->patch_new_code(15, { PATCH_ADDR(use_ointment_addr - 1) }); // Ointment
		patcher->patch_new_code(15, { PATCH_ADDR(use_glove_addr - 1) }); // Glove

		// Allow for the new items
		patcher->patch(15, 0xC490, 1, { 0x22 + 11 * 2 });

		// Point to the new table
		patcher->patch(15, 0xC494, 1, { PATCH_ADDR(addr + 1) });
		patcher->patch(15, 0xC498, 1, { PATCH_ADDR(addr) });
	}

	// Don't show price in store if item is NULL
	{
		//  1600:
		//   $EC = 40
		//   $ED = 06
		// 12:9A5C  <- calls to display price

		auto addr = patcher->patch_new_code(12, {
			OP_LDA_ZPG(0xED), // Hi byte of price
			OP_BMI(3), // Price too high, it's a NULL item
			OP_JMP_ABS(0xFA26), // Draw price
			OP_RTS(),
		});
		
		patcher->patch(12, 0x9A5C, 0, { OP_JSR(addr) });
	}

	// Was this corrupting everything? I set the common item max at 127
	// No more "can't carry anymore", all inventories should handle max (does thifs corrupt stuff?)
	//{
	//	patcher->patch(12, 0x8437, 0, { OP_NOP(), OP_NOP() });
	//}

	// King should give money even if not at zero, once.
	oSettings->setUserSetting("king_golds", "1");
	patcher->apply_king_golds_setting_patch();

	// Add door to Eolis.
	{
		// 3:885D Door locations addr
		// 3:8882 Door destination addr
		// 3:8413 Meta pointers

		// Move old door array to a new "hopefully" empty. Bank 3 is pretty tight with data.
		// According to Vagla's document, there is nothing after 0x0000E2??.
		// I did my own investigation and it seems right. Max offset = $2236.
		// So... round it up!
		// There seems to be legit data in that spot, but unsure what it is.
		// Gargabe? ROM garbage is usually 0xFF
		int new_offset = 0x3000;
		int new_addr = BANK_OFFSET(3, new_offset);
		memcpy(m_info.rom + new_addr, m_info.rom + 0x0000C85D, 9 * 4);

		// Add our new door
		m_info.rom[new_addr + 9 * 4 + 0] = 0x00; // Screen 0
		m_info.rom[new_addr + 9 * 4 + 1] = 0xA0; // x = 0, y = 10
		m_info.rom[new_addr + 9 * 4 + 2] = 0xFE; // Travel to previous world (Eolis)
		m_info.rom[new_addr + 9 * 4 + 3] = 0x9C; // Dest x = 12, y = 9
		m_info.rom[new_addr + 9 * 4 + 4] = 0xFF; // Series of door locations need to end with $FF

		// Change the meta pointer offset to point to this new location
		m_info.rom[0x0000C413 + 6] = LO(new_offset);
		m_info.rom[0x0000C413 + 7] = HI(new_offset);
	}

	// Queue / Dequeue items / location dialogs
	{
		m_info.external_interface->register_callback(0x83, [this](uint8_t item, uint8_t b, uint8_t c, uint8_t d)
		{
			m_queued_items.push_back(item);
			return 1;
		}, 1);

		m_info.external_interface->register_callback(0x84, [this](uint8_t a, uint8_t b, uint8_t c, uint8_t d) -> uint8_t
		{
			int poison_count = 0;
			while ((int)m_recv_item_queue.size() > m_item_received_count)
			{
				const auto& recv_item = m_recv_item_queue[m_item_received_count++];
				auto ap_item = get_ap_item(recv_item.item_id);
				if (ap_item && ap_item->name == "Poison")
				{
					++poison_count;
					if (poison_count > 1) continue; // Ignore subsequent poison, we got them while offline
				}

				if (recv_item.player_id != AP_GetPlayerID())
				{
					for (const auto& ap_item : AP_ITEMS)
					{
						if (ap_item.id == recv_item.item_id)
						{
							m_queued_items.push_back(ap_item.item_id);
							break;
						}
					}
				}
			}

			if (m_queued_items.empty()) return 0xFF;
			uint8_t item_id = m_queued_items.front();
			m_queued_items.erase(m_queued_items.begin());
			return item_id;
		}, 0);

		m_info.external_interface->register_callback(0x86, [this, patcher](uint8_t a, uint8_t b, uint8_t c, uint8_t d) -> uint8_t
		{
			if (m_remote_item_dialog_queue.empty()) return 0;
			const auto loc_id = m_remote_item_dialog_queue.front();
			m_remote_item_dialog_queue.erase(m_remote_item_dialog_queue.begin());
			const auto scout = get_scout_location(loc_id);
			if (!scout) return 0;
			patcher->patch_ap_message(scout->dialog);
			return 1;
		}, 0);

		auto addr = patcher->patch_new_code(15, {
			OP_JSR(0xE016), // Check if should show inventory
			OP_LDA_ABS(0x0800), // Input context flag
			OP_BEQ(1),
			OP_RTS(),

			// Dequeue location dialogs
			OP_LDA_IMM(0x86),
			OP_STA_ABS(0x6000),
			OP_LDA_ABS(0x6000),
			OP_BEQ(9),
			OP_LDA_IMM(EXTRA_ITEMS_COUNT + 0x98),
			OP_JSR(0xF859),
			0x0C, 0x41, 0x82, // I have no idea why this is needed after a dialog
			OP_RTS(),

			// Dequeue item
			OP_LDA_IMM(0x84),
			OP_STA_ABS(0x6000),
			OP_LDA_ABS(0x6000),
			OP_CMP_IMM(0xFF),
			OP_BNE(1),
			OP_RTS(),

			// Show dialog if not poison
			OP_CMP_IMM(AP_ITEM_POISON),
			OP_BEQ(10 + 10),

			// Show dialog. We need to find the matching entity for the item id.
			OP_LDX_IMM(0xFF),
			OP_INX(),
			OP_CMP_ABSX(entity_to_item_table_addr),
			OP_BNE(0xFA),
			OP_PHA(),
			OP_TXA(),

			OP_CLC(),
			OP_ADC_IMM(0x98),
			OP_JSR(0xF859),
			0x0C, 0x41, 0x82, // I have no idea why this is needed after a dialog
			OP_PLA(),

			// Give the item
			OP_TAX(),
			OP_LDA_ABS(0x0100), // Current bank
			OP_PHA(),
			OP_TXA(),
			OP_PHA(),
			OP_LDX_IMM(12), OP_JSR(0xCC1A), // Switch bank 12
			OP_PLA(),
			OP_JSR(0x9AF7), // Give item
			OP_PLA(),
			OP_TAX(),
			OP_JSR(0xCC1A), // Switch bank back

			OP_RTS(),
		});

		patcher->patch(15, 0xDB6E, 0, { OP_JSR(addr) });
	}

	// 15:C8CD Description: Stores an item in the next free slot in the item directory.
	// 12:8BED Clean dialog from screen when closing it.
}


void AP::patch_cpp_hooks()
{
	auto patcher = m_info.patcher;
	auto external_interface = m_info.external_interface;

	// Location check in world
	{
		auto addr = patcher->patch_new_code(14, {
			OP_PHA(),
			OP_LDA_IMM(0x80), // C++ message id
			OP_STA_ABS(0x6000),
			OP_LDA_ZPG(0x24), // World id
			OP_STA_ABS(0x6000),
			OP_LDA_ZPG(0x63), // Screen id
			OP_STA_ABS(0x6000),
			OP_LDA_ZPGX(0xBA), // X
			OP_STA_ABS(0x6000),
			OP_LDA_ZPGX(0xC2), // Y
			OP_STA_ABS(0x6000),
			OP_PLA(),
			OP_CMP_IMM(0x50),
			OP_BEQ(3),
			OP_JMP_ABS(0xC768), // Go back
			OP_JMP_ABS(0xC752), // Pickup mattock
		});

		patcher->patch(14, 0xC764, 0, { OP_JMP_ABS(addr) });

		external_interface->register_callback(0x80, [this](uint8_t world, uint8_t screen, uint8_t x, uint8_t y)
		{
			// Find the location
			auto found_scout = get_scout_location((int)world, (int)screen, (int)x, (int)y);
			if (!found_scout)
			{
				// Not found
				printf("Location not found. World %i, Screen %i\n", (int)world, (int)screen);
				return 0;
			}

			int64_t loc_id = found_scout->loc->id;

			if (m_locations_checked.count(loc_id))
			{
				printf("Location already checked. World %i, Screen %i\n", (int)world, (int)screen);
				return 0;
			}

			printf("Location checked! World %i, Screen %i\n", (int)world, (int)screen);
			m_locations_checked.insert(loc_id);
			patch_remove_check(loc_id);
			patch_dynamics();

			// Do location check!
			AP_SendItem(loc_id);
			return 1;
		}, 4);
	}

	// Location check in store
	{
		auto addr = patcher->patch_new_code(12, {
			OP_LDA_IMM(0x81), // C++ message id
			OP_STA_ABS(0x6000),
			OP_LDA_ZPG(0x24), // World id
			OP_STA_ABS(0x6000),
			OP_LDA_ZPG(0x63), // Screen id
			OP_STA_ABS(0x6000),
			OP_STX_ABS(0x6000), // Shop index
			OP_LDA_ABSX(0x0220), // Item Id
			OP_STA_ABS(0x6000),
			OP_RTS(),
		});

		patcher->patch(12, 0x845A, 0, { OP_JSR(addr) });

		external_interface->register_callback(0x81, [this](uint8_t world, uint8_t screen, uint8_t shop_index, uint8_t item_id)
		{
			// Find the location
			int64_t loc_id = 0;
			for (const auto& scout : m_location_scouts)
			{
				if (scout.loc->world == (int)world &&
					scout.loc->screen == (int)screen &&
					scout.loc->shop_index == shop_index)
				{
					loc_id = scout.loc->id;
					break;
				}
			}

			if (loc_id == 0)
			{
				// Not found
				printf("Shop location not found. World %i, Screen %i, Shop Index %i, Item 0x%02X\n", (int)world, (int)screen, (int)shop_index, (int)item_id);
				return 0;
			}

			if (m_locations_checked.count(loc_id))
			{
				printf("Location already checked. World %i, Screen %i, Shop Index %i, Item 0x%02X\n", (int)world, (int)screen, (int)shop_index, (int)item_id);
				return 0;
			}

			printf("Location checked! World %i, Screen %i, Shop Index %i, Item 0x%02X\n", (int)world, (int)screen, (int)shop_index, (int)item_id);
			m_locations_checked.insert(loc_id);
			patch_remove_check(loc_id);
			patch_dynamics();

			// Do location check!
			AP_SendItem(loc_id);
			
			// Queue the dialog message to pop when we close the store.
			if (item_id == AP_ITEM_AP || item_id == AP_ITEM_AP_PROGRESSION)
				m_remote_item_dialog_queue.push_back(loc_id);

			return 1;
		}, 4);
	}

	// Location check NPC giving
	{
		auto addr = patcher->patch_new_code(12, {
			OP_PHA(),

			OP_LDA_IMM(0x82), // C++ message id
			OP_STA_ABS(0x6000),
			OP_LDA_ZPG(0x24), // World id
			OP_STA_ABS(0x6000),
			OP_LDA_ZPG(0x63), // Screen id
			OP_STA_ABS(0x6000),
			OP_PLA(), // Item Id
			OP_STA_ABS(0x6000),
			
			OP_JMP_ABS(0x9AF7), // Give Item
		});

		patcher->patch(12, 0x83A4, 0, { OP_JSR(addr) });

		external_interface->register_callback(0x82, [this](uint8_t world, uint8_t screen, uint8_t item_id, uint8_t d)
		{
			// Find the location
			int64_t loc_id = 0;
			for (const auto& scout : m_location_scouts)
			{
				if (scout.loc->world == (int)world &&
					scout.loc->screen == (int)screen)
				{
					loc_id = scout.loc->id;
					break;
				}
			}

			if (loc_id == 0)
			{
				// Not found
				printf("Shop location not found. World %i, Screen %i\n", (int)world, (int)screen);
				return 0;
			}

			if (m_locations_checked.count(loc_id))
			{
				printf("Location already checked. World %i, Screen %i\n", (int)world, (int)screen);
				return 0;
			}

			printf("Location checked! World %i, Screen %i\n", (int)world, (int)screen);
			m_locations_checked.insert(loc_id);
			patch_remove_check(loc_id);
			patch_dynamics();

			// Do location check!
			AP_SendItem(loc_id);
			
			// Queue the dialog message to pop when we close the store.
			if (item_id == AP_ITEM_AP || item_id == AP_ITEM_AP_PROGRESSION)
				m_remote_item_dialog_queue.push_back(loc_id);

			return 1;
		}, 3);
	}

	// Location check final boss killed
	{
		// Just check if "EnemyDies" is called while in the last room.
		// There is probably a specific event for this, but can't find it.
		auto addr = patcher->patch_new_code(14, {
			OP_PHA(),
			OP_LDA_IMM(0x87), // C++ callback id
			OP_STA_ABS(0x6000),
			OP_LDA_ZPG(0x24), // World id
			OP_STA_ABS(0x6000),
			OP_LDA_ZPG(0x63), // Screen id
			OP_STA_ABS(0x6000),
			OP_PLA(),

			OP_JMP_ABS(0xA236), // Go to where we were meant to originally
		});

		patcher->patch(14, 0xAC21, 0, { OP_JSR(addr) });

		external_interface->register_callback(0x87, [this](uint8_t world, uint8_t screen, uint8_t item_id, uint8_t d) -> uint8_t
		{
			if (world == 7 && screen == 0)
			{
				AP_StoryComplete();
			}
			return 0;
		}, 2);
	}
}


const ap_item_t* AP::get_ap_item(int64_t id)
{
	for (const auto& ap_item : AP_ITEMS)
	{
		if (ap_item.id == id)
		{
			return &ap_item;
		}
	}
	return nullptr;
}


const ap_location_t* AP::get_ap_location(int64_t id)
{
	for (const auto& ap_loc : AP_LOCATIONS)
	{
		if (ap_loc.id == id)
		{
			return &ap_loc;
		}
	}
	return nullptr;
}


void AP::patch_remove_check(int64_t loc_id)
{
	auto patcher = m_info.patcher;

	const auto ap_loc = get_ap_location(loc_id);
	if (!ap_loc) return;

	switch (ap_loc->type)
	{
		case ap_location_type_t::shop:
		{
			// Don't remove if it's RED POTION or WINGBOOTS. We let the player buy more.
			if (m_info.rom[ap_loc->addr] != 0x90 && m_info.rom[ap_loc->addr] != 0x8F)
			{
				m_info.rom[ap_loc->addr] = AP_ITEM_NULL;
				m_info.rom[ap_loc->addr + 1] = 0xFF; // Price
				m_info.rom[ap_loc->addr + 2] = 0xFF;
			}
			break;
		}
		case ap_location_type_t::give:
		{
			m_info.rom[ap_loc->addr] = AP_ITEM_NULL;
			break;
		}
		case ap_location_type_t::world:
		case ap_location_type_t::hidden:
		case ap_location_type_t::boss_reward:
		{
			m_info.rom[ap_loc->addr] = AP_ENTITY_NULL;
			break;
		}
	}
}


void AP::patch_wingboots_shop_text()
{
	if ((*m_info.ram)[WINGBOOTS_UNLOCK_ADDR])
	{
		memcpy(ROM_LO(12, 0x9D3E), "WING BOOTS     ", 15);
	}
	else
	{
		memcpy(ROM_LO(12, 0x9D3E), "LOCKED         ", 15);
	}
}


void AP::patch_dynamics()
{
	update_progressive_sword_sprites();
	update_progressive_armor_sprites();
	update_progressive_shield_sprites();
	patch_wingboots_shop_text();
}


void AP::patch_remove_checks()
{
	for (auto loc_id : m_locations_checked)
	{
		patch_remove_check(loc_id);
	}
	patch_dynamics();
}


void AP::on_item_clear()
{
}


void AP::on_item_received(int64_t item_id, int player_id, bool notify_player)
{
	m_recv_item_queue.push_back({ item_id, player_id });
}


void AP::on_location_received(int64_t loc_id)
{
}


static std::string bake_dialog_text(const std::string& text)
{
	std::string ret;

	for (int i = 0, len = (int)text.size(); i < len; ++i)
	{
		auto c = text[i];

		if (c == ' ' || c == '.' || c == '?' || c == ',' || c == '\'' ||
			(c >= 'a' && c <= 'z') ||
			(c >= 'A' && c <= 'Z') ||
			(c >= '0' && c <= '9'))
		{
			ret.push_back(c);
		}
	}

	return ret;
}


const ap_location_scout_t* AP::get_scout_location(int world, int screen, int x, int y) const
{
	x /= 16;
	y /= 16;

	// Find the location
	const ap_location_scout_t* found_scout = nullptr;

	for (const auto& scout : m_location_scouts)
	{
		if (scout.loc->world == (int)world &&
			scout.loc->screen == (int)screen)
		{
			// Special case for screens that have 2 items in them.
			// Only 2 occurence in the entire game.
			// We compare X and Y position to know which Entity it is.
			if (world == 5 && screen == 30)
			{
				if (y < 7 && scout.loc->in_screen_index == 0)
				{
					found_scout = &scout;
					break;
				}
				else if (y > 7 && scout.loc->in_screen_index == 1)
				{
					found_scout = &scout;
					break;
				}
			}
			else if (world == 6 && screen == 19)
			{
				if (x < 9 && scout.loc->in_screen_index == 0)
				{
					found_scout = &scout;
					break;
				}
				else if (x > 9 && scout.loc->in_screen_index == 1)
				{
					found_scout = &scout;
					break;
				}
			}
			else
			{
				found_scout = &scout;
				break;
			}
		}
	}

	return found_scout;
}


void AP::check_ap_version(const std::string& version)
{
	m_apworld_version = version;
	onut::replace(m_apworld_version, "\"", "");
	onut::replace(m_apworld_version, "\\n", "");
}


const ap_location_scout_t* AP::get_scout_location(int64_t loc_id) const
{
	for (const auto& scout : m_location_scouts)
	{
		if (scout.loc->id == loc_id)
		{
			return &scout;
		}
	}

	return nullptr;
}


void AP::on_location_info(const std::vector<AP_NetworkItem>& loc_infos)
{
	for (const auto& loc_info : loc_infos)
	{
		ap_location_scout_t scout_loc;
		scout_loc.flags = loc_info.flags;
		scout_loc.item = loc_info.item;
		scout_loc.item_name = loc_info.itemName;
		scout_loc.player = loc_info.player;
		scout_loc.player_name = loc_info.playerName;

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

		// Bake player name so it can be used in UI
		for (int i = 0, len = (int)scout_loc.player_name.size(); i < len && (int)scout_loc.item_player_name.size() < 13; ++i)
		{
			auto c = scout_loc.player_name[i];
			auto C = std::toupper(c);
			if (C == ' ' || (C >= 'A' && C <= 'Z') || (C >= '0' && C <= '9'))
				scout_loc.item_player_name.push_back(C);
		}

		// Bake dialog
		scout_loc.dialog_player_name = bake_dialog_text(scout_loc.player_name);
		scout_loc.dialog_item_name = bake_dialog_text(scout_loc.item_name);
		auto dialog = "Sent " + scout_loc.dialog_item_name + " to " + scout_loc.dialog_player_name + ".";
		auto words = onut::splitString(dialog, ' ');
		std::vector<std::string> lines;
		std::string current_line;
		for (const auto& word : words)
		{
			if (current_line.size() + word.size() >= 16)
			{
				lines.push_back(current_line);
				current_line = word;
			}
			else
			{
				if (current_line.empty()) current_line = word;
				else current_line += " " + word;
			}
		}
		if (!current_line.empty()) lines.push_back(current_line);

		for (int i = 0, len = (int)lines.size(); i < len; ++i)
		{
			const auto& line = lines[i];
			scout_loc.dialog += line;
			if (i < len - 1)
			{
				if (i % 4 == 3) scout_loc.dialog += "\xFC";
				else scout_loc.dialog += "\xFE";
			}
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
					for (const auto& kv : ap_room_info.datapackage_checksums)
						printf("    %s = %s:\n", kv.first.c_str(), kv.second.c_str());
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

					// For now, naive. Make sure they always match.
					// Later on, we'll check for min supported version
					if (m_apworld_version != MIN_SUPPORTED_VERSION)
					{
						printf("ERROR: apworld version (%s) does not match minimum supported Daxanadu version (%s).\n", m_apworld_version.c_str(), MIN_SUPPORTED_VERSION);
						m_state = state_t::idle;
						m_connection_failed = true;
						break;
					}

					load_state();

					// Scout locations
					m_state = state_t::scouting;
					std::vector<int64_t> location_scouts;
					for (const auto& location : AP_LOCATIONS)
					{
						location_scouts.push_back(location.id);
					}
					printf("Scouting for %i locations...\n", (int)location_scouts.size());
					AP_SendLocationScouts(location_scouts, 0);
					break;
				}
				case AP_ConnectionStatus::ConnectionRefused:
				{
					m_state = state_t::idle;
					m_connection_failed = true;
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
				patch_dynamics();
				patch_randoms();
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
						m_state = state_t::idle;
						m_connection_failed = true;
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

	if (m_connection_failed)
	{
		m_connection_failed = false;
		if (connection_failed_delegate) connection_failed_delegate();
		return;
	}

	m_tracker->update(dt);
}


void AP::patch_locations()
{
	for (const auto& scout : m_location_scouts)
	{
		ap_item_t* ap_item = nullptr;
		if (scout.player == AP_GetPlayerID())
		{
			// Local item
			for (auto& item : AP_ITEMS)
			{
				if (item.id == scout.item)
				{
					ap_item = &item;
					break;
				}
			}
		}

		if (!ap_item)
		{
			// Show AP item
			ap_item = &AP_AP_ITEMS[scout.flags & 1];
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
	m_tracker->render();
}


void AP::serialize(FILE* f, int version) const
{
	{
		uint32_t count = (uint32_t)m_locations_checked.size();
		fwrite(&count, 1, 4, f);
		for (auto loc_id : m_locations_checked)
		{
			fwrite(&loc_id, 1, sizeof(int64_t), f);
		}
	}

	{
		uint32_t count = (uint32_t)m_queued_items.size();
		fwrite(&count, 1, 4, f);
		fwrite(m_queued_items.data(), 1, m_queued_items.size(), f);
	}

	fwrite(&m_item_received_count, 1, 4, f);

	{
		uint32_t count = (uint32_t)m_remote_item_dialog_queue.size();
		fwrite(&count, 1, 4, f);
		fwrite(m_remote_item_dialog_queue.data(), sizeof(int64_t), m_remote_item_dialog_queue.size(), f);
	}
}


void AP::deserialize(FILE* f, int version)
{
	m_locations_checked.clear();
	m_queued_items.clear();

	if (version >= 4)
	{
		uint32_t count;
		fread(&count, 1, 4, f);

		for (uint32_t i = 0; i < count; ++i)
		{
			int64_t loc_id;
			fread(&loc_id, 1, sizeof(int64_t), f);
			m_locations_checked.insert(loc_id);
		}

		patch_remove_checks();
	}

	if (version >= 5)
	{
		uint32_t count;
		fread(&count, 1, 4, f);
		m_queued_items.resize(count);
		fread(m_queued_items.data(), 1, count, f);
	}

	if (version >= 7)
	{
		fread(&m_item_received_count, 1, 4, f);
	}

	if (version >= 8)
	{
		uint32_t count;
		fread(&count, 1, 4, f);
		m_remote_item_dialog_queue.resize(count);
		fread(m_remote_item_dialog_queue.data(), sizeof(int64_t), count, f);
	}
}


static unsigned long long hash_seed(unsigned char *str)
{
    unsigned long long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}


void AP::patch_randoms()
{
	// Seed our random using the seed + player name. Which happens to be our save dir name :)
	unsigned int seed = (unsigned int)hash_seed((unsigned char*)m_save_dir_name.c_str());
	
	srand(seed);
	patch_random_musics();
	seed++;
	
	srand(seed);
	patch_random_sounds();
	seed++;
	
	srand(seed);
	patch_random_npcs();
	seed++;
	
	srand(seed);
	patch_random_monsters();
	seed++;
	
	srand(seed);
	patch_random_rewards();
	seed++;
}


void AP::patch_random_musics()
{
	//mute = 0
	//intro = 1
	//dartmoor = 2
	//trunk = 3
	//branches = 4
	//mist = 5
	//tower = 6
	//eolis = 7
	//death = 8
	//towns = 9
	//boss = A
	//hour glass = B
	//finale = C
	//king = D
	//guru = E
	//shop = F
	//evil fortress = 10

	std::vector<uint8_t> musics = {
		2, 3, 4, 5, 6, 7, 8, 9, 0xA, 0xB, 0xD, 0xE, 0xF, 0x10
	};

	for (int i = 0; i <= 0x10; ++i)
	{
		// Those 3 don't shuffle
		if (i == 0 || i == 1 || i == 0xC || !m_option_random_musics)
		{
			m_music_map.push_back((uint8_t)i);
			continue;
		}

		int rnd = rand() % (int)musics.size();
		uint8_t music_id = musics[rnd];
		musics.erase(musics.begin() + rnd);
		m_music_map.push_back(music_id);
	}

	m_info.ram->register_read_callback([this](uint8_t* out_byte, int addr) -> bool
	{
		auto byte = *out_byte;
		auto bit = byte & 0x80;
		if (bit) return false; // Already altered
		byte = m_music_map[byte];
		*out_byte = byte;
		return true;
	}, 0x00FA);

	//m_info.ram->register_write_callback([](uint8_t byte, int addr) -> uint8_t
	//{
	//	printf("Music: 0x%02X\n", (int)byte);
	//	return byte;
	//}, 0x00FA);
}


void AP::patch_random_sounds()
{
	std::vector<uint8_t> sounds;
	for (int i = 1; i <= 0x1C; ++i)
	{
		sounds.push_back(i);
	}

	for (int i = 1; i <= 0x1C; ++i)
	{
		if (!m_option_random_sounds)
		{
			m_sound_map.push_back((uint8_t)i);
			continue;
		}

		int rnd = rand() % (int)sounds.size();
		uint8_t sound_id = sounds[rnd];
		sounds.erase(sounds.begin() + rnd);
		m_sound_map.push_back(sound_id);
	}
}


uint8_t AP::remap_sound(uint8_t sound_id) const
{
	return m_sound_map[sound_id - 1];
}


void AP::patch_random_npcs()
{
}


void AP::patch_random_monsters()
{
}


void AP::patch_random_rewards()
{
}


void AP::option_random_musics(int value)
{
	m_option_random_musics = value;
}


void AP::option_random_sounds(int value)
{
	m_option_random_sounds = value;
}


void AP::option_random_npcs(int value)
{
	m_option_random_npcs = value;
}


void AP::option_random_monsters(int value)
{
	m_option_random_monsters = value;
}


void AP::option_random_rewards(int value)
{
	m_option_random_rewards = value;
}
