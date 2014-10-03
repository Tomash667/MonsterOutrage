#pragma once

enum PlayerAction
{
	PA_None,
	PA_Sleep,
	PA_DrinkBeer,
	PA_DrinkPotion,
	PA_Max
};

cstring action_str[PA_Max] = {
	"None",
	"Sleeping",
	"Drinking beer",
	"Drinking potion"
};

struct Player : public Unit
{
	int lvl, exp, need_exp, untaxed_gold, potions;
	float action_progress, action_time, action_time_max;
	PlayerAction action;

	Player() : Unit(&base_units[0]), exp(0), untaxed_gold(0), lvl(1), need_exp(1000), potions(2), action(PA_None)
	{
		gold = 100;
		is_player = true;
	}

	virtual inline cstring GetText() const
	{
		return Format("%s  Hp: %d/%d\nLvl: %d  Exp: %d/%d\nStr: %d  Vit: %d\nH2H: %d   Par: %d\nDmg: %d  Arm: %d\nSpeed: %g / %g\nGold: %d (%d)\nPotions: %d\nAction: %s%s",
			base->name, hp, hpmax, lvl, exp, need_exp, strength, vitality, melee_combat, parry, 
			CalculateDamage(), base->armor, base->move_speed, base->attack_speed, gold, untaxed_gold, potions,
			action_str[action], action != PA_None ? Format(" (%d%%)", GetProgress()) : "");
	}

	inline int GetProgress() const
	{
		return int(action_time/action_time_max*100);
	}

	inline void AddExp(int count)
	{
		exp += count;
		while(exp >= need_exp)
		{
			exp -= need_exp;
			++melee_combat;
			++parry;
			if(lvl%2 == 0)
				++strength;
			int gain = vitality/2;
			hp += gain;
			hpmax += gain;
			++lvl;
		}
	}

	inline int ExpFor(int exp_lvl)
	{
		if(exp_lvl > lvl)
			exp_lvl += (exp_lvl-lvl);
		else if(exp_lvl < lvl)
			exp_lvl -= (lvl-exp_lvl);
		if(exp_lvl > 0)
			return exp_lvl*150;
		else
		{
			switch(exp_lvl)
			{
			case 0:
				return 75;
			case -1:
				return 37;
				break;
			case -2:
				return 18;
			case -3:
				return 9;
			case -4:
				return 4;
			case -5:
				return 2;
			case -6:
				return 1;
			default:
				return 0;
			}
		}
	}

	inline void ExpForKill(int exp_lvl)
	{
		int count = ExpFor(exp_lvl);
		if(count)
			AddExp(count);
	}

	inline void ExpForDmg(const Unit& target, int dmg)
	{
		int count = ExpFor(target.base->lvl)*dmg/target.hpmax;
		if(count)
			AddExp(count);
	}
};
