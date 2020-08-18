#ifndef BASERELATION_H
#define BASERELATION_H

#include "data.h"
#include "tuple.h"

#include <vector>
#include <set>
#include <map>
#include <string>

// class DataFrame {
// 	std::vector<Dtype> dtypes;
// 	std::multiset<Tuple> data;
// public:
// 	DataFrame(const std::vector<Dtype>& dtps, const std::multiset<Tuple>& dt);
// 	DataFrame join(const DataFrame& rhs, const std::map<uint, uint> l2r) const;
// 	void filter(const map<uint, uint>& equalities, const map<uint, Data>& selections);
	
// };

/**Class to create and represent base relations that contain the data. This class is not supposed to be  */
class BaseRelation {
	static int maxid;
	//int maxid = 0;
	// metadata
	int id;
	std::string name;
	std::vector<std::string> col_names; 
	std::vector<Dtype> col_dtypes;
	
	// stats
	std::vector<int> col_domsizes;
	std::vector<int> col_remcounts;  //!< total count of all elements not in col_counts
	int N;
	std::vector<std::map<Data, int>> col_counts;
public:
	//BaseRelation(int numtups);
	BaseRelation(const std::string& nm, const std::vector<std::string>& cns, const std::vector<Dtype>& cds,
		const std::vector<int>& domsizes, int numtups, const std::vector<std::map<Data, int>>& counts);
	int get_num_cols() const;
	int get_id() const;
	std::string get_name() const;
	Dtype dtype_at(int col) const;
	int get_domsize(int col) const;
	int num_tuples() const;
	float get_count(int col) const;  //!< expected count of an arbitrary constant picked uniformaly at random from the domain
	float get_count(int col, const Data& dt) const;  //!< count of a given constant. Use col_counts to find it.
	std::string show() const;
};

#endif