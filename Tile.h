#pragma once

struct TileType
{
	SDL_Rect rect;
	bool block;
};

const TileType tiles[3] = {
	0, 0, 32, 32, false,
	32, 0, 32, 32, true,
	0, 32, 32, 32, false
};

const INT2 wall_offset(0,0), door_offset(1,0);

struct Tile
{
	UnitRef* unit;
	int type;
	struct 
	{
		Building* building;
		BuildingTile building_tile;
	};

	inline bool IsBlocking(bool is_player=false) const
	{
		if(building)
		{
			if(building_tile == BT_DOOR)
				return !is_player;
			return true;
		}
		else
			return tiles[type].block;
	}

	inline void PutBuilding(Building* b, BuildingTile bt)
	{
		assert(b);
		building = b;
		building_tile = bt;
	}
};
