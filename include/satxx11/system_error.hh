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

#ifndef SATXX11_SYSTEM_ERROR_HH
#define SATXX11_SYSTEM_ERROR_HH

#include <cstring>
#include <stdexcept>

namespace satxx11 {

class system_error:
	public std::runtime_error
{
public:
	system_error(int errno_copy):
		std::runtime_error(strerror(errno_copy))
	{
	}
};

}

#endif
