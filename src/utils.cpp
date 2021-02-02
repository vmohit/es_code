#include "utils.h"

#include <ctime>
#include <vector>
#include <cstdlib>
#include <string>
#include <cmath>
#include <algorithm>
#include <random>
#include <cassert>
#include <set>
#include <utility>
#include <iostream>

using std::set;
using std::string;
using std::vector;
using std::sort;
using std::pair;
using std::make_pair;
using std::min;
using std::max;
using std::cout;
using std::endl;

bool esutils::remove_char(char c) {
	if(c>='a' && c<='z')
		return false;
	if(c>='A' && c<='Z')
		return false;
	if(c=='_' || c=='(' || c==')' || c==',')
		return false;
	return true;
}

/**removes all characters except "a-z", "A-Z", "(", ")", "," and "_" */
string esutils::clean_br(string str) {
	str.erase(std::remove_if(str.begin(), str.end(), esutils::remove_char), str.end());
	return str;
}

vector<string> esutils::split(const string& str, 
	const string& delim) {
	
	vector<string> result;
	uint start = 0;
	auto end = str.find(delim);
	while (end != std::string::npos) {
		result.push_back(str.substr(start, end - start));
		start = end + delim.length();
		end = str.find(delim, start);
	}
	auto last_str = str.substr(start, end);
	if(last_str.size()>0)
		result.push_back(last_str);
	return result;
}

vector<int> esutils::range(uint n) {
	vector<int> result;
	for(uint i=0; i<n; i++)
		result.push_back((int) i);
	return result;
}

vector<float> esutils::normalize(const vector<float>& dist) {
	float norm_factor=0;
	for(auto prob: dist) {
		assert(prob>=0);
		norm_factor+=prob;
	}
	assert(norm_factor>=0.00001);
	vector<float> result = dist;
	for(uint i=0; i<result.size(); i++)
		result[i] /= norm_factor;
	return result;
}


float esutils::apowb(float a, float b) {
	return exp(b*log(a));
}

esutils::random_number_generator::random_number_generator() :
	distribution(0.0, 1.0) {}

uint esutils::random_number_generator::sample(const vector<float>& dist) {
	assert(dist.size()!=0);
	float sum=0;
	for(auto prob: dist) {
		assert(prob>=0);
		sum+=prob;
	}
	assert(abs(sum-1)<0.000001);
	float r = distribution(generator);
	float curr=0;
	for(uint i=0; i<dist.size(); i++) {
		if(curr+dist[i]>=r)
			return i;
		curr += dist[i];
	}
	return dist.size()-1;
}

vector<int> esutils::random_number_generator::random_permutation(int n) {
	assert(n>0);
	vector<pair<int, float>> priority_num;
	for(int i=0; i<n; i++)
		priority_num.push_back(make_pair(distribution(generator), i));
	sort(priority_num.begin(), priority_num.end());
	vector<int> result;
	for(int i=0; i<n; i++)
		result.push_back(priority_num[i].second);
	return result;
}


string esutils::left_padded_str(string str, int len, char pad_char) {
	string result = str;
	if((int)result.size()<len)
		result.insert(0, len-result.size(), pad_char);
	return result;
}


set<uint> esutils::set_difference(const set<uint>& s1, const set<uint>& s2) {
	set<uint> result;
	for(auto ele: s1)
		if(s2.find(ele)==s2.end())
			result.insert(ele);
	return result;
}

set<uint> esutils::set_intersection(const set<uint>& s1, const set<uint>& s2) {
	set<uint> result;
	for(auto ele: s1)
		if(s2.find(ele)!=s2.end())
			result.insert(ele);
	return result;
}

set<int> esutils::set_intersection(const set<int>& s1, const set<int>& s2) {
	set<int> result;
	for(auto ele: s1)
		if(s2.find(ele)!=s2.end())
			result.insert(ele);
	return result;
}

uint esutils::set_intersection_size(const set<uint>& s1, const set<uint>& s2) {
	uint result=0;
	for(auto ele: s1)
		if(s2.find(ele)!=s2.end())
			result++;
	return result;
}

void esutils::set_difference_inplace(set<uint>& s1, const set<uint>& s2) {
	for(auto ele: s2)
		s1.erase(ele);
}


vector<set<int>> esutils::generate_subsets(uint n, uint k) {
	assert(k<=n);
	if(k==0)
		return vector<set<int>>{set<int>{}};
	vector<set<int>> result;
	for(int i=n-1; i>=(int) k-1; i--) {
		auto subresults = generate_subsets(i, k-1);
		for(auto& subset: subresults) {
			result.push_back(subset);
			result.back().insert(i);
		}
	}
	return result;
}

esutils::ExtremeFraction::ExtremeFraction(){}

esutils::ExtremeFraction::ExtremeFraction(vector<double> nums, vector<double> dens) {
	for(auto num: nums)
		multiply (num);
	for(auto den: dens)
		divide (den);
}

void esutils::ExtremeFraction::multiply(double val) {
	assert(val>0);
	if(val<1) divide(1.0/val);
	else {
		auto it = denominators.find(val);
		if(it==denominators.end())
			numerators.insert(val);
		else
			denominators.erase(it);
	}
}

void esutils::ExtremeFraction::divide(double val) {
	assert(val>0);
	if(val<1) multiply(1.0/val);
	else {
		auto it = numerators.find(val);
		if(it==numerators.end())
			denominators.insert(val);
		else
			numerators.erase(it);
	}
} 

void esutils::ExtremeFraction::multiply(const esutils::ExtremeFraction& ef) {
	for(auto num: ef.numerators)
		multiply(num);
	for(auto den: ef.denominators)
		divide(den);
}

double esutils::ExtremeFraction::eval(double floor, double ceil) const {
	double val = 1;
	auto nit = numerators.begin();
	auto dit = denominators.begin();
	while (nit!=numerators.end() && dit!=denominators.end()) {
		if(val<1) {
			val *= (*nit);
			nit++;
		}
		else {
			val /= (*dit);
			dit++;
		}
	}
	while (nit!=numerators.end()) {
		val *= (*nit);
		nit++;
		if(val>ceil)
			return ceil;
	}
	while (dit!=denominators.end()) {
		val /= (*dit);
		dit++;
		if(val<floor)
			return floor;
	}
	return min(max(floor, val), ceil);
}

double esutils::OneMinusXN(const esutils::ExtremeFraction& ef_x, 
	const esutils::ExtremeFraction& ef_n) {
	
	esutils::ExtremeFraction ef_nx = ef_x;
	ef_nx.multiply(ef_n);
	double x=ef_x.eval(), nx=ef_nx.eval();
	//cout<<"nx: " << nx<<endl;
	if (x<1 && nx<0.1)
		return max(0.0, min(1-nx, 1.0));
	if (x<0.1 && nx>5)
		return exp(-1*nx);
	return max(0.0, min(1.0, 1-nx + (nx*(nx-x))/2.0));
}