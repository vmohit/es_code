#include "expression.h"
#include "utils.h"
#include "data.h"

#include <string>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include <map>
#include <set>
#include <queue>
#include <iostream>
#include <cassert>
#include <utility>
#include <limits>

using std::string;
using std::vector;
using std::to_string;
using std::stoi;
using std::map;
using std::set;
using esutils::split;
using std::queue;
using esutils::left_padded_str;
using esutils::oto_map;
using std::cout;
using std::endl;
using std::make_pair;
using std::pair;
using esutils::ExtremeFraction;
using esutils::OneMinusXN;
using std::min;
using std::max;

typedef DataFrame::ColumnMetaData ColumnMetaData;

// functions of class Expression::Symbol

Expression::Symbol::Symbol(const Data& data) : dt(data), var(0) {
	isconstant = true;
}

Expression::Symbol::Symbol(int v) : dt(0), var(v) {
	isconstant = false;
}

bool Expression::Symbol::operator==(const Expression::Symbol& symb) const {
	if(isconstant!=symb.isconstant) return false;
	if(isconstant) return dt==symb.dt;
	else return var==symb.var;
}

bool Expression::Symbol::operator!=(const Expression::Symbol& symb) const {
	return !(this->operator==(symb));
}

// bool Expression::Symbol::operator<(const Expression::Symbol& symb) const {
// 	if(isconstant != symb.isconstant)
// 		return isconstant < symb.isconstant;
// 	if(isconstant)
// 		return dt<symb.dt;
// 	else
// 		return var<symb.var;
// }

// functions of class Expression::Goal

Expression::Goal::Goal(const BaseRelation* brarg, std::vector<Symbol> symbs) 
: br(brarg), symbols(symbs) {
	assert((int) symbols.size() == brarg->get_num_cols());
}

bool Expression::Goal::operator==(const Expression::Goal& goal) const {
	if(br!=goal.br) return false;
	for(uint i=0; i<symbols.size(); i++) 
		if(!(symbols[i]==goal.symbols[i])) return false;
	return true;
}

// functions of class Expression

/**Example expression: 
Qent[k1, k2](d, e) :- K(k1, d); K(k2, d); E(e, d); C(e, str_phone); D(d, int_1)
constants can either be strings or ints. Strings are enclosed in double quotes.
Strings cannot contain the characters ':', '-', ' ', '\n', '\t', '(', ')', '[', ']', ','.
String constants start with the prefix 'str_' and ints start with the prefix 'int_'.
Variables names cannot start with either of the two prefixes.
*/
Expression::Expression(std::string expr, const map<std::string, const BaseRelation*>& name2br) :
Expression() {
	expr.erase(std::remove_if(expr.begin(), expr.end(), 
		[](char chr){ return chr == ' ' || chr == '\n' || chr == '\t';}),
	  expr.end());  // remove spaces, tabs and endlines

	vector<string> head_body = split(expr, ":-");
	assert(head_body.size()==2);
	int numvars = 0;

	// Parse the body first
	for(auto goal: split(head_body[1], ";")) {
		vector<string> br_symbols = split(goal, "(");
		assert(name2br.find(br_symbols[0])!=name2br.end());
		auto br = name2br.at(br_symbols[0]);

		vector<Symbol> symbols; 
		vector<string> tokens = split(split(br_symbols[1],")")[0], ",");
		assert(br->get_num_cols()== (int) tokens.size());
		for(uint pos=0; pos<tokens.size(); pos++) {
			if(tokens[pos].find("str_")==0) {
				symbols.push_back(Symbol(Data(tokens[pos].substr(4, tokens[pos].size()-4))));
				assert(br->dtype_at(pos)==Dtype::String);
			}
			else {
				if(tokens[pos].find("int_")==0) {
					symbols.push_back(Symbol(Data(stoi(tokens[pos].substr(4, tokens[pos].size()-4)))));
					assert(br->dtype_at(pos)==Dtype::Int);
				}
				else {
					string varname = tokens[pos];
					if(name2var.find(varname)==name2var.end()) {
						var2name[numvars] = varname;
						var2dtype[numvars] = br->dtype_at(pos);
						name2var[varname] = numvars++;
					}
					symbols.push_back(Symbol(name2var.at(varname)));
					assert(var2dtype.at(name2var.at(varname))==br->dtype_at(pos));
				}
			}
		}
		goals.push_back(Goal(br, symbols));
	}

	// Parse the head
	string head = head_body[0];
	vector<string> name_head = split(head, "[");
	assert(name_head.size()==2);
	name = name_head[0];
	for(auto varname: split(split(name_head[1], "]")[0], ",")) {  // bound head vars
		assert(name2var.find(varname)!=name2var.end());
		boundheadvars.insert(name2var.at(varname));
		headvars.insert(name2var.at(varname));
	}
	auto freevars = split(split(split(split(name_head[1], "]")[1], "(")[1], ")")[0], ",");
	for(auto varname: freevars) {
		// free head vars
		assert(name2var.find(varname)!=name2var.end());
		freeheadvars.insert(name2var.at(varname));
		headvars.insert(name2var.at(varname));
	}

	for(auto it=var2name.begin(); it!=var2name.end(); it++)
		allvars.insert(it->first);

	assert(goals.size()>0);

	compute_extrafeatures();
}


string Expression::show() const {
	string result = name+"[";
	uint n=0;
	for(auto boundhv: boundheadvars) {
		result += var2name.at(boundhv);
		n+=1;
		if(n!=boundheadvars.size()) result += ", ";
	}
	result += "](";
	n=0;
	for(auto freehv: freeheadvars) {
		result += var2name.at(freehv);
		n+=1;
		if(n!=freeheadvars.size()) result += ", ";
	}
	result += ") :- ";
	n=0;
	for(auto goal: goals) {
		result += goal.br->get_name()+"(";
		uint m=0;
		for(auto symbol: goal.symbols) {
			result += (symbol.isconstant ? symbol.dt.show() : var2name.at(symbol.var));
			m+=1;
			if(m!=goal.symbols.size())
				result +=", ";
		}
		result += ")";
		n+=1;
		if(n!=goals.size())	
			result += ", ";
	}
	result += "\n";
	return result;
}

Expression::Expression() :
name(""), name2var(), var2name(), var2dtype(), goals(), boundheadvars(), freeheadvars(), headvars() {}

Expression Expression::subexpression(const std::set<int>& subset_goals) const {
	assert(subset_goals.size()>0);
	Expression result;
	result.name = name + "::{";
	uint n=0;
	for(auto pos: subset_goals) {
		assert(pos>=0 && pos<(int)goals.size());
		result.name += to_string(pos);
		n+= 1;
		if(n!=subset_goals.size())
			result.name+=",";
		for(auto symbol: goals[pos].symbols) 
			if(!symbol.isconstant) 
				if(result.var2name.find(symbol.var)==result.var2name.end()) {
					result.var2name[symbol.var] = var2name.at(symbol.var);
					result.var2dtype[symbol.var] = var2dtype.at(symbol.var);
					result.name2var[var2name.at(symbol.var)] = symbol.var;
					result.freeheadvars.insert(symbol.var);
					result.headvars.insert(symbol.var);
				}
		result.goals.push_back(goals[pos]);
	}
	for(auto it=result.var2name.begin(); it!=result.var2name.end(); it++)
		result.allvars.insert(it->first);

	result.name+='}';
	result.compute_extrafeatures();
	return result;
}



int Expression::num_goals() const {
	return (int) goals.size();
}

const set<int>& Expression::Expression::vars() const {
	return allvars;
}

const set<int>& Expression::Expression::head_vars() const {
	return headvars;
}

const Expression::Goal& Expression::goal_at(int pos) const {
	return goals.at(pos);
}

int Expression::name_to_var(const std::string& nm) const {
	return name2var.at(nm);
}

string Expression::var_to_name(int var) const {
	return var2name.at(var);
}


map<int, string> Expression::Table::execute_goal(DataFrame& result,
			const Expression* expr, int gid, const BaseRelation::Table* table) {
	result = table->df;
	result.prepend_to_cids(to_string(gid));
	map<int, string> var2cid;
	map<int, string> pos2cid;
	for(uint i=0; i<result.get_header().size(); i++) 
		pos2cid[i]=result.get_header().at(i).cid;
	for(uint i=0; i<expr->goals[gid].symbols.size(); i++) {
		if(expr->goals[gid].symbols.at(i).isconstant)
			result.select(pos2cid.at(i), expr->goals[gid].symbols.at(i).dt);
		else {
			auto var = expr->goals[gid].symbols.at(i).var;
			if(var2cid.find(var)==var2cid.end())
				var2cid[var] = pos2cid.at(i);
			else
				var2cid[var] = result.self_join(var2cid.at(var), pos2cid.at(i));
		}
	}
	return var2cid;
}

Expression::Table::Table(const Expression* exp_arg, 
	std::map<const BaseRelation*, 
	const BaseRelation::Table*> br2table) 
: exp(exp_arg), df(vector<ColumnMetaData>(), vector<vector<Data>>()){

	auto empty_df=df;
	auto allvar2cid = execute_goal(df, exp, 0, br2table.at(exp->goals.at(0).br));

	set<int> remaining_gids;
	for(uint i=1; i<exp->goals.size(); i++)
		remaining_gids.insert(i);
	while(!remaining_gids.empty()) {
		bool done=false;
		for(auto gid: remaining_gids) {
			for(auto symbol: exp->goals.at(gid).symbols) {
				if(!symbol.isconstant)
					if(allvar2cid.find(symbol.var)!=allvar2cid.end()) {
						DataFrame df_goal = empty_df;
						auto var2cid = execute_goal(df_goal, exp, gid, br2table.at(exp->goals.at(gid).br));
						vector<pair<string, string>> this2df;
						for(auto item: var2cid) {
							auto var=item.first;
							if(allvar2cid.find(var)!=allvar2cid.end())
								this2df.push_back(make_pair(allvar2cid.at(var), var2cid.at(var)));
							else
								allvar2cid[var] = var2cid[var];
						}
						df.join(df_goal, this2df);
						remaining_gids.erase(gid);
						done=true;
						break;
					}
			}
			if(done) break;
		}
		assert(done);
	}

	for(auto item: allvar2cid) {
		auto var=item.first;
		if(exp->headvars.find(var)==exp->headvars.end())
			df.project_out(allvar2cid.at(var));
		else
			headvar2cid[var] = allvar2cid.at(var);
	}
}

const string Expression::get_name() const {
	return name;
}

void Expression::compute_extrafeatures() {
	var2goals.clear();
	sketch = "";
	join_merge_sketch = "";

	for(uint gid=0; gid<goals.size(); gid++) {
		for(auto symbol: goals[gid].symbols) {
			if(!symbol.isconstant) {
				if(var2goals.find(symbol.var)==var2goals.end())
					var2goals[symbol.var] = set<int>();
				var2goals[symbol.var].insert(gid);
			}
		}
	}

	compute_sketch();

	// computing join_merge_sketch
	map<string, int> br_name2count;
	map<int, int> var2num_occurences;
	for(uint gid=0; gid<goals.size(); gid++) {
		auto br_name = goals[gid].br->get_name();
		if(br_name2count.find(br_name)==br_name2count.end())
			br_name2count[br_name] = 0;
		br_name2count[br_name] += 1;
		for(auto symbol: goals[gid].symbols) 
			if(!symbol.isconstant) {
				if(var2num_occurences.find(symbol.var)==var2num_occurences.end())
					var2num_occurences[symbol.var] = 0;
				var2num_occurences[symbol.var] += 1;
			}
	}
	join_merge_sketch = "{";
	uint i=0;
	for(auto it=br_name2count.begin(); it!=br_name2count.end(); it++) {
		join_merge_sketch += it->first + ": " + to_string(it->second);
		i++;
		if(i!=br_name2count.size())
			join_merge_sketch += ", ";
	}
	join_merge_sketch += "}, {";
	for(auto it=var2num_occurences.begin(); it!=var2num_occurences.end(); it++) 
		if(it->second>1)
			join_merge_sketch += to_string(it->second)+" ";
	join_merge_sketch += "}";
}

void Expression::compute_sketch() {
	sketch = "{"+to_string(headvars.size())+"}, {";
	set<string> br_names;
	int i=0;
	for(uint gid=0; gid<goals.size(); gid++) {
		br_names.insert(goals[gid].br->get_name());
		for(auto symbol: goals[gid].symbols) {
			if(symbol.isconstant) {
				if(i>0)
					sketch += ", " + symbol.dt.show();
				else
					sketch += symbol.dt.show();
				i++;
			}
		}
	}	
	sketch += "}, {";
	i=0;
	for(auto br_name: br_names) {
		if(i>0)	
			sketch += ", "+br_name;
		else
			sketch += br_name;
		i++;
	}
	sketch += "}";
}

const set<int>& Expression::goals_containing(int var) const {
	return var2goals.at(var);
}

bool Expression::connected(const set<int>& subset_goals) const {
	if(subset_goals.size()==0) return false;
	set<int> uncovered_goals = subset_goals;
	queue<int> gids;
	gids.push(*(uncovered_goals.begin()));
	uncovered_goals.erase(uncovered_goals.begin());
	while(!uncovered_goals.empty()) {
		if(gids.empty()) return false;
		int gid = gids.front();
		gids.pop();
		for(auto symbol: goals.at(gid).symbols) {
			if(!symbol.isconstant) {
				for(int cgid: goals_containing(symbol.var)) {
					if(uncovered_goals.find(cgid)!=uncovered_goals.end()) {
						uncovered_goals.erase(cgid);
						gids.push(cgid);
					}
				}
			}
		}
	}
	return true;
}

const string& Expression::get_sketch() const {
	return sketch;
}

void Expression::drop_headvar(int headvar) {
	assert(headvars.find(headvar)!=headvars.end());
	assert(headvars.size()>1);
	headvars.erase(headvar);
	if(freeheadvars.find(headvar)!=freeheadvars.end())
		freeheadvars.erase(headvar);
	if(boundheadvars.find(headvar)!=boundheadvars.end())
		boundheadvars.erase(headvar);
	compute_sketch();
}

void Expression::make_headvar_bound(int headvar) {
	assert(freeheadvars.find(headvar)!=freeheadvars.end());
	freeheadvars.erase(headvar);
	boundheadvars.insert(headvar);
}

void Expression::select(int var, Data dt) {
	assert(headvars.find(var)!=headvars.end());
	assert(var2dtype.at(var)==dt.get_dtype());

	name2var.erase(var2name.at(var));
	var2name.erase(var);
	var2dtype.erase(var);

	for(uint i=0; i<goals.size(); i++)
		for(uint j=0; j<goals[i].symbols.size(); j++)
			if(!goals[i].symbols[j].isconstant && goals[i].symbols[j].var==var)
				goals[i].symbols[j] = Symbol(dt);

	boundheadvars.erase(var);
	freeheadvars.erase(var);
	headvars.erase(var);
	allvars.erase(var);
	compute_extrafeatures();
}

int Expression::join(int var1, int var2) {
	assert(headvars.find(var1)!=headvars.end());
	assert(headvars.find(var2)!=headvars.end());
	assert(var2dtype.at(var1)==var2dtype.at(var2));

	name2var.erase(var2name.at(var2));
	var2name.erase(var2);
	var2dtype.erase(var2);

	for(uint i=0; i<goals.size(); i++)
		for(uint j=0; j<goals[i].symbols.size(); j++)
			if(!goals[i].symbols[j].isconstant && goals[i].symbols[j].var==var2)
				goals[i].symbols[j] = Symbol(var1);

	boundheadvars.erase(var2);
	freeheadvars.erase(var2);
	headvars.erase(var2);
	allvars.erase(var2);
	compute_extrafeatures();
	return var1;
}


bool Expression::is_free_headvar(int var) const {
	return freeheadvars.find(var)!=freeheadvars.end();
}

bool Expression::is_bound_headvar(int var) const {
	return boundheadvars.find(var)!=boundheadvars.end();
}

const set<int>& Expression::bound_headvars() const {
	return boundheadvars;
}

const string& Expression::get_join_merge_sketch() const {
	return join_merge_sketch;
}

bool Expression::empty() const {
	return goals.size()==0;
}


bool Expression::try_merge(const Expression& exp, oto_map<int, int>& g2g,
		oto_map<int, int>& jv2jv) const {
	if(g2g.size()==goals.size()) return true;
	uint gid=g2g.size();
	for(uint exp_gid=0; exp_gid<exp.goals.size(); exp_gid++) {
		if(!g2g.find_inv(exp_gid) && goals.at(gid).br==exp.goals.at(exp_gid).br) {
			const auto& symbols = goals.at(gid).symbols;
			const auto& exp_symbols = exp.goals.at(exp_gid).symbols;
			bool match=true;
			set<int> new_mappings;
			for(uint i=0; i<symbols.size(); i++) {
				bool eligible1 = (symbols.at(i).isconstant ? false:
					var2goals.at(symbols.at(i).var).size()>1);
				bool eligible2 = (exp_symbols.at(i).isconstant ? false:
					exp.var2goals.at(exp_symbols.at(i).var).size()>1);
				if(eligible1 != eligible2) {
					match=false;
					break;
				}
				if(eligible1) {
					if(!jv2jv.find(symbols.at(i).var)) {
						if(jv2jv.find_inv(exp_symbols.at(i).var)) {
							match=false; break;
						}
						else {
							new_mappings.insert(symbols.at(i).var);
							jv2jv.insert(symbols.at(i).var, exp_symbols.at(i).var);
						}
					}
					if(jv2jv.at(symbols.at(i).var)!=exp_symbols.at(i).var) {
						match=false; break;
					}
				}
			}
			if(match) {
				g2g.insert(gid, exp_gid);
				if(try_merge(exp, g2g, jv2jv)) return true;
				g2g.erase(gid);
			}
			for(auto jvar: new_mappings) jv2jv.erase(jvar);
		}
	}
	return false;
}

int Expression::add_new_var(char code, Dtype dtp, std::string name) {
	assert(code=='h' || code=='f');
	assert(name2var.find(name)==name2var.end());
	int var=(*allvars.rbegin())+1;
	allvars.insert(var);
	if(code=='h') {
		freeheadvars.insert(var);
		headvars.insert(var);
	}
	var2dtype[var] = dtp;
	var2name[var] = name;
	name2var[name] = var;
	return var;
}


Expression Expression::merge_with(const Expression& exp) const {
	if(join_merge_sketch!=exp.join_merge_sketch)
		return Expression();
	oto_map<int, int> g2g;
	oto_map<int, int> jv2jv;
	if(!try_merge(exp, g2g, jv2jv))
		return Expression();
	Expression result(*this);
	result.name = "merge{"+name+", "+exp.name+"}";
	assert(!(this->empty()));

	int num_new_vars=0;
	for(uint gid=0; gid<goals.size(); gid++) {
		auto& symbols = result.goals[gid].symbols;
		const auto& exp_symbols = exp.goals.at(g2g.at(gid)).symbols;
		for(uint i=0; i<symbols.size(); i++) {
			if(symbols[i].isconstant) {
				if(symbols[i]!=exp_symbols.at(i))
					symbols[i] = Symbol(result.add_new_var('h', 
						result.goals[gid].br->dtype_at(i),
						(exp_symbols.at(i).isconstant ? "nv"+to_string(num_new_vars++):
							exp.name+"::"+exp.var2name.at(exp_symbols.at(i).var))));
			}
			else {
				if(result.headvars.find(symbols[i].var)==result.headvars.end()) {
					auto exp_symbol = exp_symbols.at(i);
					if(exp_symbol.isconstant 
						|| exp.headvars.find(exp_symbol.var)!=exp.headvars.end()) {
						result.headvars.insert(symbols[i].var);
						result.freeheadvars.insert(symbols[i].var);
					}
				}
			}
		}
	}
	result.compute_extrafeatures();
	return result;
}

set<int> all_goals(int n) {
	set<int> s;
	for(int i=0; i<n; i++)
		s.insert(i);
	return s;
}

CardinalityEstimator::CardinalityEstimator(const Expression& exp_arg) :
CardinalityEstimator(exp_arg, all_goals(exp_arg.num_goals())) {}

CardinalityEstimator::CardinalityEstimator(const Expression& exp_arg,
		const set<int>& goals_arg) : exp(exp_arg) {

	for(auto gid: goals_arg) 
		add_goal(gid);

	// for(auto gid: goals)
	// 	cout<<goal2selectivity[gid]<<" ";
	// cout<<endl;
}

const set<int> CardinalityEstimator::considered_goals() const {
	return goals;
}

void CardinalityEstimator::add_goal(int gid) {
	if(goals.find(gid)!=goals.end())
		return;
	auto goal = exp.goal_at(gid);
	ExtremeFraction x, n;
	for(int i=0; i<goal.br->get_num_cols(); i++) {
		x.divide(goal.br->card_at(i));
		auto symbol = goal.symbols.at(i);
		if(!symbol.isconstant) {
			if(var2card.find(symbol.var)==var2card.end())
				var2card[symbol.var] = goal.br->card_at(i);
			var2card[symbol.var] = min(var2card[symbol.var], goal.br->card_at(i));
		}
	}
	n.multiply(goal.br->num_tuples());
	goal2selectivity[gid] = max(0.0, min(1.0, 1-OneMinusXN(x, n)));
	goals.insert(gid);
}

vector<double> CardinalityEstimator::get_cardinalities(const vector<int>& varids) const {
	return get_cardinalities(varids, set<int>());
}

vector<double> CardinalityEstimator::get_cardinalities(const vector<int>& varids,
	const set<int>& pre_select_vars) const {
	set<int> select_vids;
	vector<double> result;
	for(auto varid: varids) {
		double card = var2card.at(varid);
		select_vids.insert(varid);
		set<int> remaining_goals = goals;
		while(!remaining_goals.empty()) {
			set<int> considered_vars;
			queue<int> considered_goals;
			considered_goals.push(*remaining_goals.begin());
			remaining_goals.erase(remaining_goals.begin());
			ExtremeFraction x, n;
			while(!considered_goals.empty()) {
				int gid = considered_goals.front();
				considered_goals.pop();
				bool apply_goal=false;
				for(auto symbol: exp.goal_at(gid).symbols) {
					if(symbol.isconstant) continue;
					int var = symbol.var;
					if(var==varid) apply_goal=true;
					if(select_vids.find(var)==select_vids.end() &&
						pre_select_vars.find(var)==pre_select_vars.end()) {
						apply_goal = true;
						if(considered_vars.find(var)==considered_vars.end())
							n.multiply(var2card.at(var));
						considered_vars.insert(var);
						for(auto g: exp.goals_containing(var))
							if(remaining_goals.find(g)!=remaining_goals.end()) {
								remaining_goals.erase(g);
								considered_goals.push(g);
							}
					}
				}
				if (apply_goal) x.multiply(goal2selectivity.at(gid));
			}
			card *= max(0.0, min(1.0, 1-OneMinusXN(x, n)));
		}
		result.push_back(card);
	}
	return result;
}

double CardinalityEstimator::var_card(int var) const {
	return var2card.at(var);
}

bool CardinalityEstimator::is_present(int var) const {
	return var2card.find(var)!=var2card.end();
}

const Expression& CardinalityEstimator::expression() const {
	return exp;
}