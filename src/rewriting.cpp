#include "rewriting.h"
#include "query.h"
#include "expression.h"

#include <string>
#include <set>
#include <cassert>

using std::set;
using std::string;
using std::to_string;

ViewTuple::ViewTuple(const QueryTemplate* qt, const Index* ind)
: query(qt), index(ind) {}

ViewTuple::ViewTuple(const QueryTemplate* qt, const Index* ind, 
	const std::set<int>& core, const std::map<int, int>& query2index)
	: query(qt), index(ind) {
	subcores.insert(core);
	for(auto it=query2index.begin(); it!=query2index.end(); it++) {
		assert(index2query.find(it->second)==index2query.end());
		index2query.insert({it->second, Expression::Symbol(it->first)});
	}
}

string ViewTuple::show() const {
	string result = "(query: "+to_string(query->get_id())+", index: "+to_string(index->get_id())+", cores: {";
	for(auto& subcore: subcores) {
		result += "{";
		for(auto gid: subcore)
			result += to_string(gid)+", ";
		result += "}, ";
	}
	result += "}) i2q:{";
	for(auto it=index2query.begin(); it!=index2query.end(); it++) {
		result += index->get_expr().get_var(it->first) + " -> " + 
			(it->second.isconstant ? it->second.dt.show() : query->get_expr().get_var(it->second.var)) + ", ";
	}
	result += "}\n";
	return result;
}