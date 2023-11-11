#pragma once

#include <functional>
#include <set>
#include <string>
#include <vector>


struct AP_NetworkItem;
struct ap_location_t;
struct ap_item_t;
class Patcher;
class ExternalInterface;


struct ap_info_t
{
    uint8_t* rom = nullptr;
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

    std::function<void()> connection_success_delegate;
    std::function<void()> connection_failed_delegate;

    void on_item_clear();
    void on_item_received(int64_t item_id, bool notify_player);
    void on_location_received(int64_t loc_id);
    void on_location_info(const std::vector<AP_NetworkItem>& loc_infos);

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

    static const ap_item_t* get_ap_item(int64_t id);
    static const ap_location_t* get_ap_location(int64_t id);

    ap_info_t m_info;
    state_t m_state = state_t::idle;
    std::string m_save_dir_name;
    std::vector<ap_location_scout_t> m_location_scouts;
    std::set<int64_t> m_locations_checked;
};
