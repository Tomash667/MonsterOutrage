#pragma once

enum DIR
{
	DIR_S,
	DIR_SW,
	DIR_W,
	DIR_NW,
	DIR_N,
	DIR_NE,
	DIR_E,
	DIR_SE,
	DIR_INVALID
};

INT2 dir_change[] = {
	INT2(0,1),
	INT2(-1,1),
	INT2(-1,0),
	INT2(-1,-1),
	INT2(0,-1),
	INT2(1,-1),
	INT2(1,0),
	INT2(1,1),
	INT2(0,0)
};

inline int Distance(const INT2& a, const INT2& b)
{
	int dist_x = abs(a.x - b.x),
		dist_y = abs(a.y - b.y);
	int smal = min(dist_x, dist_y);
	return smal*15 + (max(dist_x, dist_y)-smal)*10;
}
