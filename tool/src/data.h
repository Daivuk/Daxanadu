#pragma once

#include <onut/Point.h>
#include <onut/Vector2.h>

#include <map>
#include <string>
#include <vector>


struct cart_room_t;


struct room_position_t
{
    cart_room_t* room;
    Point pos;
};


struct data_t
{
    std::vector<room_position_t> room_positions;
    std::vector<cart_room_t*> selected_rooms;
};


extern data_t data;


void load_data();
void save_data();
