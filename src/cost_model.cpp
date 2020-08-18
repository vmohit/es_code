// #include "cost_model.h"

// #include <cmath>
// #include <map>
// #include <algorithm>
// #include <iostream>

// using std::cout;
// using std::endl;
// using std::min;
// using std::map;

// double CardinalityEstimator::cardinality(const Expression& expr) const {
// 	double logP=0;
// 	map<int, double> var2domsize;
// 	for(uint gid=0; gid<expr.num_goals(); gid++) {
// 		auto goal = expr.goal_at(gid);
// 		logP += log(goal.br->num_tuples());
// 		for(int col=0; col< goal.br->get_num_cols(); col++) {
// 			if(goal.symbols[col].isconstant) {
// 				logP += log(goal.br->get_count(col, goal.symbols[col].dt));
// 				logP -= log(goal.br->num_tuples());
// 			}
// 			else {
// 				logP -= log(goal.br->get_domsize(col));
// 				auto var = goal.symbols[col].var;
// 				if(var2domsize.find(var)==var2domsize.end())
// 					var2domsize[var] = goal.br->get_domsize(col);
// 				var2domsize[var] = min(var2domsize[var], (double) goal.br->get_domsize(col));
// 			}
// 		}
// 	}

// 	double logD=0;
// 	for(auto var: expr.head())
// 		logD += log(var2domsize.at(var));

// 	double result=1;
// 	for(auto var: expr.head()) {
// 		logD -= log(var2domsize.at(var));
// 		double term;
// 		if(logP<log(0.01)) {
// 			if(logP+logD > 0)
// 				term = var2domsize.at(var) * (1-exp(-1*exp(logP+logD)));
// 			else
// 				term = var2domsize.at(var) * exp(logP+logD);
// 		}
// 		else 
// 			term = var2domsize.at(var) * (1.0-pow(1.0-exp(logP), exp(min(logD, log(1000.0)))));
// 		cout<<expr.get_var(var)<<" "<<term<<" "<<logP<<" "<<" "<<logD<<endl;
// 		result += result*term;
// 	}
// 	return result;
// }