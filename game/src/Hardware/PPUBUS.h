#pragma once

#include <cinttypes>
#include <vector>


class PPUPeripheral;


class PPUBUS final
{
public:
    PPUBUS();
    ~PPUBUS();

    void add_peripheral(PPUPeripheral* peripheral);

    bool write(uint16_t addr, uint8_t data);
    bool read(uint16_t addr, uint8_t* out_data);

private:
    std::vector<PPUPeripheral*> m_peripherals;
};
