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
__thread unsigned int debug_call_depth;

/* A bit ugly, but this macro magic lets us trace function calls very
 * prettily. The purpose of the do { ... } while (0) is to ensure correct
 * nesting in all cases (e.g. if (...) debug(...);). The ({ ... }) syntax
 * lets us return a value from a block (the last expression/statement in
 * the block is taken as the value of the whole block). It is also
 * important not to evaluate arguments more than once, etc. */

#if CONFIG_DEBUG == 1
#define debug(fmt, ...) \
	printf("%u: %*s" fmt "\n", debug_thread_id, debug_call_depth, "", ##__VA_ARGS__)
#define debug_enter(fmt, ...) \
	do { \
		printf("%u: %*s%s(" fmt ")\n", debug_thread_id, debug_call_depth, "", __FUNCTION__, ##__VA_ARGS__); \
		debug_call_depth += 4; \
	} while (0)
#define debug_leave \
	do { \
		printf("%u: %*sreturn\n", debug_thread_id, debug_call_depth, ""); \
		debug_call_depth -= 4; \
		return; \
	} while (0)
#define debug_return(val, fmt, ...) \
	({ \
		typeof(val) __tmp = (val); \
		\
		printf("%u: %*sreturn " fmt "\n", debug_thread_id, debug_call_depth, "", ##__VA_ARGS__); \
		debug_call_depth -= 4; \
		\
		(__tmp); \
	})
#else
#define debug(fmt, ...)
#define debug_enter(fmt, ...)
#define debug_leave return
#define debug_return(val, fmt, ...) val
#endif

#endif
