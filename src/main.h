
/*
 *     Copyright (C) 2011-2012 Andreas Schönfelder
 *     https://github.com/escoand/socksswitch
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#ifndef MAIN_H
#define MAIN_H

#define NAME    "socksswitch"
#define VERSION "2012.02.23"
#define DESC    "forwards socks connection on basis of filter rules"
#define RIGHTS  "Copyright (C) 2011-2012 Andreas Schoenfelder"

void showHelp(char *);
void readParams(const int, char *argv[]);
void readConfig();
void cleanEnd(const int);

#endif				/* MAIN_H */
