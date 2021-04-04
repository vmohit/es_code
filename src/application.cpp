#include "application.h"
#include "query.h"
#include "utils.h"
#include "data.h"
#include "containment_map.h"
#include "expression.h"
#include "base_relation.h"

#include <vector>
#include <iostream>
#include <set>
#include <utility>
#include <map>
#include <unordered_map>
#include <list>
#include <string>
#include <queue>
#include <algorithm>
#include <cmath>
#include <memory>
#include <functional>

using std::function;
using std::shared_ptr;
using std::vector;
using std::min;
using esutils::generate_subsets;
using esutils::set_intersection_size;
using esutils::set_difference_inplace;
using esutils::set_difference;
using std::cout;
using std::endl;
using std::list;
using std::map;
using std::set;
using std::to_string;
using std::priority_queue;
using std::string;
using std::pair;
using std::unordered_map;
using std::make_pair;

// initializing parameters
int Application::max_iters=10;
double Application::max_index_size=1e20;
double Application::max_vt_lb=1e20;
double Application::wt_storage=0.001;
int Application::branch_factor=-1;
uint Application::pick_fn=0;


Application::Application(const vector<Query>& workload, int k) {
	for(auto& query: workload)
		queries.push_back(query);
	max_num_goals_index = k;
	generate_candidates();
	double num_nodes=0;
	for(auto& query: workload) 
		num_nodes += query.expression().num_goals();
	approx_factor = 2*log(num_nodes)*log(num_nodes);
}

void Application::generate_candidates() {
	// obtain complete plans for each query to estimate lower bound time estimates for view tuples
	unordered_map<const Query*, Plan> complete_plans;
	list<ViewTuple> complete_vts;
	for(auto& query: queries) {
		Index index(query.expression());
		Plan P(query);
		bool found_vt=false;
		for(auto& vt: query.get_view_tuples(index)) {
			int num_goals=0;
			for(auto& sub_core: vt.subcores)
				num_goals += sub_core.size();
			if(num_goals==query.expression().num_goals()) {
				complete_vts.push_back(vt);
				P.append(complete_vts.back());
				found_vt=true;
				break;
			}
		}
		assert(found_vt);
		complete_plans.emplace(&query, P);
	}
	cout<<"->1<-\n";

	// generate all subexpressions of every query to get an initial set of candidate index
	for(auto& query: queries) {
		for(int k=1; k<=max_num_goals_index && k<=query.expression().num_goals(); k++) {
			for(auto& subset: generate_subsets(
				query.expression().num_goals(), k)) {
				if(query.expression().connected(subset)) {
					Expression exp = query.expression().subexpression(subset);
					bool match=false;
					for(auto& index: indexes) {
						if(index.expression().get_sketch()
							==exp.get_sketch()) {
							if(isomorphic(&index.expression(), &exp)) {
								match = true;
								break;
							}
						}
					}
					if(!match)
						indexes.push_back(Index(exp));
				}
			}
		}
	}
	cout<<"->2<-\n";

	// merge candidates with same join structure to generate more candidates
	auto it=indexes.begin();
	while(it!=indexes.end()) {
		for(auto it2=indexes.begin(); it2!=it; it2++) {
			if(it->expression().get_join_merge_sketch()==it2->expression().get_join_merge_sketch()) {
				Expression exp=it->expression().merge_with(it2->expression());
				if(!exp.empty()) {
					bool match=false;
					for(auto& index: indexes) {
						if(index.expression().get_sketch()
							==exp.get_sketch()) {
							if(isomorphic(&index.expression(), &exp)) {
								match = true;
								break;
							}
						}
					}
					if(!match)
						indexes.push_back(Index(exp));
				}
			}
		}
		it++;
	}
	cout<<"->3<-\n";

	// drop a subset of head variables from each candidate to generate more candidates
	int N=indexes.size();
	for(auto index=indexes.begin(); N>0; index++, N--) {
		int num_headvars = index->expression().head_vars().size();
		vector<int> head_vars;
		for(auto var: index->expression().head_vars())
			head_vars.push_back(var);
		for(int r=1; r<num_headvars; r++) {
			for(auto& subset: generate_subsets(num_headvars, r)) {
				Expression expr=index->expression();
				for(auto i: subset)
					expr.drop_headvar(head_vars[i]);
				bool match=false;
				for(auto& some_index: indexes) {
					if(some_index.expression().get_sketch()
						==expr.get_sketch()) {
						if(isomorphic(&some_index.expression(), &expr)) {
							match = true;
							break;
						}
					}
				}
				if(!match)
					indexes.push_back(Index(expr));
			}
		}
	}
	cout<<"->4<-\n";

	// move a subset of free headvars to bound headvars
	N=indexes.size();
	for(auto index=indexes.begin(); N>0; index++, N--) {
		int num_headvars = index->expression().head_vars().size();
		vector<int> head_vars;
		for(auto var: index->expression().head_vars())
			head_vars.push_back(var);
		for(int r=1; r<num_headvars; r++) {
			for(auto& subset: generate_subsets(num_headvars, r)) {
				Expression expr=index->expression();
				for(auto i: subset)
					expr.make_headvar_bound(head_vars[i]);
				bool match=false;
				for(auto& some_index: indexes) {
					if(some_index.expression().get_sketch()
						==expr.get_sketch()) {
						if(isomorphic(&some_index.expression(), &expr)) {
							match = true;
							break;
						}
					}
				}
				if(!match)
					indexes.push_back(Index(expr));
			}
		}
	}
	cout<<"->5<-\n";

	// remove indexes that are too big
	list<list<Index>::iterator> indexes_to_delete;
	for(auto it=indexes.begin(); it!=indexes.end(); it++)
		if(it->storage_cost()>max_index_size)
			indexes_to_delete.push_back(it);
	for(auto it: indexes_to_delete)
		indexes.erase(it);

	// generate view tuples for all the candidates index generated 
	for(auto& query: queries) {
		for(auto& index: indexes) {
			for(auto& vt: query.get_view_tuples(index)) {
				vt.cost_lb = complete_plans.at(&query).time(vt);
				if(vt.cost_lb<max_vt_lb) {
					view_tuples_pool.push_back(vt);
					if(index_to_query2vt.find(&index)==index_to_query2vt.end())
						index_to_query2vt[&index] = {};
					if(index_to_query2vt[&index].find(&query)==index_to_query2vt[&index].end())
						index_to_query2vt[&index][&query] = {};
					index_to_query2vt[&index][&query].insert(&(view_tuples_pool.back()));

					if(query_to_index2vt.find(&query)==query_to_index2vt.end())
						query_to_index2vt[&query] = {};
					if(query_to_index2vt[&query].find(&index)==query_to_index2vt[&query].end())
						query_to_index2vt[&query][&index] = {};
					query_to_index2vt[&query][&index].insert(&(view_tuples_pool.back()));
				}
			}
		}
	}

	// obtain partial query plans for each goal position in each query
	unordered_map<const Query*, unordered_map<int, Plan>> query2goalordernum2plan;
	for(auto& query: queries) {
		map<int, const ViewTuple*> goalnum2vt;
		for(auto& index_vts: query_to_index2vt.at(&query)) {
			if(index_vts.first->expression().num_goals()==1) {
				for(auto vt: index_vts.second) {
					if(vt->subcores.size()==1)
						if(vt->subcores.begin()->size()==1) {
							goalnum2vt[*(vt->subcores.begin()->begin())]=vt;
							// cout<<"goal num: "<<*(vt->subcores.begin()->begin())<<endl;
							// cout<<"vt: "<<vt->show()<<endl;
						}
				}
			}
		}
		// assert(false);
		unordered_map<int, Plan> goalordernum2plan;
		for(int i=0; i<=query.expression().num_goals(); i++) {
			goalordernum2plan.emplace(i, Plan(query));
			for(int j=0; j<i; j++)
				goalordernum2plan.at(i).append(*goalnum2vt[query.goal_order().at(j)]);
			// cout<<"---------------------------\n";
			// cout<<"goalordernum="<<i<<endl;
			// cout<<goalordernum2plan.at(i).show()<<endl;
			// cout<<"++++++++++++++++++++++++++++++\n\n";
		}
		query2goalordernum2plan.emplace(&query, goalordernum2plan);
		// assert(false);
	}

	// compute cost_ub for view tuples with at least one sc goal
	for(auto& vt: view_tuples_pool)
		if(vt.first_sc_goal_ind < vt.query.goal_order().size()) {
			// cout<<vt.first_sc_goal_ind<<" "<<vt.query.goal_order().size()<<endl;
			// cout<<query2goalordernum2plan.at(&(vt.query)).size()<<endl;
			assert(query2goalordernum2plan.at(&(vt.query)).find(vt.first_sc_goal_ind)
				!=query2goalordernum2plan.at(&(vt.query)).end());
			vt.cost_ub = query2goalordernum2plan.at(&(vt.query)).at(vt.first_sc_goal_ind).time(vt);
			cout<<vt.show()<<endl;
		}

	// delete indexes that have no feasible view tuples
	vector<list<Index>::iterator> unwanted_indexes;
	for(auto it=indexes.begin(); it!=indexes.end(); it++)
		if(index_to_query2vt.find(&(*it))==index_to_query2vt.end())
			unwanted_indexes.push_back(it);
	for(auto it: unwanted_indexes)
		indexes.erase(it);
}

void Application::show_candidates() const {
	int pos=1;
	for(const auto& index: indexes) {
		cout<<"Index: "<<pos++<<endl;
		cout<<index.show()<<endl;
		cout<<"\n\nView Tuples:\n";

		int pos_q=1;
		for(auto item: index_to_query2vt.at(&index)) {
			cout<<"\tQuery: "<<pos_q++<<endl;
			cout<<"\t"<<item.first->expression().show()<<endl;

			for(auto vt: item.second) {
				cout<<"\t\t"<<vt->show()<<endl;
			}
		} 
		cout<<"\n----------------------\n\n";
	}
	cout<<indexes.size()<<" indexes\n";
	cout<<view_tuples_pool.size()<<" view tuples\n";

	// cout<<"------------END--------------------\n\n";

	// double maxindsz=-1;
	// const Index* biggest_index=NULL;
	// for(auto& index: indexes)
	// 	if(index.storage_cost()>maxindsz) {
	// 		maxindsz = index.storage_cost();
	// 		biggest_index = &index;
	// 	}

	// cout<<"Biggest Index: \n";
	// cout<<biggest_index->show()<<endl;

	// double maxvtcost=-1;
	// const ViewTuple* biggest_vt=NULL;
	// for(auto& vt: view_tuples_pool)
	// 	if(vt.cost_lb>maxvtcost) {
	// 		maxvtcost = vt.cost_lb;
	// 		biggest_vt = &vt;
	// 	}

	// cout<<"-> Index of Biggest ViewTuple: \n";
	// cout<<biggest_vt->index.show()<<endl;
	// cout<<"Biggest ViewTuple: \n";
	// cout<<biggest_vt->show()<<endl;

}


set<const ViewTuple*> Application::inner_greedy(const Index* index, uint& negc,
	double& cost, const map<const Query*, set<uint>>& rem_goals, uint num_rem_goals,
	const unordered_map<const Query*, Plan>& q2plan, bool oe) const {
	
	auto best_set = set<const ViewTuple*>{};
	double best_cost = 0;  uint best_negc = 0;

	for(uint target=1; target<=num_rem_goals; target++) {
		auto cand_set = set<const ViewTuple*>{}; 
		double cand_cost = index->storage_cost() * wt_storage; uint cand_negc=0; 
		auto cand_rem_goals = rem_goals; // goals yet to be covered by cand
		while(cand_negc<target) {
			const ViewTuple* best_vt=NULL;	double best_vt_cost=-1;  uint best_vt_negc=0;
			for(auto query_setvt: index_to_query2vt.at(index)) 
				for(auto vt: query_setvt.second) {
					double vt_cost = (oe ? q2plan.at(&(vt->query)).time(vt) 
							: vt->cost_lb) * vt->query.weight();
					uint vt_negc = min((uint) (target-cand_negc), 
						set_intersection_size(vt->covered_goals(oe), cand_rem_goals[&(vt->query)]));
					bool replace = ((best_vt==NULL || best_vt_negc==0) ? true: 
						vt_negc==0 ? false: vt_cost/vt_negc < best_vt_cost/best_vt_negc);
					if(replace) {
						best_vt = vt; best_vt_cost = vt_cost; best_vt_negc = vt_negc;
					}
				}
			if(best_vt==NULL || best_vt_negc==0)
				break;
			cand_set.insert(best_vt);
			cand_cost += best_vt_cost;   
			best_vt_negc = set_intersection_size(best_vt->covered_goals(oe), cand_rem_goals[&(best_vt->query)]);
			cand_negc += best_vt_negc;   
			set_difference_inplace(cand_rem_goals[&(best_vt->query)], best_vt->covered_goals(oe));
		}
		if(cand_negc<target)
			break;
		bool replace = (cand_negc==0 ? false : best_negc==0 ? true : cand_cost/cand_negc < best_cost/best_negc);
		if(replace) {
			best_set = cand_set; best_cost = cand_cost; best_negc = cand_negc;
		}
	}
	cost = best_cost; negc = best_negc; 
	return best_set;
}


Application::Design Application::optimize_wsc(const Design& Dinit, bool oe) const {
	set<const Index*> stored_indexes = Dinit.stored_indexes;
	unordered_map<const Query*, Plan> q2plan = Dinit.plans;
	//cout<<"Got input design: "<<Dinit.show()<<endl;
	map<const Query*, set<uint>> rem_goals;
	uint num_rem_goals=0;
	for(auto& query: queries) {
		set<uint> query_goals;
		for(uint i=0; (int) i<query.expression().num_goals(); i++)
			query_goals.insert(i);
		auto plan_goals = q2plan.at(&query).covered_goals(oe);
		rem_goals[&query] = set_difference(query_goals, plan_goals);
		num_rem_goals += rem_goals[&query].size();
	}

	while(num_rem_goals>0) {
		pair<const Index*, set<const ViewTuple*>> best_set;
		double best_cost=0; uint best_negc=0;   // cost and number of extra goals covered by best_set
		for(auto it=index_to_query2vt.begin(); it!=index_to_query2vt.end(); it++) {
			auto index = it->first;
			pair<const Index*, set<const ViewTuple*>> cand_set;
			double cand_cost=0; uint cand_negc = 0;
			if(stored_indexes.find(index)==stored_indexes.end()) 
				cand_set = make_pair(index, inner_greedy(index, cand_negc, cand_cost, 
					rem_goals, num_rem_goals, q2plan, oe));
			else {
				const ViewTuple* best_vt;  uint best_vt_negc=0;  double best_vt_cost=0;
				for(auto query_setvt: it->second) 
					for(auto vt: query_setvt.second) {
						double vt_cost = (oe ? q2plan.at(&(vt->query)).time(vt) 
							: vt->cost_lb) * vt->query.weight();
						double vt_negc = set_intersection_size(vt->covered_goals(oe), rem_goals[&(vt->query)]);
						bool replace=(best_vt_negc==0 ? true : vt_negc==0? false : 
										vt_cost/vt_negc < best_vt_cost/best_vt_negc);
						if(replace) {
							best_vt = vt; best_vt_negc=vt_negc; best_vt_cost=vt_cost;
						} 
					}
				cand_set = make_pair(index, set<const ViewTuple*>{best_vt});
				cand_cost = best_vt_cost;  cand_negc = best_vt_negc;
			}
			bool replace = ((best_set.first==NULL || best_negc==0) ? true: 
				cand_negc==0 ? false : cand_cost/cand_negc < best_cost/best_negc);
			if(replace) {
				best_set = cand_set; best_cost = cand_cost;  best_negc = cand_negc;
			}
		}
		assert(best_negc>0);
		assert(best_set.first!=NULL);
		assert(best_set.second.size()!=0);
		num_rem_goals -= best_negc;
		stored_indexes.insert(best_set.first);
		for(auto vt: best_set.second) {
			set_difference_inplace(rem_goals[&(vt->query)], vt->covered_goals(oe));
			q2plan.at(&(vt->query)).append(*vt);
		}
	}
	return Design(stored_indexes, q2plan);
}

Application::Design::Design(set<const Index*> inds, unordered_map<const Query*, Plan> ps) 
: stored_indexes(inds), plans(ps) {
	cost = 0;
	num_goals_remaining = 0;
	for(auto ind: inds)
		cost += wt_storage * ind->storage_cost();
	for(auto query_plan: ps) {
		cost += query_plan.first->weight() * query_plan.second.current_cost();
		num_goals_remaining += query_plan.first->expression().num_goals();
		num_goals_remaining -= query_plan.second.weakly_covered_goals().size();
	}
}

double Application::Design::lb_cost() const {
	double l_cost = 0;
	for(auto ind: stored_indexes)
		l_cost += wt_storage * ind->storage_cost();
	for(auto query_plan: plans) {
		for(uint i=0; i<query_plan.second.num_stages(); i++)
			l_cost += query_plan.first->weight() * query_plan.second.stage(i)->cost_lb;
	}
	return l_cost;
}

bool Application::Design::iscomplete() const {
	assert(plans.size()>0);
	for(auto it = plans.begin(); it!=plans.end(); it++)
		if(!it->second.iscomplete())
			return false;
	return true;
}

void Application::Design::append(const ViewTuple* vt) {
	num_goals_remaining -= plans.at(&(vt->query)).extra_wc_goals(*vt);
	if(stored_indexes.find(&(vt->index))==stored_indexes.end()) {
		cost += wt_storage * vt->index.storage_cost();
		stored_indexes.insert(&(vt->index));
	}
	cost -= plans.at(&(vt->query)).current_cost();
	plans.at(&(vt->query)).append(*vt);
	cost += plans.at(&(vt->query)).current_cost();
}

string Application::Design::show() const {
	string result = "{\n";
	result+="\tcomplete = " + string(iscomplete() ? "true": "false");
	result += "\n\tcost: "+to_string(cost)+", num_goals_remaining: "+to_string(num_goals_remaining);
	result += "\n\tstored indexes:\n";
	for(auto index: stored_indexes)
		result += index->show(false)+"\n";
	result += "\n\tplans:\n";
	for(auto query_plan: plans) {
		result += "\t\t" + query_plan.first->show() + " -> ";
		result += query_plan.second.show()+"\n";
	}
	result += "}";
	return result;
}

Application::Design Application::get_empty_design() const {
	unordered_map<const Query*, Plan> empty_plans;
	for(auto& query: queries)
		empty_plans.emplace(&query, Plan(query));
	return Design(set<const Index*>(), empty_plans);
}

double Application::ub_cost(const Application::Design& D) const {
	return optimize_wsc(D, true).cost;
}

double Application::lb_cost(const Application::Design& D) const {
	double added_cost=0;
	Design Dp=optimize_wsc(D, false);
	for(auto index: Dp.stored_indexes)
		if(D.stored_indexes.find(index)==D.stored_indexes.end())
			added_cost += wt_storage + index->storage_cost();
	for(auto query_plan: Dp.plans)
		for(uint i=D.plans.at(query_plan.first).num_stages(); 
			i<query_plan.second.num_stages(); i++) {
			added_cost += query_plan.first->weight() * query_plan.second.stage(i)->cost_lb;
		}
	return D.cost + added_cost/approx_factor;
}

double Application::get_priority(const Application::Design& D) const {
	assert(pick_fn<3);
	if(pick_fn==0) return -1*lb_cost(D);
	if(pick_fn==1) return -1*ub_cost(D);
	if(pick_fn==2) return -1*D.num_goals_remaining;
	return 0;
}

list<Application::Design> Application::expand(const Application::Design& D) const {
	if(D.cost>=Lub) return list<Design>();

	list<Design> neighbors;
	priority_queue<pair<double, const Design*>> best_designs;
	for(auto query_ind2vt: query_to_index2vt) {
		auto query = query_ind2vt.first;
		for(auto ind_vt: query_ind2vt.second) {
			for(auto vt: ind_vt.second) {
				if(!D.plans.at(query).has_vt(vt) && !D.plans.at(query).iscomplete()) {
					double new_cost = D.cost;
					if(D.stored_indexes.find(ind_vt.first)==D.stored_indexes.end())
						new_cost += wt_storage * ind_vt.first->storage_cost();
					new_cost += query->weight() * D.plans.at(query).time(vt);
					int extra_cov_goals = D.plans.at(query).extra_wc_goals(*(vt));
					if(new_cost<Lub) {
						neighbors.push_back(D);
						neighbors.back().append(vt);
						best_designs.push(make_pair(-1*neighbors.back().cost/extra_cov_goals, &neighbors.back()));
					}
				}
			}
		}
	}

	if(branch_factor>0) {
		list<Application::Design> filtered_neighbors;
		while(!best_designs.empty() && ((int) filtered_neighbors.size())<branch_factor) {
			filtered_neighbors.push_back(*(best_designs.top().second));
			best_designs.pop();
		}
		return filtered_neighbors;
	}
	return neighbors;
}


typedef pair<double, list<Application::Design>::iterator> score_des;

bool Compare(score_des d1, score_des d2) {
	return d1.first < d2.first;
}

Application::Design Application::optimize_wsc(bool oe) {
	return optimize_wsc(get_empty_design(), oe);
}

Application::Design Application::optimize() {
	//return optimize_wsc(get_empty_design(), true);

	Design Dinit = get_empty_design();
	assert(Dinit.plans.size()>0);
	vector<pair<uint,double>> result;
	cout<<"############ STARTING OPTIMIZE ###############\n";

	priority_queue<score_des, vector<score_des>, function<bool(score_des, score_des)>> F(Compare);

	list<Design> all_designs;
	all_designs.push_back(Dinit);
	F.push(make_pair(1.0, --all_designs.end()));

	Design bestD = Dinit; 
	assert(bestD.plans.size()>0);
	Lub = std::numeric_limits<double>::max();
	uint num_expanded=0;
	while(!F.empty()) {
		//cout<<F.size()<<endl;
		auto topD_it = F.top().second;
		auto& topD = *(F.top().second);
		F.pop();
		if(topD.iscomplete()) {
			if(topD.cost < Lub) {
				Lub = topD.cost;
				bestD = topD;
				cout<<"num_expanded: "<<num_expanded<<", Lub: "<<Lub<<endl;
				result.push_back(make_pair(num_expanded, Lub));
			}
		}
		else {
			auto neighbors = expand(topD);
			num_expanded+=1;
			for(auto& design: neighbors) {
				double cost_lb = lb_cost(design);
				Design ub_D = optimize_wsc(design, true);
				double cost_ub = ub_D.cost;
				if(cost_lb < Lub)  {
					if(cost_ub < Lub) {
						Lub = cost_ub;
						bestD = ub_D;
						cout<<"num_expanded: "<<num_expanded<<", Lub: "<<Lub<<endl;
						result.push_back(make_pair(num_expanded, Lub));
					}
					double priority = -1*cost_lb;
					if(pick_fn==1) priority = -1*cost_ub;
					if(pick_fn==2) priority = -1*design.num_goals_remaining;
					all_designs.push_back(design);
					F.push(make_pair(priority, --all_designs.end()));
				}
			}
		}
		all_designs.erase(topD_it);
		if(max_iters>0)
			if((int)num_expanded>max_iters)
				break;
		cout<<num_expanded<<" "<<F.size()<<endl;
	}
	cout<<"Total number of states expanded: "<<num_expanded<<endl;
	
	return bestD;
}


///////////////////////////////stanalone wsc functions

set<const ViewTuple*> Application::inner_greedy_standalone(const Index* index, uint& negc,
	double& cost, const map<const Query*, set<uint>>& rem_goals, uint num_rem_goals, bool oe) const {
	
	auto best_set = set<const ViewTuple*>{};
	double best_cost = 0;  uint best_negc = 0;

	for(uint target=1; target<=num_rem_goals; target++) {
		auto cand_set = set<const ViewTuple*>{}; 
		double cand_cost = index->storage_cost() * wt_storage; uint cand_negc=0; 
		auto cand_rem_goals = rem_goals; // goals yet to be covered by cand
		while(cand_negc<target) {
			const ViewTuple* best_vt=NULL;	double best_vt_cost=-1;  uint best_vt_negc=0;
			for(auto query_setvt: index_to_query2vt.at(index)) 
				for(auto vt: query_setvt.second) {
					double vt_cost = (oe ? vt->cost_ub : vt->cost_lb) * vt->query.weight();
					uint vt_negc = min((uint) (target-cand_negc), 
						set_intersection_size(vt->covered_goals(oe), cand_rem_goals[&(vt->query)]));
					bool replace = ((best_vt==NULL || best_vt_negc==0) ? true: 
						vt_negc==0 ? false: vt_cost/vt_negc < best_vt_cost/best_vt_negc);
					if(replace) {
						best_vt = vt; best_vt_cost = vt_cost; best_vt_negc = vt_negc;
					}
				}
			if(best_vt==NULL || best_vt_negc==0)
				break;
			cand_set.insert(best_vt);
			cand_cost += best_vt_cost;   
			best_vt_negc = set_intersection_size(best_vt->covered_goals(oe), cand_rem_goals[&(best_vt->query)]);
			cand_negc += best_vt_negc;   
			set_difference_inplace(cand_rem_goals[&(best_vt->query)], best_vt->covered_goals(oe));
		}
		if(cand_negc<target)
			break;
		bool replace = (cand_negc==0 ? false : best_negc==0 ? true : cand_cost/cand_negc < best_cost/best_negc);
		if(replace) {
			best_set = cand_set; best_cost = cand_cost; best_negc = cand_negc;
		}
	}
	cost = best_cost; negc = best_negc; 
	return best_set;
}

Application::Design Application::optimize_wsc_standalone(bool oe) {
	set<const Index*> stored_indexes;
	map<const Query*, set<uint>> rem_goals;
	unordered_map<const Query*, vector<pair<uint, const ViewTuple*>>> q2vts;
	uint num_rem_goals=0;
	for(auto& query: queries) {
		set<uint> query_goals;
		for(uint i=0; (int) i<query.expression().num_goals(); i++)
			query_goals.insert(i);
		rem_goals[&query] = query_goals;
		num_rem_goals += rem_goals[&query].size();
		q2vts[&query] = vector<pair<uint, const ViewTuple*>>();
	}

	while(num_rem_goals>0) {
		pair<const Index*, set<const ViewTuple*>> best_set;
		double best_cost=0; uint best_negc=0;   // cost and number of extra goals covered by best_set
		for(auto it=index_to_query2vt.begin(); it!=index_to_query2vt.end(); it++) {
			auto index = it->first;
			pair<const Index*, set<const ViewTuple*>> cand_set;
			double cand_cost=0; uint cand_negc = 0;
			if(stored_indexes.find(index)==stored_indexes.end()) 
				cand_set = make_pair(index, inner_greedy_standalone(index, cand_negc, cand_cost, 
					rem_goals, num_rem_goals, oe));
			else {
				const ViewTuple* best_vt;  uint best_vt_negc=0;  double best_vt_cost=0;
				for(auto query_setvt: it->second) 
					for(auto vt: query_setvt.second) {
						double vt_cost = (oe ? vt->cost_ub: vt->cost_lb) * vt->query.weight();
						double vt_negc = set_intersection_size(vt->covered_goals(oe), rem_goals[&(vt->query)]);
						bool replace=(best_vt_negc==0 ? true : vt_negc==0? false : 
										vt_cost/vt_negc < best_vt_cost/best_vt_negc);
						if(replace) {
							best_vt = vt; best_vt_negc=vt_negc; best_vt_cost=vt_cost;
						} 
					}
				cand_set = make_pair(index, set<const ViewTuple*>{best_vt});
				cand_cost = best_vt_cost;  cand_negc = best_vt_negc;
			}
			bool replace = ((best_set.first==NULL || best_negc==0) ? true: 
				cand_negc==0 ? false : cand_cost/cand_negc < best_cost/best_negc);
			if(replace) {
				best_set = cand_set; best_cost = cand_cost;  best_negc = cand_negc;
			}
		}
		assert(best_negc>0);
		assert(best_set.first!=NULL);
		assert(best_set.second.size()!=0);
		num_rem_goals -= best_negc;
		stored_indexes.insert(best_set.first);
		for(auto vt: best_set.second) {
			set_difference_inplace(rem_goals[&(vt->query)], vt->covered_goals(oe));
			q2vts[&(vt->query)].push_back(make_pair(vt->first_sc_goal_ind, vt));
		}
	}
	unordered_map<const Query*, Plan> q2plan;
	for(auto& query: queries) {
		sort(q2vts[&query].begin(), q2vts[&query].end());
		q2plan.emplace(&query, Plan(query));
		for(auto& priority_vt: q2vts[&query])
			q2plan.at(&query).append(*(priority_vt.second));
	}

	return Design(stored_indexes, q2plan);
}