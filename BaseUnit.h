#pragma once

struct BaseUnit
{
	cstring name;
	int lvl, hp, attack, defense;
	float move_speed, attack_speed;
	INT2 offset, gold;
};

// this should be const but temporary we edit hero stats upon level up
extern /*const*/ BaseUnit base_units[];
