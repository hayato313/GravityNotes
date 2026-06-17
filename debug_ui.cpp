#include "debug_ui.h"
#include "debug_params.h"
#include "../framework/imgui/imgui.h"
#include "../framework/imgui/imgui_impl_dx11.h"
#include "../framework/imgui/imgui_impl_win32.h"
#include "../framework/keyboard.h"
#include "../framework/camera.h"

static bool s_IsOpen = false;

void DebugUI_Draw()
{
#ifdef _DEBUG
    if (Keyboard_IsKeyDownTrigger(KK_D1))
        s_IsOpen = !s_IsOpen;

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

	/*ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_FirstUseEver);
	ImGui::Begin("Camera",&s_IsOpen);
	{
		ImGui::Text("Position:%.1f,%.1f,%.1f", GetCamera()->GetPos().x, GetCamera()->GetPos().y, GetCamera()->GetPos().z);
	}
	ImGui::End();*/

    /*ImGui::Begin("LD Parameters", &s_IsOpen);

    auto& p = D_PARAMS;

    ImGui::SeparatorText("Notes");
    ImGui::SliderFloat("Speed",        &p.noteSpeed,        1.0f,  60.0f, "%.1f u/s");
    ImGui::SliderFloat("Hit Distance", &p.hitDistance,      0.5f,  10.0f, "%.2f u");

    ImGui::SeparatorText("Player");
    ImGui::SliderFloat("Lane Width",   &p.laneWidth,        0.5f,  5.0f,  "%.2f u");
    ImGui::SliderFloat("Gravity Time", &p.gravityTransTime, 0.05f, 1.0f,  "%.2f s");

    ImGui::End();*/

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
#endif
}

bool DebugUI_IsOpen()
{
    return s_IsOpen;
}
