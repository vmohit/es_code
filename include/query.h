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
const double disk_read_time_per_unit=0;
const double disk_seek_time=1;

/** Index */
class Index {
	Expression exp;
	Expression::Table stats;
	
	double avg_disk_block_size=0;
	double total_storage_cost=0;
public:
	std::vector<std::vector<Data>> rearranged_rows;

	Index(const Expression& exp_arg, const std::map<const BaseRelation*, 
		const BaseRelation::Table*>& br2table);
	const Expression& expression() const;
	const Expression::Table& get_stats() const;
	std::string show() const;
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
public:
	Query(const Expression& exp_arg, const std::map<const BaseRelation*, 
		const BaseRelation::Table*>& stats_br2table, int num_samples=10);
	const Expression& expression() const;
	std::list<ViewTuple> get_view_tuples(const Index& index) const;
	const std::vector<std::map<int, Data>>& get_sample_inputs() const;
	std::string show() const;  
};


class ViewTuple {
public:
	const Query& query;
	const Index& index;
	std::map<int, Expression::Symbol> index2query;  //!< index variables to query variables
	std::set<std::set<int>> subcores;
	double cost_lb = 0;  //!< lower bound
	double cost_ub = 10000000; //!< upper bound
	std::set<int> sc_goals;  //!< strongly covered goals
	std::set<int> wc_goals;  //!< weakly covered goals

	ViewTuple(const Query& query_arg,
		const Index& index_arg,
		std::map<int, Expression::Symbol> index2query_arg);
	std::string show() const;

private:
	void try_match(bool& match, std::set<int>& subcore,
		std::set<int>& unmapped_goals, std::map<int, int>& mu);
};


class Plan {
	const Query& query;
	std::vector<ViewTuple> stages;
	std::vector<std::list<DataFrame>> stats;
	std::vector<std::list<std::map<int, std::string>>> queryvar2cid;
	double cost=0;

	void execute_view_tuple(const ViewTuple& vt, std::vector<DataFrame>& df_vt_lst, 
		std::vector<std::map<int, std::string>>& qvar2cid_lst) const;
	double try_append(const ViewTuple& vt, std::vector<std::list<DataFrame>>& new_stats,
		std::vector<std::list<std::map<int, std::string>>>& new_queryvar2cid) const;

	std::set<int> sc_goals;
	std::set<int> wc_goals;

	bool check_completeness(std::set<int>& covered_goals,
		std::vector<std::set<int>>& subcores, uint pos) const;
public:
	Plan(const Query& qry);
	bool append(const ViewTuple& vt); 
	double time(const ViewTuple& vt) const; 
	double current_cost() const;

	bool iscomplete() const;
	 
	std::set<int> strongly_covered_goals() const;
	std::set<int> weakly_covered_goals() const;

	int extra_sc_goals(const ViewTuple& vt) const; //!< returns how many extra strongly covered goals would be there if add vt tot he plan
	int extra_wc_goals(const ViewTuple& vt) const;

	std::string show() const;
};

#endif