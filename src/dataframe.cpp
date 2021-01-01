#include "data.h"
#include "dataframe.h"
#include "utils.h"

#include <map>
#include <set>
#include <utility>
#include <string>
#include <vector>
#include <cassert>
#include <sstream>
#include <iostream>
#include <algorithm>

using std::vector;
using std::map;
using std::string;
using std::set;
using std::cout;
using std::endl;
using std::pair;
using std::sort;
using std::make_pair;
using std::stringstream;
using esutils::set_intersection_inplace;
using esutils::set_intersection;


bool DataFrame::Row::operator<(const DataFrame::Row& r) const {
	if(row.size()!=r.row.size()) return row.size()<r.row.size();
	for(uint i=0; i<row.size(); i++)
		if(row[i]!=r.row[i]) return row[i]<r.row[i];
	return false;
}

bool DataFrame::Row::operator>(const DataFrame::Row& r) const {
	if(row.size()!=r.row.size()) return row.size()>r.row.size();
	for(uint i=0; i<row.size(); i++)
		if(row[i]!=r.row[i]) return row[i]>r.row[i];
	return false;
}

DataFrame::ColumnMetaData::ColumnMetaData(const string& cid_arg, Dtype dtp)
: cid(cid_arg), dtype(dtp) {}

DataFrame::Column::Column(const DataFrame::ColumnMetaData& cmd_arg) 
: cmd(cmd_arg){}

DataFrame::DataFrame(const vector<DataFrame::ColumnMetaData>& colmds, 
	const vector<vector<Data>>& tuples) {
	for(uint i=0; i<colmds.size(); i++) {
		header.push_back(colmds.at(i));
		cols.push_back(Column(colmds.at(i)));
	}
	for(uint i=0; i<header.size(); i++) {
		assert(cid2pos.find(header[i].cid) == cid2pos.end());
		cid2pos[header[i].cid] = i;
	}
	for(const auto& tuple: tuples)
		add_tuple(tuple);
}

void DataFrame::add_tuple(const vector<Data>& tuple) {
	assert(tuple.size() == header.size());
	for(uint i=0; i<header.size(); i++)
		assert(tuple[i].get_dtype() == header[i].dtype);

	rows[maxrowid] = DataFrame::Row();
	rows[maxrowid].row = tuple;
	for(uint i=0; i<header.size(); i++) {
		if(cols[i].val2rowids.find(tuple[i])==cols[i].val2rowids.end())
			cols[i].val2rowids[tuple[i]] = {};
		cols[i].val2rowids[tuple[i]].insert(maxrowid);
	}
	maxrowid += 1;
}

void DataFrame::select(const string& colid, const Data& val) {
	assert(cid2pos.find(colid)!=cid2pos.end());
	set<int> rowids;
	auto& val2rowids = cols[cid2pos[colid]].val2rowids;
	if (val2rowids.find(val) != val2rowids.end()) {
		rowids = val2rowids[val];
	}
	select_rowids(rowids);
	project_out(colid);
}

void DataFrame::select_rowids(const set<int>& rowids) {
	for(uint i=0; i<header.size(); i++) {
		set<Data> erase_vals;
		for(auto &x: cols[i].val2rowids) {
			set_intersection_inplace(x.second, rowids);
			if(x.second.size()==0)
				erase_vals.insert(x.first);
		}
		for(auto val: erase_vals)
			cols[i].val2rowids.erase(val);
	}
	set<int> erase_rowids;
	for(auto &x: rows) {
		if(rowids.find(x.first)==rowids.end())
			erase_rowids.insert(x.first);
	}
	for(auto rowid: erase_rowids)
		rows.erase(rowid);
}

void DataFrame::project_out(const string& colid) {
	assert(cid2pos.find(colid)!=cid2pos.end());
	int pos = cid2pos[colid];
	int lastpos = cols.size()-1;

	if(pos!=lastpos)
		header[pos] = header[lastpos];
	header.pop_back();

	if(pos!=lastpos) 
		cols[pos] = cols[lastpos];
	cols.pop_back();
	cid2pos.erase(colid);
	if(pos!=lastpos)
		cid2pos[cols[pos].cmd.cid] = pos;

	for(auto &ele: rows) {
		if(pos!=lastpos)
			ele.second.row[pos] = ele.second.row[lastpos];
		ele.second.row.pop_back();
	}
}


string DataFrame::self_join(const string& col1, const string& col2) {
	assert(col1!=col2);
	assert(cid2pos.find(col1)!=cid2pos.end());
	assert(cid2pos.find(col2)!=cid2pos.end());
	
	set<int> rowids;
	auto it1 = cols[cid2pos[col1]].val2rowids.begin();
	auto it1end = cols[cid2pos[col1]].val2rowids.end();
	auto it2 = cols[cid2pos[col2]].val2rowids.begin();
	auto it2end = cols[cid2pos[col2]].val2rowids.end();
	while((it1 != it1end) && (it2!=it2end)) {
		if(it1->first == it2->first) {
			for(int rid: set_intersection(it1->second, it2->second))
				rowids.insert(rid);
			it1++;
		}
		else {
			if(it1->first < it2->first)
				it1++;
			else
				it2++;
		}
	}

	select_rowids(rowids);
	project_out(col2);
	return col1;
}


void DataFrame::join(const DataFrame& df, const vector<pair<string, string>>& this2df) {
	assert(this2df.size()>0);
	set<pair<int, int>> final_matches;
	set<string> dfcids;
	for(const auto& ele: this2df)
		dfcids.insert(ele.second);
	for(uint i=0; i<this2df.size(); i++) {
		set<pair<int, int>> matches;
		assert(cid2pos.find(this2df[i].first)!=cid2pos.end());
		assert(df.cid2pos.find(this2df[i].second)!=df.cid2pos.end());
		auto it1 = cols[cid2pos[this2df[i].first]].val2rowids.begin();
		auto it1end = cols[cid2pos[this2df[i].first]].val2rowids.end();
		auto it2 = df.cols.at(df.cid2pos.at(this2df[i].second)).val2rowids.begin();
		auto it2end = df.cols.at(df.cid2pos.at(this2df[i].second)).val2rowids.end();
		while((it1 != it1end) && (it2!=it2end)) {
			if(it1->first == it2->first) {
				for(int rid1: it1->second)
					for(int rid2: it2->second)
						matches.insert(make_pair(rid1, rid2));
				it1++;
			}
			else {
				if(it1->first < it2->first)
					it1++;
				else
					it2++;
			}
		}
		if(i==0) final_matches = matches;
		else set_intersection_inplace(final_matches, matches);
	}


	vector<int> add_col(df.header.size(), 0);
	for(uint i=0; i<df.header.size(); i++) {
		if(dfcids.find(df.header[i].cid) == dfcids.end()) {
			add_col[i] = 1;
			assert(cid2pos.find(df.header[i].cid)==cid2pos.end());
			cid2pos[df.header[i].cid] = header.size();
			header.push_back(df.header[i]);
		}
	}

	vector<vector<Data>> tuples;
	for(auto match: final_matches) {
		auto tuple = rows[match.first].row;
		const auto& dftuple = df.rows.at(match.second).row;
		for(uint i=0; i<add_col.size(); i++) 
			if(add_col[i]==1) 
				tuple.push_back(dftuple.at(i));
		tuples.push_back(tuple);
	}

	cols.clear();
	rows.clear();
	maxrowid = 0;
	
	for(uint i=0; i<header.size(); i++) 
		cols.push_back(ColumnMetaData(header[i]));

	for(const auto& tuple: tuples)
		add_tuple(tuple);
}


string DataFrame::show() const {
	stringstream ss;
	ss<<"Row-ID: ";
	for(uint i=0; i<header.size(); i++) {
		ss << header[i].cid << " ("<<(header[i].dtype==Dtype::Int? "int" : "str");
		if(i+1==header.size())
			ss<<")\n";
		else
			ss<<"), ";
	}
	for(auto it=rows.begin(); it!=rows.end(); it++) {
		ss<<it->first<<": ";
		auto& tuple = it->second;
		for(uint i=0; i<header.size(); i++) {
			if(header[i].dtype==Dtype::Int)
				ss << tuple.row[i].get_int_val();
			if(header[i].dtype==Dtype::String)
				ss << tuple.row[i].get_str_val(); 
			ss << (i+1==header.size() ? "\n" : ", ");
		}
	}
	return ss.str();
}


void DataFrame::prepend_to_cids(const std::string& prefix) {
	map<string, int> new_cid2pos;
	for(uint i=0; i<header.size(); i++) {
		new_cid2pos[prefix + "_" + header[i].cid] = 
			cid2pos[header[i].cid];
		header[i].cid = prefix + "_" + header[i].cid;
		cols[i].cmd.cid = prefix + "_" + cols[i].cmd.cid;
	}
	cid2pos = new_cid2pos;
}

const vector<DataFrame::ColumnMetaData>& DataFrame::get_header() const {
	return header;
}

vector<vector<Data>> DataFrame::get_rows() const {
	vector<vector<Data>> result;
	for(auto item: rows)
		result.push_back(item.second.row);
	return result;
}

int DataFrame::get_cid2pos(const string& cid) const {
	return cid2pos.at(cid);
}

set<vector<Data>> DataFrame::get_unique_rows() const {
	set<Row> result;
	for(auto& item: rows)
		result.insert(item.second);
	set<vector<Data>> ret_result;
	for(auto& item: result)
		ret_result.insert(item.row);
	return ret_result;
}

int DataFrame::num_unique_rows() const {
	set<Row> result;
	for(auto& item: rows)
		result.insert(item.second);
	return result.size();
}

vector<vector<Data>> DataFrame::get_sorted_rows(set<string> prefix_cids) const {
	vector<pair<int, pair<int, int>>> priority_card_cpos;
	for(auto it=cid2pos.begin(); it!=cid2pos.end(); it++) {
		auto cid = it->first;
		auto pos = it->second;
		if(prefix_cids.find(cid)==prefix_cids.end()) 
			priority_card_cpos.push_back(make_pair(2, 
				make_pair(cols.at(pos).val2rowids.size(), pos)));
		else 
			priority_card_cpos.push_back(make_pair(1, 
				make_pair(cols.at(pos).val2rowids.size(), pos)));
	}

	sort(priority_card_cpos.begin(), priority_card_cpos.end());

	vector<Row> new_rows;
	for(auto it=rows.begin(); it!=rows.end(); it++) {
		const auto& row = it->second;
		new_rows.push_back(Row());
		for(auto ele: priority_card_cpos)
			new_rows.back().row.push_back(row.row.at(ele.second.second));
	}
	sort(new_rows.begin(), new_rows.end());
	vector<vector<Data>> result;
	for(auto& row: new_rows)
		result.push_back(row.row);
	return result;
}