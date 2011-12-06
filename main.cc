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
#include <sstream>
#include <thread>

#include <boost/program_options.hpp>

extern "C" {
#include <signal.h>

#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
}

#include "clause.hh"
#include "debug.hh"
#include "literal.hh"
#include "solver.hh"

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
			clauses.push_back(clause(0, clauses.size(), c));
	}

	printf("c Variables: %lu\n", variables.size());
	printf("c Clauses: %lu\n", clauses.size());

	assert(variables.size() == reverse_variables.size());
}

static bool keep_going = false;

/* XXX: Go through their use points and add/remove the necessary memory
 * barriers */
static std::atomic<bool> should_exit;

static void handle_sigint(int signum, ::siginfo_t *info, void *unused)
{
	/* Second signal should kill us no matter what. */
	if (should_exit)
		abort();

	should_exit = true;
}

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

	unsigned long seed;
	{
		struct timeval tv;
		int err = gettimeofday(&tv, NULL);
		assert(!err);

		seed = tv.tv_sec * 1000000 + tv.tv_usec;
	}

	/* Construct the solvers */
	solver<> *solvers[nr_threads];
	for (unsigned int i = 0; i < nr_threads; ++i)
		solvers[i] = new solver<>(nr_threads, i, keep_going, should_exit, seed + i, variables, reverse_variables, clauses, unit_clauses);

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

	{
		struct rusage usage;
		int err = getrusage(RUSAGE_SELF, &usage);
		assert(err == 0);

		printf("c CPU user time %lu.%06lu\n", usage.ru_utime.tv_sec, usage.ru_utime.tv_usec);
	}

	return 0;
}
