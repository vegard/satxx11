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

#ifndef SATXX11_ASSERT_HH
#define SATXX11_ASSERT_HH

#include <cstdio>
#include <cstdlib>

#include <satxx11/format.hh>

namespace satxx11 {

template<typename... Args>
static inline void assertion_failed(const char *filename, unsigned int line,
	const char *pretty_function,
	const char *condition)
{
	fprintf(stderr, "%s:%u: %s: Assertion `%s' failed.\n",
		filename, line, pretty_function, condition);

	/* This will give us a stacktrace in gcc, which is sorely needed for
	 * debugging. */
	abort();
}

template<typename... Args>
static inline void assertion_failed(const char *filename, unsigned int line,
	const char *pretty_function,
	const char *condition,
	const char *fmt,
	Args... args)
{
	fprintf(stderr, "%s:%u: %s: Assertion `%s' failed.\n",
		filename, line, pretty_function, condition);

	fprintf(stderr, "%s\n", format(fmt, args...).c_str());

	/* This will give us a stacktrace in gcc, which is sorely needed for
	 * debugging. */
	abort();
}

/* XXX: Work out something better. */
/* We use __builtin_expect() in order to hint to the compiler that it is very
 * unlikely that the assertion will trigger. This helps the compiler do branch
 * prediction and usually results in the compiler moving the "cold" case out
 * of the way (e.g. at the end of the function). Unfortunately, it seems that
 * gcc cannot do constant folding across __builtin_expect(), so we have to do
 * it ourselves. */
#undef assert
#define assert(cond, ...) \
	do { \
		if (__builtin_constant_p(cond) ? !(cond) : __builtin_expect(!(cond), 1)) { \
			satxx11::assertion_failed(__FILE__, __LINE__, __PRETTY_FUNCTION__, #cond, ##__VA_ARGS__); \
			__builtin_unreachable(); \
		} \
	} while(0)

}

#endif
