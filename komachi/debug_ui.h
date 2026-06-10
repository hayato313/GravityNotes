#pragma once

// Draw() の末尾（Present より前）で呼ぶ
// F1 キーでウィンドウの表示/非表示をトグル
void DebugUI_Draw();

// ImGui ウィンドウが開いているか（入力を ImGui に渡すべきか判定用）
bool DebugUI_IsOpen();
