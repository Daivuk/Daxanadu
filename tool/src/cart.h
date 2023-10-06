#pragma once

#include <map>
#include <string>
#include <vector>


struct cart_string_t
{
    int address = -1;
    int len = -1;
    std::vector<std::string> text_blocks; // Each element in the array is marked by a pause
};


struct cart_palette_t
{
    uint8_t palette[16];
};


struct cart_t
{
    std::vector<cart_string_t> strings;
    std::vector<cart_palette_t> palettes;
};


extern cart_t cart;


void init_cart();
