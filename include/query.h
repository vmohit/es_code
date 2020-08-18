#ifndef QUERY_H
#define QUERY_H

#include "expression.h"

#include <tuple>
#include <string>
#include <vector>
#include <set>
#include <map>

class QueryTemplate {
	Expression expr;
	float weight;
	static int maxid;
	int id;  //!< unique id assigned to this template

	std::vector<std::vector<int>> goal_graph; //!< undirected graph where nodes are goals and two goals are connected if they share a variable
	void dfs_visit(int node, const std::set<int>& subset_goals, std::vector<int>& visited) const;

public:
	QueryTemplate(const std::string& expr, const std::map<std::string, const BaseRelation*>& name2br, float wt);
	int get_id() const;
	const Expression& get_expr() const;
	bool connected(const std::set<int>& subset_goals) const; //!< returns true if the subset of goals form a connected component
	float get_weight() const;
};

class Index {
	Expression expr;
	static int maxid;
	int id;
	float storage_cost = 0;

public:
	Index(const Expression& exp);
	Index(const std::string& expr, const std::map<std::string, const BaseRelation*>& name2br);

	/**Obtains a generalized version of this index and ind2. If this index and ind2 shares the base relations and join structure and only differ in
	selection conditions, then this function returns a new index and a containment mapping from this new index to the two indices*/
	//std::tuple<Index, std::map<int, Expression::Symbol>, std::map<int, Expression::Symbol>> merge(const Index& ind2) const;
	
	int get_id() const;
	const Expression& get_expr() const;
};

#endif