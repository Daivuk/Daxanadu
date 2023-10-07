#include "PPUBUS.h"
#include "PPUPeripheral.h"


PPUBUS::PPUBUS()
{
}


PPUBUS::~PPUBUS()
{
}


void PPUBUS::add_peripheral(PPUPeripheral* peripheral)
{
    m_peripherals.push_back(peripheral);
    peripheral->set_ppu_bus(this);
}


bool PPUBUS::write(uint16_t addr, uint8_t data)
{
    bool ret = false;

    for (int i = 0, len = (int)m_peripherals.size(); i < len; ++i)
    {
        ret |= m_peripherals[i]->ppu_write(addr, data);
    }

    return ret;
}


bool PPUBUS::read(uint16_t addr, uint8_t* out_data)
{
    bool ret = false;

    for (int i = 0, len = (int)m_peripherals.size(); i < len; ++i)
    {
        ret |= m_peripherals[i]->ppu_read(addr, out_data);
    }

    return ret;
}
