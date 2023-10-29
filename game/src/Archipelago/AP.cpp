#include "AP.h"
#include "version.h"

#include "Archipelago.h"

#include <onut/Files.h>


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
					if (connection_success_delegate) connection_success_delegate();
					break;
				}
				case AP_ConnectionStatus::ConnectionRefused:
				{
					m_state = state_t::idle;
					if (connection_failed_delegate) connection_failed_delegate();
					break;
				}
				case AP_ConnectionStatus::Disconnected:
				{
					if (m_state == state_t::connected)
					{
						__debugbreak();
					}
					break;
				}
			}
			break;
		}
		case state_t::connected:
		{
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


void AP::render()
{
}
