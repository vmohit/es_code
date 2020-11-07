#ifndef DATAFRAME_H
#define DATAFRAME_H

#include "data.h"
#include <map>
#include <set>
#include <vector>
#include <utility>

/** Class for executing queries on sampled dataset for cost estimation */
class DataFrame {
public:
	struct ColumnMetaData {
		std::string cid; //!< unique column identifier passed by a client
		Dtype dtype;
		ColumnMetaData(const std::string& cid_arg, Dtype dtp);
	};
private:
	struct Column {
		ColumnMetaData cmd;
		std::map<Data, std::set<int>> val2rowids;
		Column(const ColumnMetaData& cmd_arg);
	};
	struct Row {
		std::vector<Data> row;
		bool operator<(const Row& r) const;
		bool operator>(const Row& r) const;
	};
	std::vector<ColumnMetaData> header;
	std::map<std::string, int> cid2pos;
	std::vector<Column> cols;
	std::map<int, Row> rows;
	int maxrowid = 0;

	void select_rowids(const std::set<int>& rowids);
public:
	DataFrame(const std::vector<ColumnMetaData>& colmds, const std::vector<std::vector<Data>>& tuples);
	void add_tuple(const std::vector<Data>& tuple);

	void select(const std::string& colid, const Data& val);  //!< projects out the colid after performing selection
	void project_out(const std::string& cid);
	std::string self_join(const std::string& col1, const std::string& col2);  //!< returns the colid of joined result and projects out the redundant column
	void join(const DataFrame& df, const std::vector<std::pair<std::string, std::string>>& this2df);  //!< joins df into this dataframe. Matched columns of df will be projected out in the output
	
	void prepend_to_cids(const std::string& prefix); //!< adds the given prefix to ids of each column
	const std::vector<ColumnMetaData>& get_header() const;  //!< return a read-only copy of header
	
	std::vector<std::vector<Data>> get_rows() const;
	int get_cid2pos(const std::string& cid) const;

	std::set<std::vector<Data>> get_unique_rows() const;

	std::string show() const;  //!< show the dataframe for debugging
};

#endif