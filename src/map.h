/**
 * Map - group of screen references (dink.dat)

 * Copyright (C) 1997, 1998, 1999, 2002, 2003  Seth A. Robinson
 * Copyright (C) 2005, 2007, 2008, 2009, 2014  Sylvain Beucler

 * This file is part of GNU FreeDink

 * GNU FreeDink is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.

 * GNU FreeDink is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef _MAP_H
#define _MAP_H

extern char current_dat[50];

/* dink.dat */
struct map_info
{
  int loc[768+1];
  int music[768+1];
  int indoor[768+1];
};
extern struct map_info map;

extern int load_info_to(char* path, struct map_info *mymap);
extern void load_info(void);
extern void save_info(void);

#endif
