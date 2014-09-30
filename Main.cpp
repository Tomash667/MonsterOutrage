#include "Pch.h"
#include "Input.h"
#include "Common.h"
#include "Building.h"
#include "Unit.h"
#include "Player.h"
#include "Tile.h"

const int VERSION = 1;
const int MAP_W = 20;
const int MAP_H = 61;
const int TAX = 10;
const INT2 SCREEN_SIZE(640, 640);

SDL_Window* window;
SDL_Renderer* renderer;

SDL_Texture* tTiles, *tUnit, *tHpBar, *tHit, *tSelected, *tBuilding, *tButton;
TTF_Font* font;

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
vector<AAction> a_actions;
UnitRefTable RefTable;
vector<Building*> buildings;

bool ActionPred(const AAction& a, const AAction& b)
{
	return actions[a.id].priority > actions[b.id].priority;
}

void SortActions()
{
	std::sort(a_actions.begin(), a_actions.end(), ActionPred);
}

void RemoveAction(ActionId id)
{
	for(vector<AAction>::iterator it = a_actions.begin(), end = a_actions.end(); it != end; ++it)
	{
		if(it->id == id)
		{
			a_actions.erase(it);
			break;
		}
	}
}

void RemoveActionSource(ActionSource source)
{
	for(vector<AAction>::iterator it = a_actions.begin(), end = a_actions.end(); it != end;)
	{
		if(it->source == source)
		{
			it = a_actions.erase(it);
			end = a_actions.end();
		}
		else
			++it;
	}
}

//=============================================================================
void InitSDL()
{
	printf("INFO: Initializing SLD.\n");

	if(SDL_Init(SDL_INIT_VIDEO) < 0)
		throw Format("ERROR: Failed to initialize SDL (%d).", SDL_GetError());

	window = SDL_CreateWindow(Format("Monster Outrage v %d", VERSION), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024, 768, SDL_WINDOW_SHOWN);
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
	buildings.push_back(b);
	M(5,1).PutBuilding(b, BT_WALL);
	M(6,1).PutBuilding(b, BT_SIGN);
	M(7,1).PutBuilding(b, BT_WALL);
	M(5,2).PutBuilding(b, BT_WALL);
	M(6,2).PutBuilding(b, BT_DOOR);
	M(7,2).PutBuilding(b, BT_WALL);
	M(6,3).type = 0;

	// marketplace
	b = new Building;
	b->base = &base_buildings[1];
	buildings.push_back(b);
	M(10,1).PutBuilding(b, BT_WALL);
	M(11,1).PutBuilding(b, BT_SIGN);
	M(12,1).PutBuilding(b, BT_WALL);
	M(10,2).PutBuilding(b, BT_WALL);
	M(11,2).PutBuilding(b, BT_DOOR);
	M(12,2).PutBuilding(b, BT_WALL);
	M(11,3).type = 0;

	// text width
	text[0].w = 300;
	text[1].w = 300;
	text[2].w = 600;
	text[3].w = 600;

	// available actions
	a_actions.push_back(AAction(Action_DrinkPotion, AS_ITEM));
}

//=============================================================================
void CleanGame()
{
	DeleteElements(buildings);
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
					r4.x = offset.x;
					r4.y = offset.y;
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

	// actions
	if(!a_actions.empty())
	{
		r.x = 32;
		r.y = 700;
		r.w = 16;
		r.h = 16;
		r4.w = 16;
		r4.h = 16;

		for(vector<AAction>::iterator it = a_actions.begin(), end = a_actions.end(); it != end; ++it)
		{
			const Action& a = actions[it->id];
			r4.x = a.offset.x;
			r4.y = a.offset.y;
			SDL_RenderCopy(renderer, tButton, &r4, &r);

			if(cursor_pos.x >= r.x && cursor_pos.x < r.x+16 && cursor_pos.y >= r.y && cursor_pos.y < r.y+16)
			{
				text[3].Set(a.desc);
				SDL_Rect r5;
				r5.x = 32;
				r5.y = 728;
				SDL_QueryTexture(text[3].tex, NULL, NULL, &r5.w, &r5.h);
				SDL_RenderCopy(renderer, text[3].tex, NULL, &r5);
			}
			r.x += 16;
		}
	}

	// building
	if(player && player->alive && player->inside_building)
	{
		// text
		text[2].Set(Format("Inside building: %s", player->inside_building->base->name));
		SDL_QueryTexture(text[2].tex, NULL, NULL, &r.w, &r.h);
		r.x = 32;
		r.y = 672;
		SDL_RenderCopy(renderer, text[2].tex, NULL, &r);
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
			if(u.inside_building)
			{
				if(u.is_player)
					RemoveActionSource(AS_BUILDING);
				u.inside_building = NULL;
			}
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
DIR GetDirKey()
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
			return keys2[i].dir;
	}
	for(int i=0; i<4; ++i)
	{
		if(KeyDown(keys1[i].k1) || KeyDown(keys1[i].k2))
			return keys1[i].dir;
	}
	
	return DIR_INVALID;
}

//=============================================================================
void TryExitBuilding()
{
	Unit& u = *player;
	if(!TryMove(u, DIR_S, false))
	{
		if(rand()%2 == 0)
		{
			if(!TryMove(u, DIR_SW, false))
				TryMove(u, DIR_SE, false);
		}
		else
		{
			if(!TryMove(u, DIR_SE, false))
				TryMove(u, DIR_SW, false);
		}
	}
}

//=============================================================================
void DoAction(ActionId id)
{
	const Action& a = actions[id];

	switch(id)
	{
	case Action_Sleep:
		if(player->action == PA_None && player->gold >= 10 && player->hp != player->base->hp)
		{
			player->gold -= 10;
			player->action = PA_Sleep;
			player->action_time = 0.f;
			player->action_time_max = float((player->base->hp - player->hp)/5+1);
			player->action_progress = 0.f;
			a_actions.push_back(AAction(Action_Cancel, AS_GENERIC));
			SortActions();
		}
		break;
	case Action_DrinkPotion:
		if(player->action == PA_None)
		{
			player->action = PA_DrinkPotion;
			player->action_time = 0.f;
			player->action_time_max = 1.f;
			a_actions.push_back(AAction(Action_Cancel, AS_GENERIC));
			SortActions();
		}
		break;
	case Action_BuyBeer:
		if(player->action == PA_None && player->gold >= 3)
		{
			player->gold -= 3;
			player->action = PA_DrinkBeer;
			player->action_time = 0.f;
			player->action_time_max = 10;
			a_actions.push_back(AAction(Action_Cancel, AS_GENERIC));
			SortActions();
		}
		break;
	case Action_Exit:
		TryExitBuilding();
		break;
	case Action_Cancel:
		player->action = PA_None;
		RemoveAction(Action_Cancel);
		break;
	case Action_BuyPotion:
		if(player->action == PA_None && player->gold >= 20)
		{
			player->gold -= 20;
			++player->potions;
			if(player->potions == 1)
			{
				a_actions.push_back(AAction(Action_DrinkPotion, AS_ITEM));
				SortActions();
			}
		}
		break;
	default:
		break;
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
	if(player && player->alive && !a_actions.empty())
	{
		INT2 pt(32,700);
		for(vector<AAction>::iterator it = a_actions.begin(), end = a_actions.end(); it != end; ++it)
		{
			if(cursor_pos.x >= pt.x && cursor_pos.y >= pt.y && cursor_pos.x < pt.x+16 && cursor_pos.y < pt.y+16)
			{
				if(MousePressedRelease(1))
					DoAction(it->id);
				break;
			}
			pt.x += 16;
		}
	}

	// update player
	if(player && player->alive && player->waiting <= 0.f && !player->moving)
	{
		Unit& u = *player;

		if(player->action != PA_None)
		{
			switch(player->action)
			{
			case PA_Sleep:
				player->action_progress += dt;
				if(player->action_progress >= 1.f)
				{
					player->hp += 5;
					player->action_progress -= 1.f;
					if(player->hp >= player->base->hp)
						player->hp = player->base->hp;
				}
				break;
			}
			
			player->action_time += dt;
			if(player->action_time >= player->action_time_max)
			{
				if(player->action == PA_DrinkPotion)
				{
					player->hp += 50;
					if(player->hp >= player->base->hp)
						player->hp = player->base->hp;
					--player->potions;
					if(player->potions == 0)
						RemoveAction(Action_DrinkPotion);
				}
				player->action = PA_None;
				RemoveAction(Action_Cancel);
			}
		}
		else if(u.inside_building)
		{
			// unit inside building, press SPACE to exit
			if(!mapa[u.pos.x+u.pos.y*MAP_W].unit)
			{
				if(KeyPressedRelease(SDL_SCANCODE_SPACE) || KeyPressedRelease(SDL_SCANCODE_KP_5))
					TryExitBuilding();
				else
				{
					DIR dir = GetDirKey();
					if(dir != DIR_INVALID)
						TryMove(u, dir, false);
				}
			}
		}
		else
		{
			DIR dir = GetDirKey();
			if(dir != DIR_INVALID)
				TryMove(u, dir, true);
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
					u.inside_building = tile.building;
					a_actions.push_back(AAction(Action_Exit, AS_BUILDING));
					const BaseBuilding& base = *tile.building->base;
					for(int i=0; i<BaseBuilding::MAX_ACTIONS; ++i)
					{
						if(base.actions[i] != Action_Invalid)
							a_actions.push_back(AAction(base.actions[i], AS_BUILDING));
					}
					SortActions();

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
