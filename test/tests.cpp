#include "expression.h"
#include "application.h"
#include "query.h"
#include "utils.h"
#include "cost_model.h"

#include <tuple>
#include <iostream>
#include <string>
#include <map>
#include <set>
#include <vector>

using namespace std;
using esutils::oto_map;

void test_baserelation();
void test_expression();
void test_query();
void test_index();
void test_cost_model();
void test_viewtuple();
void test_utils();
void test_candidate_generation();

int main() {
	cout<<"Testing base relations: \n\n";
	test_baserelation();
	cout<<"\n\n\n-------------------------\n\n\n\n";
	
	cout<<"Testing expression: \n\n";
	test_expression();
	cout<<"\n\n\n-------------------------\n\n\n\n";

	cout<<"Testing query: \n\n";
	test_query();
	cout<<"\n\n\n-------------------------\n\n\n\n";
	
	cout<<"Testing utils:\n\n";
	test_utils();
	cout<<"\n\n\n-------------------------\n\n\n\n";

	cout<<"Testing cost model: \n\n";
	test_cost_model();
	cout<<"\n\n\n-------------------------\n\n\n\n";

	cout<<"Testing indexL \n\n";
	test_index();
	cout<<"\n\n\n-------------------------\n\n\n\n";

	cout<<"Testing candidate generation: \n\n";
	test_candidate_generation();
	cout<<"\n\n\n-------------------------\n\n\n\n";

	return 0;
}

void test_baserelation() {
	/* An example baserelation
	3 columns 'customers', 'order_number', and 'product_id' with datatypes string, int and int. Example data
	"a", 1, 2
	"a", 1, 3
	"a", 2, 1
	"a", 3, 2
	"b", 1, 2
	"b", 1, 3
	"b", 1, 1
	"b", 2, 2
	"b", 2, 3
	"c", 1, 1
	"c", 1, 3
	"d", 1, 2
	"d", 1, 3
	"d", 1, 1
	"d", 2, 2
	"d", 2, 3
	*/
	BaseRelation cpo("Customer_Product_Table", vector<string>{"customer_name", "order_number", "product_id"}, 
		vector<Dtype>{Dtype::String, Dtype::Int, Dtype::Int}, vector<int>{4, 3, 3},
		16, vector<map<Data, int>>{{{Data("a"), 4}, {Data("b"), 5}}, {{Data(2), 5}}, {}});
	cout<<cpo.show()<<endl;
	cout<<cpo.get_domsize(0)<<" "<<cpo.get_domsize(1)<<" "<<cpo.get_domsize(2)<<endl;
	cout<<cpo.num_tuples()<<endl;  // 16
	cout<<cpo.get_count(0)<<endl;  // 16/4 = 4
	cout<<cpo.get_count(0, Data("a"))<<endl;  // 4
	cout<<cpo.get_count(0, Data("b"))<<endl;  // 5
	cout<<cpo.get_count(0, Data("c"))<<endl;  // 7/2 = 3.5
	cout<<cpo.get_count(0, Data("e"))<<endl;  // 3.5
	cout<<cpo.get_count(1)<<endl;  // 16/3
	cout<<cpo.get_count(1, Data(2))<<endl;  // 5
	cout<<cpo.get_count(1, Data(3))<<endl;  // 11/2 = 5.5
	cout<<cpo.get_count(2)<<endl;  // 16/3
}

void test_expression() {
	vector<BaseRelation> brs = {{"K", {"k", "d"}, {Dtype::String, Dtype::Int}, {100, 100}, 1000, {{}, {}} },
								{"E", {"e", "d"}, {Dtype::Int, Dtype::Int}, {80, 100}, 800, {{}, {}} },
								{"C", {"e", "c"}, {Dtype::Int, Dtype::String}, {80, 10}, 200, {{}, {}} } };
	for(auto &br: brs)
		cout<<br.show()<<endl;

	map<std::string, const BaseRelation*> name2br={{"K", &brs[0]}, {"E", &brs[1]}, {"C", &brs[2]}};
	string query = "Qent[k1, k2](d, e) :- K(k1, d); K(k2, d); E(e, d); C(e, str_phone); E(int_3, d); E(int_4, d)";
	Expression expr(query, name2br);
	cout<<expr.show()<<endl;  // Qent[k1, k2, ](d, e, ) :- K(k1, d, ), K(k2, d, ), E(e, d, ), C(e, String[bs=0](phone), ), E(Int(3), d, ), E(Int(4), d, ),
	cout<<expr.get_signature()<<endl;  // 000 002 003 001 003 002 002 001 001
	Expression exp_k2d = expr.subexpression(set<int>{4,1});
	cout<<exp_k2d.show()<<endl;
	cout<<exp_k2d.get_signature()<<endl;
	Expression exp_k1d = expr.subexpression(set<int>{0,4});
	cout<<exp_k1d.show()<<endl;
	cout<<exp_k1d.get_signature()<<endl;
	auto mapping = exp_k1d.isomorphic_match(exp_k2d, true);
	if(mapping.size()==0)
		cout<<"not isomorphic\n";
	for(auto it=mapping.begin(); it!=mapping.end(); it++) 
		cout<<exp_k2d.get_var(it->first)<<" "<<exp_k1d.get_var(it->second)<<endl;
}

void test_query() {
	vector<BaseRelation> brs = {{"K", {"k", "d"}, {Dtype::String, Dtype::Int}, {100, 100}, 1000, {{}, {}} },
								{"E", {"e", "d"}, {Dtype::Int, Dtype::Int}, {80, 100}, 800, {{}, {}} },
								{"C", {"e", "c"}, {Dtype::Int, Dtype::String}, {80, 10}, 200, {{}, {}} } };
	map<std::string, const BaseRelation*> name2br={{"K", &brs[0]}, {"E", &brs[1]}, {"C", &brs[2]}};
	for(auto &br: brs)
		cout<<br.show()<<endl;

	vector<QueryTemplate> workload;
	string qent_str = "Qent[k1, k2, c](d, e) :- K(k1, d); K(k2, d); E(e, d); C(e, c); C(e, str_phone); E(int_3, d); E(int_4, d)";

	QueryTemplate qent(qent_str, name2br, 1);
	cout<<qent.get_expr().show()<<endl;
	cout<<qent.get_id()<<endl<<endl;
	cout<<qent.connected(set<int>{0,1})<<endl;
	cout<<qent.connected(set<int>{0,3})<<endl;
	cout<<qent.connected(set<int>{0,1,2})<<endl;
	cout<<qent.connected(set<int>{0,1,3})<<endl;
}

void test_utils() {
	oto_map<int, string> M;
	M.insert(1, "a");
	M.insert(2, "b");
	M.insert(3, "c");
	cout<<M.at(3)<<" "<<M.at_inv("c")<<endl;
	M.erase(3);
	cout<<M.find(3)<<" "<<M.find_inv("c")<<endl;
	cout<<M.at(1)<<" "<<M.at(2)<<endl;
	M.erase_inv("b");
	cout<<M.find(2)<<endl;
	M.erase(1);
	cout<<M.find_inv("a")<<endl;
}

void test_index() {

	// vector<BaseRelation> brs = {{"K", {"k", "d"}, {Dtype::String, Dtype::Int}, {100, 100}, 1000, {{}, {}} },
	// 							{"E", {"e", "d"}, {Dtype::Int, Dtype::Int}, {80, 100}, 800, {{}, {}} },
	// 							{"C", {"e", "c"}, {Dtype::Int, Dtype::String}, {80, 10}, 200, {{}, {}} } };
	// map<std::string, const BaseRelation*> name2br={{"K", &brs[0]}, {"E", &brs[1]}, {"C", &brs[2]}};
	// for(auto &br: brs)
	// 	cout<<br.show()<<endl;

	// Index einv("EINV[k, c](d, e) :- K(k, d); E(e,d); C(e, c)", name2br);
	// cout<<einv.get_expr().show()<<endl;
	// cout<<einv.get_id()<<endl;

	// Index ind1("I1[](e, d) :- E(e, d); C(e, str_phone); E(int_3, d); K(int_3, d)", name2br);
	// Index ind2("I2[](e, d, c) :- K(int_3, d); C(e, c); E(e, d); E(int_4, d)", name2br);
	// auto result = ind1.merge(ind2);
}

void test_cost_model() {
	// vector<BaseRelation> brs = {{"K", {"k", "d"}, {Dtype::String, Dtype::Int}, {100, 100}, 1000, {{{"red", 5}, {"blue", 15}}, {{{1, 45}, {12, 34}}}} },
	// 							{"E", {"e", "d"}, {Dtype::Int, Dtype::Int}, {80, 100}, 800, {{}, {}} },
	// 							{"C", {"e", "c"}, {Dtype::Int, Dtype::String}, {80, 10}, 200, {{}, {{"phone", 1000}} } };
	// map<std::string, const BaseRelation*> name2br={{"K", &brs[0]}, {"E", &brs[1]}, {"C", &brs[2]}};

	// CardinalityEstimator E;
	// cout<<E.cardinality(Expression("Q[k, e]() :- K(k, d); E(e, d); C(e, str_phone)", name2br))<<endl;
}

void test_rewriting() {
	
}

void test_candidate_generation() {
	vector<BaseRelation> brs = {{"K", {"k", "d"}, {Dtype::String, Dtype::Int}, {100, 100}, 1000, {{}, {}} },
								{"E", {"e", "d"}, {Dtype::Int, Dtype::Int}, {80, 100}, 800, {{}, {}} },
								{"C", {"e", "c"}, {Dtype::Int, Dtype::String}, {80, 10}, 200, {{}, {}} } };
	map<std::string, const BaseRelation*> name2br={{"K", &brs[0]}, {"E", &brs[1]}, {"C", &brs[2]}};

	vector<QueryTemplate> workload;
	string qent_str = "Qent[k1, k2, c](d, e) :- K(k1, d); K(k2, d); E(e, d); C(e, c); C(e, str_phone); E(int_3, d); E(int_4, d)";
	workload.push_back(QueryTemplate(qent_str, name2br, 0.5));

	string qkd_str = "Qkd[k1, k2](d) :- K(k1, d); K(k2, d)";
	workload.push_back(QueryTemplate(qkd_str, name2br, 0.5));

	Application application(brs, workload);
	application.generate_candidates();
	cout<<application.show_candidates()<<endl;
}