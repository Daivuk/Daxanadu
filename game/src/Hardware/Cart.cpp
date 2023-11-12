#include "Cart.h"
#include "Mapper001.h"

#include <onut/Dialogs.h>
#include <onut/onut.h>

#include <memory.h>
#include <stdio.h>


static const size_t ROM_HEADER_SIZE = 0x10;
static const uint32_t ROM_CHECKSUM = 0x00D7A9F9; // Faxanadu (U).nes
static const uint8_t ROM_SIGNATURE[4] = { 'N', 'E', 'S', 0x1A };


struct ines_header_t
{
    uint8_t signature[4];
    uint8_t PRG_ROM_size; // In units of 16 KB
    uint8_t CHR_ROM_size; // In units of 8 KB

    uint8_t mirroring : 1;
    uint8_t persistent_memory : 1;
    uint8_t trainer : 1;
    uint8_t four_screen_vram : 1;
    uint8_t lo_mapper : 4;

    uint8_t vs_unisystem : 1;
    uint8_t playchoice_10 : 1;
    uint8_t ines_format : 2;
    uint8_t hi_mapper : 4;

    uint8_t PRG_RAM_size; // In units of 8 KB

    uint8_t pal : 1;
    uint8_t reserved : 7; // Must be 0

    uint8_t tv_system : 2; // 0 = NTSC, 2 = PAL, 1/3 = dual compatible
    uint8_t unused0 : 2;
    uint8_t has_PRG_RAM : 1;
    uint8_t has_bus_conflicts : 1;
    uint8_t unused1 : 2;
    
    uint8_t unused[5];
};


Cart::Cart(const char* filename)
{
    uint8_t* file_data = nullptr;

    // Load the rom from the file. We do lot of validation to make sure it's the right file
    FILE* f = fopen(filename, "rb");
    if (!f)
    {
        onut::showMessageBox("ERROR", "Missing rom file: " + std::string(filename));
        OQuit();
        return;
    }

    // Get file size
    fseek(f, 0, SEEK_END);
    size_t rom_size = ftell(f);
    if (rom_size <= ROM_HEADER_SIZE)
    {
        onut::showMessageBox("ERROR", "(0x80000001) Corrupted rom file: " + std::string(filename));
        fclose(f);
        OQuit();
        return;
    }
    file_data = new uint8_t[rom_size];

    // Read rom
    fseek(f, 0, SEEK_SET);
    if (fread(file_data, 1, rom_size, f) != rom_size)
    {
        onut::showMessageBox("ERROR", "(0x80000002) Corrupted rom file: " + std::string(filename));
        delete[] file_data;
        fclose(f);
        OQuit();
        return;
    }
    fclose(f);

    // Read header
    ines_header_t header;
    auto header_size = sizeof(header); // Just to make sure the padding on this system is correct
    if (header_size != ROM_HEADER_SIZE)
    {
        onut::showMessageBox("ERROR", "(0x80000003) Corrupted executable");
        delete[] file_data;
        fclose(f);
        OQuit();
        return;
    }
    memcpy(&header, file_data, ROM_HEADER_SIZE);

    // Validate signature
    if (memcmp(header.signature, ROM_SIGNATURE, 4))
    {
        onut::showMessageBox("ERROR", "(0x80000004) Corrupted rom file: " + std::string(filename));
        delete[] file_data;
        fclose(f);
        OQuit();
        return;
    }

    // Validate rom filesize
    size_t expected_rom_size = header.PRG_ROM_size * (16 * 1024) + header.CHR_ROM_size * (8 * 1024);
    if (expected_rom_size != rom_size - ROM_HEADER_SIZE)
    {
        onut::showMessageBox("ERROR", "(0x80000005) Corrupted rom file: " + std::string(filename));
        delete[] file_data;
        fclose(f);
        OQuit();
        return;
    }

    // Checksum the file to make sure we're loading the correct one
#if 1 // Only for Faxanadu
    uint32_t checksum = 0;
    uint32_t word = 0;
    for (size_t i = 0; i < rom_size; i += 4)
    {
        memcpy(&word, file_data + i, 4);
        checksum += word;
    }
    if (checksum != ROM_CHECKSUM)
    {
        onut::showMessageBox("ERROR", "(0x80000006) Corrupted rom file: " + std::string(filename));
        fclose(f);
        OQuit();
        return;
    }
    fclose(f);
#endif

    // Copy the part without the header into our rom
    m_prg_rom_size = header.PRG_ROM_size * (16 * 1024);
    m_prg_rom = new uint8_t[m_prg_rom_size];
    memcpy(m_prg_rom, file_data + ROM_HEADER_SIZE, m_prg_rom_size);

    if (header.CHR_ROM_size)
    {
        m_chr_rom_size = header.CHR_ROM_size * (8 * 1024);
        m_chr_rom = new uint8_t[m_chr_rom_size];
        memcpy(m_chr_rom, file_data + ROM_HEADER_SIZE + m_prg_rom_size, m_chr_rom_size);
    }
    else
    {
        m_chr_rom = new uint8_t[2 * 8 * 1024];
        memset(m_chr_rom, 0, 2 * 8 * 1024);
    }

    delete[] file_data;

    // Create mapper
    int mapper_id = (header.hi_mapper << 4) | header.lo_mapper;
    switch (mapper_id)
    {
        case 1: m_mapper = new Mapper001(header.PRG_ROM_size, header.CHR_ROM_size); break;
        default:
            onut::showMessageBox("ERROR", "(0x80000007) Unsupported mapper " + std::to_string(mapper_id) + " for rom file: " + std::string(filename));
            fclose(f);
            OQuit();
            return;
    }
}


Cart::~Cart()
{
    delete[] m_chr_rom;
    delete[] m_prg_rom;
    delete m_mapper;
}


void Cart::serialize(FILE* f, int version) const
{
    fwrite(m_chr_rom, 1, 2 * 8 * 1024, f);
    m_mapper->serialize(f, version);
}

void Cart::deserialize(FILE* f, int version)
{
    fread(m_chr_rom, 1, 2 * 8 * 1024, f);
    m_mapper->deserialize(f, version);
}


bool Cart::cpu_write(uint16_t addr, uint8_t data)
{
    uint32_t mapped_addr;
    if (m_mapper->map_cpu_write(addr, &mapped_addr, data))
    {
        // PRG_RAM
        __debugbreak();
    }

    return false;
}

#include "Daxanadu.h"
#include "Emulator.h"
#include "CPU.h"
#include "RAM.h"

extern Daxanadu* daxanadu;

bool Cart::cpu_read(uint16_t addr, uint8_t* out_data)
{
    uint32_t mapped_addr;
    if (m_mapper->map_cpu_read(addr, &mapped_addr))
    {
        if (mapped_addr == 12 * 0x4000 + 0x83A4 - 0x8000)
        {
            //auto ram = daxanadu->get_emulator()->get_ram();

            //auto SpriteBox_Left = ram->get(0x03E2);
            //auto SpriteBox_Top = ram->get(0x03E3);
            //auto SpriteBox_Width = ram->get(0x03E4);
            //auto SpriteBox_Height = ram->get(0x03E5);

            ////auto x = daxanadu->get_emulator()->get_cpu()->get_x();
            //auto x = ram->get(0x0378); // Current Sprite
            //auto y = daxanadu->get_emulator()->get_cpu()->get_y();
            //if (ram->get(0x02CC + x) == 0x89)
            {
                __debugbreak();
            }
        }
        *out_data = m_prg_rom[mapped_addr];
        return true;
    }

    return false;
}


bool Cart::ppu_write(uint16_t addr, uint8_t data)
{
    uint32_t mapped_addr;
    if (m_mapper->map_ppu_write(addr, &mapped_addr, data))
    {
        // CHR_RAM
        m_chr_rom[addr] = data;
        return true;
    }

    return false;
}


bool Cart::ppu_read(uint16_t addr, uint8_t* out_data)
{
    uint32_t mapped_addr;
    if (m_mapper->map_ppu_read(addr, &mapped_addr))
    {
        *out_data = m_chr_rom[mapped_addr];
        return true;
    }

    return false;
}


void Cart::reset()
{
    m_mapper->reset();
}
