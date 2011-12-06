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
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

#include "assert_hotpath.hh"
#include "literal.hh"
#include "system_error.hh"

class clause {
public:
	class impl {
	public:
		/* Attempt to pack these.. We might also need flags for learnt, etc. */
		uint32_t index;
		uint16_t thread;
		uint16_t size;
		literal literals[0];

		impl(uint16_t thread, uint32_t index, uint16_t size, const std::vector<literal> &v):
			thread(thread),
			index(index),
			size(size)
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

	clause(unsigned int thread, unsigned int index, const std::vector<literal> &v)
	{
		unsigned int size = v.size();
		assert(size >= 1);

		void *ptr = ::malloc(sizeof(*data) + size * sizeof(data->literals[0]));
		if (!ptr)
			throw system_error(errno);

		data = new (ptr) impl(thread, index, size, v);
	}

	template<typename... Args>
	static void collect_literals(std::vector<literal> &v)
	{
	}

	template<typename... Args>
	static void collect_literals(std::vector<literal> &v, int lit, Args... args)
	{
		v.push_back(literal(lit));
		collect_literals(v, args...);
	}

	/* For convenience. */
	template<typename... Args>
	clause(unsigned int thread, unsigned int index, Args... args)
	{
		std::vector<literal> v;

		collect_literals(v, args...);

		unsigned int size = v.size();
		assert(size >= 1);

		void *ptr = ::malloc(sizeof(*data) + size * sizeof(data->literals[0]));
		if (!ptr)
			throw system_error(errno);

		data = new (ptr) impl(thread, index, size, v);
	}

	void free()
	{
		::free((void *) data);
	}

	bool operator<(const clause &other) const
	{
		return data < other.data;
	}

	bool operator==(const clause &other) const
	{
		return data == other.data;
	}

	uint32_t thread() const
	{
		return data->thread;
	}

	uint32_t index() const
	{
		return data->index;
	}

	uint32_t size() const
	{
		return data->size;
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

		ss << "[" << data->thread << ":" << data->index << "]";

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
