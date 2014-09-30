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

	Player() : Unit(&base_units[0]), exp(0), untaxed_gold(0), lvl(1), need_exp(100), potions(2), action(PA_None)
	{
		gold = 100;
		is_player = true;
	}

	virtual inline cstring GetText() const
	{
		return Format("%s  Hp: %d/%d\nLvl: %d   Exp: %d/%d\nAttack: %d  Defense: %d\nSpeed: %g / %g\nGold: %d (%d)\nPotions: %d\nAction: %s%s", base->name, hp, base->hp, lvl, exp, need_exp,
			base->attack, base->defense, base->move_speed, base->attack_speed, gold, untaxed_gold, potions, action_str[action], action != PA_None ? Format(" (%d%%)", GetProgress()) : "");
	}

	inline int GetProgress() const
	{
		return int(action_time/action_time_max*100);
	}

	inline void AddExp(int exp_lvl)
	{
		if(exp_lvl > lvl)
			exp_lvl += (exp_lvl-lvl);
		else if(exp_lvl < lvl)
			exp_lvl -= (lvl-exp_lvl);
		if(exp_lvl > 0)
			exp += exp_lvl*50;
		else
		{
			switch(exp_lvl)
			{
			case 0:
				exp += 25;
				break;
			case -1:
				exp += 12;
				break;
			case -2:
				exp += 6;
				break;
			case -3:
				exp += 3;
				break;
			case -4:
				++exp;
				break;
			default:
				return;
			}
		}
		while(exp >= need_exp)
		{
			exp -= need_exp;
			need_exp += 100;
			base->hp += 10;
			hp += 10;
			base->attack += 4;
			++base->defense;
			++lvl;
		}
	}
};
