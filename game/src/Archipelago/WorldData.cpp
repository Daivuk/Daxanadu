#include "WorldData.h"


static const int CHUNK_TO_ALL_LEVELS_MAP[] = { 0, 2, 3, 1, 5, 6, 4, 7 };


static std::vector<int> read_pointer_table(uint8_t* rom, int start, int offset)
{
    int pos = offset + start;
    std::vector<uint16_t> pointers;
    uint16_t end;
    memcpy(&end, rom + pos, 2);
    pos += 2;
    pointers.push_back(end);
    const int count = ((end + offset) - (start + offset)) / 2;
    pointers.resize(count);
    if (count > 1)
        memcpy(pointers.data() + 1, rom + pos, (count - 1) * 2);

    std::vector<int> ret;
    for (int i = 0; i < (int)pointers.size(); ++i)
        ret.push_back((int)pointers[i]/* + NES_HEADER_SIZE*/);

    return ret;
}


WorldData::WorldData(uint8_t* rom)
    : m_rom(rom)
{
    load_levels(0);
    load_levels(1);
    load_levels(2);
    load_entities();
    load_dialogs();
}


void WorldData::load_levels(int bank)
{
    int bank_offset = bank * 0x4000;

    const auto pointers = read_pointer_table(m_rom, 0, bank_offset);
    for (const auto pointer : pointers)
    {
        level_t level;
        int level_id = (int)levels.size();
        level.id = level_id;

        const auto screen_pointers = read_pointer_table(m_rom, pointer, bank_offset);
        for (const auto screen_pointer : screen_pointers)
        {
            int screen_id = (int)level.screens.size();
            screen_t screen;
            screen.id = screen_id;
            level.screens.push_back(screen);
        }

        // TODO: Load doors
        //load_chunk_metadata(f, level, level_id);

        levels.push_back(level);
    }
}


void WorldData::load_entities()
{
    uint16_t pointers[8];
    memcpy(pointers, m_rom + 0x0002C220 - 0x10, 2 * 8);

    for (auto& level : levels)
    {
        int pointers_bank_offset = 11 * 0x4000 + (int)pointers[CHUNK_TO_ALL_LEVELS_MAP[level.id]] - 0x8000;

        std::vector<uint16_t> entity_pointers;
        entity_pointers.resize(level.screens.size());
        auto rom = m_rom + pointers_bank_offset;
        memcpy(entity_pointers.data(), rom, 2 * entity_pointers.size());
        rom += 2 * entity_pointers.size();

        for (int screen_id = 0; screen_id < (int)level.screens.size(); ++screen_id)
        {
            int screen_bank_offset = 11 * 0x4000 + (int)entity_pointers[screen_id] - 0x8000;
            rom = m_rom + screen_bank_offset;

            struct raw_entity_t
            {
                uint8_t entity_id = 0xFF;
                struct entity_pos_t
                {
                    uint8_t x : 4;
                    uint8_t y : 4;
                } pos = {0xFF, 0xFF};
                int addr = 0;
            };

            struct raw_dialog_t
            {
                uint8_t dialog_id = 0xFF;
                int addr = 0;
            };

            std::vector<raw_entity_t> raw_entities;
            std::vector<raw_dialog_t> raw_entity_dialogs;

            while (true)
            {
                raw_entity_t raw_entity;
                raw_entity.addr = (int)(rom - m_rom);
                raw_entity.entity_id = rom[0]; rom++;
                if (raw_entity.entity_id == 0xFF) break;
                memcpy(&raw_entity.pos, rom, 1); rom++;
                raw_entities.push_back(raw_entity);
            }
            while (true)
            {
                raw_dialog_t raw_dialog;
                raw_dialog.addr = (int)(rom - m_rom);
                raw_dialog.dialog_id = rom[0]; rom++;
                if (raw_dialog.dialog_id == 0xFF) break;
                raw_entity_dialogs.push_back(raw_dialog);
            }

            for (int i = 0; i < (int)raw_entities.size(); ++i)
            {
                entity_t entity;
                entity.type = raw_entities[i].entity_id;
                entity.x = raw_entities[i].pos.x;
                entity.y = raw_entities[i].pos.y;
                entity.type_addr = raw_entities[i].addr;
                if (i < (int)raw_entity_dialogs.size())
                {
                    entity.dialog_addr = raw_entity_dialogs[i].addr;
                    entity.dialog_id = raw_entity_dialogs[i].dialog_id;
                }
                level.screens[screen_id].entities.push_back(entity);
            }
        }
    }
}


void WorldData::load_dialogs()
{
    int lo_start = 0x00031F7B - 0x10;
    int hi_start = 0x00032013 - 0x10;

    int pointer_count = hi_start - lo_start;

    std::vector<uint8_t> lo_pointers;
    lo_pointers.resize(pointer_count);
    auto rom = m_rom + lo_start;
    memcpy(lo_pointers.data(), rom, lo_pointers.size());

    std::vector<uint8_t> hi_pointers;
    hi_pointers.resize(pointer_count);
    rom = m_rom + hi_start;
    memcpy(hi_pointers.data(), rom, hi_pointers.size());

    std::vector<int> pointers;
    for (int i = 0; i < pointer_count; ++i)
    {
        int pointer = (hi_pointers[i] << 8) | lo_pointers[i];
        pointer += 10 * 0x4000;
        pointers.push_back(pointer);
    }

    int dialog_id = 0;
    for (auto pointer : pointers)
    {
        dialog_t dialog;
        dialog.id = dialog_id++;
        dialog.portrait_addr = pointer;
        dialog.portrait = (int)m_rom[pointer];
        dialogs.push_back(dialog);
    }
}