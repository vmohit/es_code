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

Query::Query(const Expression& exp_arg, double wgt, const std::map<const BaseRelation*, 
	const BaseRelation::Table*>& stats_br2table, int num_samples)
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

	Expression::Table big_result(&exp, stats_br2table);
	for(auto headvar: exp.head_vars())
		if(!exp.is_bound_headvar(headvar)) {
			big_result.df.project_out(big_result.headvar2cid.at(headvar));
			big_result.headvar2cid.erase(headvar);
		}
	
	auto rowset = big_result.df.get_unique_rows();
	vector<vector<Data>> rows;
	for (auto& row: rowset) rows.push_back(row);
	assert(rows.size()>0);

	random_number_generator rng;
	auto permutation = rng.random_permutation(rows.size());
	num_samples = min(num_samples, (int) rows.size());
	for(int i=0; i<num_samples; i++) {
		map<int, Data> inputs;
		for(auto headvar: exp.head_vars())
			if(exp.is_bound_headvar(headvar))
				inputs.emplace(headvar, rows[permutation[i]]
										[big_result.df.get_cid2pos(big_result.headvar2cid.at(headvar))]);
		sample_inputs.push_back(inputs);
	}

	// for(uint i=0; i<rows.size(); i++) {
	// 	map<int, Data> inputs;
	// 	for(auto headvar: exp.head_vars())
	// 		if(exp.is_bound_headvar(headvar))
	// 			inputs.emplace(headvar, rows[permutation[i]]
	// 									[big_result.df.get_cid2pos(big_result.headvar2cid.at(headvar))]);
	// 	for(auto input: inputs) {
	// 		auto data = input.second;
	// 		cout<<(data.get_dtype()==Dtype::Int ? to_string(data.get_int_val()): data.get_str_val()) << "\t";
	// 	}
	// 	cout<<endl;
	// }
}

const vector<map<int, Data>>& Query::get_sample_inputs() const {
	return sample_inputs;
}

const Expression& Query::expression() const {
	return exp;
}

double Query::weight() const {return wt;}

string Query::show(bool verbose) const {
	string result = exp.show();
	if(verbose) {
		result += "\nsample inputs: \n";
		if(sample_inputs.size()>0) {
			for(auto hvar_data: sample_inputs[0])
				result += exp.var_to_name(hvar_data.first) + "\t";
			result += "\n";
			for(uint i=0; i<sample_inputs.size(); i++) {
				for(auto hvar_data: sample_inputs[i]) {
					auto data = hvar_data.second;
					result += (data.get_dtype()==Dtype::Int ? to_string(data.get_int_val()): data.get_str_val()) + "\t";
				}
				result += "\n";
			}
		}
	}
	return result;
}

list<ViewTuple> Query::get_view_tuples(const Index& index) const {
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

Index::Index(const Expression& exp_arg): exp(exp_arg) {
	assert(!exp.empty());

	set<int> allgids;
	for(int i=0; i<exp.num_goals(); i++) allgids.insert(i);
	CardinalityEstimator E(exp, allgids); 
	
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
	cout<<"index cardinalities: ";
	for(uint i=0; i<hvar_order.size(); i++) {
		num_combinations *= cardinalities[i];
		if(exp.is_bound_headvar(hvar_order[i])) {
			total_storage_cost += mem_storage_weight*num_combinations;
			num_blocks = num_combinations;
		}
		else {
			avg_disk_block_size += disk_storage_weight*num_combinations;
			total_storage_cost += disk_storage_weight*num_combinations;
		}
		cout<<cardinalities[i]<<" ";
	}
	cout<<endl;
	avg_disk_block_size/=num_blocks;
}

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

string ViewTuple::show() const {
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
	result += "}, lb: " + to_string(cost_lb);
	return result;
}



ViewTuple::ViewTuple(const Query& query_arg,
		const Index& index_arg,
		map<int, Expression::Symbol> index2query_arg) 
: query(query_arg), index(index_arg),
index2query(index2query_arg) {
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


Plan::Plan(const Query& qry): query(qry) {}

void Plan::execute_view_tuple(const ViewTuple& vt, list<DataFrame>& df_vt_lst, 
	list<map<int, string>>& qvar2cid_lst) const {

	// auto sample_inputs = query.get_sample_inputs();
	// for(uint i=0; i<sample_inputs.size(); i++) {
	// 	auto inputs=sample_inputs.at(i);
	// 	auto df_vt=table.df;
	// 	map<int, string> qvar2cid;
	// 	string prefix = to_string(stages.size());
	// 	for(auto& hvar_cid: table.headvar2cid) {
	// 		auto target_symbol = vt.index2query.at(hvar_cid.first);
	// 		if(target_symbol.isconstant)
	// 			df_vt.select(hvar_cid.second, target_symbol.dt);
	// 		else {
	// 			if(inputs.find(target_symbol.var)!=inputs.end()) 
	// 				df_vt.select(hvar_cid.second, inputs.at(target_symbol.var));
	// 			else {
	// 				auto var = target_symbol.var;
	// 				if(qvar2cid.find(var)==qvar2cid.end())
	// 					qvar2cid[var] = hvar_cid.second;
	// 				else
	// 					qvar2cid[var] = df_vt.self_join(qvar2cid.at(var), hvar_cid.second);
	// 			}
	// 		}
	// 	}
	// 	df_vt.prepend_to_cids(prefix);
	// 	for(auto it=qvar2cid.begin(); it!=qvar2cid.end(); it++)
	// 		it->second = prefix + "_" + it->second;
	// 	df_vt_lst.push_back(df_vt);
	// 	qvar2cid_lst.push_back(qvar2cid);
	// }
}

double Plan::try_append(const ViewTuple& vt, list<list<DataFrame>>& new_stats,
		list<list<map<int, string>>>& new_queryvar2cid) const {
	list<DataFrame> df_vt_lst;
	list<map<int, string>> var2cid_lst;
	execute_view_tuple(vt, df_vt_lst, var2cid_lst);
	
	double added_cost=0;
	if(stages.size()==0) {
		auto df_vt_it=df_vt_lst.begin(); auto var2cid_it=var2cid_lst.begin();
		for(; df_vt_it!=df_vt_lst.end(); df_vt_it++, var2cid_it++) {
			new_stats.push_back(list<DataFrame>{*df_vt_it});
			new_queryvar2cid.push_back(list<map<int, string>>{*var2cid_it});
		}
	}
	auto ns_it = new_stats.begin(); auto nq_it = new_queryvar2cid.begin();
	auto df_vt_it=df_vt_lst.begin(); auto var2cid_it=var2cid_lst.begin();
	for(; ns_it!=new_stats.end(); ns_it++, nq_it++, df_vt_it++, var2cid_it++) {
		auto& new_stats_i = *ns_it;
		auto& new_queryvar2cid_i = *nq_it;
		auto& df_vt_lst_i = *df_vt_it;
		auto& var2cid_lst_i = *var2cid_it;
		if(stages.size()>0) {
			list<list<DataFrame>::iterator> delete_df_its;
			list<list<map<int, string>>::iterator> delete_qv2cid_its;
			auto df_it=new_stats_i.begin(); auto qv2cid_it=new_queryvar2cid_i.begin();
			for(; df_it!=new_stats_i.end(); df_it++, qv2cid_it++) {
				bool join=false;
				for(auto& qvar_cid: *qv2cid_it)
					if(var2cid_lst_i.find(qvar_cid.first)!=var2cid_lst_i.end()) {
						join=true; break;
					}
				if(join) {
					vector<pair<string, string>> this2df;
					for(auto& qvar_cid: *qv2cid_it) {
						if(var2cid_lst_i.find(qvar_cid.first)!=var2cid_lst_i.end()) 
							this2df.push_back(make_pair(var2cid_lst_i.at(qvar_cid.first), 
								qvar_cid.second));
						else
							var2cid_lst_i[qvar_cid.first] = qvar_cid.second;
					}
					df_vt_lst_i.join(*df_it, this2df);
					delete_df_its.push_back(df_it);
					delete_qv2cid_its.push_back(qv2cid_it);
				}	
			}
			for(auto it: delete_df_its) new_stats_i.erase(it);
			for(auto it: delete_qv2cid_its) new_queryvar2cid_i.erase(it);
			new_stats_i.push_back(df_vt_lst_i);
			new_queryvar2cid_i.push_back(var2cid_lst_i);
		}
		set<int> keep_qvars;
		for(auto& ivar_qsymb: vt.index2query)
			if(!ivar_qsymb.second.isconstant)
				if(vt.index.expression().is_bound_headvar(ivar_qsymb.first) && 
					new_queryvar2cid_i.begin()->find(ivar_qsymb.second.var)!=new_queryvar2cid_i.begin()->end())
					keep_qvars.insert(ivar_qsymb.second.var);
		int num_disk_seeks = 1;
		if(keep_qvars.size()>0) {
			auto stats_copy = new_stats_i.back();
			for(auto& qvar_cid: *new_queryvar2cid_i.begin())
				if(keep_qvars.find(qvar_cid.first)==keep_qvars.end())
					stats_copy.project_out(qvar_cid.second);
			num_disk_seeks = stats_copy.num_unique_rows();
		}
		added_cost += num_disk_seeks*(disk_seek_time 
						+ disk_read_time_per_unit*vt.index.avg_block_size());
	}
	added_cost /= (query.get_sample_inputs().size()+0.0001);

	return added_cost;
}


bool Plan::append(const ViewTuple& vt) {
	
	double added_cost = try_append(vt, stats, queryvar2cid);

	cost += added_cost;
	stages.push_back(&vt);
	set_stages.insert(&vt);

	for(auto gid: vt.sc_goals)
		sc_goals.insert(gid);

	for(auto gid: vt.wc_goals)
		wc_goals.insert(gid);	

	vector<set<int>> subcores;
	for(auto subcore: vt.subcores)
		subcores.push_back(subcore);

	if(iscomplete()) complete=true;
	return true;
}

double Plan::time(const ViewTuple& vt) const {
	auto new_stats = stats;
	auto new_queryvar2cid = queryvar2cid;
	return try_append(vt, new_stats, new_queryvar2cid);
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
		result += "\t\t" + vt->show() + "\n";
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
