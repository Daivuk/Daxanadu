#pragma once

#include <cinttypes>
#include <vector>


class CPUPeripheral;


class CPUBUS final
{
public:
    CPUBUS();
    ~CPUBUS();

    void add_peripheral(CPUPeripheral* peripheral);

    bool write(uint16_t addr, uint8_t data);
    bool read(uint16_t addr, uint8_t* out_data);

private:
    std::vector<CPUPeripheral*> m_peripherals;
};
