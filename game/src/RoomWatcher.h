#pragma once

#include <onut/ForwardDeclaration.h>

#include <string>


OForwardDeclare(Texture);
class CPUBUS;
class TileDrawer;


class RoomWatcher final
{
public:
    RoomWatcher(TileDrawer* tile_drawer, CPUBUS* cpu_bus);

    void update(float dt);
    void render();

private:
    TileDrawer* m_tile_drawer = nullptr;
    CPUBUS* m_cpu_bus = nullptr;

    std::string m_room_name = "";
    std::string m_last_town = "";
    float m_room_anim = 0.0f;
    OTextureRef m_framebuffer;
};
