#include "cart.h"
#include "state.h"
#include <history.h>

#include <onut/onut.h>
#include <onut/Files.h>
#include <onut/Input.h>
#include <onut/Renderer.h>
#include <onut/Settings.h>
#include <onut/Timing.h>

#include <imgui/imgui.h>


static std::vector<uint8_t> COLORS_RAW;
static History<state_t> history;


void initSettings()
{
    oSettings->setGameName("Daxanadu");
    oSettings->setResolution({1600, 900});
    oSettings->setShowFPS(true);
    oSettings->setIsResizableWindow(true);
    oSettings->setStartMaximized(true);
}


void init()
{
    COLORS_RAW = onut::getFileData("FCEUX.pal");

    init_cart();
    load_state();
}


void shutdown()
{
}


void update_shortcuts()
{
    auto ctrl = OInputPressed(OKeyLeftControl);
    auto shift = OInputPressed(OKeyLeftShift);
    auto alt = OInputPressed(OKeyLeftAlt);

    if (ctrl && !shift && !alt && OInputJustPressed(OKeyZ)) history.undo(state);
    if (ctrl && shift && !alt && OInputJustPressed(OKeyZ)) history.redo(state);
    if (ctrl && !shift && !alt && OInputJustPressed(OKeyS)) save_state();
}


void update()
{
    update_shortcuts();
}


void render()
{
    oRenderer->clear(Color::Black);
}


void postRender()
{
}


void renderUI()
{
    ImGui::BeginMainMenuBar();
    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Save")) save_state();
        ImGui::Separator();
        if (ImGui::MenuItem("Generate"))
        {
            save_state();
            //generate();
        }
        if (ImGui::MenuItem("Exit")) OQuit();
        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();

    if (ImGui::Begin("Strings"))
    {
        for (const auto& cart_string : cart.strings)
        {
            ImGui::Text("0x%08X", cart_string.address);
            ImGui::SameLine();
            ImGui::BeginGroup();
            for (const auto& text_block : cart_string.text_blocks)
            {
                ImGui::Text("%s", text_block.c_str());
                ImGui::Separator();
            }
            ImGui::EndGroup();
        }
    }
    ImGui::End();

    if (ImGui::Begin("Palettes"))
    {
        for (const auto& palette : cart.palettes)
        {
            ImVec2 pos = ImGui::GetCursorScreenPos();
            for (int i = 0; i < 16; ++i)
            {
                ImGui::GetWindowDrawList()->AddRectFilled({pos.x + i * 34, pos.y}, {pos.x + i * 34 + 32, pos.y + 20}, IM_COL32(
                    COLORS_RAW[palette.palette[i] * 3 + 0],
                    COLORS_RAW[palette.palette[i] * 3 + 1],
                    COLORS_RAW[palette.palette[i] * 3 + 2],
                    255));
            }
            ImGui::Dummy(ImVec2(200, 24));
        }
    }
    ImGui::End();
}
