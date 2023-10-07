#pragma once

#include "CPUPeripheral.h"

#include <stdio.h>


class InputContext;


class Controller final : public CPUPeripheral
{
public:
    void serialize(FILE* f, int version) const;
    void deserialize(FILE* f, int version);

    bool cpu_write(uint16_t addr, uint8_t data) override;
    bool cpu_read(uint16_t addr, uint8_t* out_data) override;

    void set_input_context(InputContext* input_context);
    uint8_t read_inputs(int controller_id);

private:
    uint8_t m_shift_registers[2] = { 0 };
    InputContext* m_input_context = nullptr;
};
