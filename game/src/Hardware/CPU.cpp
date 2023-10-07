#include "CPU.h"
#include "CPUBUS.h"

#include <onut/Font.h>
#include <onut/SpriteBatch.h>

#include <cmath>


static uint8 MCS6502_global_read(uint16 addr, void * readWriteContext)
{
    return ((CPU*)readWriteContext)->MCS6502_read(addr);
}


static void MCS6502_global_write(uint16 addr, uint8 byte, void * readWriteContext)
{
    ((CPU*)readWriteContext)->MCS6502_write(addr, byte);
}


CPU::CPU()
{
    MCS6502Init(&m_cpu_context, MCS6502_global_read, MCS6502_global_write, this);
}


uint8 CPU::MCS6502_read(uint16 addr)
{
    uint8_t data = 0;
    get_cpu_bus()->read(addr, &data);
    return static_cast<uint8>(data);
}


void CPU::MCS6502_write(uint16 addr, uint8 byte)
{
    get_cpu_bus()->write(addr, static_cast<uint8_t>(byte));
}


void CPU::reset()
{
    MCS6502Reset(&m_cpu_context);
}


void CPU::NMI()
{
    MCS6502NMI(&m_cpu_context);
}


void CPU::serialize(FILE* f, int version) const
{
    fwrite(&m_halt_cycles, sizeof(m_halt_cycles), 1, f);

    fwrite(&m_cpu_context.a, sizeof(m_cpu_context.a), 1, f);
    fwrite(&m_cpu_context.x, sizeof(m_cpu_context.x), 1, f);
    fwrite(&m_cpu_context.y, sizeof(m_cpu_context.y), 1, f);
    fwrite(&m_cpu_context.sp, sizeof(m_cpu_context.sp), 1, f);
    fwrite(&m_cpu_context.pc, sizeof(m_cpu_context.pc), 1, f);
    fwrite(&m_cpu_context.p, sizeof(m_cpu_context.p), 1, f);
    fwrite(&m_cpu_context.pendingTiming, sizeof(m_cpu_context.pendingTiming), 1, f);
    fwrite(&m_cpu_context.timingForLastOperation, sizeof(m_cpu_context.timingForLastOperation), 1, f);
    uint8_t b;
    b = m_cpu_context.irqPending;
    fwrite(&b, sizeof(b), 1, f);
    b = m_cpu_context.nmiPending;
    fwrite(&b, sizeof(b), 1, f);
}


void CPU::deserialize(FILE* f, int version)
{
    fread(&m_halt_cycles, sizeof(m_halt_cycles), 1, f);

    fread(&m_cpu_context.a, sizeof(m_cpu_context.a), 1, f);
    fread(&m_cpu_context.x, sizeof(m_cpu_context.x), 1, f);
    fread(&m_cpu_context.y, sizeof(m_cpu_context.y), 1, f);
    fread(&m_cpu_context.sp, sizeof(m_cpu_context.sp), 1, f);
    fread(&m_cpu_context.pc, sizeof(m_cpu_context.pc), 1, f);
    fread(&m_cpu_context.p, sizeof(m_cpu_context.p), 1, f);
    fread(&m_cpu_context.pendingTiming, sizeof(m_cpu_context.pendingTiming), 1, f);
    fread(&m_cpu_context.timingForLastOperation, sizeof(m_cpu_context.timingForLastOperation), 1, f);
    uint8_t b;
    fread(&b, sizeof(b), 1, f);
    m_cpu_context.irqPending = b ? true : false;
    fread(&b, sizeof(b), 1, f);
    m_cpu_context.nmiPending = b ? true : false;
}


void CPU::halt(int cycles)
{
    m_halt_cycles = cycles;
}


void CPU::tick()
{
    if (m_halt_cycles > 0)
    {
        m_halt_cycles--;
        return;
    }
    MCS6502Tick(&m_cpu_context);
}

void CPU::render()
{
#if 0
    auto font = OGetFont("font.fnt");
    oSpriteBatch->begin();
    char buf[260];
    snprintf(buf, 260, "PC: 0x%02X", m_cpu_context.pc);
    oSpriteBatch->drawText(font, buf, Vector2(50, 0));
    oSpriteBatch->end();
#endif
}
