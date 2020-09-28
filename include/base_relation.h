#ifndef BASERELATION_H
#define BASERELATION_H

#include "data.h"

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
	};

private:
	static int maxid;

	// metadata
	int id;
	std::string name;
	std::vector<Column> columns; 
	
public:
	//BaseRelation(int numtups);
	BaseRelation(const std::string& nm, const std::vector<Column>& cols);
	int get_num_cols() const;
	int get_id() const;
	std::string get_name() const;
	Dtype dtype_at(int col) const;
	std::string show() const;
};

#endif