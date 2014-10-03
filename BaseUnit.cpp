#include "Pch.h"
#include "Base.h"
#include "BaseUnit.h"

const BaseUnit base_units[] = {
//	NAME		LVL	STR	VIT	H2H	PAR	DMG	ARM	MS		AS		OFF			GOLD			MONSTER
	"Hero",		1,	18,	18,	70,	45,	10,	6,	5.f,	1.f,	INT2(0,0),	INT2(0,0),		false,
	"Goblin",	1,	12,	12,	35,	45,	4,	2,	6.f,	1.2f,	INT2(1,0),	INT2(10,20),	true,
	"Orc",		2,	16,	16,	40,	40,	8,	5,	5.f,	0.9f,	INT2(0,1),	INT2(25,50),	true,
	"Minotaur",	4,	20,	20,	55,	45,	12,	6,	5.f,	0.95f,	INT2(1,1),	INT2(65,80),	true,
	"Guard",	2,	14,	14,	50, 50,	8,	5,	5.f,	0.95f,	INT2(2,0),	INT2(20,35),	false
};
