#include "Pch.h"
#include "Base.h"
#include "Action.h"

const Action actions[] = {
	"Sleep", "Sleep inside inn for 10 gp.", Action_Sleep, 1, INT2(0,0),
	"Cancel", "Cancel current action.", Action_Cancel, -100, INT2(16,0),
	"Buy beer", "Buy and drink some beer for 3 gp.", Action_BuyBeer, 0, INT2(32,0),
	"Exit", "Exit from building.", Action_Exit, -50, INT2(48,0),
	"Use potion", "Drink healing potion.", Action_UsePotion, 100, INT2(0,16),
	"Buy potion", "Buy healing potion for 20 gp.", Action_BuyPotion, 0, INT2(0,32),
	"Invalid action", "", Action_Invalid, 999, INT2(16,0)
};
