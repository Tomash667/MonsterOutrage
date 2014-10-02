#include "Pch.h"
#include "Base.h"
#include "BaseUnit.h"

const BaseUnit base_units[] = {
//	NAME		LVL	STR	VIT	H2H	PAR	DMG	ARM	MS		AS		OFF			GOLD
	"Hero",		1,	18,	18,	70,	45,	10,	6,	5.f,	1.f,	INT2(0,0),	INT2(0,0),
	"Goblin",	1,	12,	12,	35,	45,	4,	2,	6.f,	1.2f,	INT2(1,0),	INT2(10,20),
	"Orc",		2,	16,	16,	40,	40,	8,	5,	5.f,	0.9f,	INT2(0,1),	INT2(25,50),
	"Minotaur",	4,	20,	20,	60,	50,	12,	8,	5.f,	0.95f,	INT2(1,1),	INT2(65,80)
};
