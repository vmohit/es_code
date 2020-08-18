#ifndef APPLICATION_H
#define APPLICATION_H

#include "query.h"
#include "expression.h"
#include "rewriting.h"

#include <map>
#include <vector>
#include <string>
#include <list>

class Application {
	uint K = 3;
	std::list<BaseRelation> base_rels;
	std::list<QueryTemplate> workload;
	std::list<Index> cand_indexes;
	std::map<const Index*, std::list<ViewTuple>> cand_viewtups;

	void generate_subexpressions();
	void merge_indexes();
public:
	
	Application(std::vector<BaseRelation> brels, std::vector<QueryTemplate> wk);
	void generate_candidates();
	std::string show_candidates() const;
};

#endif