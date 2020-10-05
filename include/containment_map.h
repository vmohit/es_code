#ifndef CONTAINMENT_MAP_H
#define CONTAINMENT_MAP_H

#include "expression.h"
#include <map>


/** Map from variables of source expression to a target expression such that each goal of 
source expression maps to some goal of target expression */
class ContainmentMap {
	const Expression* src_exp;
	const Expression* dest_exp;
	std::map<int, int> var_s2d;
	std::map<int, int> goal_s2d;

	Expression::Goal apply(const Expression::Goal& goal);

public:

	ContainmentMap(const Expression* src, const Expression* dest, 
		const std::map<int, int>& var2var);
	int at(int gid) const; //!< maps the goal id in src expression to the mapped goal id in target expression
};

#endif