#pragma once

#include <string>
#include <vector>


// Those IDs are outside of normal range. The code is altered to jump to other tables
#define AP_ITEM_PROGRESSIVE_SWORD 0x96
#define AP_ITEM_PROGRESSIVE_ARMOR 0x97
#define AP_ITEM_PROGRESSIVE_SHIELD 0x98
#define AP_ITEM_POISON 0x99
#define AP_ITEM_OINTMENT 0x9A
#define AP_ITEM_GLOVE 0x9B
#define AP_ITEM_SPRING_ELIXIR 0x9C

// Start the new entities at 0x80. This way we can just test for N flag.
#define AP_ENTITY_RING_OF_ELF 0x80
#define AP_ENTITY_RING_OF_RUBY 0x81
#define AP_ENTITY_RING_OF_DWORF 0x82
#define AP_ENTITY_DEMONS_RING 0x83
#define AP_ENTITY_KEY_JACK 0x84
#define AP_ENTITY_KEY_QUEEN 0x85
#define AP_ENTITY_KEY_KING 0x86
#define AP_ENTITY_KEY_ACE 0x87
#define AP_ENTITY_KEY_JOKER 0x88
#define AP_ENTITY_DELUGE 0x89
#define AP_ENTITY_THUNDER 0x8A
#define AP_ENTITY_FIRE 0x8B
#define AP_ENTITY_DEATH 0x8C
#define AP_ENTITY_TILTE 0x8D
#define AP_ENTITY_SPRING_ELIXIR 0x8E
#define AP_ENTITY_PROGRESSIVE_SWORD 0x8F
#define AP_ENTITY_PROGRESSIVE_ARMOR 0x90
#define AP_ENTITY_PROGRESSIVE_SHIELD 0x91
#define AP_ENTITY_RED_POTION 0x92
#define AP_ENTITY_ELIXIR 0x93
#define AP_ENTITY_POISON 0x94
#define AP_ENTITY_OINTMENT 0x95
#define AP_ENTITY_GLOVE 0x96
#define AP_ENTITY_AP 0x97
#define AP_ENTITY_AP_PROGRESSION 0x98

#define EXTRA_ITEMS_COUNT 25


struct ap_item_t
{
    int64_t id = -1;
    std::string name;
    int item_id = -1; // If placed in shops
    int entity_id = -1; // If placed in the world
    int hidden_entity_id = -1; // Placed in the work in an item room
    int boss_entity_id = -1; // Reward from boss kill
};


static ap_item_t AP_ITEMS[] = {
    { 400000, "Progressive Sword", AP_ITEM_PROGRESSIVE_SWORD, AP_ENTITY_PROGRESSIVE_SWORD, AP_ENTITY_PROGRESSIVE_SWORD + 32, AP_ENTITY_PROGRESSIVE_SWORD + 64 },
    { 400001, "Progressive Armor", AP_ITEM_PROGRESSIVE_ARMOR, AP_ENTITY_PROGRESSIVE_SHIELD, AP_ENTITY_PROGRESSIVE_SHIELD + 32, AP_ENTITY_PROGRESSIVE_SHIELD + 64 },
    { 400002, "Progressive Shield", AP_ITEM_PROGRESSIVE_SHIELD, AP_ENTITY_PROGRESSIVE_SHIELD, AP_ENTITY_PROGRESSIVE_SHIELD + 32, AP_ENTITY_PROGRESSIVE_SHIELD + 64 },
    { 400003, "Spring Elixir", AP_ITEM_SPRING_ELIXIR, AP_ENTITY_SPRING_ELIXIR, AP_ENTITY_SPRING_ELIXIR + 32, AP_ENTITY_SPRING_ELIXIR + 64 },
    { 400004, "Mattock", 0x89, 0x50, 0x50, 0x50 },
    { 400005, "Wingboots", 0x8F, 0x55, 0x55, 0x55 },
    { 400006, "Key Jack", 0x87, AP_ENTITY_KEY_JACK, AP_ENTITY_KEY_JACK + 32, AP_ENTITY_KEY_JACK + 64 },
    { 400007, "Key Queen", 0x86, AP_ENTITY_KEY_QUEEN, AP_ENTITY_KEY_QUEEN + 32, AP_ENTITY_KEY_QUEEN + 64 },
    { 400008, "Key King", 0x85, AP_ENTITY_KEY_KING, AP_ENTITY_KEY_KING + 32, AP_ENTITY_KEY_KING + 64 },
    { 400009, "Key Joker", 0x88, AP_ENTITY_KEY_JOKER, AP_ENTITY_KEY_JOKER + 32, AP_ENTITY_KEY_JOKER + 64 },
    { 400010, "Key Ace", 0x84, AP_ENTITY_KEY_ACE, AP_ENTITY_KEY_ACE + 32, AP_ENTITY_KEY_ACE + 64 },
    { 400011, "Ring of Ruby", 0x81, AP_ENTITY_RING_OF_RUBY, AP_ENTITY_RING_OF_RUBY + 32, AP_ENTITY_RING_OF_RUBY + 64 },
    { 400012, "Ring of Dworf", 0x82, AP_ENTITY_RING_OF_DWORF, AP_ENTITY_RING_OF_DWORF + 32, AP_ENTITY_RING_OF_DWORF + 64 },
    { 400013, "Demons Ring", 0x83, AP_ENTITY_DEMONS_RING, AP_ENTITY_DEMONS_RING + 32, AP_ENTITY_DEMONS_RING + 64 },
    { 400014, "Black Onyx", 0x94, 0x49, 0x49, 0x49 },
    { 400015, "Sky Spring Flow", 0x00, 0xFF, 0xFF, 0xFF },
    { 400016, "Tower of Fortress Spring Flow", 0x00, 0xFF, 0xFF, 0xFF },
    { 400017, "Joker Spring Flow", 0x00, 0xFF, 0xFF, 0xFF },
    { 400018, "Deluge", 0x60, AP_ENTITY_DELUGE, AP_ENTITY_DELUGE + 32, AP_ENTITY_DELUGE + 64 },
    { 400019, "Thunder", 0x61, AP_ENTITY_THUNDER, AP_ENTITY_THUNDER + 32, AP_ENTITY_THUNDER + 64 },
    { 400020, "Fire", 0x62, AP_ENTITY_FIRE, AP_ENTITY_FIRE + 32, AP_ENTITY_FIRE + 64 },
    { 400021, "Death", 0x63, AP_ENTITY_DEATH, AP_ENTITY_DEATH + 32, AP_ENTITY_DEATH + 64 },
    { 400022, "Tilte", 0x64, AP_ENTITY_TILTE, AP_ENTITY_TILTE + 32, AP_ENTITY_TILTE + 64 },
    { 400023, "Ring of Elf", 0x80, AP_ENTITY_RING_OF_ELF, AP_ENTITY_RING_OF_ELF + 32, AP_ENTITY_RING_OF_ELF + 64 },
    { 400024, "Magical Rod", 0x8A, 0x57, 0x57, 0x57 },
    { 400025, "Pendant", 0x93, 0x4A, 0x4A, 0x4A },
    { 400026, "Hourglass", 0x8D, 0x56, 0x56, 0x56 },
    { 400027, "Red Potion", 0x90, 0x4B, 0x5D, 0x5B },
    { 400028, "Elixir", 0x92, 0x4D, 0x4D, 0x4D },
    { 400029, "Glove", AP_ITEM_GLOVE, 0x48, 0x48, 0x48 },
    { 400030, "Ointment", AP_ITEM_OINTMENT, 0x4E, 0x4E, 0x4E },
    { 400031, "Poison", AP_ITEM_POISON, 0x4C, 0x5E, 0x4C },
    { 400032, "Killed Evil One", 0x00, 0xFF, 0xFF, 0xFF },
};
