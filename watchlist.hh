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

#ifndef WATCHLIST_HH
#define WATCHLIST_HH

#include <vector>

#include "clause.hh"

class watchlist {
public:
	/* XXX: Turn into a single pointer */
	std::vector<clause> watches;

	watchlist()
	{
	}

	const clause operator[](unsigned int i) const
	{
		return watches[i];
	}

	clause &operator[](unsigned int i)
	{
		return watches[i];
	}

	unsigned int size() const
	{
		return watches.size();
	}

	void insert(clause c)
	{
		watches.push_back(c);
	}

	void remove(clause c)
	{
		unsigned int size = watches.size();
		for (unsigned int i = 0; i < size; ++i) {
			if (watches[i] == c) {
				watches[i] = watches[size - 1];
				watches.pop_back();
				return;
			}
		}
	}
};

#endif
