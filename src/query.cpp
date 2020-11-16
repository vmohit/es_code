#include "query.h"
#include "data.h"
#include "expression.h"

#include <map>
#include <string>
#include <set>
#include <vector>
#include <list>
#include <queue>
#include <iostream>

using std::map;
using std::vector;
using std::set;
using std::string;
using std::to_string;
using std::list;
using std::queue;
using std::cout;
using std::endl;

Query::Query(const Expression& exp_arg)
: exp(exp_arg) {
	assert(!exp.empty());
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
	for(auto row: vts.df.get_unique_rows()) {
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
: exp(exp_arg) {
	assert(!exp.empty());
}

const Expression& Index::expression() const {
	return exp;
}


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
	result += ") -> subcores: {";
	uint j=0;
	for(auto subcore: subcores) {
		result += "{";
		uint i=0;
		for(uint gid: subcore)
			result += to_string(gid)+(++i == subcore.size() ? "}":", ");
		j++;
		if(j!=subcores.size())
			result+=", ";
	}
	result += "}";
	return result;
}



ViewTuple::ViewTuple(const Query& query_arg,
		const Index& index_arg,
		map<int, Expression::Symbol> index2query_arg) 
: query(query_arg), index(index_arg),
index2query(index2query_arg) {
	set<int> unexplored_goals;
	for(int gid=0; gid<query.expression().num_goals(); gid++)
		unexplored_goals.insert(gid);
	for(int gid=0; gid<query.expression().num_goals(); gid++) {
		if(unexplored_goals.find(gid)!=unexplored_goals.end()) {
			set<int> subcore;
			set<int> unmapped_goals;
			unmapped_goals.insert(gid);
			map<int, int> mu;
			bool match=false;
			try_match(match, subcore, unmapped_goals, mu);
			if(match) 
				subcores.insert(subcore);
			for(auto id: subcore)
				unexplored_goals.erase(id);
		}
	}
}



void ViewTuple::try_match(bool& match, set<int>& subcore,
	set<int>& unmapped_goals, map<int, int>& mu) {
	if(unmapped_goals.empty()) {
		match = true;
		return;
	}
	int q_gid = *(unmapped_goals.begin());
	unmapped_goals.erase(q_gid);
	subcore.insert(q_gid);
	for(int i_gid=0; i_gid<index.expression().num_goals(); i_gid++) {
		if(query.expression().goal_at(q_gid).br
			==index.expression().goal_at(i_gid).br) {
			set<int> newkeys;
			set<int> newgoals;
			const auto& q_symbols = query.expression().goal_at(q_gid).symbols;
			const auto& i_symbols = index.expression().goal_at(i_gid).symbols;
			bool flag=true;
			for(uint i=0; i<q_symbols.size(); i++) {
				if(q_symbols.at(i).isconstant) {
					if(i_symbols.at(i)!=q_symbols.at(i) &&
						(index2query.find(i_symbols.at(i).var)==index2query.end() ? 
							true: index2query.at(i_symbols.at(i).var)!=q_symbols.at(i))) {
						flag = false;
						break;
					}
				}
				else {
					if(query.expression().head_vars().find(q_symbols.at(i).var) 
						!= query.expression().head_vars().end())  {
						if(index2query.find(i_symbols.at(i).var)!=index2query.end() ?
							index2query.at(i_symbols.at(i).var)!=q_symbols.at(i) : true) {
								flag = false;
								break;
						}
					}
					else {
						if(mu.find(q_symbols.at(i).var)==mu.end()) {
							newkeys.insert(q_symbols.at(i).var);
							mu[q_symbols.at(i).var] = i_symbols.at(i).var;
						}
						if(mu.at(q_symbols.at(i).var)!=i_symbols.at(i).var) {
							flag=false;
							break;
						}
						if(index.expression().head_vars().find(i_symbols.at(i).var) 
							== index.expression().head_vars().end())
							for(int new_q_gid: query.expression().goals_containing(q_symbols.at(i).var))
								if(subcore.find(new_q_gid)==subcore.end() 
									&& unmapped_goals.find(new_q_gid)==unmapped_goals.end())
									newgoals.insert(new_q_gid);
					}
				}
			}
			if(flag) {
				for(int goal: newgoals)
					unmapped_goals.insert(goal);
				try_match(match, subcore, unmapped_goals, mu);
				if(match) return;
				for(int goal: newgoals)
					unmapped_goals.erase(goal);
			}
			for(int key: newkeys)
				mu.erase(key);
		}
	}
}