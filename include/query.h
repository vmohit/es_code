#ifndef QUERY_H
#define QUERY_H

#include "expression.h"
#include "data.h"
#include "dataframe.h"

#include <map>
#include <string>
#include <set>
#include <list>

class ViewTuple;

const double mem_storage_weight=10;
const double disk_storage_weight=1;
const double disk_read_time_per_unit=1;
const double disk_seek_time=100;

/** Index */
class Index {
	Expression exp;
	CardinalityEstimator E;

	double avg_disk_block_size;
	double total_storage_cost;
public:
	//Index& operator=(const Index& other);
	Index(const Expression& exp_arg);
	const Expression& expression() const;
	std::string show(bool verbose=true) const;
	double storage_cost() const;
	double avg_block_size() const;
	const CardinalityEstimator& get_estimator() const;
};

/** Query */
class Query {
	Expression exp;
	std::map<Data, int> const2var;
	std::list<BaseRelation::Table> tables;
	std::map<const BaseRelation*, const BaseRelation::Table*> br2table;

	double wt;
	std::vector<int> g_order;
public:
	Query(const Expression& exp_arg, double wgt);
	const std::vector<int>& goal_order() const;
	const Expression& expression() const;
	std::list<ViewTuple> get_view_tuples(const Index& index) const;
	std::string show(bool verbose=true) const; 
	double weight() const; 
};


class ViewTuple {
public:
	const Query& query;
	const Index& index;
	std::map<int, Expression::Symbol> index2query;  //!< index variables to query variables
	std::set<std::set<int>> subcores;
	double cost_lb = 0;  //!< lower bound
	double cost_ub = 1e20; //!< upper bound
	std::set<uint> sc_goals;  //!< strongly covered goals
	std::set<uint> wc_goals;  //!< weakly covered goals

	uint first_sc_goal_ind; //!< index of the first sc goal based on the goal order of the query

	ViewTuple(const Query& query_arg,
		const Index& index_arg,
		std::map<int, Expression::Symbol> index2query_arg);
	std::string show(bool verbose=true) const;

	std::map<int, int> qvar2evar;
	CardinalityEstimator E;

	// same quantities but only considering the bound head variables
	std::map<int, int> qvar2evar_bh;
	CardinalityEstimator E_bh;

	const std::set<uint>& covered_goals(bool oe) const;  //!< returns weakly or strongly covered goals
private:

	void try_match(bool& match, std::set<int>& subcore,
		std::set<int>& unmapped_goals, std::map<int, int>& mu);
};


class Plan {
	const Query& query;
	std::vector<const ViewTuple*> stages;
	std::set<const ViewTuple*> set_stages;
	double cost=0;

	std::set<uint> sc_goals;
	std::set<uint> wc_goals;
	bool complete=false;

	CardinalityEstimator E;

	bool check_completeness(std::set<int>& covered_goals,
		std::vector<std::set<int>>& subcores, uint pos) const;
public:
	Plan(const Query& qry);
	bool append(const ViewTuple& vt); 
	double time(const ViewTuple& vt) const;
	double time(const ViewTuple* vt) const; 
	double current_cost() const;

	bool iscomplete() const;

	const ViewTuple* stage(uint i) const;
	uint num_stages() const;
	bool has_vt(const ViewTuple* vt) const;

	const std::set<uint>& strongly_covered_goals() const;
	const std::set<uint>& weakly_covered_goals() const;
	const std::set<uint>& covered_goals(bool oe) const;

	int extra_sc_goals(const ViewTuple& vt) const; //!< returns how many extra strongly covered goals would be there if add vt tot he plan
	int extra_wc_goals(const ViewTuple& vt) const;

	std::string show() const;
};

#endif