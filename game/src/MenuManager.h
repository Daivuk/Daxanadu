#pragma once

#include "InputContext.h"

#include <onut/ForwardDeclaration.h>
#include <onut/Texture.h>

#include <functional>
#include <string>
#include <vector>


OForwardDeclare(Sound)
OForwardDeclare(SoundInstance)
class GameplayInputContext;
class MenuInputContext;
class Patcher;
class TileDrawer;


struct menu_manager_info_t
{
    TileDrawer* tile_drawer = nullptr;
    Patcher* patcher = nullptr;
    GameplayInputContext* gameplay_input_context = nullptr;
    MenuInputContext* menu_input_context = nullptr;
    OSoundRef action_sfx;
    OSoundRef choice_sfx;
    OSoundRef nav_sfx;
    OSoundRef error_sfx;
};


// One in all menu: Main Menu option menu, in-game menu
class MenuManager final
{
public:
    MenuManager(const menu_manager_info_t& info);

    bool can_emulate() const { return true;/*m_menu_stack.empty();*/ }
    void hide();
    void show_in_game_menu();
    void on_ap_connect_failed();

    void update(float dt);
    void render();

    // delegates
    std::function<void()> new_game_delegate;
    std::function<void()> continue_game_delegate;
    std::function<void()> save_delegate;
    std::function<void()> dismissed_pause_menu_delegate;
    std::function<void()> play_ap_delegate;

private:
    enum class option_t
    {
        action,
        choice,
        slider,
        key_bind,
        skip,
        hide_cursor,
        key_binding
    };

    struct menu_option_t
    {
        std::string name;
        std::vector<std::string> choices;
        option_t type = option_t::action;
        std::function<void(menu_option_t*)> load_callback;
        std::function<void(menu_option_t*)> activate_callback;
        union
        {
            InputAction* input_action = nullptr;
            int default_choice;
        };
        int choice = 0;
    };

    struct menu_t
    {
        std::vector<menu_option_t> options;
        int selection = 0;
        int mapping_index = 0;
        int x = 0;
        int y = 0;
        int w = 0;
        int h = 0;
        bool spaced = true;
        bool framed = true;
        int columns = 1;
    };

    enum class state_t
    {
        hidden,
        main_menu,
        option_menu,
        input_mapping,
        alternatives1,
        alternatives2,
        reset_defaults,
        gameplay_menu,
        audio_menu,
        cosmetic_menu,
        pause_menu,
        menu_input_menu,
        gameplay_input_menu,
        key_bind_popup,
        new_game_options,
        ap_connect_menu,
        ap_connecting_menu,
        ap_failed_menu,

        COUNT
    };

    void draw_menu(int x, int y, state_t menu, bool draw_cursor);
    void push_menu(state_t state);

    void on_new_game(menu_option_t* option);
    void on_continue(menu_option_t* option);
    void on_archipelago(menu_option_t* option);
    void on_options(menu_option_t* option);
    void on_quit(menu_option_t* option);
    void on_save_and_quit(menu_option_t* option);

    void on_ap_address(menu_option_t* option);
    void on_ap_slot(menu_option_t* option);
    void on_ap_password(menu_option_t* option);
    void load_ap_address(menu_option_t* option);
    void load_ap_slot(menu_option_t* option);
    void load_ap_password(menu_option_t* option);
    void on_ap_connect(menu_option_t* option);
    void on_generic_ok(menu_option_t* option);

    void on_gameplay(menu_option_t* option);
    void on_audio(menu_option_t* option);
    void on_cosmetic(menu_option_t* option);
    void on_input_mappings(menu_option_t* option);
    void on_new_game_options(menu_option_t* option);

    void on_mappings2(menu_option_t* option);
    void on_mappings3(menu_option_t* option);
    void on_reset_defaults(menu_option_t* option);
    void on_reset_ok(menu_option_t* option);

    void on_dialog_speed(menu_option_t* option);
    void load_dialog_speed(menu_option_t* option);
    void on_music_volume(menu_option_t* option);
    void load_music_volume(menu_option_t* option);
    void on_sound_volume(menu_option_t* option);
    void load_sound_volume(menu_option_t* option);
    void on_npc_blinking_speed(menu_option_t* option);
    void load_npc_blinking_speed(menu_option_t* option);
    void on_king_golds(menu_option_t* option);
    void load_king_golds(menu_option_t* option);
    void on_coins_despawn(menu_option_t* option);
    void load_coins_despawn(menu_option_t* option);
    void on_mist_quality(menu_option_t* option);
    void load_mist_quality(menu_option_t* option);
    void on_cigarettes(menu_option_t* option);
    void load_cigarettes(menu_option_t* option);
    void on_double_golds(menu_option_t* option);
    void load_double_golds(menu_option_t* option);
    void on_double_xp(menu_option_t* option);
    void load_double_xp(menu_option_t* option);
    void on_equip_in_shops_xp(menu_option_t* option);
    void load_equip_in_shops_xp(menu_option_t* option);
    void on_start_full_health(menu_option_t* option);
    void load_start_full_health(menu_option_t* option);
    void on_secret_items_counter(menu_option_t* option);
    void load_secret_items_counter(menu_option_t* option);
    void on_show_level_popup(menu_option_t* option);
    void load_show_level_popup(menu_option_t* option);
    void on_keep_gold(menu_option_t* option);
    void load_keep_gold(menu_option_t* option);
    void on_keep_xp(menu_option_t* option);
    void load_keep_xp(menu_option_t* option);
    void on_xp_timeouts(menu_option_t* option);
    void load_xp_timeouts(menu_option_t* option);
    void on_xp_speed(menu_option_t* option);
    void load_xp_speed(menu_option_t* option);
    void on_pendant(menu_option_t* option);
    void load_pendant(menu_option_t* option);
    void on_fast_cpu(menu_option_t* option);
    void load_fast_cpu(menu_option_t* option);

    menu_manager_info_t m_info;
    std::vector<state_t> m_menu_stack;
    menu_t m_menus[(int)state_t::COUNT];
    OTextureRef m_framebuffer;
    Input m_binding_input_down = Input::None;
    inputs_t m_last_inputs;
    InputAction* m_binding_input_action = nullptr;
    float m_sfx_volume = 1.0f;
    OSoundInstanceRef m_action_sfx;
    OSoundInstanceRef m_choice_sfx;
    OSoundInstanceRef m_nav_sfx;
    OSoundInstanceRef m_error_sfx;
};
