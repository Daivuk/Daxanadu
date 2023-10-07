#pragma once

#include <cinttypes>


class CPUBUS;


class CPUPeripheral
{
public:
    virtual ~CPUPeripheral() {}

    virtual bool cpu_write(uint16_t addr, uint8_t data) { return false; };
    virtual bool cpu_read(uint16_t addr, uint8_t* out_data) { return false; };

    CPUBUS* get_cpu_bus() const { return m_cpu_bus; }
    void set_cpu_bus(CPUBUS* bus);

private:
    CPUBUS* m_cpu_bus = nullptr;
};
