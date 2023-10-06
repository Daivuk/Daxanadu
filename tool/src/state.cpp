#include "state.h"
#include "cart.h"

#include <onut/Json.h>


state_t state;


void load_state()
{
    for (const auto& cart_string : cart.strings)
    {
        state.string_labels[cart_string.address] = {};
    }
}


void save_state()
{
    Json::Value json;

    Json::Value json_string_labels;
    for (const auto& kv : state.string_labels)
    {

    }

    onut::saveJson(json, "state.json", true);
}
