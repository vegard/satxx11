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

#include <boost/program_options.hpp>

extern "C" {
#include <signal.h>

#include <sys/sysinfo.h>
}

extern "C" {
#include <urcu-qsbr.h>
/* XXX: urcu/compiler.h workaround for C++ */
#undef max
#undef min
}

#include "analyze_1uip.hh"
#include "clause.hh"
#include "debug.hh"
#include "decide_random.hh"
#include "literal.hh"
#include "propagate_watchlists.hh"
#include "restart_fixed.hh"
#include "restart_geometric.hh"
#include "restart_luby.hh"
#include "restart_nested.hh"
#include "thread.hh"

typedef unsigned int variable;
typedef std::map<variable, variable> variable_map;
typedef std::vector<clause> clause_vector;

void read_cnf(std::istream &file,
	variable_map &variables, variable_map &reverse_variables,
	clause_vector &clauses)
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

		clauses.push_back(clause(clauses.size(), c));
	}

	printf("c Variables: %lu\n", variables.size());
	printf("c Clauses: %lu\n", clauses.size());

	assert(variables.size() == reverse_variables.size());
}


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

template<class Random = std::mt19937,
	class Decide = decide_random,
	class Propagate = propagate_watchlists,
	class Analyze = analyze_1uip,
	class Restart = restart_nested<restart_geometric<2, 10>, restart_geometric<100, 10>>>
class solver_thread:
	public thread
{
public:
	Random random;
	Decide decide;
	Propagate propagate;
	Analyze analyze;
	Restart restart;

	unsigned int id;
	unsigned int nr_variables;
	const variable_map &variables;
	const variable_map &reverse_variables;
	const clause_vector &clauses;
	std::atomic<unsigned int> *clause_counter;

	solver_thread(unsigned int id,
		const variable_map &variables,
		const variable_map &reverse_variables,
		const clause_vector &clauses):
		/* XXX: This gives a way to seed each thread independently,
		 * but we should still derive the seeds from the kernel's
		 * "true" random number generator. */
		random(id),
		decide(*this),
		propagate(variables.size(), clauses),
		analyze(*this),
		id(id),
		nr_variables(variables.size()),
		variables(variables),
		reverse_variables(reverse_variables),
		clauses(clauses),
		clause_counter(&::clause_counter)
	{
	}

	~solver_thread()
	{
	}

	void attach(clause c)
	{
		propagate.attach(c);
		decide.attach(c);
	}

	void attach(clause c, watch_indices w)
	{
		propagate.attach(c, w);
		decide.attach(c);
	}

	void detach(clause c)
	{
		propagate.detach(c);
		decide.detach(c);
	}

	void decision(literal lit)
	{
		propagate.decision(lit);
	}

	void backtrack(unsigned int decision)
	{
		propagate.backtrack(decision);
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

	void run()
	{
		printf("c Thread %u started\n", id);
		debug_thread_id = id;

		while (!should_exit) {
			/* XXX: Receive learnt clauses from other threads */

			/* Search */

			/* If we have assigned values to all the variables
			 * without reaching a conflict, this means that the
			 * current assignment satisfies the instance. */
			if (propagate.trail_index == nr_variables) {
				should_exit = true;
				verify();
			}

			if (restart())
				backtrack(0);

			propagate.decision(decide(*this, propagate));
			while (!propagate.propagate() && !should_exit)
				analyze(*this, propagate);

			/* Let writers know that we're done with old copies
			 * of RCU-protected data. */
			rcu_quiescent_state();
		}

		printf("c Thread %u stopping\n", id);
	}
};

int main(int argc, char *argv[])
{
#if CONFIG_DEBUG == 1
	/* Don't buffer stdout/stderr. This REALLY helps for debugging
	 * as it also makes sure that messages from the solver and e.g.
	 * valgrind appear in the right order. */
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
#endif

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
			printf("c sizeof(solver_thread) = %lu\n", sizeof(solver_thread<>));
			printf("c sizeof(watch_indices) = %lu\n", sizeof(watch_indices));
			printf("c sizeof(watchlist) = %lu\n", sizeof(watchlist));
			return 0;
		}
	}

	/* Read instance */
	variable_map variables;
	variable_map reverse_variables;
	clause_vector clauses;

	if (input_files.size() >= 1) {
		for (unsigned int i = 0; i < input_files.size(); ++i) {
			std::ifstream file;

			file.open(input_files[i].c_str());
			if (!file)
				throw std::runtime_error("Could not open file");

			printf("c Reading %s\n", input_files[i].c_str());
			read_cnf(file, variables, reverse_variables, clauses);
			file.close();
		}
	} else {
		printf("c Reading standard input\n");
		read_cnf(std::cin, variables, reverse_variables, clauses);
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

	/* Construct and start the solvers */
	solver_thread<> *threads[nr_threads];
	for (unsigned int i = 0; i < nr_threads; ++i) {
		threads[i] = new solver_thread<>(i, variables, reverse_variables, clauses);
		threads[i]->start();
	}

	/* Wait for the solvers to finish/exit */
	for (unsigned int i = 0; i < nr_threads; ++i) {
		threads[i]->join();
		delete threads[i];
	}

	/* Free clauses */
	for (unsigned int i = 0; i < clauses.size(); ++i)
		clauses[i].free();

	return 0;
}
