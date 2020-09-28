#include "expression.h"
#include "data.h"

#include <tuple>
#include <iostream>
#include <string>
#include <map>
#include <set>
#include <vector>

using namespace std;

void test_expression();

int main() {
	cout<<"Testing expression: \n\n";
	test_expression();

	return 0;
}

void test_expression() {
	vector<BaseRelation> brs {{"K", {{Dtype::Int, "k"}, {Dtype::Int, "d"}}},
								{"E", {{Dtype::Int, "e"}, {Dtype::Int, "d"}}},
								{"C", {{Dtype::Int, "e"}, {Dtype::String, "c"}}} };
	for(auto &br: brs)
		cout<<br.show()<<endl;

	map<std::string, const BaseRelation*> name2br={{"K", &brs[0]}, {"E", &brs[1]}, {"C", &brs[2]}};
	string query = "Qent[k1, k2](d, e) :- K(k1, d); K(k2, d); E(e, d); C(e, str_phone); E(int_3, d); E(int_4, d)";
	Expression expr(query, name2br);
	cout<<expr.show()<<endl;  // Qent[k1, k2, ](d, e, ) :- K(k1, d, ), K(k2, d, ), E(e, d, ), C(e, String[bs=0](phone), ), E(Int(3), d, ), E(Int(4), d, ),
	
	Expression exp_k2d = expr.subexpression(set<int>{4,1});
	cout<<exp_k2d.show()<<endl;
	
	Expression exp_k1d = expr.subexpression(set<int>{0,4});
	cout<<exp_k1d.show()<<endl;
}
