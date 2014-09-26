#include "Pch.h"
#include "Base.h"

const int VERSION = 0;
const int MAP_SIZE = 20;

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

enum InputState
{
	IS_UP,			// 00
	IS_RELEASED,	// 01
	IS_DOWN,		// 10
	IS_PRESSED		// 11
};

SDL_Window* window;
SDL_Renderer* renderer;
INT2 cursor_pos;
byte keystate[SDL_NUM_SCANCODES];
const uint MAX_BUTTON = 8;
byte mousestate[MAX_BUTTON];

inline bool KeyDown(int key)
{
	return IS_SET(keystate[key], IS_DOWN);
}

inline bool MousePressedRelease(int bt)
{
	if(IS_ALL_SET(mousestate[bt], IS_PRESSED))
	{
		mousestate[bt] = IS_DOWN;
		return true;
	}
	else
		return false;
}

inline int Distance(const INT2& a, const INT2& b)
{
	int dist_x = abs(a.x - b.x),
		dist_y = abs(a.y - b.y);
	int smal = min(dist_x, dist_y);
	return smal*15 + (max(dist_x, dist_y)-smal)*10;
}

SDL_Texture* tTiles, *tUnit, *tHpBar, *tHit, *tSelected;
TTF_Font* font;
SDL_Rect tiles[2];

struct BaseUnit
{
	cstring name;
	int hp, attack, defense;
	float move_speed, attack_speed;
	INT2 offset;
};

const BaseUnit base_units[] = {
	"Hero", 100, 40, 5, 5.f, 1.f, INT2(0,0),
	"Goblin", 50, 12, 0, 6.f, 1.2f, INT2(1,0),
	"Orc", 75, 20, 5, 5.f, 0.9f, INT2(0,1)
};

struct Unit
{
	const BaseUnit* base;
	int hp;
	INT2 pos, new_pos;
	DIR dir;
	float move_progress, waiting;
	bool moving;

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

	inline cstring GetText() const
	{
		return Format("%s  Hp: %d/%d\nAttack: %d  Defense: %d\nSpeed: %g / %g", base->name, hp, base->hp, base->attack, base->defense, base->move_speed, base->attack_speed);
	}
};

struct Tile
{
	Unit* unit;
	bool blocked;
};

struct Hit
{
	INT2 pos;
	float timer;
};

struct Text
{
	string text;
	SDL_Texture* tex;

	Text() : tex(NULL)
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
		SDL_Surface* s = TTF_RenderText_Blended_Wrapped(font, text.c_str(), color, 200);
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

Tile mapa[MAP_SIZE*MAP_SIZE];
vector<Unit*> units;
vector<Hit> hits;
Unit* player, *selected;
Text text[2];

//=============================================================================
void InitSDL()
{
	printf("INFO: Initializing SLD.");

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
	printf("INFO: Cleaning SDL.");
	
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

	for(int i=0; i<2; ++i)
	{
		tiles[i].w = 32;
		tiles[i].h = 32;
	}
	tiles[0].x = 0;
	tiles[0].y = 0;
	tiles[1].x = 32;
	tiles[1].y = 0;

	font = TTF_OpenFont("data/HighlandGothicFLF.ttf", 16);
	if(!font)
		throw Format("ERROR: Failed to load font 'data/HighlandGothicFLF.ttf' (%d).", TTF_GetError());
}

//=============================================================================
void CleanMedia()
{
	for(int i=0; i<2; ++i)
		text[i].Delete();
	SDL_DestroyTexture(tTiles);
	SDL_DestroyTexture(tUnit);
	SDL_DestroyTexture(tHpBar);
	SDL_DestroyTexture(tHit);
	SDL_DestroyTexture(tSelected);
	TTF_CloseFont(font);
}

//=============================================================================
void InitGame()
{
	srand((uint)time(NULL));
	for(int i=0; i<MAP_SIZE*MAP_SIZE; ++i)
		mapa[i].blocked = (rand()%8 == 0);
	
	player = new Unit;
	player->base = &base_units[0];
	player->pos = INT2(0,0);
	player->hp = player->base->hp;
	player->moving = false;
	player->waiting = 0.f;
	units.push_back(player);
	mapa[0].blocked = false;
	mapa[0].unit = player;

	Unit* enemy;
	
	for(int i=0; i<3; ++i)
	{
		enemy = new Unit;
		enemy->base = &base_units[1];
		enemy->pos = INT2(8+rand()%8,8+rand()%8);
		enemy->hp = enemy->base->hp;
		enemy->moving = false;
		enemy->waiting = false;
		units.push_back(enemy);
		Tile& tile = mapa[enemy->pos.x+enemy->pos.y*MAP_SIZE];
		tile.blocked = false;
		tile.unit = enemy;
	}

	enemy = new Unit;
	enemy->base = &base_units[2];
	enemy->pos = INT2(19,19);
	enemy->hp = enemy->base->hp;
	enemy->moving = false;
	enemy->waiting = false;
	units.push_back(enemy);
	Tile& tile = mapa[enemy->pos.x+enemy->pos.y*MAP_SIZE];
	tile.blocked = false;
	tile.unit = enemy;
}

//=============================================================================
void Draw()
{
	// clear render
	SDL_RenderClear(renderer);
	
	// tiles
	SDL_Rect r;
	r.w = 32;
	r.h = 32;
	for(int yy=0; yy<MAP_SIZE; ++yy)
	{
		for(int xx=0; xx<MAP_SIZE; ++xx)
		{
			r.x = xx*32;
			r.y = yy*32;
			SDL_RenderCopy(renderer, tTiles, &tiles[mapa[xx+yy*MAP_SIZE].blocked ? 1 : 0], &r);
		}
	}
	
	// unit
	for(vector<Unit*>::iterator it = units.begin(), end = units.end(); it != end; ++it)
	{
		Unit& u = **it;
		INT2 pos = u.GetPos();
		r.x = pos.x;
		r.y = pos.y;
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
			r2.w = int(float(u.hp)/100*32);
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
		r.x = it->pos.x;
		r.y = it->pos.y;
		SDL_RenderCopy(renderer, tHit, NULL, &r);
	}

	// selected unit border
	if(selected)
	{
		INT2 pos = selected->GetPos();
		r.x = pos.x;
		r.y = pos.y;
		SDL_RenderCopy(renderer, tSelected, NULL, &r);
	}
	
	// text
	if(player)
		text[0].Set(player->GetText());
	SDL_QueryTexture(text[0].tex, NULL, NULL, &r.w, &r.h);
	r.x = 32*(MAP_SIZE+1);
	r.y = 32;
	SDL_RenderCopy(renderer, text[0].tex, NULL, &r);

	// selected unit text
	if(selected)
	{
		text[1].Set(selected->GetText());
		SDL_QueryTexture(text[1].tex, NULL, NULL, &r.w, &r.h);
		r.y = 232;
		SDL_RenderCopy(renderer, text[1].tex, NULL, &r);
	}

	// display render
	SDL_RenderPresent(renderer);
}

//=============================================================================
bool TryMove(Unit& u, DIR new_dir, bool attack)
{
	u.new_pos = u.pos + dir_change[new_dir];
	if(u.new_pos.x >= 0 && u.new_pos.y >= 0 && u.new_pos.x < MAP_SIZE && u.new_pos.y < MAP_SIZE)
	{
		Tile& tile = mapa[u.new_pos.x+u.new_pos.y*MAP_SIZE];
		if(tile.blocked)
			return false;
		else if(tile.unit)
		{
			if(attack)
			{
				// attack
				Hit& hit = Add1(hits);
				hit.pos = tile.unit->GetPos();
				hit.timer = 0.5f;
				u.waiting = 1.f/u.base->attack_speed;
				tile.unit->hp -= (u.base->attack - tile.unit->base->defense);
				return true;
			}
			else
				return false;
		}
		else
		{
			if(new_dir == DIR_NE || new_dir == DIR_NW || new_dir == DIR_SE || new_dir == DIR_SW)
			{
				if(mapa[u.pos.x+dir_change[new_dir].x+u.pos.y*MAP_SIZE].blocked &&
					mapa[u.pos.x+(u.pos.y+dir_change[new_dir].y)*MAP_SIZE].blocked)
					return false;
			}

			u.dir = new_dir;
			u.moving = true;
			u.move_progress = 0.f;
			mapa[u.new_pos.x+u.new_pos.y*MAP_SIZE].unit = &u;
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
	if(MousePressedRelease(1))
	{
		INT2 tile(cursor_pos.x/32, cursor_pos.y/32);
		if(tile.x >= 0 && tile.y >= 0 && tile.x < MAP_SIZE && tile.y < MAP_SIZE)
		{
			Tile& t = mapa[tile.x+tile.y*MAP_SIZE];
			selected = t.unit;
		}
	}

	// update player
	if(player && player->waiting <= 0.f && !player->moving)
	{
		Unit& u = *player;

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

	// update ai
	for(vector<Unit*>::iterator it = units.begin(), end = units.end(); it != end; ++it)
	{
		Unit& u = **it;
		if(u.hp <= 0)
		{
			if(&u == player)
				player = NULL;
			if(&u == selected)
				selected = NULL;
			if(u.moving)
				mapa[u.new_pos.x+u.new_pos.y*MAP_SIZE].unit = NULL;
			mapa[u.pos.x+u.pos.y*MAP_SIZE].unit = NULL;
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
				mapa[u.pos.x+u.pos.y*MAP_SIZE].unit = NULL;
				u.pos = u.new_pos;
			}
		}
		else if(&u != player && player)
		{
			INT2 enemy_pt;
			int dist = GetClosestPoint(u.pos, *player, enemy_pt);
			if(dist <= 50)
			{
				if(dist <= 15)
				{
					// in attack range
					Hit& hit = Add1(hits);
					hit.pos = player->GetPos();
					hit.timer = 0.5f;
					u.waiting = 1.f/u.base->attack_speed;
					player->hp -= u.base->attack - player->base->defense;
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
							DIR_NW, DIR_NE, //DIR_W,
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
		it->timer -= dt;
		if(it->timer <= 0.f)
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
			++it;
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
					{
						if(keystate[e.key.keysym.scancode] <= IS_RELEASED)
							keystate[e.key.keysym.scancode] = IS_PRESSED;
					}
				}
				else if(e.type == SDL_KEYUP)
				{
					if(e.key.state == SDL_RELEASED)
					{
						if(keystate[e.key.keysym.scancode] >= IS_DOWN)
							keystate[e.key.keysym.scancode] = IS_RELEASED;
					}
				}
				else if(e.type == SDL_MOUSEMOTION || e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP)
				{
					SDL_GetMouseState(&cursor_pos.x, &cursor_pos.y);
					if(e.type == SDL_MOUSEBUTTONDOWN)
					{
						if(e.button.state == SDL_PRESSED && e.button.button < MAX_BUTTON)
						{
							if(mousestate[e.button.button] <= IS_RELEASED)
								mousestate[e.button.button] = IS_PRESSED;
						}
					}
					else if(e.type == SDL_MOUSEBUTTONUP)
					{
						if(e.button.state == SDL_RELEASED && e.button.button < MAX_BUTTON)
						{
							if(mousestate[e.button.button] >= IS_DOWN)
								mousestate[e.button.button] = IS_RELEASED;
						}
					}
				}
			}

			// calculate dt
			uint new_ticks = SDL_GetTicks();
			float dt = float(new_ticks - ticks)/1000.f;
			ticks = new_ticks;

			Draw();
			Update(dt);

			// update input state
			for(uint i=0; i < SDL_NUM_SCANCODES; ++i)
			{
				if(keystate[i] & 1)
					--keystate[i];
			}
			for(uint i=0; i< MAX_BUTTON; ++i)
			{
				if(mousestate[i] & 1)
					--mousestate[i];
			}
		}

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
