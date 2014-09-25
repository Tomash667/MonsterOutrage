#include <SDL.h>
#include <SDL_image.h>
#include <cstdio>
#include <conio.h>
#include <cstdlib>
#include <ctime>

typedef const char* cstring;
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

SDL_Window* window;
SDL_Renderer* renderer;

SDL_Texture* tTiles, *tUnit;
int x, y;
bool mapa[MAP_SIZE*MAP_SIZE];
SDL_Rect tiles[2];

void InitSDL()
{
	printf("INFO: Initializing SLD.");

	if(SDL_Init(SDL_INIT_VIDEO) < 0)
		throw Format("ERROR: Failed to initialize SDL (%d).", SDL_GetError());

	window = SDL_CreateWindow("Monster Outrage v 0", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024, 768, SDL_WINDOW_SHOWN);
	if(!window)
		throw Format("ERROR: Failed to create window (%d).", SDL_GetError());

	renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_ACCELERATED);
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
	r.x = x*32;
	r.y = y*32;
	SDL_RenderCopy(renderer, tUnit, NULL, &r);

	SDL_RenderPresent(renderer);
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

		while(!quit)
		{
			SDL_Event e;
			while(SDL_PollEvent(&e) != 0)
			{
				if(e.type == SDL_QUIT)
					quit = true;
				else if(e.type == SDL_KEYDOWN)
				{
					switch(e.key.keysym.sym)
					{
					case SDLK_UP:
					case SDLK_KP_8:
						if(y > 0 && !mapa[x+(y-1)*MAP_SIZE])
							--y;
						break;
					case SDLK_DOWN:
					case SDLK_KP_2:
						if(y < MAP_SIZE-1 && !mapa[x+(y+1)*MAP_SIZE])
							++y;
						break;
					case SDLK_LEFT:
					case SDLK_KP_4:
						if(x > 0 && !mapa[x-1+y*MAP_SIZE])
							--x;
						break;
					case SDLK_RIGHT:
					case SDLK_KP_6:
						if(x < MAP_SIZE-1 && !mapa[x+1+y*MAP_SIZE])
							++x;
						break;
					case SDLK_ESCAPE:
						quit = true;
						break;
					case SDLK_KP_1:
						if(x > 0 && y < MAP_SIZE-1 && !mapa[x-1+(y+1)*MAP_SIZE])
						{
							--x;
							++y;
						}
						break;
					case SDLK_KP_3:
						if(x < MAP_SIZE-1 && y < MAP_SIZE-1 && !mapa[x+1+(y+1)*MAP_SIZE])
						{
							++x;
							++y;
						}
						break;
					case SDLK_KP_7:
						if(x > 0 && y > 0 && !mapa[x-1+(y-1)*MAP_SIZE])
						{
							--x;
							--y;
						}
						break;
					case SDLK_KP_9:
						if(x < MAP_SIZE-1 && y > 0 && !mapa[x+1+(y-1)*MAP_SIZE])
						{
							++x;
							--y;
						}
						break;
					}
				}
			}

			Draw();
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

// http://lazyfoo.net/tutorials/SDL/index.php
// TODO: animacja ruchu
