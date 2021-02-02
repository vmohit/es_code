#ifndef BASERELATION_H
#define BASERELATION_H

#include "data.h"
#include "dataframe.h"

#include <vector>
#include <set>
#include <map>
#include <string>


/**Class to create and represent base relations that contain the data. This class is not supposed to be  */
class BaseRelation {
public:
	struct Column {
		Dtype dtype;
		std::string name;
		double cardinality;
	};

	/**A dataframe that is tied together to a base relation*/
	struct Table {
		const BaseRelation* br;
		DataFrame df;
		//!< creates an empty dataframe with the same column data types as br_args and column ids as "<cid_prefix>_<col no.>"
		Table(const BaseRelation* br_arg, const std::string& cid_prefix); 
	};
private:
	static int maxid;

	// metadata
	int id;
	std::string name;
	std::vector<Column> columns; 
	double numtups;
public:
	BaseRelation(const std::string& nm, const std::vector<Column>& cols, double numtuples);
	int get_num_cols() const;
	int get_id() const;
	std::string get_name() const;
	Dtype dtype_at(int col) const;
	double card_at(int col) const;
	double num_tuples() const;
	std::string show() const;

private:
	std::vector<DataFrame::ColumnMetaData> get_colmds(const std::string& cid_prefix) const;
};

#endif