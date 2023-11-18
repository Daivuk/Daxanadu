#pragma once

#include <string>
#include <vector>


enum class ap_location_type_t
{
    unknown,
    world, // Just standing there in the world
    hidden, // Kill all monsters in the room to reveal, each "item room" counter tick.
    boss_reward, // Kill a boss to reveal the item
    shop, // Buy at a shop
    give, // Given by an NPC
    spring, // Activatable spring
    boss, // Entity to kill to trigger the check
};


#define WORLD_EOLIS 0
#define WORLD_TRUNK 1
#define WORLD_MIST 2
#define WORLD_TOWN 3
#define WORLD_SHOPS 4
#define WORLD_BRANCHES 5
#define WORLD_DARTMOOR 6
#define WORLD_EVIL 7


struct ap_location_t
{
    int64_t id = -1;
    int world; // 0 = eolis
               // 1 = trunk
               // 2 = mist
               // 3 = town
               // 4 = shops
               // 5 = branches
               // 6 = dartmoor/fraternal castle
               // 7 = evil fortress
    int screen;
    std::string name;
    ap_location_type_t type = ap_location_type_t::unknown;
    int item_id = -1;
    int addr = -1;
    union
    {
        int giver_cond_addr = -1;
        int store_item_list_addr;
    };
    union
    {
        int shop_index = -1;
        int in_screen_index;
    };
};


static ap_location_t AP_LOCATIONS[] = {
    // Eolis
    { 400100, WORLD_SHOPS, 2, "Eolis Guru", ap_location_type_t::give, 0x80, 0x000320C4, 0x000320BE },
    { 400101, WORLD_SHOPS, 3, "Eolis Key Jack", ap_location_type_t::shop, 0x87, 0x0003257A, 0x00032508, 0 },
    { 400102, WORLD_SHOPS, 4, "Eolis Hand Dagger", ap_location_type_t::shop, 0x00, 0x0003242E, 0x00032386, 0 },
    { 400103, WORLD_SHOPS, 4, "Eolis Red Potion", ap_location_type_t::shop, 0x90, 0x00032431, 0x00032386, 1 },
    { 400104, WORLD_SHOPS, 4, "Eolis Elixir", ap_location_type_t::shop, 0x92, 0x00032434, 0x00032386, 2 },
    { 400105, WORLD_SHOPS, 4, "Eolis Deluge", ap_location_type_t::shop, 0x60, 0x00032437, 0x00032386, 3 },

    // Path to Apolune
    { 400106, WORLD_SHOPS, 20, "Path to Apolune Magic Shield", ap_location_type_t::shop, 0x42, 0x00032448, 0x000323AA, 0 },
    { 400107, WORLD_SHOPS, 20, "Path to Apolune Death", ap_location_type_t::shop, 0x63, 0x0003244B, 0x000323AA, 1 },

    // Apolune
    { 400108, WORLD_SHOPS, 8, "Apolune Small Shield", ap_location_type_t::shop, 0x40, 0x0003243B, 0x00032398, 0 },
    { 400109, WORLD_SHOPS, 8, "Apolune Hand Dagger", ap_location_type_t::shop, 0x00, 0x0003243E, 0x00032398, 1 },
    { 400110, WORLD_SHOPS, 8, "Apolune Deluge", ap_location_type_t::shop, 0x60, 0x00032441, 0x00032398, 2 },
    { 400111, WORLD_SHOPS, 8, "Apolune Red Potion", ap_location_type_t::shop, 0x90, 0x00032444, 0x00032398, 3 },
    { 400112, WORLD_SHOPS, 9, "Apolune Key Jack", ap_location_type_t::shop, 0x87, 0x0003257E, 0x0003251A, 0 },

    // Tower of Trunk
    { 400113, WORLD_TRUNK, 14, "Tower of Trunk Hidden Mattock", ap_location_type_t::hidden, 0x50, 0x0002C337 },
    { 400114, WORLD_TRUNK, 19, "Tower of Trunk Hidden Hourglass", ap_location_type_t::hidden, 0x56, 0x0002C355 },
    { 400115, WORLD_TRUNK, 21, "Tower of Trunk Boss Mattock", ap_location_type_t::boss_reward, 0x5B, 0x0002C367 },

    // Path to Forepaw
    { 400116, WORLD_TRUNK, 12, "Path to Forepaw Hidden Red Potion", ap_location_type_t::hidden, 0x5D, 0x0002C327 },
    { 400117, WORLD_TRUNK, 24, "Path to Forepaw Glove", ap_location_type_t::world, 0x48, 0x0002C381 },

    // Forepaw
    { 400118, WORLD_SHOPS, 15, "Forepaw Long Sword", ap_location_type_t::shop, 0x01, 0x0003244F, 0x000323BC, 0 },
    { 400119, WORLD_SHOPS, 15, "Forepaw Studded Mail", ap_location_type_t::shop, 0x21, 0x00032452, 0x000323BC, 1 },
    { 400120, WORLD_SHOPS, 15, "Forepaw Small Shield", ap_location_type_t::shop, 0x40, 0x00032455, 0x000323BC, 2 },
    { 400121, WORLD_SHOPS, 15, "Forepaw Red Potion", ap_location_type_t::shop, 0x90, 0x00032458, 0x000323BC, 3 },
    { 400122, WORLD_SHOPS, 15, "Forepaw Wingboots", ap_location_type_t::shop, 0x8F, 0x0003245B, 0x000323BC, 4 },
    { 400123, WORLD_SHOPS, 19, "Forepaw Key Jack", ap_location_type_t::shop, 0x87, 0x00032582, 0x0003252C, 0 },
    { 400124, WORLD_SHOPS, 19, "Forepaw Key Queen", ap_location_type_t::shop, 0x86, 0x00032585, 0x0003252C, 1 },

    // Trunk
    { 400125, WORLD_TRUNK, 29, "Trunk Hidden Ointment", ap_location_type_t::hidden, 0x60, 0x0002C397 },
    { 400126, WORLD_TRUNK, 33, "Trunk Hidden Red Potion", ap_location_type_t::hidden, 0x5D, 0x0002C3B3 },
    { 400127, WORLD_TRUNK, 34, "Trunk Red Potion", ap_location_type_t::world, 0x4B, 0x0002C3B9 },

    // Joker Spring
    { 400128, WORLD_TRUNK, 63, "Joker Spring Ruby Ring", ap_location_type_t::give, 0x81, 0x000321D4, 0x000321CE },

    // Tower of Fortress
    { 400129, WORLD_TRUNK, 41, "Tower of Fortress Poison 1", ap_location_type_t::world, 0x4C, 0x0002C3F6 },
    { 400130, WORLD_TRUNK, 53, "Tower of Fortress Poison 2", ap_location_type_t::world, 0x4C, 0x0002C442 },
    { 400131, WORLD_TRUNK, 45, "Tower of Fortress Hidden Wingboots", ap_location_type_t::world, 0x55, 0x0002C410 },
    { 400132, WORLD_TRUNK, 55, "Tower of Fortress Ointment", ap_location_type_t::world, 0x4E, 0x0002C450 },
    { 400133, WORLD_TRUNK, 59, "Tower of Fortress Boss Wingboots", ap_location_type_t::boss_reward, 0x5C, 0x0002C46A },
    { 400134, WORLD_TRUNK, 60, "Tower of Fortress Elixir", ap_location_type_t::world, 0x4D, 0x0002C472 },
    { 400135, WORLD_SHOPS, 21, "Tower of Fortress Guru", ap_location_type_t::give, 0x88, 0x00032199, 0x00032193 },

    // Path to Mascon
    { 400136, WORLD_MIST, 17, "Path to Mascon Hidden Wingboots", ap_location_type_t::hidden, 0x55, 0x0002C598 },

    // Tower of Red Potion
    { 400137, WORLD_MIST, 77, "Tower of Red Potion", ap_location_type_t::world, 0x4B, 0x0002C724 },

    // Mascon
    { 400138, WORLD_SHOPS, 24, "Mascon Large Shield", ap_location_type_t::shop, 0x41, 0x0003245F, 0x000323CE, 0 },
    { 400139, WORLD_SHOPS, 24, "Mascon Thunder", ap_location_type_t::shop, 0x61, 0x00032462, 0x000323CE, 1 },
    { 400140, WORLD_SHOPS, 24, "Mascon Mattock", ap_location_type_t::shop, 0x89, 0x00032465, 0x000323CE, 2 },
    { 400141, WORLD_SHOPS, 24, "Mascon Red Potion", ap_location_type_t::shop, 0x90, 0x00032468, 0x000323CE, 3 },
    { 400142, WORLD_SHOPS, 25, "Mascon Key Jack", ap_location_type_t::shop, 0x87, 0x00032589, 0x0003253E, 0 },
    { 400143, WORLD_SHOPS, 25, "Mascon Key Queen", ap_location_type_t::shop, 0x86, 0x0003258C, 0x0003253E, 1 },

    // Path to Victim
    { 400144, WORLD_SHOPS, 32, "Misty Shop Death", ap_location_type_t::shop, 0x63, 0x0003246C, 0x000323E0, 0 },
    { 400145, WORLD_SHOPS, 32, "Misty Shop Hourglass", ap_location_type_t::shop, 0x8D, 0x0003246F, 0x000323E0, 1 },
    { 400146, WORLD_SHOPS, 32, "Misty Shop Elixir", ap_location_type_t::shop, 0x92, 0x00032472, 0x000323E0, 2 },
    { 400147, WORLD_SHOPS, 32, "Misty Shop Red Potion", ap_location_type_t::shop, 0x90, 0x00032475, 0x000323E0, 3 },
    { 400148, WORLD_SHOPS, 31, "Misty Doctor Office", ap_location_type_t::hidden, 0x55, 0x0002CBB4 },

    // Tower of Suffer
    { 400149, WORLD_MIST, 50, "Tower of Suffer Hidden Wingboots", ap_location_type_t::hidden, 0x55, 0x0002C670 },
    { 400150, WORLD_MIST, 48, "Tower of Suffer Hidden Hourglass", ap_location_type_t::hidden, 0x56, 0x0002C660 },
    { 400151, WORLD_MIST, 52, "Tower of Suffer Pendant", ap_location_type_t::boss_reward, 0xA4, 0x0002C67E },

    // Victim
    { 400152, WORLD_SHOPS, 69, "Victim Full Plate", ap_location_type_t::shop, 0x22, 0x00032479, 0x000323F2, 0 },
    { 400153, WORLD_SHOPS, 69, "Victim Mattock", ap_location_type_t::shop, 0x89, 0x0003247C, 0x000323F2, 1 },
    { 400154, WORLD_SHOPS, 69, "Victim Red Potion", ap_location_type_t::shop, 0x90, 0x0003247F, 0x000323F2, 2 },
    { 400155, WORLD_SHOPS, 39, "Victim Key King", ap_location_type_t::shop, 0x85, 0x00032590, 0x00032550, 0 },
    { 400156, WORLD_SHOPS, 39, "Victim Key Queen", ap_location_type_t::shop, 0x86, 0x00032593, 0x00032550, 1 },
    { 400157, WORLD_SHOPS, 40, "Victim Tavern", ap_location_type_t::give, 0x22, 0x00032261, 0x0003225B },

    // Mist
    { 400158, WORLD_MIST, 31, "Mist Hidden Poison 1", ap_location_type_t::hidden, 0x5E, 0x0002C5FE },
    { 400159, WORLD_MIST, 30, "Mist Hidden Poison 2", ap_location_type_t::hidden, 0x5E, 0x0002C5F8 },
    { 400160, WORLD_MIST, 27, "Mist Hidden Wingboots", ap_location_type_t::hidden, 0x55, 0x0002C5E4 },
    { 400161, WORLD_SHOPS, 28, "Misty Magic Hall", ap_location_type_t::give, 0x62, 0x00032373, 0x0003236A },
    { 400162, WORLD_SHOPS, 34, "Misty House", ap_location_type_t::give, 0x84, 0x0003228B, 0x00032285 },

    // Useless Tower
    { 400163, WORLD_MIST, 80, "Useless Tower", ap_location_type_t::hidden, 0x5D, 0x0002C736 },

    // Tower of Mist
    { 400164, WORLD_MIST, 71, "Tower of Mist Hidden Ointment", ap_location_type_t::hidden, 0x60, 0x0002C6FE },
    { 400165, WORLD_MIST, 65, "Tower of Mist Elixir", ap_location_type_t::world, 0x4D, 0x0002C6D0 },
    { 400166, WORLD_MIST, 76, "Tower of Mist Black Onyx", ap_location_type_t::boss_reward, 0x49, 0x0002C71C },

    // Path to Conflate
    { 400167, WORLD_BRANCHES, 4, "Path to Conflate Hidden Ointment", ap_location_type_t::hidden, 0x60, 0x0002C7B0 },
    { 400168, WORLD_BRANCHES, 11, "Path to Conflate Poison", ap_location_type_t::hidden, 0x4C, 0x0002C7DE },

    // Helm Branch
    { 400169, WORLD_BRANCHES, 8, "Helm Branch Hidden Glove", ap_location_type_t::hidden, 0x5F, 0x0002C7CA },
    { 400170, WORLD_BRANCHES, 7, "Helm Branch Battle Helmet", ap_location_type_t::boss_reward, 0x59, 0x0002C7C2 },

    // Conflate
    { 400171, WORLD_SHOPS, 45, "Conflate Giant Blade", ap_location_type_t::shop, 0x02, 0x00032483, 0x00032404, 0 },
    { 400172, WORLD_SHOPS, 45, "Conflate Magic Shield", ap_location_type_t::shop, 0x42, 0x00032486, 0x00032404, 1 },
    { 400173, WORLD_SHOPS, 45, "Conflate Wingboots", ap_location_type_t::shop, 0x8F, 0x00032489, 0x00032404, 2 },
    { 400174, WORLD_SHOPS, 45, "Conflate Red Potion", ap_location_type_t::shop, 0x90, 0x0003248C, 0x00032404, 3 },
    { 400175, WORLD_SHOPS, 43, "Conflate Guru", ap_location_type_t::give, 0x82, 0x0003263B, 0x00032635 },

    // Branches
    { 400176, WORLD_BRANCHES, 16, "Branches Hidden Ointment", ap_location_type_t::hidden, 0x60, 0x0002C7F8 },
    { 400177, WORLD_BRANCHES, 23, "Branches Poison", ap_location_type_t::world, 0x4C, 0x0002C826 },
    { 400178, WORLD_BRANCHES, 24, "Branches Hidden Mattock", ap_location_type_t::hidden, 0x50, 0x0002C82E },
    { 400179, WORLD_BRANCHES, 22, "Branches Hidden Hourglass", ap_location_type_t::hidden, 0x56, 0x0002C820 },

    // Path to Daybreak
    { 400180, WORLD_BRANCHES, 28, "Path to Daybreak Hidden Wingboots 1", ap_location_type_t::hidden, 0x55, 0x0002C848 },
    { 400181, WORLD_BRANCHES, 30, "Path to Daybreak Magical Rod", ap_location_type_t::world, 0x57, 0x0002C858, -1, 1 },
    { 400182, WORLD_BRANCHES, 34, "Path to Daybreak Hidden Wingboots 2", ap_location_type_t::hidden, 0x55, 0x0002C876 },
    { 400183, WORLD_BRANCHES, 30, "Path to Daybreak Poison", ap_location_type_t::world, 0x4C, 0x0002C85A, -1, 0 },
    { 400184, WORLD_BRANCHES, 33, "Path to Daybreak Glove", ap_location_type_t::world, 0x48, 0x0002C86E },
    { 400185, WORLD_BRANCHES, 29, "Path to Daybreak Battle Suit", ap_location_type_t::boss_reward, 0x58, 0x0002C850 },

    // Daybreak
    { 400186, WORLD_SHOPS, 48, "Daybreak Tilte", ap_location_type_t::shop, 0x64, 0x00032490, 0x00032416, 0 },
    { 400187, WORLD_SHOPS, 48, "Daybreak Giant Blade", ap_location_type_t::shop, 0x02, 0x00032493, 0x00032416, 1 },
    { 400188, WORLD_SHOPS, 48, "Daybreak Red Potion", ap_location_type_t::shop, 0x90, 0x00032496, 0x00032416, 2 },
    { 400189, WORLD_SHOPS, 47, "Daybreak Key King", ap_location_type_t::shop, 0x85, 0x00032597, 0x00032562, 0 },
    { 400190, WORLD_SHOPS, 47, "Daybreak Key Queen", ap_location_type_t::shop, 0x86, 0x0003259A, 0x00032562, 1 },

    // Dartmoor Castle
    { 400191, WORLD_DARTMOOR, 4, "Dartmoor Castle Hidden Hourglass", ap_location_type_t::hidden, 0x56, 0x0002C8FC },
    { 400192, WORLD_SHOPS, 55, "Dartmoor Castle Hidden Red Potion", ap_location_type_t::hidden, 0x5D, 0x0002CC40 },

    // Dartmoor
    { 400193, WORLD_SHOPS, 65, "Dartmoor Giant Blade", ap_location_type_t::shop, 0x02, 0x0003249A, 0x00032428, 0 },
    { 400194, WORLD_SHOPS, 65, "Dartmoor Red Potion", ap_location_type_t::shop, 0x90, 0x0003249D, 0x00032428, 1 },
    { 400195, WORLD_SHOPS, 64, "Dartmoor Key King", ap_location_type_t::shop, 0x85, 0x0003259E, 0x00032574, 0 },

    // Fraternal Castle
    { 400196, WORLD_DARTMOOR, 26, "Fraternal Castle Hidden Ointment", ap_location_type_t::hidden, 0x60, 0x0002C996 },
    { 400197, WORLD_SHOPS, 67, "Fraternal Castle Shop Hidden Ointment", ap_location_type_t::hidden, 0x60, 0x0002CC7D },
    { 400198, WORLD_DARTMOOR, 29, "Fraternal Castle Poison 1", ap_location_type_t::world, 0x4C, 0x0002C9AC },
    { 400199, WORLD_DARTMOOR, 19, "Fraternal Castle Poison 2", ap_location_type_t::world, 0x4C, 0x0002C96C, -1, 0 },
    { 400200, WORLD_DARTMOOR, 20, "Fraternal Castle Poison 3", ap_location_type_t::world, 0x4C, 0x0002C974 },
    { 400201, WORLD_DARTMOOR, 19, "Fraternal Castle Red Potion", ap_location_type_t::world, 0x4B, 0x0002C96E, -1, 1 },
    { 400202, WORLD_DARTMOOR, 18, "Fraternal Castle Hidden Hourglass", ap_location_type_t::hidden, 0x56, 0x0002C966 },
    { 400203, WORLD_DARTMOOR, 21, "Fraternal Castle Dragon Slayer", ap_location_type_t::boss_reward, 0x5A, 0x0002C978 },
    { 400204, WORLD_SHOPS, 66, "Fraternal Castle Guru", ap_location_type_t::give, 0x83, 0x0003234B, 0x00032345 },

    // Evil Fortress
    { 400205, WORLD_EVIL, 6, "Evil Fortress Ointment", ap_location_type_t::world, 0x4E, 0x0002CA08 },
    { 400206, WORLD_EVIL, 9, "Evil Fortress Poison 1", ap_location_type_t::world, 0x4C, 0x0002CA1A },
    { 400207, WORLD_EVIL, 10, "Evil Fortress Glove", ap_location_type_t::world, 0x48, 0x0002CA24 },
    { 400208, WORLD_EVIL, 13, "Evil Fortress Poison 2", ap_location_type_t::world, 0x4C, 0x0002CA38 },
    { 400209, WORLD_EVIL, 14, "Evil Fortress Poison 3", ap_location_type_t::world, 0x4C, 0x0002CA42 },
    { 400210, WORLD_EVIL, 17, "Evil Fortress Hidden Glove", ap_location_type_t::hidden, 0x5F, 0x0002CA5C },
};
