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

#ifndef SATXX11_PROPAGATE_CLAUSE_HH
#define SATXX11_PROPAGATE_CLAUSE_HH

#include <cstdint>
#include <cstdio>
#include <vector>

#include <satxx11/assert.hh>
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
class propagate_clause {
public:
	static_assert(propagate_prefetch_first_clause < propagate_prefetch_watchlist,
		"propagate_prefetch_first_clause < propagate_prefetch_watchlist");
	static_assert(propagate_prefetch_watchlist < propagate_prefetch_clause,
		"propagate_prefetch_watchlist < propagate_prefetch_clause");

	/* XXX: Maybe put this in its own class using uint8 or something. */
	watchlist *watchlists;

	/* Use as follows: watches[thread_id][clause_id] */
	std::vector<watch_indices> *watches;

	propagate_clause()
	{
	}

	template<class Solver>
	void start(Solver &s)
	{
		watchlists = new watchlist[2 * s.nr_variables];
		watches = new std::vector<watch_indices>[s.nr_threads];
	}

	~propagate_clause()
	{
		delete[] watchlists;
		delete[] watches;
	}

	template<class Solver, typename ClauseType>
	bool attach(Solver &s, ClauseType c)
	{
		return true;
	}

	template<class Solver>
	void attach(Solver &s, clause c, unsigned int i, unsigned int j)
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

	template<class Solver>
	bool attach(Solver &s, std::tuple<clause, unsigned int, unsigned int> c)
	{
		/* Unpack arguments and call the right function. */
		attach(s, std::get<0>(c), std::get<1>(c), std::get<2>(c));
		return true;
	}

	template<class Solver>
	void attach_true_and_undefined(Solver &s, clause c)
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
			if (s.defined(c[i]) && !s.value(c[i]))
				continue;

			for (unsigned int j = i + 1; j < size; ++j) {
				if (s.defined(c[j]) && !s.value(c[j]))
					continue;

				attach(c, i, j);
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
			if (s.defined(c[i])) {
				if (s.value(c[i])) {
					if (!(found & 1)) {
						found |= 1;
						found_true = i;
					} else {
						/* Found two true literals */
						attach(s, c, found_true, i);
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
					attach(s, c, found_true, found_undefined);
					return true;
				} else {
					/* Found two undefined literals */
					attach(s, c, found_undefined, i);
					return true;
				}
			}
		}

		if (found & 1) {
			/* We found only one true literal. */
			if (found & 4) {
				attach(s, c, found_true, found_undefined);
				return true;
			} else if (found & 2) {
				attach(s, c, found_true, found_false);
				return true;
			}
		} else if (found & 4) {
			/* No true literal, but we did find an undefined one, so the undefined one
			 * must be implied. */
			/* XXX: implication() assumes the literal may be defined or undefined. From
			 * this particular callsite it is always undefined, so we could optimize it. */
			return s.implication(c[found_undefined], c);
		}

		/* No true literal and no undefined literal. This is a conflict
		 * if ever I saw one! */
		return false;
	}

	template<class Solver, typename ClauseType>
	void detach(Solver &s, ClauseType c)
	{
	}

	template<class Solver>
	void detach(Solver &s, clause c)
	{
		debug_enter("clause = $", c);

		watch_indices w = watches[c.thread()][c.index()];
		watchlists[~c[w[0]]].remove(c);
		watchlists[~c[w[1]]].remove(c);
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

			if (s.defined(l)) {
				if (s.value(l)) {
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
		return debug_return(s.implication(c[wi[!watch]], c), "$");
	}

	/* Return false if and only if there was a conflict. */
	template<class Solver>
	bool propagate(Solver &s, literal lit)
	{
		debug_enter("literal = $", lit);

		/* Every variable that we are propagating should
		 * already be defined with the correct polarity. */
		assert_hotpath(s.defined(lit));
		assert_hotpath(s.value(lit));

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
};

}

#endif
