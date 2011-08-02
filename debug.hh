/*
 * SAT solver
 * Copyright (C) 2011  Vegard Nossum <vegardno@ifi.uio.no>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DEBUG_HH
#define DEBUG_HH

#include <cstdio>
#include <typeinfo>

/* XXX: This should move to its own .cc file, but it only matters as long as
 * we have more than one .cc file to begin with (and both of them need
 * debug.hh) */
__thread unsigned int debug_thread_id;

#if CONFIG_DEBUG == 1
#define debug(fmt, ...) \
	printf("%u: %s::%s(" fmt ")\n", debug_thread_id, typeid(*this).name(), __FUNCTION__, ##__VA_ARGS__)
#else
#define debug(fmt, ...)
#endif

#endif
