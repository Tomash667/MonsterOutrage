#pragma once

#include "BaseUnit.h"

struct Unit;

struct UnitRef
{
	Unit* unit;
	uint refs;

	UnitRef() : unit(NULL), refs(0)
	{

	}

	inline Unit& GetRef()
	{
		assert(unit);
		return *unit;
	}
};

struct UnitRefTable
{
	uint last_index;
	vector<uint> empty_ids;
	vector<UnitRef> refs;

	inline void Init()
	{
		last_index = 0;
		refs.resize(64);
	}

	inline UnitRef* Add(Unit* u)
	{
		assert(u);
		UnitRef* _ref;
		if(!empty_ids.empty())
		{
			uint id = empty_ids.back();
			empty_ids.pop_back();
			_ref = &refs[id];
		}
		else
		{
			if(last_index >= 64u)
				throw "ERROR: No space in UnitRefTable!";
			_ref = &refs[last_index];
			++last_index;
		}
		_ref->unit = u;
		_ref->refs = 1;
		return _ref;
	}

	inline void Remove(UnitRef* _ref)
	{
		assert(_ref);
		--_ref->refs;
		assert(_ref->refs > 0 || !_ref->unit);
		if(_ref->refs == 0)
			empty_ids.push_back(GetIndex(_ref));
	}

	inline void Delete(UnitRef* _ref)
	{
		assert(_ref);
		_ref->unit = NULL;
		--_ref->refs;
		if(_ref->refs == 0)
			empty_ids.push_back(GetIndex(_ref));
	}

	inline uint GetIndex(UnitRef* _ref)
	{
		assert(_ref);
		int index = _ref - &refs[0];
		assert(index >= 0 && index < (int)last_index && &refs[index] == _ref);
		return (uint)index;
	}

};

extern UnitRefTable RefTable;

inline bool CheckRef(UnitRef*& _ref)
{
	if(!_ref)
		return false;
	else if(_ref->unit)
		return true;
	else
	{
		RefTable.Remove(_ref);
		_ref = NULL;
		return false;
	}
}

struct Unit
{
	/*const*/ BaseUnit* base;
	UnitRef* _ref;
	int hp, gold;
	INT2 pos, new_pos;
	DIR dir;
	float move_progress, waiting, attack_timer;
	bool moving, alive, is_player;
	Building* inside_building;

	Unit(/*const*/ BaseUnit* base) : moving(false), waiting(0.f), alive(true), is_player(false), base(base), hp(base->hp), gold(random(base->gold)), inside_building(NULL), attack_timer(0.f)
	{
		_ref = RefTable.Add(this);
	}

	virtual ~Unit()
	{
		RefTable.Delete(_ref);
	}

	inline INT2 GetPos() const
	{
		INT2 pt;
		if(moving)
		{
			pt.x = pos.x*32+int(move_progress*32*dir_change[dir].x);
			pt.y = pos.y*32+int(move_progress*32*dir_change[dir].y);
		}
		else
		{
			pt.x = pos.x*32;
			pt.y = pos.y*32;
		}
		return pt;
	}

	virtual inline cstring GetText() const
	{
		return Format("%s  Hp: %d/%d\nAttack: %d  Defense: %d\nSpeed: %g / %g\nGold: %d", base->name, hp, base->hp, base->attack, base->defense, base->move_speed, base->attack_speed, gold);
	}
};
