#pragma once

#include "Mapper.h"

#include <stdio.h>


class Mapper001 final : public Mapper
{
public:
    Mapper001(int prg_banks, int chr_banks);

    void serialize(FILE* f, int version) const override;
    void deserialize(FILE* f, int version) override;

    bool map_cpu_write(uint16_t addr, uint32_t* mapped_addr, uint8_t data) override;
    bool map_cpu_read(uint16_t addr, uint32_t* mapped_addr) override;
    bool map_ppu_write(uint16_t addr, uint32_t* mapped_addr, uint8_t data) override;
    bool map_ppu_read(uint16_t addr, uint32_t* mapped_addr) override;

    void reset() override;

private:
    uint8_t m_shift_register = 0b10000;
    int m_shift_register_writes = 0;
    int m_control_register = 0;

    int m_first_chr_bank = 0;
    int m_second_chr_bank = 0;
    int m_merged_chr_bank = 0;

    int m_first_prg_bank = 0;
    int m_second_prg_bank = 0;
    int m_merged_prg_bank = 0;
};
