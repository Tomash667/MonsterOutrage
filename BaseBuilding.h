#pragma once

#include "Action.h"

enum BuildingTile
{
	BT_WALL,
	BT_DOOR,
	BT_SIGN
};

struct BaseBuilding
{
	static const int MAX_ACTIONS = 2;

	cstring name;
	INT2 offset;
	ActionId actions[MAX_ACTIONS];
	cstring desc;
};

extern const BaseBuilding base_buildings[];
