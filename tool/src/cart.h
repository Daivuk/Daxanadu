#pragma once

#include <onut/ForwardDeclaration.h>

#include <map>
#include <string>
#include <vector>


OForwardDeclare(Sound);
OForwardDeclare(Texture);
struct cart_room_t;


static const int SCREEN_BLOCKS_WIDTH = 16;
static const int SCREEN_BLOCKS_HEIGHT = 13;
static const int FIRST_SCREEN_PALETTE = 0;
static const int GURU_PALETTE = 18;
static const int ENTITY_PALETTE = 28;

// Chunks
static const int CHUNK_EOLIS = 0;
static const int CHUNK_MISTY = 1;
static const int CHUNK_TOWNS = 2;
static const int CHUNK_TRUNK = 3;
static const int CHUNK_BRANCHES = 4;
static const int CHUNK_DARTMOOR_CASTLE = 5;
static const int CHUNK_SHOPS = 6;
static const int CHUNK_EVIL_FORTRESS = 7;

// Door requirement
static const int KEY_ACE = 1;
static const int KEY_KING = 2;
static const int KEY_QUEEN = 3;
static const int KEY_JACK = 4;
static const int KEY_JOKEY = 5;
static const int RING_OF_ELF = 6;
static const int RING_OF_DWARF = 7;
static const int RING_OF_DEMON = 8;


static const char* MUSIC_NAMES[] = {
    "Title music",
    "Death music",
    "Eolis music",
    "Cathedral music",
    "King music",
    "Shops music",
    "Trunk music",
    "Towns music",
    "Towers music",
    "Boss music",
    "Mist music",
    "House Glass music",
    "Branches music",
    "Dartmoor Castle music",
    "Evil Fortress music",
    "Ending music",

    "Typing sfx", // 10
    "Enemy hit sfx", // 11
    "Enemy kill sfx", // 12
    "Player hit sfx", // 13
    "Magic sfx", // 14
    "Door sfx", // 15
    "unknown sfx", // 16
    "Coin sfx", // 17
    "Coin sfx", // 18
    "unknown", // 19
    "Menu sfx", // 1A
    "unknown", // 1B
    "Cannot equip sfx", // 1C
    "unknown", // 1D
    "Push sfx", // 1E
    "Coin sfx", // 1F
    "unknown", // 20
    "unknown", // 21
    "Health fill finish sfx", // 22
    "unknown", // 23
    "Typing sfx", // 24
    "Die sfx", // 25
    "unknown", // 26
    "Menu enter sfx", // 27
    "unknown", // 28
    "unknown", // 29
    "Bread pickup sfx", // 2A
    "Coin sfx" // AB
};


// Items
enum item_t : int
{
    ITEM_INVALID = -1,

    ITEM_DAGGER = 0, // 2B4B0
    ITEM_LONG_SWORD = 1, // 2B4B4
    ITEM_GIANT_BLADE = 2, // 2B4B8
    ITEM_DRAGON_SLAYER = 3, // 2B4BC

    ITEM_LEATHER_ARMOR = 32, // 2B4C0
    ITEM_STUDDED_MAIL = 33, // 2B4C4
    ITEM_FULL_PLATE = 34, // 2B4C8
    ITEM_BATTLE_SUIT = 35, // 2B4CC

    ITEM_SMALL_SHIELD = 64, // 2B4D0
    ITEM_LARGE_SHIELD = 65, // 2B4D4
    ITEM_MAGIC_SHIELD = 66, // 2B4D8
    ITEM_BATTLE_HELMET = 67, // 2B4DC

    ITEM_DELUGE = 96, // 2B4E0
    ITEM_THUNDER = 97, // 2B4E4
    ITEM_FIRE = 98, // 2B4E8
    ITEM_DEATH = 99, // 2B4EC
    ITEM_TILTE = 100, // 2B4F0

    ITEM_ELF_RING = 128, // 2B4F4
    ITEM_RUBY_RING = 129, // 2B4F8
    ITEM_DWORF_RING = 130, // 2B4FC
    ITEM_DEMON_RING = 131, // 2B500
    ITEM_ACE_KEY = 132, // 2B504
    ITEM_KING_KEY = 133, // 2B508
    ITEM_QUEEN_KEY = 134, // 2B50C
    ITEM_JACK_KEY = 135, // 2B510
    ITEM_JOKER_LEY = 136, // 2B514
    ITEM_MATTOCK = 137, // 2B518
    ITEM_ROD = 138, // 2B51C
    ITEM_CRYSTAL = 139, // 2B520
    ITEM_LAMP = 140, // 2B524
    ITEM_HOURGLASS = 141, // 2B5218
    ITEM_BOOK = 142, // 2B52C
    ITEM_WINGBOOTS = 143, // 2B530
    ITEM_RED_POTION = 144, // 2B534
    ITEM_BLACK_POTION = 145, // 2B538
    ITEM_ELIXIR = 146, // 2B53C
    ITEM_PENDANT = 147, // 2B540
    ITEM_BLACK_ONYX = 148, // 2B544
    ITEM_FIRE_CRYSTAL = 149 // 2B548
};

// Entities
static const int ENTITY_BREAD = 1;
static const int ENTITY_COIN = 2;
static const int ENTITY_RAIDEN = 4;
static const int ENTITY_NECRON_AIDES = 5;
static const int ENTITY_ZOMBIE = 6;
static const int ENTITY_HORNET = 7;
static const int ENTITY_BIHORUDA = 8;
static const int ENTITY_LILITH = 9;
static const int ENTITY_YAINURA = 11;
static const int ENTITY_SNOWMAN = 12;
static const int ENTITY_NASH = 13;
static const int ENTITY_FIRE_GIANT = 14;
static const int ENTITY_ISHIISU = 15;
static const int ENTITY_EXECUTION_HOOD = 16;
static const int ENTITY_ROKUSUTAHN = 17;
static const int ENTITY_ROCK_SNAKE = 18; // UNUSED
static const int ENTITY_MONSTER_DEATH_FX = 19;
static const int ENTITY_MONSTER_DEATH_FX_2 = 20; // Duplicate
static const int ENTITY_CHARRON = 21;
static const int ENTITY_GERIBUTA = 23;
static const int ENTITY_SUGATA = 24;
static const int ENTITY_GRIMLOCK = 25;
static const int ENTITY_GIANT_BEES = 26;
static const int ENTITY_MYCONID = 27;
static const int ENTITY_NAGA = 28;
static const int ENTITY_STILL_NIGHT = 29; // UNUSED
static const int ENTITY_GIANT_STRIDER = 30;
static const int ENTITY_SIR_GAWAINE = 31;
static const int ENTITY_MASK_MAN = 32;
static const int ENTITY_WOLF_MAN = 33;
static const int ENTITY_YAREEKA = 34;
static const int ENTITY_MAGMAN = 35;
static const int ENTITY_JOUSTER = 36; // UNUSED
static const int ENTITY_IKEDA = 38;
static const int ENTITY_SLUG = 39; // UNUSED
static const int ENTITY_LAMPREY = 40;
static const int ENTITY_MONODRON = 42;
static const int ENTITY_BAT = 43; // UNUSED
static const int ENTITY_TAMAZUTSU = 44;
static const int ENTITY_RIPASHEIKU = 45;
static const int ENTITY_ZORADOHNA = 46;
static const int ENTITY_BORABOHRA = 47;
static const int ENTITY_PAKUKAME = 48;
static const int ENTITY_ZORUGERIRU = 49;
static const int ENTITY_KING_GRIEVE = 50;
static const int ENTITY_SHADOW_EURA = 51;
static const int ENTITY_BANDANA_TOWN_PERSON = 52;
static const int ENTITY_RING_OF_DEMON = 53;
static const int ENTITY_RING_OF_DWARF = 54;
static const int ENTITY_BLACKSMITH = 55;
static const int ENTITY_MARTIAL_ARTS_TRAINER = 56;
static const int ENTITY_GURU = 57;
static const int ENTITY_KING = 58;
static const int ENTITY_MAGE = 59;
static const int ENTITY_KEY_SALESMAN = 60;
static const int ENTITY_SMOKER_TOWN_PERSON = 61;
static const int ENTITY_DOCTOR = 62;
static const int ENTITY_GRUMPY_TOWN_PERSON = 63;
static const int ENTITY_BUTCHER = 64;
static const int ENTITY_NICE_LADY_TOWN_PERSON = 65;
static const int ENTITY_GUARD = 66;
static const int ENTITY_SITTING_TOWN_PERSON = 67;
static const int ENTITY_NURSE = 68;
static const int ENTITY_LADY_TOWN_PERSON = 69;
static const int ENTITY_EYE = 70; // UNUSED
static const int ENTITY_ZOZURA = 71;
static const int ENTITY_GLOVE = 72;
static const int ENTITY_BLACK_ONYX = 73;
static const int ENTITY_PENDANT = 74;
static const int ENTITY_RED_POTION = 75;
static const int ENTITY_POISON = 76;
static const int ENTITY_ELIXIR = 77;
static const int ENTITY_OINTMENT = 78;
static const int ENTITY_INTRO_TRIGGER = 79;
static const int ENTITY_MATTOCK = 80;
static const int ENTITY_STAR = 81;
static const int ENTITY_ROCK_FOUNTAIN = 82;
static const int ENTITY_IMPACT = 83;
static const int ENTITY_MAGICS = 84;
static const int ENTITY_WING_BOOTS = 85;
static const int ENTITY_HOURGLASS = 86;
static const int ENTITY_MAGICAL_ROD = 87;
static const int ENTITY_BATTLE_SUIT = 88;
static const int ENTITY_BATTLE_HELMET = 89;
static const int ENTITY_DRAGON_SLAYER = 90;
static const int ENTITY_MATTOCK_BOSS_REWARD = 91;
static const int ENTITY_WING_BOOTS_BOSS_REWARD = 92;
static const int ENTITY_RED_POTION2 = 93;
static const int ENTITY_POISON2 = 94;
static const int ENTITY_GLOVE2 = 95;
static const int ENTITY_OINTMENT2 = 96;
static const int ENTITY_TRUNK_FOUNTAIN = 97;
static const int ENTITY_SKY_FOUNTAIN = 98;
static const int ENTITY_HIDDEN_FOUNTAIN = 99;
static const int ENTITY_MONSTER_DEATH_FX_3 = 100; // Duplicate


struct cart_string_t
{
    int address = -1;
    int len = -1;
    std::vector<std::string> text_blocks; // Each element in the array is marked by a pause
};


struct cart_tile_data_t
{
    uint8_t data[16] = {0};
    uint8_t cooked[8 * 8 * 4] = {0};
    OTextureRef texture;
};


// A tileset is a collection of 8x8 tiles
struct cart_tileset_t
{
    int offset = -1;
    int addr = 0;
    std::vector<cart_tile_data_t> tiles_data;
};


struct cart_palette_t
{
    uint8_t palette[16] = {0};
};


// Block, or TSA (Tile Square Assembly)
// Commonly called "metatiles". It's a 2x2 tile, resulting in a 16x16 pixels block
struct cart_block_t
{
    int top_left = 0;
    int top_right = 0;
    int bottom_left = 0;
    int bottom_right = 0;
    int attrib_tl = -1; // This is the only relevant one
    int attrib_tr = -1; // unused
    int attrib_bl = -1; // unused
    int attrib_br = -1; // unused
    uint8_t properties;
    OTextureRef texture;
};


struct cart_entity_t
{
    int type = -1;
    int x = -1, y = -1;
    int dialog_id = -1;
    int addr = -1;
};


struct cart_sprite_t
{
    int sprite_id  = -1;
    bool flip_h = false;
    bool flip_v = false;
    int offset_x = -1;
    int offset_y = -1;
    uint8_t* palette = nullptr;
};


struct cart_frame_t
{
    int offset_x = 0;
    int offset_y = 0;
    int w = 0;
    int h = 0;
    std::vector<cart_sprite_t> sprites;
    OTextureRef texture;
};


struct cart_entity_type_t
{
    int width = 0;
    int height = 0;
    int spritesheet_id = -1;
    std::vector<uint8_t> raw_frames;
    std::vector<cart_frame_t> frames;
};


struct cart_shop_item_t
{
    item_t item = item_t::ITEM_INVALID;
    int price = -1;
    int addr;
};


struct cart_shop_content_t
{
    std::vector<cart_shop_item_t> items;
};


struct cart_door_location_t
{
    uint8_t screen_id = 0xFF;
    uint8_t x : 4;
    uint8_t y : 4;
    uint8_t index = 0xFF;
    uint8_t dest_x : 4;
    uint8_t dest_y : 4;
};


struct cart_door_destination_t
{
    uint8_t screen_id = 0xFF;
    union
    {
        uint8_t palette = 0xFF;
        uint8_t shop_screen_id;
    };
    uint8_t key = 0xFF;
    uint8_t padding0 = 0xFF;
};


struct cart_door_t
{
    int dest_screen_id = -1;
    int dest_shop_id = -1;
    int x = -1;
    int y = -1;
    int dest_chunk_id = -1;
    int dest_x = -1;
    int dest_y = -1;
    uint8_t* dest_palette = nullptr;
    cart_door_location_t raw_location;
    cart_door_destination_t raw_destination;
    int dest_room_id = -1;
    int key = -1;
    cart_shop_content_t shop_content; // If this leads to a shop
};


// Like a door, but triggered by screen edges
struct cart_pathway_t
{
    int dest_chunk_id = -1;
    int dest_screen_id = -1;
    int dest_x = -1;
    int dest_y = -1;
    uint8_t* dest_palette = nullptr;
    uint8_t raw_data1[5] = {0};
    uint8_t raw_data2[4] = {0};
    int dest_room_id = -1;
};


struct cart_scroll_t
{
    uint8_t left = 0xFF;
    uint8_t right = 0xFF;
    uint8_t top = 0xFF;
    uint8_t bottom = 0xFF;
};


struct cart_screen_t
{
    int id = -1;
    std::vector<uint8_t> blocks;
    std::vector<cart_entity_t> entities;
    cart_tileset_t* tileset = nullptr;
    OTextureRef texture;
    uint8_t* palette = nullptr;
    cart_scroll_t scroll;
    std::vector<cart_door_t> doors;
    std::vector<cart_pathway_t> pathways;
    int x_offset_in_room = 0;
    int y_offset_in_room = 0;
    int room_id = -1;
};


struct cart_chunk_t
{
    int id = -1;
    std::vector<cart_screen_t> screens;
    std::vector<cart_block_t> blocks;
};


struct cart_room_t
{
    int id = -1;
    int chunk_id = -1;
    std::vector<int> screens;
};


enum class cart_dialog_commands_t : int
{
    INVALID = 0,
    SHOW_DIALOG_1,
    SHOW_DIALOG_2,
    SHOW_DIALOG_3,
    JUMP_IF_HAS_ANY_MONEY,
    JUMP_IF_HAS_MONEY,
    JUMP_IF_HAS_ITEM,
    JUMP_IF_HAS_TITLE,
    JUMP_IF_CAN_LEVEL_UP,
    JUMP_IF_FOUNTAIN,
    SET_GURU,
    MEDITATE,
    TAKE_ITEM,
    GIVE_ITEM,
    GIVE_MONEY,
    GIVE_MAGIC,
    GIVE_HEALTH,
    ASK_BUY_OR_SELL,
    SHOW_BUY,
    SHOW_SELL,
    FLOW_FOUNTAIN
};


struct cart_dialog_command_t
{
    cart_dialog_commands_t command = cart_dialog_commands_t::INVALID;
    union
    {
        item_t item;
        int golds;
        int magic;
        int health;
        int title;
        int guru;
        int fountain;
        struct
        {
            int dialog_type;
            int text_block;
        };
    };
    int addr = 0;
    std::string string; // For debug purpose
    std::vector<cart_dialog_command_t> branch_commands;
    std::vector<cart_shop_item_t> shop_items;
};


struct cart_dialog_raw_code_t
{
    int addr = -1;
    int code = -1;
};


struct cart_dialog_t
{
    int addr = -1;
    int portrait = 0;
    std::vector<cart_dialog_command_t> commands;
    std::vector<cart_dialog_raw_code_t> raw_codes;
};


struct cart_chunk_link_t
{
    int previous_chunk = -1;
    int previous_screen = -1;
    uint8_t* previous_palette = nullptr;

    int next_chunk = -1;
    int next_screen = -1;
    uint8_t* next_palette = nullptr;
    int next_chunk_key_requirement = -1;
};


struct cart_music_t
{
    int id = -1;
    std::string name;
};


struct cart_sound_t
{
    int id = -1;
    OSoundRef sound;
    std::string name;
};


struct cart_item_type_t
{
    int id = -1;
    uint8_t tiles_raw[4] = {0xff, 0xff, 0xff, 0xff};
    std::string name;
    OTextureRef texture;
};


struct cart_t
{
    cart_chunk_link_t chunk_link_table[8];
    int chunk_to_bank_table[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
    uint8_t shop_tilesets_table[10] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    uint8_t chunk_tilesets_table[8] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    uint8_t chunks_palette_table[8] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    std::vector<cart_tileset_t> tilesets;
    std::vector<cart_tileset_t> spritesets;
    std::vector<cart_tileset_t> portrait_tilesets;
    std::vector<cart_string_t> strings;
    std::vector<std::string> titles;
    std::vector<std::string> item_names;
    std::vector<cart_palette_t> palettes;
    std::vector<cart_chunk_t> chunks;
    std::vector<cart_room_t> rooms;
    std::vector<cart_dialog_t> dialogs;
    std::vector<cart_screen_t> shop_screens;
    std::vector<cart_door_t> shop_doors;
    std::vector<uint8_t> rom; // Raw bytes from the rom
    std::vector<cart_music_t> musics;
    std::vector<cart_sound_t> sounds;
    std::vector<cart_entity_type_t> entity_types;
    std::vector<cart_item_type_t> item_types;
};


extern cart_t cart;
extern std::vector<uint8_t> COLORS_RAW;


void init_cart();
