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

#ifndef THREAD_HH
#define THREAD_HH

#include "system_error.hh"

extern "C" {
#include <pthread.h>
#include <urcu-qsbr.h>
/* XXX: urcu/compiler.h workaround for C++ */
#undef max
#undef min
}

class thread {
private:
	static void *entry(void *);

public:
	pthread_t self;

	thread()
	{
	}

	virtual ~thread()
	{
	}

	void start()
	{
		int err = pthread_create(&self, NULL, &entry, (void *) this);
		if (err != 0)
			throw system_error(err);
	}

	void join()
	{
		/* If this returns an error, something is way wrong and
		 * there is _nothing_ we can do to fix it. We could ignore
		 * it, but that would be wrong too. */
		int err = pthread_join(self, NULL);
		if (err != 0)
			throw system_error(err);
	}

	virtual void run() = 0;
};

void *thread::entry(void *arg)
{
	rcu_register_thread();

	thread *t = (thread *) arg;
	t->run();

	rcu_unregister_thread();

	return NULL;
}

#endif
