#include "Emulator.h"
#include "APU.h"
#include "Cart.h"
#include "Controller.h"
#include "CPU.h"
#include "CPUBUS.h"
#include "ExternalInterface.h"
#include "PPU.h"
#include "PPUBUS.h"
#include "RAM.h"

#include <onut/Input.h>
#include <onut/Renderer.h>


static const int CPU_CLOCK_SPEED = 1789773; // hz
static const int PPU_CLOCK_SPEED = CPU_CLOCK_SPEED * 3; // hz
static const auto MAX_FRAME_DURATION = std::chrono::nanoseconds(1000000000 / 20);


Emulator::Emulator()
{
    m_cpu_bus = new CPUBUS();
    m_ppu_bus = new PPUBUS();

    m_ram = new RAM();
    m_cpu = new CPU();
    m_ppu = new PPU(m_cpu);
    m_apu = new APU();
    m_controller = new Controller();
    m_cart = new Cart("Faxanadu (U).nes");
    m_external_interface = new ExternalInterface();

    m_cpu_bus->add_peripheral(m_ram);
    m_cpu_bus->add_peripheral(m_cpu);
    m_cpu_bus->add_peripheral(m_ppu);
    m_cpu_bus->add_peripheral(m_apu);
    m_cpu_bus->add_peripheral(m_controller);
    m_cpu_bus->add_peripheral(m_cart);
    m_cpu_bus->add_peripheral(m_external_interface);

    m_ppu_bus->add_peripheral(m_ppu);
    m_ppu_bus->add_peripheral(m_cart);

    reset();
}


Emulator::~Emulator()
{
    delete m_external_interface;
    delete m_cart;
    delete m_controller;
    delete m_apu;
    delete m_ppu;
    delete m_cpu;
    delete m_ram;

    delete m_ppu_bus;
    delete m_cpu_bus;
}


void Emulator::reset()
{
    m_cart->reset();
    m_ppu->reset();
    m_cpu->reset();

    m_tick_progress = 0.0;
    m_pputick = 0;
    m_last_frame_time = std::chrono::high_resolution_clock::now();
}


void Emulator::update(float dt)
{
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::nanoseconds time_elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now - m_last_frame_time);
    m_last_frame_time = now;

    // If too large, slow down the simulation to about 20 fps).
    if (time_elapsed_ns > MAX_FRAME_DURATION) time_elapsed_ns = MAX_FRAME_DURATION;

    m_tick_progress += static_cast<double>(time_elapsed_ns.count()) * static_cast<double>(PPU_CLOCK_SPEED) / 1000000000.0;
    if (OInputPressed(OKeyLeftShift)) m_tick_progress *= 4.0;
    while (m_tick_progress > 1.0)
    {
        m_tick_progress--;

        // Tick CPU
        if (m_pputick == 3)
        {
            m_cpu->tick();
            m_pputick = 0;
        }

        // Tick PPU
        m_ppu->tick();
        m_pputick++;
    }
}


void Emulator::render()
{
    oRenderer->clear(Color(0.1f));
    oRenderer->renderStates.sampleFiltering = OFilterNearest;

    m_ppu->render();
    m_cpu->render();
}


void Emulator::serialize(FILE* f, int version) const
{
    m_apu->serialize(f, version);
    m_cart->serialize(f, version);
    m_cpu->serialize(f, version);
    m_ppu->serialize(f, version);
    m_ram->serialize(f, version);
    m_controller->serialize(f, version);
    m_external_interface->serialize(f, version);
}


void Emulator::deserialize(FILE* f, int version)
{
    m_apu->deserialize(f, version);
    m_cart->deserialize(f, version);
    m_cpu->deserialize(f, version);
    m_ppu->deserialize(f, version);
    m_ram->deserialize(f, version);
    m_controller->deserialize(f, version);
    m_external_interface->deserialize(f, version);
}
