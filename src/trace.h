
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


#ifndef TRACE_H
#define TRACE_H

#define TRACE_NO(...)      TRACE(TRACE_LEVEL_NO, __VA_ARGS__)
#define TRACE_WARNING(...) TRACE(TRACE_LEVEL_WARNING, __VA_ARGS__)
#define TRACE_INFO(...)    TRACE(TRACE_LEVEL_INFO, __VA_ARGS__)
#define TRACE_ERROR(...)   TRACE(TRACE_LEVEL_ERROR, __VA_ARGS__)
#define TRACE_VERBOSE(...) TRACE(TRACE_LEVEL_VERBOSE, __VA_ARGS__)

#define TRACEAPPEND(...)   trace_append(__VA_ARGS__)
#define DUMP(...)          trace_dump(__VA_ARGS__)

#if defined(_DEBUG) || defined(_DEBUG_)
#define TRACE(...)         trace(strchr(__FILE__,'/')+1, __LINE__, __FUNCTION__, __VA_ARGS__)
#define DEBUG              TRACE(TRACE_LEVEL_NO, "DEBUG:\n");
#define DEBUG_ENTER        TRACE(TRACE_LEVEL_NO, "DEBUG: enter function\n");
#define DEBUG_LEAVE        TRACE(TRACE_LEVEL_NO, "DEBUG: leave function\n");
#else
#define TRACE(...)         trace(strchr(__FILE__,'/')+1, __LINE__, NULL, __VA_ARGS__)
#define DEBUG
#define DEBUG_ENTER
#define DEBUG_LEAVE
#endif

enum TRACE_LEVEL {
    TRACE_LEVEL_NO,
    TRACE_LEVEL_ERROR,
    TRACE_LEVEL_WARNING,
    TRACE_LEVEL_INFO,
    TRACE_LEVEL_VERBOSE
};

void trace(const char *, const int,
	   const char *, enum TRACE_LEVEL, const char *, ...);
void trace_append(enum TRACE_LEVEL, const char *, ...);
void trace_dump(const void *, int);
void trace_memory();

#endif				/* TRACE_H */
