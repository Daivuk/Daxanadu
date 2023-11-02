#include "AP.h"
#include "APItems.h"
#include "APLocations.h"
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

	// Replace Crystal with spring elixir
	copy_sprite(m_info.rom + 0x00028500 + 0x34 * 16, m_info.rom + 0x00028500 + 0x12 * 16, false, true);
	copy_sprite(m_info.rom + 0x00028500 + 0x35 * 16, m_info.rom + 0x00028500 + 0x13 * 16, false, true);
	copy_sprite(m_info.rom + 0x00028500 + 0x36 * 16, m_info.rom + 0x00028500 + 0x14 * 16, false, true);
	copy_sprite(m_info.rom + 0x00028500 + 0x37 * 16, m_info.rom + 0x00028500 + 0x15 * 16, false, true);

	// Replace Lamp with glove
	copy_sprite(m_info.rom + 0x0001CD26 - 0x10 + 0 * 16, m_info.rom + 0x00028500 + 0x16 * 16, false, false, 4);
	copy_sprite(m_info.rom + 0x0001CD26 - 0x10 + 0 * 16, m_info.rom + 0x00028500 + 0x17 * 16, false, false, -4);
	copy_sprite(m_info.rom + 0x0001CD26 - 0x10 + 1 * 16, m_info.rom + 0x00028500 + 0x18 * 16, false, false, 4);
	copy_sprite(m_info.rom + 0x0001CD26 - 0x10 + 1 * 16, m_info.rom + 0x00028500 + 0x19 * 16, false, false, -4);

	// Replace fire crystal tiles with ointment
	copy_sprite(m_info.rom + 0x0001CE06 - 0x10 + 0 * 16, m_info.rom + 0x00028500 + 0x40 * 16, false, true);
	copy_sprite(m_info.rom + 0x0001CE06 - 0x10 + 0 * 16, m_info.rom + 0x00028500 + 0x41 * 16, true, true);
	copy_sprite(m_info.rom + 0x0001CE06 - 0x10 + 1 * 16, m_info.rom + 0x00028500 + 0x42 * 16, false, true);
	copy_sprite(m_info.rom + 0x0001CE06 - 0x10 + 1 * 16, m_info.rom + 0x00028500 + 0x43 * 16, true, true);

	// Prepare space in unused area for all the new item texts
	for (int i = 0; i < 16; ++i)
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
	ADD_ITEM("PROG SWORD", 0x96, 0x4D, 0x4E, 0x4F, 0x50);
	ADD_ITEM("PROG ARMOR", 0x97, 0x5D, 0x5E, 0x5F, 0x60);

	ADD_ITEM("PROG SHIELD", 0x98, 0x69, 0x6A, 0x6B, 0x6C);
	ADD_ITEM("POISON", 0x99, 0x30, 0x31, 0x32, 0x33);
	ADD_ITEM("OINTMENT", 0x9A, 0x40, 0x41, 0x42, 0x43);
	ADD_ITEM("GLOVE", 0x9B, 0x16, 0x17, 0x18, 0x19);

	ADD_ITEM("SPRING ELIXIR", 0x9C, 0x12, 0x13, 0x14, 0x15);

	// Write new code in bank12 that allows to jump further to index the new text
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

	// We no longer have unused items. Place at new location and the code will have to fix the text lookup...
	//for (int i = 0; i < 15; ++i) m_info.rom[0x00032D9E + i] = 0x20;
	//m_info.rom[0x00032D9E + 15] = 0x0D;
	//memcpy(m_info.rom + 0x00032D9E, "GLOVE", strlen("GLOVE"));

	//m_info.patcher->apply_dialog_sound_patch(
		//0x9C4D -> 0xAE4D

	// Texts are in bank 12, addr $9B3E
	// Offsets for each categories are at $9B33
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
