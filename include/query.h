#ifndef QUERY_H
#define QUERY_H

#include "expression.h"
#include "data.h"

#include <map>
#include <string>
#include <set>
#include <list>

class ViewTuple;

/** Index */
class Index {
	Expression exp;
public:
	Index(const Expression& exp_arg);
	const Expression& expression() const;
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
public:
	ViewTuple(const Query& query_arg,
		const Index& index_arg,
		std::map<int, Expression::Symbol> index2query_arg);
	std::string show() const;
};

#endif