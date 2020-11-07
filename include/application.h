#ifndef APPLICATION_H
#define APPLICATION_H

#include "query.h"

#include <map>
#include <set>
#include <vector>
#include <list>
#include <string>

class Application {
	std::list<Query> queries;
	std::list<Index> indexes;
	std::list<ViewTuple> view_tuples_pool;
	std::map<const Index*, std::map<const Query*, std::set<const ViewTuple*>>> index_to_query2vt;
	std::map<const Query*, std::map<const Index*, std::set<const ViewTuple*>>> query_to_index2vt;
	int max_num_goals_index;  //!< maximum number of goals in a candidate index's body
	void generate_candidates();
public:
	Application(const std::vector<Query>& workload, int k=3);
	void show_candidates() const;
};

#endif