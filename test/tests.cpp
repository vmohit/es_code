#include "expression.h"
#include "containment_map.h"
#include "data.h"
#include "dataframe.h"
#include "utils.h"
#include "query.h"
#include "application.h"

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
void test_application();
void test_cost_model();
void test_plan();
void test_utils();

int main() {
	srand (time(NULL));
	// test_utils();
	// test_subcores();
	// test_expression();
	// test_dataframe();
	// test_exp_execution();
	// test_viewtuple_construction();
	// test_application();
	test_cost_model();
	// test_plan();
	return 0;
}

void test_utils() {
	ExtremeFraction frac;
	frac.multiply(1e5);
	frac.divide(1e20);
	frac.multiply(1e10);
	frac.multiply(1e6);
	cout<<frac.eval()<<endl;
	ExtremeFraction frac2;
	frac2.multiply(1e-3);
	frac2.divide(1e-4);
	frac2.multiply(1e2);
	cout<<frac2.eval()<<endl;
	frac2.multiply(frac);
	cout<<frac2.eval()<<endl;

	double nk=1e5, Nk=1e7, Ne=8e6, Nc=1e5, nd=1e5, ne=8e4, nc=10;
	ExtremeFraction x;
	x.multiply(Nk); x.multiply(Ne); x.multiply(Nc);
	x.divide(nk); x.divide(nd); x.divide(ne); x.divide(nd); x.divide(ne); x.divide(nc);
	ExtremeFraction n;
	n.multiply(ne); n.multiply(nd); n.multiply(nc);
	cout<<nk*(1-OneMinusXN(x, n))<<endl;

	n.divide(nc);
	cout<<nc*(1-OneMinusXN(x, n))<<endl;

	n.divide(nd);
	cout<<nd*(1-OneMinusXN(x, n))<<endl;	

	n.divide(ne);
	cout<<ne*(1-OneMinusXN(x, n))<<endl;		
}

void test_expression() {
	cout<<"--------------------Start test_expression()-------------------------\n\n";
	vector<BaseRelation> brs {{"K", {{Dtype::String, "k", 1e5}, {Dtype::Int, "d", 1e5}}, 1e7},
								{"E", {{Dtype::String, "e", 8e4}, {Dtype::Int, "d", 1e5}}, 8e6},
								{"C", {{Dtype::String, "e", 8e4}, {Dtype::String, "c", 10}}, 1e5} };
	for(auto &br: brs)
		cout<<br.show()<<endl;

	map<std::string, const BaseRelation*> name2br {{"K", &brs[0]}, {"E", &brs[1]}, {"C", &brs[2]}};
	string query = "Qent[k1, k2](e) :- K(k1, d); K(k2, d); E(e, d); C(e, str_phone); E(int_3, d); E(int_4, d)";
	Expression expr(query, name2br);
	cout<<expr.show()<<endl;  

	cout<<expr.get_sketch()<<endl;  
	cout<<expr.connected(set<int>{0, 1, 2})<<" "<<expr.connected(set<int>{0, 3})<<endl;
	cout<<expr.subexpression(set<int>{0, 1, 2}).get_sketch()<<endl;
	
	Expression exp1 = expr.subexpression(set<int>{0, 2});
	cout<<exp1.show()<<endl;
	
	Expression exp2 = expr.subexpression(set<int>{1, 4});
	cout<<exp2.show()<<endl;

	cout<<find_containment_map(&exp1, &exp2).show()<<endl;
	cout<<find_containment_map(&exp2, &exp1).show()<<endl;

	cout<<"\n\nTesting join merge of expressions:\n\n";

	vector<BaseRelation> brs_2 {{"K", {{Dtype::Int, "k", 1}, {Dtype::Int, "d", 1}}, 1},
								{"R", {{Dtype::Int, "k", 1}, {Dtype::Int, "d", 1}}, 1},
								{"E", {{Dtype::Int, "e", 1}, {Dtype::Int, "d", 1}}, 1},
								{"C", {{Dtype::Int, "e", 1}, {Dtype::String, "c", 1}}, 1},
								{"T", {{Dtype::Int, "e", 1}, {Dtype::String, "c", 1}}, 1},
								{"F", {{Dtype::Int, "d", 1}, {Dtype::Int, "f", 1}}, 1} };
	for(auto &br: brs_2)
		cout<<br.show()<<endl;

	map<std::string, const BaseRelation*> name2br_2 {{"K", &brs_2[0]}, {"R", &brs_2[1]}, {"E", &brs_2[2]},
													{"C", &brs_2[3]}, {"T", &brs_2[4]}, {"F", &brs_2[5]}};
	string E1 = "E1[](k1, d, e, f1) :- E(int_3, d); K(k1, d); R(k2, d); E(e, d); C(e, str_phone); T(e, str_email); F(d, f1)";
	exp1 = Expression(E1, name2br_2);
	cout<<endl<<exp1.show();
	cout<<exp1.get_join_merge_sketch()<<endl;

	string E2 = "E2[](d, e, c3) :- K(int_1, d); E(int_3, d); R(int_2, d); E(e, d); C(e, c); T(e, c3); F(d, f2)";
	exp2 = Expression(E2, name2br_2);
	cout<<endl<<exp2.show();
	cout<<exp2.get_join_merge_sketch()<<endl;

	if(exp1.get_join_merge_sketch()==exp2.get_join_merge_sketch()) 
		cout<<endl<<exp2.merge_with(exp1).show()
			<<"\nEmpty: "<<exp2.merge_with(exp1).empty()<<endl;
	else
		cout<<"join merge sketches don't match\n";
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
	vector<BaseRelation> brs {{"K", {{Dtype::String, "k", 1e5}, {Dtype::Int, "d", 1e5}}, 1e7},
								{"E", {{Dtype::String, "e", 8e4}, {Dtype::Int, "d", 1e5}}, 8e6},
								{"C", {{Dtype::String, "e", 8e4}, {Dtype::String, "c", 10}}, 1e5} };

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
	vector<BaseRelation> brs {{"K", {{Dtype::String, "k", 1e5}, {Dtype::Int, "d", 1e5}}, 1e7},
								{"E", {{Dtype::String, "e", 8e4}, {Dtype::Int, "d", 1e5}}, 8e6},
								{"C", {{Dtype::String, "e", 8e4}, {Dtype::String, "c", 10}}, 1e5} };
	for(auto &br: brs)
		cout<<br.show()<<endl;

	vector<vector<Data>> ktups, etups, ctups;
	vector<ColumnMetaData> kcolmds {{"k", Dtype::Int}, {"d", Dtype::Int}};
	vector<ColumnMetaData> ecolmds {{"e", Dtype::Int}, {"d", Dtype::Int}};
	vector<ColumnMetaData> ccolmds {{"e", Dtype::Int}, {"c", Dtype::String}};
	vector<DataFrame> dfs{{kcolmds, ktups}, {ecolmds, etups}, {ccolmds, ctups}};
	vector<BaseRelation::Table> tabs;
	for(uint i=0; i<brs.size(); i++) {
		BaseRelation::Table tab(&brs[i], "");
		tab.df = dfs[i];
		tabs.push_back(tab);
	}

	map<const BaseRelation*, const BaseRelation::Table*> br2table;
	for(uint i=0; i<brs.size(); i++) {
		br2table[&brs[i]] = &tabs[i];
	}

	map<std::string, const BaseRelation*> name2br {{"K", &brs[0]}, {"E", &brs[1]}, {"C", &brs[2]}};
	string query_str = "Qent[k1, k2](e, c) :- K(k1, d); K(k2, d); E(e, d); C(e, c); C(e, str_phone)";
	Expression query_expr(query_str, name2br);
	Query query(query_expr, 1, br2table);
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
	vector<BaseRelation> brs {{"car", {{Dtype::Int, "Make", 1}, {Dtype::String, "Dealer", 1}}, 1},
								{"loc", {{Dtype::String, "Dealer", 1}, {Dtype::Int, "City", 1}}, 1},
								{"part", {{Dtype::Int, "Store", 1}, {Dtype::Int, "Make", 1}, {Dtype::Int, "City", 1}}, 1} };
	for(auto &br: brs)
		cout<<br.show()<<endl;

	vector<vector<Data>> ctups, ltups, ptups;
	vector<ColumnMetaData> ccolmds {{"m", Dtype::Int}, {"d", Dtype::String}};
	vector<ColumnMetaData> lcolmds {{"d", Dtype::String}, {"c", Dtype::Int}};
	vector<ColumnMetaData> pcolmds {{"s", Dtype::Int}, {"m", Dtype::Int}, {"c", Dtype::Int}};
	vector<DataFrame> dfs{{ccolmds, ctups}, {lcolmds, ltups}, {pcolmds, ptups}};
	vector<BaseRelation::Table> tabs;
	for(uint i=0; i<brs.size(); i++) {
		BaseRelation::Table tab(&brs[i], "");
		tab.df = dfs[i];
		tabs.push_back(tab);
	}

	map<const BaseRelation*, const BaseRelation::Table*> br2table;
	for(uint i=0; i<brs.size(); i++) {
		br2table[&brs[i]] = &tabs[i];
	}

	map<std::string, const BaseRelation*> name2br {{"car", &brs[0]}, {"loc", &brs[1]}, {"part", &brs[2]}};
	string query_str = "q1[S](C) :- car(M, str_anderson); loc(str_anderson, C); part(S, M, C)";
	Expression query_expr(query_str, name2br);
	Query query(query_expr, 1, br2table);
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

vector<vector<Data>> generate_random_data(int num_tuples,
	vector<int> col_cardinalities) {

	vector<vector<Data>> result;
	for(int i=0; i<num_tuples; i++) {
		vector<Data> row;
		for(uint j=0; j<col_cardinalities.size(); j++)
			row.push_back(rand() % col_cardinalities[j]);
		result.push_back(row);
	}
	return result;
}

void test_application() {
	cout<<"--------------------Start test_candidate_generation()-------------------------\n\n";
	vector<BaseRelation> brs {{"K", {{Dtype::String, "k", 1e5}, {Dtype::Int, "d", 1e5}}, 1e7},
								{"E", {{Dtype::String, "e", 8e4}, {Dtype::Int, "d", 1e5}}, 8e6},
								{"C", {{Dtype::String, "e", 8e4}, {Dtype::String, "c", 10}}, 1e5} };
	cout<<"Base Relations: \n";
	for(auto &br: brs)
		cout<<br.show()<<endl;

	auto ktups = generate_random_data(1000, vector<int>{100, 100});
	auto etups = generate_random_data(800, vector<int>{80, 100}); 
	auto ctups = generate_random_data(160, vector<int>{80, 5});
	vector<ColumnMetaData> kcolmds {{"k", Dtype::Int}, {"d", Dtype::Int}};
	vector<ColumnMetaData> ecolmds {{"e", Dtype::Int}, {"d", Dtype::Int}};
	vector<ColumnMetaData> ccolmds {{"e", Dtype::Int}, {"c", Dtype::Int}};
	vector<DataFrame> dfs{{kcolmds, ktups}, {ecolmds, etups}, {ccolmds, ctups}};
	vector<BaseRelation::Table> tabs;
	for(uint i=0; i<brs.size(); i++) {
		BaseRelation::Table tab(&brs[i], "");
		tab.df = dfs[i];
		tabs.push_back(tab);
	}

	map<const BaseRelation*, const BaseRelation::Table*> br2table;
	for(uint i=0; i<brs.size(); i++) {
		br2table[&brs[i]] = &tabs[i];
	}

	map<std::string, const BaseRelation*> name2br {{"K", &brs[0]}, {"E", &brs[1]}, {"C", &brs[2]}};
	string query = "Qent[k1, k2, c](d, e) :- K(k1, d); K(k2, d); E(e, d); C(e, c)";
	Expression expr(query, name2br);
	cout<<"Query: ";
	Query Q(expr, 1, br2table, 10);
	cout<<Q.show()<<endl;  

	Application app(vector<Query>{Q}, br2table, 1);
	app.show_candidates();
	cout<<app.optimize().show()<<endl;
}



void test_cost_model() {
	cout<<"--------------------Start test_cost_model()-------------------------\n\n";
	vector<BaseRelation> brs {{"K", {{Dtype::String, "k", 1e5}, {Dtype::Int, "d", 1e5}}, 1e7},
								{"E", {{Dtype::String, "e", 8e4}, {Dtype::Int, "d", 1e5}}, 8e6},
								{"C", {{Dtype::String, "e", 8e4}, {Dtype::String, "c", 10}}, 1e5} };
	cout<<"Base Relations: \n";
	for(auto &br: brs)
		cout<<br.show()<<endl;

	map<std::string, const BaseRelation*> name2br {{"K", &brs[0]}, {"E", &brs[1]}, {"C", &brs[2]}};
	string query = "Qent[k1, k2, c](d, e) :- K(k1, d); K(k2, d); E(e, d); C(e, c)";
	Expression expr(query, name2br);
	
	CardinalityEstimator E{expr, {0, 1, 2, 3}};
	vector<string> varnames{"k1", "d", "k2", "e", "c"};
	vector<int> varids;
	for(auto name: varnames)
		varids.push_back(expr.name_to_var(name));
	vector<double> cards = E.get_cardinalities(varids);
	for(uint i=0; i<varids.size(); i++)
		cout<<varnames[i]<<": "<<cards[i]<<", ";
	cout<<endl;

	Index inv{{"INV[k](d) :- K(k, d)", name2br}};
	cout<<inv.storage_cost()<<" "<<inv.avg_block_size()<<endl;

	Index i1{{"INV[](d) :- K(k, d)", name2br}};
	cout<<i1.storage_cost()<<" "<<i1.avg_block_size()<<endl;

	i1 = Index{{"INV[k]() :- K(k, d)", name2br}};
	cout<<i1.storage_cost()<<" "<<i1.avg_block_size()<<endl;

	i1 = Index{{"EINV[k](d, e) :- K(k, d); E(e, d)", name2br}};
	cout<<i1.storage_cost()<<" "<<i1.avg_block_size()<<endl;

	i1 = Index{{"EINV[k, c](d, e) :- K(k, d); E(e, d); C(e, c)", name2br}};
	cout<<i1.storage_cost()<<" "<<i1.avg_block_size()<<endl;

	i1 = Index{{"DINV[d](e, c) :- E(e, d); C(e, c)", name2br}};
	cout<<i1.storage_cost()<<" "<<i1.avg_block_size()<<endl;
}


void test_plan() {
	cout<<"--------------------Start test_subcores()-------------------------\n\n";
	vector<BaseRelation> brs {{"car", {{Dtype::Int, "Make", 1}, {Dtype::String, "Dealer", 1}}, 1},
								{"loc", {{Dtype::String, "Dealer", 1}, {Dtype::Int, "City", 1}}, 1},
								{"part", {{Dtype::Int, "Store", 1}, {Dtype::Int, "Make", 1}, {Dtype::Int, "City", 1}}, 1} };
	for(auto &br: brs)
		cout<<br.show()<<endl;

	vector<vector<Data>> ctups{{Data(11), Data("anderson")}};
	vector<vector<Data>> ltups{{Data("anderson"), Data(12)}}; 
	vector<vector<Data>> ptups{{Data(13), Data(11), Data(12)}};
	vector<ColumnMetaData> ccolmds {{"m", Dtype::Int}, {"d", Dtype::String}};
	vector<ColumnMetaData> lcolmds {{"d", Dtype::String}, {"c", Dtype::Int}};
	vector<ColumnMetaData> pcolmds {{"s", Dtype::Int}, {"m", Dtype::Int}, {"c", Dtype::Int}};
	vector<DataFrame> dfs{{ccolmds, ctups}, {lcolmds, ltups}, {pcolmds, ptups}};
	vector<BaseRelation::Table> tabs;
	for(uint i=0; i<brs.size(); i++) {
		BaseRelation::Table tab(&brs[i], "");
		tab.df = dfs[i];
		tabs.push_back(tab);
	}

	map<const BaseRelation*, const BaseRelation::Table*> br2table;
	for(uint i=0; i<brs.size(); i++) {
		br2table[&brs[i]] = &tabs[i];
	}

	map<std::string, const BaseRelation*> name2br {{"car", &brs[0]}, {"loc", &brs[1]}, {"part", &brs[2]}};
	string query_str = "q1[](S) :- car(M, D); loc(D, C); part(S, M, C)";
	Expression query_expr(query_str, name2br);
	Query query(query_expr, 1, br2table);
	cout<<query.expression().show()<<endl;  

	vector<string> index_strs {
		"v1[M](C) :- car(M, D); loc(D, C)",
		"v2[S](M, C) :- part(S, M, C)",
		"v3[](S) :- car(M, D); loc(D, C); part(S, M, C)",
		"v4[](D, C, S) :- car(M, D); part(S, M, C)",
		"v5[D](S, M) :- loc(D, C); part(S, M, C)"
	};
	Index v1{{index_strs[0], name2br}};
	ViewTuple vt1  = *(query.get_view_tuples(v1).begin());
	cout<<vt1.show()<<endl;

	Index v2{{index_strs[1], name2br}};
	ViewTuple vt2  = *(query.get_view_tuples(v2).begin());
	cout<<vt2.show()<<endl;

	Index v3{{index_strs[2], name2br}};
	ViewTuple vt3  = *(query.get_view_tuples(v3).begin());
	cout<<vt3.show()<<endl;

	Index v4{{index_strs[3], name2br}};
	ViewTuple vt4  = *(query.get_view_tuples(v4).begin());
	cout<<vt4.show()<<endl;

	Index v5{{index_strs[4], name2br}};
	ViewTuple vt5  = *(query.get_view_tuples(v5).begin());
	cout<<vt5.show()<<endl;

	Plan P(query);
	P.append(vt4);
	cout<<"-----------------\n\n";
	// cout<<P.show()<<"\n\n\n\n";
	// P.append(vt3);
	// cout<<"-----------------\n\n";
	cout<<P.show()<<"\n\n\n\n";
	P.append(vt1);
	cout<<"-----------------\n\n";
	cout<<P.show()<<"\n\n\n\n";
	P.append(vt5);
	cout<<"-----------------\n\n";
	cout<<P.show()<<"\n\n\n\n";
	P.append(vt2);
	cout<<"-----------------\n\n";
	cout<<P.show()<<"\n\n\n\n";

}









