#include "application.h"
#include "utils.h"
#include "query.h"
#include "rewriting.h"

#include <vector>
#include <string>
#include <set>
#include <cassert>
#include <list>
#include <iostream>

using std::vector;
using std::set;
using std::string;
using std::to_string;
using esutils::generate_subsets;
using std::list;
using std::cout;
using std::endl;

Application::Application(std::vector<BaseRelation> brels, std::vector<QueryTemplate> wk) {
	for(auto& br: brels)
		base_rels.push_back(br);
	float sumwt=0;
	for(auto& query: wk) {
		// cout<<query.get_weight()<<"      here\n\n";
		assert(query.get_weight()>=0 && query.get_weight()<=1);
		workload.push_back(query);
		sumwt+=query.get_weight();
	}
	assert(sumwt>=0.99 && sumwt<=1.01);
}

void Application::generate_subexpressions() {
	for(const auto& query : workload) {
		for(uint k=1; k<=K && k<=query.get_expr().num_goals(); k++) {
			for(auto& subset: generate_subsets(query.get_expr().num_goals(), k)) {
				if(query.connected(subset)) {
					Expression subexpr = query.get_expr().subexpression(subset);
					bool matched = false;
					for(const auto& index : cand_indexes) 
						if(index.get_expr().num_goals()==subexpr.num_goals()) 
							if(index.get_expr().get_signature()==subexpr.get_signature()) {
								auto subexpr2index = index.get_expr().isomorphic_match(subexpr);
								if(subexpr2index.size()!=0) {
									cand_viewtups[&index].push_back(ViewTuple(&query, &index, subset, subexpr2index));
									matched = true;
									break;
								}
							}
					if(!matched) {
						cand_indexes.push_back(Index(subexpr));
						cand_viewtups[&cand_indexes.back()] = list<ViewTuple>{ViewTuple(&query, 
							&cand_indexes.back(), subset, 
							cand_indexes.back().get_expr().isomorphic_match(cand_indexes.back().get_expr()))};
					}
				}
			}
		}
	}
}

void Application::merge_indexes() {

}

void Application::generate_candidates() {
	generate_subexpressions();
	merge_indexes();
	// remove_attributes();
	// rearrange_columns();
}

string Application::show_candidates() const {
	string result = "WORKLOAD:\n";
	for(const auto& query: workload) 
		result += "(queryid: "+to_string(query.get_id())+") "+query.get_expr().show();
	result += "\n\nCANDIDATES:\n";
	for(const auto& index: cand_indexes) {
		result += "(indexid: "+to_string(index.get_id())+") "+index.get_expr().show();
		result += "\n\tViewTuples:\n";
		for(const auto& vt: cand_viewtups.at(&index)) 
			result += "\t"+vt.show();
		result += "\n";
	}
	return result;
}