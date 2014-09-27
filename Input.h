#pragma once

#include "Base.h"

enum InputState
{
	IS_UP,			// 00
	IS_RELEASED,	// 01
	IS_DOWN,		// 10
	IS_PRESSED		// 11
};

const uint MAX_BUTTON = 8;
extern byte keystate[SDL_NUM_SCANCODES];
extern byte mousestate[MAX_BUTTON];
extern INT2 cursor_pos;

inline bool KeyDown(int key)
{
	return IS_SET(keystate[key], IS_DOWN);
}

inline bool KeyPressedRelease(int key)
{
	if(IS_ALL_SET(keystate[key], IS_PRESSED))
	{
		keystate[key] = IS_DOWN;
		return true;
	}
	else
		return false;
}

inline bool MousePressedRelease(int bt)
{
	if(IS_ALL_SET(mousestate[bt], IS_PRESSED))
	{
		mousestate[bt] = IS_DOWN;
		return true;
	}
	else
		return false;
}

void UpdateInput();
void ProcessKey(int key, bool down);
void ProcessButton(int bt, bool down);
