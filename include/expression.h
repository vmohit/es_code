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
	std::string sketch;
	std::map<int, std::set<int>> var2goals; //!< maps a variable to the set of goals in which it appears
	std::string join_merge_sketch;

	void compute_extrafeatures();
	void compute_sketch();
	bool try_merge(const Expression& exp, esutils::oto_map<int, int>& g2g,
		esutils::oto_map<int, int>& sv2sv) const;
	int add_new_var(char code, Dtype dtp, std::string name); //!< 'h', 'f' for head or free
public:
	Expression(std::string expr, const std::map<std::string, const BaseRelation*>& name2br);   //!< parse an expression from a string representation
	std::string show() const;  //!< returns a string representation for debugging purposes
	Expression subexpression(const std::set<int>& subset_goals) const;  //!< returns a subexpression made up of given subset of goals. It shares the variables with the original query
	int num_goals() const;
	int name_to_var(const std::string& nm) const; //!< for debugging
	std::string var_to_name(int var) const;
	const std::set<int>& vars() const;
	const std::set<int>& head_vars() const;
	const Goal& goal_at(int pos) const;
	const std::string get_name() const;
	const std::set<int>& goals_containing(int var) const;
	bool connected(const std::set<int>& subset_goals) const; //!< returns true if the subset of goals are connected via joins
	const std::string& get_sketch() const;
	void drop_headvar(int headvar); 
	void make_headvar_bound(int headvar);
	bool is_free_headvar(int var) const;
	bool is_bound_headvar(int var) const;

	const std::string& get_join_merge_sketch() const;
	bool empty() const;
	Expression merge_with(const Expression& exp) const;
private:
	Expression(); //!< creates an empty expression
};

class CardinalityEstimator {
	const Expression& exp;
	std::set<int> goals;  //!< which goals to consider while estimating cost
	std::map<int, double> goal2selectivity;
	std::map<int, double> var2card;
public:
	CardinalityEstimator(const Expression& exp_arg,
		const std::set<int>& goals_arg);
	void add_goal(int gid);  
	/** Get cardinalities of an array of variables in the given order
	Example:- if Q(k, d) :- K(k, d) is the expression with there being
	1000 distinct keywords, 100 distinct documents and 10000 tuples in K,
	then if the vector of variables is [d, k] it returns [100, 100].
	*/
	double var_card(int var) const;  //!< returns maximum cardinality of a given variable
	std::vector<double> get_cardinalities(const std::vector<int>& varids) const;
};

#endif