#include <SDL.h>
#include <SDL_image.h>
#include <cstdio>
#include <conio.h>
#include <cstdlib>
#include <ctime>

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

const int VERSION = 0;
const int MAP_SIZE = 15;

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

SDL_Texture* tTiles, *tUnit;
bool mapa[MAP_SIZE*MAP_SIZE];
SDL_Rect tiles[2];
INT2 pos, new_pos;
DIR dir;
bool moving;
float move_progress;

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
}

void CleanSDL()
{
	printf("INFO: Cleaning SDL.");
	
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

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

	for(int i=0; i<2; ++i)
	{
		tiles[i].w = 32;
		tiles[i].h = 32;
	}
	tiles[0].x = 0;
	tiles[0].y = 0;
	tiles[1].x = 32;
	tiles[1].y = 0;
}

void CleanMedia()
{
	SDL_DestroyTexture(tTiles);
	SDL_DestroyTexture(tUnit);
}

void InitGame()
{
	srand((uint)time(NULL));
	for(int i=0; i<MAP_SIZE*MAP_SIZE; ++i)
		mapa[i] = (rand()%8 == 0);
	mapa[0] = false;
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
			SDL_RenderCopy(renderer, tTiles, &tiles[mapa[xx+yy*MAP_SIZE] ? 1 : 0], &r);
		}
	}
	
	// unit
	if(moving)
	{
		r.x = pos.x*32+int(move_progress*32*dir_change[dir].x);
		r.y = pos.y*32+int(move_progress*32*dir_change[dir].y);
	}
	else
	{
		r.x = pos.x*32;
		r.y = pos.y*32;
	}
	SDL_RenderCopy(renderer, tUnit, NULL, &r);

	SDL_RenderPresent(renderer);
}

void Update(float dt)
{
	if(moving)
	{
		move_progress += dt*5;
		if(move_progress >= 1.f)
		{
			moving = false;
			pos = new_pos;
		}
	}
	else
	{
		DIR new_dir = DIR_INVALID;
		if((KeyDown(SDL_SCANCODE_LEFT) || KeyDown(SDL_SCANCODE_KP_4)) && pos.x > 0 && !mapa[pos.x-1+pos.y*MAP_SIZE])
			new_dir = DIR_W;
		else if((KeyDown(SDL_SCANCODE_RIGHT) || KeyDown(SDL_SCANCODE_KP_6)) && pos.x < MAP_SIZE-1 && !mapa[pos.x+1+pos.y*MAP_SIZE])
			new_dir = DIR_E;
		else if((KeyDown(SDL_SCANCODE_UP) || KeyDown(SDL_SCANCODE_KP_8)) && pos.y > 0 && !mapa[pos.x+(pos.y-1)*MAP_SIZE])
			new_dir = DIR_N;
		else if((KeyDown(SDL_SCANCODE_DOWN) || KeyDown(SDL_SCANCODE_KP_2)) && pos.y < MAP_SIZE-1 && !mapa[pos.x+(pos.y+1)*MAP_SIZE])
			new_dir = DIR_S;
		else if(KeyDown(SDL_SCANCODE_KP_1) && pos.x > 0 && pos.y < MAP_SIZE-1 && !mapa[pos.x-1+(pos.y+1)*MAP_SIZE])
			new_dir = DIR_SW;
		else if(KeyDown(SDL_SCANCODE_KP_3) && pos.x < MAP_SIZE-1 && pos.y < MAP_SIZE-1 && !mapa[pos.x+1+(pos.y+1)*MAP_SIZE])
			new_dir = DIR_SE;
		else if(KeyDown(SDL_SCANCODE_KP_7) && pos.x > 0 && pos.y > 0 && !mapa[pos.x-1+(pos.y-1)*MAP_SIZE])
			new_dir = DIR_NW;
		else if(KeyDown(SDL_SCANCODE_KP_9) && pos.x < MAP_SIZE-1 && pos.y > 0 && !mapa[pos.x+1+(pos.y-1)*MAP_SIZE])
			new_dir = DIR_NE;

		if(new_dir != DIR_INVALID)
		{
			dir = new_dir;
			moving = true;
			move_progress = 0.f;
			new_pos = pos + dir_change[dir];
		}
	}
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
