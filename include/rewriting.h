#ifndef REWRITING_H
#define REWRITING_H

#include "query.h"

#include <map>
#include <string>
#include <set>

class ViewTuple {
	const QueryTemplate* query;
	const Index* index;
	/** mapping from the variables of index to the variables of the query 
	such that each goal in the body of the index maps to some goal in 
	the body of the query*/
	std::map<int, Expression::Symbol> index2query;  
	std::multiset<std::set<int>> subcores;

public:
	ViewTuple(const QueryTemplate* qt, const Index* ind); //!< creates an empty view tuple for the given query index pair
	/** creates a view tuple with one-to-one matching between index and query
		WARNING: This function does not perform any integrity checks. 
	*/
	ViewTuple(const QueryTemplate* qt, const Index* ind, 
		const std::set<int>& core, const std::map<int, int>& query2index);  
	/** creates a new view from an old view. The index behind this new view is same as the index underlying the old view
	except that the old index has one extra variable in head.
	*/
	ViewTuple(const ViewTuple& old_vt, const Index* newind, int removed_var);

	std::string show() const; //!< returns a string representation
};

#endif