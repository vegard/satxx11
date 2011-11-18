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

#ifndef LITERAL_HH
#define LITERAL_HH

#include <cstdlib>
#include <sstream>
#include <string>

extern "C" {
#include <stdint.h>
}

class literal {
public:
	uint32_t x;

	literal()
	{
	}

	explicit literal(int lit):
		x((abs(lit) << 1) + (lit > 0))
	{
	}

	literal(unsigned int variable, bool value):
		x((variable << 1) + value)
	{
	}

	literal operator~() const
	{
		/* XXX: Check that the compiler indeed compiles this as
		 * return x ^ 1; */
		return literal(variable(), !value());
	}

	operator uint32_t() const
	{
		return x;
	}

	unsigned int variable() const
	{
		return x >> 1;
	}

	bool value() const
	{
		return x & 1;
	}

	std::string string() const
	{
		std::ostringstream ss;

		if (!value())
			ss << "-";

		ss << variable();

		return ss.str();
	}
};

std::ostream &operator<<(std::ostream &os, const literal &l)
{
	return os << l.string();
}

#endif
