#pragma once

#include <functional>
#include <set>
#include <string>
#include <vector>


struct AP_NetworkItem;
struct ap_location_t;
struct ap_item_t;
class APTracker;
class Cart;
class CPU;
class CPUBUS;
class ExternalInterface;
class Patcher;
class RAM;
class TileDrawer;


struct ap_info_t
{
    uint8_t* rom = nullptr;
    RAM* ram = nullptr;
    CPUBUS* cpu_bus = nullptr;
    CPU* cpu = nullptr;
    Cart* cart = nullptr;
    TileDrawer* tile_drawer = nullptr;
    size_t rom_size = 0;
    std::string address;
    std::string slot_name;
    std::string password;
    Patcher* patcher = nullptr;
    ExternalInterface* external_interface = nullptr;
};


struct ap_location_scout_t
{
    ap_location_t* loc = nullptr;
    int64_t item;
    int player;
    int flags;
    std::string player_name;
    std::string item_name;
    std::string dialog_player_name;
    std::string dialog_item_name;
    std::string item_player_name;
    std::string dialog;
};


class AP final
{
public:
    enum class state_t
    {
        idle,
        connecting,
        connected,
        scouting
    };

    AP(const ap_info_t& info);
    ~AP();

    void connect();
    void update(float dt);
    void render();
    state_t get_state() const { return m_state; }
    const std::string& get_dir_name() const { return m_save_dir_name; }

    std::function<void()> connection_success_delegate;
    std::function<void()> connection_failed_delegate;

    void on_item_clear();
    void on_item_received(int64_t item_id, int player_id, bool notify_player);
    void on_location_received(int64_t loc_id);
    void on_location_info(const std::vector<AP_NetworkItem>& loc_infos);
    void check_ap_version(const std::string& version);

    void serialize(FILE* f, int version) const;
    void deserialize(FILE* f, int version);

private:
    void load_state();
    void save_state();
    void patch_locations();
    void patch_items();
    void patch_cpp_hooks();
    void patch_remove_checks();
    void patch_remove_check(int64_t loc_id);

    void patch_dynamics();
    void update_progressive_sword_sprites();
    void update_progressive_armor_sprites();
    void update_progressive_shield_sprites();
    void patch_wingboots_shop_text();

    int get_progressive_sword_level();
    int get_progressive_armor_level();
    int get_progressive_shield_level();

    static const ap_item_t* get_ap_item(int64_t id);
    static const ap_location_t* get_ap_location(int64_t id);
    const ap_location_scout_t* get_scout_location(int world, int screen, int x, int y) const;
    const ap_location_scout_t* get_scout_location(int64_t loc_id) const;

    ap_info_t m_info;
    state_t m_state = state_t::idle;
    std::string m_save_dir_name;
    std::vector<ap_location_scout_t> m_location_scouts;
    std::set<int64_t> m_locations_checked;
    APTracker* m_tracker = nullptr;
    std::vector<uint8_t> m_queued_items;
    int32_t m_item_received_count = 0;
    //int32_t m_item_received_current_count = 0;
    std::vector<int64_t> m_remote_item_dialog_queue;
    std::string m_apworld_version; // Daxanadu version used by the apworld
    bool m_connection_failed = false;

    struct recv_item_t
    {
        int64_t item_id = 0;
        int player_id = 0;
    };
    std::vector<recv_item_t> m_recv_item_queue;
};
