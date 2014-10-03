#pragma once

enum ActionId
{
	Action_Sleep,
	Action_Cancel,
	Action_BuyBeer,
	Action_Exit,
	Action_DrinkPotion,
	Action_BuyPotion,
	Action_Invalid
};

struct Action
{
	cstring name, desc;
	ActionId id;
	int priority;
	INT2 offset;
	SDL_Scancode key;
};

enum ActionSource
{
	AS_ITEM,
	AS_BUILDING,
	AS_GENERIC
};

struct AAction
{
	ActionId id;
	ActionSource source;

	AAction(ActionId id, ActionSource source) : id(id), source(source)
	{

	}
};

extern const Action actions[];
