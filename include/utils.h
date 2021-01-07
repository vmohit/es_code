#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <string>
#include <random>
#include <set>
#include <map>
#include <cassert>

namespace esutils {

	bool remove_char(char c);

	/**removes all characters except "a-z", "A-Z", "(", ")", "," and "_" */
	std::string clean_br(std::string str);

	std::vector<std::string> split(const std::string& str, 
		const std::string& delim); 

	std::vector<int> range(uint n);

	float apowb(float a, float b);
	std::vector<float> normalize(const std::vector<float>& dist);

	class random_number_generator {
		std::default_random_engine generator;
		std::uniform_real_distribution<float> distribution;
	public:
		random_number_generator();
		uint sample(const std::vector<float>& dist);  //!< sample from a given multinomial distribution
		std::vector<int> random_permutation(int n); //!< returns a random permutation of sequence [0, 1, ..., n-1]
	};

	//!< pads the given character to the string until its size becomes len
	std::string left_padded_str(std::string str, int len, char pad_char);

	//!< generates all possible subsets of size k of the set {0, 1, ..., n-1}. k must be <= n
	std::vector<std::set<int>> generate_subsets(uint n, uint k);

	// set operations
	std::set<uint> set_intersection(const std::set<uint>& s1, const std::set<uint>& s2);
	std::set<int> set_intersection(const std::set<int>& s1, const std::set<int>& s2);
	std::set<uint> set_difference(const std::set<uint>& s1, const std::set<uint>& s2);
	uint set_intersection_size(const std::set<uint>& s1, const std::set<uint>& s2);
	void set_difference_inplace(std::set<uint>& s1, const std::set<uint>& s2);

	template <class T>
	std::set<T> set_intersection(const std::set<T>& s1, const std::set<T>& s2) {
		std::set<T> newset;
		if(s1.size()<s2.size()) {
			for(auto& ele: s1)
				if(s2.find(ele)!=s2.end())
					newset.insert(ele);
		}
		else {
			for(auto& ele: s2)
				if(s1.find(ele)!=s1.end())
					newset.insert(ele);
		}
		return newset;
	}

	template <class T>
	void set_intersection_inplace(std::set<T>& s1, const std::set<T>& s2) {
		s1 = set_intersection(s1, s2);
	}

	/**One-to-one map that supports functions like find, at, etc. in the reverse direction as well*/
	template <class L, class R>
	class oto_map { 
		std::map<L, R> forward;
		std::map<R, L> backward;
	public:
		void insert(const L& left, const R& right) {  
			assert(forward.find(left)==forward.end());
			assert(backward.find(right)==backward.end());
			forward[left] = right;
			backward[right] = left;
		}
		bool find(const L& left) const {
			return forward.find(left)!=forward.end();
		} 
		bool find_inv(const R& right) const {
			return backward.find(right)!=backward.end();
		} 
		const R& at(const L& left) const {
			assert(forward.find(left)!=forward.end());
			return forward.at(left);
		} 
		const L& at_inv(const R& right) const {
			assert(backward.find(right)!=backward.end());
			return backward.at(right);
		} 
		void erase(const L& left) {  //!< does nothing if element not present
			if(forward.find(left)!=forward.end()) {
				backward.erase(forward.at(left));
				forward.erase(left);
			}
		}
		void erase_inv(const R& right) {  //!< does nothing if element not present
			if(backward.find(right)!=backward.end()) {
				forward.erase(backward.at(right));
				backward.erase(right);
			}
		}
		uint size() const {
			return forward.size();
		}
	};
}

#endif