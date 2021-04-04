# Everything Search - Code Structure

test_application() function in test/tests.cpp is an end-to-end example. Different components are implemented in the following classes:
- BaseRelation (base_relation.h): class to store the base relations
- Expression (expression.h): class for conjunctive expressions
- Index, Query, ViewTuple, Plan (query.h): same as in the paper
- ContainmentMap (containment_map.h): compute and store containment maps between conjunctive queries (see: http://pages.cs.wisc.edu/~paris/cs838-s16/lecture-notes/lecture2.pdf)
- DataFrame (dataframe.h): class to store results of execution of an expression. Expression execution is needed during view tuples construction (see definition of view tuples in: https://www.ics.uci.edu/~chenli/pub/SIGMOD01-aquv.pdf)
- CardinalityEstimator (expression.h): compute cardinalities of variables in an expression
- Application (application.h): over arching class that brings everything together
	* Application::Design: index design
	* Application::generate_candidates: generates candidate indexes and view tuples
	* Application::optimize_wsc_standalone: function for two-level greedy algorithm described in the paper
	* Application::optimize: branch and bound algorithm for guaranteed optimal solution in doubly exponential time
	* Application::optimize_wsc: weighted set cover subroutine within Application::optimize
