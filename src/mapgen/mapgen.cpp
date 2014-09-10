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

// TODO: Apply dbglog patches
DBGLOG_DEFAULT_CHANNEL(mapgen);

// The map generator entry point
void mapgen();

// TESTING
void show_test_window();
SDL_Window *testWindow;

void mapgen()
{
	// Backup config, so we can restore on exit
	Config realConfig = config;

	sys.is_mp_game = 0;
	
	

	// TODO: set config dynamically.

	// World topography depends on:
	config.land_mass = OPTION_MODERATE; // OPTION_LOW, OPTION_MODERATE, OPTION_HIGH
	// Game objects layout depends on:
	config.random_start_up = 0; // Lots depends on this, throughout
	config.ai_nation_count = MAX_NATION-1; // Hence, on mp or not
	config.start_up_has_mine_nearby = 0;
	config.start_up_independent_town = 30; // 7, 15, 30
	// Needed
	config.explore_whole_map = 1;



	//----------- START OF PER-GAME (PER MAP) CODE -----------//

	game.init();
	
	//------ From battle.run(0); // 0-not multiplayer game ------//

	// To use specific seed: info.init_random_seed(869639665); 
	// Using time-based seed:
	info.init_random_seed(0);
	// To get the string representation: ltoa( info.random_seed , mapIdStr, 10);

	// Generate map, using seed as set above
	world.generate_map();

	// Base map is done. Now pregame objects are placed, again using the random seed
	// Number of AI nations influences all further steps. For multiplayer, the map will not be
	// the same starting from this point on.
	// Pretty much all the steps depend on random_start_up, at various places.
	battle.create_ai_nation(config.ai_nation_count); // depends also on config.random_start_up
	battle.create_pregame_object(); // depends on config.start_up_has_mine_nearby, config.start_up_independent_town, config.monster_type
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
		// TODO: Errors
		throw "tempSurface";
	}
	SDL_Surface *mapSurface = SDL_ConvertSurfaceFormat(tempSurface, SDL_PIXELFORMAT_ARGB8888, 0);
	SDL_FreeSurface(tempSurface);
	if (!mapSurface)
	{
		// TODO: Errors
		throw "mapSurface";
	}
	SDL_Rect mapRect = {mapX, mapY, mapWidth, mapHeight};
	SDL_BlitSurface(vga_front.get_buf()->get_surface(), &mapRect, mapSurface, NULL);

	// TESTING - Displays a test window, draws the map to it, waits, then exits

	show_test_window();

	auto screen = SDL_GetWindowSurface(testWindow);
	if (screen)
	{
		SDL_Rect scaledMap = {10, 10, 2 * mapRect.w, 2 * mapRect.h};
		SDL_BlitScaled(mapSurface, NULL, screen, &scaledMap);
		//SDL_BlitSurface(mapSurface, NULL, screen, NULL);
		SDL_UpdateWindowSurface(testWindow);
		SDL_Delay(5000);
	}
	
	game.deinit();

	//----------- END OF PER-GAME (PER MAP) CODE -----------//


	
	// Restore config
	config = realConfig;
}


void show_test_window()
{
	testWindow = SDL_CreateWindow("Test 7kmapgen", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1000, 800, SDL_WINDOW_SHOWN );
	if (!testWindow)
	{
		ERR("Failed to create window");
		exit(1);
	}
}




// Own versions of Sys::run and Sys::main_loop

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