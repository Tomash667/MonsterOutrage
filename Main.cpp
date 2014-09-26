#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <cstdio>
#include <conio.h>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <algorithm>

using std::vector;

typedef const char* cstring;
typedef unsigned char byte;
typedef unsigned int uint;

cstring Format(cstring str, ...)
{
	const uint FORMAT_STRINGS = 8;
	const uint FORMAT_LENGTH = 2048;

	assert(str);

	static char buf[FORMAT_STRINGS][FORMAT_LENGTH];
	static int marker = 0;

	va_list list;
	va_start(list,str);
	_vsnprintf_s((char*)buf[marker],FORMAT_LENGTH,FORMAT_LENGTH-1,str,list);
	char* cbuf = buf[marker];
	cbuf[FORMAT_LENGTH-1] = 0;

	marker = (marker+1)%FORMAT_STRINGS;

	return cbuf;
}

inline byte lerp(byte a, byte b, float t)
{
	int ia = (int)a,
		ib = (int)b;
	return byte(ia+(ib-ia)*t);
}

template<typename T>
inline void RemoveElement(vector<T>& v, const T& e)
{
	for(typename vector<T>::iterator it = v.begin(), end = v.end(); it != end; ++it)
	{
		if(e == *it)
		{
			std::iter_swap(it, end-1);
			v.pop_back();
			return;
		}
	}

	assert(0 && "Can't find element to remove!");
}

template<typename T>
inline void RemoveElement(vector<T>* v, const T& e)
{
	RemoveElement(*v, e);
}

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

struct INT2
{
	int x, y;

	INT2() {}
	INT2(int x, int y) : x(x), y(y) {}

	inline INT2 operator + (const INT2& pt) const
	{
		return INT2(x+pt.x, y+pt.y);
	}
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
byte keystate[SDL_NUM_SCANCODES];

inline bool KeyDown(int key)
{
	return (keystate[key] & IS_DOWN) != 0;
}

SDL_Texture* tTiles, *tUnit, *tText, *tHpBar, *tHit;
TTF_Font* font;
SDL_Rect tiles[2];

struct Unit
{
	int hp;
	INT2 pos, new_pos;
	DIR dir;
	float move_progress, waiting;
	bool moving;
};

struct Tile
{
	Unit* unit;
	bool blocked;

	inline bool IsBlocking() const
	{
		return blocked || unit;
	}
};

Tile mapa[MAP_SIZE*MAP_SIZE];
vector<Unit*> units;
Unit* player;
INT2 hit_pos;
float hit_timer;

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

void CleanSDL()
{
	printf("INFO: Cleaning SDL.");
	
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	TTF_Quit();
	IMG_Quit();
	SDL_Quit();
}

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

void LoadMedia()
{
	tTiles = LoadTexture("data/textures/tile.png");
	tUnit = LoadTexture("data/textures/unit.png");
	tHpBar = LoadTexture("data/textures/hpbar.png");
	tHit = LoadTexture("data/textures/hit.png");

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

	SDL_Color color = {0,0,0};
	SDL_Surface* s = TTF_RenderText_Solid(font, "Hp: 100/100\nAttack: 30\nDefense: 5", color);
	if(!s)
		throw Format("ERROR: Failed to render text to surfac (%d).", TTF_GetError());
	tText = SDL_CreateTextureFromSurface(renderer, s);
	SDL_FreeSurface(s);
	if(!tText)
		throw Format("ERROR: Failed to convert text surface to texture (%d).", SDL_GetError());
}

void CleanMedia()
{
	SDL_DestroyTexture(tTiles);
	SDL_DestroyTexture(tUnit);
	SDL_DestroyTexture(tText);
	SDL_DestroyTexture(tHpBar);
	SDL_DestroyTexture(tHit);
	TTF_CloseFont(font);
}

void InitGame()
{
	srand((uint)time(NULL));
	for(int i=0; i<MAP_SIZE*MAP_SIZE; ++i)
		mapa[i].blocked = (rand()%8 == 0);
	
	player = new Unit;
	player->pos = INT2(0,0);
	player->hp = 100;
	player->moving = false;
	player->waiting = 0.f;
	units.push_back(player);
	mapa[0].blocked = false;
	mapa[0].unit = player;

	Unit* enemy = new Unit;
	enemy->pos = INT2(10+rand()%5,10+rand()%5);
	enemy->hp = 100;
	enemy->moving = false;
	enemy->waiting = false;
	units.push_back(enemy);
	Tile& tile = mapa[enemy->pos.x+enemy->pos.y*MAP_SIZE];
	tile.blocked = false;
	tile.unit = enemy;
}

void Draw()
{
	SDL_RenderClear(renderer);
	SDL_Rect r;

	// tiles
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
		if(u.moving)
		{
			r.x = u.pos.x*32+int(u.move_progress*32*dir_change[u.dir].x);
			r.y = u.pos.y*32+int(u.move_progress*32*dir_change[u.dir].y);
		}
		else
		{
			r.x = u.pos.x*32;
			r.y = u.pos.y*32;
		}
		SDL_RenderCopy(renderer, tUnit, NULL, &r);
		
		// pasek hp
		if(u.hp != 100)
		{
			Uint8 cr, g, b = 0;
			if(u.hp <= 50)
			{
				float t = float(u.hp)/50;
				cr = lerp(255, 255, t);
				g = lerp(0, 216, t);
			}
			else
			{
				float t = float(u.hp-50)/50;
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

	// uderzenie
	if(hit_timer > 0.f)
	{
		r.x = hit_pos.x;
		r.y = hit_pos.y;
		SDL_RenderCopy(renderer, tHit, NULL, &r);
	}
	
	// tekst
	SDL_QueryTexture(tText, NULL, NULL, &r.w, &r.h);
	r.x = 32*(MAP_SIZE+1);
	r.y = 32;
	SDL_RenderCopy(renderer, tText, NULL, &r);

	SDL_RenderPresent(renderer);
}

bool TryMove(DIR new_dir)
{
	player->new_pos = player->pos + dir_change[new_dir];
	if(player->new_pos.x >= 0 && player->new_pos.y >= 0 && player->new_pos.x < MAP_SIZE && player->new_pos.y < MAP_SIZE)
	{
		Tile& tile = mapa[player->new_pos.x+player->new_pos.y*MAP_SIZE];
		if(tile.blocked)
			return false;
		else if(tile.unit)
		{
			// attack
			hit_pos = INT2(player->new_pos.x*32, player->new_pos.y*32);
			hit_timer = 0.5f;
			player->waiting = 1.f;
			tile.unit->hp -= 25;
			if(tile.unit->hp <= 0)
			{
				RemoveElement(units, tile.unit);
				delete tile.unit;
				tile.unit = NULL;
			}
			return true;
		}
		else
		{
			if(new_dir == DIR_NE || new_dir == DIR_NW || new_dir == DIR_SE || new_dir == DIR_SW)
			{
				if(mapa[player->pos.x+dir_change[new_dir].x+player->pos.y*MAP_SIZE].blocked &&
					mapa[player->pos.x+(player->pos.y+dir_change[new_dir].y)*MAP_SIZE].blocked)
					return false;
			}

			player->dir = new_dir;
			player->moving = true;
			player->move_progress = 0.f;
			mapa[player->new_pos.x+player->new_pos.y*MAP_SIZE].unit = player;
			return true;
		}
	}

	return false;
}

void Update(float dt)
{
	if(player->waiting > 0.f)
		player->waiting -= dt;
	else if(player->moving)
	{
		player->move_progress += dt*5;
		if(player->move_progress >= 1.f)
		{
			player->moving = false;
			mapa[player->pos.x+player->pos.y*MAP_SIZE].unit = NULL;
			player->pos = player->new_pos;
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
				if(TryMove(keys2[i].dir))
					break;
			}
		}
		if(!player->moving && player->waiting <= 0.f)
		{
			for(int i=0; i<4; ++i)
			{
				if(KeyDown(keys1[i].k1) || KeyDown(keys1[i].k2))
				{
					if(TryMove(keys1[i].dir))
						break;
				}
			}
		}
	}

	if(hit_timer > 0.f)
		hit_timer -= dt;
}

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
			}

			uint new_ticks = SDL_GetTicks();
			float dt = float(new_ticks - ticks)/1000.f;
			ticks = new_ticks;

			Draw();
			Update(dt);

			for(uint i = 0; i < SDL_NUM_SCANCODES; ++i)
			{
				if(keystate[i] & 1)
					--keystate[i];
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
