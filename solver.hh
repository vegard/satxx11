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

#include <atomic>
#include <random>

#include "analyze_1uip.hh"
#include "clause.hh"
#include "debug.hh"
#include "decide_random.hh"
#include "decide_vsids.hh"
#include "literal.hh"
#include "print_noop.hh"
#include "print_stdio.hh"
#include "propagate_watchlists.hh"
#include "reduce_noop.hh"
#include "restart_and.hh"
#include "restart_fixed.hh"
#include "restart_geometric.hh"
#include "restart_luby.hh"
#include "restart_nested.hh"
#include "restart_not.hh"
#include "restart_or.hh"

typedef unsigned int variable;
typedef std::map<variable, variable> variable_map;
typedef std::vector<clause> clause_vector;
typedef std::vector<literal> literal_vector;

template<class Random = std::ranlux24_base,
	class Decide = decide_vsids,
	class Propagate = propagate_watchlists,
	class Analyze = analyze_1uip,
	class Restart = restart_nested<restart_geometric<100, 10>, restart_geometric<100, 10>>,
	class Reduce = reduce_noop,
	class Print = print_stdio>
class solver {
public:
	unsigned int id;
	bool keep_going;
	std::atomic<bool> &should_exit;
	std::atomic<unsigned int> &clause_counter;
	unsigned int nr_variables;
	const variable_map &variables;
	const variable_map &reverse_variables;
	const clause_vector &clauses;
	const literal_vector &unit_clauses;

	Random random;
	Decide decide;
	Propagate propagate;
	Analyze analyze;
	Restart restart;
	Reduce reduce;
	Print print;

	solver(unsigned int id,
		bool keep_going,
		std::atomic<bool> &should_exit,
		std::atomic<unsigned int> &clause_counter,
		unsigned long seed,
		const variable_map &variables,
		const variable_map &reverse_variables,
		const clause_vector &clauses,
		const literal_vector &unit_clauses):
		id(id),
		keep_going(keep_going),
		should_exit(should_exit),
		clause_counter(clause_counter),
		nr_variables(variables.size()),
		variables(variables),
		reverse_variables(reverse_variables),
		clauses(clauses),
		unit_clauses(unit_clauses),
		/* XXX: This gives a way to seed each thread independently,
		 * but we should still derive the seeds from the kernel's
		 * "true" random number generator. */
		random(seed),
		decide(*this),
		propagate(variables.size(), clauses.size()),
		analyze(*this),
		reduce(*this),
		print(*this)
	{
	}

	~solver()
	{
	}

	bool is_learnt(clause c)
	{
		return c.index() >= clauses.size();
	}

	void attach(clause c)
	{
		propagate.attach(c);
		decide.attach(c);
		reduce.attach(*this, c);
		print.attach(*this, c);
	}

	void attach(clause c, watch_indices w)
	{
		propagate.attach(c, w);
		decide.attach(c);
		reduce.attach(*this, c);
		print.attach(*this, c);
	}

	void detach(clause c)
	{
		propagate.detach(c);
		decide.detach(c);
		reduce.detach(*this, c);
		print.detach(*this, c);
	}

	void decision(literal lit)
	{
		propagate.decision(lit);
		print.decision(*this, lit);
	}

	void backtrack(unsigned int decision)
	{
		propagate.backtrack(decision);
		print.backtrack(*this, decision);
	}

	void verify()
	{
		/* Verify that the solution is indeed a solution */
		for (unsigned int i = 0; i < clauses.size(); ++i) {
			clause c = clauses[i];

			bool v = false;
			for (unsigned int j = 0; j < c.size(); ++j) {
				assert(propagate.defined(c[j]));
				v = v || propagate.value(c[j]);
			}

			assert(v);
		}

		std::ostringstream ss;

		variable_map::const_iterator it = variables.begin();
		variable_map::const_iterator end = variables.end();

		if (it != end) {
			assert(propagate.defined(it->second));

			if (!propagate.value(it->second))
				ss << "-";

			ss << it->first;
		}

		while (++it != end) {
			assert(propagate.defined(it->second));

			ss << " ";

			if (!propagate.value(it->second))
				ss << "-";

			ss << it->first;
		}

		printf("v %s\n", ss.str().c_str());
		printf("c SATISFIABLE\n");
	}

	void unsat()
	{
		should_exit = true;
		printf("c UNSATISFIABLE\n");
	}

	void run()
	{
		printf("c Thread %u started\n", id);
		debug_thread_id = id;

		/* Attach all clauses in the original instance */
		for (clause c: clauses)
			attach(c);

		/* Queue unit clauses */
		for (literal l: unit_clauses) {
			if (!propagate.implication(l, clause()))
				unsat();
		}

		if (!should_exit && !propagate.propagate())
			unsat();

		while (!should_exit) {
			/* XXX: Receive learnt clauses from other threads */

			/* Search */

			/* If we have assigned values to all the variables
			 * without reaching a conflict, this means that the
			 * current assignment satisfies the instance. */
			if (propagate.trail_index == nr_variables) {
				if (propagate.decision_index == 0 || !keep_going) {
					should_exit = true;
					verify();
					break;
				}

				verify();

				/* Add the negated current set of decisions
				 * as a clause; this will prevent the solver
				 * from finding the same solution again. */
				std::vector<literal> conflict_clause;

				for (unsigned int i = 0; i < propagate.decision_index; ++i) {
					unsigned int variable = propagate.trail[propagate.decisions[i]];

					conflict_clause.push_back(literal(variable,
						!propagate.value(variable)));
				}

				backtrack(0);

				assert(!conflict_clause.empty());
				if (conflict_clause.size() == 1) {
					/* XXX: This can never fail, so we should not check
					 * whether it does in the hotpath. */
					bool ret = propagate.implication(conflict_clause[0], clause());
					assert(ret);

					if (!propagate.propagate()) {
						should_exit = true;
						break;
					}
				} else {
					unsigned int clause_id = clause_counter++;
					clause learnt_clause(clause_id, conflict_clause);

					while (propagate.watches.size() <= clause_id)
						propagate.watches.push_back(watch_indices());

					attach(learnt_clause);
				}

				continue;
			}

			propagate.decision(decide(*this, propagate));
			while (!propagate.propagate() && !should_exit) {
				if (restart()) {
					backtrack(0);
					reduce(*this);
					break;
				}

				analyze(*this, propagate);
			}

#if 0
			/* Let writers know that we're done with old copies
			 * of RCU-protected data. */
			rcu_quiescent_state();
#endif
		}

		printf("c Thread %u stopping\n", id);
	}
};

#endif
