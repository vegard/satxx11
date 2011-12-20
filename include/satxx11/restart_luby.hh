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

#ifndef SATXX11_RESTART_LUBY_HH
#define SATXX11_RESTART_LUBY_HH

namespace satxx11 {

class restart_luby {
private:
	static unsigned int luby(unsigned int i)
	{
		for (unsigned int k = 1; k < 32; ++k) {
			if (i == (1U << k) - 1)
				return 1 << (k - 1);
		}

		for (unsigned int k = 1; ; ++k) {
			if ((1U << (k - 1)) <= i && i < (1U << k) - 1)
				return luby(i - (1 << (k - 1)) + 1);
		}
	}

public:
	unsigned int counter;

	unsigned int value;
	unsigned int max;

	restart_luby():
		counter(0),
		value(1),
		max(luby(1))
	{
	}

	bool operator()()
	{
		if (++counter < max)
			return false;

		counter = 0;
		max = luby(++value);
		return true;
	}
};

}

#endif
