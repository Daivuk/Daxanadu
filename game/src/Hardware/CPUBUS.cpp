#include "CPUBUS.h"
#include "CPUPeripheral.h"


CPUBUS::CPUBUS()
{
}


CPUBUS::~CPUBUS()
{
}


void CPUBUS::add_peripheral(CPUPeripheral* peripheral)
{
    m_peripherals.push_back(peripheral);
    peripheral->set_cpu_bus(this);
}


bool CPUBUS::write(uint16_t addr, uint8_t data)
{
    bool ret = false;

    for (int i = 0, len = (int)m_peripherals.size(); i < len; ++i)
    {
        ret |= m_peripherals[i]->cpu_write(addr, data);
    }

    return ret;
}


bool CPUBUS::read(uint16_t addr, uint8_t* out_data)
{
    bool ret = false;

    for (int i = 0, len = (int)m_peripherals.size(); i < len; ++i)
    {
        ret |= m_peripherals[i]->cpu_read(addr, out_data);
    }

    return ret;
}
