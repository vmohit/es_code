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
const double disk_read_time_per_unit=10;
const double disk_seek_time=1;

/** Index */
class Index {
	Expression exp;
	
	double avg_disk_block_size=0;
	double total_storage_cost=0;
public:

	Index(const Expression& exp_arg);
	const Expression& expression() const;
	std::string show(bool verbose=true) const;
	double storage_cost() const;
	double avg_block_size() const;
};


/** Query */
class Query {
	Expression exp;
	std::map<Data, int> const2var;
	std::list<BaseRelation::Table> tables;
	std::map<const BaseRelation*, const BaseRelation::Table*> br2table;

	/**
	Randomly selected sample inputs used for estimating cost.
	Each sample input is a map from bound head var to some constant such that 
	the output of the query instance is non empty
	*/
	std::vector<std::map<int, Data>> sample_inputs;  
	double wt;
public:
	Query(const Expression& exp_arg, double wgt, const std::map<const BaseRelation*, 
		const BaseRelation::Table*>& stats_br2table, int num_samples=10);
	const Expression& expression() const;
	std::list<ViewTuple> get_view_tuples(const Index& index) const;
	const std::vector<std::map<int, Data>>& get_sample_inputs() const;
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
	double cost_ub = 10000000; //!< upper bound
	std::set<uint> sc_goals;  //!< strongly covered goals
	std::set<uint> wc_goals;  //!< weakly covered goals

	ViewTuple(const Query& query_arg,
		const Index& index_arg,
		std::map<int, Expression::Symbol> index2query_arg);
	std::string show() const;

	const std::set<uint>& covered_goals(bool oe) const;

private:
	void try_match(bool& match, std::set<int>& subcore,
		std::set<int>& unmapped_goals, std::map<int, int>& mu);
};


class Plan {
	const Query& query;
	std::vector<const ViewTuple*> stages;
	std::set<const ViewTuple*> set_stages;
	std::list<std::list<DataFrame>> stats;
	std::list<std::list<std::map<int, std::string>>> queryvar2cid;
	double cost=0;

	void execute_view_tuple(const ViewTuple& vt, std::list<DataFrame>& df_vt_lst, 
		std::list<std::map<int, std::string>>& qvar2cid_lst) const;
	double try_append(const ViewTuple& vt, std::list<std::list<DataFrame>>& new_stats,
		std::list<std::list<std::map<int, std::string>>>& new_queryvar2cid) const;

	std::set<uint> sc_goals;
	std::set<uint> wc_goals;
	bool complete=false;

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