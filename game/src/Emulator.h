#pragma once

#include <stdio.h>
#include <chrono>


class APU;
class Cart;
class Controller;
class CPU;
class CPUBUS;
class PPU;
class PPUBUS;
class RAM;
class ExternalInterface;


class Emulator final
{
public:
    Emulator();
    ~Emulator();

    void serialize(FILE* f, int version) const;
    void deserialize(FILE* f, int version);

    void reset();
    void update(float dt);
    void render();

    ExternalInterface* get_external_interface() const { return m_external_interface; }
    Cart* get_cart() const { return m_cart; }
    PPU* get_ppu() const { return m_ppu; }
    CPU* get_cpu() const { return m_cpu; }
    Controller* get_controller() const { return m_controller; }
    CPUBUS* get_cpu_bus() const { return m_cpu_bus; };
    APU* get_apu() const { return m_apu; }
    RAM* get_ram() const { return m_ram; }

private:
    CPUBUS* m_cpu_bus = nullptr;
    PPUBUS* m_ppu_bus = nullptr;

    APU* m_apu = nullptr;
    Cart* m_cart = nullptr;
    Controller* m_controller = nullptr;
    CPU* m_cpu = nullptr;
    PPU* m_ppu = nullptr;
    RAM* m_ram = nullptr;
    ExternalInterface* m_external_interface = nullptr;

    std::chrono::high_resolution_clock::time_point m_last_frame_time;
    double m_tick_progress = 0.0;
    int m_pputick = 0; // 0, 1, 2 then repeats
};
