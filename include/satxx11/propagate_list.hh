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

#ifndef SATXX11_PROPAGATE_LIST_HH
#define SATXX11_PROPAGATE_LIST_HH

namespace satxx11 {

template<typename... Propagations>
class propagate_list {
public:
	std::tuple<Propagations...> propagations;

	template<class Solver, unsigned int I = 0, typename... Args>
	typename std::enable_if<I == sizeof...(Args), void>::type start(Solver &s, std::tuple<Args...> &args)
	{
	}

	template<class Solver, unsigned int I = 0, typename... Args>
	typename std::enable_if<I < sizeof...(Args), void>::type start(Solver &s, std::tuple<Args...> &args)
	{
		std::get<I>(args).start(s);
		start<Solver, I + 1>(s, args);
	}

	template<class Solver>
	void start(Solver &s)
	{
		start(s, propagations);
	}

	template<class Solver, class ClauseType, unsigned int I = 0, typename... Args>
	typename std::enable_if<I == sizeof...(Args), bool>::type attach(Solver &s, ClauseType clause, std::tuple<Args...> &args)
	{
		return true;
	}

	template<class Solver, class ClauseType, unsigned int I = 0, typename... Args>
	typename std::enable_if<I < sizeof...(Args), bool>::type attach(Solver &s, ClauseType clause, std::tuple<Args...> &args)
	{
		if (!std::get<I>(args).attach(s, clause))
			return false;

		return attach<Solver, ClauseType, I + 1>(s, clause, args);
	}

	template<class Solver, class ClauseType>
	bool attach(Solver &s, ClauseType clause)
	{
		return attach(s, clause, propagations);
	}

	template<class Solver, class ClauseType, unsigned int I = 0, typename... Args>
	typename std::enable_if<I == sizeof...(Args), void>::type detach(Solver &s, ClauseType clause, std::tuple<Args...> &args)
	{
	}

	template<class Solver, class ClauseType, unsigned int I = 0, typename... Args>
	typename std::enable_if<I < sizeof...(Args), void>::type detach(Solver &s, ClauseType clause, std::tuple<Args...> &args)
	{
		std::get<I>(args).detach(s, clause);
		detach<Solver, ClauseType, I + 1>(s, clause, args);
	}

	template<class Solver, class ClauseType>
	void detach(Solver &s, ClauseType clause)
	{
		detach(s, clause, propagations);
	}

	template<class Solver, unsigned int I = 0, typename... Args>
	typename std::enable_if<I == sizeof...(Args), bool>::type propagate(Solver &s, literal lit, std::tuple<Args...> &args)
	{
		return true;
	}

	template<class Solver, unsigned int I = 0, typename... Args>
	typename std::enable_if<I < sizeof...(Args), bool>::type propagate(Solver &s, literal lit, std::tuple<Args...> &args)
	{
		if (!std::get<I>(args).propagate(s, lit))
			return false;

		return propagate<Solver, I + 1>(s, lit, args);
	}

	template<class Solver>
	bool propagate(Solver &s, literal lit)
	{
		return propagate(s, lit, propagations);
	}
};

}

#endif
