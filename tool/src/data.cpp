#include "data.h"
#include "cart.h"

#include <onut/Json.h>


data_t data;


void load_data()
{
    data.room_positions.resize(cart.rooms.size());
    for (int i = 0; i < (int)cart.rooms.size(); ++i)
    {
        data.room_positions[i].room = &cart.rooms[i];
    }

    Json::Value json;
    if (!onut::loadJson(json, "data.json")) return;

    Json::Value json_rooms = json["rooms"];
    for (const auto& json_room : json_rooms)
    {
        auto level_id = onut::deserializeInt32(json_room["chunk_id"], -1);
        auto first_screen_id = onut::deserializeInt32(json_room["first_screen_id"], -1);
        if (level_id < 0 || level_id >= (int)cart.chunks.size()) continue;
        auto& level = cart.chunks[level_id];
        if (first_screen_id < 0 || first_screen_id >= (int)level.screens.size()) continue;

        auto room_id = level.screens[first_screen_id].room_id;
        if (room_id != -1)
        {
            data.room_positions[room_id].pos.x = onut::deserializeInt32(json_room["x"], 0);
            data.room_positions[room_id].pos.y = onut::deserializeInt32(json_room["y"], 0);
        }
    }
}


void save_data()
{
    Json::Value json;

    // Room positions
    Json::Value json_rooms(Json::arrayValue);
    int room_id = 0;
    for (const auto& room : cart.rooms)
    {
        Json::Value json_room;

        json_room["chunk_id"] = room.chunk_id;
        json_room["first_screen_id"] = room.screens[0];
        json_room["x"] = data.room_positions[room.id].pos.x;
        json_room["y"] = data.room_positions[room.id].pos.y;

        json_rooms.append(json_room);
        ++room_id;
    }

    json["rooms"] = json_rooms;

    onut::saveJson(json, "data.json", true);
}
