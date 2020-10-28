#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "data.h"
#include "base_relation.h"
#include "dataframe.h"
#include "utils.h"

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
		bool operator!=(const Symbol& symb) const;
	};
	struct Goal {
		const BaseRelation* br;
		std::vector<Symbol> symbols; 
		Goal(const BaseRelation* brarg, std::vector<Symbol> symbs);
		bool operator==(const Goal& goal) const;
	};

	/**A dataframe associated to an expression*/
	class Table {
	public:
		const Expression* exp;
		DataFrame df;
		std::map<int, std::string> headvar2cid;
		Table(const Expression* exp_arg, std::map<const BaseRelation*, 
			const BaseRelation::Table*> br2table);
	private:
		std::map<int, std::string> execute_goal(DataFrame& result,
			const Expression* exp_arg, int gid, const BaseRelation::Table* table);
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
	std::set<int> allvars;
	std::string signature;
public:
	Expression(std::string expr, const std::map<std::string, const BaseRelation*>& name2br);   //!< parse an expression from a string representation
	std::string show() const;  //!< returns a string representation for debugging purposes
	Expression subexpression(std::set<int> subset_goals) const;  //!< returns a subexpression made up of given subset of goals. It shares the variables with the original query
	int num_goals() const;
	int name_to_var(const std::string& nm) const; //!< for debugging
	std::string var_to_name(int var) const;
	const std::set<int>& vars() const;
	const std::set<int>& head_vars() const;
	const Goal& goal_at(int pos) const;
	const std::string get_name() const;

private:
	Expression(); //!< creates an empty expression
};

#endif