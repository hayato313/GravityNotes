# Direct3D のビューポート/リサイズ仕様

対象: main ブランチの `framework/direct3d.cpp` / `framework/direct3d.h`

## 1. 結論

この実装では、**ウィンドウサイズ変更時の 2D/3D の切り替えは「バックバッファ自体の自動リサイズ」ではなく、クライアントサイズ情報に基づくビューポート再設定で制御**しています。

- `Direct3D_SetViewport2D()` / `Direct3D_SetViewport3D()` は、**描画先ビューポートを設定するだけ**で、バックバッファや深度バッファの生成・破棄は行いません。
- バックバッファ / 深度バッファを実際に作り直す処理は `configureBackBuffer()` と `releaseBackBuffer()` にあり、明示的な `Direct3D_Resize(...)` で行われます。
- `Direct3D_ResizeWindow()` は、**ウィンドウのクライアント領域サイズを記録するだけ**で、D3D リソースは変更しません。

## 2. 関数別仕様

### 2.1 `Direct3D_SetViewport2D()`

**役割**
- 2D 描画向けに、`DRAW_SCREEN_WIDTH x DRAW_SCREEN_HEIGHT` の比率を保つビューポートを設定する。
- ウィンドウのアスペクト比に応じて、中央寄せのレターボックス/ピラーボックス領域を作る。

**挙動**
- `targetAspect = DRAW_SCREEN_WIDTH / DRAW_SCREEN_HEIGHT`
- `windowAspect = g_ClientWidth / g_ClientHeight`
- `windowAspect > targetAspect` の場合:
  - ウィンドウが横長
  - 2D は **縦を基準** に合わせる
  - `vpH = g_ClientHeight`
  - `vpW = g_ClientHeight * targetAspect`
  - 左右に余白が出る
- それ以外の場合:
  - ウィンドウが縦長または同等
  - 2D は **横を基準** に合わせる
  - `vpW = g_ClientWidth`
  - `vpH = g_ClientWidth / targetAspect`
  - 上下に余白が出る

**重要点**
- 最終的な `D3D11_VIEWPORT` は、`g_BackBufferDesc.Width / g_ClientWidth` と `g_BackBufferDesc.Height / g_ClientHeight` でスケーリングされる。
- つまり、**クライアントサイズとバックバッファサイズの比率差を吸収したうえで、画面内の有効描画領域を調整**している。
- ただし、この関数自体は **バッファの再作成をしない**。

### 2.2 `Direct3D_SetViewport3D()`

**役割**
- 3D 描画向けに、`DRAW_SCREEN_WIDTH x DRAW_SCREEN_HEIGHT` の比率を保つビューポートを設定する。
- 2D とは逆の見せ方を意図しており、ウィンドウサイズ差に応じて「はみ出し」を作る方向が異なる。

**挙動**
- `windowAspect > targetAspect` の場合:
  - ウィンドウが横長
  - 3D は **横いっぱいを使い、縦がはみ出す** 方向
  - `vpW = g_ClientWidth`
  - `vpH = g_ClientWidth / targetAspect`
  - `vpY = (g_ClientHeight - vpH) * 0.5f`
- それ以外の場合:
  - ウィンドウが縦長
  - 3D は **縦いっぱいを使い、横がはみ出す** 方向
  - `vpH = g_ClientHeight`
  - `vpW = g_ClientHeight * targetAspect`
  - `vpX = (g_ClientWidth - vpW) * 0.5f`

**重要点**
- こちらも **ビューポートの設定のみ** を行う。
- 深度テストの有効/無効切り替えは `SetDepthTest(true/false)` 側が担当し、`true` のときに `Direct3D_SetViewport3D()` を呼ぶ。
- つまり 3D のアスペクト比調整は、**DepthTest の ON とセットで使われる想定**になっている。

### 2.3 `Direct3D_ResizeWindow(unsigned int clientW, unsigned int clientH)`

**役割**
- ウィンドウのクライアント領域サイズを内部変数に保存する。

**挙動**
- `g_ClientWidth = clientW` ただし 0 以下相当なら 1.0f に補正
- `g_ClientHeight = clientH` ただし 0 以下相当なら 1.0f に補正

**重要点**
- **この関数は D3D バッファをリサイズしない**。
- 実際の 2D/3D ビューポート計算の基準となる「現在のクライアントサイズ」を更新するだけ。
- サイズ 0 をそのまま保持すると除算や無効 viewport の原因になるため、最小値 1.0f に丸めている。

### 2.4 `Direct3D_GetClientWidth()` / `Direct3D_GetClientHeight()`

**役割**
- 現在記録されているクライアントサイズを取得する。

**挙動**
- `Direct3D_GetClientWidth()` は `g_ClientWidth` を返す
- `Direct3D_GetClientHeight()` は `g_ClientHeight` を返す

**重要点**
- 戻り値は `float`
- 実際のサイズは `Direct3D_ResizeWindow()` 呼び出し時に更新される
- 初期値はそれぞれ `DRAW_SCREEN_WIDTH` / `DRAW_SCREEN_HEIGHT`

## 3. バックバッファと深度バッファのリサイズ仕様

### 3.1 初期化時

`Direct3D_Initialize(HWND hWnd)` では、スワップチェーンのバックバッファ解像度を以下に固定している。

- `swap_chain_desc.BufferDesc.Width = DRAW_SCREEN_WIDTH`
- `swap_chain_desc.BufferDesc.Height = DRAW_SCREEN_HEIGHT`

つまり、**ウィンドウサイズに追従してバックバッファが変わる設計ではない**。

その後 `configureBackBuffer()` により、以下を生成する。

- バックバッファの `RenderTargetView`
- バックバッファ情報 `g_BackBufferDesc`
- 同じ解像度の深度ステンシルバッファ
- 同じ解像度の深度ステンシルビュー

### 3.2 実際のリサイズ処理

`Direct3D_Resize(UINT width, UINT height)` が、バックバッファ/深度バッファの再構築を担当する。

**手順**
1. `g_BackBufferDesc.Width` / `Height` を更新
2. `releaseBackBuffer()` で既存リソースを解放
3. `configureBackBuffer()` で新しいバックバッファと深度バッファを再生成
4. `g_Viewport` を新しい `width` / `height` に合わせて設定

**重要点**
- この関数が呼ばれない限り、**2D/3D の描画先バッファはサイズ変更されない**。
- したがって、ウィンドウサイズ変更イベント時に `Direct3D_ResizeWindow()` だけ呼んでも、**実際のバッファサイズは変わらない**。
- 逆に `Direct3D_Resize()` を呼べば、**バッファサイズとビューポートが再構成される**。

### 3.3 `configureBackBuffer()` の生成対象

この関数は `g_pSwapChain->GetBuffer()` でバックバッファを取得し、以下を作る。

- `g_pRenderTargetView`
- `g_BackBufferDesc`
- `g_pDepthStencilBuffer`
- `g_pDepthStencilView`
- `g_Viewport`

結果として、**D3D の描画先はバックバッファ解像度に依存**する。

### 3.4 `releaseBackBuffer()` の解放対象

解放されるのは以下。

- `g_pRenderTargetView`
- `g_pDepthStencilBuffer`
- `g_pDepthStencilView`

## 4. ウィンドウサイズ変更時の実行順序として読むべきこと

この実装の意図をそのまま読むと、ウィンドウサイズ変更時は次の 2 段階に分かれる。

### 4.1 クライアントサイズの通知

- `Direct3D_ResizeWindow(newClientW, newClientH)`
- ここで `g_ClientWidth` / `g_ClientHeight` が更新される

### 4.2 必要に応じた D3D リソースの再構築

- `Direct3D_Resize(newWidth, newHeight)`
- ここでバックバッファと深度バッファが作り直される
- 同時に `g_Viewport` も更新される

## 5. 2D/3D の使い分け

### 2D
- `SetDepthTest(false)` が呼ばれると `Direct3D_SetViewport2D()` が選ばれる
- 2D はバックバッファ全体に対してではなく、**アスペクト比維持の中央領域**を使う

### 3D
- `SetDepthTest(true)` が呼ばれると `Direct3D_SetViewport3D()` が選ばれる
- 3D は **レターボックス/ピラーボックスの向きが 2D と逆** になる

## 6. 実装上の注意点

- `Direct3D_SetViewport2D()` / `3D()` は、`g_ClientWidth` / `g_ClientHeight` を使うため、`Direct3D_ResizeWindow()` の呼び出しが前提。
- `g_BackBufferDesc.Width` / `Height` は、`configureBackBuffer()` または `Direct3D_Resize()` によってのみ意味を持つ。
- 0 サイズ防止のため、クライアントサイズは最低 1.0f に丸められている。
- `Direct3D_GetBackBufferWidth()` / `Height()` は、**バックバッファの実サイズ** を返す。クライアントサイズとは別概念。

## 7. まとめ

このブランチの仕様は、

- **クライアントサイズ管理**: `Direct3D_ResizeWindow()` / `GetClientWidth()` / `GetClientHeight()`
- **描画領域調整**: `Direct3D_SetViewport2D()` / `Direct3D_SetViewport3D()`
- **実バッファ再生成**: `Direct3D_Resize()` + `configureBackBuffer()`

の 3 層に分かれている。

特に重要なのは、**ウィンドウサイズが変わっただけでは 2D/3D の実バッファは変わらず、`Direct3D_Resize()` を呼んだ場合にのみバックバッファと深度バッファが再構築される** という点である。
