#pragma once

#include <cinttypes>
#include <stdio.h>


class Mapper
{
public:
    Mapper(int prg_banks, int chr_banks);
    virtual ~Mapper() {}

    virtual void serialize(FILE* f, int version) const = 0;
    virtual void deserialize(FILE* f, int version) = 0;

    virtual bool map_cpu_write(uint16_t addr, uint32_t* mapped_addr, uint8_t data) { return false; }
    virtual bool map_cpu_read(uint16_t addr, uint32_t* mapped_addr) { return false; }
    virtual bool map_ppu_write(uint16_t addr, uint32_t* mapped_addr, uint8_t data) { return false; }
    virtual bool map_ppu_read(uint16_t addr, uint32_t* mapped_addr) { return false; }

    virtual void reset() {}

    int get_prg_banks() const { return m_prg_banks; }
    int get_chr_banks() const { return m_chr_banks; }

private:
    int m_prg_banks = 0;
    int m_chr_banks = 0;
};
