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
#include <iostream>
#include <cassert>

using std::string;
using std::vector;
using std::to_string;
using std::stoi;
using std::map;
using std::set;
using esutils::split;
using esutils::left_padded_str;
using std::cout;
using std::endl;

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
name(""), name2var(), var2name(), var2dtype(), goals(), boundheadvars(), freeheadvars(), headvars(), signature("") {}

Expression Expression::subexpression(std::set<int> subset_goals) const {
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