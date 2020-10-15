#include "expression.h"
#include "containment_map.h"
#include "data.h"
#include "dataframe.h"
#include "utils.h"

#include <tuple>
#include <iostream>
#include <string>
#include <map>
#include <set>
#include <vector>

using namespace std;
using namespace esutils;
typedef Expression::Symbol Symbol;

void test_expression();
void test_dataframe();

int main() {
	cout<<"Testing expression: \n\n";
	test_expression();
	//test_dataframe();
	return 0;
}

void test_expression() {
	cout<<"--------------------Start test_expression()-------------------------\n\n";
	vector<BaseRelation> brs {{"K", {{Dtype::Int, "k"}, {Dtype::Int, "d"}}},
								{"E", {{Dtype::Int, "e"}, {Dtype::Int, "d"}}},
								{"C", {{Dtype::Int, "e"}, {Dtype::String, "c"}}} };
	for(auto &br: brs)
		cout<<br.show()<<endl;

	map<std::string, const BaseRelation*> name2br {{"K", &brs[0]}, {"E", &brs[1]}, {"C", &brs[2]}};
	string query = "Qent[k1, k2](d, e) :- K(k1, d); K(k2, d); E(e, d); C(e, str_phone); E(int_3, d); E(int_4, d)";
	Expression expr(query, name2br);
	cout<<expr.show()<<endl;  
	
	Expression exp1 = expr.subexpression(set<int>{0, 2});
	cout<<exp1.show()<<endl;
	
	Expression exp2 = expr.subexpression(set<int>{1, 4});
	cout<<exp2.show()<<endl;

	cout<<find_containment_map(&exp1, &exp2).show()<<endl;
	cout<<find_containment_map(&exp2, &exp1).show()<<endl;
}

void test_dataframe() {
	cout<<"--------------------Start test_dataframe()-------------------------\n\n";
	vector<string> docs {{"long red ferrari on long road"}, {"long blue ferrari in long island"}};
	vector<DataFrame::ColumnMetaData> colmds {{"k", Dtype::String}, {"d", Dtype::Int}, {"p", Dtype::Int}};
	vector<vector<Data>> tuples;
	int did = 0;
	for(auto doc: docs) {
		auto tokens = split(doc, " ");
		for(uint i=0; i<tokens.size(); i++)
			tuples.push_back(vector<Data>{Data(tokens[i]), Data(did), Data(i)});
		did += 1;
	}
	DataFrame df(colmds, tuples);
	DataFrame df1 = df;
	cout<<df.show()<<endl;

	df.select("k", Data("long"));
	cout<<df.show()<<endl;

	df.self_join("p", "d");
	cout<<df.show()<<endl;

	vector<DataFrame::ColumnMetaData> colmds2 {{"k_2", Dtype::String}, {"d_2", Dtype::Int}, {"p_2", Dtype::Int}};
	DataFrame df2(colmds2, tuples);

	df1.join(df2, vector<pair<string, string>>{{"k", "k_2"}, {"p", "p_2"}});
	cout<<df1.show()<<endl;
}