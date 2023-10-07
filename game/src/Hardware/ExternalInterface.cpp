#include "ExternalInterface.h"


void ExternalInterface::serialize(FILE* f, int version) const
{
    fwrite(&m_id_to_call, 1, 1, f);
    fwrite(&m_arg_accum, 1, 1, f);
    fwrite(&m_arg_count, 1, 1, f);
    fwrite(m_args, 1, 4, f);
    fwrite(&m_return_value, 1, 1, f);
}


void ExternalInterface::deserialize(FILE* f, int version)
{
    if (version < 2) return;

    fread(&m_id_to_call, 1, 1, f);
    fread(&m_arg_accum, 1, 1, f);
    fread(&m_arg_count, 1, 1, f);
    fread(m_args, 1, 4, f);
    fread(&m_return_value, 1, 1, f);
}


void ExternalInterface::register_callback(uint8_t id, const std::function<uint8_t(uint8_t, uint8_t, uint8_t, uint8_t)>& callback, int arg_count)
{
    m_callbacks[id] = { arg_count, callback };
}


bool ExternalInterface::cpu_write(uint16_t addr, uint8_t data)
{
    if (addr != 0x6000) return false;

    if (m_id_to_call == 0)
    {
        auto& callback = m_callbacks[data];
        if (callback.fn)
        {
            m_id_to_call = data;
            m_arg_accum = 0;
            if (callback.arg_count == 0)
            {
                // Call it!
                m_id_to_call = 0;
                m_arg_accum = 0;
                m_return_value = callback.fn(0, 0, 0, 0);
            }
        }
    }
    else
    {
        const auto& callback = m_callbacks[m_id_to_call];
        m_args[m_arg_accum++] = data;
        if (m_arg_accum == callback.arg_count)
        {
            m_id_to_call = 0;
            m_arg_accum = 0;
            m_return_value = callback.fn(m_args[0], m_args[1], m_args[2], m_args[3]);
        }
    }

    return true;
}


bool ExternalInterface::cpu_read(uint16_t addr, uint8_t* out_data)
{
    if (addr != 0x6000) return false;

    *out_data = m_return_value;

    return true;
}
