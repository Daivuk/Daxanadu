#include "Daxanadu.h"
#include "AP.h"
#include "APU.h"
#include "Cart.h"
#include "Controller.h"
#include "Emulator.h"
#include "ExternalInterface.h"
#include "GameplayInputContext.h"
#include "MenuInputContext.h"
#include "MenuManager.h"
#include "NewGameInputContext.h"
#include "Patcher.h"
#include "PPU.h"
#include "RAM.h"
#include "RoomWatcher.h"
#include "SoundRenderer.h"
#include "TileDrawer.h"
#include "version.h"

#include <onut/Files.h>
#include <onut/Font.h>
#include <onut/Input.h>
#include <onut/Log.h>
#include <onut/Renderer.h>
#include <onut/Sound.h>
#include <onut/Settings.h>
#include <onut/SpriteBatch.h>

#include <vector>


static const int32_t STATE_VERSION = 8;
static const int32_t MIN_STATE_VERSION = 1;


Daxanadu::Daxanadu()
{
    init();
}


Daxanadu::~Daxanadu()
{
    cleanup();
}


void Daxanadu::init()
{
    // Reinitialize variables
    m_emulator = nullptr;
    m_patcher = nullptr;
    m_menu_manager = nullptr;
    m_tile_drawer = nullptr;
    m_loading_continue_state = false;
    m_gameplay_input_context = nullptr;
    m_menu_input_context = nullptr;
    m_new_game_input_context = nullptr;
    m_room_watcher = nullptr;
    m_active_sound = nullptr;
    m_ap = nullptr;
    m_need_reset = false;
    m_king_gave_money = 0;

    // Allocate emulator
    m_loading_continue_state = false;
    m_gameplay_input_context = new GameplayInputContext();
    m_menu_input_context = new MenuInputContext(m_gameplay_input_context);
    m_new_game_input_context = new NewGameInputContext();
    m_emulator = new Emulator();
    m_emulator->get_ram()->cpu_write(0x800, 0xFF); // Input context flag
    m_emulator->get_controller()->set_input_context(nullptr);

    // Create sounds
    auto sound_renderer = new SoundRenderer(m_emulator->get_cart()->get_prg_rom(), m_emulator->get_cart()->get_prg_rom_size());
    for (int i = 0; i < 28; ++i)
    {
        m_sounds[i] = sound_renderer->render_sound(i + 16);
    }
    delete sound_renderer;

    m_patcher = new Patcher(m_emulator->get_cart()->get_prg_rom());
    m_tile_drawer = new TileDrawer(m_emulator->get_cart()->get_prg_rom(), m_emulator->get_ppu());

    menu_manager_info_t menu_manager_info;
    menu_manager_info.apu = m_emulator->get_apu();
    menu_manager_info.tile_drawer = m_tile_drawer;
    menu_manager_info.patcher = m_patcher;
    menu_manager_info.gameplay_input_context = m_gameplay_input_context;
    menu_manager_info.menu_input_context = m_menu_input_context;
    menu_manager_info.action_sfx = m_sounds[0x18 - 1];
    menu_manager_info.choice_sfx = m_sounds[0x08 - 1];
    menu_manager_info.nav_sfx = m_sounds[0x0B - 1];
    menu_manager_info.error_sfx = m_sounds[0x0D - 1];
    m_menu_manager = new MenuManager(menu_manager_info);

    m_room_watcher = new RoomWatcher(m_tile_drawer, m_emulator->get_cpu_bus());

    update_volumes();


    //--- Delegates
    m_menu_manager->new_game_delegate = [this]()
    {
        // Change mapping to a new game context mapping that will always return "start" pressed
        m_emulator->get_ram()->cpu_write(0x800, 2); // Input context flag
        m_emulator->get_controller()->set_input_context(m_new_game_input_context);
    };

    m_menu_manager->continue_game_delegate = [this]()
    {
        // Is there a state to load?
        auto filename = "save_states/state_0.sav";
        if (onut::fileExists(filename))
        {
            m_menu_manager->hide();
            load_state(0);
        }
        else
        {
            // Do nothing, play error sound
        }
    };

    m_menu_manager->save_delegate = [this]()
    {
        save_state(0);
    };

    m_menu_manager->dismissed_pause_menu_delegate = [this]()
    {
        m_menu_manager->hide();
        m_emulator->get_ram()->cpu_write(0x800, 2); // Input context flag
        m_emulator->get_controller()->set_input_context(m_new_game_input_context); // New game context just shoots "starts" continuously so it will resume the game
    };

    m_menu_manager->dismissed_ap_error = [this]()
    {
        m_emulator->get_ram()->cpu_write(0x800, 0xFF); // Input context flag
        m_emulator->get_controller()->set_input_context(nullptr); // We will reset cart next frame, make sure no inputs interfere
        m_need_reset = true;
    };

    m_menu_manager->play_ap_delegate = [this]()
    {
        ap_info_t ap_info;
        ap_info.address = oSettings->getUserSetting("ap_address");
        ap_info.slot_name = oSettings->getUserSetting("ap_slot");
        ap_info.password = oSettings->getUserSetting("ap_password");
        ap_info.rom = m_emulator->get_cart()->get_prg_rom();
        ap_info.rom_size = m_emulator->get_cart()->get_prg_rom_size();
        ap_info.ram = m_emulator->get_ram();
        ap_info.tile_drawer = m_tile_drawer;
        ap_info.patcher = m_patcher;
        ap_info.external_interface = m_emulator->get_external_interface();
        ap_info.cpu_bus = m_emulator->get_cpu_bus();
        ap_info.cart = m_emulator->get_cart();
        ap_info.cpu = m_emulator->get_cpu();
        m_ap = new AP(ap_info);
        m_ap->connection_success_delegate = [this]()
        {
            m_menu_manager->hide();

            auto filename = m_ap->get_dir_name() + "/state_0.sav";
            if (onut::fileExists(filename))
            {
                load_state(0);
                return;
            }

            // Change mapping to a new game context mapping that will always return "start" pressed
            m_emulator->get_ram()->cpu_write(0x800, 2); // Input context flag
            m_emulator->get_controller()->set_input_context(m_new_game_input_context);
        };
        m_ap->connection_failed_delegate = [this]()
        {
            m_menu_manager->on_ap_connect_failed();
            m_emulator->get_ram()->cpu_write(0x800, 1); // Input context flag
            m_emulator->get_controller()->set_input_context(m_menu_input_context);
        };
        m_ap->connect();
    };

    //--- C++ callbacks

    // Save game (Meditate)
    m_emulator->get_external_interface()->register_callback(0x01, [this](uint8_t a, uint8_t b, uint8_t c, uint8_t d) -> uint8_t
    {
        uint8_t save_flag = 0;
        m_emulator->get_ram()->cpu_read(0x801, &save_flag);
        if (save_flag)
        {
            m_emulator->get_ram()->cpu_write(0x801, 0); // How was this save state saved
            m_patcher->apply_welcome_back(); // To show "welcome back"
            return 0;
        }

        m_emulator->get_ram()->cpu_write(0x801, 1); // So when we load back, we should "welcome back"
        save_state(0);
        m_emulator->get_ram()->cpu_write(0x801, 0); // Revert back the save flag so we can proceed
        m_patcher->apply_progress_saved(); // To show progress saved dialog
        return 0;
    }, 0);

    // Continue game
    m_emulator->get_external_interface()->register_callback(0x02, [this](uint8_t a, uint8_t b, uint8_t c, uint8_t d) -> uint8_t
    {
        load_state(0);
        return 0;
    }, 0);

    // Scroll mist
    m_emulator->get_external_interface()->register_callback(0x03, [this](uint8_t a, uint8_t b, uint8_t c, uint8_t d) -> uint8_t
    {
        auto cart = m_emulator->get_cart();
        auto ppu = m_emulator->get_ppu();

        // To make sure we're not in a mist tower, check the palette.
        // Kind of hacky, but oh well... Just check byte 4 to 7
        static uint8_t MIST_TOWERS_DESIRED_PAL[4] = { 0x0F, 0x09, 0x1B, 0x27 };
        uint8_t mist_towers_pal[4];
        ppu->ppu_read(0x3F04, &mist_towers_pal[0]);
        ppu->ppu_read(0x3F05, &mist_towers_pal[1]);
        ppu->ppu_read(0x3F06, &mist_towers_pal[2]);
        ppu->ppu_read(0x3F07, &mist_towers_pal[3]);
        if (memcmp(mist_towers_pal, MIST_TOWERS_DESIRED_PAL, 4) == 0) return 0;

        static const int mist_tile_addrs[11] = { 0x1970, 0x1890, 0x1800, 0x1810, 0x1820, 0x1830, 0x1840, 0x1850, 0x1860, 0x1870, 0x1880 };
        static const int mist_speeds[11] = { 20, 20, 16, 12, 7, 6, 6, 6, 5, 5, 5 };
        static int phase = 0;

        phase++;

        uint8_t line;
        for (int i = 0; i < 11; ++i)
        {
            if (phase % mist_speeds[i]) continue;
            int addr = mist_tile_addrs[i];
            for (int y = 0; y < 16; ++y)
            {
                cart->ppu_read(addr + y, &line);
                int bit = line & 0b1;
                line >>= 1;
                line = (line & 0b01111111) | (bit << 7);
                cart->ppu_write(addr + y, line);
            }
        }

        return 0;
    }, 0);

    // Get if king give us 1500 yet, and change the flag if he didn't give the money yet.
    m_emulator->get_external_interface()->register_callback(0x04, [this](uint8_t a, uint8_t b, uint8_t c, uint8_t d) -> uint8_t
    {
        uint8_t ret = m_king_gave_money;
        m_king_gave_money = 1;
        return ret;
    }, 0);

    // New game
    m_emulator->get_external_interface()->register_callback(0x05, [this](uint8_t a, uint8_t b, uint8_t c, uint8_t d) -> uint8_t
    {
        m_menu_manager->hide();
        m_emulator->get_ram()->cpu_write(0x800, 0); // Input context flag
        m_emulator->get_controller()->set_input_context(m_gameplay_input_context);
        return 0;
    }, 0);

    // Game pausing
    m_emulator->get_external_interface()->register_callback(0x06, [this](uint8_t a, uint8_t b, uint8_t c, uint8_t d) -> uint8_t
    {
        m_menu_manager->show_in_game_menu();
        m_emulator->get_ram()->cpu_write(0x800, 0xFF); // Input context flag
        m_emulator->get_controller()->set_input_context(nullptr);
        return 0;
    }, 0);

    // Game resuming
    m_emulator->get_external_interface()->register_callback(0x07, [this](uint8_t a, uint8_t b, uint8_t c, uint8_t d) -> uint8_t
    {
        m_menu_manager->hide();
        m_emulator->get_ram()->cpu_write(0x800, 0); // Input context flag
        m_emulator->get_controller()->set_input_context(m_gameplay_input_context);
        return 0;
    }, 0);

    // Show inventory
    m_emulator->get_external_interface()->register_callback(0x08, [this](uint8_t a, uint8_t b, uint8_t c, uint8_t d) -> uint8_t
    {
        if (m_emulator->get_controller()->get_input_context() == m_new_game_input_context)
        {
            // Ignore. Sometimes this is triggered and it's causing issue. We want for "hide inventory" event.
            return 0;
        }
        m_emulator->get_ram()->cpu_write(0x800, 1); // Input context flag
        m_emulator->get_controller()->set_input_context(m_menu_input_context);
        return 0;
    }, 0);

    // Hide inventory
    m_emulator->get_external_interface()->register_callback(0x09, [this](uint8_t a, uint8_t b, uint8_t c, uint8_t d) -> uint8_t
    {
        m_emulator->get_ram()->cpu_write(0x800, 0); // Input context flag
        m_emulator->get_controller()->set_input_context(m_gameplay_input_context);
        return 0;
    }, 0);

    // Play sound
    m_emulator->get_external_interface()->register_callback(0x0A, [this](uint8_t a, uint8_t b, uint8_t c, uint8_t d) -> uint8_t
    {
        a--;
        if (a < 28)
        {
            if (m_active_sound)
                m_active_sound->stop();
            m_active_sound = m_sounds[a]->createInstance();
            m_active_sound->setVolume(m_sfx_volume);
            m_active_sound->play();
        }
        return 0;
    }, 1);

    m_emulator->reset();
}


void Daxanadu::cleanup()
{
    delete m_ap;
    delete m_gameplay_input_context;
    delete m_menu_input_context;
    delete m_new_game_input_context;
    delete m_room_watcher;
    delete m_menu_manager;
    delete m_tile_drawer;
    delete m_patcher;
    delete m_emulator;
}


void Daxanadu::update_volumes()
{
    int sfx_volume_setting = 5;
    int music_volume_setting = 5;

    try
    {
        sfx_volume_setting = std::stoi(oSettings->getUserSetting("sound_volume"));
    } catch (...) {}
    try
    {
        music_volume_setting = std::stoi(oSettings->getUserSetting("music_volume"));
    } catch (...) {}

    m_sfx_volume = (float)sfx_volume_setting / 8.0f;
    m_music_volume = (float)music_volume_setting / 8.0f;

    m_emulator->get_apu()->set_volume(m_music_volume);
}


void Daxanadu::serialize(FILE* f, int version) const
{
    fwrite(&m_king_gave_money, 1, 1, f);

    // Useless, legacy
    uint8_t saved_while_medidating = 0;
    fwrite(&saved_while_medidating, 1, 1, f);

    if (m_ap) m_ap->serialize(f, version);
}


void Daxanadu::deserialize(FILE* f, int version)
{
    if (version < 3) return;
    fread(&m_king_gave_money, 1, 1, f);
    if (version >= 6)
    {
        // Useless, legacy
        uint8_t saved_while_medidating = 0;
        fread(&saved_while_medidating, 1, 1, f);
    }

    if (m_ap) m_ap->deserialize(f, version);
}


void Daxanadu::save_state(int slot)
{
    auto filename = "save_states/state_" + std::to_string(slot) + ".sav";
    if (m_ap)
    {
        filename = m_ap->get_dir_name() + "/state_" + std::to_string(slot) + ".sav";
    }
    else if (!onut::fileExists("save_states"))
	{
		printf("  save_states/ Doesn't exist, creating...\n");
		onut::createFolder("save_states");
	}

    FILE* f = fopen(filename.c_str(), "wb");
    if (!f)
    {
        onut::showMessageBox("Error", "Failed to open file: " + filename);
        return;
    }

    fwrite(&STATE_VERSION, sizeof(STATE_VERSION), 1, f);
    m_emulator->serialize(f, STATE_VERSION);
    serialize(f, STATE_VERSION);
    fclose(f);
    
    OLog("State " + std::to_string(slot) + " saved");
}


void Daxanadu::load_state(int slot)
{
    auto filename = "save_states/state_" + std::to_string(slot) + ".sav";
    if (m_ap)
    {
        filename = m_ap->get_dir_name() + "/state_" + std::to_string(slot) + ".sav";
    }
    load_state(slot, filename);
}


void Daxanadu::load_state(int slot, const std::string& filename)
{
    FILE* f = fopen(filename.c_str(), "rb");
    if (!f)
    {
        // Silently failed, maybe the state doesn't exist yet
        return;
    }

    int32_t version;
    fread(&version, sizeof(version), 1, f);

    if (version < MIN_STATE_VERSION)
    {
        fclose(f);
        onut::showMessageBox("Error", "Failed to open file: " + filename + "\nWrong version. File version: " + std::to_string(version) + ", expected: " + std::to_string(STATE_VERSION));
        return;
    }

    m_emulator->deserialize(f, version);
    deserialize(f, version);
    fclose(f);

    //if (m_loading_continue_state)
    //{
    //    m_patcher->apply_welcome_back();
    //}

    OLog("State " + std::to_string(slot) + " loaded");
    m_menu_manager->hide();
    m_emulator->get_ram()->cpu_write(0x800, 0); // Input context flag
    m_emulator->get_controller()->set_input_context(m_gameplay_input_context);
}


void Daxanadu::update(float dt)
{
    // Reset everything
    if (OInputJustPressed(OKeyF5) || m_need_reset)
    {
        cleanup();
        init();
        return;
    }

    if (m_ap)
    {
        m_ap->update(dt);
        if (m_ap->get_state() == AP::state_t::idle)
        {
            delete m_ap;
            m_ap = nullptr;
        }
    }

    for (int i = 1; i <= 9; ++i)
    {
        if (OInputJustPressed((onut::Input::State)((int)OKey1 - 1 + i)))
        {
            if (OInputPressed(OKeyLeftControl))
            {
                save_state(i);
            }
            else if (OInputPressed(OKeyLeftAlt))
            {
                auto filename = "dev_states/state_" + std::to_string(i) + ".sav";
                load_state(i, filename);
            }
            else
            {
                load_state(i);
            }
        }
    }

    // This is the continue state
    if (OInputJustPressed(OKey0))
    {
        load_state(0);
    }

    m_emulator->update(dt);
    m_menu_manager->update(dt);
    m_room_watcher->update(dt);

    update_volumes();
}


void Daxanadu::render()
{
    m_emulator->render();
    m_menu_manager->render();
    m_room_watcher->render();
    if (m_ap) m_ap->render();

    // Version
    {
        auto font = OGetFont("font.fnt");
        oSpriteBatch->begin();
        oSpriteBatch->drawText(font, DAX_VERSION_FULL_TEXT, { 0.0f, OScreenHf }, OBottomLeft);
        oSpriteBatch->end();
    }
}
