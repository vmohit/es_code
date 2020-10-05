#include "expression.h"
#include "containment_map.h"

#include <cassert>

Expression::Goal ContainmentMap::apply(const Expression::Goal& goal) {
	Expression::Goal newgoal = goal;
	for(uint i=0; i<newgoal.symbols.size(); i++)
		if(!newgoal.symbols[i].isconstant)
			newgoal.symbols[i].var = var_s2d.at(newgoal.symbols[i].var);
	return newgoal;
}

ContainmentMap::ContainmentMap(const Expression* src, const Expression* dest, 
	const std::map<int, int>& var2var) : src_exp(src), dest_exp(dest), var_s2d(var2var){

	for(int var: src->vars())
		assert(var_s2d.find(var) != var_s2d.end());

	int n=src->num_goals();
	for(int i=0; i<n; i++) {
		auto ng = apply(src->goal_at(i));
		for(int j=0; j<dest->num_goals(); j++) {
			if(ng==dest->goal_at(j)) {
				goal_s2d[i] = j;
				break;		
			}
		}
		assert(goal_s2d.find(i)!=goal_s2d.end());
	}
}

int ContainmentMap::at(int gid) const {
	return goal_s2d.at(gid);
}