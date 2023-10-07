#pragma once

#include "CPUPeripheral.h"
#include "MCS6502.h"

#include <stdio.h>


class CPU final : public CPUPeripheral
{
public:
    CPU();

    void serialize(FILE* f, int version) const;
    void deserialize(FILE* f, int version);

    void NMI();
    void halt(int cycles);
    void reset();
    void tick();

    void render();

public:
    // Reserved for the MSC6502 emulator
    uint8 MCS6502_read(uint16 addr);
    void MCS6502_write(uint16 addr, uint8 byte);

private:
    MCS6502ExecutionContext m_cpu_context;
    int m_halt_cycles = 0;
};
