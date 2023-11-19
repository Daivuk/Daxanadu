#include "RAM.h"

#include <onut/Font.h>
#include <onut/Renderer.h>
#include <onut/SpriteBatch.h>

#include <imgui/imgui.h>

#include <memory.h>


RAM::RAM()
{
    memset(m_data, 0, sizeof(m_data));
#if SHOW_RAM
    memset(m_usage, 0, sizeof(m_usage));
#endif
}


void RAM::register_write_callback(const std::function<uint8_t(uint8_t,int)>& callback, int addr)
{
    m_write_callbacks.push_back({ addr, callback });
}


void RAM::register_read_callback(const std::function<bool(uint8_t*,int)>& callback, int addr)
{
    m_read_callbacks.push_back({ addr, callback });
}


void RAM::serialize(FILE* f, int version) const
{
    fwrite(m_data, 1, 0x2000, f);
}


void RAM::deserialize(FILE* f, int version)
{
    if (version < 5)
        fread(m_data, 1, 0x800, f);
    else
        fread(m_data, 1, 0x2000, f);
}


bool RAM::cpu_write(uint16_t addr, uint8_t data)
{
    if (addr < 0x2000)
    {
        for (const auto& write_callback : m_write_callbacks)
            if (write_callback.first == (int)addr)
                data = write_callback.second(data, (int)addr);

        //if (addr == 0x0220)
        //{
        //    __debugbreak();
        //}
        
        // We don't need to mirror, Faxanadu never writes out of range.
        // So we can expand our ram for other usage
        m_data[addr/* % 0x800*/] = data;

        //int behaviour_addr = ((int)m_data[0x035C + 7] << 8) | m_data[0x0354 + 7];
        //if (behaviour_addr == 0xAE12)
        //{
        //    __debugbreak();
        //}

#if SHOW_RAM
        m_usage[addr] = 2.0f;
#endif

        return true;
    }

    return false;
}


bool RAM::cpu_read(uint16_t addr, uint8_t* out_data)
{
    if (addr < 0x2000)
    {

        //if (addr == 0x0220)
        //{
        //    __debugbreak();
        //}

        // We don't need to mirror, Faxanadu never writes out of range.
        // So we can expand our ram for other usage
        *out_data = m_data[addr/* % 0x800*/];

        for (const auto& read_callback : m_read_callbacks)
            if (read_callback.first == (int)addr)
                if (read_callback.second(out_data, (int)addr))
                    return true;

        return true;
    }

    return false;
}


void RAM::update(float dt)
{
#if SHOW_RAM
    if (m_time_passed < 5.0f)
    {
        m_time_passed += dt;
        if (m_time_passed >= 5.0f)
        {
            // Late re-initialization because we want to make sure the ram is initialized
            memset(m_usage, 0, sizeof(m_usage));
        }
    }
    for (int i = 0; i < 0x2000; ++i)
    {
        float val = m_usage[i];
        if (val > 1.0f)
        {
            val -= dt;
            if (val < 1.0f) val = 1.0f;
            m_usage[i] = val;
        }
    }
#endif
}


void RAM::render()
{
#if defined(_DEBUG)
    if (ImGui::Begin("RAM"))
    {
        ImGui::Text("Quests: 0x%02X", (int)m_data[0x042D]);
        ImGui::Text("Input Context: 0x%02X", (int)m_data[0x0800]);

        ImGui::Separator();
        ImGui::Text("Weapon: 0x%02X", (int)m_data[0x03BD]);
        ImGui::Text("Armor: 0x%02X", (int)m_data[0x03BE]);
        ImGui::Text("Shield: 0x%02X", (int)m_data[0x03BF]);
        ImGui::Text("Magic: 0x%02X", (int)m_data[0x03C0]);
        ImGui::Text("Item: 0x%02X", (int)m_data[0x03C1]);

        if (ImGui::TreeNodeEx("Items:", ImGuiTreeNodeFlags_DefaultOpen))
        {
            for (int i = 0; i < (int)m_data[0x03C6]; ++i)
                ImGui::Text("0x%02X", (int)m_data[0x03AD + i]);
            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Commons:", ImGuiTreeNodeFlags_DefaultOpen))
        {
            for (int i = 0; i < 4; ++i)
            {
                if (m_data[0x03B5 + i] == 0) break;
                ImGui::Text("0x%02X  (%i)", (int)m_data[0x03B5 + i], (int)m_data[0x03B9 + i]);
            }
            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Active entities:"))
        {
            for (int i = 0; i < 8; ++i)
            {
                if (m_data[0x02CC + i] == 0xFF) continue;
                ImGui::Text("ID: 0x%02X - 0x%02X", (int)m_data[0x02CC + i], (int)m_data[0x02D4 + i]);
                ImGui::Text("Position: %i, %i", (int)m_data[0x00BA + i], (int)m_data[0x00C2 + i]);
                auto flags = m_data[0x02DC + i];
                ImGui::Text("Flags: 0x%02X", (int)flags);
                ImGui::Text("Visible: %s", (flags & 0x10) ? "false" : "true");
                ImGui::Text("Phase: 0x%02X - 0x%02X", (int)m_data[0x02E4 + i], (int)m_data[0x02EC + i]);
                int behaviour_addr = ((int)m_data[0x035C + i] << 8) | m_data[0x0354 + i];
                ImGui::Text("Behaviour: 0x%04X", behaviour_addr);
                ImGui::Text("Message: 0x%02X", (int)m_data[0x036C + i]);
                ImGui::Separator();
            }
            ImGui::TreePop();
        }
    }
    ImGui::End();

#if SHOW_RAM
    // Ram inspection
    {
        auto sb = oSpriteBatch.get();
        float start_x = OScreenWf - 64 * 9 - 8.0f;
        float x = start_x;
        float y = 8.0f;

        sb->begin();

        sb->drawRect(nullptr, Rect(x - 1, y - 1, 64 * 9 + 1, 128 * 9 + 1), Color(0.5f, 0.25f, 0.35f, 1.0f));
        for (int j = 0; j < 32; ++j)
        {
            sb->drawRect(nullptr, Rect(x - 1, y + (float)j * 4 * 9 - 1, 64 * 9, 1), Color(0.75f, 0.5f, 0.7f, 1.0f));
        }

#define WATCH(addr) sb->drawRect(nullptr, Rect(start_x + (float)((addr) % 64) * 9 - 1, y + (float)((addr) / 64) * 9 - 1, 10, 10), Color(1, 1, 0, 1))

        for (int i = 0; i < 8; ++i)
        {
            WATCH(0x03B5 + i);
        }

        float* usage = m_usage;
        float r = 0.0f;
        float gb = 0.0f;
        for (int j = 0; j < 128; ++j)
        {
            for (int i = 0; i < 64; ++i, ++usage, x += 9)
            {
                r = *usage;
                gb = *usage - 1.0f;
                sb->drawRect(nullptr, Rect(x, y, 8, 8), Color(r, gb, gb, 1.0f));
            }
            x = start_x;
            y += 9;
        }

        sb->end();
    }
#endif
#endif
}
