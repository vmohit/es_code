#ifndef APPLICATION_H
#define APPLICATION_H

#include "query.h"
#include "base_relation.h"

#include <map>
#include <unordered_map>
#include <set>
#include <vector>
#include <list>
#include <string>

class Application {
public:
	struct Design {
		std::set<const Index*> stored_indexes;
		std::unordered_map<const Query*, Plan> plans;
		double cost;
		int num_goals_remaining; //!< considering weakly covered goals

		Design(std::set<const Index*> inds, std::unordered_map<const Query*, Plan> ps);
		void append(const ViewTuple* vt);
		bool iscomplete() const;
		std::string show() const;
		double lb_cost() const; 
	};
private:

	std::list<Query> queries;
	std::list<Index> indexes;
	std::list<ViewTuple> view_tuples_pool;
	std::map<const Index*, std::map<const Query*, std::set<const ViewTuple*>>> index_to_query2vt;
	std::map<const Query*, std::map<const Index*, std::set<const ViewTuple*>>> query_to_index2vt;
	int max_num_goals_index;  //!< maximum number of goals in a candidate index's body
	void generate_candidates();

	Design get_empty_design() const;
	Design optimize_wsc(const Design& Dinit, bool oe) const;
	std::set<const ViewTuple*> inner_greedy(const Index* index, uint& negc,
		double& cost, const std::map<const Query*, std::set<uint>>& rem_goals, uint num_rem_goals,
		const std::unordered_map<const Query*, Plan>& q2plan, bool oe) const;

	std::set<const ViewTuple*> inner_greedy_standalone(const Index* index, uint& negc,
		double& cost, const std::map<const Query*, std::set<uint>>& rem_goals, uint num_rem_goals,
		bool oe) const;

	double approx_factor;
	double Lub;

	double lb_cost(const Design& D) const;
	double ub_cost(const Design& D) const;
	double get_priority(const Design& D) const;
	std::list<Design> expand(const Design& D) const;
public:

	//!< parameters
	static int max_iters; //!< max number of iterations
	static double max_index_size;  //!< maximum size of a condidate index
	static double max_vt_lb;  //!< maximum lower bound cost of a view tuple
	static double wt_storage;  //!< relative weight of storage cost w.r.t time cost
	static uint pick_fn;  //!< 0->best LB, 1->best UB, 2->least remaining goals
	static int branch_factor; //!< max number of designs to return when calling expand. If it is -1, return all designs 

	Application(const std::vector<Query>& workload, int k=3);
	void show_candidates() const;

	Design optimize();
	Design optimize_wsc(bool oe);

	Design optimize_wsc_standalone(bool oe);  //!< optimizes weighted set cover instance using greedy goal order
};

#endif