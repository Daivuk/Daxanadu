#include "MenuManager.h"
#include "GameplayInputContext.h"
#include "MenuInputContext.h"
#include "Patcher.h"
#include "PPU.h"
#include "TileDrawer.h"

#include <onut/Dialogs.h>
#include <onut/GamePad.h>
#include <onut/Input.h>
#include <onut/onut.h>
#include <onut/Renderer.h>
#include <onut/Settings.h>
#include <onut/Sound.h>
#include <onut/SpriteBatch.h>
#include <onut/Strings.h>

#include "../src/tinyfiledialogs/tinyfiledialogs.h"


static int get_user_setting(const std::string& setting_name, int default_value)
{
    try
    {
        return std::stoi(oSettings->getUserSetting(setting_name));
    }
    catch (...)
    {
        return default_value;
    }
}


MenuManager::MenuManager(const menu_manager_info_t& info)
    : m_info(info)
{
    m_framebuffer = OTexture::createRenderTarget({ PPU::SCREEN_W, PPU::SCREEN_H });
    m_sfx_volume = (float)get_user_setting("sound_volume", 4) / 8.0f;

    // We create instances of the sounds so we don't have overlay when we replay it fast. We stop previous sound
    m_action_sfx = m_info.action_sfx->createInstance();
    m_choice_sfx = m_info.choice_sfx->createInstance();
    m_nav_sfx = m_info.nav_sfx->createInstance();
    m_error_sfx = m_info.error_sfx->createInstance();
    m_action_sfx->setVolume(m_sfx_volume);
    m_choice_sfx->setVolume(m_sfx_volume);
    m_nav_sfx->setVolume(m_sfx_volume);
    m_error_sfx->setVolume(m_sfx_volume);

#define BIND(fn) std::bind(&MenuManager::fn, this, std::placeholders::_1)

    m_menus[(int)state_t::main_menu].options = {
        { "START", {}, option_t::action, nullptr, BIND(on_new_game) },
        { "CONTINUE", {}, option_t::action, nullptr, BIND(on_continue) },
        { "OPTIONS", {}, option_t::action, nullptr, BIND(on_options) },
        { "ARCHIPELAGO", {}, option_t::action, nullptr, BIND(on_archipelago) },
        { "QUIT", {}, option_t::action, nullptr, BIND(on_quit) },
    };
    m_menus[(int)state_t::main_menu].x = 3;
    m_menus[(int)state_t::main_menu].y = 18;
    m_menus[(int)state_t::main_menu].w = 29;
    m_menus[(int)state_t::main_menu].h = 4;
    m_menus[(int)state_t::main_menu].spaced = false;
    m_menus[(int)state_t::main_menu].framed = false;
    m_menus[(int)state_t::main_menu].columns = 2;

    m_menus[(int)state_t::ap_connect_menu].options = {
        { "ADDRESS:", {}, option_t::action, nullptr, BIND(on_ap_address) },
        { "", {}, option_t::skip, BIND(load_ap_address), nullptr },
        { "SLOT:", {}, option_t::action, nullptr, BIND(on_ap_slot) },
        { "", {}, option_t::skip, BIND(load_ap_slot), nullptr },
        { "PASSWORD:", {}, option_t::action, nullptr, BIND(on_ap_password) },
        { "", {}, option_t::skip, BIND(load_ap_password), nullptr },
        { "CONNECT", {}, option_t::action, nullptr, BIND(on_ap_connect) },
    };
    m_menus[(int)state_t::ap_connect_menu].x = 1;
    m_menus[(int)state_t::ap_connect_menu].y = 5;
    m_menus[(int)state_t::ap_connect_menu].w = 30;

    m_menus[(int)state_t::pause_menu].options = {
        { "CONTROLS", {}, option_t::action, nullptr, BIND(on_input_mappings) },
        { "GAMEPLAY", {}, option_t::action, nullptr, BIND(on_gameplay) },
        { "AUDIO", {}, option_t::action, nullptr, BIND(on_audio) },
        { "VISUALS", {}, option_t::action, nullptr, BIND(on_cosmetic) },
        { "SAVE AND QUIT", {}, option_t::action, nullptr, BIND(on_save_and_quit) },
    };
    m_menus[(int)state_t::pause_menu].x = 2;
    m_menus[(int)state_t::pause_menu].y = 6;

    m_menus[(int)state_t::option_menu].options = {
        { "CONTROLS", {}, option_t::action, nullptr, BIND(on_input_mappings) },
        { "GAMEPLAY", {}, option_t::action, nullptr, BIND(on_gameplay) },
        { "AUDIO", {}, option_t::action, nullptr, BIND(on_audio) },
        { "VISUALS", {}, option_t::action, nullptr, BIND(on_cosmetic) },
        { "NEW GAME OPTIONS", {}, option_t::action, nullptr, BIND(on_new_game_options) },
    };
    m_menus[(int)state_t::option_menu].x = 12;
    m_menus[(int)state_t::option_menu].y = 17;

    m_menus[(int)state_t::new_game_options].options = {
        { "START FULL HEALTH", { "NO", "YES" }, option_t::choice, BIND(load_start_full_health), BIND(on_start_full_health), 0 },
    };
    m_menus[(int)state_t::new_game_options].x = 1;
    m_menus[(int)state_t::new_game_options].y = 22;

    m_menus[(int)state_t::gameplay_menu].options = {
        { "DIALOG SPEED", { "NORMAL", "FAST", "FASTER" }, option_t::choice, BIND(load_dialog_speed), BIND(on_dialog_speed), 0 },
        { "KEEP GOLD ON DEATH", { "NO", "YES" }, option_t::choice, BIND(load_keep_gold), BIND(on_keep_gold), 0 },
        { "KEEP XP ON DEATH", { "NO", "YES" }, option_t::choice, BIND(load_keep_xp), BIND(on_keep_xp), 0 },
        { "DOUBLE GOLD", { "NO", "YES" }, option_t::choice, BIND(load_double_golds), BIND(on_double_golds), 0 },
        { "DOUBLE XP", { "NO", "YES" }, option_t::choice, BIND(load_double_xp), BIND(on_double_xp), 0 },
        { "KING GOLDS", { "NORMAL", "ONCE" }, option_t::choice, BIND(load_king_golds), BIND(on_king_golds), 0 },
        { "COINS DESPAWN", { "YES", "NO" }, option_t::choice, BIND(load_coins_despawn), BIND(on_coins_despawn), 0 },
        { "EQUIP IN SHOPS", { "NO", "YES" }, option_t::choice, BIND(load_equip_in_shops_xp), BIND(on_equip_in_shops_xp), 0 },
        { "ITEM ROOM COUNTER", { "YES", "NO" }, option_t::choice, BIND(load_secret_items_counter), BIND(on_secret_items_counter), 0 },
        { "XP AFFECTS WINGBOOTS", { "YES", "NO" }, option_t::choice, BIND(load_xp_timeouts), BIND(on_xp_timeouts), 0 },
        { "XP AFFECTS SPEED", { "YES", "NO" }, option_t::choice, BIND(load_xp_speed), BIND(on_xp_speed), 0 },
        { "FAST CPU", { "NO", "YES" }, option_t::choice, BIND(load_fast_cpu), BIND(on_fast_cpu), 0 },
    };
    m_menus[(int)state_t::gameplay_menu].x = 1;
    m_menus[(int)state_t::gameplay_menu].y = 0;

    m_menus[(int)state_t::audio_menu].options = {
        { "MUSIC VOLUME", { "--------",
                            "=-------",
                            "==------",
                            "===-----",
                            "====----",
                            "=====---",
                            "======--",
                            "=======-",
                            "========", }, option_t::slider, BIND(load_music_volume), BIND(on_music_volume) },
        { "SOUND VOLUME", { "--------",
                            "=-------",
                            "==------",
                            "===-----",
                            "====----",
                            "=====---",
                            "======--",
                            "=======-",
                            "========", }, option_t::slider, BIND(load_sound_volume), BIND(on_sound_volume) },
    };
    m_menus[(int)state_t::audio_menu].x = 1;
    m_menus[(int)state_t::audio_menu].y = 16;

    m_menus[(int)state_t::cosmetic_menu].options = {
        //{ "NPC BLINKING", { "NORMAL", "LESS" }, option_t::choice, BIND(load_npc_blinking_speed), BIND(on_npc_blinking_speed), 0 },
        { "MIST SCROLL", { "NORMAL", "MORE" }, option_t::choice, BIND(load_mist_quality), BIND(on_mist_quality), 0 },
        { "CIGARETTES", { "YES", "NO" }, option_t::choice, BIND(load_cigarettes), BIND(on_cigarettes), 0 },
        { "LEVEL NAME POPUP", { "NO", "YES" }, option_t::choice, BIND(load_show_level_popup), BIND(on_show_level_popup), 0 },
    };
    m_menus[(int)state_t::cosmetic_menu].x = 1;
    m_menus[(int)state_t::cosmetic_menu].y = 16;

    m_menus[(int)state_t::input_mapping].options = {
        { "LEFT", {}, option_t::key_bind, nullptr, nullptr, &m_info.gameplay_input_context->left },
        { "RIGHT", {}, option_t::key_bind, nullptr, nullptr, &m_info.gameplay_input_context->right },
        { "UP", {}, option_t::key_bind, nullptr, nullptr, &m_info.gameplay_input_context->up },
        { "DOWN", {}, option_t::key_bind, nullptr, nullptr, &m_info.gameplay_input_context->down },
        { "INVENTORY", {}, option_t::key_bind, nullptr, nullptr, &m_info.gameplay_input_context->inventory },
        { "PAUSE", {}, option_t::key_bind, nullptr, nullptr, &m_info.gameplay_input_context->pause },
        { "ATTACK", {}, option_t::key_bind, nullptr, nullptr, &m_info.gameplay_input_context->attack },
        { "JUMP", {}, option_t::key_bind, nullptr, nullptr, &m_info.gameplay_input_context->jump },
        //{ "MAGIC", {}, option_t::key_bind, nullptr, nullptr, &m_info.gameplay_input_context->magic },
        //{ "USE ITEM", {}, option_t::key_bind, nullptr, nullptr, &m_info.gameplay_input_context->use_item },
        { "ALTERNATIVES 1", {}, option_t::action, nullptr, BIND(on_mappings2) },
        { "ALTERNATIVES 2", {}, option_t::action, nullptr, BIND(on_mappings3) },
        { "RESET DEFAULTS", {}, option_t::action, nullptr, BIND(on_reset_defaults) },
    };
    m_menus[(int)state_t::input_mapping].mapping_index = 0;
    m_menus[(int)state_t::input_mapping].x = 1;
    m_menus[(int)state_t::input_mapping].y = 1;

    m_menus[(int)state_t::alternatives1].options = {
        { "ALTERNATIVES 1", {}, option_t::skip },
        { "", {}, option_t::skip },
        { "LEFT", {}, option_t::key_bind, nullptr, nullptr, &m_info.gameplay_input_context->left },
        { "RIGHT", {}, option_t::key_bind, nullptr, nullptr, &m_info.gameplay_input_context->right },
        { "UP", {}, option_t::key_bind, nullptr, nullptr, &m_info.gameplay_input_context->up },
        { "DOWN", {}, option_t::key_bind, nullptr, nullptr, &m_info.gameplay_input_context->down },
        { "INVENTORY", {}, option_t::key_bind, nullptr, nullptr, &m_info.gameplay_input_context->inventory },
        { "PAUSE", {}, option_t::key_bind, nullptr, nullptr, &m_info.gameplay_input_context->pause },
        { "ATTACK", {}, option_t::key_bind, nullptr, nullptr, &m_info.gameplay_input_context->attack },
        { "JUMP", {}, option_t::key_bind, nullptr, nullptr, &m_info.gameplay_input_context->jump },
        //{ "MAGIC", {}, option_t::key_bind, nullptr, nullptr, &m_info.gameplay_input_context->magic },
        //{ "USE ITEM", {}, option_t::key_bind, nullptr, nullptr, &m_info.gameplay_input_context->use_item },
    };
    m_menus[(int)state_t::alternatives1].mapping_index = 1;
    m_menus[(int)state_t::alternatives1].x = 1;
    m_menus[(int)state_t::alternatives1].y = 2;

    m_menus[(int)state_t::alternatives2].options = {
        { "ALTERNATIVES 2", {}, option_t::skip },
        { "", {}, option_t::skip },
        { "LEFT", {}, option_t::key_bind, nullptr, nullptr, &m_info.gameplay_input_context->left },
        { "RIGHT", {}, option_t::key_bind, nullptr, nullptr, &m_info.gameplay_input_context->right },
        { "UP", {}, option_t::key_bind, nullptr, nullptr, &m_info.gameplay_input_context->up },
        { "DOWN", {}, option_t::key_bind, nullptr, nullptr, &m_info.gameplay_input_context->down },
        { "INVENTORY", {}, option_t::key_bind, nullptr, nullptr, &m_info.gameplay_input_context->inventory },
        { "PAUSE", {}, option_t::key_bind, nullptr, nullptr, &m_info.gameplay_input_context->pause },
        { "ATTACK", {}, option_t::key_bind, nullptr, nullptr, &m_info.gameplay_input_context->attack },
        { "JUMP", {}, option_t::key_bind, nullptr, nullptr, &m_info.gameplay_input_context->jump },
        //{ "MAGIC", {}, option_t::key_bind, nullptr, nullptr, &m_info.gameplay_input_context->magic },
        //{ "USE ITEM", {}, option_t::key_bind, nullptr, nullptr, &m_info.gameplay_input_context->use_item },
    };
    m_menus[(int)state_t::alternatives2].mapping_index = 2;
    m_menus[(int)state_t::alternatives2].x = 1;
    m_menus[(int)state_t::alternatives2].y = 2;

    m_menus[(int)state_t::reset_defaults].options = {
        { "MAPPINGS HAVE BEEN", {}, option_t::skip },
        { " RESET TO DEFAULT", {}, option_t::skip },
        { "", {}, option_t::skip },
        { "       OK", {}, option_t::action, nullptr, BIND(on_reset_ok) },
    };
    m_menus[(int)state_t::reset_defaults].x = 5;
    m_menus[(int)state_t::reset_defaults].y = 8;

    m_menus[(int)state_t::key_bind_popup].options = {
        { "PRESS KEYS OR COMBOS", {}, option_t::skip },
        { "", {}, option_t::skip },
        { "", {}, option_t::key_binding },
    };
    m_menus[(int)state_t::key_bind_popup].x = 4;
    m_menus[(int)state_t::key_bind_popup].y = 8;

    m_menus[(int)state_t::ap_connecting_menu].options = {
        { "CONNECTING...", {}, option_t::action },
    };
    m_menus[(int)state_t::ap_connecting_menu].x = 8;
    m_menus[(int)state_t::ap_connecting_menu].y = 12;

    m_menus[(int)state_t::ap_failed_menu].options = {
        { "FAILED TO CONNECT", {}, option_t::skip },
        { " TO ARCHIPELAGO", {}, option_t::skip },
        { "", {}, option_t::skip },
        { "       OK", {}, option_t::action, nullptr, BIND(on_generic_ok) },
    };
    m_menus[(int)state_t::ap_failed_menu].x = 5;
    m_menus[(int)state_t::ap_failed_menu].y = 8;

    push_menu(state_t::main_menu);
}


void MenuManager::update(float dt)
{
    if (m_menu_stack.empty()) return;

    auto& menu = m_menus[(int)m_menu_stack.back()];

    if (m_menu_stack.back() == state_t::key_bind_popup)
    {
        if (!m_binding_input_down.is_bound())
        {
            for (int i = 0; i < (int)OInput::State::STATE_COUNT; ++i)
            {
                if (OInputJustPressed((OInput::State)i))
                {
                    m_binding_input_down = (OInput::State)i;
                    break;
                }
            }
            if (!m_binding_input_down.is_bound()) // Try gamepad
            {
                for (int i = 0; i < (int)OGamePad::Button::COUNT; ++i)
                {
                    if (OGamePadJustPressed((OGamePad::Button)i))
                    {
                        m_binding_input_down = (OGamePad::Button)i;
                        break;
                    }
                }
            }
        }
        if (m_binding_input_down.is_bound())
        {
            if (m_binding_input_down.is_just_released())
            {
                // That's it, single input
                m_binding_input_action->bind(menu.mapping_index, Input::None, m_binding_input_down);
                m_menu_stack.pop_back();
                m_info.gameplay_input_context->save_settings();
                return;
            }
            for (int i = 0; i < (int)OInput::State::STATE_COUNT; ++i)
            {
                if (m_binding_input_down == (OInput::State)i) continue;
                if (OInputJustPressed((OInput::State)i))
                {
                    m_binding_input_action->bind(menu.mapping_index, m_binding_input_down, (OInput::State)i);
                    m_menu_stack.pop_back();
                    m_info.gameplay_input_context->save_settings();
                    break;
                }
            }
            for (int i = 0; i < (int)OGamePad::Button::COUNT; ++i)
            {
                if (m_binding_input_down == (OGamePad::Button)i) continue;
                if (OGamePadJustPressed((OGamePad::Button)i))
                {
                    m_binding_input_action->bind(menu.mapping_index, m_binding_input_down, (OGamePad::Button)i);
                    m_menu_stack.pop_back();
                    m_info.gameplay_input_context->save_settings();
                    break;
                }
            }
        }

        return;
    }

    inputs_t inputs = m_info.menu_input_context->read_inputs(0);

    auto nav_left = !m_last_inputs.left && inputs.left;
    auto nav_right = !m_last_inputs.right && inputs.right;
    auto nav_up = !m_last_inputs.up && inputs.up;
    auto nav_down = !m_last_inputs.down && inputs.down;
    auto nav_activate = !m_last_inputs.a && inputs.a;
    auto nav_back = !m_last_inputs.b && inputs.b;
    auto clear_binding = OInputJustPressed(OKeyDelete);

    m_last_inputs = inputs;

    if (menu.columns > 1)
    {
        const int rest = (int)menu.options.size() % menu.columns;
        const int row_count = (int)std::ceilf((float)menu.options.size() / (float)menu.columns);
        int col = menu.selection % menu.columns;
        int row = menu.selection / menu.columns;
        int col_in_row = (row == row_count - 1) ? rest : menu.columns;
        const int row_in_col = ((int)menu.options.size() / menu.columns) + ((int)menu.options.size() % menu.columns > col ? 1 : 0);

        if ((menu.options.size() % menu.columns) == 0) col_in_row = menu.columns;

        if (nav_down)
        {
            row++;
            if (row >= row_in_col) row = 0;
            menu.selection = row * menu.columns + col;
            m_nav_sfx->stop(); m_nav_sfx->play();
        }

        if (nav_up)
        {
            row--;
            if (row < 0) row = row_in_col - 1;
            menu.selection = row * menu.columns + col;
            m_nav_sfx->stop(); m_nav_sfx->play();
        }

        if (nav_right)
        {
            col++;
            if (col >= col_in_row) col = 0;
            menu.selection = row * menu.columns + col;
            m_nav_sfx->stop(); m_nav_sfx->play();
        }

        if (nav_left)
        {
            col--;
            if (col < 0) col = col_in_row - 1;
            menu.selection = row * menu.columns + col;
            m_nav_sfx->stop(); m_nav_sfx->play();
        }
    }
    else
    {
        if (nav_down)
        {
            do
            {
                menu.selection = (menu.selection + 1) % (int)menu.options.size();
            } while (menu.options[menu.selection].type == option_t::skip);
            m_nav_sfx->stop(); m_nav_sfx->play();
        }
        if (nav_up)
        {
            do
            {
                menu.selection = (menu.selection + ((int)menu.options.size() - 1)) % (int)menu.options.size();
            } while (menu.options[menu.selection].type == option_t::skip);
            m_nav_sfx->stop(); m_nav_sfx->play();
        }
    }

    auto& option = menu.options[menu.selection];
    bool should_callback = false;

    switch (option.type)
    {
        case option_t::action:
            if (nav_activate)
            {
                should_callback = true;
                m_action_sfx->stop(); m_action_sfx->play();
            }
            break;
        case option_t::choice:
            if (nav_right || nav_activate)
            {
                option.choice = (option.choice + 1) % (int)option.choices.size();
                should_callback = true;
                m_choice_sfx->stop(); m_choice_sfx->play();
            }
            if (nav_left)
            {
                option.choice = (option.choice + ((int)option.choices.size() - 1)) % (int)option.choices.size();
                should_callback = true;
                m_choice_sfx->stop(); m_choice_sfx->play();
            }
            break;
        case option_t::slider:
            if (nav_right)
            {
                option.choice = onut::min(option.choice + 1, (int)option.choices.size() - 1);
                should_callback = true;
                m_choice_sfx->stop(); m_choice_sfx->play();
            }
            if (nav_left)
            {
                option.choice = onut::max(option.choice - 1, 0);
                should_callback = true;
                m_choice_sfx->stop(); m_choice_sfx->play();
            }
            break;
        case option_t::key_bind:
            if (nav_activate)
            {
                m_binding_input_action = option.input_action;
                m_menus[(int)state_t::key_bind_popup].mapping_index = menu.mapping_index;
                m_binding_input_down = Input::None;
                option.input_action->bind(menu.mapping_index, Input::None, Input::None);

                m_action_sfx->stop(); m_action_sfx->play();
                push_menu(state_t::key_bind_popup);
            }
            if (clear_binding)
            {
                option.input_action->bind(menu.mapping_index, Input::None, Input::None);
                m_info.gameplay_input_context->save_settings();
                m_action_sfx->stop(); m_action_sfx->play();
            }
            break;
    }

    if (nav_back &&
        m_menu_stack.back() != state_t::main_menu &&
        m_menu_stack.back() != state_t::ap_connecting_menu)
    {
        auto state = m_menu_stack.back();
        m_menu_stack.pop_back();
        if (state == state_t::pause_menu)
        {
            dismissed_pause_menu_delegate();
        }
    }

    if (should_callback && option.activate_callback)
    {
        option.activate_callback(&option);
    }
}


void MenuManager::hide()
{
    m_menu_stack.clear();
}


void MenuManager::show_in_game_menu()
{
    m_last_inputs = { true, true, true, true, true, true, true, true };
    m_menu_stack.clear();
    m_action_sfx->stop(); m_action_sfx->play();
    push_menu(state_t::pause_menu);
}


void MenuManager::draw_menu(int x, int y, state_t state, bool draw_cursor)
{
    auto& menu = m_menus[(int)state];
    
    x = menu.x;
    y = menu.y;

    int spacing = menu.spaced ? 2 : 1;
    if (menu.columns > 1)
    {
        int w = menu.w;
        int h = onut::max(menu.h, ((int)std::ceilf((float)menu.options.size() / (float)menu.columns)) * spacing + 2);

        m_info.tile_drawer->draw_ui_frame(x, y, w, h, !menu.framed);

        char option_text[260];
        int col = 0;
        int option_y = y + spacing;
        for (const auto& option : menu.options)
        {
            snprintf(option_text, 260, "%s", option.name.c_str());
            m_info.tile_drawer->draw_text(x + 2 + col * (w - 3) / menu.columns, option_y, option_text);
            col++;
            if (col == menu.columns)
            {
                col = 0;
                option_y += spacing;
            }
        }

        if (draw_cursor)
        {
            m_info.tile_drawer->draw_cursor(x + 1 + (menu.selection % menu.columns) * (w - 3) / menu.columns,
                                       y + spacing + (menu.selection / menu.columns) * spacing);
        }
        return;
    }
    else
    {
        int w = menu.w;
        int h = onut::max(menu.h, (int)menu.options.size() * spacing + 2);

        for (const auto& option : menu.options)
        {
            switch (option.type)
            {
                case option_t::action:
                case option_t::skip:
                    break;
                case option_t::slider:
                case option_t::choice:
                    w = 31 - x;
                    break;
                case option_t::key_bind:
                    w = 30;
                    break;
            }
            w = onut::max(w, (int)option.name.size() + 4);
        }
        if (menu.options.back().type == option_t::key_bind) h++;

        x = onut::min(x, 31 - w);
        y = onut::max(1, onut::min(y, 28 - h) + 1);
        if (h > 28) y = 1;

        m_info.tile_drawer->draw_ui_frame(x, y, w, h, !menu.framed);

        int option_y = y + spacing;
        int text_w = w - 4;
        char option_text[260];
        for (const auto& option : menu.options)
        {
            switch (option.type)
            {
                case option_t::action:
                case option_t::skip:
                    snprintf(option_text, 260, "%s", option.name.c_str());
                    break;
                case option_t::choice:
                    if (option.choice == option.default_choice)
                    {
                        snprintf(option_text, 260, "%s%*s",
                                 option.name.c_str(),
                                 text_w - (int)option.name.size(),
                                 option.choices[option.choice].c_str());
                    }
                    else
                    {
                        snprintf(option_text, 260, "%s^1%*s",
                                 option.name.c_str(),
                                 text_w - (int)option.name.size(),
                                 option.choices[option.choice].c_str());
                    }
                    break;
                case option_t::slider:
                    snprintf(option_text, 260, "%s%*s",
                             option.name.c_str(),
                             text_w - (int)option.name.size(),
                             option.choices[option.choice].c_str());
                    break;
                case option_t::key_bind:
                    if (option.input_action->modifiers[menu.mapping_index].is_bound())
                    {
                        snprintf(option_text, 260, "%s%*s\n%*s",
                                 option.name.c_str(),
                                 text_w - (int)option.name.size(),
                                 option.input_action->get_modifier_name(menu.mapping_index).c_str(),
                                 text_w,
                                 option.input_action->get_state_name(menu.mapping_index).c_str());
                    }
                    else
                    {
                        snprintf(option_text, 260, "%s%*s",
                                 option.name.c_str(),
                                 text_w - (int)option.name.size(),
                                 option.input_action->get_state_name(menu.mapping_index).c_str());
                    }
                    break;
                case option_t::key_binding:
                    if (m_binding_input_down.is_bound())
                    {
                        auto ret = m_binding_input_down.get_name();
                        std::transform(ret.begin(), ret.end(), ret.begin(), toupper);
                        snprintf(option_text, 260, "%s", ret.c_str());
                    }
                    break;
            }

            m_info.tile_drawer->draw_text(x + 2, option_y, option_text);
            option_y += spacing;
        }

        if (draw_cursor)
        {
            m_info.tile_drawer->draw_cursor(x + 1, y + spacing + menu.selection * spacing);
        }
    } // 1 column
}


void MenuManager::render()
{
    oRenderer->renderStates.renderTargets[0].push(m_framebuffer);
    oRenderer->clear(Color(0.0f, 0.0f, 0.0f, 0.0f));
    oSpriteBatch->begin();
    {
        int x = 2;
        int y = 6;
        for (auto menu : m_menu_stack)
        {
            draw_menu(x, y, menu, menu == m_menu_stack.back() && m_menus[(int)menu].options[0].type != option_t::hide_cursor);
            x += 2;
            y += 2;
        }
    }
    oSpriteBatch->end();
    oRenderer->renderStates.renderTargets[0].pop();

    auto res = OScreenf;
    float scale = std::floorf(res.y / (float)PPU::SCREEN_H);
    oSpriteBatch->begin();
    oSpriteBatch->drawSprite(m_framebuffer, OScreenCenterf, Color::White, 0.0f, scale);
    oSpriteBatch->end();
}


void MenuManager::push_menu(state_t state)
{
    m_menu_stack.push_back(state);
    auto& menu = m_menus[(int)state];
    menu.selection = 0;
    while (menu.options[menu.selection].type == option_t::skip)
    {
        menu.selection = (menu.selection + 1) % (int)menu.options.size();
    }
    for (auto& option : menu.options)
    {
        if (option.load_callback)
        {
            option.load_callback(&option);
        }
    }
}


void MenuManager::on_new_game(menu_option_t* option)
{
    if (new_game_delegate)
    {
        new_game_delegate();
    }
}


void MenuManager::on_continue(menu_option_t* option)
{
    if (continue_game_delegate)
    {
        continue_game_delegate();
    }
}


void MenuManager::on_archipelago(menu_option_t* option)
{
    m_action_sfx->stop(); m_action_sfx->play();
    push_menu(state_t::ap_connect_menu);
}


void MenuManager::on_options(menu_option_t* option)
{
    m_action_sfx->stop(); m_action_sfx->play();
    push_menu(state_t::option_menu);
}


void MenuManager::on_quit(menu_option_t* option)
{
    OQuit();
}


void MenuManager::on_ap_address(menu_option_t* option)
{
    auto address = oSettings->getUserSetting("ap_address");
    auto new_address = tinyfd_inputBox("Archipelago Address", "Enter the Archipelago address. Followed by a colon then the port. i.e.: archipelago.gg:38281", address.c_str());
    if (!new_address) return; // Canceled
    oSettings->setUserSetting("ap_address", new_address);
    load_ap_address(&m_menus[(int)state_t::ap_connect_menu].options[1]);
}


void MenuManager::on_ap_slot(menu_option_t* option)
{
    auto slot = oSettings->getUserSetting("ap_slot");
    auto new_slot = tinyfd_inputBox("Archipelago Address", "Enter the Archipelago slot name. This correspond to the player name.", slot.c_str());
    if (!new_slot) return; // Canceled
    oSettings->setUserSetting("ap_slot", new_slot);
    load_ap_slot(&m_menus[(int)state_t::ap_connect_menu].options[3]);
}


void MenuManager::on_ap_password(menu_option_t* option)
{
    auto password = oSettings->getUserSetting("ap_password");
    auto new_password = tinyfd_inputBox("Archipelago Address", "Enter the password.", nullptr);
    if (!new_password) return; // Canceled
    oSettings->setUserSetting("ap_password", new_password);
    load_ap_password(&m_menus[(int)state_t::ap_connect_menu].options[5]);
}


void MenuManager::load_ap_address(menu_option_t* option)
{
    option->name = "  " + onut::toUpper(oSettings->getUserSetting("ap_address"));
    if (option->name.size() > 26) option->name.resize(26);
}


void MenuManager::load_ap_slot(menu_option_t* option)
{
    option->name = "  " + onut::toUpper(oSettings->getUserSetting("ap_slot"));
    if (option->name.size() > 26) option->name.resize(26);
}


void MenuManager::load_ap_password(menu_option_t* option)
{
    auto password = oSettings->getUserSetting("ap_password");
    for (auto& c : password) c = '*';
    option->name = "  " + password;
    if (option->name.size() > 26) option->name.resize(26);
    m_menus[(int)state_t::ap_connect_menu].selection = 6;
}


void MenuManager::on_ap_connect(menu_option_t* option)
{
    if (play_ap_delegate)
    {
        play_ap_delegate();
    }
    m_action_sfx->stop(); m_action_sfx->play();
    push_menu(state_t::ap_connecting_menu);
}


void MenuManager::on_ap_connect_failed()
{
    m_menu_stack.pop_back();
    m_error_sfx->stop(); m_error_sfx->play();
    push_menu(state_t::ap_failed_menu);
}


void MenuManager::on_save_and_quit(menu_option_t* option)
{
    if (save_delegate)
    {
        save_delegate();
    }
    OQuit();
}


void MenuManager::on_gameplay(menu_option_t* option)
{
    m_action_sfx->stop(); m_action_sfx->play();
    push_menu(state_t::gameplay_menu);
}


void MenuManager::on_audio(menu_option_t* option)
{
    m_action_sfx->stop(); m_action_sfx->play();
    push_menu(state_t::audio_menu);
}


void MenuManager::on_cosmetic(menu_option_t* option)
{
    m_action_sfx->stop(); m_action_sfx->play();
    push_menu(state_t::cosmetic_menu);
}


void MenuManager::on_input_mappings(menu_option_t* option)
{
    m_action_sfx->stop(); m_action_sfx->play();
    push_menu(state_t::input_mapping);
}


void MenuManager::on_new_game_options(menu_option_t* option)
{
    m_action_sfx->stop(); m_action_sfx->play();
    push_menu(state_t::new_game_options);
}


void MenuManager::on_mappings2(menu_option_t* option)
{
    m_action_sfx->stop(); m_action_sfx->play();
    push_menu(state_t::alternatives1);
}


void MenuManager::on_mappings3(menu_option_t* option)
{
    m_action_sfx->stop(); m_action_sfx->play();
    push_menu(state_t::alternatives2);
}


void MenuManager::on_reset_defaults(menu_option_t* option)
{
    m_info.gameplay_input_context->set_default_bindings();
    m_info.gameplay_input_context->save_settings();
    m_action_sfx->stop(); m_action_sfx->play();
    push_menu(state_t::reset_defaults);
}


void MenuManager::on_generic_ok(menu_option_t* option)
{
    m_menu_stack.pop_back();
}


void MenuManager::on_reset_ok(menu_option_t* option)
{
    m_menu_stack.pop_back();
}


void MenuManager::on_dialog_speed(menu_option_t* option)
{
    oSettings->setUserSetting("dialog_speed", std::to_string(option->choice));
    m_info.patcher->apply_dialog_speed_setting_patch();
}

void MenuManager::load_dialog_speed(menu_option_t* option)
{
    option->choice = get_user_setting("dialog_speed", 0);
}


void MenuManager::on_music_volume(menu_option_t* option)
{
    oSettings->setUserSetting("music_volume", std::to_string(option->choice));
}


void MenuManager::load_music_volume(menu_option_t* option)
{
    option->choice = get_user_setting("music_volume", 4);
}


void MenuManager::on_sound_volume(menu_option_t* option)
{
    oSettings->setUserSetting("sound_volume", std::to_string(option->choice));
    m_sfx_volume = (float)option->choice / 8.0f;
    m_action_sfx->setVolume(m_sfx_volume);
    m_choice_sfx->setVolume(m_sfx_volume);
    m_nav_sfx->setVolume(m_sfx_volume);
    m_error_sfx->setVolume(m_sfx_volume);
}


void MenuManager::load_sound_volume(menu_option_t* option)
{
    option->choice = get_user_setting("sound_volume", 4);
}


void MenuManager::on_npc_blinking_speed(menu_option_t* option)
{
    oSettings->setUserSetting("npc_blinking", std::to_string(option->choice));
}


void MenuManager::load_npc_blinking_speed(menu_option_t* option)
{
    option->choice = get_user_setting("npc_blinking", 0);
}


void MenuManager::on_king_golds(menu_option_t* option)
{
    oSettings->setUserSetting("king_golds", std::to_string(option->choice));
    m_info.patcher->apply_king_golds_setting_patch();
}


void MenuManager::load_king_golds(menu_option_t* option)
{
    option->choice = get_user_setting("king_golds", 0);
}


void MenuManager::on_coins_despawn(menu_option_t* option)
{
    oSettings->setUserSetting("coins_despawn", std::to_string(option->choice));
    m_info.patcher->apply_coins_despawn_setting_patch();
}


void MenuManager::load_coins_despawn(menu_option_t* option)
{
    option->choice = get_user_setting("coins_despawn", 0);
}


void MenuManager::on_mist_quality(menu_option_t* option)
{
    oSettings->setUserSetting("mist_quality", std::to_string(option->choice));
    m_info.patcher->apply_mist_quality_setting_patch();
}


void MenuManager::load_mist_quality(menu_option_t* option)
{
    option->choice = get_user_setting("mist_quality", 0);
}


void MenuManager::on_cigarettes(menu_option_t* option)
{
    oSettings->setUserSetting("cigarettes", std::to_string(option->choice));
    m_info.patcher->apply_cigarettes_setting_patch();
}


void MenuManager::load_cigarettes(menu_option_t* option)
{
    option->choice = get_user_setting("cigarettes", 0);
}


void MenuManager::on_double_golds(menu_option_t* option)
{
    oSettings->setUserSetting("double_golds", std::to_string(option->choice));
    m_info.patcher->apply_double_golds_setting_patch();
}


void MenuManager::load_double_golds(menu_option_t* option)
{
    option->choice = get_user_setting("double_golds", 0);
}


void MenuManager::on_double_xp(menu_option_t* option)
{
    oSettings->setUserSetting("double_xp", std::to_string(option->choice));
    m_info.patcher->apply_double_xp_setting_patch();
}


void MenuManager::load_double_xp(menu_option_t* option)
{
    option->choice = get_user_setting("double_xp", 0);
}


void MenuManager::on_equip_in_shops_xp(menu_option_t* option)
{
    oSettings->setUserSetting("equip_in_shops", std::to_string(option->choice));
    m_info.patcher->apply_equip_in_shops_setting_patch();
}


void MenuManager::load_equip_in_shops_xp(menu_option_t* option)
{
    option->choice = get_user_setting("equip_in_shops", 0);
}


void MenuManager::on_start_full_health(menu_option_t* option)
{
    oSettings->setUserSetting("start_full_health", std::to_string(option->choice));
    m_info.patcher->apply_start_full_health_setting_patch();
}


void MenuManager::load_start_full_health(menu_option_t* option)
{
    option->choice = get_user_setting("start_full_health", 0);
}


void MenuManager::on_secret_items_counter(menu_option_t* option)
{
    oSettings->setUserSetting("secret_items_counter", std::to_string(option->choice));
    m_info.patcher->apply_secret_item_setting_patch();
}


void MenuManager::load_secret_items_counter(menu_option_t* option)
{
    option->choice = get_user_setting("secret_items_counter", 0);
}


void MenuManager::on_show_level_popup(menu_option_t* option)
{
    oSettings->setUserSetting("show_level_popup", std::to_string(option->choice));
}


void MenuManager::load_show_level_popup(menu_option_t* option)
{
    option->choice = get_user_setting("show_level_popup", 0);
}


void MenuManager::on_keep_gold(menu_option_t* option)
{
    oSettings->setUserSetting("keep_gold", std::to_string(option->choice));
    m_info.patcher->apply_reset_gold_xp_setting_patch();
}


void MenuManager::load_keep_gold(menu_option_t* option)
{
    option->choice = get_user_setting("keep_gold", 0);
}


void MenuManager::on_keep_xp(menu_option_t* option)
{
    oSettings->setUserSetting("keep_xp", std::to_string(option->choice));
    m_info.patcher->apply_reset_gold_xp_setting_patch();
}


void MenuManager::load_keep_xp(menu_option_t* option)
{
    option->choice = get_user_setting("keep_xp", 0);
}


void MenuManager::on_xp_timeouts(menu_option_t* option)
{
    oSettings->setUserSetting("xp_wingboots", std::to_string(option->choice));
    m_info.patcher->apply_xp_wingboots_setting_patch();
}


void MenuManager::load_xp_timeouts(menu_option_t* option)
{
    option->choice = get_user_setting("xp_wingboots", 0);
}


void MenuManager::on_xp_speed(menu_option_t* option)
{
    oSettings->setUserSetting("xp_speed", std::to_string(option->choice));
    m_info.patcher->apply_xp_speed_setting_patch();
}


void MenuManager::load_xp_speed(menu_option_t* option)
{
    option->choice = get_user_setting("xp_speed", 0);
}


void MenuManager::on_pendant(menu_option_t* option)
{
    oSettings->setUserSetting("pendant", std::to_string(option->choice));
    m_info.patcher->apply_pendant_setting_patch();
}


void MenuManager::load_pendant(menu_option_t* option)
{
    option->choice = get_user_setting("pendant", 0);
}


void MenuManager::on_fast_cpu(menu_option_t* option)
{
    oSettings->setUserSetting("fast_cpu", std::to_string(option->choice));
}


void MenuManager::load_fast_cpu(menu_option_t* option)
{
    option->choice = get_user_setting("fast_cpu", 0);
}
