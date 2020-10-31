#include "expression.h"
#include "containment_map.h"
#include "data.h"
#include "dataframe.h"
#include "utils.h"
#include "query.h"

#include <tuple>
#include <iostream>
#include <string>
#include <map>
#include <set>
#include <vector>
#include <cstdlib>
#include <ctime>

using namespace std;
using namespace esutils;
typedef Expression::Symbol Symbol;
typedef DataFrame::ColumnMetaData ColumnMetaData;

void test_expression();
void test_dataframe();
void test_exp_execution();
void test_viewtuple_construction();
void test_subcores();

int main() {
	test_expression();
	test_dataframe();
	test_exp_execution();
	test_viewtuple_construction();
	test_subcores();
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
	vector<ColumnMetaData> colmds {{"k", Dtype::String}, {"d", Dtype::Int}, {"p", Dtype::Int}};
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

void test_exp_execution() {
	cout<<"--------------------Start test_exp_execution()-------------------------\n\n";
	vector<BaseRelation> brs {{"K", {{Dtype::Int, "k"}, {Dtype::Int, "d"}}},
								{"E", {{Dtype::Int, "e"}, {Dtype::Int, "d"}}},
								{"C", {{Dtype::Int, "e"}, {Dtype::Int, "c"}}} };

	srand (time(NULL));

	BaseRelation::Table kd(&brs[0], "kd");
	int Nk=3, Nd=3, Nkd=6;
	for(int i=0; i<Nkd; i++)
		kd.df.add_tuple(vector<Data>{rand()%Nk+1, 10+rand()%Nd+1});
	cout<<kd.df.show()<<endl;

	BaseRelation::Table ed(&brs[1], "ed");
	int Ne=3, Ned=5;
	for(int i=0; i<Ned; i++)
		ed.df.add_tuple(vector<Data>{100+rand()%Ne+1, 10+rand()%Nd+1});
	cout<<ed.df.show()<<endl;

	BaseRelation::Table ec(&brs[2], "ec");
	int Nc=2, Nec=4;
	for(int i=0; i<Nec; i++)
		ec.df.add_tuple(vector<Data>{100+rand()%Ne+1, 1000+rand()%Nc+1});
	cout<<ec.df.show()<<endl;

	map<const BaseRelation*, const BaseRelation::Table*> br2table {
		{&brs[0], &kd}, {&brs[1], &ed}, {&brs[2], &ec}
	};

	map<std::string, const BaseRelation*> name2br {{"K", &brs[0]}, {"E", &brs[1]}, {"C", &brs[2]}};
	string query = "Qent[k1, k2](e, d, c) :- K(k1, d); K(k2, d); E(e, d); C(e, c)";
	Expression expr(query, name2br);
	cout<<expr.show()<<endl;  

	Expression::Table table(&expr, br2table);
	for(auto item: table.headvar2cid) {
		cout<<expr.var_to_name(item.first)<<": "<<item.second<<", ";
	}
	cout<<endl;
	cout<<table.df.show()<<endl;
}

void test_viewtuple_construction() {
	cout<<"--------------------Start test_viewtuple_construction()-------------------------\n\n";
	vector<BaseRelation> brs {{"K", {{Dtype::Int, "k"}, {Dtype::Int, "d"}}},
								{"E", {{Dtype::Int, "e"}, {Dtype::Int, "d"}}},
								{"C", {{Dtype::Int, "e"}, {Dtype::String, "c"}}} };
	for(auto &br: brs)
		cout<<br.show()<<endl;

	map<std::string, const BaseRelation*> name2br {{"K", &brs[0]}, {"E", &brs[1]}, {"C", &brs[2]}};
	string query_str = "Qent[k1, k2](e, c) :- K(k1, d); K(k2, d); E(e, d); C(e, c); C(e, str_phone)";
	Expression query_expr(query_str, name2br);
	Query query(query_expr);
	cout<<query.expression().show()<<endl;  

	string einv_str = "EINV[k1, k2](e, c) :- K(k1, d); K(k2, d); E(e, d); C(e, c)";
	Expression einv_expr(einv_str, name2br);
	Index einv(einv_expr);
	cout<<einv.expression().show()<<endl; 

	for(auto vt: query.get_view_tuples(einv))
		cout<<vt.show()<<endl;
}

void test_subcores() {
	cout<<"--------------------Start test_subcores()-------------------------\n\n";
	vector<BaseRelation> brs {{"car", {{Dtype::Int, "Make"}, {Dtype::String, "Dealer"}}},
								{"loc", {{Dtype::String, "Dealer"}, {Dtype::Int, "City"}}},
								{"part", {{Dtype::Int, "Store"}, {Dtype::Int, "Make"}, {Dtype::Int, "City"}}} };
	for(auto &br: brs)
		cout<<br.show()<<endl;

	map<std::string, const BaseRelation*> name2br {{"car", &brs[0]}, {"loc", &brs[1]}, {"part", &brs[2]}};
	string query_str = "q1[S](C) :- car(M, str_anderson); loc(str_anderson, C); part(S, M, C)";
	Expression query_expr(query_str, name2br);
	Query query(query_expr);
	cout<<query.expression().show()<<endl;  

	vector<string> index_strs {
		"v1[M](D, C) :- car(M, D); loc(D, C)",
		"v2[S](M, C) :- part(S, M, C)",
		"v3[](S) :- car(M, str_anderson); loc(str_anderson, C); part(S, M, C)",
		"v4[M](D, C, S) :- car(M, D); loc(D, C); part(S, M, C)",
		"v5[D](S, C) :- car(M, D); part(S, M, C)"
	};
	for(string index_str: index_strs) {
		Expression index_expr(index_str, name2br);
		Index index(index_expr);
		cout<<"Index: ";
		cout<<index.expression().show()<<endl<<endl; 

		for(auto vt: query.get_view_tuples(index))
			cout<<vt.show()<<endl;
		cout<<endl<<endl;
	}
}


















