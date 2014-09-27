#include "Pch.h"
#include "Input.h"

byte keystate[SDL_NUM_SCANCODES];
byte mousestate[MAX_BUTTON];
INT2 cursor_pos;

//=============================================================================
void UpdateInput()
{
	for(uint i=0; i < SDL_NUM_SCANCODES; ++i)
	{
		if(keystate[i] & 1)
			--keystate[i];
	}
	for(uint i=0; i< MAX_BUTTON; ++i)
	{
		if(mousestate[i] & 1)
			--mousestate[i];
	}
}

//=============================================================================
void ProcessKey(int key, bool down)
{
	if(down)
	{
		if(keystate[key] <= IS_RELEASED)
			keystate[key] = IS_PRESSED;
	}
	else
	{
		if(keystate[key] >= IS_DOWN)
			keystate[key] = IS_RELEASED;
	}
}

//=============================================================================
void ProcessButton(int bt, bool down)
{
	if(down)
	{
		if(mousestate[bt] <= IS_RELEASED)
			mousestate[bt] = IS_PRESSED;
	}
	else
	{
		if(mousestate[bt] >= IS_DOWN)
			mousestate[bt] = IS_RELEASED;
	}
}
