#pragma once

enum Attribute
{
	A_STR, // dmg, penetracja kp
	A_CON, // hp, odpornoœæ
	A_DEX, // szybkoœæ ataku, ruchu, szansa na trafienie
	//A_INT, // magia, szybkoœæ zdobywania expa, krytyk
	A_MAX
};

struct Class
{
	cstring id;
	int base_attrib[A_MAX];
	int attrib_bonus[A_MAX];
};

const Class g_classes[] = {
	"warrior", 18, 16, 14, 20, 20, 15,
	"hunter", 14, 16, 16, 17, 19, 19,
	"rogue", 12, 16, 18, 15, 18, 22
};
