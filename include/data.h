#ifndef DATA_H
#define DATA_H

#include <string>
#include <vector>

// --------------------------------------------------------------------------

/** Data types supported by the system. */
enum class Dtype { String, Int }; 

/** Class for storing cell data. Datatypes string and int are supported */
class Data {
	Dtype dtype;
	std::string str_val="";
	int int_val;

public:
	// Constructors
	Data(const std::string& val);
	Data(const int& val);

	// Utility functions
	std::string show(int verbose=0) const;

	// getters and setters
	Dtype get_dtype() const;
	int get_int_val() const;
	std::string get_str_val() const;

	// Operators
	bool operator!=(const Data& dt) const;
	bool operator<(const Data& dt) const;
	bool operator==(const Data& dt) const;
};

#endif