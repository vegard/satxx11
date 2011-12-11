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

#ifndef PROPAGATE_WATCHLISTS_HH
#define PROPAGATE_WATCHLISTS_HH

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <queue>
#include <vector>

#include "assert_hotpath.hh"
#include "clause.hh"
#include "debug.hh"
#include "literal.hh"
#include "watch_indices.hh"
#include "watchlist.hh"

class propagate_watchlists {
public:
	unsigned int nr_variables;

	/* XXX: Maybe put this in its own class using uint8 or something. */
	std::vector<bool> valuation;
	watchlist *watchlists;

	/* Use as follows: watches[thread_id][clause_id] */
	std::vector<watch_indices> *watches;

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

	/* Decision level at which the variable was set. */
	/* XXX: Use uint32_t? */
	unsigned int *levels;

	literal conflict_literal;
	clause conflict_reason;

	propagate_watchlists(unsigned int nr_threads, unsigned int nr_variables):
		nr_variables(nr_variables),
		valuation(2 * nr_variables),
		watchlists(new watchlist[2 * nr_variables]),
		watches(new std::vector<watch_indices>[nr_threads]),
		trail(new unsigned int[nr_variables]),
		trail_index(0),
		decisions(new unsigned int[nr_variables]),
		decision_index(0),
		reasons(new clause[nr_variables]),
		levels(new unsigned int[nr_variables])
	{
	}

	~propagate_watchlists()
	{
		delete[] watchlists;
		delete[] watches;
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
		debug_enter("variable = $, value = $", variable, value);

		assert_hotpath(variable < nr_variables);
		assert_hotpath(!defined(variable));
		valuation[2 * variable + 0] = true;
		valuation[2 * variable + 1] = value;
	}

	void assign(literal l, bool value)
	{
		debug_enter("literal = $, value = $", l, value);

		assign(l.variable(), l.value() == value);
	}

	void unassign(unsigned int variable)
	{
		debug_enter("variable = $", variable);

		assert_hotpath(variable < nr_variables);
		assert_hotpath(defined(variable));
		valuation[2 * variable + 0] = false;
	}

	void attach(clause c, watch_indices w)
	{
		debug_enter("clause = $", c);

		assert_hotpath(c.size() >= 2);

		assert_hotpath(w[0] != w[1]);
		assert_hotpath(defined(c[w[0]]) && value(c[w[0]]));
		assert_hotpath(defined(c[w[1]]) && !value(c[w[1]]));

		watchlists[~c[w[0]]].insert(c);
		watchlists[~c[w[1]]].insert(c);

		/* XXX: A bit ugly. Please fix. */
		if (watches[c.thread()].size() <= c.index())
			watches[c.thread()].resize(c.index() + 1);
		watches[c.thread()][c.index()] = w;
	}

	void attach(clause c)
	{
		debug_enter("clause = $", c);

		assert_hotpath(c.size() >= 2);

		/* We can select undefined and true literals (if we select an
		 * undefined one, we may visit it during propagation when it
		 * becomes defined; if we select a satisfied literal, nothing
		 * will happen with the watch until we backtrack and the
		 * literal becomes undefined). Of course, we're better off
		 * here if we always select true literals that were defined
		 * early in the decision stack, etc. since this makes it less
		 * likely that we will follow a "stale" watch. */
		watch_indices w;

		unsigned int size = c.size();
		for (unsigned int i = 0; i < size; ++i) {
			if (defined(c[i]) && !value(c[i]))
				continue;

			w[0] = i;

			for (unsigned int j = i + 1; j < size; ++j) {
				if (defined(c[j]) && !value(c[j]))
					continue;

				w[1] = j;
				break;
			}

			break;
		}

		assert_hotpath(w[0] != w[1]);
		assert_hotpath(!defined(c[w[0]]) || value(c[w[0]]));
		assert_hotpath(!defined(c[w[1]]) || value(c[w[1]]));

		watchlists[~c[w[0]]].insert(c);
		watchlists[~c[w[1]]].insert(c);

		/* XXX: A bit ugly. Please fix. */
		if (watches[c.thread()].size() <= c.index())
			watches[c.thread()].resize(c.index() + 1);
		watches[c.thread()][c.index()] = w;
	}

	void detach(clause c)
	{
		debug_enter("clause = $", c);

		watch_indices w = watches[c.thread()][c.index()];
		watchlists[~c[w[0]]].remove(c);
		watchlists[~c[w[1]]].remove(c);
	}

	void decision(literal lit)
	{
		debug_enter("literal = $", lit);

		assert_hotpath(!defined(lit));

		unsigned int variable = lit.variable();
		assign(lit, true);
		trail[trail_index] = variable;
		decisions[decision_index] = trail_index;
		++trail_index;
		reasons[variable] = clause();
		++decision_index;
		levels[variable] = decision_index;
		queue.push(lit);
	}

	/* Called whenever a variable is forced to a particular value. The
	 * variable may be defined already, in which case we have a conflict
	 * and this function will return false. */
	bool implication(literal lit, clause reason)
	{
		debug_enter("literal = $", lit);

		if (defined(lit)) {
			/* If it's already defined to have the right value,
			 * we don't have a conflict. */
			if (value(lit))
				return debug_return(true, "$ /* no conflict; already defined */");

			conflict_literal = lit;
			conflict_reason = reason;
			return debug_return(false, "$ /* conflict */");
		}

		unsigned int variable = lit.variable();
		assign(lit, true);
		trail[trail_index] = variable;
		++trail_index;
		reasons[variable] = reason;
		levels[variable] = decision_index;
		queue.push(lit);
		return debug_return(true, "$ /* no conflict */");
	}

	/* Return false if and only if there was a conflict. */
	bool find_new_watch(clause c, unsigned int watch, bool &replace)
	{
		debug_enter("clause = $", c);

		/* At this point we know that the watchlists may not be
		 * entirely in sync with the current assignments, because
		 * variables are assigned before the propagations are
		 * actually carried out (and watchlists updated). */

		watch_indices wi = watches[c.thread()][c.index()];

		/* Find a new watch */
		unsigned int n = c.size();
		for (unsigned int i = 0; i < n; ++i) {
			literal l = c[i];

			if (defined(l)) {
				if (value(l)) {
					/* Literal was already satisfied;
					 * clause is satisfied; we don't
					 * need to do _anything_ else here */
					return debug_return(true, "$ /* clause is satisfied */");
				}

				/* Literal was falsified; we cannot
				 * use it as a watch */
				continue;
			}

			/* Don't select the other (existing) watch */
			if (i == wi[!watch])
				continue;

			/* Replace the old watch with the new one */
			replace = true;
			watches[c.thread()][c.index()][watch] = i;
			watchlists[~l].insert(c);
			return debug_return(true, "$ /* found new watch */");
		}

		/* There was no other watch to replace the one we just
		 * falsified; therefore, the other watched literal must
		 * be satisfied (this is the implication). */
		return debug_return(implication(c[wi[!watch]], c), "$");
	}

	/* Return false if and only if there was a conflict. */
	bool propagate(literal lit)
	{
		debug_enter("literal = $", lit);

		/* Every variable that we are propagating should
		 * already be defined with the correct polarity. */
		assert_hotpath(defined(lit));
		assert_hotpath(value(lit));

		/* Go through the watchlist; for each clause in
		 * the watchlist, we need to look for a new watch. */
		watchlist &w = watchlists[lit];

		unsigned int n = w.size();
		debug("watchlist size = $", n);

		for (unsigned int i = 0; i < n;) {
			assert_hotpath(i < w.size());

			clause c = w[i];
			watch_indices wi = watches[c.thread()][c.index()];

			/* This literal is exactly one of the watched literals */
			assert_hotpath((c[wi[0]] == ~lit) ^ (c[wi[1]] == ~lit));

			bool replace = false;
			if (!find_new_watch(c, c[wi[1]] == ~lit, replace))
				return debug_return(false, "$");

			if (replace) {
				w.watches[i] = w.watches[--n];
				w.watches.pop_back();
			} else {
				++i;
			}
		}

		return debug_return(true, "$");
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
				return debug_return(false, "$");
		}

		return debug_return(true, "$");
	}

	void backtrack(unsigned int decision)
	{
		debug_enter("decision = $", decision);

		assert(decision < decision_index);

		/* Unassign the variables that we've assigned since the
		 * given index. */
		unsigned new_trail_index = decisions[decision];
		debug("new trail index = $", new_trail_index);

		for (unsigned int i = trail_index; i-- > new_trail_index; )
			unassign(trail[i]);

		trail_index = new_trail_index;
		decision_index = decision;

		/* Clear queue (XXX: Use trail for this) */
		queue = std::queue<literal>();
	}
};

#endif
