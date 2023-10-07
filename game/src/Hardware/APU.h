#pragma once

#include "CPUPeripheral.h"

#include <onut/AudioStream.h>

#include <atomic>
#include <stdio.h>
#include <mutex>


ForwardDeclare(APUAudioStream);


class APU final : public CPUPeripheral
{
public:
    APU();

    bool cpu_write(uint16_t addr, uint8_t data) override;
    bool cpu_read(uint16_t addr, uint8_t* out_data) override;

    void serialize(FILE* f, int version) const;
    void deserialize(FILE* f, int version);

    void set_volume(float volume);

private:
    APUAudioStreamRef m_audio_stream;
};


class APUAudioStream final : public OAudioStream, public std::enable_shared_from_this<APUAudioStream>
{
public:
    APUAudioStream();

    bool progress(int frame_count, int sample_rate, int channel_count, float* out, float volume = 1.0f, float balance = 0.0f, float pitch = 1.0f) override;

    float render_frame(int sample_rate);

    void cpu_write(int addr, uint8_t val);
    uint8_t cpu_read(int addr);

    void serialize(FILE* f, int version);
    void deserialize(FILE* f, int version);

    void set_volume(float volume);

private:
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
    triangle_t m_triangle;
    noise_t m_noise;
    dmc_t m_dmc;
    uint8_t m_status_register = 0;
    uint8_t m_frame_counter_register = 0;
    float m_volume = 1.0f;
    std::mutex m_mutex;

    double m_cpu_progress = 0.0;
    double m_60hz_rate = 0.0;
    double m_60hz_progress = 0.0;
    double m_120hz_rate = 0.0;
    double m_120hz_progress = 0.0;
    double m_240hz_rate = 0.0;
    double m_240hz_progress = 0.0;

    float m_previous_filtered_sample = 0.0f;
    float m_previous_unfiltered_sample = 0.0f;
};
