#include "expression.h"
#include "containment_map.h"

#include <cassert>
#include <map>
#include <string>
#include <set>
#include <iostream>

using std::map;
using std::set;
using std::string;
using std::cout;
using std::endl;

Expression::Goal ContainmentMap::apply(const Expression::Goal& goal) {
	Expression::Goal newgoal = goal;
	for(uint i=0; i<newgoal.symbols.size(); i++)
		if(!newgoal.symbols[i].isconstant)
			newgoal.symbols[i] = var_s2d.at(newgoal.symbols[i].var);
	return newgoal;
}

ContainmentMap::ContainmentMap(const Expression* src, const Expression* dest, 
	const map<int, Expression::Symbol>& var2symb) : src_exp(src), dest_exp(dest), var_s2d(var2symb){
	if(src==NULL || dest==NULL) {
		assert(src==NULL && dest==NULL);
		return;
	}
	for(int var: src->vars())
		assert(var_s2d.find(var) != var_s2d.end());

	int n=src->num_goals();
	for(int i=0; i<n; i++) {
		auto mapped_goal = apply(src->goal_at(i));
		for(int j=0; j<dest->num_goals(); j++) {
			if(mapped_goal==dest->goal_at(j)) {
				goal_s2d[i] = j;
				break;		
			}
		}
		assert(goal_s2d.find(i)!=goal_s2d.end());
	}
}

int ContainmentMap::goal_at(int gid) const {
	return goal_s2d.at(gid);
}

bool ContainmentMap::empty() const {
	return src_exp==NULL;
}

string ContainmentMap::show() const {
	if(src_exp==NULL) return "(empty)";
	string result="{";
	uint pos=0;
	for(auto entry: var_s2d) {
		result += src_exp->var_to_name(entry.first)+"->";
		auto symb = entry.second;
		if(symb.isconstant)
			result += symb.dt.show();
		else
			result += dest_exp->var_to_name(symb.var);
		pos+=1;
		if(pos!=var_s2d.size())
			result+=", ";
	}
	return result+"}";
}



bool containment_map_exists(const Expression* src_exp, const Expression* dest_exp, 
	int pos, map<int, Expression::Symbol>& var2symb) {
	if(pos==src_exp->num_goals()) {
		set<int> covered_head_vars;
		auto target_head_vars = dest_exp->head_vars();
		for(auto head_var: src_exp->head_vars()) {
			if(!var2symb.at(head_var).isconstant) 
				if(target_head_vars.find(var2symb.at(head_var).var)!=target_head_vars.end())
					covered_head_vars.insert(var2symb.at(head_var).var);
		}
		return covered_head_vars.size()==target_head_vars.size();
	}

	auto src_goal = src_exp->goal_at(pos);
	for(int dpos=0; dpos<dest_exp->num_goals(); dpos++) {
		auto dest_goal = dest_exp->goal_at(dpos);
		if(src_goal.br==dest_goal.br) {
			set<int> newly_mapped_vars;
			bool match=true;
			for(uint i=0; i<src_goal.symbols.size(); i++) {
				if(src_goal.symbols.at(i).isconstant) {
					if(src_goal.symbols.at(i) != dest_goal.symbols.at(i)) {
						match=false; 
						break;
					}
				}
				else {
					if(var2symb.find(src_goal.symbols.at(i).var)!=var2symb.end()) {
						if(var2symb.at(src_goal.symbols.at(i).var)!=dest_goal.symbols.at(i)) {
							match=false;
							break;
						}
					}
					else {
						var2symb.emplace(src_goal.symbols.at(i).var, dest_goal.symbols.at(i));
						newly_mapped_vars.insert(src_goal.symbols.at(i).var);
					}
				}
			}
			if(match) {
				pos+=1;
				if(containment_map_exists(src_exp, dest_exp, pos, var2symb))
					return true;
				pos-=1;
			}
			for(auto var: newly_mapped_vars) var2symb.erase(var);
		}
	}
	return false;
}

ContainmentMap find_containment_map(const Expression* src_exp, const Expression* dest_exp) {
	assert(src_exp!=NULL && dest_exp!=NULL);
	map<int, Expression::Symbol> var2symb;
	int pos=0;
	bool match = containment_map_exists(src_exp, dest_exp, pos, var2symb);
	if(match) 
		return ContainmentMap(src_exp, dest_exp, var2symb);
	else
		return ContainmentMap(NULL, NULL, map<int, Expression::Symbol>());
}