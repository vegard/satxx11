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

#ifndef SATXX11_SOLVER_HH
#define SATXX11_SOLVER_HH

#include <atomic>
#include <random>

#include <satxx11/analyze_1uip.hh>
#include <satxx11/clause.hh>
#include <satxx11/clause_allocator.hh>
#include <satxx11/debug.hh>
#include <satxx11/decide_cached_polarity.hh>
#include <satxx11/decide_random.hh>
#include <satxx11/decide_vsids.hh>
#include <satxx11/literal.hh>
#include <satxx11/minimise_minisat.hh>
#include <satxx11/minimise_noop.hh>
#include <satxx11/plugin_graphviz.hh>
#include <satxx11/plugin_list.hh>
#include <satxx11/plugin_stdio.hh>
#include <satxx11/propagate_watchlists.hh>
#include <satxx11/receive_all.hh>
#include <satxx11/reduce_noop.hh>
#include <satxx11/reduce_size.hh>
#include <satxx11/restart_fixed.hh>
#include <satxx11/restart_geometric.hh>
#include <satxx11/restart_luby.hh>
#include <satxx11/restart_multiply.hh>
#include <satxx11/send_size.hh>
#include <satxx11/simplify_list.hh>
#include <satxx11/simplify_failed_literal_probing.hh>

/* Workaround for missing implementation in libstdc++ for gcc 4.6. */
namespace std {

void atomic_thread_fence(memory_order __m)
{
	switch (__m) {
	case memory_order_relaxed:
		break;
	case memory_order_acquire:
		asm volatile ("lfence" : : : "memory");
		break;
	case memory_order_release:
		asm volatile ("sfence" : : : "memory");
		break;
	case memory_order_acq_rel:
		asm volatile ("mfence" : : : "memory");
		break;
	default:
		assert(false);
	}
}

}

namespace satxx11 {

typedef unsigned int variable;
typedef std::map<variable, variable> variable_map;
typedef std::vector<literal> literal_vector;
typedef std::vector<literal_vector> literal_vector_vector;

template<class ReasonType,
	class Random = std::ranlux24_base,
	class Decide = decide_cached_polarity<decide_vsids<95>>,
	class Propagate = propagate_watchlists<>,
	class Analyze = analyze_1uip<minimise_minisat>,
	class Send = send_size<4>,
	class Receive = receive_all,
	class NextRestart = restart_multiply<restart_luby, restart_fixed<400>>,
	class Reduce = reduce_size<2>,
	class Simplify = simplify_list<simplify_failed_literal_probing>,
	class Plugin = plugin_list<plugin_stdio>>
class solver {
public:
	typedef ReasonType reason_type;

	unsigned int nr_threads;
	solver **solvers;
	unsigned int id;
	bool keep_going;
	std::atomic<bool> &should_exit;
	unsigned int nr_variables;
	const variable_map &variables;
	const variable_map &reverse_variables;
	const literal_vector_vector &original_clauses;

	std::vector<bool> valuation;

	/* Indexed by variable. Gives the reason why a variable was set
	 * if the variable was implied. */
	reason_type *reasons;

	literal conflict_literal;
	reason_type conflict_reason;

	clause_allocator allocate;

	/* A message to/from other threads */
	struct message {
		bool empty;
		std::vector<literal> learnt_literals;
		std::vector<clause> learnt_clauses;
		std::vector<unsigned int> detached_clauses;

		message():
			empty(true)
		{
		}
	};

	/* Outgoing messages to other threads; indexed by thread id */
	message **output;

	std::atomic<message *> channel;

	/* Clauses which have been received from other threads but which
	 * are not yet attached (because we had to wait for a restart to
	 * attach them). XXX: This might go away if we figure out a nice
	 * way to attach clauses on partial backtracks. */
	std::vector<literal> pending_literals;
	std::vector<clause> pending_clauses;

	Random random;
	Decide decide;
	Propagate propagate;
	Analyze analyze;
	Send send;
	Receive receive;
	NextRestart next_restart;
	Reduce reduce;
	Simplify simplify;
	Plugin plugin;

	solver(unsigned int nr_threads,
		solver **solvers,
		unsigned int id,
		bool keep_going,
		std::atomic<bool> &should_exit,
		unsigned long seed,
		const variable_map &variables,
		const variable_map &reverse_variables,
		const literal_vector_vector &original_clauses):

		nr_threads(nr_threads),
		solvers(solvers),
		id(id),
		keep_going(keep_going),
		should_exit(should_exit),
		nr_variables(variables.size()),
		variables(variables),
		reverse_variables(reverse_variables),
		original_clauses(original_clauses),

		valuation(2 * nr_variables),
		reasons(new reason_type[nr_variables]),

		/* XXX: Not RAII. */
		output(new message *[nr_threads]),
		channel(0),

		random(seed),
		decide(*this),
		propagate(*this),
		analyze(*this),
		send(*this),
		receive(*this),
		next_restart(*this),
		reduce(*this)
	{
		for (unsigned int i = 0; i < nr_threads; ++i)
			output[i] = new message();

		/* XXX: Necessary? */
		for (unsigned int i = 0; i < nr_variables; ++i)
			reasons[i] = reason_type();
	}

	~solver()
	{
		for (unsigned int i = 0; i < nr_threads; ++i)
			delete output[i];

		delete[] output;
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

		//propagate.assign(variable, value);
		decide.assign(*this, variable, value);
		plugin.assign(*this, variable, value);
	}

	void assign(literal l, bool value)
	{
		assign(l.variable(), l.value() == value);
	}

	void unassign(unsigned int variable)
	{
		debug_enter("variable = $", variable);

		assert_hotpath(variable < nr_variables);
		assert_hotpath(defined(variable));
		valuation[2 * variable + 0] = false;

		decide.unassign(*this, variable);
		plugin.unassign(*this, variable);
	}

	void attach(literal l)
	{
		plugin.attach(*this, l);
	}

	bool attach(clause c) __attribute__ ((warn_unused_result))
	{
		if (!propagate.attach(*this, c))
			return false;
		if (!propagate.propagate(*this))
			return false;

		decide.attach(c);
		send.attach(*this, c);
		receive.attach(*this, c);
		reduce.attach(*this, c);
		plugin.attach(*this, c);
		return true;
	}

	void attach_with_watches(clause c, unsigned int i, unsigned int j)
	{
		propagate.attach_with_watches(c, i, j);
		decide.attach(c);
		send.attach(*this, c);
		receive.attach(*this, c);
		reduce.attach(*this, c);
		plugin.attach(*this, c);
	}

	void detach(clause c)
	{
		propagate.detach(c);
		decide.detach(c);
		send.detach(*this, c);
		receive.detach(*this, c);
		reduce.detach(*this, c);
		plugin.detach(*this, c);

		/* After this, we are not allowed to keep any references to the
		 * clause. So signal to the owning thread that we no longer hold
		 * a reference to it. */
		unsigned int thread = c.thread();
		unsigned int index = c.index();

		if (thread == id) {
			/* We _are_ the owning thread. */
			allocate.free(index);
		} else {
			output[thread]->detached_clauses.push_back(index);
			output[thread]->empty = false;
		}
	}

	void share(literal l)
	{
		if (!send(*this, l))
			return;

		for (unsigned int i = 0; i < nr_threads; ++i) {
			if (i == id)
				continue;

			output[i]->learnt_literals.push_back(l);
			output[i]->empty = false;
		}
	}

	void share(clause c)
	{
		/* Always share non-learnt (i.e. non-removable) clauses. */
		if (c.is_learnt()) {
			if (!send(*this, c))
				return;
		}

		for (unsigned int i = 0; i < nr_threads; ++i) {
			if (i == id)
				continue;

			output[i]->learnt_clauses.push_back(c);
			output[i]->empty = false;
		}
	}

	void decision(literal lit)
	{
		reasons[lit.variable()] = reason_type();
		propagate.decision(*this, lit);
		plugin.decision(*this, lit);
	}

	bool implication(literal lit, reason_type reason = reason_type())
	{
		if (defined(lit)) {
			/* If it's already defined to have the right value,
			 * we don't have a conflict. */
			if (value(lit))
				return true;

			conflict_literal = lit;
			conflict_reason = reason;
			return false;
		}

		reasons[lit.variable()] = reason;
		plugin.implication(*this, lit, reason);
		propagate.implication(*this, lit);
		return true;
	}

	/* XXX: The name is a bit misleading... */
	void resolve(literal l)
	{
		decide.resolve(*this, l);
	}

	/* XXX: We would also like to know what we are resolving _with_... */
	void resolve(const std::vector<literal> &v)
	{
		decide.resolve(*this, v);
		reduce.resolve(*this, v);
	}

	void conflict()
	{
		decide.conflict(*this);
		plugin.conflict(*this);
	}

	void backtrack(unsigned int decision)
	{
		propagate.backtrack(*this, decision);
		plugin.backtrack(*this, decision);
	}

	bool is_redundant(clause c)
	{
		/* XXX: Implement a mechanism for signalling the other
		 * threads that the clause is redundant (and if so, why),
		 * so that the original thread can benefit from our
		 * discovery. */

		unsigned int nr_decisions = 0;

		for (unsigned int i = 1, n = c.size(); i < n; ++i) {
			unsigned int var = c[i].variable();

			if (defined(var)) {
				/* The clause is already satisfied! */
				if (value(var) == c[i].value())
					return true;

				/* The literal is already falsified. This means
				 * we could simplify the clause, though. */
				continue;
			}

			++nr_decisions;
			propagate.decision(*this, ~c[i]);
			if (!propagate.propagate(*this)) {
				/* The clause is subsumed by knowledge that
				 * we already have (i.e. it is implied by the
				 * clause database); don't attach it. */
				propagate.backtrack(*this, 0);
				return true;
			}
		}

		literal lit = c[0];

		if (defined(lit)) {
			if (value(lit) == true) {
				/* The last literal is implied by the clause
				 * database; don't attach it. */
				if (nr_decisions)
					propagate.backtrack(*this, 0);
				return true;
			} else {
				/* The negated literal was implied. This
				 * means that we can shorten the clause. */
				/* XXX: Actually shorten it. */
				if (nr_decisions)
					propagate.backtrack(*this, 0);
				return false;
			}
		}

		/* As far as we could tell, the clause is not redundant. */
		if (nr_decisions)
			propagate.backtrack(*this, 0);
		return false;
	}

	/* Returns false if and only if we detected unsat. This may happen
	 * because we received some literals/clauses from other threads. */
	bool restart()
	{
		propagate.backtrack(*this, 0);
		/* XXX: For the time being, this is a small hack to prevent
		 * the stdout plugin from seeing backtrack(0) in every restart,
		 * and always printing 0 as the minimum backtrack level. */
		plugin.restart(*this);
		simplify(*this);

		/* XXX: This might change if/when we figure out that we
		 * want to and can attach clauses at other times than a
		 * full restart. */
		for (literal l: pending_literals) {
			if (!implication(l))
				return false;

			attach(l);
		}

		pending_literals.clear();

		if (!propagate.propagate(*this))
			return false;

		for (clause c: pending_clauses) {
			if (is_redundant(c)) {
				unsigned int thread = c.thread();
				output[thread]->detached_clauses.push_back(c.index());
				output[thread]->empty = false;
				continue;
			}

			if (!attach(c))
				return false;
		}

		pending_clauses.clear();

		if (!propagate.propagate(*this))
			return false;

		return true;
	}

	void sat()
	{
		plugin.sat(*this);

		/* Verify that the solution is indeed a solution */
		for (literal_vector c: original_clauses) {
			bool v = false;
			for (literal lit: c) {
				assert(defined(lit));
				v = v || value(lit);
			}

			assert(v);
		}

		std::ostringstream ss;

		variable_map::const_iterator it = variables.begin();
		variable_map::const_iterator end = variables.end();

		if (it != end) {
			assert(defined(it->second));

			if (!value(it->second))
				ss << "-";

			ss << it->first;
		}

		while (++it != end) {
			assert(defined(it->second));

			ss << " ";

			if (!value(it->second))
				ss << "-";

			ss << it->first;
		}

		printf("v %s 0\n", ss.str().c_str());
		printf("s SATISFIABLE\n");
	}

	void unsat()
	{
		plugin.unsat(*this);

		should_exit = true;
		printf("s UNSATISFIABLE\n");
	}

	void run(const std::vector<literal> &literals, const std::vector<clause> &clauses)
	{
		debug_thread_id = id;

		plugin.start(*this);

		/* XXX: We currently attach clauses before propagating unit
		 * literals because we might otherwise try to attach a clause
		 * that would immediately propagate a value (and we don't
		 * handle this in attach()). */

		/* Attach all clauses in the original instance */
		for (clause c: clauses) {
			if (!attach(c)) {
				unsat();
				break;
			}
		}

		if (should_exit)
			return;

		/* Queue unit clauses */
		for (literal l: literals) {
			if (!implication(l)) {
				unsat();
				break;
			}
		}

		if (should_exit)
			return;

		if (!propagate.propagate(*this))
			unsat();

		/* Simplify the instance before doing anything else. */
		/* XXX: Maybe this should really be a restart? We need to signal
		 * the other plugins too in any case. */
		simplify(*this);

		unsigned int nr_conflicts = next_restart(*this);
		assert(nr_conflicts > 0);

		while (!should_exit) {
			/* This orders the writes to our outgoing messages with the atomic
			 * compare and exchange below. This barrier is paired with the
			 * read barrier in incoming message reading code of the recipient
			 * threads. */
			std::atomic_thread_fence(std::memory_order_release);

			/* Try to send outgoing messages */
			for (unsigned int i = 0; i < nr_threads; ++i) {
				if (i == id)
					continue;

				/* No messages to write. */
				if (output[i]->empty)
					continue;

				message *expected = 0;
				if (solvers[i]->channel.compare_exchange_strong(expected, output[i], std::memory_order_relaxed)) {
					/* Sending was successful. */
					output[i] = new message();
				} else {
					/* Sending failed. In this case, we just leave the message
					 * as it is, and hope that it will succeed some time in the
					 * future. */
				}
			}

			/* Read incoming messages */
			message *m = channel.exchange(0, std::memory_order_relaxed);
			if (m) {
				/* Process message */

				for (literal l: m->learnt_literals) {
					if (!receive(*this, l))
						continue;

					pending_literals.push_back(l);
				}

				for (clause c: m->learnt_clauses) {
					if (!receive(*this, c)) {
						/* Reject the clause by not attaching it in
						 * the first place, but letting the owning
						 * thread know that we don't want to use it. */
						unsigned int thread = c.thread();

						assert_hotpath(thread != id);
						output[thread]->detached_clauses.push_back(c.index());
						output[thread]->empty = false;
						continue;
					}

					pending_clauses.push_back(c);
				}

				/* Decrement reference counts for detached clauses */
				for (unsigned int id: m->detached_clauses)
					allocate.free(id);

				delete m;
			}

			/* This orders our reads from incoming messages with the atomic
			 * exchange above. This barrier is paired with the write barrier
			 * in outgoing message writing code of the sender threads. */
			std::atomic_thread_fence(std::memory_order_acquire);

			/* Search */

			/* If we have assigned values to all the variables
			 * without reaching a conflict, this means that the
			 * current assignment satisfies the instance. */
			if (propagate.trail_index == nr_variables) {
				if (propagate.decision_index == 0 || !keep_going) {
					should_exit = true;
					sat();
					break;
				}

				sat();

				/* Add the negated current set of decisions
				 * as a clause; this will prevent the solver
				 * from finding the same solution again. */
				std::vector<literal> conflict_clause;

				for (unsigned int i = 0; i < propagate.decision_index; ++i) {
					unsigned int variable = propagate.trail[propagate.decisions[i]];

					conflict_clause.push_back(literal(variable, !value(variable)));
				}

				backtrack(0);

				assert(!conflict_clause.empty());
				if (conflict_clause.size() == 1) {
					/* XXX: This can never fail, so we should not check
					 * whether it does in the hotpath. */
					bool ret = implication(conflict_clause[0]);
					assert(ret);

					if (!propagate.propagate(*this)) {
						should_exit = true;
						break;
					}

					share(conflict_clause[0]);
				} else {
					clause learnt_clause = allocate.allocate(nr_threads, id, false, conflict_clause);
					if (!attach(learnt_clause)) {
						should_exit = true;
						break;
					}

					share(learnt_clause);
				}

				continue;
			}

			decision(decide(*this));
			while (!propagate.propagate(*this) && !should_exit) {
				conflict();

				if (propagate.decision_index == 0) {
					/* A conflict at decision level 0 means the instance
					 * is unsat. */
					unsat();
					/* XXX: This breaks out of the inner loop, but we know
					 * enough to break out of the outer loop too. Maybe a
					 * a goto? */
					break;
				}

				if (--nr_conflicts == 0) {
					/* Restart */
					nr_conflicts = next_restart(*this);
					assert(nr_conflicts > 0);

					if (!restart()) {
						unsat();
						break;
					}

					reduce(*this);
					break;
				}

				analyze(*this);
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

}

#endif
