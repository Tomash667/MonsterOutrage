#include "Pch.h"
#include "Base.h"
#include "BaseBuilding.h"

const BaseBuilding base_buildings[] = {
	"Inn", INT2(0,32), Action_BuyBeer, Action_Sleep,
	"Marketplace", INT2(32,32), Action_BuyPotion, Action_Invalid
};
