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

#ifndef SATXX11_PLUGIN_LIST_HH
#define SATXX11_PLUGIN_LIST_HH

#include <tuple>
#include <utility>

#include <satxx11/clause.hh>
#include <satxx11/literal.hh>

namespace satxx11 {

/* Specify a possibly empty ordered list of generic plugins. This class just
 * forwards the calls of the solver to each element in the list. */
template<typename... Plugins>
class plugin_list {
public:
	std::tuple<Plugins...> plugins;

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
		start(s, plugins);
	}

	template<class Solver, unsigned int I = 0, typename... Args>
	typename std::enable_if<I == sizeof...(Args), void>::type assign(Solver &s, std::tuple<Args...> &args, unsigned int variable, bool value)
	{
	}

	template<class Solver, unsigned int I = 0, typename... Args>
	typename std::enable_if<I < sizeof...(Args), void>::type assign(Solver &s, std::tuple<Args...> &args, unsigned int variable, bool value)
	{
		std::get<I>(args).assign(s, variable, value);
		assign<Solver, I + 1>(s, args, variable, value);
	}

	template<class Solver>
	void assign(Solver &s, unsigned int variable, bool value)
	{
		assign(s, plugins, variable, value);
	}

	template<class Solver, unsigned int I = 0, typename... Args>
	typename std::enable_if<I == sizeof...(Args), void>::type unassign(Solver &s, std::tuple<Args...> &args, unsigned int variable)
	{
	}

	template<class Solver, unsigned int I = 0, typename... Args>
	typename std::enable_if<I < sizeof...(Args), void>::type unassign(Solver &s, std::tuple<Args...> &args, unsigned int variable)
	{
		std::get<I>(args).unassign(s, variable);
		unassign<Solver, I + 1>(s, args, variable);
	}

	template<class Solver>
	void unassign(Solver &s, unsigned int variable)
	{
		unassign(s, plugins, variable);
	}

	template<class Solver, unsigned int I = 0, typename... Args>
	typename std::enable_if<I == sizeof...(Args), void>::type attach(Solver &s, std::tuple<Args...> &args, literal lit)
	{
	}

	template<class Solver, unsigned int I = 0, typename... Args>
	typename std::enable_if<I < sizeof...(Args), void>::type attach(Solver &s, std::tuple<Args...> &args, literal lit)
	{
		std::get<I>(args).attach(s, lit);
		attach<Solver, I + 1>(s, args, lit);
	}

	template<class Solver>
	void attach(Solver &s, literal lit)
	{
		attach(s, plugins, lit);
	}

	template<class Solver, unsigned int I = 0, typename... Args>
	typename std::enable_if<I == sizeof...(Args), void>::type attach(Solver &s, std::tuple<Args...> &args, clause c)
	{
	}

	template<class Solver, unsigned int I = 0, typename... Args>
	typename std::enable_if<I < sizeof...(Args), void>::type attach(Solver &s, std::tuple<Args...> &args, clause c)
	{
		std::get<I>(args).attach(s, c);
		attach<Solver, I + 1>(s, args, c);
	}

	template<class Solver>
	void attach(Solver &s, clause c)
	{
		attach(s, plugins, c);
	}

	template<class Solver, unsigned int I = 0, typename... Args>
	typename std::enable_if<I == sizeof...(Args), void>::type detach(Solver &s, std::tuple<Args...> &args, clause c)
	{
	}

	template<class Solver, unsigned int I = 0, typename... Args>
	typename std::enable_if<I < sizeof...(Args), void>::type detach(Solver &s, std::tuple<Args...> &args, clause c)
	{
		std::get<I>(args).detach(s, c);
		detach<Solver, I + 1>(s, args, c);
	}

	template<class Solver>
	void detach(Solver &s, clause c)
	{
		detach(s, plugins, c);
	}

	template<class Solver, unsigned int I = 0, typename... Args>
	typename std::enable_if<I == sizeof...(Args), void>::type decision(Solver &s, std::tuple<Args...> &args, literal lit)
	{
	}

	template<class Solver, unsigned int I = 0, typename... Args>
	typename std::enable_if<I < sizeof...(Args), void>::type decision(Solver &s, std::tuple<Args...> &args, literal lit)
	{
		std::get<I>(args).decision(s, lit);
		decision<Solver, I + 1>(s, args, lit);
	}

	template<class Solver>
	void decision(Solver &s, literal lit)
	{
		decision(s, plugins, lit);
	}

	template<class Solver, unsigned int I = 0, typename... Args>
	typename std::enable_if<I == sizeof...(Args), void>::type implication(Solver &s, std::tuple<Args...> &args, literal lit, clause reason)
	{
	}

	template<class Solver, unsigned int I = 0, typename... Args>
	typename std::enable_if<I < sizeof...(Args), void>::type implication(Solver &s, std::tuple<Args...> &args, literal lit, clause reason)
	{
		std::get<I>(args).implication(s, lit, reason);
		implication<Solver, I + 1>(s, args, lit, reason);
	}

	template<class Solver>
	void implication(Solver &s, literal lit, clause reason)
	{
		implication(s, plugins, lit, reason);
	}

	template<class Solver, unsigned int I = 0, typename... Args>
	typename std::enable_if<I == sizeof...(Args), void>::type conflict(Solver &s, std::tuple<Args...> &args)
	{
	}

	template<class Solver, unsigned int I = 0, typename... Args>
	typename std::enable_if<I < sizeof...(Args), void>::type conflict(Solver &s, std::tuple<Args...> &args)
	{
		std::get<I>(args).conflict(s);
		conflict<Solver, I + 1>(s, args);
	}

	template<class Solver>
	void conflict(Solver &s)
	{
		conflict(s, plugins);
	}

	template<class Solver, unsigned int I = 0, typename... Args>
	typename std::enable_if<I == sizeof...(Args), void>::type backtrack(Solver &s, std::tuple<Args...> &args, unsigned int decision)
	{
	}

	template<class Solver, unsigned int I = 0, typename... Args>
	typename std::enable_if<I < sizeof...(Args), void>::type backtrack(Solver &s, std::tuple<Args...> &args, unsigned int decision)
	{
		std::get<I>(args).backtrack(s, decision);
		backtrack<Solver, I + 1>(s, args, decision);
	}

	template<class Solver>
	void backtrack(Solver &s, unsigned int decision)
	{
		backtrack(s, plugins, decision);
	}

	template<class Solver, unsigned int I = 0, typename... Args>
	typename std::enable_if<I == sizeof...(Args), void>::type restart(Solver &s, std::tuple<Args...> &args)
	{
	}

	template<class Solver, unsigned int I = 0, typename... Args>
	typename std::enable_if<I < sizeof...(Args), void>::type restart(Solver &s, std::tuple<Args...> &args)
	{
		std::get<I>(args).restart(s);
		restart<Solver, I + 1>(s, args);
	}

	template<class Solver>
	void restart(Solver &s)
	{
		restart(s, plugins);
	}

	template<class Solver, unsigned int I = 0, typename... Args>
	typename std::enable_if<I == sizeof...(Args), void>::type sat(Solver &s, std::tuple<Args...> &args)
	{
	}

	template<class Solver, unsigned int I = 0, typename... Args>
	typename std::enable_if<I < sizeof...(Args), void>::type sat(Solver &s, std::tuple<Args...> &args)
	{
		std::get<I>(args).sat(s);
		sat<Solver, I + 1>(s, args);
	}

	template<class Solver>
	void sat(Solver &s)
	{
		sat(s, plugins);
	}

	template<class Solver, unsigned int I = 0, typename... Args>
	typename std::enable_if<I == sizeof...(Args), void>::type unsat(Solver &s, std::tuple<Args...> &args)
	{
	}

	template<class Solver, unsigned int I = 0, typename... Args>
	typename std::enable_if<I < sizeof...(Args), void>::type unsat(Solver &s, std::tuple<Args...> &args)
	{
		std::get<I>(args).unsat(s);
		unsat<Solver, I + 1>(s, args);
	}

	template<class Solver>
	void unsat(Solver &s)
	{
		unsat(s, plugins);
	}
};

}

#endif
