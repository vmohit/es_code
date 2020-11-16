#include "application.h"
#include "query.h"
#include "utils.h"
#include "containment_map.h"
#include "expression.h"

#include <vector>
#include <iostream>
#include <set>
#include <utility>

using std::vector;
using esutils::generate_subsets;
using std::cout;
using std::endl;

Application::Application(const vector<Query>& workload, int k) {
	for(auto& query: workload)
		queries.push_back(query);
	max_num_goals_index = k;
	generate_candidates();
}

void Application::generate_candidates() {
	// generate all subexpressions of every query to get an initial set of candidate index
	for(auto& query: queries) {
		for(int k=1; k<=max_num_goals_index && k<=query.expression().num_goals(); k++) {
			for(auto& subset: generate_subsets(
				query.expression().num_goals(), k)) {
				if(query.expression().connected(subset)) {
					Index ind(query.expression().subexpression(subset));
					bool match=false;
					for(auto& index: indexes) {
						if(index.expression().get_sketch()
							==ind.expression().get_sketch()) {
							if(isomorphic(&index.expression(), &ind.expression())) {
								match = true;
								break;
							}
						}
					}
					if(!match)
						indexes.push_back(ind);
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
						indexes.push_back(Index(exp));
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
					indexes.push_back(Index(expr));
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
					indexes.push_back(Index(expr));
			}
		}
	}

	// generate view tuples for all the candidates index generated 
	for(auto& query: queries) {
		for(auto& index: indexes) {
			for(auto& vt: query.get_view_tuples(index)) {
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

void Application::show_candidates() const {
	int pos=1;
	for(const auto& index: indexes) {
		cout<<"Index: "<<pos++<<endl;
		cout<<index.expression().show()<<endl;

		int pos_q=1;
		for(auto item: index_to_query2vt.at(&index)) {
			cout<<"\tQuery: "<<pos_q++<<endl;
			cout<<"\t"<<item.first->expression().show()<<endl;

			for(auto vt: item.second) {
				cout<<"\t\t"<<vt->show()<<endl;
			}
		} 
	}
	cout<<indexes.size()<<" indexes\n";
	cout<<view_tuples_pool.size()<<" view tuples\n";

}