#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "data.h"
#include "base_relation.h"

#include <vector>
#include <string>
#include <map>
#include <set>


/**Class to create and manipulate conjunctive expressions used in index definitions and queries.*/
class Expression {
public:
	struct Symbol {
		bool isconstant;
		Data dt;
		int var;
		Symbol(const Data& data);
		Symbol(int v);
		bool operator==(const Symbol& symb) const;
		bool operator<(const Symbol& symb) const;
	};
	struct Goal {
		const BaseRelation* br;
		std::vector<Symbol> symbols; 
		Goal(const BaseRelation* brarg, std::vector<Symbol> symbs);
	};
private:
	std::string name;
	std::map<std::string, int> name2var;
	std::map<int, std::string> var2name;
	std::map<int, Dtype> var2dtype;
	std::vector<Goal> goals;
	std::set<int> boundheadvars;
	std::set<int> freeheadvars;
	std::set<int> headvars;
	std::string signature;
public:
	Expression(std::string expr, const std::map<std::string, const BaseRelation*>& name2br);   //!< parse an expression from a string representation
	std::string show() const;  //!< returns a string representation for debugging purposes
	Expression subexpression(std::set<int> subset_goals) const;  //!< returns a subexpression made up of given subset of goals. It shares the variables with the original query
	/**Signature is a summary of the expression graph. It can used to quickly test if two expressions 
	are isomorphic. It has the following format: [(br1, countbr1, numvars_br1), (br2, countbr2, numvars_br2), ..., (brn, countbrn, numvars_brn)]
	Each br<i> is a unique base relation id that appears in the expression. countbr<i> is the number of goals with that bare relation.
	numvars_br<i> is the number of unique variables that appear in those goals.*/
	std::string get_signature() const;
	std::string get_var(int id) const; //!< returns variable name with the given id
	uint num_goals() const;
	Goal goal_at(uint gid) const;
	const std::set<int>& head() const;

	/**Tries to match this object with the given expression. If the two expressions are isomorphic, it returns
	a mapping from the variables of this expression to the target expression such that the goals of this
	expression transform to the goals of the given expression. In case they are not isomorphic, it returns an empty map.
	If matchconsts is true, then any two constants are considered equal during the matching process.
	WARNING: it compares only body of the expressions and ignores the head*/
	std::map<int, int> isomorphic_match(Expression exp, bool matchconsts = true) const;

private:
	Expression(); //!< creates an empty expression
	void compute_signature();
	bool backtrack_match(const Expression& exp, std::map<int,int>& goal2goal, 
		std::map<int,int>& var2var, bool matchconsts) const;
};

#endif