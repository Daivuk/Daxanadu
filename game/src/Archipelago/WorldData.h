#pragma once

#include <cinttypes>
#include <vector>


class WorldData final
{
public:
    struct entity_type_t
    {
        int width = -1;
        int height = -1;
    };

    struct entity_t
    {
        int type = -1;
        int dialog_id = -1;
        int x = -1;
        int y = -1;
        int type_addr = -1;
        int dialog_addr = -1;
    };

    struct screen_t
    {
        int id = -1;
        std::vector<entity_t> entities;
    };

    struct level_t
    {
        int id = -1;
        std::vector<screen_t> screens;
    };

    struct dialog_t
    {
        int id = -1;
        int portrait = -1;
        int portrait_addr = -1;
    };

    WorldData(uint8_t* rom);

    std::vector<level_t> levels;
    std::vector<dialog_t> dialogs;
    std::vector<entity_type_t> entity_types;

private:
    void load_levels(int bank);
    void load_entities();
    void load_dialogs();
    void load_entity_types();

    uint8_t* m_rom = nullptr;
};