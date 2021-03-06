#include "Pch.h"
#include "Input.h"

const int VERSION = 0;
const int MAP_W = 20;
const int MAP_H = 61;
const int TAX = 10;
const INT2 SCREEN_SIZE(640, 640);

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

SDL_Window* window;
SDL_Renderer* renderer;

inline int Distance(const INT2& a, const INT2& b)
{
	int dist_x = abs(a.x - b.x),
		dist_y = abs(a.y - b.y);
	int smal = min(dist_x, dist_y);
	return smal*15 + (max(dist_x, dist_y)-smal)*10;
}

struct TileType
{
	SDL_Rect rect;
	bool block;
};

SDL_Texture* tTiles, *tUnit, *tHpBar, *tHit, *tSelected, *tBuilding, *tButton;
TTF_Font* font;
TileType tiles[3] = {
	0, 0, 32, 32, false,
	32, 0, 32, 32, true,
	0, 32, 32, 32, false
};
INT2 wall_offset(0,0), door_offset(1,0);

struct BaseUnit
{
	cstring name;
	int lvl, hp, attack, defense;
	float move_speed, attack_speed;
	INT2 offset, gold;
};

// this should be const but temporary we edit hero stats upon level up
/*const*/ BaseUnit base_units[] = {
	"Hero", 1, 100, 40, 5, 5.f, 1.f, INT2(0,0), INT2(0,0),
	"Goblin", 1, 50, 12, 0, 6.f, 1.2f, INT2(1,0), INT2(10,20),
	"Orc", 2, 75, 20, 5, 5.f, 0.9f, INT2(0,1), INT2(25,50),
	"Minotaur", 3, 120, 30, 10, 4.f, 0.95f, INT2(1,1), INT2(65,80)
};

struct BaseBuilding
{
	cstring name;
	INT2 offset;
};

const BaseBuilding base_buildings[] = {
	"Inn", INT2(0,1)
};

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

} RefTable;

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
	bool moving, alive, is_player, inside_building;

	Unit(/*const*/ BaseUnit* base) : moving(false), waiting(0.f), alive(true), is_player(false), base(base), hp(base->hp), gold(random(base->gold)), inside_building(false), attack_timer(0.f)
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

struct Player : public Unit
{
	int lvl, exp, need_exp, untaxed_gold;
	bool sleeping;
	float sleeping_progress;

	Player() : Unit(&base_units[0]), exp(0), untaxed_gold(0), lvl(1), need_exp(100), sleeping(false)
	{
		gold = 100;
		is_player = true;
	}

	virtual inline cstring GetText() const
	{
		return Format("%s  Hp: %d/%d\nLvl: %d   Exp: %d/%d\nAttack: %d  Defense: %d\nSpeed: %g / %g\nGold: %d (%d)", base->name, hp, base->hp, lvl, exp, need_exp, base->attack, base->defense,
			base->move_speed, base->attack_speed, gold, untaxed_gold);
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

struct Building
{
	const BaseBuilding* base;
};

enum BuildingTile
{
	BT_WALL,
	BT_DOOR,
	BT_SIGN
};

struct Tile
{
	UnitRef* unit;
	int type;
	struct 
	{
		Building* building;
		BuildingTile building_tile;
	};

	inline bool IsBlocking(bool is_player=false) const
	{
		if(building)
		{
			if(building_tile == BT_DOOR)
				return !is_player;
			return true;
		}
		else
			return tiles[type].block;
	}

	inline void PutBuilding(Building* b, BuildingTile bt)
	{
		assert(b);
		building = b;
		building_tile = bt;
	}
};

struct Hit
{
	INT2 pos;
	float timer;
	UnitRef* _ref;
};

struct Text
{
	string text;
	SDL_Texture* tex;
	int w;

	Text() : tex(NULL), w(200)
	{

	}

	void Set(cstring new_text)
	{
		if(text == new_text)
			return;
		text = new_text;
		if(tex)
			SDL_DestroyTexture(tex);
		SDL_Color color = {0,0,0};
		SDL_Surface* s = TTF_RenderText_Blended_Wrapped(font, text.c_str(), color, w);
		if(!s)
			throw Format("ERROR: Failed to render TTF text (%d).", TTF_GetError());
		tex = SDL_CreateTextureFromSurface(renderer, s);
		SDL_FreeSurface(s);
		if(!tex)
			throw Format("ERROR: Failed to convert text surface to texture (%d).", SDL_GetError());
	}

	void Delete()
	{
		if(tex)
			SDL_DestroyTexture(tex);
	}
};

INT2 cam_pos;
Tile mapa[MAP_W*MAP_H];
vector<Unit*> units;
vector<Hit> hits;
Player* player;
UnitRef* selected;
Text text[4];
Building* inn;

//=============================================================================
void InitSDL()
{
	printf("INFO: Initializing SLD.\n");

	if(SDL_Init(SDL_INIT_VIDEO) < 0)
		throw Format("ERROR: Failed to initialize SDL (%d).", SDL_GetError());

	window = SDL_CreateWindow("Monster Outrage v 0", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024, 768, SDL_WINDOW_SHOWN);
	if(!window)
		throw Format("ERROR: Failed to create window (%d).", SDL_GetError());

	renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if(!renderer)
		throw Format("ERROR: Failed to create renderer (%d).", SDL_GetError());

	int imgFlags = IMG_INIT_PNG;
	if(!(IMG_Init(imgFlags) & imgFlags))
		throw Format("ERROR: Failed to initialize SDL_image (%d).", IMG_GetError());

	SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);

	if(TTF_Init() == -1)
		throw Format("ERROR: Failed to initialize SDL_ttf (%d).", TTF_GetError());
}

//=============================================================================
void CleanSDL()
{
	printf("INFO: Cleaning SDL.\n");
	
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	TTF_Quit();
	IMG_Quit();
	SDL_Quit();
}

//=============================================================================
SDL_Texture* LoadTexture(cstring path)
{
	SDL_Surface* s = IMG_Load(path);
	if(!s)
		throw Format("ERROR: Failed to load image '%s' (%d).", path, IMG_GetError());
	SDL_Texture* t = SDL_CreateTextureFromSurface(renderer, s);
	SDL_FreeSurface(s);
	if(!t)
		throw Format("ERROR: Failed to convert surface '%s' to texture (%d).", SDL_GetError());
	return t;
}

//=============================================================================
void LoadMedia()
{
	tTiles = LoadTexture("data/textures/tile.png");
	tUnit = LoadTexture("data/textures/unit.png");
	tHpBar = LoadTexture("data/textures/hpbar.png");
	tHit = LoadTexture("data/textures/hit.png");
	tSelected = LoadTexture("data/textures/selected.png");
	tBuilding = LoadTexture("data/textures/building.png");
	tButton = LoadTexture("data/textures/button.png");

	font = TTF_OpenFont("data/HighlandGothicFLF.ttf", 16);
	if(!font)
		throw Format("ERROR: Failed to load font 'data/HighlandGothicFLF.ttf' (%d).", TTF_GetError());
}

//=============================================================================
void CleanMedia()
{
	for(int i=0; i<4; ++i)
		text[i].Delete();
	SDL_DestroyTexture(tTiles);
	SDL_DestroyTexture(tUnit);
	SDL_DestroyTexture(tHpBar);
	SDL_DestroyTexture(tHit);
	SDL_DestroyTexture(tSelected);
	SDL_DestroyTexture(tBuilding);
	SDL_DestroyTexture(tButton);
	TTF_CloseFont(font);
}

//=============================================================================
void InitGame()
{
	RefTable.Init();
	srand((uint)time(NULL));

	// map
	for(int i=0; i<MAP_W*MAP_H; ++i)
	{
		int c = rand()%8;
		Tile& tile = mapa[i];
		tile.unit = NULL;
		tile.building = NULL;
		if(c == 0)
			tile.type = 1;
		else if(c == 1 || c == 2)
			tile.type = 2;
		else
			tile.type = 0;
	}
	
	// player
	player = new Player;
	player->pos = INT2(0,0);
	units.push_back(player);
	mapa[0].type = 0;
	mapa[0].unit = player->_ref;

	// enemies
	struct SpawnGroup
	{
		int line, enemy[3], drop;
	};

	const SpawnGroup groups[] = {
		11, 2, 0, 0, 2,
		15, 3, 0, 0, 2,
		19, 0, 1, 0, 0,
		25, 2, 1, 0, 2,
		29, 0, 2, 0, 1,
		35, 1, 2, 0, 2,
		39, 2, 2, 0, 2,
		45, 0, 3, 0, 1,
		50, 0, 0, 1, 0,
		55, 0, 2, 1, 1,
		60, 0, 0, 2, 0
	};
	const int count = countof(groups);

	for(int i=0; i<11; ++i)
	{
		const SpawnGroup& group = groups[i];
		for(int j=0; j<3; ++j)
		{
			int enemy_count = group.enemy[j];
			while(enemy_count)
			{
				INT2 pt(rand()%MAP_W, group.line);
				if(group.drop)
					pt.y += random(-group.drop, group.drop);
				Tile& tile = mapa[pt.x+pt.y*MAP_W];
				if(tile.type != 1 && !tile.unit)
				{
					--enemy_count;
					Unit* enemy = new Unit(&base_units[j+1]);
					enemy->pos = pt;
					units.push_back(enemy);
					tile.unit = enemy->_ref;
				}
			}
		}
	}
	
	// inn
#define M(x,y) mapa[(x)+(y)*MAP_W]

	Building* b = new Building;
	b->base = &base_buildings[0];
	M(5,1).PutBuilding(b, BT_WALL);
	M(6,1).PutBuilding(b, BT_SIGN);
	M(7,1).PutBuilding(b, BT_WALL);
	M(5,2).PutBuilding(b, BT_WALL);
	M(6,2).PutBuilding(b, BT_DOOR);
	M(7,2).PutBuilding(b, BT_WALL);
	M(6,3).type = 0;

	inn = b;

	// text width
	text[0].w = 300;
	text[1].w = 300;
	text[2].w = 600;
	text[3].w = 600;
}

//=============================================================================
void CleanGame()
{
	delete inn;
	DeleteElements(units);
}

//=============================================================================
void Draw()
{
	// clear render
	SDL_RenderClear(renderer);

	// viewport
	SDL_Rect view;
	view.w = 32*20;
	view.h = 32*20;
	view.x = 0;
	view.y = 0;
	SDL_RenderSetViewport(renderer, &view);
	
	// tiles
	SDL_Rect r, r4;
	r4.w = r.w = 32;
	r4.h = r.h = 32;
	int start_x = cam_pos.x/32,
		start_y = cam_pos.y/32,
		end_x = min((cam_pos.x+SCREEN_SIZE.x)/32+1, MAP_W),
		end_y = min((cam_pos.y+SCREEN_SIZE.y)/32+1, MAP_H);
	for(int y=start_y; y<end_y; ++y)
	{
		for(int x=start_x; x<end_x; ++x)
		{
			r.x = x*32 - cam_pos.x;
			r.y = y*32 - cam_pos.y;
			Tile& tile = mapa[x+y*MAP_W];
			if(tile.building)
			{
				if(tile.building_tile == BT_DOOR)
				{
					r4.x = door_offset.x*32;
					r4.y = door_offset.y*32;
				}
				else if(tile.building_tile == BT_WALL)
				{
					r4.x = wall_offset.x*32;
					r4.y = wall_offset.y*32;
				}
				else
				{
					r4.x = wall_offset.x*32;
					r4.y = wall_offset.y*32;
					SDL_RenderCopy(renderer, tBuilding, &r4, &r);
					const INT2& offset = tile.building->base->offset;
					r4.x = offset.x*32;
					r4.y = offset.y*32;
				}
				SDL_RenderCopy(renderer, tBuilding, &r4, &r);
			}
			else
				SDL_RenderCopy(renderer, tTiles, &tiles[mapa[x+y*MAP_W].type].rect, &r);
		}
	}
	
	// unit
	for(vector<Unit*>::iterator it = units.begin(), end = units.end(); it != end; ++it)
	{
		Unit& u = **it;
		if(u.inside_building)
			continue;
		INT2 pos = u.GetPos();
		r.x = pos.x-cam_pos.x;
		r.y = pos.y-cam_pos.y;
		SDL_Rect rr;
		rr.w = 32;
		rr.h = 32;
		rr.x = u.base->offset.x*32;
		rr.y = u.base->offset.y*32;
		SDL_RenderCopy(renderer, tUnit, &rr, &r);
		
		// hp bar
		if(u.hp != u.base->hp)
		{
			Uint8 cr, g, b = 0;
			float hpp = float(u.hp)/u.base->hp;
			if(hpp <= 0.5f)
			{
				float t = hpp*2;
				cr = lerp(255, 255, t);
				g = lerp(0, 216, t);
			}
			else
			{
				float t = (hpp-0.5f)*2;
				cr = lerp(255, 76, t);
				g = lerp(216, 255, t);
			}
			SDL_SetTextureColorMod(tHpBar, cr, g, b);
			SDL_Rect r2;
			r2.w = int(float(u.hp)/u.base->hp*32);
			if(r2.w <= 0)
				r2.w = 1;
			r2.h = 2;
			r2.x = 0;
			r2.y = 0;
			SDL_Rect r3;
			r3.w = r2.w;
			r3.h = r2.h;
			r3.x = r.x;
			r3.y = r.y;
			SDL_RenderCopy(renderer, tHpBar, &r2, &r3);
		}
	}

	// attack image
	for(vector<Hit>::iterator it = hits.begin(), end = hits.end(); it != end; ++it)
	{
		r.x = it->pos.x - cam_pos.x;
		r.y = it->pos.y - cam_pos.y;
		SDL_RenderCopy(renderer, tHit, NULL, &r);
	}

	// selected unit border
	if(CheckRef(selected))
	{
		INT2 pos = selected->GetRef().GetPos();
		r.x = pos.x - cam_pos.x;
		r.y = pos.y - cam_pos.y;
		SDL_RenderCopy(renderer, tSelected, NULL, &r);
	}

	SDL_RenderSetViewport(renderer, NULL);
	
	// text
	if(player)
		text[0].Set(player->GetText());
	SDL_QueryTexture(text[0].tex, NULL, NULL, &r.w, &r.h);
	r.x = 32*(MAP_W+1);
	r.y = 32;
	SDL_RenderCopy(renderer, text[0].tex, NULL, &r);

	// selected unit text
	if(CheckRef(selected))
	{
		text[1].Set(selected->GetRef().GetText());
		SDL_QueryTexture(text[1].tex, NULL, NULL, &r.w, &r.h);
		r.y = 232;
		SDL_RenderCopy(renderer, text[1].tex, NULL, &r);
	}

	// building
	if(player && player->alive && player->inside_building)
	{
		// text
		text[2].Set(Format("Inside building: %s", inn->base->name));
		SDL_QueryTexture(text[2].tex, NULL, NULL, &r.w, &r.h);
		r.x = 32;
		r.y = 672;
		SDL_RenderCopy(renderer, text[2].tex, NULL, &r);

		// button
		r.x = 32;
		r.y = 700;
		r.w = 16;
		r.h = 16;
		r4.x = 0;
		r4.y = 0;
		r4.w = 16;
		r4.h = 16;
		SDL_RenderCopy(renderer, tButton, &r4, &r);

		// button text
		bool draw_text = false;
		if(cursor_pos.x >= 32 && cursor_pos.y >= 700 && cursor_pos.x < 32+16 && cursor_pos.y < 700+16)
		{
			draw_text = true;
			text[3].Set("Sleep inside inn for 10 gp.");
		}

		if(draw_text)
		{
			SDL_QueryTexture(text[3].tex, NULL, NULL, &r.w, &r.h);
			r.x = 32;
			r.y = 728;
			SDL_RenderCopy(renderer, text[3].tex, NULL, &r);
		}
	}

	// display render
	SDL_RenderPresent(renderer);
}

//=============================================================================
bool TryMove(Unit& u, DIR new_dir, bool attack)
{
	u.new_pos = u.pos + dir_change[new_dir];
	if(u.new_pos.x >= 0 && u.new_pos.y >= 0 && u.new_pos.x < MAP_W && u.new_pos.y < MAP_H)
	{
		Tile& tile = mapa[u.new_pos.x+u.new_pos.y*MAP_W];
		if(tile.IsBlocking(u.is_player))
			return false;
		else if(tile.unit)
		{
			if(CheckRef(tile.unit) && attack && tile.unit->GetRef().alive)
			{
				// attack
				if(u.attack_timer <= 0.f)
				{
					Unit& target = tile.unit->GetRef();
					Hit& hit = Add1(hits);
					hit.pos = target.GetPos();
					hit.timer = 0.5f;
					hit._ref = tile.unit;
					u.waiting = 0.5f;
					u.attack_timer = 1.f/u.base->attack_speed;
					target.hp -= (u.base->attack - target.base->defense);
					if(target.hp <= 0)
					{
						target.alive = false;
						player->untaxed_gold += target.gold;
						player->AddExp(target.base->lvl);
					}
				}
				return true;
			}
			else
				return false;
		}
		else
		{
			if(new_dir == DIR_NE || new_dir == DIR_NW || new_dir == DIR_SE || new_dir == DIR_SW)
			{
				if(mapa[u.pos.x+dir_change[new_dir].x+u.pos.y*MAP_W].IsBlocking() &&
					mapa[u.pos.x+(u.pos.y+dir_change[new_dir].y)*MAP_W].IsBlocking())
					return false;
			}

			u.dir = new_dir;
			u.moving = true;
			u.move_progress = 0.f;
			mapa[u.new_pos.x+u.new_pos.y*MAP_W].unit = u._ref;
			return true;
		}
	}

	return false;
}

//=============================================================================
int GetClosestPoint(const INT2& my_pos, const Unit& target, INT2& closest)
{
	if(target.moving)
	{
		int dist1 = Distance(my_pos, target.pos),
			dist2 = Distance(my_pos, target.new_pos);
		if(dist1 >= dist2)
		{
			closest = target.new_pos;
			return dist2;
		}
		else
		{
			closest = target.pos;
			return dist1;
		}
	}
	else
	{
		closest = target.pos;
		return Distance(my_pos, closest);
	}
}

//=============================================================================
DIR GetDir(const INT2& a, const INT2& b)
{
	if(a.x > b.x)
	{
		if(a.y > b.y)
			return DIR_NW;
		else if(a.y < b.y)
			return DIR_SW;
		else
			return DIR_W;
	}
	else if(a.x < b.x)
	{
		if(a.y > b.y)
			return DIR_NE;
		else if(a.y < b.y)
			return DIR_SE;
		else
			return DIR_E;
	}
	else
	{
		if(a.y > b.y)
			return DIR_N;
		else
			return DIR_S;
	}
}

//=============================================================================
void Update(float dt)
{
	// selecting unit
	INT2 tile((cursor_pos.x + cam_pos.x)/32, (cursor_pos.y + cam_pos.y)/32);
	if(tile.x >= 0 && tile.y >= 0 && tile.x < MAP_W && tile.y < MAP_W && MousePressedRelease(1))
	{
		Tile& t = mapa[tile.x+tile.y*MAP_W];
		selected = t.unit;
	}

	// sleep button
	if(player && player->alive && player->inside_building && cursor_pos.x >= 32 && cursor_pos.y >= 700 && cursor_pos.x < 32+16 && cursor_pos.y < 700+16 && MousePressedRelease(1))
	{
		if(player->gold >= 10 && !player->sleeping && player->hp != player->base->hp)
		{
			player->gold -= 10;
			player->sleeping = true;
			player->sleeping_progress = 0.f;
		}
	}

	// update player
	if(player && player->alive && player->waiting <= 0.f && !player->moving)
	{
		Unit& u = *player;

		if(player->sleeping)
		{
			player->sleeping_progress += dt;
			if(player->sleeping_progress >= 1.f)
			{
				player->hp += 5;
				player->sleeping_progress = 0.f;
				if(player->hp >= player->base->hp)
				{
					player->hp = player->base->hp;
					player->sleeping = false;
				}
			}
		}
		else if(u.inside_building)
		{
			// unit inside building, press SPACE to exit
			if(KeyPressedRelease(SDL_SCANCODE_SPACE) && !mapa[u.pos.x+u.pos.y*MAP_W].unit)
			{
				if(!TryMove(u, DIR_S, false))
				{
					if(rand()%2 == 0)
					{
						if(!TryMove(u, DIR_SW, false))
						{
							if(TryMove(u, DIR_SE, false))
								u.inside_building = false;
						}
						else
							u.inside_building = false;
					}
					else
					{
						if(!TryMove(u, DIR_SE, false))
						{
							if(TryMove(u, DIR_SW, false))
								u.inside_building = false;
						}
						else
							u.inside_building = false;
					}
				}
				else
					u.inside_building = false;
			}
		}
		else
		{
			struct Key1
			{
				SDL_Scancode k1;
				SDL_Scancode k2;
				DIR dir;
			};
			const Key1 keys1[] = {
				SDL_SCANCODE_LEFT, SDL_SCANCODE_KP_4, DIR_W,
				SDL_SCANCODE_RIGHT, SDL_SCANCODE_KP_6, DIR_E,
				SDL_SCANCODE_UP, SDL_SCANCODE_KP_8, DIR_N,
				SDL_SCANCODE_DOWN, SDL_SCANCODE_KP_2, DIR_S
			};
			struct Key2
			{
				SDL_Scancode k1;
				SDL_Scancode k2;
				SDL_Scancode k3;
				DIR dir;
			};
			const Key2 keys2[] = {
				SDL_SCANCODE_KP_1, SDL_SCANCODE_LEFT, SDL_SCANCODE_DOWN, DIR_SW,
				SDL_SCANCODE_KP_3, SDL_SCANCODE_RIGHT, SDL_SCANCODE_DOWN, DIR_SE,
				SDL_SCANCODE_KP_7, SDL_SCANCODE_LEFT, SDL_SCANCODE_UP, DIR_NW,
				SDL_SCANCODE_KP_9, SDL_SCANCODE_RIGHT, SDL_SCANCODE_UP, DIR_NE
			};

			for(int i=0; i<4; ++i)
			{
				if(KeyDown(keys2[i].k1) || (KeyDown(keys2[i].k2) && KeyDown(keys2[i].k3)))
				{
					if(TryMove(u, keys2[i].dir, true))
						break;
				}
			}
			if(!u.moving && u.waiting <= 0.f)
			{
				for(int i=0; i<4; ++i)
				{
					if(KeyDown(keys1[i].k1) || KeyDown(keys1[i].k2))
					{
						if(TryMove(u, keys1[i].dir, true))
							break;
					}
				}
			}
		}
	}

	// update ai
	for(vector<Unit*>::iterator it = units.begin(), end = units.end(); it != end; ++it)
	{
		Unit& u = **it;
		u.attack_timer -= dt;
		if(u.hp <= 0)
		{
			if(&u == player)
				player = NULL;
			if(u.moving)
				mapa[u.new_pos.x+u.new_pos.y*MAP_W].unit = NULL;
			mapa[u.pos.x+u.pos.y*MAP_W].unit = NULL;
			delete &u;
			it = units.erase(it);
			end = units.end();
			if(it == end)
				break;
		}
		else if(u.waiting > 0.f)
			u.waiting -= dt;
		else if(u.moving)
		{
			u.move_progress += dt*u.base->move_speed;
			if(u.move_progress >= 1.f)
			{
				u.moving = false;
				mapa[u.pos.x+u.pos.y*MAP_W].unit = NULL;
				u.pos = u.new_pos;
				Tile& tile = mapa[u.pos.x+u.pos.y*MAP_W];
				if(tile.building)
				{
					tile.unit = NULL;
					u.inside_building = true;
					if(player->untaxed_gold)
					{
						int tax = player->untaxed_gold*TAX/100;
						player->untaxed_gold -= tax;
						player->gold += player->untaxed_gold;
						player->untaxed_gold = 0;
					}
				}
			}
		}
		else if(&u != player && player && player->alive && !player->inside_building)
		{
			INT2 enemy_pt;
			int dist = GetClosestPoint(u.pos, *player, enemy_pt);
			if(dist <= 50)
			{
				if(dist <= 15)
				{
					// in attack range
					if(u.attack_timer <= 0.f)
					{
						Hit& hit = Add1(hits);
						hit.pos = player->GetPos();
						hit.timer = 0.5f;
						hit._ref = player->_ref;
						u.waiting = 0.5f;
						u.attack_timer = 1.f/u.base->attack_speed;
						player->hp -= u.base->attack - player->base->defense;
						if(player->hp <= 0)
						{
							player->alive = false;
							u.gold += (player->gold + player->untaxed_gold);
							player->gold = 0;
							player->untaxed_gold = 0;
						}
					}
				}
				else
				{
					DIR dir = GetDir(u.pos, enemy_pt);
					if(!TryMove(u, dir, false))
					{
						struct SubDir
						{
							DIR a, b;
						};
						const SubDir sub_dir[8] = {
							DIR_SE, DIR_SW, //DIR_S,
							DIR_W, DIR_S, //DIR_SW,
							DIR_NW, DIR_SW, //DIR_W,
							DIR_N, DIR_W, //DIR_NW,
							DIR_NE, DIR_NW, //DIR_N,
							DIR_N, DIR_E, //DIR_NE,
							DIR_SE, DIR_NE, //DIR_E,
							DIR_S, DIR_E //DIR_SE,
						};
						if(rand()%2 == 0)
						{
							if(!TryMove(u, sub_dir[dir].a, false))
								TryMove(u, sub_dir[dir].b, false);
						}
						else
						{
							if(!TryMove(u, sub_dir[dir].b, false))
								TryMove(u, sub_dir[dir].a, false);
						}
					}
				}
			}
		}
	}

	// update hit animations
	for(vector<Hit>::iterator it = hits.begin(), end = hits.end(); it != end;)
	{
		Hit& hit = *it;
		hit.timer -= dt;
		if(hit.timer <= 0.f)
		{
			if(it+1 == end)
			{
				hits.pop_back();
				break;
			}
			else
			{
				std::iter_swap(it, end-1);
				hits.pop_back();
				end = hits.end();
			}
		}
		else
		{
			if(CheckRef(hit._ref))
				hit.pos = hit._ref->GetRef().GetPos();
			++it;
		}
	}

	// set camera
	if(player)
	{
		cam_pos = player->GetPos() - SCREEN_SIZE/2;
		if(cam_pos.x < 0)
			cam_pos.x = 0;
		if(cam_pos.y < 0)
			cam_pos.y = 0;
		if(cam_pos.x + SCREEN_SIZE.x > MAP_W*32)
			cam_pos.x = MAP_W*32-SCREEN_SIZE.x;
		if(cam_pos.y + SCREEN_SIZE.y > MAP_H*32)
			cam_pos.y = MAP_H*32-SCREEN_SIZE.y;
	}
}

//=============================================================================
int main(int argc, char *argv[])
{
	printf("INFO: Monster Outrage v %d\n", VERSION);

	try
	{
		InitSDL();
		LoadMedia();
		InitGame();

		bool quit = false;
		uint ticks = SDL_GetTicks();

		while(!quit)
		{
			// handle events
			SDL_Event e;
			while(SDL_PollEvent(&e) != 0)
			{
				if(e.type == SDL_QUIT)
					quit = true;
				else if(e.type == SDL_KEYDOWN)
				{
					if(e.key.state == SDL_PRESSED)
						ProcessKey(e.key.keysym.scancode, true);
				}
				else if(e.type == SDL_KEYUP)
				{
					if(e.key.state == SDL_RELEASED)
						ProcessKey(e.key.keysym.scancode, false);
				}
				else if(e.type == SDL_MOUSEMOTION || e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP)
				{
					SDL_GetMouseState(&cursor_pos.x, &cursor_pos.y);
					if(e.type == SDL_MOUSEBUTTONDOWN)
					{
						if(e.button.state == SDL_PRESSED && e.button.button < MAX_BUTTON)
							ProcessButton(e.button.button, true);
					}
					else if(e.type == SDL_MOUSEBUTTONUP)
					{
						if(e.button.state == SDL_RELEASED && e.button.button < MAX_BUTTON)
							ProcessButton(e.button.button, false);
					}
				}
			}

			// calculate dt
			uint new_ticks = SDL_GetTicks();
			float dt = float(new_ticks - ticks)/1000.f;
			ticks = new_ticks;

			Draw();
			Update(dt);
			UpdateInput();
		}

		CleanGame();
		CleanMedia();
		CleanSDL();
	}
	catch(cstring err)
	{
		printf("%s\nERROR: Read error and press any key to exit.\n", err);
		_getch();
		return 1;
	}

	return 0;
}
