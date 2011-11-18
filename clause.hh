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

#ifndef CLAUSE_HH
#define CLAUSE_HH

#include <cerrno>
#include <cstdarg>
#include <sstream>
#include <string>
#include <vector>

extern "C" {
#include <stdint.h>
}

#include "assert_hotpath.hh"
#include "literal.hh"
#include "system_error.hh"

class clause {
public:
	class impl {
	public:
		uint32_t size;
		uint32_t index;
		literal literals[0];

		impl(uint32_t size, uint32_t index, const std::vector<literal> &v):
			size(size),
			index(index)
		{
			for (unsigned int i = 0; i < size; ++i)
				literals[i] = v[i];
		}
	};

	impl *data;

	clause():
		data(0)
	{
	}

	clause(unsigned int index, const std::vector<literal> &v)
	{
		unsigned int size = v.size();
		assert(size >= 1);

		void *ptr = ::malloc(sizeof(*data) + size * sizeof(data->literals[0]));
		if (!ptr)
			throw system_error(errno);

		data = new (ptr) impl(size, index, v);
	}

	/* For convenience. */
	clause(unsigned int index, ...)
	{
		std::vector<literal> v;

		va_list ap;
		va_start(ap, index);

		while (1) {
			int lit = va_arg(ap, int);
			if (lit == 0)
				break;

			v.push_back(literal(lit));
		}

		va_end(ap);

		unsigned int size = v.size();
		assert(size >= 1);

		void *ptr = ::malloc(sizeof(*data) + size * sizeof(data->literals[0]));
		if (!ptr)
			throw system_error(errno);

		data = new (ptr) impl(size, index, v);
	}

	void free()
	{
		::free((void *) data);
	}

	bool operator==(const clause &other)
	{
		return data == other.data;
	}

	uint32_t size() const
	{
		return data->size;
	}

	uint32_t index() const
	{
		return data->index;
	}

	literal operator[](unsigned int i) const
	{
		assert_hotpath(data);
		assert_hotpath(i < data->size);
		return data->literals[i];
	}

	literal &operator[](unsigned int i)
	{
		assert_hotpath(data);
		assert_hotpath(i < data->size);
		return data->literals[i];
	}

	operator bool() const
	{
		return data;
	}

	std::string string() const
	{
		assert_hotpath(data);

		std::ostringstream ss;

		ss << "[" << data->index << "]";

		for (unsigned int i = 0; i < data->size; ++i)
			ss << " " << data->literals[i].string();

		return ss.str();
	}
};

std::ostream &operator<<(std::ostream &os, const clause &c)
{
	return os << c.string();
}

#endif
