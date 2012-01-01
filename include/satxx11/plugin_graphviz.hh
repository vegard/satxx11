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

#ifndef SATXX11_PLUGIN_GRAPHVIZ_HH
#define SATXX11_PLUGIN_GRAPHVIZ_HH

#include <limits>
#include <stack>

#include <satxx11/clause.hh>
#include <satxx11/literal.hh>
#include <satxx11/plugin_base.hh>
#include <satxx11/system_error.hh>

namespace satxx11 {

template<bool show_implications = false>
class plugin_graphviz:
	public plugin_base
{
public:
	/* XXX: Use iostreams? */
	FILE *fp;
	unsigned int node;

	std::stack<std::stack<unsigned int>> nodes;

	plugin_graphviz():
		node(0)
	{
		nodes.push(std::stack<unsigned int>());
		nodes.top().push(node);
	}

	template<class Solver>
	void start(Solver &s)
	{
		fp = fopen(format("thread-$.dot", s.id).c_str(), "w");
		if (!fp)
			throw system_error(errno);

		fprintf(fp, "digraph g {\n");
		fprintf(fp, "\tnode [label=\"\",shape=point,height=0.2];\n");
		fprintf(fp, "\tedge [arrowsize=0.5];\n");
		fprintf(fp, "\tn%u;\n", nodes.top().top());
	}

	~plugin_graphviz()
	{
		fprintf(fp, "}\n");
		fclose(fp);
	}

	template<class Solver>
	void decision(Solver &s, literal lit)
	{
		++node;

		fprintf(fp, "\tn%u;\n", node);
		fprintf(fp, "\tn%u -> n%u [label=\"%s\"];\n", nodes.top().top(), node, lit.string().c_str());

		nodes.push(std::stack<unsigned int>());
		nodes.top().push(node);
	}

	template<class Solver>
	void implication(Solver &s, literal lit, typename Solver::reason_type reason)
	{
		if (!show_implications)
			return;

		++node;

		fprintf(fp, "\tn%u [colorscheme=blues3,color=3];\n", node);
		fprintf(fp, "\tn%u -> n%u [label=\"%s\"];\n", nodes.top().top(), node, lit.string().c_str());

		nodes.top().push(node);
	}

	template<class Solver>
	void conflict(Solver &s)
	{
		++node;

		/* XXX: Label the conflict edge with the conflicting literal. */
		fprintf(fp, "\tn%u [colorscheme=reds3,color=3];\n", node);
		fprintf(fp, "\tn%u -> n%u;\n", nodes.top().top(), node);
	}

	template<class Solver>
	void backtrack(Solver &s, unsigned int decision)
	{
		unsigned int old_top = nodes.top().top();

		while (nodes.size() > 1 + decision)
			nodes.pop();

		fprintf(fp, "\tn%u -> n%u [colorscheme=greys3,color=2];\n", old_top, nodes.top().top());
	}

	template<class Solver>
	void restart(Solver &s)
	{
		unsigned int old_top = nodes.top().top();

		while (nodes.size() > 1)
			nodes.pop();

		fprintf(fp, "\tn%u -> n%u [colorscheme=greys3,color=3];\n", old_top, nodes.top().top());
	}

	template<class Solver>
	void sat(Solver &s)
	{
		++node;

		fprintf(fp, "\tn%u [colorscheme=greens3,color=3];\n", node);
		fprintf(fp, "\tn%u -> n%u;\n", nodes.top().top(), node);
	}

	template<class Solver>
	void unsat(Solver &s)
	{
	}
};

}

#endif
