/*==============================================================================

   日本語フォント描画システム [font.h]
                                          Author : Copilot
                                          Date   : 2025/01/10
--------------------------------------------------------------------------------

==============================================================================*/
#ifndef FONT_H
#define FONT_H

#include <Windows.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <string>
#include <vector>
#include <map>
#include <deque>
#include "component.h"
using namespace DirectX;

// フォント設定定数
#define FONT_MARGIN_RATIO (0)          // グリフ幅に対するマージン比率（5%）
#define FONT_ATLAS_WIDTH (2048)            // アトラステクスチャの幅
#define FONT_ATLAS_HEIGHT (2048)           // アトラステクスチャの高さ
#define FONT_MAX_CACHE_GLYPHS (512)        // 最大キャッシュグリフ数
#define FONT_CACHE_GLYPH_SIZE (48)         // 1グリフのサイズ（ピクセル）
#define FONT_OFFSET_Y (0.25f)              // Y座標オフセット（フォントサイズの25%）

// フォント文字情報
struct CharInfo {
    float x0, y0, x1, y1;  // アトラス上のテクスチャ座標
    float xadvance;         // 文字の進む量
    int glyphIndex;         // グリフインデックス
};

// グローバルフォントデータ管理
void Font_InitializeGlobalData();
void Font_FinalizeGlobalData();

// フォント管理クラス
class FontRenderer : public Transform2D
{
public:
	// pos: 中心位置, fontSize: フォントサイズ(px), rotation: 角度(度)
	// color: 色(R,G,B,A), text: 表示テキスト
	FontRenderer(XMFLOAT2 pos, float fontSize, float rotation,
				 XMFLOAT4 color, const std::string& text);
	~FontRenderer();

	virtual void Draw();
	void SetColor(XMFLOAT4 color) { m_Color = color; UpdateAtlasTexture(); }
	virtual void SetText(const std::string& text);
	XMFLOAT4 GetColor() const { return m_Color; }

	// テキスト中のグリフを事前にアトラスへ登録（描画時のスタッター防止）
	void PreCacheGlyphs();

private:
	ID3D11Texture2D* m_pTexture;
	ID3D11ShaderResourceView* m_pSRV;
	ID3D11Buffer* m_pVertexBuffer;

	XMFLOAT4 m_Color;
	std::string m_Text;
	float m_FontSize;                         // フォントサイズ（ピクセル）
	
	UINT m_VertexCount;
	std::map<int, CharInfo> m_CharCache;      // グリフIDからCharInfo への マッピング
	std::deque<int> m_CacheLRU;               // LRU キャッシュ用キュー
	
	unsigned char* m_pAtlasData;              // アトラスバッファ
	int m_AtlasWidth;
	int m_AtlasHeight;
	int m_AtlasNextX;                         // 次のグリフを配置するX位置
	int m_AtlasNextY;                         // 次のグリフを配置するY位置
	int m_AtlasRowHeight;                     // 現在の行の高さ
	
	// フォントメトリクス
	int m_FontAscender;                        // アセンダー（フォント単位）
	int m_FontDescender;                       // ディセンダー（フォント単位）

	// ヘルパー関数
	bool CreateShaders();
	bool BakeAtlas();
	bool AddGlyphToAtlas(int glyphIndex);
	bool AddGlyphToAtlasBatch(int glyphIndex); // テクスチャ更新なし版
	void EvictLRUGlyph();
	int UTF8ToCodePoint(const std::string& text, size_t& index);
	void UpdateAtlasTexture();

	// stbtt 関連
	struct stbtt_fontinfo* m_pFontInfo;
};

#endif // FONT_H
