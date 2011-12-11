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

#ifndef CLAUSE_ALLOCATOR_HH
#define CLAUSE_ALLOCATOR_HH

#include <vector>

#include "assert_hotpath.hh"

class clause_allocator {
public:
	struct entry {
		/* We pack a pointer + one bit into this member. Our pointers
		 * are always aligned on at least a 4-byte boundary (probably
		 * 8-byte, though), which means we can use the lower 2 (or 3)
		 * bits for something else. We also rely on the fact that
		 * sizeof(unsigned long) == sizeof(void *). */
		unsigned long data;

		/* Number of threads with this clause attached */
		unsigned int reference_count;

		entry()
		{
		}

		entry(clause c, unsigned int reference_count):
			data((unsigned long) c.data),
			reference_count(reference_count)
		{
		}

		bool is_free() const
		{
			return data & 1;
		}

		void free(unsigned int next)
		{
			data = (next << 1) + 1;
		}

		unsigned int next() const
		{
			assert_hotpath(is_free());
			return data >> 1;
		}

		clause get_clause() const
		{
			assert_hotpath(!is_free());
			return clause((clause::impl *) (data & ~1));
		}
	};

	std::vector<entry> clauses;

	/* The index of the first free entry, or clauses.size() if there
	 * is no free entry. */
	unsigned int first_free;

	clause_allocator():
		first_free(0)
	{
	}

	clause allocate(unsigned int nr_threads, unsigned int thread, bool learnt, const std::vector<literal> &v)
	{
		unsigned int id = first_free;
		clause c(thread, id, learnt, v);

		if (id == clauses.size()) {
			++first_free;
			clauses.push_back(entry(c, nr_threads));
		} else {
			first_free = clauses[id].next();
			clauses[id] = entry(c, nr_threads);
		}

		return c;
	}

	void free(unsigned int id)
	{
		entry &e = clauses[id];

		if (--e.reference_count == 0) {
			e.get_clause().free();
			e.free(first_free);
			first_free = id;
		}
	}
};

#endif
