#include "stageselect.h"
#include "sprite2d.h"
#include "texture.h"
#include "keyboard.h"
#include "fade.h"
#include "debug_ostream.h"
#include "define.h"
#include "font.h"
#include "mouse.h"
#include "sound.h"
#include "ClickFont.h"
#include "MultiLineFontRenderer.h"
#include "scoresummaryloader.h"
#include "scene.h"
#include <cstdio>

using namespace DirectX;

// 1. 背景画像（ビニール盤ジャケット背景）
static Sprite2D* g_pBackground = nullptr;

// 2. 中央のメインビニール盤とトーンアーム（腕）
static Sprite2D* g_pMainVinyl = nullptr;
static Sprite2D* g_pToneArm = nullptr;

// 3. 左側のステージ選択盤（配列で管理）
const int MAX_STAGES = 5;
static Sprite2D* g_pStageDisks[MAX_STAGES] = { nullptr };
static ClickFont* g_pStageButtons[MAX_STAGES] = { nullptr };

// 4. ロジック管理用の変数
static int g_SelectedStage = 0;      // 現在選択されているステージ
static float g_VinylRotation = 0.0f; // メインビニール盤の回転角度

// ①インスタンス、ポインタ用意
static MultiLineFontRenderer* g_pScoreInfoText = nullptr;
static std::vector<ScoreSummary> g_ScoreSummaries;
static int g_SelectedScoreIndex = 0;

//g_pScoreInfoTextに現在表示しているjsonのファイル名を返す関数
static std::string GetSelectedJsonName()
{
	if (g_ScoreSummaries.empty())
	{
		return "";
	}

	if (g_SelectedScoreIndex < 0)
	{
		g_SelectedScoreIndex = 0;
	}
	if (g_SelectedScoreIndex >= static_cast<int>(g_ScoreSummaries.size()))
	{
		g_SelectedScoreIndex = static_cast<int>(g_ScoreSummaries.size()) - 1;
	}

	return g_ScoreSummaries[static_cast<size_t>(g_SelectedScoreIndex)].jsonname;
}

// 現在選択中の楽曲情報を1曲分だけ整形して表示する
static void RefreshSelectedScoreText()
{
	if (g_pScoreInfoText == nullptr)
	{
		return;
	}

	if (g_ScoreSummaries.empty())
	{
		g_pScoreInfoText->SetText("No score json found");
		return;
	}

	if (g_SelectedScoreIndex < 0)
	{
		g_SelectedScoreIndex = 0;
	}
	if (g_SelectedScoreIndex >= static_cast<int>(g_ScoreSummaries.size()))
	{
		g_SelectedScoreIndex = static_cast<int>(g_ScoreSummaries.size()) - 1;
	}

	const ScoreSummary& summary = g_ScoreSummaries[static_cast<size_t>(g_SelectedScoreIndex)];

	char buf[1024] = {};
	std::snprintf(
		buf,
		sizeof(buf),
		"[%d/%d]\nMusic: %s\n作曲者: %s\n譜面作者: %s\n難易度: %.1f\nBPM: %.1f\nJSON: %s",
		g_SelectedScoreIndex + 1,
		static_cast<int>(g_ScoreSummaries.size()),
		summary.musicname.c_str(),
		summary.musicauthor.c_str(),
		summary.scoreauthor.c_str(),
		summary.difficulty,
		summary.bpm,
		summary.jsonname.c_str()
	);
	g_pScoreInfoText->SetText(buf);
}

// 左右入力で選択中インデックスを循環更新し、表示内容を更新する
static void ChangeSelectedScore(int delta)
{
	if (g_ScoreSummaries.empty())
	{
		return;
	}

	const int count = static_cast<int>(g_ScoreSummaries.size());
	g_SelectedScoreIndex = (g_SelectedScoreIndex + delta) % count;
	if (g_SelectedScoreIndex < 0)
	{
		g_SelectedScoreIndex += count;
	}

	RefreshSelectedScoreText();
}

// StageSelectシーンの生成と、譜面概要一覧の再読み込みを行う
void StageSelect_Initialize(void)
{
	// 背景スプライトの初期化（解像度 2560 x 1440）
	g_pBackground = new Sprite2D(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f },
		{ 740.0f, 556.0f },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset\\texture\\vinylbg.png"
	);

	// 左側のステージディスク配列とそれに対応するボタンを初期化
	for (int i = 0; i < MAX_STAGES; i++) {
		float posX = 130.0f;
		float posY = 50.0f + (i * 150.0f); // Y座標は徐々に下へ移動

		g_pStageDisks[i] = new Sprite2D(
			{ posX, posY },
			{ 150.0f, 150.0f },
			0.0f,
			{ 1.0f, 1.0f, 1.0f, 1.0f },
			BLENDSTATE_ALFA,
			L"asset\\texture\\vinmain.png"
		);

		// ディスクの位置にテキスト/ボタンを配置
		g_pStageButtons[i] = new ClickFont(
			{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 4.0f * 3.5 },
			50.0f, // ディスクに合わせるようにサイズを調整
			0.0f,
			{ 1.0f, 1.0f, 1.0f, 1.0f },
			{ 1.0f, 0.8f, 0.2f, 1.0f },
			"PLAY" // 仮の表示テキスト、必要に応じてカスタマイズ可能
		);
	}

	// 画面中央のメインビニール盤を初期化
	g_pMainVinyl = new Sprite2D(
		{ SCREEN_WIDTH / 2.0f - 85.0f, SCREEN_HEIGHT / 2.0f + 2.0f },
		{ 500.0f, 500.0f },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA, // BLENDSTATE_NONEからアルファブレンディングに変更
		L"asset\\texture\\vinmain.png"
	);

	// トーンアーム（針）も同様に初期化
	g_pToneArm = new Sprite2D(
		{ SCREEN_WIDTH / 2.0f + 350.0f, SCREEN_HEIGHT / 2.0f - 300.0f },
		{ 400.0f, 400.0f },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA, // アルファブレンディングを使用
		L"asset\\texture\\tonearm.png"
	);

	// スコア情報テキストの初期化（左右矢印キーで曲を変更可能）
	g_pScoreInfoText = new MultiLineFontRenderer(
		{ SCREEN_WIDTH - 200.0f, SCREEN_HEIGHT - 200.0f },
		28.0f,
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		"loading...",
		1.35f
	);

	g_ScoreSummaries = LoadScoreSummaries();
	const bool loaded = !g_ScoreSummaries.empty();
	hal::dout << "[StageSelect] score summary reload: "
		<< (loaded ? "success" : "failed")
		<< " count=" << g_ScoreSummaries.size()
		<< std::endl;

	g_SelectedScoreIndex = 0;
	RefreshSelectedScoreText();

	UnLockMouse(); // マウスをアンロック
}

// 入力処理（左右で選曲）と決定クリック時のシーン遷移を行う
void StageSelect_Update(void)
{
	// 1. 左側の全ボタンの状態を更新
	for (int i = 0; i < MAX_STAGES; i++)
	{
		g_pStageButtons[i]->Update();

		// プレイヤーがディスク i をクリックした場合
		if (g_pStageButtons[i]->IsClick())
		{
			g_SelectedStage = i; // 選択されたステージを記録

			// ここでsound.hを使用してディスク切り替え音を再生可能
			// PlaySound(...);

			// ==============================================
			// 
			//  ここ超重要
			//
			// ==============================================
			//jsonの指定はSetPlayJson関数を使用する
			//引数の例　「shiningstar.json」
			//今回は仮表示テキストg_pScoreInfoTextと連携するため、
			//GetSelectedJsonName（返り値string）を使用して選択中のファイル名を返している
			//SetPlayJson→SetSceneFadeの順を厳守。

			SetPlayJson(GetSelectedJsonName());
			SetSceneFade(SCENE_GAME);

			// ==============================================

		}
	}

	// 2. メインビニール盤を滑らかに回転させる
	g_VinylRotation += 0.5f; // 回転速度（必要に応じて増減可能）
	if (g_VinylRotation >= 360.0f) {
		g_VinylRotation -= 360.0f;
	}

	// メインビニール盤スプライトに回転角度を反映
	if (g_pMainVinyl != nullptr) {
		g_pMainVinyl->SetRot(g_VinylRotation);
		// ③回転処理
	}

	if (Keyboard_IsKeyDownTrigger(KK_LEFT))
	{
		ChangeSelectedScore(-1); // 左矢印キー: 前の曲に変更
	}

	if (Keyboard_IsKeyDownTrigger(KK_RIGHT))
	{
		ChangeSelectedScore(1); // 右矢印キー: 次の曲に変更
	}

}

// StageSelectシーンの描画を行う
void StageSelect_Draw(void)
{
	// ④ 描画順序: 奥から手前へ層状に描画
	g_pBackground->Draw(); // 背景を先に描画

	// 左側の全てのディスクリストとテキストを描画
	for (int i = 0; i < MAX_STAGES; i++) {
		g_pStageDisks[i]->Draw();
		g_pStageButtons[i]->Draw();
	}

	g_pMainVinyl->Draw(); // 中央のメインビニール盤を描画
	g_pToneArm->Draw();   // トーンアーム（針）をメインビニール盤の上に描画

	// ④スコア情報テキストを描画
	g_pScoreInfoText->Draw();
}

// StageSelectシーンで確保したリソースを解放する
void StageSelect_Finalize(void)
{
	// ⑤ メモリリーク防止のため全てのメモリを適切に解放
	SAFE_DELETE(g_pBackground);
	SAFE_DELETE(g_pMainVinyl);
	SAFE_DELETE(g_pToneArm);

	for (int i = 0; i < MAX_STAGES; i++) {
		SAFE_DELETE(g_pStageDisks[i]);
		SAFE_DELETE(g_pStageButtons[i]);
	}

	SAFE_DELETE(g_pScoreInfoText);
	g_ScoreSummaries.clear(); // スコア情報リストをクリア
}
