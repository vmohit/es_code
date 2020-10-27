#include "base_relation.h"
#include "data.h"

#include <string>
#include <vector>
#include <set>
#include <map>

using std::string;
using std::vector;
using std::map;
using std::to_string;
 
typedef DataFrame::ColumnMetaData ColumnMetaData;


// functions of class BaseRelation
int BaseRelation::maxid=0;

BaseRelation::BaseRelation(const string& nm, const vector<Column>& cols) : 
id(0), name(nm), columns(cols) {
	id = maxid++;
}

int BaseRelation::get_num_cols() const {
	return columns.size();
}

int BaseRelation::get_id() const {
	return id;
}

string BaseRelation::get_name() const {
	return name;
}

Dtype BaseRelation::dtype_at(int pos) const {
	return columns[pos].dtype;
}

string BaseRelation::show() const {
	string result = "[id: "+to_string(id)+"] "+name+"(";
	for(uint col=0; col<columns.size(); col++) {
		result += (columns[col].dtype==Dtype::Int ? "Int_": "String_") + columns[col].name;
		if(col+1!=columns.size())
			result +=", ";
	}
	return result+")";
}

vector<ColumnMetaData> BaseRelation::get_colmds(const string& cid_prefix) const {
	vector<ColumnMetaData> result;
	for(uint i=0; i<columns.size(); i++)
		result.push_back(ColumnMetaData(cid_prefix+"_"+to_string(i), columns.at(i).dtype));
	return result;
}

BaseRelation::Table::Table(const BaseRelation* br_arg, const std::string& cid_prefix) :
br(br_arg), df(br_arg->get_colmds(cid_prefix), vector<vector<Data>>()) {}