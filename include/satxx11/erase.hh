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

#ifndef SATXX11_ERASE_HH
#define SATXX11_ERASE_HH

#include <algorithm>

namespace satxx11 {

#if 0
template<class Iterator, class T>
Iterator erase(Iterator first, Iterator last, const T &value)
{
	std::copy_n(last - 1, 1, std::find(first, last, value));
	return last - 1;
}
#endif

template<class Container, typename T>
void erase(Container &container, const T &value)
{
	container.erase(std::remove(container.begin(), container.end(), value));
}

}

#endif
