#include "base_relation.h"

#include <string>
#include <vector>
#include <set>
#include <map>

using std::string;
using std::vector;
using std::map;
using std::to_string;



// functions of class BaseRelation
int BaseRelation::maxid=0;

BaseRelation::BaseRelation(const string& nm, const vector<string>& cns, const vector<Dtype>& cds,
	const vector<int>& domsizes, int numtups, const vector<map<Data, int>>& counts) : 
id(0), name(nm), col_names(cns), col_dtypes(cds), col_domsizes(domsizes),
col_remcounts(cns.size(), numtups), N(numtups), col_counts(counts) {
	
	assert(cns.size()>0);
	assert(cns.size()==cds.size());
	id = maxid++;

	assert(domsizes.size()==counts.size());
	assert(domsizes.size()==cds.size());
	for(uint col=0; col<cns.size(); col++) {
		assert(col_domsizes[col] >= (int) counts[col].size());
		for(auto it=counts[col].begin(); it!=counts[col].end(); it++) {
			assert(it->first.get_dtype()==col_dtypes[col]);
			assert(it->second > 0);
			col_remcounts[col] -= it->second;
		}
	}
}

int BaseRelation::get_num_cols() const {
	return col_names.size();
}

int BaseRelation::get_id() const {
	return id;
}

string BaseRelation::get_name() const {
	return name;
}

Dtype BaseRelation::dtype_at(int pos) const {
	return col_dtypes[pos];
}

int BaseRelation::get_domsize(int col) const {
	return col_domsizes[col];
}

int BaseRelation::num_tuples() const {
	return N;
}

float BaseRelation::get_count(int col) const {  
	return ((float) N) / col_domsizes[col];
}

float BaseRelation::get_count(int col, const Data& dt) const {
	if(dt.get_dtype()!=col_dtypes[col])
		return 0;
	if(col_counts[col].find(dt)!=col_counts[col].end())
		return col_counts[col].at(dt);
	return ((float) col_remcounts[col]) / (col_domsizes[col] - col_counts[col].size());
}

string BaseRelation::show() const {
	string result = "[id: "+to_string(id)+"] "+name+"(";
	for(uint col=0; col<col_names.size(); col++) 
		result += (col_dtypes[col]==Dtype::Int ? "Int_": "String_") + col_names[col]+", ";
	return result+")";
}