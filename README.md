Current status:
- [x] Everything is going well
- [ ] I'm falling behind and need to reconfigure


# Everything Search

Implementation of everything search project.

## Weekly Plan

Following plan has weekly checkpoints. Please refer to the checkboxes to measure the progress.

- [x] [September 27] Build expressions: create the class to hold conjunctive expressions and write methods to create them from string representations and as a subexpression of other expressions. Eg:- construct an expression object from the string "Q[k1, k2, c]\(d, e\):- K(k1, d), K(k2, d), E(e, d), C(e, c)". And then create subexpressions of Q corresponding to a given subset of sub-goals, say "Q134[k1, c]\(d, e\):- K(k1, d), E(e, d), C(e, c)".

- [x] [October 4] Implement containment maps: it is a map from the variables of one expression to another, eg:- from the expression "S[k]\(d, e\):- K(k, d), E(d, e)" to the expression "T[k1, k2, c]\(e\):- K(k1, d), K(k2, d), E(e, d), C(e, c)" we can have a partial map {k->k1, d->d, e->e}. The partial containment map must make sure that each sub-goal of the source expression "S" maps to some sub-goal of target expression "T". 

- [ ] [October 11] Identify and merge similar expressions: implement two functionalities:
	1. Given two expressions "E1[k1]\(d, e\):-K(k1, d), E(e, d)" and "E2[k2]\(d, e\):-K(k2, d), E(e, d)", identify that they are identicle give two containment maps for both side, {k1->k2, d->d, e->e} and {k2->k1, d->d, e->e}.
	2. Identify if two given expressions have the same join structure. Eg:- "E1[k1]\(d, e\):- K(k1, d), E(e, d), C(e, phone)" and "E2[]\(d, e, c\):- K(red, d), E(e, d), C(e, c)" has the same join structure with the join variables "d" and "e". Return a merged expression "Emerged[k1]\(d, e, c\):- K(k1, d), E(e, d), C(e, c)" which is a generalization of the two subexpressions. Also obtain containment maps from this merged expression to the two original expressions. Eg:- {k1->k1, d->d, e->e, c->phone} and {k1->red, d->d, e->e, c->c}.

- [ ] [October 18] Implement simple dataframe class along with functions to execute selection, projection and join operations on it. This class will be used in the cost model to compute queries on sampled dataset in order to generate cost estimates. Eg:- say we have sub-sampled tables K, E and C. Then we can estimate the size of the output of the query "Q[k1, k2, c]\(d, e\):- K(k1, d), K(k2, d), E(e, d), C(e, c)" by running the query on the sub-sampled tables. We replace the bound head variables (k1, k2 and c) with randomly sampled constants and repeat the execution a set number of times to obtain expected number of tuples in a randomly generated query instance.

- [ ] [October 25] Create classes to represent queries, indexes, view tuples, and query plans. Also write code to compute the sub-cores of a view tuple. For instance: the view tuple "I(a, c, d)" of the index "I(x, z, w):- A(x, y), B(y, z), C(z, w)" within the query "Q(a, c, e):- A(a, b), B(b, c), C(c, d), D(d, e)" has sub-cores {1, 2} and {3}.

- [ ] [November 1] Write test to check if a plan is complete or not by implementing backtracking algorithm for exact cover problem.

- [ ] [November 8] Compute index maintenance cost: use the dataframe class to estimate the index storage costs. 

- [ ] [November 15] Compute query evaluation cost: use the dataframe class to estimate cost for each step of a query plan

- [ ] [November 22] Generate candidate index and view tuples: given a query "Q[k1, k2, c]\(d, e\):- K(k1, d), K(k2, d), E(e, d), C(e, c)", exhaustively generate indexes and the corresponding view tuples for each sub-expression.

- [ ] [November 29] Merge join isomorphic candidate indexes. Eg:- merge "I1(e):- C(e, phone)" and "I2(e):- C(e, email)" to obtain "I12(e, c):- C(e, c)". Project out head variables to obtain new candidate indexes from existing candidates. Eg:- starting from an index "I(x, y, z, w):- A(x, y), B(y, z), C(z, w)" and its view tuples, obtain the index "'I(x, z, w):- A(x, y), B(y, z), C(z, w)" and its updated view tuples.

- [ ] [December 6] Implement cost based pruning of indexes. 
	1. If an index has too high maintenance cost then don't consider it
	2. Compute lower and upper bounds on the cost contribution of each view tuple. If the lower bound of cost contribution of a view tuple is too high, then don't consider it.

- [ ] [December 13] Implement weighted set cover based optimizer

- [ ] [December 20] Implement branch and bound based optimizer


