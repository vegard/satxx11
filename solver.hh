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

#ifndef SOLVER_HH
#define SOLVER_HH

#include <cassert>
#include <cstdio>
#include <queue>
#include <vector>

extern "C" {
#include <stdint.h>
}

#include "assert_hotpath.hh"
#include "clause.hh"
#include "debug.hh"
#include "literal.hh"
#include "thread.hh"
#include "watch_indices.hh"
#include "watchlist.hh"

class solver {
public:
	unsigned int nr_variables;

	/* XXX: Maybe put this in its own class using uint8 or something. */
	std::vector<bool> valuation;
	watchlist *watchlists;
	std::vector<watch_indices> watches;

	/* XXX: Use uint32_t for variables */
	unsigned int *trail;
	unsigned int trail_index;

	/* XXX: Use uint32_t for variables */
	unsigned int *decisions;
	unsigned int decision_index;

	/* XXX: Use trail for this */
	std::queue<literal> queue;

	/* Indexed by variable. Gives the reason why a variable was set
	 * if the variable was implied. */
	clause *reasons;

	solver(unsigned int nr_variables, const std::vector<clause> &clauses):
		nr_variables(nr_variables),
		valuation(2 * nr_variables),
		watchlists(new watchlist[2 * nr_variables]),
		watches(clauses.size()),
		trail(new unsigned int[nr_variables]),
		trail_index(0),
		decisions(new unsigned int[nr_variables]),
		decision_index(0),
		reasons(new clause[nr_variables])
	{
		/* Attach all clauses in the original instance */
		for (unsigned int i = 0; i < clauses.size(); ++i)
			attach(clauses[i]);
	}

	~solver()
	{
		delete[] watchlists;
		delete[] trail;
		delete[] decisions;
	}

	bool defined(unsigned int variable) const
	{
		assert_hotpath(variable < nr_variables);
		return valuation[2 * variable + 0];
	}

	bool defined(literal l) const
	{
		return defined(l.variable());
	}

	bool value(unsigned int variable) const
	{
		assert_hotpath(variable < nr_variables);
		assert_hotpath(defined(variable));
		return valuation[2 * variable + 1];
	}

	bool value(literal l) const
	{
		return value(l.variable()) == l.value();
	}

	void assign(unsigned int variable, bool value)
	{
		debug_enter("variable = %u, value = %u", variable, value);

		assert_hotpath(variable < nr_variables);
		assert_hotpath(!defined(variable));
		valuation[2 * variable + 0] = true;
		valuation[2 * variable + 1] = value;

		debug_leave;
	}

	void assign(literal l, bool value)
	{
		debug_enter("literal = %s, value = %u", l.string().c_str(), value);

		assign(l.variable(), l.value() == value);

		debug_leave;
	}

	void unassign(unsigned int variable)
	{
		debug_enter("variable = %u", variable);

		assert_hotpath(variable < nr_variables);
		assert_hotpath(defined(variable));
		valuation[2 * variable + 0] = false;

		debug_leave;
	}

	void attach(clause c)
	{
		debug_enter("clause = %s", c.string().c_str());

		assert_hotpath(c.size() >= 2);

		/* Pick watches among the unassigned literals. */
		watch_indices w;

		unsigned int size = c.size();
		for (unsigned int i = 0; i < size; ++i) {
			if (defined(c[i]))
				continue;

			w[0] = i;

			for (unsigned int j = i + 1; j < size; ++j) {
				if (defined(c[j]))
					continue;

				w[1] = j;
				break;
			}

			break;
		}

		watchlists[~c[w[0]]].insert(c);
		watchlists[~c[w[1]]].insert(c);
		watches[c.index()] = w;

		debug_leave;
	}

	void detach(clause c)
	{
		debug_enter("clause = %s", c.string().c_str());

		watch_indices w = watches[c.index()];
		watchlists[~c[w[0]]].remove(c);
		watchlists[~c[w[1]]].remove(c);

		debug_leave;
	}

	void decision(literal lit)
	{
		debug_enter("literal = %s", lit.string().c_str());

		assert_hotpath(!defined(lit));

		unsigned int variable = lit.variable();
		assign(lit, true);
		trail[trail_index] = lit.variable();
		decisions[decision_index++] = trail_index++;
		reasons[variable] = clause();
		queue.push(lit);

		debug_leave;
	}

	/* Called whenever a variable is forced to a particular value. The
	 * variable may be defined already, in which case we have a conflict
	 * and this function will return false. */
	bool implication(literal lit, clause reason)
	{
		debug_enter("literal = %s", lit.string().c_str());

		if (defined(lit)) {
			/* If it's already defined to have the right value,
			 * we don't have a conflict. */
			if (value(lit))
				return debug_return(true, "true /* no conflict; already defined */");

			return debug_return(false, "false /* conflict */");
		}

		unsigned int variable = lit.variable();
		assign(lit, true);
		trail[trail_index++] = variable;
		reasons[variable] = reason;
		queue.push(lit);
		return debug_return(true, "true /* no conflict */");
	}

	/* Return false if and only if there was a conflict. */
	bool find_new_watch(clause c, unsigned int watch)
	{
		debug_enter("clause = %s", c.string().c_str());

		/* At this point we know that the watchlists may not be
		 * entirely in sync with the current assignments, because
		 * variables are assigned before the propagations are
		 * actually carried out (and watchlists updated). */

		watch_indices wi = watches[c.index()];

		/* Find a new watch */
		unsigned int n = c.size();
		for (unsigned int i = 0; i < n; ++i) {
			literal l = c[i];

			if (defined(l)) {
				if (value(l)) {
					/* Literal was already satisfied;
					 * clause is satisfied; we don't
					 * need to do _anything_ else here */
					return debug_return(true, "true /* clause is satisfied */");
				}

				/* Literal was falsified; we cannot
				 * use it as a watch */
				continue;
			}

			/* Don't select the other (existing) watch */
			if (i == wi[!watch])
				continue;

			/* Replace the old watch with the new one */
			watchlists[~c[wi[watch]]].remove(c);
			watches[c.index()][watch] = i;
			watchlists[~l].insert(c);
			return debug_return(true, "true /* found new watch */");
		}

		/* There was no other watch to replace the one we just
		 * falsified; therefore, the other watched literal must
		 * be satisfied (this is the implication). */
		return debug_return(implication(c[wi[!watch]], c), "<same>");
	}

	/* Return false if and only if there was a conflict. */
	bool propagate(literal lit)
	{
		debug_enter("literal = %s", lit.string().c_str());

		/* Every variable that we are propagating should
		 * already be defined with the correct polarity. */
		assert_hotpath(defined(lit));
		assert_hotpath(value(lit));

		/* Go through the watchlist; for each clause in
		 * the watchlist, we need to look for a new watch. */
		watchlist &w = watchlists[lit];

		unsigned int n = w.size();
		debug("watchlist size = %u", n);

		for (unsigned int i = 0; i < n; ++i) {
			clause c = w[i];
			watch_indices wi = watches[c.index()];

			/* This literal is exactly one of the watched literals */
			assert_hotpath((c[wi[0]] == ~lit) ^ (c[wi[1]] == ~lit));

			if (!find_new_watch(c, c[wi[1]] == ~lit))
				return debug_return(false, "false");
		}

		return debug_return(true, "true");
	}

	/* Return false if and only if there was a conflict. */
	bool propagate()
	{
		debug_enter("");

		while (!queue.empty()) {
			literal lit = queue.front();
			queue.pop();

			/* If propagation caused a conflict, abort the rest
			 * of the conflicts as well. */
			if (!propagate(lit))
				return debug_return(false, "false");
		}

		return debug_return(true, "true");
	}

	void backtrack(unsigned int decision)
	{
		debug_enter("decision = %u", decision);

		assert(decision < decision_index);

		/* Unassign the variables that we've assigned since the
		 * given index. */
		unsigned new_trail_index = decisions[decision];
		debug("new trail index = %u", new_trail_index);

		for (unsigned int i = trail_index; i-- > new_trail_index; )
			unassign(trail[i]);

		trail_index = new_trail_index;
		decision_index = decision;

		/* Clear queue (XXX: Use trail for this) */
		queue = std::queue<literal>();

		debug_leave;
	}
};

#endif
