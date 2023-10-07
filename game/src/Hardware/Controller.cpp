#include "Controller.h"
#include "InputContext.h"

#include <onut/GamePad.h>
#include <onut/Input.h>


uint8_t Controller::read_inputs(int controller_id)
{
    if (!m_input_context) return 0;

    uint8_t ret = 0;

    auto inputs = m_input_context->read_inputs(controller_id);

    if (inputs.right) ret |= 0b00000001;
    if (inputs.left) ret |= 0b00000010;
    if (inputs.down) ret |= 0b00000100;
    if (inputs.up) ret |= 0b00001000;
    if (inputs.start) ret |= 0b00010000;
    if (inputs.select) ret |= 0b00100000;
    if (inputs.b) ret |= 0b01000000;
    if (inputs.a) ret |= 0b10000000;

    return ret;
}


void Controller::serialize(FILE* f, int version) const
{
    fwrite(m_shift_registers, 1, 2, f);
}


void Controller::deserialize(FILE* f, int version)
{
    fread(m_shift_registers, 1, 2, f);
}


bool Controller::cpu_write(uint16_t addr, uint8_t data)
{
    if (addr < 0x4016 || addr > 0x4017) return false;

    switch (addr)
    {
        case 0x4016:
            if (data == 0x01)
            {
                m_shift_registers[0] = read_inputs(0);
            }
            break;
        case 0x4017:
            if (data == 0x01)
            {
                m_shift_registers[0] = read_inputs(1);
            }
            break;
    }

    return true;
}


bool Controller::cpu_read(uint16_t addr, uint8_t* out_data)
{
    if (addr < 0x4016 || addr > 0x4017) return false;

    switch (addr)
    {
        case 0x4016:
            *out_data = (m_shift_registers[0] & 0x80) ? 1 : 0;
            m_shift_registers[0] <<= 1;
            break;
        case 0x4017:
            *out_data = (m_shift_registers[1] & 0x80) ? 1 : 0;
            m_shift_registers[1] <<= 1;
            break;
    }

    return true;
}


void Controller::set_input_context(InputContext* input_context)
{
    m_input_context = input_context;
}
