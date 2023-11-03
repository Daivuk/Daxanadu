#pragma once

#include <string>
#include <vector>


#define AP_ITEM_PROGRESSIVE_SWORD 0x96
#define AP_ITEM_PROGRESSIVE_ARMOR 0x97
#define AP_ITEM_PROGRESSIVE_SHIELD 0x98
#define AP_ITEM_POISON 0x99
#define AP_ITEM_OINTMENT 0x9A
#define AP_ITEM_GLOVE 0x9B
#define AP_ITEM_SPRING_ELIXIR 0x9C


enum class ap_item_type_t
{
    unknown,
    spring,
    boss_kill,
    inventory,
    world
};


struct ap_item_t
{
    int64_t id = -1;
    std::string name;
    ap_item_type_t type = ap_item_type_t::unknown;
    int item_id = -1; // If placed in shops
    int entity_id = -1; // If placed in the world
    int hidden_entity_id = -1; // Placed in the work in an item room
    int boss_entity_id = -1; // Reward from boss kill
};


static ap_item_t AP_ITEMS[] = {
    { 400000, "Progressive Sword", ap_item_type_t::inventory, AP_ITEM_PROGRESSIVE_SWORD, 0x5A },
    { 400001, "Progressive Armor", ap_item_type_t::inventory, AP_ITEM_PROGRESSIVE_ARMOR, 0x58 },
    { 400002, "Progressive Shield", ap_item_type_t::inventory, AP_ITEM_PROGRESSIVE_SHIELD, 0x59 },
    { 400003, "Spring Elixir", ap_item_type_t::inventory, AP_ITEM_SPRING_ELIXIR, 0x4D },
    { 400004, "Mattock", ap_item_type_t::inventory, 0x89, 0x50 },
    { 400005, "Wingboots", ap_item_type_t::inventory, 0x8F, 0x55 },
    { 400006, "Key Jack", ap_item_type_t::inventory, 0x87, 0x00 },
    { 400007, "Key Queen", ap_item_type_t::inventory, 0x86, 0x00 },
    { 400008, "Key King", ap_item_type_t::inventory, 0x85, 0x00 },
    { 400009, "Key Joker", ap_item_type_t::inventory, 0x88, 0x00 },
    { 400010, "Key Ace", ap_item_type_t::inventory, 0x84, 0x00 },
    { 400011, "Ring of Ruby", ap_item_type_t::inventory, 0x81, 0x00 },
    { 400012, "Ring of Dworf", ap_item_type_t::inventory, 0x82, 0x00 },
    { 400013, "Demons Ring", ap_item_type_t::inventory, 0x83, 0x00 },
    { 400014, "Black Onyx", ap_item_type_t::inventory, 0x94, 0x49 },
    { 400015, "Sky Spring Flow", ap_item_type_t::spring, 0x00, 0x00 },
    { 400016, "Tower of Fortress Spring Flow", ap_item_type_t::spring, 0x00, 0x00 },
    { 400017, "Joker Spring Flow", ap_item_type_t::spring, 0x00, 0x00 },
    { 400018, "Deluge", ap_item_type_t::inventory, 0x60, 0x00 },
    { 400019, "Thunder", ap_item_type_t::inventory, 0x61, 0x00 },
    { 400020, "Fire", ap_item_type_t::inventory, 0x62, 0x00 },
    { 400021, "Death", ap_item_type_t::inventory, 0x63, 0x00 },
    { 400022, "Tilte", ap_item_type_t::inventory, 0x64, 0x00 },
    { 400023, "Ring of Elf", ap_item_type_t::inventory, 0x80, 0x00 },
    { 400024, "Magical Rod", ap_item_type_t::inventory, 0x8A, 0x57 },
    { 400025, "Pendant", ap_item_type_t::inventory, 0x93, 0x4A },
    { 400026, "Hourglass", ap_item_type_t::inventory, 0x8D, 0x56 },
    { 400027, "Red Potion", ap_item_type_t::inventory, 0x90, 0x4B },
    { 400028, "Elixir", ap_item_type_t::inventory, 0x92, 0x4D },
    { 400029, "Glove", ap_item_type_t::world, AP_ITEM_GLOVE, 0x5F },
    { 400030, "Ointment", ap_item_type_t::world, AP_ITEM_OINTMENT, 0x4E },
    { 400031, "Poison", ap_item_type_t::world, AP_ITEM_POISON, 0x5E },
    { 400032, "Killed Evil One", ap_item_type_t::boss_kill, 0x00, 0x00 },
};
