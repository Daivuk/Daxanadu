#include "apu.h"
#include "instructions_6502.h"
#include <onut/Files.h>
#include <onut/Json.h>
#include <onut/Random.h>


static const int BANK_OFFSET = 0x00014010;
static const int LOAD_ADDR = 0x8000;
static const int INIT_ADDR = 0xB3D8 - LOAD_ADDR + BANK_OFFSET;
static const int PLAY_ADDR = 0xB3D1 - LOAD_ADDR + BANK_OFFSET;
static const int CPU_CLOCK_SPEED = 1789773;


uint16_t read_word(uint8_t* ptr)
{
    uint16_t addr = (uint16_t)ptr[0];
    addr |= (uint16_t)ptr[1] << 8;
    return addr;
}


uint8 MCS6502_read(uint16 addr, void * readWriteContext)
{
    return ((APUAudioStream*)readWriteContext)->cpu_read((int)addr);
}


void MCS6502_write(uint16 addr, uint8 byte, void * readWriteContext)
{
    ((APUAudioStream*)readWriteContext)->cpu_write((int)addr, byte);
}


APUAudioStream::APUAudioStream(uint8_t* rom, size_t size)
{
    MCS6502Init(&m_cpu_context, MCS6502_read, MCS6502_write, this);

    m_nsf_load_addr = 0x8000;
    m_nsf_play_addr = 0xB3D1;
    m_nsf_init_addr = 0xB3D8;
    m_nsf_play_speed = 16666;

    memset(m_ram, 0, sizeof(m_ram));
    memcpy(m_ram + m_nsf_load_addr, rom + BANK_OFFSET, 0x4000);

    m_60hz_rate = (1000000.0 / (double)m_nsf_play_speed) / (double)CPU_CLOCK_SPEED;
    m_120hz_rate = m_60hz_rate * 2.0;
    m_240hz_rate = m_120hz_rate * 2.0;

    // Patch in the init and play code from NSF
    uint8_t nsf_code[] = {
        // play:
            0x20, 0x09, 0x80, // jsr $8009
            0x20, 0x03, 0x80, // jsr $8003
            0x60, // rts
            
        // init: // a = 0 (First song), x = 0 (NTSC)
            0xC9, 0x10, // cpm $10
            0x90, 0x06, // bcc song_id_less_than_16
            0x38, // sec
            0xE9, 0x0F, // sbc $0F
            0x85, 0xFB, // sta $FB
            0x60, // rts
            
        // song_id_less_than_16:
            0xAA, // tax
            0xBD, 0xE9, 0xB3, // lda table, x
            0x85, 0xFA, // sta $FA
            0x60, // rts

        // table:
            0x01, 0x08, 0x07, 0x0E, 0x0D, 0x0F, 0x03, 0x09, 0x06, 0x0A, 0x05, 0x0B, 0x04, 0x02, 0x10, 0x0C // .db
    };
    int nsf_init_addr = m_nsf_play_addr;
    memcpy(m_ram + nsf_init_addr, nsf_code, sizeof(nsf_code));

    // Init code
    uint8_t init_code[] = {
        // Initialize the sound registers
        0xA9, 0x00, // lda $00
        0x8D, 0x00, 0x40, // sta $4000
        0x8D, 0x01, 0x40, // sta $4001
        0x8D, 0x02, 0x40, // sta $4002
        0x8D, 0x03, 0x40, // sta $4003
        0x8D, 0x04, 0x40, // sta $4004
        0x8D, 0x05, 0x40, // sta $4005
        0x8D, 0x06, 0x40, // sta $4006
        0x8D, 0x07, 0x40, // sta $4007
        0x8D, 0x08, 0x40, // sta $4008
        0x8D, 0x09, 0x40, // sta $4009
        0x8D, 0x0A, 0x40, // sta $400A
        0x8D, 0x0B, 0x40, // sta $400B
        0x8D, 0x0C, 0x40, // sta $400C
        0x8D, 0x0D, 0x40, // sta $400D
        0x8D, 0x0E, 0x40, // sta $400E
        0x8D, 0x0F, 0x40, // sta $400F
        0x8D, 0x10, 0x40, // sta $4010
        0x8D, 0x11, 0x40, // sta $4011
        0x8D, 0x12, 0x40, // sta $4012
        0x8D, 0x13, 0x40, // sta $4013
        0x8D, 0x15, 0x40, // sta $4015
        0xA9, 0x0F, // lda $0F
        0x8D, 0x15, 0x40, // sta $4015

        // Initialize the frame counter to 4-step mode
        0xA9, 0x40, // lda $40
        0x8D, 0x17, 0x40, // sta $4017

        // Call init(song 1, NTSC)
        0xA9, 0x00, // lda $00
        0xA2, 0x00, // ldx $00
        0x20, (uint8_t)(m_nsf_init_addr), (uint8_t)(m_nsf_init_addr >> 8), // jsr init

        // Infinite loop
        0x4C, 0000, 0000
    };
    int init_addr = nsf_init_addr + sizeof(nsf_code);
    m_init_infinite_loop_addr = init_addr + (int)sizeof(init_code) - 3;
    init_code[sizeof(init_code) - 2] = (uint8_t)(m_init_infinite_loop_addr);
    init_code[sizeof(init_code) - 1] = (uint8_t)(m_init_infinite_loop_addr >> 8);
    memcpy(m_ram + init_addr, init_code, sizeof(init_code));

    m_init_addr = init_addr;
    m_music_id_addr = init_addr + sizeof(init_code) - 9;

    uint8_t play_code[] = {
        // Call play()
        0x20, (uint8_t)(m_nsf_play_addr), (uint8_t)(m_nsf_play_addr >> 8), // jsr play

        // Infinite loop
        0x4C, 0000, 0000
    };
    int play_addr = init_addr + sizeof(init_code);
    m_play_infinite_loop_addr = play_addr + (int)sizeof(play_code) - 3;
    play_code[sizeof(play_code) - 2] = (uint8_t)(m_play_infinite_loop_addr);
    play_code[sizeof(play_code) - 1] = (uint8_t)(m_play_infinite_loop_addr >> 8);
    memcpy(m_ram + play_addr, play_code, sizeof(play_code));

    m_play_addr = play_addr;
}


void APUAudioStream::play(int music_id)
{
    if (!m_suspended && m_music_id == music_id) return;
    stop();
    while (m_running_cpu) {}

    m_music_id = music_id;

    // Set our reset vector to the init function then run the CPU for a bit so we initialize correctly
    m_ram[m_music_id_addr] = music_id;
    m_ram[0xFFFC] = (uint8_t)m_init_addr;
    m_ram[0xFFFD] = (uint8_t)(m_init_addr >> 8);
    MCS6502Reset(&m_cpu_context);
    while (m_cpu_context.pc != m_init_infinite_loop_addr)
        MCS6502ExecNext(&m_cpu_context);

    // Set the reset addr to our play function, this is what will be used next time we reset the CPU
    m_ram[0xFFFC] = (uint8_t)m_play_addr;
    m_ram[0xFFFD] = (uint8_t)(m_play_addr >> 8);
    MCS6502Reset(&m_cpu_context);

    m_60hz_progress = 0.0;
    m_120hz_progress = 0.0;
    m_240hz_progress = 0.0;

    m_suspended = false;
}


void APUAudioStream::stop()
{
    m_suspended = true;
}


int APUAudioStream::playing_music() const
{
    if (m_suspended) return -1;
    return m_music_id;
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


void APUAudioStream::cpu_write(int addr, uint8_t val)
{
    if (addr < 0x0800)
    {
        // Ram
        m_ram[addr] = val;
    }
    else if (addr == 0x4000 || addr == 0x4004) // Pulse reg 0
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

    return m_ram[addr];
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
        if ((int)m_cpu_context.pc != m_play_infinite_loop_addr)
        {
            break; // Not ready yet
        }

        // Reset to the play function
        MCS6502Reset(&m_cpu_context);
    }

    MCS6502Tick(&m_cpu_context);
}


bool APUAudioStream::progress(int frame_count, int sample_rate, int channel_count, float* out, float volume, float balance, float pitch)
{
    if (m_suspended)
    {
        memset(out, 0, sizeof(float) * frame_count * channel_count);
        return true;
    }

    m_running_cpu = true;

    const double cpu_progress_speed = (double)CPU_CLOCK_SPEED / (double)sample_rate;

    for (int i = 0; i < frame_count; ++i)
    {
        m_cpu_progress += cpu_progress_speed;
        while (m_cpu_progress >= 1.0)
        {
            emulate();
            m_cpu_progress--;
        }

        float sample = render_frame(sample_rate);
        for (int c = 0; c < channel_count; ++c)
            out[i * channel_count + c] = sample;
    }

    m_running_cpu = false;

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
    if (use_filter)
    {
        float filtered_sample = sample;
        filtered_sample = m_previous_filtered_sample * 0.999835f + filtered_sample - m_previous_unfiltered_sample;
        filtered_sample = m_previous_filtered_sample * 0.996039f + filtered_sample - m_previous_unfiltered_sample;
        filtered_sample = (filtered_sample - m_previous_filtered_sample) * 0.815686f;
        filtered_sample *= 0.70f; // Somehow this makes it too loud

        m_previous_filtered_sample = filtered_sample;
        m_previous_unfiltered_sample = sample;

        return filtered_sample * m_volume;
    }
    else
    {
        return sample * m_volume;
    }
}


OSoundRef APUAudioStream::render_sound(int sound_id)
{
    oAudioEngine->removeInstance(OThis);
    play(sound_id);

    std::vector<float> samples;
    int silence_time = 0;
    const double cpu_progress_speed = (double)CPU_CLOCK_SPEED / 48000.0;

    while (silence_time < 48000) // Render until we have 1 sec of silence at the end of the sound
    {
        m_cpu_progress += cpu_progress_speed;
        while (m_cpu_progress >= 1.0)
        {
            emulate();
            m_cpu_progress--;
        }

        float sample = render_frame(48000);

        samples.push_back(sample);
        if (sample == 0)
            silence_time++;
        else
            silence_time = 0;
    }

    oAudioEngine->addInstance(OThis);

    // Remove 1sec at the end we know it's silence
    samples.erase(samples.end() - 48000, samples.end());

    return OSound::createFromData(samples.data(), (int)samples.size(), 1, 48000);
}
