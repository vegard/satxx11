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

#ifndef SATXX11_RESTART_GEOMETRIC_HH
#define SATXX11_RESTART_GEOMETRIC_HH

namespace satxx11 {

template<unsigned int initial, unsigned int per, unsigned int cent = 100>
class restart_geometric {
public:
	unsigned int counter;
	double value;

	restart_geometric():
		counter(0),
		value(initial)
	{
	}

	bool operator()()
	{
		if (++counter < value)
			return false;

		counter = 0;
		value *= (1 + 1. * per / cent);
		return true;
	}
};

}

#endif
