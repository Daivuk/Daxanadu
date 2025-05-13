#include "APU.h"

#include <onut/AudioEngine.h>


static const int CPU_CLOCK_SPEED = 1789773;


APU::APU()
{
    m_audio_stream = OMake<APUAudioStream>();
    oAudioEngine->addInstance(m_audio_stream);
}


void APU::serialize(FILE* f, int version) const
{
    m_audio_stream->serialize(f, version);
}


void APU::deserialize(FILE* f, int version)
{
    m_audio_stream->deserialize(f, version);
}


bool APU::cpu_write(uint16_t addr, uint8_t data)
{
    if ((addr >= 0x4000 && addr <= 0x4017) && addr != 0x4016)
    {
        m_audio_stream->cpu_write(addr, data);
        return true;
    }

    return false;
}


bool APU::cpu_read(uint16_t addr, uint8_t* out_data)
{
    if ((addr >= 0x4000 && addr <= 0x4017) && addr != 0x4016)
    {
        *out_data = m_audio_stream->cpu_read(addr);
        return true;
    }

    return false;
}


APUAudioStream::APUAudioStream()
{
    m_60hz_rate = (1000000.0 / (double)16666) / (double)CPU_CLOCK_SPEED;
    m_120hz_rate = m_60hz_rate * 2.0;
    m_240hz_rate = m_120hz_rate * 2.0;
}


static const int LENGTH_COUNTER_TABLE[0x1F + 1] = {
    10, 254, 20, 2,
    40, 4, 80, 6,
    160, 8, 60, 10,
    14, 12, 26, 14,
    12, 16, 24, 18,
    48, 20, 96, 22,
    192, 24, 72, 26,
    16, 28, 32, 30
};

static const double NOISE_FREQUENCY_TABLE[16] = {
    4, 8, 16, 32, 
    64, 96, 128, 160, 
    202, 254, 380, 508, 
    762, 1016, 2034, 4068
};


void APUAudioStream::serialize(FILE* f, int version)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    for (int i = 0; i < 2; ++i)
    {
        auto pulse = &m_pulses[i];
        
        fwrite(&pulse->enabled, sizeof(pulse->enabled), 1, f);
        fwrite(&pulse->loop, sizeof(pulse->loop), 1, f);
        fwrite(&pulse->constant_volume, sizeof(pulse->constant_volume), 1, f);
        fwrite(&pulse->duty, sizeof(pulse->duty), 1, f);
        fwrite(&pulse->progress, sizeof(pulse->progress), 1, f);
        fwrite(&pulse->frequency, sizeof(pulse->frequency), 1, f);
        fwrite(&pulse->volume, sizeof(pulse->volume), 1, f);
        fwrite(pulse->registers, 1, 4, f);
        fwrite(&pulse->length_counter, sizeof(pulse->length_counter), 1, f);
        fwrite(&pulse->timer, sizeof(pulse->timer), 1, f);
        fwrite(&pulse->time, sizeof(pulse->time), 1, f);
        fwrite(&pulse->envelope_counter, sizeof(pulse->envelope_counter), 1, f);
        fwrite(&pulse->envelope_rate, sizeof(pulse->envelope_rate), 1, f);
        fwrite(&pulse->envelope_progress, sizeof(pulse->envelope_progress), 1, f);
    }

    fwrite(&m_triangle.enabled, sizeof(m_triangle.enabled), 1, f);
    fwrite(&m_triangle.reload_flag, sizeof(m_triangle.reload_flag), 1, f);
    fwrite(&m_triangle.progress, sizeof(m_triangle.progress), 1, f);
    fwrite(&m_triangle.frequency, sizeof(m_triangle.frequency), 1, f);
    fwrite(m_triangle.registers, 1, 4, f);
    fwrite(&m_triangle.length_counter, sizeof(m_triangle.length_counter), 1, f);
    fwrite(&m_triangle.control_flag, sizeof(m_triangle.control_flag), 1, f);
    fwrite(&m_triangle.linear_counter, sizeof(m_triangle.linear_counter), 1, f);
    fwrite(&m_triangle.timer, sizeof(m_triangle.timer), 1, f);
    fwrite(&m_triangle.time, sizeof(m_triangle.time), 1, f);
    fwrite(&m_triangle.reload_value, sizeof(m_triangle.reload_value), 1, f);

    fwrite(&m_noise.enabled, sizeof(m_noise.enabled), 1, f);
    fwrite(&m_noise.loop, sizeof(m_noise.loop), 1, f);
    fwrite(&m_noise.mode, sizeof(m_noise.mode), 1, f);
    fwrite(&m_noise.constant_volume, sizeof(m_noise.constant_volume), 1, f);
    fwrite(&m_noise.progress, sizeof(m_noise.progress), 1, f);
    fwrite(&m_noise.frequency, sizeof(m_noise.frequency), 1, f);
    fwrite(&m_noise.volume, sizeof(m_noise.volume), 1, f);
    fwrite(m_noise.registers, 1, 4, f);
    fwrite(&m_noise.length_counter, sizeof(m_noise.length_counter), 1, f);
    fwrite(&m_noise.period, sizeof(m_noise.period), 1, f);
    fwrite(&m_noise.time, sizeof(m_noise.time), 1, f);
    fwrite(&m_noise.envelope_counter, sizeof(m_noise.envelope_counter), 1, f);
    fwrite(&m_noise.envelope_rate, sizeof(m_noise.envelope_rate), 1, f);
    fwrite(&m_noise.envelope_progress, sizeof(m_noise.envelope_progress), 1, f);
    fwrite(&m_noise.previous_sample, sizeof(m_noise.previous_sample), 1, f);
    fwrite(&m_noise.shift_register, sizeof(m_noise.shift_register), 1, f);

    fwrite(&m_dmc.enabled, sizeof(m_dmc.enabled), 1, f);
    fwrite(&m_dmc.loop, sizeof(m_dmc.loop), 1, f);
    fwrite(m_dmc.registers, 1, 4, f);

    fwrite(&m_status_register, sizeof(m_status_register), 1, f);
    fwrite(&m_frame_counter_register, sizeof(m_frame_counter_register), 1, f);
    fwrite(&m_volume, sizeof(m_volume), 1, f);
    fwrite(&m_cpu_progress, sizeof(m_cpu_progress), 1, f);
    fwrite(&m_previous_filtered_sample, sizeof(m_previous_filtered_sample), 1, f);
    fwrite(&m_previous_unfiltered_sample, sizeof(m_previous_unfiltered_sample), 1, f);
}


void APUAudioStream::deserialize(FILE* f, int version)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    for (int i = 0; i < 2; ++i)
    {
        auto pulse = &m_pulses[i];
        
        fread(&pulse->enabled, sizeof(pulse->enabled), 1, f);
        fread(&pulse->loop, sizeof(pulse->loop), 1, f);
        fread(&pulse->constant_volume, sizeof(pulse->constant_volume), 1, f);
        fread(&pulse->duty, sizeof(pulse->duty), 1, f);
        fread(&pulse->progress, sizeof(pulse->progress), 1, f);
        fread(&pulse->frequency, sizeof(pulse->frequency), 1, f);
        fread(&pulse->volume, sizeof(pulse->volume), 1, f);
        fread(pulse->registers, 1, 4, f);
        fread(&pulse->length_counter, sizeof(pulse->length_counter), 1, f);
        fread(&pulse->timer, sizeof(pulse->timer), 1, f);
        fread(&pulse->time, sizeof(pulse->time), 1, f);
        fread(&pulse->envelope_counter, sizeof(pulse->envelope_counter), 1, f);
        fread(&pulse->envelope_rate, sizeof(pulse->envelope_rate), 1, f);
        fread(&pulse->envelope_progress, sizeof(pulse->envelope_progress), 1, f);
    }

    fread(&m_triangle.enabled, sizeof(m_triangle.enabled), 1, f);
    fread(&m_triangle.reload_flag, sizeof(m_triangle.reload_flag), 1, f);
    fread(&m_triangle.progress, sizeof(m_triangle.progress), 1, f);
    fread(&m_triangle.frequency, sizeof(m_triangle.frequency), 1, f);
    fread(m_triangle.registers, 1, 4, f);
    fread(&m_triangle.length_counter, sizeof(m_triangle.length_counter), 1, f);
    fread(&m_triangle.control_flag, sizeof(m_triangle.control_flag), 1, f);
    fread(&m_triangle.linear_counter, sizeof(m_triangle.linear_counter), 1, f);
    fread(&m_triangle.timer, sizeof(m_triangle.timer), 1, f);
    fread(&m_triangle.time, sizeof(m_triangle.time), 1, f);
    fread(&m_triangle.reload_value, sizeof(m_triangle.reload_value), 1, f);

    fread(&m_noise.enabled, sizeof(m_noise.enabled), 1, f);
    fread(&m_noise.loop, sizeof(m_noise.loop), 1, f);
    fread(&m_noise.mode, sizeof(m_noise.mode), 1, f);
    fread(&m_noise.constant_volume, sizeof(m_noise.constant_volume), 1, f);
    fread(&m_noise.progress, sizeof(m_noise.progress), 1, f);
    fread(&m_noise.frequency, sizeof(m_noise.frequency), 1, f);
    fread(&m_noise.volume, sizeof(m_noise.volume), 1, f);
    fread(m_noise.registers, 1, 4, f);
    fread(&m_noise.length_counter, sizeof(m_noise.length_counter), 1, f);
    fread(&m_noise.period, sizeof(m_noise.period), 1, f);
    fread(&m_noise.time, sizeof(m_noise.time), 1, f);
    fread(&m_noise.envelope_counter, sizeof(m_noise.envelope_counter), 1, f);
    fread(&m_noise.envelope_rate, sizeof(m_noise.envelope_rate), 1, f);
    fread(&m_noise.envelope_progress, sizeof(m_noise.envelope_progress), 1, f);
    fread(&m_noise.previous_sample, sizeof(m_noise.previous_sample), 1, f);
    fread(&m_noise.shift_register, sizeof(m_noise.shift_register), 1, f);

    fread(&m_dmc.enabled, sizeof(m_dmc.enabled), 1, f);
    fread(&m_dmc.loop, sizeof(m_dmc.loop), 1, f);
    fread(m_dmc.registers, 1, 4, f);

    fread(&m_status_register, sizeof(m_status_register), 1, f);
    fread(&m_frame_counter_register, sizeof(m_frame_counter_register), 1, f);
    fread(&m_volume, sizeof(m_volume), 1, f);
    fread(&m_cpu_progress, sizeof(m_cpu_progress), 1, f);
    fread(&m_previous_filtered_sample, sizeof(m_previous_filtered_sample), 1, f);
    fread(&m_previous_unfiltered_sample, sizeof(m_previous_unfiltered_sample), 1, f);
}


void APUAudioStream::cpu_write(int addr, uint8_t val)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    if (addr == 0x4000 || addr == 0x4004) // Pulse reg 0
    {
        auto pulse = &m_pulses[(addr - 0x4000) / 4];
        pulse->registers[0] = val;

        int duty = (val >> 6) & 0b11;
        int loop = (val & 0b100000) ? 1 : 0;
        int constant_volume = (val & 0b10000) ? 1 : 0;
        int volume = (val & 0b1111);

        pulse->constant_volume = constant_volume ? true : false;
        pulse->duty = duty;
        pulse->loop = loop ? true : false;
        if (constant_volume)
        {
            pulse->volume = (float)volume / 15.0f;
            pulse->envelope_counter = 0;
            pulse->envelope_rate = 0.0;
            pulse->envelope_progress = 0.0;
        }
        else
        {
            pulse->volume = 1.0f;
            pulse->envelope_counter = 15;
            pulse->envelope_rate = 1.0 / (double)(volume + 1);
            pulse->envelope_progress = 0.0;
        }
    }
    else if (addr == 0x4001 || addr == 0x4005) // Pulse reg 1
    {
        auto pulse = &m_pulses[(addr - 0x4001) / 4];
        pulse->registers[1] = val;

        int sweep = (val & 0b10000000) ? 1 : 0;
        int period = (val >> 4) & 0b111;
        int negate = (val & 0b1000) ? 1 : 0;
        int shift = val & 0b111;
    }
    else if (addr == 0x4002 || addr == 0x4006) // Pulse reg 2
    {
        auto pulse = &m_pulses[(addr - 0x4002) / 4];
        pulse->registers[2] = val;

        int timer_low = val;

        pulse->timer = timer_low;
    }
    else if (addr == 0x4003 || addr == 0x4007) // Pulse reg 3
    {
        auto pulse = &m_pulses[(addr - 0x4003) / 4];
        pulse->registers[3] = val;

        int length_counter = (val >> 3) & 0b11111;
        int timer_high = val & 0b111;

        pulse->length_counter = LENGTH_COUNTER_TABLE[length_counter];
        pulse->timer = pulse->timer | (timer_high << 8);
        pulse->progress = 0.0;
        pulse->time = 0;
        pulse->frequency = (double)CPU_CLOCK_SPEED / (16.0 * ((double)pulse->timer + 1.0));
        
        if (pulse->enabled && !pulse->constant_volume)
        {
            pulse->envelope_counter = 15;
            pulse->envelope_rate = 1.0 / (double)(pulse->volume * 15.0f + 1);
            pulse->envelope_progress = 0.0;
        }
    }
    else if (addr == 0x4008) // Triangle reg 0
    {
        m_triangle.registers[0] = val;

        int control_flag = (val & 0b10000000) ? 1 : 0;
        int counter_reload = val & 0b01111111;

        m_triangle.control_flag = control_flag ? true : false;
        m_triangle.reload_value = counter_reload + 1;
    }
    else if (addr == 0x4009) // Triangle reg 1
    {
        m_triangle.registers[1] = val;
    }
    else if (addr == 0x400A) // Triangle reg 2
    {
        m_triangle.registers[2] = val;

        int timer_low = val;

        m_triangle.timer = timer_low;
    }
    else if (addr == 0x400B) // Triangle reg 3
    {
        m_triangle.registers[3] = val;

        int length_counter = (val >> 3) & 0b11111;
        int timer_high = val & 0b111;

        m_triangle.length_counter = LENGTH_COUNTER_TABLE[length_counter];
        m_triangle.timer = m_triangle.timer | (timer_high << 8);
        m_triangle.frequency = (double)CPU_CLOCK_SPEED / (32.0 * ((double)m_triangle.timer + 1.0));
        m_triangle.reload_flag = true;
    }
    else if (addr == 0x400C) // Noise reg 0
    {
        m_noise.registers[0] = val;

        int loop = (val & 0b100000) ? 1 : 0;
        int constant_volume = (val & 0b10000) ? 1 : 0;
        int volume = (val & 0b1111);

        m_noise.constant_volume = constant_volume ? true : false;
        m_noise.loop = loop ? true : false;
        if (constant_volume)
        {
            m_noise.volume = (float)volume / 15.0f;
            m_noise.envelope_counter = 0;
            m_noise.envelope_rate = 0.0;
            m_noise.envelope_progress = 0.0;
        }
        else
        {
            m_noise.volume = (float)volume / 15.0f;
            m_noise.envelope_counter = 15;
            m_noise.envelope_rate = 1.0 / (double)(volume + 1);
            m_noise.envelope_progress = 0.0;
        }
    }
    else if (addr == 0x400D) // Noise reg 1
    {
        m_noise.registers[1] = val;
    }
    else if (addr == 0x400E) // Noise reg 2
    {
        m_noise.registers[2] = val;

        int mode = (val & 0b10000000) ? 1 : 0;
        int period = val & 0b1111;

        m_noise.frequency = (double)CPU_CLOCK_SPEED / NOISE_FREQUENCY_TABLE[period];
        m_noise.mode = mode ? true : false;
    }
    else if (addr == 0x400F) // Noise reg 3
    {
        m_noise.registers[3] = val;

        int length_counter = (val >> 3) & 0b11111;

        m_noise.length_counter = LENGTH_COUNTER_TABLE[length_counter];
        if (m_noise.enabled && !m_noise.constant_volume)
        {
            m_noise.envelope_counter = 15;
            m_noise.envelope_rate = 1.0 / (double)(m_noise.volume * 15.0f + 1);
            m_noise.envelope_progress = 0.0;
        }
    }
    else if (addr == 0x4010) // DCM reg 0
    {
        m_dmc.registers[0] = val;
    }
    else if (addr == 0x4011) // DCM reg 1
    {
        m_dmc.registers[1] = val;
    }
    else if (addr == 0x4012) // DCM reg 2
    {
        m_dmc.registers[2] = val;
    }
    else if (addr == 0x4013) // DCM reg 3
    {
        m_dmc.registers[3] = val;
    }
    else if (addr == 0x4015)
    {
        m_status_register = val;

        m_pulses[0].enabled = (val & 0b1) != 0;
        m_pulses[1].enabled = (val & 0b10) != 0;
        m_triangle.enabled = (val & 0b100) != 0;
        m_noise.enabled = (val & 0b1000) != 0;
        m_dmc.enabled = (val & 0b10000) != 0;

        if (!m_pulses[0].enabled) m_pulses[0].length_counter = 0;
        if (!m_pulses[1].enabled) m_pulses[1].length_counter = 0;
        if (!m_triangle.enabled) m_triangle.length_counter = 0;
        if (!m_noise.enabled) m_noise.length_counter = 0;
    }
    else if (addr == 0x4017)
    {
        m_frame_counter_register = val;
    }
}


uint8_t APUAudioStream::cpu_read(int addr)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    if (addr == 0x4000 || addr == 0x4004)
    {
        auto pulse = &m_pulses[(addr - 0x4000) / 4];
        return pulse->registers[0];
    }
    else if (addr == 0x4001 || addr == 0x4005)
    {
        auto pulse = &m_pulses[(addr - 0x4000) / 4];
        return pulse->registers[1];
    }
    else if (addr == 0x4002 || addr == 0x4006)
    {
        auto pulse = &m_pulses[(addr - 0x4000) / 4];
        return pulse->registers[2];
    }
    else if (addr == 0x4003 || addr == 0x4007)
    {
        auto pulse = &m_pulses[(addr - 0x4000) / 4];
        return pulse->registers[3];
    }
    else if (addr == 0x4008)
    {
        return m_triangle.registers[0];
    }
    else if (addr == 0x4009)
    {
        return m_triangle.registers[1];
    }
    else if (addr == 0x400A)
    {
        return m_triangle.registers[2];
    }
    else if (addr == 0x400B)
    {
        return m_triangle.registers[3];
    }
    else if (addr == 0x400C)
    {
        return m_noise.registers[0];
    }
    else if (addr == 0x400D)
    {
        return m_noise.registers[1];
    }
    else if (addr == 0x400E)
    {
        return m_noise.registers[2];
    }
    else if (addr == 0x400F)
    {
        return m_noise.registers[3];
    }
    else if (addr == 0x4010)
    {
        return m_dmc.registers[0];
    }
    else if (addr == 0x4011)
    {
        return m_dmc.registers[1];
    }
    else if (addr == 0x4012)
    {
        return m_dmc.registers[2];
    }
    else if (addr == 0x4013)
    {
        return m_dmc.registers[3];
    }
    else if (addr == 0x4015)
    {
        return m_status_register;
    }
    else if (addr == 0x4017)
    {
        return m_frame_counter_register;
    }

    return 0;
}


void APUAudioStream::emulate()
{
    m_240hz_progress += m_240hz_rate;
    while (m_240hz_progress >= 1.0)
    {
        m_240hz_progress -= 1.0;

        // Envelopes on pulses
        for (int i = 0; i < 2; ++i)
        {
            auto pulse = &m_pulses[i];

            if (!pulse->constant_volume)
            {
                if (pulse->envelope_counter > 0 || pulse->loop)
                {
                    pulse->envelope_progress += pulse->envelope_rate;
                    while (pulse->envelope_progress >= 1.0)
                    {
                        pulse->envelope_progress -= 1.0;
                        pulse->envelope_counter--;
                        pulse->volume = (float)pulse->envelope_counter / 15.0f;
                        if (pulse->envelope_counter == 0 && pulse->loop)
                        {
                            pulse->envelope_counter = 15;
                            pulse->volume = 1.0f;
                        }
                    }
                }
            }
        }

        // Envelope on noise
        if (!m_noise.constant_volume)
        {
            if (m_noise.envelope_counter > 0 || m_noise.loop)
            {
                m_noise.envelope_progress += m_noise.envelope_rate;
                while (m_noise.envelope_progress >= 1.0)
                {
                    m_noise.envelope_progress -= 1.0;
                    m_noise.envelope_counter--;
                    m_noise.volume = (float)m_noise.envelope_counter / 15.0f;
                    if (m_noise.envelope_counter == 0 && m_noise.loop)
                    {
                        m_noise.envelope_counter = 15;
                        m_noise.volume = 1.0f;
                    }
                }
            }
        }
    }

    m_120hz_progress += m_120hz_rate;
    while (m_120hz_progress >= 1.0)
    {
        m_120hz_progress -= 1.0;

        // Pulses
        for (int i = 0; i < 2; ++i)
        {
            auto pulse = &m_pulses[i];

            if (pulse->length_counter > 0 && !pulse->loop)
                pulse->length_counter--;
        }

        // Triangle
        if (m_triangle.length_counter > 0 && !m_triangle.control_flag)
            m_triangle.length_counter--;

        if (m_triangle.reload_flag)
        {
            m_triangle.linear_counter = m_triangle.reload_value;
        }
        else if (m_triangle.linear_counter > 0)
        {
            m_triangle.linear_counter--;
        }
        if (!m_triangle.control_flag)
        {
            m_triangle.reload_flag = false;
        }

        // Noise
        if (m_noise.length_counter > 0 && !m_noise.loop)
            m_noise.length_counter--;
    }

    // Call "NMI" every frame
    m_60hz_progress += m_60hz_rate;
    while (m_60hz_progress >= 1.0)
    {
        m_60hz_progress -= 1.0;
    }
}


bool APUAudioStream::progress(int frame_count, int sample_rate, int channel_count, float* out, float volume, float balance, float pitch)
{
    const double cpu_progress_speed = (double)CPU_CLOCK_SPEED / (double)sample_rate;

    for (int i = 0; i < frame_count; ++i)
    {
        m_cpu_progress += cpu_progress_speed;
        while (m_cpu_progress >= 1.0)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            emulate();
            m_cpu_progress--;
        }

        float sample = render_frame(sample_rate);
        for (int c = 0; c < channel_count; ++c)
            out[i * channel_count + c] = sample;
    }

    return true;
}


static const int TRIANGLE_WAVE_DATA[32] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
};

static const int PULSE_WAVE_DATA[4][8] = {
    {0, 1, 0, 0, 0, 0, 0, 0},
    {0, 1, 1, 0, 0, 0, 0, 0},
    {0, 1, 1, 1, 1, 0, 0, 0},
    {1, 0, 0, 1, 1, 1, 1, 1}
};


float APUAudioStream::render_frame(int sample_rate)
{
    float pulse_samples[2] = { 0.0f, 0.0f };
    float triangle_sample = 0.0;
    float noise_sample = 0.0;
    float dmc_sample = 0.0;

    // Pulses
    for (int i = 0; i < 2; ++i)
    {
        auto pulse = &m_pulses[i];
        if (pulse->length_counter > 0 && pulse->timer >= 8)
        {
            float volume = pulse->volume;
            pulse_samples[i] = (float)PULSE_WAVE_DATA[pulse->duty][pulse->time] * volume;
            double pulse_rate = pulse->frequency / (double)sample_rate;
            pulse->progress += pulse_rate * 8.0f;
            while (pulse->progress >= 1.0)
            {
                pulse->progress--;
                pulse->time++;
                if (pulse->time == 8)
                    pulse->time = 0;
            }
        }
    }

    // Triangle
    if ((m_triangle.enabled && m_triangle.length_counter > 0 && m_triangle.linear_counter > 0 && m_triangle.timer > 0) || m_triangle.time != 0)
    {
        if ((m_triangle.length_counter > 0 && m_triangle.linear_counter > 0) || m_triangle.time != 0)
        {
            triangle_sample = (float)TRIANGLE_WAVE_DATA[m_triangle.time] / 15.0f;
        }
        double triangle_rate = m_triangle.frequency / (double)sample_rate;
        m_triangle.progress += triangle_rate * 32.0f;
        while (m_triangle.progress >= 1.0)
        {
            m_triangle.progress--;
            m_triangle.time++;
            if (m_triangle.time == 32)
                m_triangle.time = 0;
        }
    }

    // Noise
    if (m_noise.enabled && m_noise.length_counter > 0)
    {
        noise_sample = m_noise.previous_sample;
        double noise_rate = m_noise.frequency / (double)sample_rate;
        m_noise.progress += noise_rate;
        while (m_noise.progress >= 1.0)
        {
            m_noise.progress--;

            int bit_a = m_noise.shift_register & 0b1;
            int bit_b = (m_noise.shift_register >> 1) & 0b1;
            if (m_noise.mode)
                bit_b = (m_noise.shift_register >> 6) & 0b1;
            int feedback = bit_a ^ bit_b;
            m_noise.shift_register >>= 1;
            m_noise.shift_register = (m_noise.shift_register & 0b11111111111111) | (feedback << 14);

            noise_sample = (float)(!(m_noise.shift_register & 0b1)) * m_noise.volume;
            m_noise.previous_sample = noise_sample;
        }
        noise_sample *= m_noise.volume * 1.0f; // Why is this too loud?
    }

    // Mixing
    float sample = 0.0f;

    // DACs
    float pulse_group = pulse_samples[0] * 15.0f + pulse_samples[1] * 15.0f;
    if (pulse_group > 0.0f)
        sample += 95.88f / ((8128.0f / pulse_group) + 100.0f);

    float tnd_group = (triangle_sample * 15.0f / 8227.0f) + (noise_sample * 15.0f / 12241.0f) + (dmc_sample * 127.0f / 22638.0f);
    if (tnd_group > 0.0f)
        sample += 159.79f / (1.0f / tnd_group + 100.0f);

    // Filtering (I don't fully understand this)
    float filtered_sample = sample;
    filtered_sample = m_previous_filtered_sample * 0.999835f + filtered_sample - m_previous_unfiltered_sample;
    filtered_sample = m_previous_filtered_sample * 0.996039f + filtered_sample - m_previous_unfiltered_sample;
    filtered_sample = (filtered_sample - m_previous_filtered_sample) * 0.815686f;
    filtered_sample *= 0.70f; // Somehow this makes it too loud

    m_previous_filtered_sample = filtered_sample;
    m_previous_unfiltered_sample = sample;

    return filtered_sample * m_volume;
}


void APU::set_volume(float volume)
{
    m_audio_stream->set_volume(volume);
}


float APU::get_volume() const
{
    return m_audio_stream->get_volume();
}


float APUAudioStream::get_volume()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_volume;
}


void APUAudioStream::set_volume(float volume)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    m_volume = volume;
}
