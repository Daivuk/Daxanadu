#pragma once

#include "CPUPeripheral.h"
#include "PPUPeripheral.h"

#include <stdio.h>
#include <functional>
#include <vector>


class Mapper;


class Cart final : public CPUPeripheral, public PPUPeripheral
{
public:
    Cart(const char* filename);
    ~Cart();

    bool cpu_write(uint16_t addr, uint8_t data) override;
    bool cpu_read(uint16_t addr, uint8_t* out_data) override;
    bool ppu_write(uint16_t addr, uint8_t data) override;
    bool ppu_read(uint16_t addr, uint8_t* out_data) override;

    void serialize(FILE* f, int version) const;
    void deserialize(FILE* f, int version);

    void reset();

    uint8_t* get_prg_rom() const { return m_prg_rom; }
    size_t get_prg_rom_size() const { return m_prg_rom_size; }

    void register_write_callback(const std::function<void(int)>& callback, int addr);
    void register_read_callback(const std::function<void(int)>& callback, int addr);

private:
    uint8_t* m_prg_rom = nullptr;
    uint8_t* m_chr_rom = nullptr;

    size_t m_prg_rom_size = 0;
    size_t m_chr_rom_size = 0;

    Mapper* m_mapper = nullptr;

    std::vector<std::pair<int, std::function<void(int)>>> m_write_callbacks;
    std::vector<std::pair<int, std::function<void(int)>>> m_read_callbacks;
};
