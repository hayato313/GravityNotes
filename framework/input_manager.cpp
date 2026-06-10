#include "input_manager.h"

#include "keyboard.h"

namespace
{
    const float kMoveStickThreshold = 0.5f;

    int GetActivePlayerIndex()
    {
        return Gamepad_FindConnectedPlayer();
    }

    float MinFloat(float lhs, float rhs)
    {
        return (lhs < rhs) ? lhs : rhs;
    }

    float MaxFloat(float lhs, float rhs)
    {
        return (lhs > rhs) ? lhs : rhs;
    }
}

void Input_Initialize(void)
{
    Gamepad_Initialize();
    Gamepad_SetLayout(GAMEPAD_LAYOUT_SWITCH_ABXY);
}

void Input_Finalize(void)
{
    Gamepad_Finalize();
}

void Input_Update(void)
{
    Gamepad_Update();
}

bool Input_IsActionDown(Input_Action action)
{
    const int player = GetActivePlayerIndex();

    switch (action)
    {
    case INPUT_ACTION_DECIDE:
        return Keyboard_IsKeyDown(KK_ENTER) || Keyboard_IsKeyDown(KK_SPACE) || Gamepad_IsButtonDown(player, GPB_A);
    case INPUT_ACTION_CANCEL:
        return Keyboard_IsKeyDown(KK_ESCAPE) || Keyboard_IsKeyDown(KK_BACK) || Gamepad_IsButtonDown(player, GPB_B);
    case INPUT_ACTION_MENU_UP:
        return Keyboard_IsKeyDown(KK_UP) || Gamepad_IsButtonDown(player, GPB_DPAD_UP) || Input_GetMoveVector().y > kMoveStickThreshold;
    case INPUT_ACTION_MENU_DOWN:
        return Keyboard_IsKeyDown(KK_DOWN) || Gamepad_IsButtonDown(player, GPB_DPAD_DOWN) || Input_GetMoveVector().y < -kMoveStickThreshold;
    case INPUT_ACTION_MENU_LEFT:
        return Keyboard_IsKeyDown(KK_LEFT) || Gamepad_IsButtonDown(player, GPB_DPAD_LEFT) || Input_GetMoveVector().x < -kMoveStickThreshold;
    case INPUT_ACTION_MENU_RIGHT:
        return Keyboard_IsKeyDown(KK_RIGHT) || Gamepad_IsButtonDown(player, GPB_DPAD_RIGHT) || Input_GetMoveVector().x > kMoveStickThreshold;
    case INPUT_ACTION_PAUSE:
        return Keyboard_IsKeyDown(KK_P) || Gamepad_IsButtonDown(player, GPB_START);
    default:
        return false;
    }
}

bool Input_IsActionTrigger(Input_Action action)
{
    const int player = GetActivePlayerIndex();

    switch (action)
    {
    case INPUT_ACTION_DECIDE:
        return Keyboard_IsKeyDownTrigger(KK_ENTER) || Keyboard_IsKeyDownTrigger(KK_SPACE) || Gamepad_IsButtonTrigger(player, GPB_A);
    case INPUT_ACTION_CANCEL:
        return Keyboard_IsKeyDownTrigger(KK_ESCAPE) || Keyboard_IsKeyDownTrigger(KK_BACK) || Gamepad_IsButtonTrigger(player, GPB_B);
    case INPUT_ACTION_MENU_UP:
        return Keyboard_IsKeyDownTrigger(KK_UP) || Gamepad_IsButtonTrigger(player, GPB_DPAD_UP);
    case INPUT_ACTION_MENU_DOWN:
        return Keyboard_IsKeyDownTrigger(KK_DOWN) || Gamepad_IsButtonTrigger(player, GPB_DPAD_DOWN);
    case INPUT_ACTION_MENU_LEFT:
        return Keyboard_IsKeyDownTrigger(KK_LEFT) || Gamepad_IsButtonTrigger(player, GPB_DPAD_LEFT);
    case INPUT_ACTION_MENU_RIGHT:
        return Keyboard_IsKeyDownTrigger(KK_RIGHT) || Gamepad_IsButtonTrigger(player, GPB_DPAD_RIGHT);
    case INPUT_ACTION_PAUSE:
        return Keyboard_IsKeyDownTrigger(KK_P) || Gamepad_IsButtonTrigger(player, GPB_START);
    default:
        return false;
    }
}

Input_Vector2 Input_GetMoveVector(void)
{
    const int player = GetActivePlayerIndex();
    const Gamepad_ThumbStick leftStick = Gamepad_GetLeftStick(player);

    Input_Vector2 out = {};
    out.x = leftStick.x;
    out.y = leftStick.y;

    if (Keyboard_IsKeyDown(KK_LEFT)) out.x = MinFloat(out.x, -1.0f);
    if (Keyboard_IsKeyDown(KK_RIGHT)) out.x = MaxFloat(out.x, 1.0f);
    if (Keyboard_IsKeyDown(KK_UP)) out.y = MaxFloat(out.y, 1.0f);
    if (Keyboard_IsKeyDown(KK_DOWN)) out.y = MinFloat(out.y, -1.0f);

    return out;
}

Input_Vector2 Input_GetLookVector(void)
{
    const int player = GetActivePlayerIndex();
    const Gamepad_ThumbStick rightStick = Gamepad_GetRightStick(player);
    Input_Vector2 out = {};
    out.x = rightStick.x;
    out.y = rightStick.y;
    return out;
}

void Input_SetRumble(float leftMotor, float rightMotor)
{
    const int player = GetActivePlayerIndex();
    if (player >= 0)
    {
        Gamepad_SetVibration(player, leftMotor, rightMotor);
    }
}

void Input_SetGamepadLayout(Gamepad_Layout layout)
{
    Gamepad_SetLayout(layout);
}

Gamepad_Layout Input_GetGamepadLayout(void)
{
    return Gamepad_GetLayout();
}