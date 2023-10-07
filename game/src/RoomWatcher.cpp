#include "RoomWatcher.h"
#include "CPUBUS.h"
#include "PPU.h"
#include "TileDrawer.h"

#include <onut/Curve.h>
#include <onut/Renderer.h>
#include <onut/Settings.h>
#include <onut/SpriteBatch.h>
#include <onut/Texture.h>


static const float ROOM_ANIM_DURATION = 3.0f;
static const float ROOM_ANIM_SLIDE_DURATION = 0.25f;


RoomWatcher::RoomWatcher(TileDrawer* tile_drawer, CPUBUS* cpu_bus)
    : m_tile_drawer(tile_drawer)
    , m_cpu_bus(cpu_bus)
{
    m_framebuffer = OTexture::createRenderTarget({ PPU::SCREEN_W, PPU::SCREEN_H - 16 });
    m_room_anim = 0.0f;
    m_room_name = "";
}


void RoomWatcher::update(float dt)
{
    // We use a mix of level id and screen id to determine the name of the current room.
    // These names are not in the game except for towns. The rest is made up.
    uint8_t level_id;
    uint8_t screen_id;
    m_cpu_bus->read(0x0024, &level_id);
    m_cpu_bus->read(0x0063, &screen_id);

    std::string room_name = "";
    switch (level_id)
    {
        case 0:
            if (screen_id > 0) room_name = "EOLIS";
            break;
        case 1:
            if (screen_id >= 0 && screen_id <= 7) room_name = "TRUNK"; // PATH TO APOLUNE
            else if ((screen_id >= 8 && screen_id <= 12) || (screen_id >= 22 && screen_id <= 26)) room_name = "TRUNK"; // PATH TO FOREPAW
            else if (screen_id >= 13 && screen_id <= 21) room_name = "TOWER OF TRUNK";
            else if (screen_id == 62 || screen_id == 63) room_name = "JOKER SPRING";
            else if (screen_id >= 28 && screen_id <= 40) room_name = "SKY SPRING";
            else room_name = "TOWER OF FORTRESS";
            break;
        case 2:
            if (screen_id >= 77 && screen_id <= 79) room_name = "TOWER OF RED POTION";
            else if (screen_id >= 48 && screen_id <= 62) room_name = "TOWER OF SUFFER";
            else if (screen_id >= 80 && screen_id <= 82) room_name = "USELESS TOWER";
            else if (screen_id >= 63 && screen_id <= 76) room_name = "TOWER OF MIST";
            else if ((screen_id >= 23 && screen_id <= 33) || (screen_id >= 37 && screen_id <= 41) || (screen_id >= 45 && screen_id <= 47)) room_name = "MIST"; // "PATH TO TOWER OF MIST";
            else if ((screen_id >= 1 && screen_id <= 3) || (screen_id >= 7 && screen_id <= 9) || (screen_id >= 16 && screen_id <= 17)) room_name = "MIST"; // "PATH TO MASCON";
            else room_name = "MIST"; // "PATH TO VICTIM";
            break;
        case 3:
            switch (screen_id)
            {
                case 0:
                case 1:
                    room_name = "APOLUNE";
                    break;
                case 2:
                case 3:
                    room_name = "FOREPAW";
                    break;
                case 4:
                case 5:
                    room_name = "NASCON";
                    break;
                case 6:
                case 7:
                    room_name = "VICTIM";
                    break;
                case 8:
                case 9:
                    room_name = "CONFLATE";
                    break;
                case 10:
                case 11:
                    room_name = "DAYBREAK";
                    break;
                case 12:
                case 13:
                    room_name = "DARTMOOR";
                    break;
            }
            break;
        case 4:
            // SHOPS. We don't show that, we keep within the same level
            break;
        case 5:
            room_name = "BRANCHES";
            break;
        case 6:
            if (screen_id >= 16) room_name = "FRATERNAL CASTLE";
            else room_name = "DARTMOOR CASTLE";
            break;
        case 7:
            room_name = "EVIL FORTRESS";
            break;
    }

    if (room_name != m_room_name && room_name != "")
    {
        m_room_anim = ROOM_ANIM_DURATION;
        m_room_name = room_name;
    }

    if (m_room_anim > 0.0f)
    {
        m_room_anim -= dt;
    }
}


void RoomWatcher::render()
{
    if (oSettings->getUserSetting("show_level_popup") == "0") return;

    uint8_t level_id;
    m_cpu_bus->read(0x0024, &level_id);
    uint8_t screen_id;
    m_cpu_bus->read(0x0063, &screen_id);

    oRenderer->renderStates.renderTargets[0].push(m_framebuffer);
    oRenderer->clear(Color(0.0f, 0.0f, 0.0f, 0.0f));
    oSpriteBatch->begin();
    if (m_room_anim > 0.0f)
    {
        bool show_at_bottom = false;
        float t = 1.0f;
        if (m_room_anim > ROOM_ANIM_DURATION - ROOM_ANIM_SLIDE_DURATION)
        {
            t = (ROOM_ANIM_DURATION - m_room_anim) / ROOM_ANIM_SLIDE_DURATION;
        }
        else if (m_room_anim < ROOM_ANIM_SLIDE_DURATION)
        {
            t = m_room_anim / ROOM_ANIM_SLIDE_DURATION;
        }
        int len = (int)m_room_name.size();
        int y = onut::lerp(show_at_bottom ? PPU::SCREEN_H : (-3 * 8), show_at_bottom ? (PPU::SCREEN_H - 5 * 8) : 0, t);
        m_tile_drawer->draw_ui_frame_fine((16 - len / 2 - 2) * 8, y, len + 4, 3);
        if (y == (show_at_bottom ? (PPU::SCREEN_H - 5 * 8) : 0))
            m_tile_drawer->draw_text(16 - len / 2, show_at_bottom ? (PPU::SCREEN_H / 8 - 4) : 1, m_room_name.c_str());

    }
    //m_tile_drawer->draw_text(0, 0, std::to_string(level_id).c_str());
    //m_tile_drawer->draw_text(0, 1, std::to_string(screen_id).c_str());
    oSpriteBatch->end();
    oRenderer->renderStates.renderTargets[0].pop();

    auto res = OScreenf;
    float scale = std::floorf(res.y / (float)PPU::SCREEN_H);
    oSpriteBatch->begin();
    oSpriteBatch->drawSprite(m_framebuffer, OScreenCenterf, Color::White, 0.0f, scale);
    oSpriteBatch->end();
}
