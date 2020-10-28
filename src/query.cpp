#include "query.h"
#include "data.h"
#include "expression.h"

#include <map>
#include <string>
#include <set>
#include <vector>
#include <list>

using std::map;
using std::vector;
using std::set;
using std::string;
using std::to_string;
using std::list;

Query::Query(const Expression& exp_arg)
: exp(exp_arg) {
	map<const BaseRelation*, BaseRelation::Table*> br2tab;
	map<int, Data> var2const;
	int maxconst=0;
	set<Data> consts;
	for(int gid=0; gid<exp.num_goals(); gid++) 
		for(uint i=0; i<exp.goal_at(gid).symbols.size(); i++) 
			if(exp.goal_at(gid).symbols.at(i).isconstant)
				consts.insert(exp.goal_at(gid).symbols.at(i).dt);

	for(int gid=0; gid<exp.num_goals(); gid++) {
		auto goal = exp.goal_at(gid);
		if(br2tab.find(goal.br)==br2tab.end()) {
			tables.push_back(BaseRelation::Table(goal.br, goal.br->get_name()));
			br2tab[goal.br] = &tables.back();
		}
		auto table = br2tab.at(goal.br);
		vector<Data> row;
		for(uint i=0; i<goal.symbols.size(); i++) {
			auto symbol = goal.symbols.at(i);
			if(symbol.isconstant)
				row.push_back(symbol.dt);
			else {
				if(var2const.find(symbol.var)==var2const.end()) {
					if(goal.br->dtype_at(i)==Dtype::Int) {
						while(consts.find(Data(maxconst++))!=consts.end()) {}
						var2const.emplace(symbol.var, Data(maxconst-1));
					}
					else {
						while(consts.find(Data(to_string(maxconst++)))!=consts.end()) {}
						var2const.emplace(symbol.var, Data(to_string(maxconst-1)));
					}
				}
				row.push_back(var2const.at(symbol.var));
			}
		}
		table->df.add_tuple(row);
	}
	for(auto item: br2tab)
		br2table[item.first] = item.second;
	for(auto item: var2const)
		const2var[item.second] = item.first;
}

const Expression& Query::expression() const {
	return exp;
}

list<ViewTuple> Query::get_view_tuples(const Index& index) const {
	list<ViewTuple> result;
	Expression::Table vts(&index.expression(), br2table);
	for(auto row: vts.df.get_rows()) {
		map<int, Expression::Symbol> index2query;
		for(auto headvar: index.expression().head_vars()) {
			Data dt = row[vts.df.get_cid2pos(vts.headvar2cid.at(headvar))];
			if(const2var.find(dt)==const2var.end())
				index2query.emplace(headvar, Expression::Symbol(dt));
			else
				index2query.emplace(headvar, Expression::Symbol(const2var.at(dt)));
		}
		result.push_back(ViewTuple(*this, index, index2query));
	}
	return result;
}

Index::Index(const Expression& exp_arg)
: exp(exp_arg) {}

const Expression& Index::expression() const {
	return exp;
}

ViewTuple::ViewTuple(const Query& query_arg,
		const Index& index_arg,
		map<int, Expression::Symbol> index2query_arg) 
: query(query_arg), index(index_arg),
index2query(index2query_arg) {}

string ViewTuple::show() const {
	string result=index.expression().get_name();
	result += "(";
	uint i=0;
	for(auto headvar: index.expression().head_vars()) {
		result += index.expression().var_to_name(headvar)+": ";
		auto symbol = index2query.at(headvar);
		if(symbol.isconstant)
			result += symbol.dt.show();
		else
			result += query.expression().var_to_name(symbol.var);
		i++;
		if(i!=index.expression().head_vars().size())
			result += ", ";
	}
	result += ")";
	return result;
}