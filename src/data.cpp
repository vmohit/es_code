#include "data.h"

#include <string>
#include <vector>
#include <cstdlib>
#include <cstdio>

using std::string;
using std::vector;
using std::to_string;


// Constructors

Data::Data(const std::string& val) : 
dtype(Dtype::String), str_val(val), int_val(0) {}

Data::Data(const int& val) :
dtype(Dtype::Int), str_val(""), int_val(val) {}


string Data::show(int verbose /*=0*/) const{
	string result = "";
	switch (dtype) { 
	case Dtype::String: 
		result.append("str_"+str_val);
		break; 
	case Dtype::Int: 
		result.append("int_"+to_string(int_val));
		break;
	default:
		perror("ERROR: unsupported datatype");   
	}
	return result;
}

// getters and setters

Dtype Data::get_dtype() const{
	return dtype;
}

int Data::get_int_val() const{
	return int_val;
}

string Data::get_str_val() const{
	return str_val;
}

// operators

bool Data::operator!=(const Data& dt) const {
	if(dtype!=dt.dtype)
		return true;
	else
		if(dtype==Dtype::Int)
			return int_val!=dt.int_val;
		else
			return ((int_val!=dt.int_val) || (str_val.compare(dt.str_val)!=0));
}

/**Shorter tuples are smaller and Strings are smaller than ints.*/
bool Data::operator<(const Data& dt) const {
	if(dtype!=dt.dtype)
		return (dtype==Dtype::String ? true : false);
	else
		if(dtype==Dtype::Int)
			return int_val<dt.int_val;
		else
			return (str_val.compare(dt.str_val)<0);
}

bool Data::operator>(const Data& dt) const {
	if(dtype!=dt.dtype)
		return (dtype==Dtype::String ? false : true);
	else
		if(dtype==Dtype::Int)
			return int_val>dt.int_val;
		else
			return (str_val.compare(dt.str_val)>0);
}

bool Data::operator==(const Data& dt) const {
	return !(this->operator!=(dt));
}