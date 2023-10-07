#include "Mapper001.h"

#include <memory.h>


Mapper001::Mapper001(int prg_banks, int chr_banks)
    : Mapper(prg_banks, chr_banks)
{
}


bool Mapper001::map_cpu_write(uint16_t addr, uint32_t* mapped_addr, uint8_t data)
{
    if (addr >= 0x6000 && addr <= 0x7FFF)
    {
        // We dont need cart ram for Faxanadu, so let's use this address space to call into C++ functions. Check ExternalInterface peripheral
        return false; // Pretend like nothing happened
    }

    if (addr < 0x8000) return false;

    if (data & 0x80)
    {
        m_shift_register = 0b10000;
        m_shift_register_writes = 0;
        m_control_register |= 0x0C;
    }
    else
    {
        // This gets messy
        m_shift_register >>= 1;
        m_shift_register |= (data & 0b1) << 4;
        m_shift_register_writes++;

        if (m_shift_register_writes == 5)
        {
            int mode = (addr >> 13) & 3;

            switch (mode)
            {
                case 0:
                    m_control_register = m_shift_register & 0b11111;
                    // Don't care about mirror modes for Faxanadu
                    break;
                case 1:
                    if (m_control_register & 0b00010000)
                    {
                        m_first_chr_bank = m_shift_register & 0b11111;
                    }
                    else
                    {
                        m_merged_chr_bank = m_shift_register & 0b11110;
                    }
                    break;
                case 2:
                    if (m_control_register & 0b00010000)
                    {
                        m_second_chr_bank = m_shift_register & 0b11111;
                    }
                    break;
                case 3:
                {
                    int mode = (m_control_register >> 2) & 3;
                    switch (mode)
                    {
                        case 0:
                        case 1:
                            m_merged_prg_bank = (m_shift_register >> 1) & 0b111;
                            break;
                        case 2:
                            m_first_prg_bank = 0;
                            m_second_prg_bank = m_shift_register & 0b1111;
                            break;
                        case 3:
                            m_first_prg_bank = m_shift_register & 0b1111;
                            m_second_prg_bank = get_prg_banks() - 1;
                            break;
                    }
                    break;
                }
            }

            m_shift_register = 0b10000;
            m_shift_register_writes = 0;
        }
    }

    return false;
}


bool Mapper001::map_cpu_read(uint16_t addr, uint32_t* mapped_addr)
{
    if (addr >= 0x6000 && addr <= 0x7FFF)
    {
        // We dont need cart ram for Faxanadu, lets ignore this for now
        return false;
    }

    if (addr < 0x8000) return false;

    if (m_control_register & 0b00001000)
    {
        if (addr <= 0xBFFF)
        {
            *mapped_addr = m_first_prg_bank * 0x4000 + (addr - 0x8000);
        }
        else
        {
            *mapped_addr = m_second_prg_bank * 0x4000 + (addr - 0xC000);
        }
    }
    else
    {
        *mapped_addr = m_merged_prg_bank * 0x8000 + (addr - 0x8000);
    }

    return true;
}


bool Mapper001::map_ppu_write(uint16_t addr, uint32_t* mapped_addr, uint8_t data)
{
    if (addr >= 0x2000) return false;

    // Faxanadu use RAM chr
    *mapped_addr = addr;
    return true;
}


bool Mapper001::map_ppu_read(uint16_t addr, uint32_t* mapped_addr)
{
    if (addr >= 0x2000) return false;

    // Faxanadu use RAM chr
    *mapped_addr = addr;
    return true;
}


void Mapper001::reset()
{
    m_shift_register = 0b10000;
    m_shift_register_writes = 0;
    m_control_register = 0x1C;

    m_first_chr_bank = 0;
    m_second_chr_bank = 0;
    m_merged_chr_bank = 0;

    m_first_prg_bank = 0;
    m_second_prg_bank = get_prg_banks() - 1;
    m_merged_prg_bank = 0;
}


void Mapper001::serialize(FILE* f, int version) const
{
    fwrite(&m_shift_register, sizeof(m_shift_register), 1, f);
    fwrite(&m_shift_register_writes, sizeof(m_shift_register_writes), 1, f);
    fwrite(&m_control_register, sizeof(m_control_register), 1, f);

    fwrite(&m_first_chr_bank, sizeof(m_first_chr_bank), 1, f);
    fwrite(&m_second_chr_bank, sizeof(m_second_chr_bank), 1, f);
    fwrite(&m_merged_chr_bank, sizeof(m_merged_chr_bank), 1, f);

    fwrite(&m_first_prg_bank, sizeof(m_first_prg_bank), 1, f);
    fwrite(&m_second_prg_bank, sizeof(m_second_prg_bank), 1, f);
    fwrite(&m_merged_prg_bank, sizeof(m_merged_prg_bank), 1, f);
}


void Mapper001::deserialize(FILE* f, int version)
{
    fread(&m_shift_register, sizeof(m_shift_register), 1, f);
    fread(&m_shift_register_writes, sizeof(m_shift_register_writes), 1, f);
    fread(&m_control_register, sizeof(m_control_register), 1, f);

    fread(&m_first_chr_bank, sizeof(m_first_chr_bank), 1, f);
    fread(&m_second_chr_bank, sizeof(m_second_chr_bank), 1, f);
    fread(&m_merged_chr_bank, sizeof(m_merged_chr_bank), 1, f);

    fread(&m_first_prg_bank, sizeof(m_first_prg_bank), 1, f);
    fread(&m_second_prg_bank, sizeof(m_second_prg_bank), 1, f);
    fread(&m_merged_prg_bank, sizeof(m_merged_prg_bank), 1, f);
}
