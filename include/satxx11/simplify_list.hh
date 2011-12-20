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

#ifndef SATXX11_SIMPLIFY_LIST_HH
#define SATXX11_SIMPLIFY_LIST_HH

#include <tuple>
#include <utility>

namespace satxx11 {

/* Specify an ordered list of simplification routines to call */
template<typename... Simplifies>
class simplify_list {
public:
	std::tuple<Simplifies...> simplifies;

	simplify_list()
	{
	}

	template<unsigned int I = 0, typename... Args>
	typename std::enable_if<I == sizeof...(Args), void>::type call(std::tuple<Args...> &t)
	{
	}

	template<unsigned int I = 0, typename... Args>
	typename std::enable_if<I < sizeof...(Args), void>::type call(std::tuple<Args...> &t)
	{
		std::get<I>(t)();
		call<I + 1>(t);
	}

	void operator()()
	{
		call(simplifies);
	}
};

}

#endif
