#ifndef QUERY_H
#define QUERY_H

#include "expression.h"
#include "data.h"

#include <map>
#include <string>
#include <set>
#include <list>

class ViewTuple;

const double mem_storage_weight=10;
const double disk_storage_weight=1;

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
	double storage_cost() const;
	double avg_block_size() const;
};


/** Query */
class Query {
	Expression exp;
	std::map<Data, int> const2var;
	std::list<BaseRelation::Table> tables;
	std::map<const BaseRelation*, const BaseRelation::Table*> br2table;
public:
	Query(const Expression& exp_arg);
	const Expression& expression() const;
	std::list<ViewTuple> get_view_tuples(const Index& index) const;
};


class ViewTuple {
	const Query& query;
	const Index& index;
	std::map<int, Expression::Symbol> index2query;
	std::set<std::set<int>> subcores;
	void try_match(bool& match, std::set<int>& subcore,
		std::set<int>& unmapped_goals, std::map<int, int>& mu);
public:
	ViewTuple(const Query& query_arg,
		const Index& index_arg,
		std::map<int, Expression::Symbol> index2query_arg);
	std::string show() const;
};

#endif