#ifndef APPLICATION_H
#define APPLICATION_H

#include "query.h"
#include "base_relation.h"

#include <map>
#include <set>
#include <vector>
#include <list>
#include <string>

class Application {

	// struct Design {
	// 	std::set<const Index*> stored_indexes;
	// 	std::map<const Query*, Plan> plans;

	// 	Design(std::set<const Index*> inds, std::map<const Query*, Plan> ps);
	// 	bool iscomplete() const;
	// 	std::string show(const std::map<const MCD*, uint>& mcd_to_order) const;
	// };

	std::list<Query> queries;
	std::list<Index> indexes;
	std::list<ViewTuple> view_tuples_pool;
	std::map<const Index*, std::map<const Query*, std::set<const ViewTuple*>>> index_to_query2vt;
	std::map<const Query*, std::map<const Index*, std::set<const ViewTuple*>>> query_to_index2vt;
	int max_num_goals_index;  //!< maximum number of goals in a candidate index's body
	void generate_candidates(const std::map<const BaseRelation*, const BaseRelation::Table*> br2table);

public:
	//!< parameters
	static int max_iters; //!< max number of iterations
	static double max_index_size;  //!< maximum size of a condidate index
	static double max_vt_lb;  //!< maximum lower bound cost of a view tuple
	static double wt_storage;  //!< relative weight of storage cost w.r.t time cost

	Application(const std::vector<Query>& workload, const std::map<const BaseRelation*, 
		const BaseRelation::Table*> br2table, int k=3);
	void show_candidates() const;

	// void optimize();

	// void show_design() const;
};

#endif