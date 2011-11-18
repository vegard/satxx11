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

#ifndef WATCH_INDICES_HH
#define WATCH_INDICES_HH

extern "C" {
#include <stdint.h>
}

/* This class represents a pair of indices into the clause's literals. */
class watch_indices {
public:
	/* XXX: We MAY consider downgrading this to uint8_t for space/cache
	 * efficiency reasons. But it imposes a limit of 256 literals per
	 * clause. */
	uint16_t index[2];

	watch_indices()
	{
	}

	watch_indices(uint16_t i, uint16_t j):
		index({i, j})
	{
	}

	uint16_t operator[](unsigned int i) const
	{
		return index[i];
	}

	uint16_t &operator[](unsigned int i)
	{
		return index[i];
	}
};

#endif
