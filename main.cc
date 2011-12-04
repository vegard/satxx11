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

#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <map>
#include <random>
#include <sstream>
#include <thread>

#include <boost/program_options.hpp>

extern "C" {
#include <signal.h>

#include <sys/sysinfo.h>
}

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

void read_cnf(std::istream &file,
	variable_map &variables, variable_map &reverse_variables,
	clause_vector &clauses,
	literal_vector &unit_clauses)
{
	while (!file.eof()) {
		std::string line;
		getline(file, line);

		if (line.size() == 0)
			continue;

		/* Skip problem line -- we don't use it anyway */
		if (line[0] == 'p')
			continue;

		/* Skip comments */
		if (line[0] == 'c')
			continue;

		/* XOR clauses */
		if (line[0] == 'x')
			throw std::runtime_error("Cannot read XOR clauses");

		std::vector<literal> c;

		std::stringstream s(line);
		while (!s.eof()) {
			int x;
			s >> x;

			if (x == 0)
				break;

			variable v = abs(x);

			/* We remap variables to the range [0, n - 1], where
			 * n is the total number of variables. */
			variable v2;
			variable_map::iterator it = variables.find(v);
			if (it == variables.end()) {
				v2 = variables.size();
				variables[v] = v2;
				reverse_variables[v2] = v;
			} else {
				v2 = it->second;
			}

			c.push_back(literal(v2, x > 0));
		}

		if (c.size() == 1)
			unit_clauses.push_back(c[0]);
		else
			clauses.push_back(clause(clauses.size(), c));
	}

	printf("c Variables: %lu\n", variables.size());
	printf("c Clauses: %lu\n", clauses.size());

	assert(variables.size() == reverse_variables.size());
}

static bool keep_going = false;

/* XXX: Go through their use points and add/remove the necessary memory
 * barriers */
static std::atomic<bool> should_exit;
static std::atomic<unsigned int> clause_counter;

static void handle_sigint(int signum, ::siginfo_t *info, void *unused)
{
	/* Second signal should kill us no matter what. */
	if (should_exit)
		abort();

	should_exit = true;
}

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
	unsigned int nr_variables;
	const variable_map &variables;
	const variable_map &reverse_variables;
	const clause_vector &clauses;
	const literal_vector &unit_clauses;
	std::atomic<unsigned int> *clause_counter;

	Random random;
	Decide decide;
	Propagate propagate;
	Analyze analyze;
	Restart restart;
	Reduce reduce;
	Print print;

	solver(unsigned int id,
		const variable_map &variables,
		const variable_map &reverse_variables,
		const clause_vector &clauses,
		const literal_vector &unit_clauses):
		id(id),
		nr_variables(variables.size()),
		variables(variables),
		reverse_variables(reverse_variables),
		clauses(clauses),
		unit_clauses(unit_clauses),
		clause_counter(&::clause_counter),
		/* XXX: This gives a way to seed each thread independently,
		 * but we should still derive the seeds from the kernel's
		 * "true" random number generator. */
		random(id),
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
					unsigned int clause_id = (*clause_counter)++;
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

template<typename t>
static void solve(t *s)
{
	s->run();
}

int main(int argc, char *argv[])
{
	/* Don't buffer stdout/stderr. This REALLY helps for debugging
	 * as it also makes sure that messages from the solver and e.g.
	 * valgrind appear in the right order. */
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	unsigned int nr_threads = 1;

	/* Get the number of "enabled CPUs" from the kernel */
	{
		int nprocs = get_nprocs();
		if (nprocs >= 1)
			nr_threads = nprocs;
	}

	std::vector<std::string> input_files;

	/* Process command line */
	{
		using namespace boost::program_options;

		options_description options("Options");
		options.add_options()
			("help", "Display this information")
			("keep-going", value<bool>(&keep_going)->zero_tokens(), "find all solutions")
			("threads", value<unsigned int>(&nr_threads), "number of threads")
			("input", value<std::vector<std::string> >(&input_files), "input file")
		;

		options_description debug_options("Debugging options");
		debug_options.add_options()
			("debug-sizes", "Dump the sizes of various data structures")
		;

		options_description all_options;
		all_options.add(options);
		all_options.add(debug_options);

		positional_options_description p;
		p.add("input", -1);

		variables_map map;
		store(command_line_parser(argc, argv)
			.options(all_options)
			.positional(p)
			.run(), map);
		notify(map);

		if (map.count("help")) {
			std::cout << all_options;
			return 0;
		}

		if (map.count("threads") > 1 || nr_threads < 1) {
			std::cerr << all_options;
			return 1;
		}

		if (map.count("debug-sizes")) {
			printf("c sizeof(clause) = %lu\n", sizeof(clause));
			printf("c sizeof(clause::impl) = %lu\n", sizeof(clause::impl));
			printf("c sizeof(literal) = %lu\n", sizeof(literal));
			printf("c sizeof(solver<>) = %lu\n", sizeof(solver<>));
			printf("c sizeof(watch_indices) = %lu\n", sizeof(watch_indices));
			printf("c sizeof(watchlist) = %lu\n", sizeof(watchlist));
			return 0;
		}
	}

	/* Read instance */
	variable_map variables;
	variable_map reverse_variables;
	clause_vector clauses;
	literal_vector unit_clauses;

	if (input_files.size() >= 1) {
		for (unsigned int i = 0; i < input_files.size(); ++i) {
			std::ifstream file;

			file.open(input_files[i].c_str());
			if (!file)
				throw std::runtime_error("Could not open file");

			printf("c Reading %s\n", input_files[i].c_str());
			read_cnf(file, variables, reverse_variables, clauses, unit_clauses);
			file.close();
		}
	} else {
		printf("c Reading standard input\n");
		read_cnf(std::cin, variables, reverse_variables, clauses, unit_clauses);
	}

	clause_counter = clauses.size();

	/* Catch Ctrl-C and stop the threads gracefully (NOTE: Do this after
	 * reading the instance, to allow the default handler to abort the
	 * program "ungracefully" while reading the instance). */
	struct ::sigaction sigint_act;
	sigint_act.sa_sigaction = &handle_sigint;
	sigemptyset(&sigint_act.sa_mask);
	sigint_act.sa_flags = SA_SIGINFO;
	sigint_act.sa_restorer = NULL;
	if (::sigaction(SIGINT, &sigint_act, NULL) == -1)
		throw system_error(errno);

	/* XXX: Make all the stuff below RAII (we currently don't clean up
	 * if anything bad happens = an exception is thrown). */

	/* Construct the solvers */
	solver<> *solvers[nr_threads];
	for (unsigned int i = 0; i < nr_threads; ++i)
		solvers[i] = new solver<>(i, variables, reverse_variables, clauses, unit_clauses);

	/* Start threads */
	std::thread *threads[nr_threads];
	for (unsigned int i = 0; i < nr_threads; ++i)
		threads[i] = new std::thread(solve<solver<>>, solvers[i]);

	/* Wait for the solvers to finish/exit */
	for (unsigned int i = 0; i < nr_threads; ++i)
		threads[i]->join();

	/* Free clauses */
	for (unsigned int i = 0; i < clauses.size(); ++i)
		clauses[i].free();

	return 0;
}
