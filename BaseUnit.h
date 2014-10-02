#pragma once

struct BaseUnit
{
	cstring name;
	int lvl, strength, vitality, melee_combat, parry, damage, armor;
	float move_speed, attack_speed;
	INT2 offset, gold;
	bool monster;
};

extern const BaseUnit base_units[];
