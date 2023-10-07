#include "RAM.h"

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
