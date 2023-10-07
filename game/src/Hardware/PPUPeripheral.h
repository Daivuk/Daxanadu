#pragma once

#include <cinttypes>


class PPUBUS;


class PPUPeripheral
{
public:
    virtual ~PPUPeripheral() {}

    virtual bool ppu_write(uint16_t addr, uint8_t data) { return false; };
    virtual bool ppu_read(uint16_t addr, uint8_t* out_data) { return false; };

    PPUBUS* get_ppu_bus() const { return m_ppu_bus; }
    void set_ppu_bus(PPUBUS* ppu_bus);

private:
    PPUBUS* m_ppu_bus = nullptr;
};
