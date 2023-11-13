#pragma once

#include <onut/ForwardDeclaration.h>

#include <stdio.h>
#include <cinttypes>
#include <string>


OForwardDeclare(Sound);
OForwardDeclare(SoundInstance);
class AP;
class Emulator;
class MenuManager;
class Patcher;
class RoomWatcher;
class TileDrawer;
class GameplayInputContext;
class MenuInputContext;
class NewGameInputContext;


class Daxanadu final
{
public:
    Daxanadu();
    ~Daxanadu();

    void update(float dt);
    void render();

    Emulator* get_emulator() const { return m_emulator; }

private:
    void init();
    void cleanup();
    void update_volumes();

    void serialize(FILE* f, int version) const;
    void deserialize(FILE* f, int version);

    void save_state(int slot);
    void load_state(int slot);
    void load_state(int slot, const std::string& filename);

    Emulator* m_emulator = nullptr;
    Patcher* m_patcher = nullptr;
    MenuManager* m_menu_manager = nullptr;
    TileDrawer* m_tile_drawer = nullptr;
    bool m_loading_continue_state = false;
    GameplayInputContext* m_gameplay_input_context = nullptr;
    MenuInputContext* m_menu_input_context = nullptr;
    NewGameInputContext* m_new_game_input_context = nullptr;
    RoomWatcher* m_room_watcher = nullptr;
    OSoundRef m_sounds[28];
    OSoundInstanceRef m_active_sound;
    float m_sfx_volume = 1.0f;
    float m_music_volume = 1.0f;
    AP* m_ap = nullptr;
    bool m_need_reset = false;

    // Extra Daxanadu ram "registers"
    uint8_t m_king_gave_money = 0;
    uint8_t m_saved_while_medidating = 0;
};
