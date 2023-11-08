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


struct ap_location_t
{
    int64_t id = -1;
    std::string name;
    ap_location_type_t type = ap_location_type_t::unknown;
    int item_id = -1;
    int addr = -1;
    int giver_cond_addr = -1;
};


static ap_location_t AP_LOCATIONS[] = {
    // Eolis
    { 400100, "Eolis Guru", ap_location_type_t::give, 0x80, 0x000320C4, 0x000320BE },
    { 400101, "Eolis Key Jack", ap_location_type_t::shop, 0x87, 0x0003257A },
    { 400102, "Eolis Hand Dagger", ap_location_type_t::shop, 0x00, 0x0003242E },
    //{ , "Eolis Red Potion", ap_location_type_t::shop, 0x90, 0x00032431 },  // For now leave Red Potions untouched so player can buy them
    { 400103, "Eolis Elixir", ap_location_type_t::shop, 0x92, 0x00032434 },
    { 400104, "Eolis Deluge", ap_location_type_t::shop, 0x60, 0x00032437 },

    // Path to Apolune
    { 400105, "Path to Apolune Magic Shield", ap_location_type_t::shop, 0x42, 0x00032448 },
    { 400106, "Path to Apolune Death", ap_location_type_t::shop, 0x63, 0x0003244B },

    // Apolune
    { 400107, "Apolune Small Shield", ap_location_type_t::shop, 0x40, 0x0003243B },
    { 400108, "Apolune Hand Dagger", ap_location_type_t::shop, 0x00, 0x0003243E },
    { 400109, "Apolune Deluge", ap_location_type_t::shop, 0x60, 0x00032441 },
    //{ , "Apolune Red Potion", ap_location_type_t::shop, 0x90, 0x00032444 },  // For now leave Red Potions untouched so player can buy them
    { 400110, "Apolune Key Jack", ap_location_type_t::shop, 0x87, 0x0003257E },

    // Tower of Trunk
    { 400111, "Tower of Trunk Hidden Mattock", ap_location_type_t::hidden, 0x50, 0x0002C337 },
    { 400112, "Tower of Trunk Hidden Hourglass", ap_location_type_t::hidden, 0x56, 0x0002C355 },
    { 400113, "Tower of Trunk Boss Mattock", ap_location_type_t::boss_reward, 0x5B, 0x0002C367 },

    // Path to Forepaw
    { 400114, "Path to Forepaw Hidden Red Potion", ap_location_type_t::hidden, 0x5D, 0x0002C327 },
    { 400115, "Path to Forepaw Glove", ap_location_type_t::world, 0x48, 0x0002C381 },

    // Forepaw
    { 400116, "Forepaw Long Sword", ap_location_type_t::shop, 0x01, 0x0003244F },
    { 400117, "Forepaw Studded Mail", ap_location_type_t::shop, 0x21, 0x00032452 },
    { 400118, "Forepaw Small Shield", ap_location_type_t::shop, 0x40, 0x00032455 },
    //{ , "Forepaw Red Potion", ap_location_type_t::shop, 0x90, 0x00032458 },  // For now leave Red Potions untouched so player can buy them
    { 400119, "Forepaw Wingboots", ap_location_type_t::shop, 0x8F, 0x0003245B },
    { 400120, "Forepaw Key Jack", ap_location_type_t::shop, 0x87, 0x00032582 },
    { 400121, "Forepaw Key Queen", ap_location_type_t::shop, 0x86, 0x00032585 },

    // Trunk
    { 400122, "Trunk Hidden Ointment", ap_location_type_t::hidden, 0x60, 0x0002C397 },
    { 400123, "Trunk Hidden Red Potion", ap_location_type_t::hidden, 0x5D, 0x0002C3B3 },
    { 400124, "Trunk Red Potion", ap_location_type_t::world, 0x4B, 0x0002C3B9 },
    { 400125, "Sky Spring", ap_location_type_t::spring, 0x62, 0x000321A5, 0x0003219A },

    // Joker Spring
    { 400126, "Joker Spring Ruby Ring", ap_location_type_t::give, 0x81, 0x000321D4, 0x000321CE },
    { 400127, "Joker Spring", ap_location_type_t::spring, 0x63, 0x000321CA, 0x000321C4 },

    // Tower of Fortress
    { 400128, "Tower of Fortress Poison 1", ap_location_type_t::world, 0x4C, 0x0002C3F6 },
    { 400129, "Tower of Fortress Poison 2", ap_location_type_t::world, 0x4C, 0x0002C442 },
    { 400130, "Tower of Fortress Hidden Wingboots", ap_location_type_t::world, 0x55, 0x0002C410 },
    { 400131, "Tower of Fortress Ointment", ap_location_type_t::world, 0x4E, 0x0002C450 },
    { 400132, "Tower of Fortress Boss Wingboots", ap_location_type_t::world, 0x5C, 0x0002C46A },
    { 400133, "Tower of Fortress Elixir", ap_location_type_t::world, 0x4D, 0x0002C472 },
    { 400134, "Tower of Fortress Guru", ap_location_type_t::give, 0x88, 0x00032199, 0x00032193 },
    { 400135, "Tower of Fortress Spring", ap_location_type_t::spring, 0x61, 0x000321AC, 0x000321AC },

    // Path to Mascon
    { 400136, "Path to Mascon Hidden Wingboots", ap_location_type_t::hidden, 0x55, 0x0002C598 },

    // Tower of Red Potion
    { 400137, "Tower of Red Potion", ap_location_type_t::world, 0x4B, 0x0002C724 },

    // Mascon
    { 400138, "Mascon Large Shield", ap_location_type_t::shop, 0x41, 0x0003245F },
    { 400139, "Mascon Thunder", ap_location_type_t::shop, 0x61, 0x00032462 },
    { 400140, "Mascon Mattock", ap_location_type_t::shop, 0x89, 0x00032465 },
    //{ , "Mascon Red Potion", ap_location_type_t::shop, 0x90, 0x00032468 },   // For now leave Red Potions untouched so player can buy them
    { 400141, "Mascon Key Jack", ap_location_type_t::shop, 0x87, 0x00032589 },
    { 400142, "Mascon Key Queen", ap_location_type_t::shop, 0x86, 0x0003258C },

    // Path to Victim
    { 400143, "Misty Shop Death", ap_location_type_t::shop, 0x63, 0x0003246C },
    { 400144, "Misty Shop Hourglass", ap_location_type_t::shop, 0x8D, 0x0003246F },
    { 400145, "Misty Shop Elixir", ap_location_type_t::shop, 0x92, 0x00032472 },
    //{ , "Misty Shop Red Potion", ap_location_type_t::shop, 0x90, 0x00032475 },   // For now leave Red Potions untouched so player can buy them
    { 400146, "Misty Doctor Office", ap_location_type_t::hidden, 0x55, 0x0002CBB4 },

    // Tower of Suffer
    { 400147, "Tower of Suffer Hidden Wingboots", ap_location_type_t::hidden, 0x55, 0x0002C670 },
    { 400148, "Tower of Suffer Hidden Hourglass", ap_location_type_t::hidden, 0x56, 0x0002C660 },
    { 400149, "Tower of Suffer Pendant", ap_location_type_t::boss_reward, 0xA4, 0x0002C67E },

    // Victim
    { 400150, "Victim Full Plate", ap_location_type_t::shop, 0x22, 0x00032479 },
    { 400151, "Victim Mattock", ap_location_type_t::shop, 0x89, 0x0003247C },
    //{ , "Victim Red Potion", ap_location_type_t::shop, 0x90, 0x0003247F },   // For now leave Red Potions untouched so player can buy them
    { 400152, "Victim Key King", ap_location_type_t::shop, 0x85, 0x00032590 },
    { 400153, "Victim Key Queen", ap_location_type_t::shop, 0x86, 0x00032593 },

    // Mist
    { 400154, "Mist Hidden Poison 1", ap_location_type_t::hidden, 0x5E, 0x0002C5FE },
    { 400155, "Mist Hidden Poison 2", ap_location_type_t::hidden, 0x5E, 0x0002C5F8 },
    { 400156, "Mist Hidden Wingboots", ap_location_type_t::hidden, 0x55, 0x0002C5E4 },
    { 400157, "Misty Magic Hall", ap_location_type_t::give, 0x62, 0x00032373, 0x0003236A },
    { 400158, "Misty House", ap_location_type_t::give, 0x84, 0x0003228B, 0x00032285 },

    // Useless Tower
    { 400159, "Useless Tower", ap_location_type_t::hidden, 0x5D, 0x0002C736 },

    // Tower of Mist
    { 400160, "Tower of Mist Hidden Ointment", ap_location_type_t::hidden, 0x60, 0x0002C6FE },
    { 400161, "Tower of Mist Elixir", ap_location_type_t::world, 0x4D, 0x0002C6D0 },
    { 400162, "Tower of Mist Black Onyx", ap_location_type_t::boss_reward, 0x49, 0x0002C71C },

    // Path to Conflate
    { 400163, "Path to Conflate Hidden Ointment", ap_location_type_t::hidden, 0x60, 0x0002C7B0 },
    { 400164, "Path to Conflate Poison", ap_location_type_t::hidden, 0x4C, 0x0002C7DE },

    // Helm Branch
    { 400165, "Helm Branch Hidden Glove", ap_location_type_t::hidden, 0x5F, 0x0002C7CA },
    { 400166, "Helm Branch Battle Helmet", ap_location_type_t::boss_reward, 0x59, 0x0002C7C2 },

    // Conflate
    { 400167, "Conflate Giant Blade", ap_location_type_t::shop, 0x02, 0x00032483 },
    { 400168, "Conflate Magic Shield", ap_location_type_t::shop, 0x42, 0x00032486 },
    { 400169, "Conflate Wingboots", ap_location_type_t::shop, 0x8F, 0x00032489 },
    //{ , "Conflate Red Potion", ap_location_type_t::shop, 0x90, 0x0003248C }, // For now leave Red Potions untouched so player can buy them
    { 400170, "Conflate Guru", ap_location_type_t::give, 0x82, 0x0003263B, 0x00032635 },

    // Branches
    { 400171, "Branches Hidden Ointment", ap_location_type_t::hidden, 0x60, 0x0002C7F8 },
    { 400172, "Branches Poison", ap_location_type_t::world, 0x4C, 0x0002C826 },
    { 400173, "Branches Hidden Mattock", ap_location_type_t::hidden, 0x50, 0x0002C82E },
    { 400174, "Branches Hidden Hourglass", ap_location_type_t::hidden, 0x56, 0x0002C820 },

    // Path to Daybreak
    { 400175, "Path to Daybreak Hidden Wingboots 1", ap_location_type_t::hidden, 0x55, 0x0002C848 },
    { 400176, "Path to Daybreak Magical Rod", ap_location_type_t::world, 0x57, 0x0002C858 },
    { 400177, "Path to Daybreak Hidden Wingboots 2", ap_location_type_t::hidden, 0x55, 0x0002C876 },
    { 400178, "Path to Daybreak Poison", ap_location_type_t::world, 0x4C, 0x0002C85A },
    { 400179, "Path to Daybreak Glove", ap_location_type_t::world, 0x48, 0x0002C86E },

    // Daybreak
    { 400180, "Daybreak Tilte", ap_location_type_t::shop, 0x64, 0x00032490 },
    { 400181, "Daybreak Giant Blade", ap_location_type_t::shop, 0x02, 0x00032493 },
    //{ , "Daybreak Red Potion", ap_location_type_t::shop, 0x90, 0x00032496 }, // For now leave Red Potions untouched so player can buy them
    { 400182, "Daybreak Key King", ap_location_type_t::shop, 0x85, 0x00032597 },
    { 400183, "Daybreak Key Queen", ap_location_type_t::shop, 0x86, 0x0003259A },

    // Dartmoor Castle
    { 400184, "Dartmoor Castle Hidden Hourglass", ap_location_type_t::hidden, 0x56, 0x0002C8FC },
    { 400185, "Dartmoor Castle Hidden Red Potion", ap_location_type_t::hidden, 0x5D, 0x0002CC40 },

    // Dartmoor
    { 400186, "Dartmoor Giant Blade", ap_location_type_t::shop, 0x02, 0x0003249A },
    //{ , "Dartmoor Red Potion", ap_location_type_t::shop, 0x90, 0x0003249D }, // For now leave Red Potions untouched so player can buy them
    { 400187, "Dartmoor Key King", ap_location_type_t::shop, 0x85, 0x0003259E },

    // Fraternal Castle
    { 400188, "Fraternal Castle Hidden Ointment", ap_location_type_t::hidden, 0x60, 0x0002C996 },
    { 400189, "Fraternal Castle Shop Hidden Ointment", ap_location_type_t::hidden, 0x60, 0x0002CC7D },
    { 400190, "Fraternal Castle Poison 1", ap_location_type_t::world, 0x4C, 0x0002C9AC },
    { 400191, "Fraternal Castle Poison 2", ap_location_type_t::world, 0x4C, 0x0002C96C },
    { 400192, "Fraternal Castle Poison 3", ap_location_type_t::world, 0x4C, 0x0002C974 },
    { 400193, "Fraternal Castle Red Potion", ap_location_type_t::world, 0x4B, 0x0002C96E },
    { 400194, "Fraternal Castle Hidden Hourglass", ap_location_type_t::hidden, 0x56, 0x0002C966 },
    { 400195, "Fraternal Castle Dragon Slayer", ap_location_type_t::boss_reward, 0x5A, 0x0002C978 },
    { 400196, "Fraternal Castle Guru", ap_location_type_t::give, 0x83, 0x0003234B, 0x00032345 },

    // Evil Fortress
    { 400197, "Evil Fortress Ointment", ap_location_type_t::world, 0x4E, 0x0002CA08 },
    { 400198, "Evil Fortress Poison 1", ap_location_type_t::world, 0x4C, 0x0002CA1A },
    { 400199, "Evil Fortress Glove", ap_location_type_t::world, 0x48, 0x0002CA24 },
    { 400200, "Evil Fortress Poison 2", ap_location_type_t::world, 0x4C, 0x0002CA38 },
    { 400201, "Evil Fortress Poison 3", ap_location_type_t::world, 0x4C, 0x0002CA42 },
    { 400202, "Evil Fortress HIdden Glove", ap_location_type_t::hidden, 0x5F, 0x0002CA5C },
    { 400203, "Evil One", ap_location_type_t::boss, 0x33, 0x0002C9E4 },
};
