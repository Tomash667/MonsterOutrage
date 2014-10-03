#include "Pch.h"
#include "Base.h"
#include "Action.h"

const Action actions[] = {
	"Sleep", "(1) Sleep inside inn for 10 gp.", Action_Sleep, 1, INT2(0,0), SDL_SCANCODE_1,
	"Cancel", "(C) Cancel current action.", Action_Cancel, -100, INT2(16,0), SDL_SCANCODE_C,
	"Buy beer", "(2) Buy and drink some beer for 3 gp.", Action_BuyBeer, 0, INT2(32,0), SDL_SCANCODE_2,
	"Exit", "(X) Exit from building.", Action_Exit, -50, INT2(48,0), SDL_SCANCODE_X,
	"Use potion", "(H) Drink healing potion.", Action_DrinkPotion, 100, INT2(0,16), SDL_SCANCODE_H,
	"Buy potion", "(1) Buy healing potion for 20 gp.", Action_BuyPotion, 0, INT2(16,16), SDL_SCANCODE_1,
	"Invalid action", "", Action_Invalid, 999, INT2(16,0), SDL_SCANCODE_UNKNOWN
};
