#include "cart.h"
#include <onut/onut.h>
#include <onut/Dialogs.h>


static const int NES_HEADER_SIZE = 16;


cart_t cart;


static void load_strings(FILE* f);
static void load_palettes(FILE* f);


void init_cart()
{
    FILE* f = fopen("Faxanadu (U).nes", "rb");
    if (!f)
    {
        onut::showMessageBox("ERROR", "Failed to load Faxanadu (U).nes");
        onut::quit();
        return;
    }

    load_strings(f);
    load_palettes(f);

    fclose(f);
}


static cart_string_t get_fixed_string(FILE* f, int addr)
{
    char raw[16];
    fseek(f, addr, SEEK_SET);
    fread(raw, 1, 16, f);
    int j = 14;
    for (; j >= 0; --j)
    {
        if (raw[j] != 0x20)
        {
            raw[j + 1] = '\0';
            ++j;
            break;
        }
    }
    raw[15] = '\0';

    cart_string_t string;
    string.address = addr;
    string.len = j;
    string.text_blocks.push_back(raw);
    return string;
}


void load_strings(FILE* f)
{
    // Titles
    {
        const int start_addr = 0x0003F659;
        const int end_addr = 0x0003F74E;

        for (int i = start_addr; i < end_addr; i += 16)
            cart.strings.push_back(get_fixed_string(f, i));
    }

    // Item names
    {
        const int start_addr = 0x00031B4E;
        const int end_addr = 0x00031DBE;

        for (int i = start_addr; i < end_addr; i += 16)
            cart.strings.push_back(get_fixed_string(f, i));
    }

    // Multiline strings
    {
        const int start_addr = 0x00034310;
        const int end_addr = 0x000373CA;
        
        fseek(f, start_addr, SEEK_SET);
        int addr = start_addr;
        do
        {
            cart_string_t string;
            string.address = addr;
            string.len = 0;
            string.text_blocks.push_back("");

            int c = 0;
            int cur_line_len = 0;
            do
            {
                fread(&c, 1, 1, f);
                if (c == 0xFC)
                {
                    string.text_blocks.push_back("");
                    cur_line_len = 0;
                }
                else if (c == 0xFD)
                {
                    string.text_blocks.back() += ' ';
                    cur_line_len++;
                }
                else if (c == 0xFE)
                {
                    string.text_blocks.back() += '\n';
                    cur_line_len = 0;
                }
                else if (c != 0xFF)
                {
                    string.text_blocks.back() += (char)c;
                    cur_line_len++;
                }

                if (cur_line_len == 16) // Forced line wrap
                {
                    string.text_blocks.back() += '\n';
                    cur_line_len = 0;
                }

            } while (c != 0xFF);

            cart.strings.push_back(string);

            addr = ftell(f);
        } while (addr < end_addr);
    }
}


void load_palettes(FILE* f)
{
    const int start_addr = 0x0002C010;
    const int end_addr = 0x0002C200;

    fseek(f, start_addr, SEEK_SET);
    for (int i = start_addr; i < end_addr; i += 16)
    {
        cart_palette_t palette;
        fread(&palette, 1, 16, f);
        cart.palettes.push_back(palette);
    }
}
