#include "RAM.h"

#include <onut/Font.h>
#include <onut/SpriteBatch.h>
#include <imgui/imgui.h>

#include <memory.h>


RAM::RAM()
{
    memset(m_data, 0, sizeof(m_data));
}


void RAM::serialize(FILE* f, int version) const
{
    fwrite(m_data, 1, 0x800, f);
}


void RAM::deserialize(FILE* f, int version)
{
    fread(m_data, 1, 0x800, f);
}


bool RAM::cpu_write(uint16_t addr, uint8_t data)
{
    if (addr < 0x2000)
    {
        //if (addr == 0x03B0)
        //{
        //    __debugbreak();
        //}

        m_data[addr % 0x800] = data;

        //int behaviour_addr = ((int)m_data[0x035C + 7] << 8) | m_data[0x0354 + 7];
        //if (behaviour_addr == 0xAE12)
        //{
        //    __debugbreak();
        //}

        return true;
    }

    return false;
}


bool RAM::cpu_read(uint16_t addr, uint8_t* out_data)
{
    if (addr < 0x2000)
    {
        *out_data = m_data[addr % 0x800];
        return true;
    }

    return false;
}


void RAM::render()
{
#if defined(_DEBUG)
    if (ImGui::Begin("RAM"))
    {
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
#endif
}
