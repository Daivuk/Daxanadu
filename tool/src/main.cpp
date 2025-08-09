#include "cart.h"
#include "data.h"
#include "history.h"
#include "apu.h"
#include "..\..\game\src\Archipelago\APLocations.h"

#include <onut/onut.h>
#include <onut/Files.h>
#include <onut/Font.h>
#include <onut/Input.h>
#include <onut/PrimitiveBatch.h>
#include <onut/Renderer.h>
#include <onut/Settings.h>
#include <onut/SpriteBatch.h>
#include <onut/Texture.h>
#include <onut/Timing.h>
#include <onut/Vector2.h>

#include <imgui/imgui.h>

#include <set>


#define SHOW_SMALL_TILES 0
#define SHOW_GIZMOS 1


enum class state_t
{
    idle,
    panning,
    move,
    selection_box
};


enum class tool_t
{
    rooms
};


static const float ZOOM_LEVELS[] = { 0.11f, 0.13f, 0.16f, 0.19f, 0.23f, 0.275f, 0.33f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 1.0f, 1.25f, 1.5f, 1.75f, 2.0f, 2.5f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 10.0f, 12.0f, 14.0f, 17.0f, 20.0f };
static const int ZOOM_LEVEL_COUNT = sizeof(ZOOM_LEVELS) / sizeof(float);


static History<data_t> history;
static Vector2 cam_pos;
static int cam_zoom = 8;
static Vector2 mouse_pos;
static state_t state = state_t::idle;
static tool_t tool = tool_t::rooms;
static Vector2 mouse_pos_on_down;
static Vector2 cam_pos_on_down;
static Vector2 selected_box_start;
static cart_room_t* mouse_over_room = nullptr;
static std::vector<Point> rooms_pos_on_down;
static OFontRef font;
static std::vector<cart_room_t*> selection_on_down;
static APUAudioStreamRef apu_player;


const char* get_title_name(int title_id)
{
    switch (title_id)
    {
        case 0: return "Notive";
        case 1: return "Aspirant";
        case 2: return "Battler";
        case 3: return "Fighter";
        case 4: return "Adept";
        case 5: return "Chevalier";
        case 6: return "Veteran";
        case 7: return "Warrior";
        case 8: return "Swordsman";
        case 9: return "Hero";
        case 10: return "Soldier";
        case 11: return "Myrmidon";
        case 12: return "Champion";
        case 13: return "Superhero";
        case 14: return "Paladin";
        case 15: return "Lord";
        default: return "UNKNOWN";
    }
}


const char* get_item_name(item_t item)
{
    switch (item)
    {
        case ITEM_DAGGER: return cart.item_names[0x00].c_str();
        case ITEM_LONG_SWORD: return cart.item_names[0x01].c_str();
        case ITEM_GIANT_BLADE: return cart.item_names[0x02].c_str();
        case ITEM_DRAGON_SLAYER: return cart.item_names[0x03].c_str();

        case ITEM_LEATHER_ARMOR: return cart.item_names[0x04].c_str();
        case ITEM_STUDDED_MAIL: return cart.item_names[0x05].c_str();
        case ITEM_FULL_PLATE: return cart.item_names[0x06].c_str();
        case ITEM_BATTLE_SUIT: return cart.item_names[0x07].c_str();

        case ITEM_SMALL_SHIELD: return cart.item_names[0x08].c_str();
        case ITEM_LARGE_SHIELD: return cart.item_names[0x09].c_str();
        case ITEM_MAGIC_SHIELD: return cart.item_names[0x0A].c_str();
        case ITEM_BATTLE_HELMET: return cart.item_names[0x0B].c_str();

        case ITEM_DELUGE: return cart.item_names[0x0C].c_str();
        case ITEM_THUNDER: return cart.item_names[0x0D].c_str();
        case ITEM_FIRE: return cart.item_names[0x0E].c_str();
        case ITEM_DEATH: return cart.item_names[0x0F].c_str();
        case ITEM_TILTE: return cart.item_names[0x10].c_str();

        case ITEM_ELF_RING: return cart.item_names[0x11].c_str();
        case ITEM_RUBY_RING: return cart.item_names[0x12].c_str();
        case ITEM_DWORF_RING: return cart.item_names[0x13].c_str();
        case ITEM_DEMON_RING: return cart.item_names[0x14].c_str();
        case ITEM_ACE_KEY: return cart.item_names[0x15].c_str();
        case ITEM_KING_KEY: return cart.item_names[0x16].c_str();
        case ITEM_QUEEN_KEY: return cart.item_names[0x17].c_str();
        case ITEM_JACK_KEY: return cart.item_names[0x18].c_str();
        case ITEM_JOKER_LEY: return cart.item_names[0x19].c_str();
        case ITEM_MATTOCK: return cart.item_names[0x1A].c_str();
        case ITEM_ROD: return cart.item_names[0x1B].c_str();
        case ITEM_CRYSTAL: return cart.item_names[0x1C].c_str();
        case ITEM_LAMP: return cart.item_names[0x1D].c_str();
        case ITEM_HOURGLASS: return cart.item_names[0x1E].c_str();
        case ITEM_BOOK: return cart.item_names[0x1F].c_str();
        case ITEM_WINGBOOTS: return cart.item_names[0x20].c_str();
        case ITEM_RED_POTION: return cart.item_names[0x21].c_str();
        case ITEM_BLACK_POTION: return cart.item_names[0x22].c_str();
        case ITEM_ELIXIR: return cart.item_names[0x23].c_str();
        case ITEM_PENDANT: return cart.item_names[0x24].c_str();
        case ITEM_BLACK_ONYX: return cart.item_names[0x25].c_str();
        case ITEM_FIRE_CRYSTAL: return cart.item_names[0x26].c_str();

        default:
#if defined(MSVC)
            __debugbreak();
#endif
            return "UNKONWN ITEM";
    }
}


const char* get_entity_name(int type)
{
    switch (type)
    {
        case ENTITY_BREAD: return "Bread";
        case ENTITY_COIN: return "Coin";
        case ENTITY_RAIDEN: return "Raiden";
        case ENTITY_NECRON_AIDES: return "Necron Aides";
        case ENTITY_ZOMBIE: return "Zombie";
        case ENTITY_HORNET: return "Hornet";
        case ENTITY_BIHORUDA: return "Bihoruda";
        case ENTITY_LILITH: return "Lilith";
        case ENTITY_YAINURA: return "Yuinaru";
        case ENTITY_SNOWMAN: return "Snowman";
        case ENTITY_NASH: return "Nash";
        case ENTITY_FIRE_GIANT: return "Fire Giant";
        case ENTITY_ISHIISU: return "Ishiisu";
        case ENTITY_EXECUTION_HOOD: return "Execution Hood";
        case ENTITY_ROKUSUTAHN: return "Rokusutahn";
        case ENTITY_ROCK_SNAKE: return "UNUSED - Rock Snake";
        case ENTITY_MONSTER_DEATH_FX: return "Monster Death FX";
        case ENTITY_MONSTER_DEATH_FX_2: return "Monster Death FX";
        case ENTITY_CHARRON: return "Charron";
        case ENTITY_GERIBUTA: return "Geributa";
        case ENTITY_SUGATA: return "Sugata";
        case ENTITY_GRIMLOCK: return "Grimlock";
        case ENTITY_GIANT_BEES: return "Giant Bees";
        case ENTITY_MYCONID: return "Myconid";
        case ENTITY_NAGA: return "Naga";
        case ENTITY_STILL_NIGHT: return "UNUSED - Still Night";
        case ENTITY_GIANT_STRIDER: return "Giant Strider";
        case ENTITY_SIR_GAWAINE: return "Sir Gawaine";
        case ENTITY_MASK_MAN: return "Mask Man";
        case ENTITY_WOLF_MAN: return "Wolf Man";
        case ENTITY_YAREEKA: return "Yareeka";
        case ENTITY_MAGMAN: return "Magman";
        case ENTITY_JOUSTER: return "UNUSED - Jouster";
        case ENTITY_IKEDA: return "Ikeda";
        case ENTITY_SLUG: return "UNUSED - Slug";
        case ENTITY_LAMPREY: return "Lamprey";
        case ENTITY_MONODRON: return "Monodron";
        case ENTITY_BAT: return "UNUSED - Bat";
        case ENTITY_TAMAZUTSU: return "Tamazutsu";
        case ENTITY_RIPASHEIKU: return "Ripasheiku";
        case ENTITY_ZORADOHNA: return "Zoradohna";
        case ENTITY_BORABOHRA: return "Borabohra";
        case ENTITY_PAKUKAME: return "Pakukame";
        case ENTITY_ZORUGERIRU: return "Zorugeriru";
        case ENTITY_KING_GRIEVE: return "King Grieve";
        case ENTITY_SHADOW_EURA: return "Shadow Eura";
        case ENTITY_BANDANA_TOWN_PERSON: return "Bandana Town person";
        case ENTITY_RING_OF_DEMON: return "Ring of Demon";
        case ENTITY_RING_OF_DWARF: return "Ring of Dwarf";
        case ENTITY_BLACKSMITH: return "Blacksmith";
        case ENTITY_MARTIAL_ARTS_TRAINER: return "Martial Arts Trainer";
        case ENTITY_GURU: return "Guru";
        case ENTITY_KING: return "King";
        case ENTITY_MAGE: return "Mage";
        case ENTITY_KEY_SALESMAN: return "Key salesman";
        case ENTITY_SMOKER_TOWN_PERSON: return "Smoker town person";
        case ENTITY_DOCTOR: return "Doctor";
        case ENTITY_GRUMPY_TOWN_PERSON: return "Grumpy Town person";
        case ENTITY_BUTCHER: return "Butcher";
        case ENTITY_NICE_LADY_TOWN_PERSON: return "Nice Lady Town Person";
        case ENTITY_GUARD: return "Guard";
        case ENTITY_SITTING_TOWN_PERSON: return "Sitting Town person";
        case ENTITY_NURSE: return "Nurse";
        case ENTITY_LADY_TOWN_PERSON: return "Lady Town person";
        case ENTITY_EYE: return "UNUSED - Eye";
        case ENTITY_ZOZURA: return "Zozura";
        case ENTITY_GLOVE: return "Glove";
        case ENTITY_BLACK_ONYX: return "Black Onyx";
        case ENTITY_PENDANT: return "Pendant";
        case ENTITY_RED_POTION: return "Red Potion";
        case ENTITY_POISON: return "Poison";
        case ENTITY_ELIXIR: return "Elixir";
        case ENTITY_OINTMENT: return "Ointment";
        case ENTITY_INTRO_TRIGGER: return "Intro Trigger";
        case ENTITY_MATTOCK: return "Mattock";
        case ENTITY_STAR: return "Star";
        case ENTITY_ROCK_FOUNTAIN: return "Rock Fountain";
        case ENTITY_IMPACT: return "Impact";
        case ENTITY_MAGICS: return "Magics";
        case ENTITY_WING_BOOTS: return "Wing Boots";
        case ENTITY_HOURGLASS: return "Hourglass";
        case ENTITY_MAGICAL_ROD: return "Magical Rod";
        case ENTITY_BATTLE_SUIT: return "Battle suit";
        case ENTITY_BATTLE_HELMET: return "Battle Helmet";
        case ENTITY_DRAGON_SLAYER: return "Dragon Slayer";
        case ENTITY_MATTOCK_BOSS_REWARD: return "Mattock Boss Reward";
        case ENTITY_WING_BOOTS_BOSS_REWARD: return "Wingboots Boss Reward";
        case ENTITY_RED_POTION2: return "Red Potion 2";
        case ENTITY_POISON2: return "Poison 2";
        case ENTITY_GLOVE2: return "Glove 2";
        case ENTITY_OINTMENT2: return "Ointment 2";
        case ENTITY_TRUNK_FOUNTAIN: return "Trunk Fountain";
        case ENTITY_SKY_FOUNTAIN: return "Sky Fountain";
        case ENTITY_HIDDEN_FOUNTAIN: return "Hidden Fountain";
        case ENTITY_MONSTER_DEATH_FX_3: return "Monster Death FX";
    }
    return nullptr;
}

void initSettings()
{
    oSettings->setGameName("DaxanaduTool");
    oSettings->setResolution({1600, 900});
    oSettings->setShowFPS(true);
    oSettings->setIsResizableWindow(true);
    oSettings->setStartMaximized(true);
    oSettings->setUserSettingDefault("cam_pos_x", "0");
    oSettings->setUserSettingDefault("cam_pos_y", "0");
    oSettings->setUserSettingDefault("cam_zoom", "8");
}


void init()
{
    oAudioEngine->addInstance(apu_player);

    cam_pos.x = std::stof(oSettings->getUserSetting("cam_pos_x"));
    cam_pos.y = std::stof(oSettings->getUserSetting("cam_pos_y"));
    cam_zoom = std::stoi(oSettings->getUserSetting("cam_zoom"));

    // Assets
    font = OGetFont("font.fnt");

    // Init
    init_cart();
    apu_player = OMake<APUAudioStream>(cart.rom.data(), cart.rom.size());
    oAudioEngine->addInstance(apu_player);

    // Render sounds
    for (int i = 16; i < 44; ++i)
    {
        cart_sound_t sound;

        sound.id = i - 15;
        sound.name = MUSIC_NAMES[i];
        sound.sound = apu_player->render_sound(i);

        cart.sounds.push_back(sound);
    }

    load_data();
}


void shutdown()
{
    oAudioEngine->removeInstance(apu_player);
    apu_player.reset();

    oSettings->setUserSetting("cam_pos_x", std::to_string(cam_pos.x));
    oSettings->setUserSetting("cam_pos_y", std::to_string(cam_pos.y));
    oSettings->setUserSetting("cam_zoom", std::to_string(cam_zoom));
}


void update_shortcuts()
{
    auto ctrl = OInputPressed(OKeyLeftControl);
    auto shift = OInputPressed(OKeyLeftShift);
    auto alt = OInputPressed(OKeyLeftAlt);

    if (ctrl && !shift && !alt && OInputJustPressed(OKeyZ)) history.undo(data);
    if (ctrl && shift && !alt && OInputJustPressed(OKeyZ)) history.redo(data);
    if (ctrl && !shift && !alt && OInputJustPressed(OKeyS)) save_data();
}


cart_room_t* get_room_at(const Vector2& pos)
{
    for (auto it = cart.rooms.rbegin(); it != cart.rooms.rend(); ++it)
    {
        auto& room = *it;

        float room_x = (float)(data.room_positions[room.id].pos.x * 16);
        float room_y = (float)(data.room_positions[room.id].pos.y * 16);

        for (auto screen_id : room.screens)
        {
            auto screen = &cart.chunks[room.chunk_id].screens[screen_id];

            if (pos.x >= (float)screen->x_offset_in_room * SCREEN_BLOCKS_WIDTH * 16.0f + room_x &&
                pos.x < (float)(screen->x_offset_in_room + 1) * SCREEN_BLOCKS_WIDTH * 16.0f + room_x &&
                pos.y >= (float)screen->y_offset_in_room * SCREEN_BLOCKS_HEIGHT * 16.0f + room_y &&
                pos.y < (float)(screen->y_offset_in_room + 1) * SCREEN_BLOCKS_HEIGHT * 16.0f + room_y)
            {
                return &room;
            }
        }
    }

    return nullptr;
}


static bool operator==(const std::vector<cart_room_t*>& a, const std::vector<cart_room_t*>& b)
{
    if (a.size() != b.size()) return false;
    for (int i = 0; i < (int)a.size(); ++i)
        if (a[i] != b[i]) return false;
    return true;
}


static bool operator!=(const std::vector<cart_room_t*>& a, const std::vector<cart_room_t*>& b)
{
    if (a.size() == b.size())
        for (int i = 0; i < (int)a.size(); ++i)
            if (a[i] == b[i]) return false;
    return true;
}

bool show_gizmos = true;

void update()
{
    // Update mouse pos in world
    auto cam_matrix = Matrix::Create2DTranslationZoom(OScreenf, cam_pos, ZOOM_LEVELS[cam_zoom]);
    auto inv_cam_matrix = cam_matrix.Invert();
    mouse_pos = Vector2::Transform(OGetMousePos(), inv_cam_matrix);

    mouse_over_room = nullptr;

    if (OInputJustPressed(OKeyTab)) show_gizmos = !show_gizmos;
    
    switch (state)
    {
        case state_t::idle:
        {
            if (!ImGui::GetIO().WantCaptureMouse)
            {
                if (OInputJustPressed(OMouse3) || OInputJustPressed(OKeySpaceBar))
                {
                    state = state_t::panning;
                    mouse_pos_on_down = OGetMousePos();
                    cam_pos_on_down = cam_pos;
                }
                else if (oInput->getStateValue(OMouseZ) > 0.0f)
                {
                    cam_zoom = onut::min(ZOOM_LEVEL_COUNT - 1, cam_zoom + 1);
                }
                else if (oInput->getStateValue(OMouseZ) < 0.0f)
                {
                    cam_zoom = onut::max(0, cam_zoom - 1);
                }
                else if (tool == tool_t::rooms)
                {
                    mouse_over_room = get_room_at(mouse_pos);
                    if (OInputJustPressed(OMouse1))
                    {
                        selection_on_down = data.selected_rooms;
                        if (mouse_over_room)
                        {
                            if (OInputPressed(OKeyLeftShift) || OInputPressed(OKeyLeftControl))
                            {
                                auto it = std::find(data.selected_rooms.begin(), data.selected_rooms.end(), mouse_over_room);
                                if (it != data.selected_rooms.end())
                                    data.selected_rooms.erase(it);
                                else
                                    data.selected_rooms.push_back(mouse_over_room);
                            }
                            else
                            {
                                if (std::find(data.selected_rooms.begin(), data.selected_rooms.end(), mouse_over_room) == data.selected_rooms.end())
                                    data.selected_rooms = { mouse_over_room };
                            }

                            if (data.selected_rooms.empty())
                            {
                                history.push_undo(data);
                            }
                            else
                            {
                                state = state_t::move;
                                mouse_pos_on_down = OGetMousePos();
                                rooms_pos_on_down.resize(data.selected_rooms.size());
                                for (int i = 0; i < (int)data.selected_rooms.size(); ++i)
                                    rooms_pos_on_down[i] = data.room_positions[data.selected_rooms[i]->id].pos;
                            }
                        }
                        else
                        {
                            state = state_t::selection_box;
                            selected_box_start = mouse_pos;
                        }
                    }
                }
            }

            if (!ImGui::GetIO().WantCaptureKeyboard)
            {
                update_shortcuts();
            }
            break;
        }
        case state_t::panning:
        {
            ImGui::GetIO().WantCaptureMouse = false;
            ImGui::GetIO().WantCaptureKeyboard = false;
            auto diff = OGetMousePos() - mouse_pos_on_down;
            cam_pos = cam_pos_on_down - diff / ZOOM_LEVELS[cam_zoom];
            if (OInputJustReleased(OMouse3) || OInputJustReleased(OKeySpaceBar))
                state = state_t::idle;
            break;
        }
        case state_t::move:
        {
            auto diff = (OGetMousePos() - mouse_pos_on_down) / ZOOM_LEVELS[cam_zoom];
            for (int i = 0; i < (int)data.selected_rooms.size(); ++i)
            {
                auto room_id = data.selected_rooms[i]->id;
                data.room_positions[room_id].pos.x = rooms_pos_on_down[i].x + (int)(diff.x / 16.0f);
                data.room_positions[room_id].pos.y = rooms_pos_on_down[i].y + (int)(diff.y / 16.0f);
            }
            if (OInputJustReleased(OMouse1))
            {
                if (selection_on_down != data.selected_rooms)
                    history.push_undo(data);
                history.push_undo(data);
                state = state_t::idle;
            }
            break;
        }
        case state_t::selection_box:
        {
            Rect rect(onut::min(mouse_pos.x, selected_box_start.x),
                      onut::min(mouse_pos.y, selected_box_start.y), 0, 0);
            rect.z = onut::max(mouse_pos.x, selected_box_start.x) - rect.x;
            rect.w = onut::max(mouse_pos.y, selected_box_start.y) - rect.y;

            data.selected_rooms.clear();
            for (auto& room : cart.rooms)
            {
                float room_x = (float)(data.room_positions[room.id].pos.x * 16);
                float room_y = (float)(data.room_positions[room.id].pos.y * 16);

                for (auto screen_id : room.screens)
                {
                    auto screen = &cart.chunks[room.chunk_id].screens[screen_id];
                    Rect screen_rect((float)screen->x_offset_in_room * SCREEN_BLOCKS_WIDTH * 16.0f + room_x,
                                     (float)screen->y_offset_in_room * SCREEN_BLOCKS_HEIGHT * 16.0f + room_y,
                                     SCREEN_BLOCKS_WIDTH * 16.0f, SCREEN_BLOCKS_HEIGHT * 16.0f);
                    if (rect.Overlaps(screen_rect))
                    {
                        data.selected_rooms.push_back(&room);
                        break;
                    }
                }
            }

            if (OInputPressed(OKeyLeftShift) || OInputPressed(OKeyLeftControl))
            {
                // Merge with previous selection
                std::vector<cart_room_t*> merged = data.selected_rooms;
                for (auto room_on_down : selection_on_down)
                    if (std::find(data.selected_rooms.begin(), data.selected_rooms.end(), room_on_down) == data.selected_rooms.end())
                        data.selected_rooms.push_back(room_on_down);
            }

            if (OInputJustReleased(OMouse1))
            {
                if (selection_on_down != data.selected_rooms)
                    history.push_undo(data);
                state = state_t::idle;
            }
            break;
        }
    }
}


static Matrix get_view_transform()
{
    return Matrix::Create2DTranslationZoom(OScreenf, cam_pos, ZOOM_LEVELS[cam_zoom]);
}


Color get_pal_color(int pal, int sub, float alpha = 1.0f)
{
    auto pal_col = cart.palettes[pal].palette[sub];
    return Color(
        (float)COLORS_RAW[pal_col * 3 + 0] / 255.0f,
        (float)COLORS_RAW[pal_col * 3 + 1] / 255.0f,
        (float)COLORS_RAW[pal_col * 3 + 2] / 255.0f,
        alpha);
}


static void draw_screen(const cart_screen_t* screen, float offset_x, float offset_y)
{
    auto sb = oSpriteBatch.get();

    Vector2 pos((float)screen->x_offset_in_room * (float)SCREEN_BLOCKS_WIDTH * 16 + offset_x,
                (float)screen->y_offset_in_room * (float)SCREEN_BLOCKS_HEIGHT * 16 + offset_y);

    sb->drawSprite(screen->texture, pos, Color::White, 0.0f, 1.0f, OTopLeft);

    if (ZOOM_LEVELS[cam_zoom] >= 1.0f && show_gizmos)
    {
        sb->drawRect(nullptr, Rect(pos, 14, 12), Color::Black);
        sb->drawText(font, std::to_string(screen->id), {pos.x + 1, pos.y + 1});
    }
}


static void draw_room(const cart_room_t* room)
{
    if (room == mouse_over_room && show_gizmos) return; // Will be drawn on top later

    auto sb = oSpriteBatch.get();
    float room_x = (float)(data.room_positions[room->id].pos.x * 16);
    float room_y = (float)(data.room_positions[room->id].pos.y * 16);

    // Outline
    if (std::find(data.selected_rooms.begin(), data.selected_rooms.end(), room) != data.selected_rooms.end())
    {
        Color selected_color = get_pal_color(1, 15);
        float outline_size = 2.0f / ZOOM_LEVELS[cam_zoom];
        for (const auto screen_id : room->screens)
        {
            auto screen = &cart.chunks[room->chunk_id].screens[screen_id];

            sb->drawRect(nullptr,
                         Rect((float)screen->x_offset_in_room * (float)SCREEN_BLOCKS_WIDTH * 16.0f + room_x - outline_size,
                              (float)screen->y_offset_in_room * (float)SCREEN_BLOCKS_HEIGHT * 16.0f + room_y - outline_size,
                              (float)SCREEN_BLOCKS_WIDTH * 16.0f + outline_size * 2.0f,
                              (float)SCREEN_BLOCKS_HEIGHT * 16.0f + outline_size * 2.0f),
                         selected_color);
        }
    }

    // Draw screens
    for (const auto screen_id : room->screens)
    {
        auto screen = &cart.chunks[room->chunk_id].screens[screen_id];
        draw_screen(screen, room_x, room_y);
    }
}


static void draw_hover_room(const cart_room_t* room)
{
    if (!room) return;

    auto sb = oSpriteBatch.get();
    sb->begin(get_view_transform());
    
    float room_x = (float)(data.room_positions[room->id].pos.x * 16);
    float room_y = (float)(data.room_positions[room->id].pos.y * 16);
    float w = (float)SCREEN_BLOCKS_WIDTH * 16.0f;
    float h = (float)SCREEN_BLOCKS_HEIGHT * 16.0f;

    // Outline
    Color hover_color = get_pal_color(1, 11);
    float outline_size = 4.0f / ZOOM_LEVELS[cam_zoom];
    for (const auto screen_id : room->screens)
    {
        auto screen = &cart.chunks[room->chunk_id].screens[screen_id];

        sb->drawRect(nullptr,
                     Rect((float)screen->x_offset_in_room * (float)SCREEN_BLOCKS_WIDTH * 16.0f + room_x - outline_size,
                          (float)screen->y_offset_in_room * (float)SCREEN_BLOCKS_HEIGHT * 16.0f + room_y - outline_size,
                          (float)SCREEN_BLOCKS_WIDTH * 16.0f + outline_size * 2.0f,
                          (float)SCREEN_BLOCKS_HEIGHT * 16.0f + outline_size * 2.0f),
                     hover_color);
    }

    // Draw screens
    for (const auto screen_id : room->screens)
    {
        auto screen = &cart.chunks[room->chunk_id].screens[screen_id];
        draw_screen(screen, room_x, room_y);
    }

    sb->end();

    // Draw outlines around screens
    auto pb = oPrimitiveBatch.get();
    pb->begin(OPrimitiveLineList, nullptr, get_view_transform());
    for (const auto screen_id : room->screens)
    {
        auto screen = &cart.chunks[room->chunk_id].screens[screen_id];

        Vector2 pos((float)screen->x_offset_in_room * (float)(SCREEN_BLOCKS_WIDTH * 16) + room_x,
                    (float)screen->y_offset_in_room * (float)(SCREEN_BLOCKS_HEIGHT * 16) + room_y);

        pb->draw({pos.x, pos.y}, Color::Black); pb->draw({pos.x, pos.y + h}, Color::Black);
        pb->draw({pos.x, pos.y + h}, Color::Black); pb->draw({pos.x + w, pos.y + h}, Color::Black);
        pb->draw({pos.x + w, pos.y + h}, Color::Black); pb->draw({pos.x + w, pos.y}, Color::Black);
        pb->draw({pos.x + w, pos.y}, Color::Black); pb->draw({pos.x, pos.y}, Color::Black);
    }
    pb->end();
}


static void draw_rooms()
{
    auto sb = oSpriteBatch.get();

    sb->begin(get_view_transform());
    for (const auto& room : cart.rooms)
    {
        draw_room(&room);
    }
    sb->end();
}


static void draw_arrow(const Vector2& from, const Vector2& to, const Color& color)
{
#if SHOW_GIZMOS
    auto pb = oPrimitiveBatch.get();

    pb->draw(from, color);
    pb->draw(to, color);

    Vector2 dir = to - from;
    dir.Normalize();
    Vector2 right(-dir.y, dir.x);

    const float head_size = 8.0f / ZOOM_LEVELS[cam_zoom];
    pb->draw(to, color); pb->draw(to - dir * head_size - right * head_size, color);
    pb->draw(to, color); pb->draw(to - dir * head_size + right * head_size, color);
#endif
}


static Vector2 get_world_position_centered(const cart_screen_t* screen, int x, int y)
{
    if (screen->room_id == -1) return Vector2(0, 0);

    auto room = &cart.rooms[screen->room_id];
    
    float room_x = (float)(data.room_positions[room->id].pos.x * 16);
    float room_y = (float)(data.room_positions[room->id].pos.y * 16);

    return Vector2((float)screen->x_offset_in_room * (float)SCREEN_BLOCKS_WIDTH * 16 + room_x + (float)x * 16.0f + 8.0f,
                   (float)screen->y_offset_in_room * (float)SCREEN_BLOCKS_HEIGHT * 16 + room_y + (float)y * 16.0f + 8.0f);
}


static Vector2 get_world_position(const cart_screen_t* screen, int x, int y)
{
    if (screen->room_id == -1) return Vector2(0, 0);

    auto room = &cart.rooms[screen->room_id];
    
    float room_x = (float)(data.room_positions[room->id].pos.x * 16);
    float room_y = (float)(data.room_positions[room->id].pos.y * 16);

    return Vector2((float)screen->x_offset_in_room * (float)SCREEN_BLOCKS_WIDTH * 16 + room_x + (float)x * 16.0f,
                   (float)screen->y_offset_in_room * (float)SCREEN_BLOCKS_HEIGHT * 16 + room_y + (float)y * 16.0f);
}


static void draw_line_rect(const Rect& rect, const Color& color)
{
    auto pb = oPrimitiveBatch.get();

    pb->draw(rect.TopLeft(), color); pb->draw(rect.BottomLeft(), color);
    pb->draw(rect.BottomLeft(), color); pb->draw(rect.BottomRight(), color);
    pb->draw(rect.BottomRight(), color); pb->draw(rect.TopRight(), color);
    pb->draw(rect.TopRight(), color); pb->draw(rect.TopLeft(), color);
}


static bool can_draw_tooltips()
{
    return !ImGui::GetIO().WantCaptureMouse && state == state_t::idle;
}


static void draw_doors()
{
#if SHOW_GIZMOS
    auto pb = oPrimitiveBatch.get();

    // Doors
    Color door_color = get_pal_color(2, 11);
    Color pathway_color = get_pal_color(8, 15);
    ImVec4 shop_text_color(0, 1, 1, 1);
    ImVec4 key_text_color(1, 0.4f, 0.4f, 1);

    pb->begin(OPrimitiveLineList, nullptr, get_view_transform());
    for (const auto& chunk : cart.chunks)
    {
        for (const auto& screen : chunk.screens)
        {
            if (screen.room_id == -1) continue; // Not on screen

            for (const auto& door : screen.doors)
            {
                auto pos = get_world_position(&screen, door.x, door.y);
                Rect rect(pos, 16, 32);
                draw_line_rect(rect, door_color);

                if (rect.Contains(mouse_pos) && can_draw_tooltips())
                {
                    ImGui::BeginTooltip();
                    ImGui::CollapsingHeader("DOOR", ImGuiTreeNodeFlags_DefaultOpen);

                    if (door.dest_chunk_id == CHUNK_SHOPS)
                    {
                        switch (door.dest_shop_id)
                        {
                            case 0:
                                ImGui::TextColored(shop_text_color, "King");
                                break;
                            case 1:
                                ImGui::TextColored(shop_text_color, "Guru");
                                break;
                            case 2:
                                ImGui::TextColored(shop_text_color, "Doctor");
                                break;
                            case 3:
                                ImGui::TextColored(shop_text_color, "Tavern");
                                break;
                            case 4:
                                ImGui::TextColored(shop_text_color, "Tools");
                                break;
                            case 5:
                                ImGui::TextColored(shop_text_color, "Keys");
                                break;
                            case 6:
                                ImGui::TextColored(shop_text_color, "House");
                                break;
                            case 7:
                                ImGui::TextColored(shop_text_color, "Meat");
                                break;
                            case 8:
                                ImGui::TextColored(shop_text_color, "Martial Arts");
                                break;
                            case 9:
                                ImGui::TextColored(shop_text_color, "Magic");
                                break;
                            default:
                                ImGui::TextColored(shop_text_color, "Unknown shop");
                                break;
                        }
                    }

                    if (door.key != -1)
                    {
                        switch (door.key)
                        {
                            case KEY_ACE:
                                ImGui::TextColored(key_text_color, "Ace Key");
                                break;
                            case KEY_KING:
                                ImGui::TextColored(key_text_color, "King Key");
                                break;
                            case KEY_QUEEN:
                                ImGui::TextColored(key_text_color, "Queen Key");
                                break;
                            case KEY_JACK:
                                ImGui::TextColored(key_text_color, "Jack Key");
                                break;
                            case KEY_JOKEY:
                                ImGui::TextColored(key_text_color, "Joker Key");
                                break;
                            case RING_OF_DWARF:
                                ImGui::TextColored(key_text_color, "Ring of Dwarf");
                                break;
                            case RING_OF_DEMON:
                                ImGui::TextColored(key_text_color, "Ring of Demon");
                                break;
                            case RING_OF_ELF:
                                ImGui::TextColored(key_text_color, "Ring of Elf");
                                break;
                            default:
                                ImGui::TextColored(key_text_color, "Unknown key");
                                break;
                        }
                    }
                    
                    ImGui::Spacing();
                    ImGui::Text("Raw location Data:");
                    ImGui::Bullet(); ImGui::TextColored(door.raw_location.screen_id == 0xFF ? ImVec4(1, 1, 1, .35f) : ImVec4(1, 1, 1, 1), "screen id = %i", (int)door.raw_location.screen_id);
                    ImGui::Bullet(); ImGui::TextColored(door.raw_location.x == 0xF ? ImVec4(1, 1, 1, .35f) : ImVec4(1, 1, 1, 1), "x = %i", (int)door.raw_location.x);
                    ImGui::Bullet(); ImGui::TextColored(door.raw_location.y == 0xF ? ImVec4(1, 1, 1, .35f) : ImVec4(1, 1, 1, 1), "y = %i", (int)door.raw_location.y);
                    ImGui::Bullet(); ImGui::TextColored(door.raw_location.index == 0xFF ? ImVec4(1, 1, 1, .35f) : ImVec4(1, 1, 1, 1), "index = %i", (int)door.raw_location.index);
                    ImGui::Bullet(); ImGui::TextColored(door.raw_location.dest_x == 0xF ? ImVec4(1, 1, 1, .35f) : ImVec4(1, 1, 1, 1), "dest x = %i", (int)door.raw_location.dest_x);
                    ImGui::Bullet(); ImGui::TextColored(door.raw_location.dest_y == 0xF ? ImVec4(1, 1, 1, .35f) : ImVec4(1, 1, 1, 1), "dest y = %i", (int)door.raw_location.dest_y);
                    ImGui::Spacing();
                    ImGui::Text("Raw destination data:");
                    ImGui::Bullet(); ImGui::TextColored(door.raw_destination.screen_id == 0xFF ? ImVec4(1, 1, 1, .35f) : ImVec4(1, 1, 1, 1), "screen id = %i", (int)door.raw_destination.screen_id);
                    ImGui::Bullet(); ImGui::TextColored(door.raw_destination.palette == 0xFF ? ImVec4(1, 1, 1, .35f) : ImVec4(1, 1, 1, 1), "palette = %i", (int)door.raw_destination.palette);
                    ImGui::Bullet(); ImGui::TextColored(door.raw_destination.shop_screen_id == 0xFF ? ImVec4(1, 1, 1, .35f) : ImVec4(1, 1, 1, 1), "shop = %i", (int)door.raw_destination.shop_screen_id);
                    ImGui::Bullet(); ImGui::TextColored(door.raw_destination.key == 0xFF ? ImVec4(1, 1, 1, .35f) : ImVec4(1, 1, 1, 1), "key = %i", (int)door.raw_destination.key);
                    ImGui::Bullet(); ImGui::TextColored(door.raw_destination.padding0 == 0xFF ? ImVec4(1, 1, 1, .35f) : ImVec4(1, 1, 1, 1), "unkonwn = %i", (int)door.raw_destination.padding0);

                    ImGui::EndTooltip();
                }
            }
        }
    }
    pb->end();
#endif
}


static void draw_dialog(const std::vector<cart_dialog_command_t>& commands);

static void draw_entities()
{
    auto sb = oSpriteBatch.get();
    auto pb = oPrimitiveBatch.get();

    // Sprite
    sb->begin(get_view_transform());
    for (const auto& chunk : cart.chunks)
    {
        for (const auto& screen : chunk.screens)
        {
            if (screen.room_id == -1) continue; // Not on screen

            for (const auto& entity : screen.entities)
            {
                const auto& entity_type = cart.entity_types[entity.type];
                if (entity_type.frames.empty()) continue;
                auto pos = get_world_position(&screen, entity.x, entity.y);
                pos.x += (float)entity_type.frames[0].offset_x;
                pos.y += (float)entity_type.frames[0].offset_y;
                sb->drawSprite(entity_type.frames[0].texture, pos, Color::White, 0.0f, 1.0f, OTopLeft);
            }
        }
    }
    sb->end();

    // Frame
#if SHOW_GIZMOS
    Color entity_color = get_pal_color(3, 11);
    pb->begin(OPrimitiveLineList, nullptr, get_view_transform());
    for (const auto& chunk : cart.chunks)
    {
        for (const auto& screen : chunk.screens)
        {
            if (screen.room_id == -1) continue; // Not on screen

            for (const auto& entity : screen.entities)
            {
                auto pos = get_world_position(&screen, entity.x, entity.y);
                const auto& entity_type = cart.entity_types[entity.type];
                Rect rect(pos, (float)entity_type.width * 8, (float)entity_type.height * 8);
                draw_line_rect(rect, entity_color);

                //if (entity.dialog_id == 0x79) __debugbreak();
                if (rect.Contains(mouse_pos) && can_draw_tooltips())
                {
                    ImGui::BeginTooltip();
                    ImGui::CollapsingHeader("ENTITY", ImGuiTreeNodeFlags_DefaultOpen);
                    auto name = get_entity_name(entity.type);
                    ImGui::Text("%s", name ? name : "UNKNOWN");
                    ImGui::Text("Addr: 0x%08X", entity.addr);
                    ImGui::Text("Type: 0x%02X", entity.type);

                    int ap_location_count = (int)(sizeof(AP_LOCATIONS) / sizeof(ap_location_t));
                    static const int CHUNK_TO_ALL_LEVELS_MAP[] = { 0, 2, 3, 1, 5, 6, 4, 7 };
                    for (int i = 0; i < ap_location_count; ++i)
                    {
                        const auto& ap_location = AP_LOCATIONS[i];
                        if (ap_location.world == CHUNK_TO_ALL_LEVELS_MAP[chunk.id] &&
                            ap_location.screen == screen.id &&
                            ap_location.addr == entity.addr)
                        {
                            ImGui::Text("AP ID: %" PRId64, ap_location.id);
                            break;
                        }
                    }
                    
                    ImGui::TextColored(entity.dialog_id == -1 ? ImVec4(1, 1, 1, .35f) : ImVec4(1, 1, 1, 1), "Dialog Id: 0x%02X", entity.dialog_id);
                    if (entity.dialog_id != -1)
                    {
                        ImGui::Separator();
                        ImGui::Text("Portrait: 0x%02X", cart.dialogs[entity.dialog_id].portrait);
                        draw_dialog(cart.dialogs[entity.dialog_id].commands);
                    }
                    ImGui::EndTooltip();
                }
            }
        }
    }
    pb->end();
#endif
}


static void draw_connections()
{
    auto pb = oPrimitiveBatch.get();

    // Doors
    Color door_color = get_pal_color(2, 11);
    Color pathway_color = get_pal_color(8, 15);

    pb->begin(OPrimitiveLineList, nullptr, get_view_transform());
    for (const auto& chunk : cart.chunks)
    {
        for (const auto& screen : chunk.screens)
        {
            if (screen.room_id == -1) continue; // Not on screen

            for (const auto& door : screen.doors)
            {
                auto from = get_world_position_centered(&screen, door.x, door.y);
                auto to = get_world_position_centered(&cart.chunks[door.dest_chunk_id].screens[door.dest_screen_id], door.dest_x, door.dest_y);
                draw_arrow(from, to, door_color);
            }

            for (const auto& pathway : screen.pathways)
            {
                auto from = get_world_position_centered(&screen, SCREEN_BLOCKS_WIDTH / 2, SCREEN_BLOCKS_HEIGHT / 2);
                auto to = get_world_position_centered(&cart.chunks[pathway.dest_chunk_id].screens[pathway.dest_screen_id], pathway.dest_x, pathway.dest_y);
                draw_arrow(from, to, pathway_color);
            }
        }
    }

    pb->end();
}


void draw_selection_box()
{
    if (state != state_t::selection_box) return;

    auto sb = oSpriteBatch.get();
    
    Color box_color = get_pal_color(0, 15);
    Rect rect(onut::min(mouse_pos.x, selected_box_start.x),
              onut::min(mouse_pos.y, selected_box_start.y), 0, 0);
    rect.z = onut::max(mouse_pos.x, selected_box_start.x) - rect.x;
    rect.w = onut::max(mouse_pos.y, selected_box_start.y) - rect.y;

    sb->begin(get_view_transform());
    sb->drawInnerOutlineRect(rect, 2.0f / ZOOM_LEVELS[cam_zoom], box_color);
    sb->end();
}


void render()
{
    oRenderer->clear(Color::Black);
    oRenderer->renderStates.sampleFiltering = OFilterNearest;

    draw_rooms();
    if (show_gizmos)
    {
        draw_hover_room(mouse_over_room);
        draw_doors();
        draw_entities();
        draw_connections();
        draw_selection_box();
    }
}


void postRender()
{
}


static void draw_dialog(const std::vector<cart_dialog_command_t>& commands)
{
    ImGui::Indent();
    for (const auto& cmd : commands)
    {
        ImGui::Text("0x%08X", cmd.addr); ImGui::SameLine();
        switch (cmd.command)
        {
            case cart_dialog_commands_t::SHOW_DIALOG_1:
            case cart_dialog_commands_t::SHOW_DIALOG_2:
            case cart_dialog_commands_t::SHOW_DIALOG_3:
                ImGui::Text("%s", cmd.string.c_str());
                break;
            case cart_dialog_commands_t::JUMP_IF_HAS_ANY_MONEY:
                ImGui::Text("Has any money?");
                draw_dialog(cmd.branch_commands);
                break;
            case cart_dialog_commands_t::JUMP_IF_HAS_MONEY:
                ImGui::Text("Has %i golds?", cmd.golds);
                draw_dialog(cmd.branch_commands);
                break;
            case cart_dialog_commands_t::JUMP_IF_HAS_TITLE:
                ImGui::Text("Has the \"%s\" title?", get_title_name(cmd.title));
                draw_dialog(cmd.branch_commands);
                break;
            case cart_dialog_commands_t::JUMP_IF_HAS_ITEM:
                ImGui::Text("Has %s?", get_item_name(cmd.item));
                draw_dialog(cmd.branch_commands);
                break;
            case cart_dialog_commands_t::JUMP_IF_CAN_LEVEL_UP:
                ImGui::Text("Can level up?");
                draw_dialog(cmd.branch_commands);
                break;
            case cart_dialog_commands_t::JUMP_IF_FOUNTAIN:
                ImGui::Text("Is fountain %i flowing?", cmd.fountain);
                draw_dialog(cmd.branch_commands);
                break;
            case cart_dialog_commands_t::SET_GURU:
                ImGui::Text("Set guru %i", cmd.guru);
                break;
            case cart_dialog_commands_t::MEDITATE:
                ImGui::Text("Meditate");
                break;
            case cart_dialog_commands_t::TAKE_ITEM:
                ImGui::Text("Take %s", get_item_name(cmd.item));
                break;
            case cart_dialog_commands_t::GIVE_ITEM:
                ImGui::Text("Give %s 0x%02X", get_item_name(cmd.item), (int)cmd.item);
                break;
            case cart_dialog_commands_t::GIVE_MONEY:
                ImGui::Text("Give %i", cmd.golds);
                break;
            case cart_dialog_commands_t::GIVE_MAGIC:
                ImGui::Text("Give %i magic", cmd.magic);
                break;
            case cart_dialog_commands_t::GIVE_HEALTH:
                ImGui::Text("Give %i health", cmd.health);
                break;
            case cart_dialog_commands_t::FLOW_FOUNTAIN:
                ImGui::Text("Flow fountain %i", cmd.fountain);
                break;
            case cart_dialog_commands_t::ASK_BUY_OR_SELL:
                ImGui::Text("Want to buy?");
                draw_dialog(cmd.branch_commands);
                break;
            case cart_dialog_commands_t::SHOW_BUY:
                ImGui::Text("Buy:");
                ImGui::Indent();
                for (const auto& shop_item : cmd.shop_items)
                    ImGui::Text("0x%08X %s (0x%02X)   %i", shop_item.addr, get_item_name(shop_item.item), shop_item.item, shop_item.price);
                ImGui::Unindent();
                break;
            case cart_dialog_commands_t::SHOW_SELL:
                ImGui::Text("Sell:");
                ImGui::Indent();
                for (const auto& shop_item : cmd.shop_items)
                    ImGui::Text("%s   %i", get_item_name(shop_item.item), shop_item.price);
                ImGui::Unindent();
                break;
        }
    }
    ImGui::Unindent();
}


void renderUI()
{
    if (!show_gizmos) return;

    ImGui::BeginMainMenuBar();
    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Save")) save_data();
        ImGui::Separator();
        if (ImGui::MenuItem("Generate"))
        {
            save_data();
            //generate();
        }
        if (ImGui::MenuItem("Exit")) OQuit();
        ImGui::EndMenu();
    }
    ImGui::Text("Zoom: %f", ZOOM_LEVELS[cam_zoom]);
    ImGui::EndMainMenuBar();

    if (ImGui::Begin("Strings"))
    {
        int id = 1;
        for (const auto& cart_string : cart.strings)
        {
            ImGui::Text("(%02X) 0x%08X", id, cart_string.address);
            ImGui::SameLine();
            ImGui::BeginGroup();
            for (const auto& text_block : cart_string.text_blocks)
            {
                ImGui::Text("%s", text_block.c_str());
                ImGui::Separator();
            }
            ImGui::EndGroup();
            ++id;
        }
        ImGui::Separator();
        id = 0;
        for (const auto& string : cart.titles)
        {
            ImGui::Text("(%02X) %s", id, string.c_str());
            ++id;
        }
        ImGui::Separator();
        id = 0;
        for (const auto& string : cart.item_names)
        {
            ImGui::Text("(%02X) %s", id, string.c_str());
            ++id;
        }
    }
    ImGui::End();

    if (ImGui::Begin("Dialogs"))
    {
        int id = 0;
        char buf[260];
        for (const auto& dialog : cart.dialogs)
        {
            snprintf(buf, 260, "Dialog (0x%08X) 0x%02X", dialog.addr, id);
            if (ImGui::CollapsingHeader(buf))
            {
                ImGui::Text("Portrait: 0x%02X", dialog.portrait);
                draw_dialog(dialog.commands);

                // Show raw code
                ImGui::Separator();
                for (auto code : dialog.raw_codes)
                {
                    ImGui::Text("(0x%04X)   0x%02X", code.addr, code.code);
                }
            }
            ++id;
        }
    }
    ImGui::End();

    if (ImGui::Begin("Palettes"))
    {
        int i = 0;
        for (const auto& palette : cart.palettes)
        {
            ImGui::Text("%2i", i); ImGui::SameLine();
            ImVec2 pos = ImGui::GetCursorScreenPos();
            for (int i = 0; i < 16; ++i)
            {
                ImGui::GetWindowDrawList()->AddRectFilled({pos.x + i * 34, pos.y}, {pos.x + i * 34 + 32, pos.y + 20}, IM_COL32(
                    COLORS_RAW[palette.palette[i] * 3 + 0],
                    COLORS_RAW[palette.palette[i] * 3 + 1],
                    COLORS_RAW[palette.palette[i] * 3 + 2],
                    255));
                auto pos_before = ImGui::GetCursorScreenPos();
                ImGui::SetCursorScreenPos(ImVec2(pos.x + i * 34, pos.y));
                ImGui::Text("%02X", palette.palette[i]);
                ImGui::SetCursorScreenPos(pos_before);
            }
            ImGui::Dummy(ImVec2(200, 24));
            ++i;
        }
    }
    ImGui::End();

    if (ImGui::Begin("Tilesets"))
    {
        int timeset_id = 0;
        for (const auto& tileset : cart.tilesets)
        {
            if (ImGui::CollapsingHeader(("Tileset " + std::to_string(timeset_id)).c_str(), ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("Offset: %i", tileset.offset);
                int col = 0;
                int id = 0;
                int columns = 16;//tileset.columns > 0 ? tileset.columns : 16;
                for (const auto& tile_data : tileset.tiles_data)
                {
                    if (col != 0 && col < columns) ImGui::SameLine();
                    auto pos_before = ImGui::GetCursorScreenPos();
                    ImGui::Image((ImTextureID)&tile_data.texture, {
#if SHOW_SMALL_TILES
                        8, 8
#else
                        32, 32
#endif
                    });
                    auto pos_after = ImGui::GetCursorScreenPos();
                    ImGui::SetCursorScreenPos(pos_before);
#if !SHOW_SMALL_TILES
                    ImGui::Text("%02X", id++);
#endif
                    ImGui::SetCursorScreenPos(pos_before);
#if SHOW_SMALL_TILES
                    ImGui::Dummy({8, 8});
#else
                    ImGui::Dummy({32, 32});
#endif
                    ++col;
                    if (col == columns) col = 0;
                }
            }
            ++timeset_id;
        }
    }
    ImGui::End();

    if (ImGui::Begin("Portrait Tilesets"))
    {
        int timeset_id = 0;
        for (const auto& tileset : cart.portrait_tilesets)
        {
            if (ImGui::CollapsingHeader(("Portrait tileset " + std::to_string(timeset_id)).c_str(), ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("Offset: %i", tileset.offset);
                int col = 0;
                int columns = 16;//tileset.columns > 0 ? tileset.columns : 16;
                for (const auto& tile_data : tileset.tiles_data)
                {
                    if (col != 0 && col < columns) ImGui::SameLine();
                    ImGui::Image((ImTextureID)&tile_data.texture, {
#if SHOW_SMALL_TILES
                        8, 8
#else
                        32, 32
#endif
                    });
                    ++col;
                    if (col == columns) col = 0;
                }
            }
            ++timeset_id;
        }
    }
    ImGui::End();

    if (ImGui::Begin("Chunks"))
    {
        int i = 0;
        for (const auto& chunk : cart.chunks)
        {
            if (ImGui::CollapsingHeader(("Chunk " + std::to_string(i)).c_str(), ImGuiTreeNodeFlags_DefaultOpen))
            {
                std::set<cart_tileset_t*> tilesets;
                for (const auto& screen : chunk.screens)
                {
                    if (screen.tileset == nullptr) continue;
                    tilesets.insert(screen.tileset);
                }
                int block_count = 0;
                for (auto tileset : tilesets)
                    block_count += (int)chunk.blocks.size();

                if (ImGui::TreeNode(("Tileset (" + std::to_string(block_count) + ")##chunk_tileset" + std::to_string(i)).c_str()))
                {
                    for (auto tileset : tilesets)
                    {
                        int col = 0;
                        auto draw_list = ImGui::GetWindowDrawList();
                        for (const auto& block : chunk.blocks)
                        {
                            if (col != 0 && col < 16) ImGui::SameLine();
                    
                            ImVec2 pos = ImGui::GetCursorScreenPos();
                            int offset = tileset->offset;
                            if (block.top_left - offset >= 0 && block.top_left - offset < (int)tileset->tiles_data.size() &&
                                block.top_right - offset >= 0 && block.top_right - offset < (int)tileset->tiles_data.size() &&
                                block.bottom_left - offset >= 0 && block.bottom_left - offset < (int)tileset->tiles_data.size() &&
                                block.bottom_right - offset >= 0 && block.bottom_right - offset < (int)tileset->tiles_data.size())
                            {
                                draw_list->AddImage((ImTextureID)&tileset->tiles_data[block.top_left - offset].texture, {pos.x, pos.y}, {pos.x + 16, pos.y + 16});
                                draw_list->AddImage((ImTextureID)&tileset->tiles_data[block.top_right - offset].texture, {pos.x + 16, pos.y}, {pos.x + 32, pos.y + 16});
                                draw_list->AddImage((ImTextureID)&tileset->tiles_data[block.bottom_left - offset].texture, {pos.x, pos.y + 16}, {pos.x + 16, pos.y + 32});
                                draw_list->AddImage((ImTextureID)&tileset->tiles_data[block.bottom_right - offset].texture, {pos.x + 16, pos.y + 16}, {pos.x + 32, pos.y + 32});
                            }
                            ImGui::Dummy({33, 33});
                            if (ImGui::IsItemHovered())
                            {
                                ImGui::BeginTooltip();
                                ImGui::Text("Properties: 0x%02X", block.properties);
                                ImGui::EndTooltip();
                            }

                            ++col;
                            if (col == 16) col = 0;
                        }
                    }
                    ImGui::TreePop();
                }

                if (ImGui::TreeNode(("Screen (" + std::to_string(chunk.screens.size()) + ")##chunk_screens" + std::to_string(i)).c_str()))
                {
                    int j = 0;
                    int col_count = (int)(ImGui::GetWindowWidth() - 20) / (SCREEN_BLOCKS_WIDTH * 16 + 20);
                    ImGui::Columns(col_count);
                    for (const auto& screen : chunk.screens)
                    {
                        if (screen.id == -1) continue;

                        ImVec2 pos_before = ImGui::GetCursorScreenPos();
                        ImGui::Image((ImTextureID)&screen.texture, {SCREEN_BLOCKS_WIDTH * 16.0f, SCREEN_BLOCKS_HEIGHT * 16.0f});
                        ImVec2 pos_after = ImGui::GetCursorScreenPos();
                        ImGui::SetCursorScreenPos(pos_before);
                        ImGui::GetWindowDrawList()->AddRectFilled({pos_before.x, pos_before.y}, {pos_before.x + 16.0f, pos_before.y + 16.0f}, IM_COL32(0, 0, 0, 255));
                        ImGui::Text("%i", j);
                        ImGui::SetCursorScreenPos(pos_after);
                        ImGui::NextColumn();
                        ++j;
                    }
                    ImGui::Columns(1);
                    ImGui::TreePop();
                }
            }
            ++i;
        }
    }
    ImGui::End();

    if (ImGui::Begin("Sprites"))
    {
        int timeset_id = 0;
        for (const auto& spriteset : cart.spritesets)
        {
            char buf[260];
            snprintf(buf, 260, "Spriteset %02X", timeset_id);
            if (ImGui::CollapsingHeader(buf, ImGuiTreeNodeFlags_DefaultOpen))
            {
                int col = 0;
                int columns = 16;//spriteset.width > 0 ? spriteset.width : 16;
                int id = spriteset.offset;
                ImGui::Text("Addr: 0x%08X", spriteset.addr);
                for (const auto& tile_data : spriteset.tiles_data)
                {
                    if (col != 0 && col < columns) ImGui::SameLine();
                    auto pos_before = ImGui::GetCursorScreenPos();
                    ImGui::Image((ImTextureID)&tile_data.texture, {
#if SHOW_SMALL_TILES
                        8, 8
#else
                        32, 32
#endif
                    });
                    auto pos_after = ImGui::GetCursorScreenPos();
                    ImGui::SetCursorScreenPos(pos_before);
#if !SHOW_SMALL_TILES
                    ImGui::Text("%02X", id++);
#endif
                    ImGui::SetCursorScreenPos(pos_before);
                    ImGui::Dummy({
#if SHOW_SMALL_TILES
                        8, 8
#else
                        32, 32
#endif
                    });
                    ++col;
                    if (col == columns) col = 0;
                }
            }
            ++timeset_id;
        }
    }
    ImGui::End();

    if (ImGui::Begin("Music"))
    {
        bool use_filter = apu_player->use_filter;
        if (ImGui::Checkbox("Use filter", &use_filter))
        {
            apu_player->use_filter = use_filter;
        }
        for (const auto& music : cart.musics)
        {
            if (apu_player->playing_music() == music.id)
            {
                if (ImGui::Button(("Stop##music_stop_btn" + std::to_string(music.id)).c_str()))
                {
                    apu_player->stop();
                }
            }
            else if (ImGui::Button(("Play##music_play_btn" + std::to_string(music.id)).c_str()))
            {
                apu_player->play(music.id);
            }
            ImGui::SameLine();
            ImGui::Text("(%02X) %s", music.id, music.name.c_str());
            ImGui::Separator();
        }
    }
    ImGui::End();

    if (ImGui::Begin("Sound"))
    {
        bool use_filter = apu_player->use_filter;
        for (const auto& sound : cart.sounds)
        {
            if (ImGui::Button(("Play##snd_play_btn" + std::to_string(sound.id)).c_str()))
            {
                sound.sound->play();
            }
            ImGui::SameLine();
            ImGui::Text("(%02X) %s", sound.id, sound.name.c_str());
            ImGui::Separator();
        }
    }
    ImGui::End();

    if (ImGui::Begin("Entities"))
    {
        int entity_id = 0;
        for (const auto& entity_type : cart.entity_types)
        {
            auto name = get_entity_name(entity_id);
            auto name_str = name ? name : "UNKNOWN";
            char id_hex[8];
            snprintf(id_hex, 8, "0x%02X", entity_id);
            if (ImGui::CollapsingHeader((name_str + std::string(" ") + id_hex).c_str(), name ? ImGuiTreeNodeFlags_DefaultOpen : 0))
            {
                auto draw_list = ImGui::GetWindowDrawList();
                for (const auto& frame : entity_type.frames)
                {
                    auto size = frame.texture->getSizef();
                    auto cur = ImGui::GetCursorPos();

                    Rect entity_rect(0.0f, 0.0f, (float)entity_type.width * 8.0f * 4.0f, (float)entity_type.height * 8.0f * 4.0f);
                    Rect frame_rect((float)frame.offset_x * 4.0f, (float)frame.offset_y * 4.0f, (float)frame.w * 8.0f * 4.0f, (float)frame.h * 8.0f * 4.0f);
                    Rect global_rect = entity_rect.Merge(frame_rect);

                    if (frame.offset_x < 0)
                    {
                        entity_rect.x -= (float)frame.offset_x * 4.0f;
                        frame_rect.x -= (float)frame.offset_x * 4.0f;
                    }
                    if (frame.offset_y < 0)
                    {
                        entity_rect.y -= (float)frame.offset_y * 4.0f;
                        frame_rect.y -= (float)frame.offset_y * 4.0f;
                    }

                    auto screen_pos = ImGui::GetCursorScreenPos();
                    auto w = ImGui::GetWindowWidth();
                    if (cur.x + global_rect.z >= w)
                    {
                        ImGui::NewLine();
                        screen_pos = ImGui::GetCursorScreenPos();
                    }

                    draw_list->AddImage((ImTextureID)&frame.texture, 
                                        ImVec2(screen_pos.x + frame_rect.x, screen_pos.y + frame_rect.y),
                                        ImVec2(screen_pos.x + frame_rect.x + frame_rect.z, screen_pos.y + frame_rect.y + frame_rect.w));

                    draw_list->AddRect(ImVec2(screen_pos.x + entity_rect.x, screen_pos.y + entity_rect.y),
                                       ImVec2(screen_pos.x + entity_rect.x + entity_rect.z, screen_pos.y + entity_rect.y + entity_rect.w),
                                       ImGui::ColorConvertFloat4ToU32({1, 1, 0, 1}));

                    ImGui::Dummy({global_rect.z, global_rect.w});
                    ImGui::SameLine();
                }
                ImGui::NewLine();
            }
            ++entity_id;
        }
    }
    ImGui::End();

    if (ImGui::Begin("Items"))
    {
        for (const auto& item_type : cart.item_types)
        {
            ImGui::Image((ImTextureID)&item_type.texture, {48, 48});
            ImGui::SameLine();
            ImGui::Text("(%02X) %s", item_type.id, item_type.name.c_str());
        }
    }
    ImGui::End();

    if (ImGui::Begin("Animations"))
    {
        //for (const auto& item_type : cart.item_types)
        //{
        //}
    }
    ImGui::End();
}
