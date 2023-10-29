#pragma once

#include <functional>
#include <string>
#include <vector>


struct AP_NetworkItem;


struct ap_info_t
{
    std::string address;
    std::string slot_name;
    std::string password;
};


class AP final
{
public:
    enum class state_t
    {
        idle,
        connecting,
        connected
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

private:
    void load_state();
    void save_state();

    ap_info_t m_info;
    state_t m_state = state_t::idle;
    std::string m_save_dir_name;
};
