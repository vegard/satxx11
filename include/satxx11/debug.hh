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

#ifndef SATXX11_DEBUG_HH
#define SATXX11_DEBUG_HH

#include <cstdio>
#include <typeinfo>

#include <satxx11/format.hh>
#include <satxx11/unique_name.hh>

namespace satxx11 {

/* XXX: This should move to its own .cc file, but it only matters as long as
 * we have more than one .cc file to begin with (and both of them need
 * debug.hh) */
__thread unsigned int debug_thread_id;
__thread unsigned int debug_call_depth;

/* A bit ugly, but this macro and template magic lets us trace function calls
 * very prettily. */

#if CONFIG_DEBUG == 1

template<typename... Args>
static inline void debug(const char *fmt, Args... args)
{
	printf("%u: %*s%s\n", debug_thread_id, debug_call_depth, "",
		format(fmt, args...).c_str());
}

class debug_enter_helper {
public:
	template<typename... Args>
	debug_enter_helper(const char *function, const char *pretty_function,
		const char *fmt, Args... args)
	{
		printf("%u: %*s%s(%s)\n", debug_thread_id, debug_call_depth, "",
			function, format(fmt, args...).c_str());
		printf("%u: %*s[%s]\n", debug_thread_id, debug_call_depth, "", pretty_function);
		debug_call_depth += 4;
	}

	~debug_enter_helper()
	{
		debug_call_depth -= 4;
	}
};

#define debug_enter(fmt, ...) \
	debug_enter_helper UNIQUE_NAME(debug_enter)(__FUNCTION__, __PRETTY_FUNCTION__, fmt, ##__VA_ARGS__)

template<typename t, typename... Args>
static inline t debug_return(t x, const char *fmt, Args... args)
{
	printf("%u: %*sreturn %s\n", debug_thread_id, debug_call_depth, "",
		format(fmt, x, args...).c_str());
	return x;
}

#else

#define debug(fmt, ...)
#define debug_enter(fmt, ...)
#define debug_return(val, fmt, ...) val

#endif

}

#endif
