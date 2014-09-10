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

// Filename    : OROCK.CPP
// Description : class Rock and RockArray
// Owner       : Gilbert


#include <OROCK.h>
#include <OROCKRES.h>

// --------- define constant ----------//
#define ROCK_ALT_PATH 19


Rock::Rock(short rockRecno, short xLoc, short yLoc)
{
	init( rockRecno, xLoc, yLoc );
}


void Rock::init(short rockRecno, short xLoc, short yLoc)
{
	rock_recno = rockRecno;
	loc_x = xLoc;
	loc_y = yLoc;
	seed = (xLoc + yLoc + 3) * ( 2 * xLoc + 7 * yLoc + 5);

	// ------- random frame, random initial delay_remain  -----//
	RockInfo *rockInfo = rock_res.get_rock_info(rockRecno);
	cur_frame = 1 + (char) random(rockInfo->max_frame);

	char initDelayCount = rock_res.get_anim_info(
		rock_res.get_anim_recno(rockRecno, cur_frame))->delay;
	delay_remain = 1 + (char) random(initDelayCount);
}


unsigned Rock::random(unsigned bound)
{
   #define MULTIPLIER      0x015a4e35L
   #define INCREMENT       1
   seed = MULTIPLIER * seed + INCREMENT;
	return seed % bound;
}


void Rock::process()
{
	if( --delay_remain <= 0)
	{
		cur_frame = rock_res.choose_next(rock_recno, cur_frame, random(ROCK_ALT_PATH) );
		delay_remain = rock_res.get_anim_info(rock_res.get_anim_recno(rock_recno, cur_frame))->delay;
	}
}


void Rock::draw()
{
	rock_res.draw(rock_recno, loc_x, loc_y, cur_frame);
}

void Rock::draw_block(short xLoc, short yLoc)
{
	rock_res.draw_block(rock_recno, xLoc, yLoc, xLoc-loc_x, yLoc-loc_y, cur_frame);
}

RockArray::RockArray(int initArraySize) : DynArrayB(sizeof(Rock),initArraySize, DEFAULT_REUSE_INTERVAL_DAYS)
{
}


void RockArray::init()
{
	zap();
}


void RockArray::deinit()
{
}


void RockArray::process()
{
	Rock* rockPtr;
	for( int i = 1; i <= size(); ++i)
	{
		if( (rockPtr = (Rock *)get(i)) != NULL)
		{
			rockPtr->process();
		}
	}
}
