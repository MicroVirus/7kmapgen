/*
 * Seven Kingdoms: Ancient Adversaries
 *
 * Copyright 1997,1998 Enlight Software Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

//Filename    : OISOAREA.CPP
//Description : Object to determine an isolated area
//Owner		  : Alex

#ifndef __OISOAREA_H
#define __OISOAREA_H

//--------- define cross_dir ---------//
enum	{	CROSS_LINE_SAME_DIR=1,
			CROSS_LINE_DIFF_DIR,
		};

//--------- defne struct AreaInfo ---------//
struct AreaInfo
{
	short x_loc;
	short y_loc;
	short distance;
	char	with_data;
};


//----------- Define class IsolateArea -----------//
class IsolateArea
{

public:
			IsolateArea();
			~IsolateArea();
	void	init();
	void	deinit();
	void	reset_area_info(AreaInfo *infoPtr);

	char	cal_direction(short sx, short sy, short dx, short dy);
	int	detect_isolate_area(short xLoc1, short yLoc1, short xLoc2, short yLoc2);
	void	process_move_around_boundary(char& dir, short& xLoc, short& yLoc);
	int	to_regions_line_left(char dir, int withVerticalCheck=0);

	int	is_isolate_area();


private:
	AreaInfo	same_region;
	AreaInfo	diff_region;
	char		regions_line_dir;
	short		regions_line_x_loc1;
	short		regions_line_y_loc1;
	short		regions_line_x_loc2;
	short		regions_line_y_loc2;
	short		region_check_x_offset;
	short		region_check_y_offset;


protected:


};

extern IsolateArea isolate_area;
//-----------------------------------------//

#endif
