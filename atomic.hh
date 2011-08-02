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

#ifndef ATOMIC_HH
#define ATOMIC_HH

extern "C" {
#include <urcu-qsbr.h>
/* XXX: urcu/compiler.h workaround for C++ */
#undef max
#undef min
}

template<typename t>
class atomic {
	t data;

public:
	atomic(t value = t()):
		data(value)
	{
	}

	void operator=(t value)
	{
		uatomic_set(&data, value);
	}

	operator bool() const
	{
		return uatomic_read(&data);
	}
};

#endif
