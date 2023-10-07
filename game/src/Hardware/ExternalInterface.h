#pragma once

#include "CPUPeripheral.h"

#include <stdio.h>
#include <functional>


class ExternalInterface final : public CPUPeripheral
{
public:
    void serialize(FILE* f, int version) const;
    void deserialize(FILE* f, int version);

    bool cpu_write(uint16_t addr, uint8_t data) override;
    bool cpu_read(uint16_t addr, uint8_t* out_data) override;

    void register_callback(uint8_t id, const std::function<uint8_t(uint8_t, uint8_t, uint8_t, uint8_t)>& callback, int arg_count = 0);

private:
    struct callback_t
    {
        int arg_count = 0;
        std::function<uint8_t(uint8_t, uint8_t, uint8_t, uint8_t)> fn;
    };

    callback_t m_callbacks[256];
    uint8_t m_id_to_call = 0;
    uint8_t m_arg_accum = 0;
    uint8_t m_arg_count = 0;
    uint8_t m_args[4];
    uint8_t m_return_value = 0;
};
