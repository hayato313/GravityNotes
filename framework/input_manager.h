#pragma once

#include "gamepad.h"

typedef enum Input_Action_tag
{
    INPUT_ACTION_DECIDE = 0,
    INPUT_ACTION_CANCEL,
    INPUT_ACTION_MENU_UP,
    INPUT_ACTION_MENU_DOWN,
    INPUT_ACTION_MENU_LEFT,
    INPUT_ACTION_MENU_RIGHT,
    INPUT_ACTION_PAUSE,
} Input_Action;

typedef struct Input_Vector2_tag
{
    float x;
    float y;
} Input_Vector2;

void Input_Initialize(void);
void Input_Finalize(void);
void Input_Update(void);

bool Input_IsActionDown(Input_Action action);
bool Input_IsActionTrigger(Input_Action action);

Input_Vector2 Input_GetMoveVector(void);
Input_Vector2 Input_GetLookVector(void);

void Input_SetRumble(float leftMotor, float rightMotor);

void Input_SetGamepadLayout(Gamepad_Layout layout);
Gamepad_Layout Input_GetGamepadLayout(void);