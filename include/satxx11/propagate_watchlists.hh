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

#ifndef SATXX11_PROPAGATE_WATCHLISTS_HH
#define SATXX11_PROPAGATE_WATCHLISTS_HH

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>

#include <satxx11/assert_hotpath.hh>
#include <satxx11/clause.hh>
#include <satxx11/debug.hh>
#include <satxx11/literal.hh>
#include <satxx11/watch_indices.hh>
#include <satxx11/watchlist.hh>

namespace satxx11 {

template<unsigned int propagate_prefetch_first_clause = 2,
	/* We tested baseline (162s), +1 (148s), +2 (138s),
	 * +3 (136s), +4 (136s), +5 (140s). Supplying an
	 * argument of 1 for the rw parameter made it worse
	 * by 2-10s. */
	unsigned int propagate_prefetch_watchlist = 3,
	/* We tested baseline (138s), +4 (121s), +5 (110s),
	 * and +6 (112s). */
	unsigned int propagate_prefetch_clause = 5>
class propagate_watchlists {
public:
	static_assert(propagate_prefetch_first_clause < propagate_prefetch_watchlist,
		"propagate_prefetch_first_clause < propagate_prefetch_watchlist");
	static_assert(propagate_prefetch_watchlist < propagate_prefetch_clause,
		"propagate_prefetch_watchlist < propagate_prefetch_clause");

	unsigned int nr_variables;

	/* XXX: Maybe put this in its own class using uint8 or something. */
	std::vector<bool> valuation;
	watchlist *watchlists;

	/* Use as follows: watches[thread_id][clause_id] */
	std::vector<watch_indices> *watches;

	unsigned int *trail;
	unsigned int trail_index;
	unsigned int trail_size;

	unsigned int *decisions;
	unsigned int decision_index;

	/* Indexed by variable. Gives the reason why a variable was set
	 * if the variable was implied. */
	clause *reasons;

	/* Decision level at which the variable was set. */
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
		trail_size(0),
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

	void unassign(unsigned int variable)
	{
		debug_enter("variable = $", variable);

		assert_hotpath(variable < nr_variables);
		assert_hotpath(defined(variable));
		valuation[2 * variable + 0] = false;
	}

	void attach_with_watches(clause c, unsigned int i, unsigned int j)
	{
		debug_enter("clause = $, i = $, j = $", c, i, j);

		assert_hotpath(c.size() >= 2);
		assert_hotpath(i != j);

		watch_indices w(i, j);
		watchlists[~c[w[0]]].insert(c);
		watchlists[~c[w[1]]].insert(c);

		/* XXX: A bit ugly. Please fix. */
		if (watches[c.thread()].size() <= c.index())
			watches[c.thread()].resize(c.index() + 1);
		watches[c.thread()][c.index()] = w;
	}

	void attach_true_and_undefined(clause c)
	{
		/* We can select undefined and true literals (if we select an
		 * undefined one, we may visit it during propagation when it
		 * becomes defined; if we select a satisfied literal, nothing
		 * will happen with the watch until we backtrack and the
		 * literal becomes undefined). Of course, we're better off
		 * here if we always select true literals that were defined
		 * early in the decision stack, etc. since this makes it less
		 * likely that we will follow a "stale" watch. */

		unsigned int size = c.size();
		for (unsigned int i = 0; i < size; ++i) {
			if (defined(c[i]) && !value(c[i]))
				continue;

			for (unsigned int j = i + 1; j < size; ++j) {
				if (defined(c[j]) && !value(c[j]))
					continue;

				attach_with_watches(c, i, j);
				return;
			}

			assert(false);
		}

		assert(false);
	}

	/* Attach a clause that may be in any state (partially or completely
	 * satisfied or falsified). Returns false if and only if there was a
	 * conflict. */
	template<class Solver>
	bool attach(Solver &s, clause c) /* XXX: __attribute__ ((warn_unused_result)) */
	{
		debug_enter("clause = $", c);

		assert_hotpath(c.size() >= 2);

		watch_indices w;

		/* This is an attempt at making a somewhat fast, one-pass search
		 * for all kinds of literals. This function is typically called
		 * for clauses received from other threads, and the clauses we
		 * receive are typically long-ish, so we want it to be really
		 * efficient. We try to cache one true literal, one undefined
		 * literal, and one false literal, since this is all we really
		 * need to know in the worst case. */
		uint8_t found = 0;
		unsigned int found_true;
		unsigned int found_false;
		unsigned int found_undefined;

		unsigned int size = c.size();
		for (unsigned int i = 0; i < size; ++i) {
			if (defined(c[i])) {
				if (value(c[i])) {
					if (!(found & 1)) {
						found |= 1;
						found_true = i;
					} else {
						/* Found two true literals */
						attach_with_watches(c, found_true, i);
						return true;
					}
				} else {
					if (!(found & 2)) {
						found |= 2;
						found_false = i;
					}
				}
			} else {
				if (!(found & 4)) {
					found |= 4;
					found_undefined = i;
				} else if (found & 1) {
					/* We have one true and one undefined literal */
					attach_with_watches(c, found_true, found_undefined);
					return true;
				} else {
					/* Found two undefined literals */
					attach_with_watches(c, found_undefined, i);
					return true;
				}
			}
		}

		if (found & 1) {
			/* We found only one true literal. */
			if (found & 4) {
				attach_with_watches(c, found_true, found_undefined);
				return true;
			} else if (found & 2) {
				attach_with_watches(c, found_true, found_false);
				return true;
			}
		} else {
			assert(found & 4);
			/* No true literal! This means one of the undefined ones must be implied. */
			/* XXX: implication() assumes the literal may be defined or undefined. From
			 * this particular callsite it is always undefined, so we could optimize it. */
			return implication(s, c[found_undefined], c);
		}

		assert(false);
	}

	void detach(clause c)
	{
		debug_enter("clause = $", c);

		watch_indices w = watches[c.thread()][c.index()];
		watchlists[~c[w[0]]].remove(c);
		watchlists[~c[w[1]]].remove(c);
	}

	template<class Solver>
	void decision(Solver &s, literal lit)
	{
		debug_enter("literal = $", lit);

		assert_hotpath(!defined(lit));
		assert_hotpath(trail_index == trail_size);

		unsigned int variable = lit.variable();
		s.assign(lit, true);
		trail[trail_size] = variable;
		decisions[decision_index] = trail_size;
		++trail_size;
		reasons[variable] = clause();
		++decision_index;
		levels[variable] = decision_index;
	}

	/* Called whenever a variable is forced to a particular value. The
	 * variable may be defined already, in which case we have a conflict
	 * and this function will return false. */
	template<class Solver>
	bool implication(Solver &s, literal lit, clause reason)
	{
		debug_enter("literal = $", lit);

		if (defined(lit)) {
			/* If it's already defined to have the right value,
			 * we don't have a conflict. */
			/* XXX: In this case, Minisat changes reasons[variable]
			 * if the new reason is a smaller clause. */
			if (value(lit))
				return debug_return(true, "$ /* no conflict; already defined */");

			conflict_literal = lit;
			conflict_reason = reason;
			return debug_return(false, "$ /* conflict */");
		}

		unsigned int variable = lit.variable();
		s.assign(lit, true);
		trail[trail_size] = variable;
		++trail_size;
		reasons[variable] = reason;
		levels[variable] = decision_index;
		return debug_return(true, "$ /* no conflict */");
	}

	/* Return false if and only if there was a conflict. */
	template<class Solver>
	bool find_new_watch(Solver &s, clause c, const watch_indices &wi, unsigned int watch, bool &replace)
	{
		debug_enter("clause = $", c);

		/* At this point we know that the watchlists may not be
		 * entirely in sync with the current assignments, because
		 * variables are assigned before the propagations are
		 * actually carried out (and watchlists updated). */

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
		return debug_return(implication(s, c[wi[!watch]], c), "$");
	}

	/* Return false if and only if there was a conflict. */
	template<class Solver>
	bool propagate(Solver &s, literal lit)
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

		if (propagate_prefetch_first_clause < n)
			__builtin_prefetch(w[propagate_prefetch_first_clause].data, 0);

		for (unsigned int i = 0; i < n;) {
			if (i + propagate_prefetch_clause < n)
				__builtin_prefetch(w[i + propagate_prefetch_clause].data, 0);

			if (i + propagate_prefetch_watchlist < n)
				__builtin_prefetch(&watches[w[i + propagate_prefetch_watchlist].thread()][w[i + propagate_prefetch_watchlist].index()], 0);

			/* Don't use "n" here... That's the whole point; we want to
			 * make sure that n correctly reflects the true size of the
			 * watchlist. */
			assert_hotpath(i < w.size());

			clause c = w[i];
			watch_indices wi = watches[c.thread()][c.index()];

			/* This literal is exactly one of the watched literals */
			assert_hotpath((c[wi[0]] == ~lit) ^ (c[wi[1]] == ~lit));

			bool replace = false;
			if (!find_new_watch(s, c, wi, c[wi[1]] == ~lit, replace))
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
	template<class Solver>
	bool propagate(Solver &s)
	{
		debug_enter("");

		while (trail_index < trail_size) {
			debug("trail_index = $, trail_size = $", trail_index, trail_size);
			unsigned int var = trail[trail_index++];

			/* If propagation caused a conflict, abort the rest
			 * of the conflicts as well. */
			/* XXX: Make a version of propagate() that takes the
			 * variable and the value as separate arguments. */
			if (!propagate(s, literal(var, value(var))))
				return debug_return(false, "$");
		}

		return debug_return(true, "$");
	}

	template<class Solver>
	void backtrack(Solver &s, unsigned int decision)
	{
		debug_enter("decision = $", decision);

		assert(decision < decision_index);

		/* Unassign the variables that we've assigned since the
		 * given index. */
		unsigned new_trail_size = decisions[decision];
		debug("new trail size = $", new_trail_size);

		/* XXX: It doesn't REALLY matter what order we do this in. */
		for (unsigned int i = trail_size; i-- > new_trail_size; )
			s.unassign(trail[i]);

		trail_index = new_trail_size;
		trail_size = new_trail_size;
		decision_index = decision;
	}
};

}

#endif
