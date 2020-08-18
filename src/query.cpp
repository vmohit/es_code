#include "query.h"
#include "expression.h"

#include <string>
#include <map>
#include <iostream>
#include <set>
#include <vector>
#include <tuple>

using std::vector;
using std::string;
using std::map;
using std::set;
using std::cout;
using std::endl;
using std::tuple;

int QueryTemplate::maxid = 0;
int Index::maxid = 0;

QueryTemplate::QueryTemplate(const string& exp, const map<string, const BaseRelation*>& name2br, 
	float wt) : expr(exp, name2br), weight(wt) {
	assert(wt>=0 && wt<=1);
	id = maxid++;
	goal_graph.resize(expr.num_goals());
	map<uint, set<uint>> var2goal;
	for(uint gid=0; gid<expr.num_goals(); gid++) {
		for(auto symbol: expr.goal_at(gid).symbols) 
			if(!symbol.isconstant && var2goal.find(symbol.var)!=var2goal.end()) 
				for(uint gid2: var2goal.at(symbol.var)) {
					goal_graph[gid].push_back(gid2);
					goal_graph[gid2].push_back(gid);
				}
		for(auto symbol: expr.goal_at(gid).symbols)
			if(!symbol.isconstant) {
				if(var2goal.find(symbol.var)==var2goal.end())
					var2goal[symbol.var] = set<uint>{gid};
				else
					var2goal[symbol.var].insert(gid);
			}
	}
}


bool QueryTemplate::connected(const set<int>& subset_goals) const {
	if(subset_goals.size()<=1)
		return true;
	vector<int> visited(goal_graph.size(), 0);
	dfs_visit(*subset_goals.begin(), subset_goals, visited);
	for(uint gid=0; gid<visited.size(); gid++)
		if(visited[gid]==0 && subset_goals.find(gid)!=subset_goals.end())
			return false;
	return true;
}

void QueryTemplate::dfs_visit(int node, const set<int>& subset_goals, vector<int>& visited) const {
	visited[node] = 1;
	for(uint gid: goal_graph[node])
		if(subset_goals.find(gid)!=subset_goals.end() && visited[gid]==0)
			dfs_visit(gid, subset_goals, visited);
	visited[node] = 2;
}

const Expression& QueryTemplate::get_expr() const {
	return expr;
}

int QueryTemplate::get_id() const {
	return id;
}

float QueryTemplate::get_weight() const {
	return weight;
}

/////////////////////////////////////////////
/////////////////////////////////////////////
/////////////////////////////////////////////
/////////////////////////////////////////////
/////////////////////////////////////////////
/////////////////////////////////////////////
/////////////////////////////////////////////


Index::Index(const Expression& exp) : expr(exp), id(maxid++) {}

Index::Index(const string& exp_str, const map<string, const BaseRelation*>& name2br) 
: expr(exp_str, name2br), id(maxid++) {}

int Index::get_id() const {
	return id;
}

const Expression& Index::get_expr() const {
	return expr;
}

// tuple<Index, map<int, Expression::Symbol>, map<int, Expression::Symbol>> Index::merge(const Index& ind2) const {
	
// }