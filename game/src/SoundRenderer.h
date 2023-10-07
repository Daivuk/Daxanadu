#pragma once

#include <onut/Sound.h>

#include "MCS6502.h"


class SoundRenderer final
{
public:
    SoundRenderer(uint8_t* rom, size_t size);

    OSoundRef render_sound(int sound_id);

    void cpu_write(int addr, uint8_t val);
    uint8_t cpu_read(int addr);

    bool use_filter = true;

private:
    float render_frame(int sample_rate);
    void play(int music_id);
    void emulate();

    // Music stuff
    struct pulse_t
    {
        bool enabled = false;
        bool loop = false;
        bool constant_volume = false;
        int duty = 0;
        double progress = 0.0;
        double frequency = 261.63;
        float volume = 0.0f;
        uint8_t registers[4] = {0};
        int length_counter = 0;
        int timer = 0;
        int time = 0;
        int envelope_counter = 0;
        double envelope_rate = 0.0;
        double envelope_progress = 0.0;
    };

    struct triangle_t
    {
        bool enabled = false;
        bool reload_flag = false;
        double progress = 0.0;
        double frequency = 329.63;
        uint8_t registers[4] = {0};
        int length_counter = 0;
        bool control_flag = false;
        int linear_counter = 0;
        int timer = 0;
        int time = 0;
        int reload_value = 0;
    };

    struct noise_t
    {
        bool enabled = false;
        bool loop = false;
        bool mode = false;
        bool constant_volume = false;
        double progress = 0.0;
        double frequency = 261.63;
        float volume = 0.0f;
        uint8_t registers[4] = {0};
        int length_counter = 0;
        int period = 0;
        int time = 0;
        int envelope_counter = 0;
        double envelope_rate = 0.0;
        double envelope_progress = 0.0;
        float previous_sample = 0.0f;
        int shift_register = 1;
    };

    struct dmc_t
    {
        bool enabled = false;
        bool loop = false;
        uint8_t registers[4] = {0};
    };

    pulse_t m_pulses[2];
    noise_t m_noise;
    triangle_t m_triangle;
    dmc_t m_dmc;
    uint8_t m_status_register = 0;
    uint8_t m_frame_counter_register = 0;
    int m_init_infinite_loop_addr = 0;
    int m_play_infinite_loop_addr = 0;
    int m_music_id = 0;
    int m_init_addr = 0;
    int m_music_id_addr = 0;
    int m_play_addr = 0;

    // Cpu vars
    double m_cpu_progress = 0.0;
    uint8_t m_ram[0x10000];
    MCS6502ExecutionContext m_cpu_context;

    // NSF crap
    int m_nsf_load_addr = -1;
    int m_nsf_init_addr = -1;
    int m_nsf_play_addr = -1;
    int m_nsf_play_speed = -1;

    double m_60hz_rate = 0.0;
    double m_60hz_progress = 0.0;
    double m_120hz_rate = 0.0;
    double m_120hz_progress = 0.0;
    double m_240hz_rate = 0.0;
    double m_240hz_progress = 0.0;

    float m_previous_filtered_sample = 0.0f;
    float m_previous_unfiltered_sample = 0.0f;
};
