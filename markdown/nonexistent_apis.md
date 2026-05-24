# 存在しない API の整理

renderer.cpp の公開 API に存在しないため、呼び出し側から削除したもの。

- `Shader_BindDefault3D`
- `Shader_SetLightingType`
- `Shader_ClearPointLights`
- `Shader_SetAmbientLight`
- `Shader_SetPointLight`
- `Shader_SetMaterialColor`
- `Shader_Initialize`
- `Shader_Finalize`
- `Shader_RefreshState`
- `Direct3D_Initialize`
- `Direct3D_Finalize`
- `Direct3D_Clear`
- `Direct3D_Present`
- `Direct3D_SetViewport2D`
- `Direct3D_SetViewport3D`
- `Direct3D_ResizeWindow`
- `Direct3D_GetClientWidth`
- `Direct3D_GetClientHeight`

対応方針:

- 行列系は `SetWorldMatrix` / `SetViewMatrix` / `SetProjectionMatrix` に寄せる。
- 深度制御は `SetDepthEnable` を使う。
- それ以外の描画状態は、必要になった時点で renderer.cpp 側に新規 API を足す。