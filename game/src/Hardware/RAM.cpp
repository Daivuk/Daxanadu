#include "RAM.h"

#include <onut/Font.h>
#include <onut/SpriteBatch.h>

#include <memory.h>


RAM::RAM()
{
    memset(m_data, 0, sizeof(m_data));
}


void RAM::serialize(FILE* f, int version) const
{
    fwrite(m_data, 1, 0x800, f);
}


void RAM::deserialize(FILE* f, int version)
{
    fread(m_data, 1, 0x800, f);
}


bool RAM::cpu_write(uint16_t addr, uint8_t data)
{
    if (addr < 0x2000)
    {
        //if (addr == 0x03B0)
        //{
        //    __debugbreak();
        //}
        m_data[addr % 0x800] = data;
        return true;
    }

    return false;
}


bool RAM::cpu_read(uint16_t addr, uint8_t* out_data)
{
    if (addr < 0x2000)
    {
        *out_data = m_data[addr % 0x800];
        return true;
    }

    return false;
}


void RAM::render()
{
#if 1
    auto font = OGetFont("font.fnt");
    char buf[260];

    oSpriteBatch->begin();

    // Equiped weapon
    snprintf(buf, 260, "Weapon: 0x%02X", (int)m_data[0x03BD]);
    oSpriteBatch->drawText(font, buf, Vector2(0, 50));

    // Equiped armor
    snprintf(buf, 260, "Armor: 0x%02X", (int)m_data[0x03BE]);
    oSpriteBatch->drawText(font, buf, Vector2(0, 70));

    // Equiped shield
    snprintf(buf, 260, "Armor: 0x%02X", (int)m_data[0x03BF]);
    oSpriteBatch->drawText(font, buf, Vector2(0, 90));

    // Equiped magic
    snprintf(buf, 260, "Armor: 0x%02X", (int)m_data[0x03C0]);
    oSpriteBatch->drawText(font, buf, Vector2(0, 110));

    // Equiped item
    snprintf(buf, 260, "Item: 0x%02X", (int)m_data[0x03C1]);
    oSpriteBatch->drawText(font, buf, Vector2(0, 130));
    
    snprintf(buf, 260, "Items (%i):", (int)m_data[0x03C6]);
    oSpriteBatch->drawText(font, buf, Vector2(0, 150));
    for (int i = 0; i < (int)m_data[0x03C6]; ++i)
    {
        snprintf(buf, 260, "  0x%02X", (int)m_data[0x03AD + i]);
        oSpriteBatch->drawText(font, buf, Vector2(0, 170 + (float)i * 20));
    }

    oSpriteBatch->end();
#endif
}
