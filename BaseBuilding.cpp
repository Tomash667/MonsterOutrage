#include "Pch.h"
#include "Base.h"
#include "BaseBuilding.h"

const BaseBuilding base_buildings[] = {
	"Inn", INT2(0,32), Action_BuyBeer, Action_Sleep, "Place when travelers can eat and sleep for small price.",
	"Marketplace", INT2(32,32), Action_BuyPotion, Action_Invalid, "Center of trade, can buy healing potions here."
};
