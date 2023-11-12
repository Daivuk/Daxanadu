#pragma once

#include "CPUPeripheral.h"

#include <stdio.h>


#if defined(_DEBUG)
#define SHOW_RAM 0
#endif


class RAM final : public CPUPeripheral
{
public:
    RAM();

    void serialize(FILE* f, int version) const;
    void deserialize(FILE* f, int version);

    bool cpu_write(uint16_t addr, uint8_t data) override;
    bool cpu_read(uint16_t addr, uint8_t* out_data) override;

    void render();
    void update(float dt);
    
    uint8_t get(uint16_t addr) const { return m_data[addr]; }
    uint8_t operator[](uint16_t addr) const { return m_data[addr]; }

private:
    uint8_t m_data[0x800];
#if SHOW_RAM
    float m_usage[0x2000];
    float m_time_passed = 0.0f;
#endif
};
