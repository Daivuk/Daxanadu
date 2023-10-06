#pragma once

#include <string>
#include <vector>
#include <map>


struct string_t
{
    std::string label;
};


struct state_t
{
    std::map<int, string_t> string_labels;
};


extern state_t state;


void load_state();
void save_state();
