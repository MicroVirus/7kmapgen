#ifndef USE_SDL
#error 7kmapgen requires the use of SDL
#endif

#include <OCONFIG.h>
#include <OSYS.h>
#include <OGAME.h>
#include <OINFO.h>
#include <OBATTLE.h>
#include <ONATIONA.h>
#include <OMOUSE.h>
#include <OMOUSECR.h>
#include <OOPTMENU.h>
#include <OINGMENU.h>
#include <dbglog.h>
#include <cassert>

DBGLOG_DEFAULT_CHANNEL(mapgen);

// Prototypes:
void mapgen();
void generate_map();
SDL_Surface* genmap(int32_t *seed = NULL, SDL_Rect *mapSize = NULL);
bool create_window();
void mapgen_loop();
void hittest(int button, int x, int y);
void update_window();


enum {MapRegionID = 0, GenerateRegionID, ClipboardRegionID};
struct MapGenWindow
{
	enum {MAX_REGIONS = 10};

	SDL_Window *window;
	SDL_Renderer *renderer;

	// Regions are parts of the screen that are drawn to or can respond to UI
	int	region_count;
	SDL_Rect regions[MAX_REGIONS];
};


// Main window
namespace {
	MapGenWindow window;
	SDL_Texture* mapTexture = NULL;
	SDL_Texture* generateTexture = NULL;
	SDL_Texture* clipboardTexture = NULL;
	bool updating = false; // True if currently generation a new map - this takes a little bit of time, so it's noticeable
	int32_t seed = 0;
}


// HINTS:
// - Look at OINGMENU InGameMenu::disp -> font_bible.center_put( MAP_ID_X1, MAP_ID_Y1, MAP_ID_X2, MAP_ID_Y2, str);
//   It has font drawing. We want font drawing.
// - Might be able to leverage single player new game menu graphics.


void mapgen()
{
	// Backup config, so we can restore on exit
	Config realConfig = config;

	sys.is_mp_game = 0;
	
	// TODO: set config dynamically.

	// World topography depends on:
	config.land_mass = OPTION_MODERATE; // OPTION_LOW, OPTION_MODERATE, OPTION_HIGH
	// Game objects layout depends on:
	config.random_start_up = 0; // Lots of things depend on this, throughout the generation process
	config.ai_nation_count = MAX_NATION-1; // Hence, on mp or not
	config.start_up_has_mine_nearby = 0;
	config.start_up_raw_site = 3;
	config.independent_town_resistance = OPTION_LOW;
	config.start_up_independent_town = 30; // 7, 15, 30
	config.monster_type = OPTION_MONSTER_DEFENSIVE; // only depends on NONE or not-NONE

	// Needed for extracting the minimap
	config.explore_whole_map = 1;

	// Create main window
	if (!create_window())
		return;

	// Generate the map
	generate_map();

	mapgen_loop();

	// Restore config
	config = realConfig;
}

void generate_map()
{
	SDL_Rect mapSize;
	SDL_Surface *mapSurface;

	updating = true;
	update_window();

	seed = 0;
	mapSurface = genmap(&seed, &mapSize);

	SDL_Rect scaledMap = {0, 0, window.regions[MapRegionID].w, window.regions[MapRegionID].h};
	// Use the 1-2-step trick to get ARGB surface of the right format
	SDL_Surface *tempSurface = SDL_CreateRGBSurface(0, scaledMap.w, scaledMap.h, 8, 0, 0, 0, 0);
	SDL_Surface *scaledMapSurface = SDL_ConvertSurfaceFormat(tempSurface, SDL_PIXELFORMAT_ARGB8888, 0);
	if (!tempSurface || !scaledMapSurface) {ERR("Surface creation failed for copying the map: %s", SDL_GetError()); return;}
	SDL_BlitScaled(mapSurface, NULL, scaledMapSurface, &scaledMap);
	SDL_UpdateTexture(mapTexture, NULL, scaledMapSurface->pixels, scaledMapSurface->pitch);
	SDL_FreeSurface(tempSurface);
	SDL_FreeSurface(scaledMapSurface);

	updating = false;
	update_window();
}


bool create_window()
{
	enum {	WindowWidth = 900, WindowHeight = 800,
			MapDrawWidth = 400, MapDrawHeight = 400,
			GenerateButtonWidth = 180, GenerateButtonHeight = 45,
			ClipboardButtonWidth = 110, ClipboardButtonHeight = 120};

	window.window = SDL_CreateWindow("7kaa map generator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WindowWidth, WindowHeight, SDL_WINDOW_SHOWN );
	if (!window.window)
	{
		ERR("Failed to create mapgen window: %s", SDL_GetError()); return false;
	}

	window.renderer = SDL_CreateRenderer(window.window, -1, SDL_RENDERER_ACCELERATED);
	if (!window.renderer)
	{
		ERR("Failed to create the renderer: %s", SDL_GetError()); return false;
	}

	mapTexture = SDL_CreateTexture(window.renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, MapDrawWidth, MapDrawHeight);
	if (!mapTexture)
	{
		ERR("Failed to create texture for map: %s", SDL_GetError()); return false;
	}

	SDL_Surface *fromBMP = SDL_LoadBMP("7kmapgen/generate.bmp");
	if (!fromBMP)
	{
		ERR("Failed to load Generate button bitmap: %s", SDL_GetError()); return false;
	}
	generateTexture = SDL_CreateTextureFromSurface(window.renderer, fromBMP);
	if (!generateTexture)
	{
		ERR("Failed to create texture for Generate button: %s", SDL_GetError()); return false;
	}
	SDL_FreeSurface(fromBMP);

	fromBMP = SDL_LoadBMP("7kmapgen/clipboard.bmp");
	if (!fromBMP)
	{
		ERR("Failed to load Generate button bitmap: %s", SDL_GetError()); return false;
	}
	clipboardTexture = SDL_CreateTextureFromSurface(window.renderer, fromBMP);
	if (!clipboardTexture)
	{
		ERR("Failed to create texture for Clipboard button: %s", SDL_GetError()); return false;
	}
	SDL_FreeSurface(fromBMP);

	// Add regions
	window.region_count = 3;
	assert(window.region_count <= MapGenWindow::MAX_REGIONS);
	// Map
	window.regions[MapRegionID].x = (WindowWidth - MapDrawWidth) / 2;
	window.regions[MapRegionID].y = 12;
	window.regions[MapRegionID].w = MapDrawWidth;
	window.regions[MapRegionID].h = MapDrawHeight;
	// Generate button
	window.regions[GenerateRegionID].x = (WindowWidth - GenerateButtonWidth) / 2;
	window.regions[GenerateRegionID].y = window.regions[MapRegionID].y + window.regions[MapRegionID].h + 20;
	window.regions[GenerateRegionID].w = GenerateButtonWidth;
	window.regions[GenerateRegionID].h = GenerateButtonHeight;
	// Clipboard button
	window.regions[ClipboardRegionID].x = window.regions[MapRegionID].x + window.regions[MapRegionID].w + 40;
	window.regions[ClipboardRegionID].y = window.regions[MapRegionID].y + (window.regions[MapRegionID].h - ClipboardButtonHeight) / 2;
	window.regions[ClipboardRegionID].w = ClipboardButtonWidth;
	window.regions[ClipboardRegionID].h = ClipboardButtonHeight;

	SDL_ShowCursor(SDL_ENABLE);

	return true;
}

void mapgen_loop()
{
	SDL_Event e;
	while (SDL_WaitEvent(&e) != 0)
	{
		switch(e.type)
		{
		case SDL_QUIT:
			goto Exit;
		case SDL_MOUSEBUTTONUP:
			hittest(e.button.button, e.button.x, e.button.y);
			break;
		}
	}
	ERR("SDL main loop: %s", SDL_GetError());

Exit:
	;
}

void hittest(int button, int x, int y)
{
	SDL_Rect const *r;
	
	r = &window.regions[GenerateRegionID];
	if (x >= r->x && x < r->x + r->w && y >= r->y && y < r->y + r->h)
	{
		// Hit generate button
		generate_map();
	}

	r = &window.regions[ClipboardRegionID];
	if (x >= r->x && x < r->x + r->w && y >= r->y && y < r->y + r->h)
	{
		// Hit clipboard button
		char szSeed[20];
		SDL_SetClipboardText(ltoa(seed, szSeed, 10));
	}
}

void update_window()
{
	int result;
	result = SDL_SetRenderDrawColor(window.renderer, 216, 216, 216, 0xFF);
	result = SDL_RenderClear(window.renderer);

	result = SDL_SetRenderDrawColor(window.renderer, 0xFF, 0xFF, 0xFF, 0xFF);
	result = SDL_RenderCopy(window.renderer, mapTexture, NULL, &window.regions[MapRegionID]);
	result = SDL_RenderCopy(window.renderer, generateTexture, NULL, &window.regions[GenerateRegionID]);
	result = SDL_RenderCopy(window.renderer, clipboardTexture, NULL, &window.regions[ClipboardRegionID]);

	// Draw grey over map and buttons
	if (updating)
	{
		result = SDL_SetRenderDrawColor(window.renderer, 0x40, 0x40, 0x40, 0x80);
		result = SDL_SetRenderDrawBlendMode(window.renderer, SDL_BLENDMODE_BLEND);
		result = SDL_RenderFillRect(window.renderer, &window.regions[MapRegionID]);
		result = SDL_RenderFillRect(window.renderer, &window.regions[GenerateRegionID]);
		result = SDL_RenderFillRect(window.renderer, &window.regions[ClipboardRegionID]);
		result = SDL_SetRenderDrawBlendMode(window.renderer, SDL_BLENDMODE_NONE);
	}

	SDL_RenderPresent(window.renderer);
}


// Returns the map for a given random seed.
// If seed is NULL or *seed is 0, a time based random seed is generated and returned.
// If seed is non-NULL, then on return it will contain the random seed
// If mapSize is non-NULL, the size of the map is returned.
// The return value is an RGB surface which contains the map. When done with the surface it should be freed.
SDL_Surface* genmap(int32_t *seed, SDL_Rect *mapSize)
{
	game.init();
	
	//------ From battle.run(0); // 0-not multiplayer game ------//

	// TODO: When generating time based seed, the code ensures this is non-negative.
	// But why? Doesn't seem like a seed needs such a restriction at all ...
	// NOTE: The info.random_seed is the initial seed, and is used for display purposes
	//       The misc.random_seed is continuously altered and is the current seed value.

	// Initialise the random seed
	if (seed)
	{
		info.init_random_seed(*seed); // Use given random seed
		*seed = misc.get_random_seed();
	}
	else
		info.init_random_seed(0); // Use new random seed based on time

	// To get the string representation: ltoa( info.random_seed , mapIdStr, 10);

	// Generate map, using seed as set above
	world.generate_map();

	// Base map is done. Now pregame objects are placed, again using the random seed.
	// Number of AI nations influences all further steps. For multiplayer, the map will not be the same starting from this point on.
	// Pretty much all the steps depend on random_start_up, at various places.
	battle.create_ai_nation(config.ai_nation_count); // depends also on config.random_start_up
	// Place all objects
	battle.create_pregame_object(); // depends on config.random_start_up, config.ai_nation_count, config.start_up_has_mine_nearby, config.start_up_raw_site, config.independent_town_resistance, config.start_up_independent_town, config.monster_type. Roughly in that order.

	// Map has been generated! Initialise the game.
	nation_array.update_statistic();
	sys.set_speed(18);
	//---- reset cheats ----//
	config.fast_build = 0;
	config.king_undie_flag = 0;
	config.blacken_map = 1;
	config.disable_ai_flag = 0;

	// Run our 'custom game'. What it does is boot up the game, draw the map and exit.
	world.map_matrix->map_mode = MAP_MODE_TERRAIN;
	sys.run_mapgen();

	// Extract the map from the front buffer

	int mapX = world.map_matrix->image_x1;
	int mapY = world.map_matrix->image_y1;
	int	mapWidth = world.map_matrix->image_width;
	int mapHeight = world.map_matrix->image_height;

	SDL_Surface *tempSurface = SDL_CreateRGBSurface(0, mapWidth, mapHeight, 8, 0, 0, 0, 0);
	if (!tempSurface)
	{
		ERR("Failed to create tempSurface"); return NULL;
	}
	SDL_Surface *mapSurface = SDL_ConvertSurfaceFormat(tempSurface, SDL_PIXELFORMAT_ARGB8888, 0);
	SDL_FreeSurface(tempSurface);
	if (!mapSurface)
	{
		ERR("Failed to create mapSurface"); return NULL;
	}
	SDL_Rect mapRect = {mapX, mapY, mapWidth, mapHeight};
	SDL_BlitSurface(vga_front.get_buf()->get_surface(), &mapRect, mapSurface, NULL);

	game.deinit();

	// Return mapsize
	if (mapSize)
	{
		mapSize->x = mapSize->y = 0;
		mapSize->w = mapWidth; mapSize->h = mapHeight;
	}

	return mapSurface;
}



//
// Own version of Sys::run and Sys::main_loop
//
// Starts the game, draws the map, and exits again. vga_front contains the drawn map
void Sys::run_mapgen()
{
	sys_flag  = SYS_RUN;
	view_mode = MODE_NORMAL;

	//------ reset mouse ---------//

	mouse.reset_click();
	mouse_cursor.set_frame(0);

	//----- sys::disp_frame() will redraw everything when this flag is set to 1 ----//

	sys.need_redraw_flag = 1;
	sys.map_need_redraw = 1;
	user_pause_flag = 0;

	option_menu.active_flag = 0;
	in_game_menu.active_flag = 0;

	//   From main_loop

	day_frame_count = 0;       // for determining when the day counter should be increased.
	frame_count = 1;
	for( int i=nation_array.size() ; i>0 ; i-- )
	{
		if( !nation_array.is_deleted(i) )
			nation_array[i]->next_frame_ready = 0;
	}

	is_sync_frame   = 0;

	// Draw only the map
	sys.disp_map();


	// End of main_loop


	misc.unlock_seed();

	sys_flag = SYS_PREGAME;
}
