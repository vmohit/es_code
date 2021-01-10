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
#include <list>

using std::vector;
using esutils::generate_subsets;
using std::cout;
using std::endl;
using std::list;
using std::map;

// initializing parameters
int Application::max_iters=1000;
double Application::max_index_size=1000;
double Application::max_vt_lb=1000000;
double Application::wt_storage=1;


Application::Application(const vector<Query>& workload, const std::map<const BaseRelation*, 
		const BaseRelation::Table*> br2table, int k) {
	for(auto& query: workload)
		queries.push_back(query);
	max_num_goals_index = k;
	generate_candidates(br2table);
}

void Application::generate_candidates(const map<const BaseRelation*, const BaseRelation::Table*> br2table) {
	// obtain complete plans for each query to estimate lower bound time estimates for view tuples
	map<const Query*, Plan> complete_plans;
	for(auto& query: queries) {
		Index index(query.expression(), br2table);
		Plan P(query);
		bool found_vt=false;
		for(auto& vt: query.get_view_tuples(index)) {
			int num_goals=0;
			for(auto& sub_core: vt.subcores)
				num_goals += sub_core.size();
			if(num_goals==query.expression().num_goals()) {
				P.append(vt);
				found_vt=true;
				break;
			}
		}
		assert(found_vt);
		complete_plans.emplace(&query, P);
	}

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
						indexes.push_back(Index(exp, br2table));
				}
			}
		}
	}

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
						indexes.push_back(Index(exp, br2table));
				}
			}
		}
		it++;
	}

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
					indexes.push_back(Index(expr, br2table));
			}
		}
	}

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
					indexes.push_back(Index(expr, br2table));
			}
		}
	}

	// remove indexes that are too big
	list<Index> feasible_indexes;
	for(auto& index: indexes)
		if(index.storage_cost()<max_index_size)
			feasible_indexes.push_back(index);
	indexes = feasible_indexes;

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