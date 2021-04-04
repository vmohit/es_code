#include "query.h"
#include "data.h"
#include "dataframe.h"
#include "expression.h"
#include "utils.h"

#include <map>
#include <string>
#include <set>
#include <vector>
#include <list>
#include <queue>
#include <utility>
#include <iostream>
#include <algorithm>

using std::min;
using std::map;
using std::vector;
using std::set;
using std::string;
using std::to_string;
using std::list;
using std::queue;
using std::cout;
using std::endl;
using std::sort;
using std::pair;
using std::make_pair;
using esutils::random_number_generator;

vector<int> get_goal_order(const Expression& qexpr) {
	CardinalityEstimator E(qexpr, set<int>());
	set<int> free_hvars, bound_hvars;
	vector<int> goal_order;
	for(int i=0; i<qexpr.num_goals(); i++) {
		int best_goal=-1;
		double best_card=std::numeric_limits<double>::max();
		auto considered_goals = E.considered_goals();
		for(int gid=0; gid<qexpr.num_goals(); gid++)
			if(considered_goals.find(gid)==considered_goals.end()) {
				CardinalityEstimator Ep = E;
				auto fvars=free_hvars;
				auto bvars=bound_hvars;
				Ep.add_goal(gid);
				for(auto symb: qexpr.goal_at(gid).symbols)
					if(!symb.isconstant){
						if(qexpr.is_free_headvar(symb.var))
							fvars.insert(symb.var);
						if(qexpr.is_bound_headvar(symb.var))
							bvars.insert(symb.var);
					}
				auto cards = Ep.get_cardinalities(vector<int>(fvars.begin(), fvars.end()), bvars);
				double total_card = 1;
				for(auto card: cards) total_card*=card;
				if (total_card<best_card) {
					best_goal=gid; best_card=total_card;
				}
			}
		goal_order.push_back(best_goal);
		E.add_goal(best_goal);
		for(auto symb: qexpr.goal_at(best_goal).symbols)
			if(!symb.isconstant){
				if(qexpr.is_free_headvar(symb.var))
					free_hvars.insert(symb.var);
				if(qexpr.is_bound_headvar(symb.var))
					bound_hvars.insert(symb.var);
			}
	}
	return goal_order;
}

Query::Query(const Expression& exp_arg, double wgt)
: exp(exp_arg), wt(wgt) {
	assert(!exp.empty());
	map<const BaseRelation*, BaseRelation::Table*> br2tab;
	map<int, Data> var2const;
	int maxconst=0;
	set<Data> consts;
	for(int gid=0; gid<exp.num_goals(); gid++) 
		for(uint i=0; i<exp.goal_at(gid).symbols.size(); i++) 
			if(exp.goal_at(gid).symbols.at(i).isconstant)
				consts.insert(exp.goal_at(gid).symbols.at(i).dt);

	for(int gid=0; gid<exp.num_goals(); gid++) {
		auto goal = exp.goal_at(gid);
		if(br2tab.find(goal.br)==br2tab.end()) {
			tables.push_back(BaseRelation::Table(goal.br, goal.br->get_name()));
			br2tab[goal.br] = &tables.back();
		}
		auto table = br2tab.at(goal.br);
		vector<Data> row;
		for(uint i=0; i<goal.symbols.size(); i++) {
			auto symbol = goal.symbols.at(i);
			if(symbol.isconstant)
				row.push_back(symbol.dt);
			else {
				if(var2const.find(symbol.var)==var2const.end()) {
					if(goal.br->dtype_at(i)==Dtype::Int) {
						while(consts.find(Data(maxconst++))!=consts.end()) {}
						var2const.emplace(symbol.var, Data(maxconst-1));
					}
					else {
						while(consts.find(Data(to_string(maxconst++)))!=consts.end()) {}
						var2const.emplace(symbol.var, Data(to_string(maxconst-1)));
					}
				}
				row.push_back(var2const.at(symbol.var));
			}
		}
		table->df.add_tuple(row);
	}
	for(auto item: br2tab)
		br2table[item.first] = item.second;
	for(auto item: var2const)
		const2var[item.second] = item.first;

	g_order = get_goal_order(exp_arg);
}

const vector<int>& Query::goal_order() const {
	return g_order;
}

const Expression& Query::expression() const {
	return exp;
}

double Query::weight() const {return wt;}

string Query::show(bool verbose) const {
	string result = exp.show();
	return result;
}

list<ViewTuple> Query::get_view_tuples(const Index& index) const {
	for(int i=0; i<index.expression().num_goals(); i++)
		if(br2table.find(index.expression().goal_at(i).br)==br2table.end())
			return list<ViewTuple>();

	list<ViewTuple> result;
	Expression::Table vts(&index.expression(), br2table);
	for(auto row: vts.df.get_unique_rows()) {
		map<int, Expression::Symbol> index2query;
		for(auto headvar: index.expression().head_vars()) {
			Data dt = row[vts.df.get_cid2pos(vts.headvar2cid.at(headvar))];
			if(const2var.find(dt)==const2var.end())
				index2query.emplace(headvar, Expression::Symbol(dt));
			else
				index2query.emplace(headvar, Expression::Symbol(const2var.at(dt)));
		}
		result.push_back(ViewTuple(*this, index, index2query));
	}
	return result;
}

Index::Index(const Expression& exp_arg): exp(exp_arg), E(exp),
avg_disk_block_size(0), total_storage_cost(0) {
	assert(!exp.empty());
	
	vector<pair<double, int>> card_vars;
	for(auto hv: exp.head_vars()) 
		card_vars.push_back(make_pair(E.var_card(hv), hv));
	sort(card_vars.begin(), card_vars.end());
	vector<int> hvar_order;
	for(auto card_var: card_vars)
		if(exp.is_bound_headvar(card_var.second))
			hvar_order.push_back(card_var.second);
	for(auto card_var: card_vars)
		if(exp.is_free_headvar(card_var.second))
			hvar_order.push_back(card_var.second);
	auto cardinalities = E.get_cardinalities(hvar_order);
	double num_combinations=1;
	double num_blocks=1;
	cout<<exp.show()<<" "<<"index cardinalities: ";
	for(uint i=0; i<hvar_order.size(); i++) {
		num_combinations *= cardinalities[i];
		if(exp.is_bound_headvar(hvar_order[i])) {
			total_storage_cost += mem_storage_weight*num_combinations;
			num_blocks = num_combinations;
		}
		else {
			avg_disk_block_size += num_combinations;
			total_storage_cost += disk_storage_weight*num_combinations;
		}
		cout<<cardinalities[i]<<" ";
	}
	cout<<endl;
	avg_disk_block_size/=num_blocks;
}

// Index& Index::operator=(const Index& other) {
// 	exp = other.exp;

// 	avg_disk_block_size = other.avg_disk_block_size;
// 	total_storage_cost = other.total_storage_cost;
// }

double Index::storage_cost() const { return total_storage_cost;}
double Index::avg_block_size() const {return avg_disk_block_size;}

const Expression& Index::expression() const {
	return exp;
}

string Index::show(bool verbose) const {
	string result = exp.show();
	if(verbose) {
		result += "\nTotal storage cost: " + to_string(total_storage_cost) + "\n";
		result += "Avg disk block size: " + to_string(avg_disk_block_size) + "\n";
	}
	return result;
}

string ViewTuple::show(bool verbose) const {
	string result=index.expression().get_name();
	result += "(";
	uint i=0;
	for(auto headvar: index.expression().head_vars()) {
		result += index.expression().var_to_name(headvar)+": ";
		auto symbol = index2query.at(headvar);
		if(symbol.isconstant)
			result += symbol.dt.show();
		else
			result += query.expression().var_to_name(symbol.var);
		i++;
		if(i!=index.expression().head_vars().size())
			result += ", ";
	}
	result += ") -> subcores: {";
	uint j=0;
	for(auto subcore: subcores) {
		result += "{";
		uint i=0;
		for(uint gid: subcore)
			result += to_string(gid)+(++i == subcore.size() ? "}":", ");
		j++;
		if(j!=subcores.size())
			result+=", ";
	}
	result += "}, lb: " + to_string(cost_lb) + ", ub: " + to_string(cost_ub);
	if(verbose)
		result += ", exp: " + E.expression().show() + ", exp_bh: " + E_bh.expression().show();
	return result;
}



ViewTuple::ViewTuple(const Query& query_arg, const Index& index_arg,
		map<int, Expression::Symbol> index2query_arg) 
: query(query_arg), index(index_arg), index2query(index2query_arg), first_sc_goal_ind(query.goal_order().size()),
E(index.expression(), set<int>()), E_bh(index.expression(), set<int>()) {
	set<int> unexplored_goals;
	for(int gid=0; gid<query.expression().num_goals(); gid++)
		unexplored_goals.insert(gid);
	for(int gid=0; gid<query.expression().num_goals(); gid++) {
		if(unexplored_goals.find(gid)!=unexplored_goals.end()) {
			set<int> subcore;
			set<int> unmapped_goals;
			unmapped_goals.insert(gid);
			map<int, int> mu;
			bool match=false;
			try_match(match, subcore, unmapped_goals, mu);
			if(match) 
				subcores.insert(subcore);
			for(auto id: subcore)
				unexplored_goals.erase(id);
		}
	}

	for(auto subcore: subcores) {
		if(subcore.size()==1)
			sc_goals.insert(*subcore.begin());
		for(auto gid: subcore)
			wc_goals.insert(gid);
	}

	Expression expansion = index.expression();
	for(auto const& kv: index2query) {
		if(kv.second.isconstant)
			expansion.select(kv.first, kv.second.dt);
		else
			if(qvar2evar.find(kv.second.var)==qvar2evar.end())
				qvar2evar[kv.second.var] = kv.first;
			else
				qvar2evar[kv.second.var] = expansion.join(kv.first, qvar2evar[kv.second.var]);
	}
	E = CardinalityEstimator(expansion);

	Expression expansion_bh = index.expression();
	for(auto const& kv: index2query) {
		if(index.expression().is_bound_headvar(kv.first)) {
			if(kv.second.isconstant)
				expansion_bh.select(kv.first, kv.second.dt);
			else
				if(qvar2evar_bh.find(kv.second.var)==qvar2evar_bh.end())
					qvar2evar_bh[kv.second.var] = kv.first;
				else
					qvar2evar_bh[kv.second.var] = expansion_bh.join(kv.first, qvar2evar_bh[kv.second.var]);
		}
	}
	E_bh = CardinalityEstimator(expansion_bh);

	for(uint i=0; i<query.goal_order().size(); i++)
		if(sc_goals.find(query.goal_order()[i])!=sc_goals.end()) {
			first_sc_goal_ind = i;
			break;
		}
}

const set<uint>& ViewTuple::covered_goals(bool oe) const {
	if(oe) return sc_goals;
	else return wc_goals;
}

void ViewTuple::try_match(bool& match, set<int>& subcore,
	set<int>& unmapped_goals, map<int, int>& mu) {
	if(unmapped_goals.empty()) {
		match = true;
		return;
	}
	int q_gid = *(unmapped_goals.begin());
	unmapped_goals.erase(q_gid);
	subcore.insert(q_gid);
	for(int i_gid=0; i_gid<index.expression().num_goals(); i_gid++) {
		if(query.expression().goal_at(q_gid).br
			==index.expression().goal_at(i_gid).br) {
			set<int> newkeys;
			set<int> newgoals;
			const auto& q_symbols = query.expression().goal_at(q_gid).symbols;
			const auto& i_symbols = index.expression().goal_at(i_gid).symbols;
			bool flag=true;
			for(uint i=0; i<q_symbols.size(); i++) {
				if(q_symbols.at(i).isconstant) {
					if(i_symbols.at(i)!=q_symbols.at(i) &&
						(index2query.find(i_symbols.at(i).var)==index2query.end() ? 
							true: index2query.at(i_symbols.at(i).var)!=q_symbols.at(i))) {
						flag = false;
						break;
					}
				}
				else {
					if(query.expression().head_vars().find(q_symbols.at(i).var) 
						!= query.expression().head_vars().end())  {
						if(index2query.find(i_symbols.at(i).var)!=index2query.end() ?
							index2query.at(i_symbols.at(i).var)!=q_symbols.at(i) : true) {
								flag = false;
								break;
						}
					}
					else {
						if(mu.find(q_symbols.at(i).var)==mu.end()) {
							newkeys.insert(q_symbols.at(i).var);
							mu[q_symbols.at(i).var] = i_symbols.at(i).var;
						}
						if(mu.at(q_symbols.at(i).var)!=i_symbols.at(i).var) {
							flag=false;
							break;
						}
						if(index.expression().head_vars().find(i_symbols.at(i).var) 
							== index.expression().head_vars().end())
							for(int new_q_gid: query.expression().goals_containing(q_symbols.at(i).var))
								if(subcore.find(new_q_gid)==subcore.end() 
									&& unmapped_goals.find(new_q_gid)==unmapped_goals.end())
									newgoals.insert(new_q_gid);
					}
				}
			}
			if(flag) {
				for(int goal: newgoals)
					unmapped_goals.insert(goal);
				try_match(match, subcore, unmapped_goals, mu);
				if(match) return;
				for(int goal: newgoals)
					unmapped_goals.erase(goal);
			}
			for(int key: newkeys)
				mu.erase(key);
		}
	}
}


Plan::Plan(const Query& qry): query(qry), 
E(query.expression(), set<int>()) {
	complete = false;
}


bool Plan::append(const ViewTuple& vt) {
	cost += time(vt);

	for(auto gid: vt.sc_goals)
		sc_goals.insert(gid);

	for(auto gid: vt.wc_goals)
		wc_goals.insert(gid);	

	auto& Egoals = E.considered_goals();
	vector<set<int>> subcores;
	for(auto subcore: vt.subcores) {
		subcores.push_back(subcore);
		bool add_subcore = true;
		for(auto gid: subcore)
			if(Egoals.find(gid)!=Egoals.end()) 
				add_subcore = false;
		if(add_subcore) {
			// cout<<query.expression().show()<<endl;
			// cout<<vt.show()<<endl;
			// cout<<E.considered_goals().size()<<endl;
			// cout<<Egoals.size()<<endl;
			// cout<<E.expression().show()<<endl;
			for(auto gid: subcore) {
				//cout<<gid<<endl;
				E.add_goal(gid);
			}
		}
	}

	if(iscomplete()) { 
		complete=true;
		for(int i=0; i<query.expression().num_goals(); i++)
			E.add_goal(i);
	}
	stages.push_back(&vt);
	set_stages.insert(&vt);

	return true;
}

double Plan::time(const ViewTuple& nvt) const {
	double num_lookups=1;
	vector<int> qvars;
	for(auto const& kv: nvt.qvar2evar_bh) 
		if(!query.expression().is_bound_headvar(kv.first)) qvars.push_back(kv.first);
	
	vector<int> vars;
	for(auto qvar: qvars) vars.push_back(nvt.qvar2evar_bh.at(qvar));
	set<int> pre_select_vars;
	for(auto const& kv: nvt.qvar2evar_bh) 
		if(query.expression().is_bound_headvar(kv.first)) 
			pre_select_vars.insert(kv.second);
	
	map<int, double> qvar2card;
	auto cards = nvt.E_bh.get_cardinalities(vars, pre_select_vars);
	for(uint i=0; i<qvars.size(); i++)
		qvar2card[qvars[i]] = cards[i];

	vars.clear();
	pre_select_vars.clear();
	for(auto qvar: qvars) 
		if(E.is_present(qvar)) vars.push_back(qvar);
	pre_select_vars = query.expression().bound_headvars();
	cards = E.get_cardinalities(vars, pre_select_vars);
	for(uint i=0; i<vars.size(); i++)
		qvar2card[vars[i]] = min(cards[i], qvar2card[vars[i]]);
	
	for(auto vt: stages) {
		vars.clear();
		pre_select_vars.clear();
		vector<int> vars_q;
		for(auto qvar: qvars) 
			if(vt->qvar2evar.find(qvar)!=vt->qvar2evar.end()) {
				vars.push_back(vt->qvar2evar.at(qvar));
				vars_q.push_back(qvar);
			}
		for(auto const& kv: vt->qvar2evar) 
			if(query.expression().is_bound_headvar(kv.first)) 
				pre_select_vars.insert(kv.second);
		cards = vt->E.get_cardinalities(vars, pre_select_vars);
		for(uint i=0; i<vars.size(); i++)
			qvar2card[vars_q[i]] = min(cards[i], qvar2card[vars_q[i]]);
	}
	
	for(auto qvar: qvars)
		num_lookups *= qvar2card.at(qvar);
	//cout<<"num_lookups: "<<num_lookups<<endl;

	return num_lookups*(disk_seek_time + disk_read_time_per_unit*nvt.index.avg_block_size());
}

double Plan::time(const ViewTuple* vt) const {return time(*vt);}

double Plan::current_cost() const {return cost;}

bool Plan::check_completeness(set<int>& covered_goals,
	vector<set<int>>& subcores, uint pos) const {
	if(((int)covered_goals.size())==query.expression().num_goals())
		return true;
	if(pos>=subcores.size())
		return false;
	auto& subcore=subcores.at(pos);
	bool try_adding=true;
	for(auto gid: subcore)
		if(covered_goals.find(gid)!=covered_goals.end()) {
			try_adding=false;
			break;
		}
	if(try_adding) {
		for(auto gid: subcore)
			covered_goals.insert(gid);
		if(check_completeness(covered_goals, subcores, pos+1)) 
			return true;
		for(auto gid: subcore)
			covered_goals.erase(gid);
	}
	return check_completeness(covered_goals, subcores, pos+1);
}

bool Plan::iscomplete() const {
	if(complete) return complete;
	if(((int)wc_goals.size())<query.expression().num_goals()) 
		return false;
	vector<set<int>> all_subcores;
	for(auto vt: stages)
		for(auto& subcore: vt->subcores)
			all_subcores.push_back(subcore);
	set<int> covered_goals;
	bool result = check_completeness(covered_goals, all_subcores, 0);
	
	return result;
}

const set<uint>& Plan::strongly_covered_goals() const {return sc_goals;}
const set<uint>& Plan::weakly_covered_goals() const {return wc_goals;}
const set<uint>& Plan::covered_goals(bool oe) const {
	if(oe) return sc_goals;
	else return wc_goals;
}

int Plan::extra_sc_goals(const ViewTuple& vt) const {
	int result=0;
	for(auto gid: vt.sc_goals)
		if(sc_goals.find(gid)==sc_goals.end())
			result += 1;
	return result;
} 
int Plan::extra_wc_goals(const ViewTuple& vt) const {
	int result=0;
	for(auto gid: vt.wc_goals)
		if(wc_goals.find(gid)==wc_goals.end())
			result += 1;
	return result;
}

string Plan::show() const {
	string result="{\n";
	result += "\t(complete = "+to_string(complete)+")\n";
	result += "\tViewTuples:\n";
	for(auto vt: stages)
		result += "\t\t" + vt->show(false) + "\n";
	result += "\tcost = " + to_string(cost) + "\n";
	result += "\tstrongly covered goals: ";
	for(auto gid: sc_goals)
		result += to_string(gid) + " ";
	result += "\n";

	result += "\nweakly covered goals: ";
	for(auto gid: wc_goals)
		result += to_string(gid) + " ";
	result += "\n}";
	return result;
}

const ViewTuple* Plan::stage(uint i) const {
	assert(i<stages.size());
	return stages.at(i);
}

uint Plan::num_stages() const {
	return stages.size();
}

bool Plan::has_vt(const ViewTuple* vt) const {
	return set_stages.find(vt)!=set_stages.end();
}
