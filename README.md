# Everything Search

Implementation of everything search system. The case study below describes the details of how the system works. At the end a road map identifies different components to be implemented and shows the status of completion.

## Example: Entity Search

In this case study, we consider the entity search queries like "amazon customer service #emailid" where the goal is to retrieve entities of category "emailid" that appear in the same document as the keywords "amazon", "customer" and "service".

### Step-1: specify base relations

Initially, a user must list all the base relations along with a representative sample of it tuples. For this example, consider the following base relations:

1. "K" with columns ["k", "d"] of types [string, int] respectively where "k" stands for keywords and "d" stands for document-id. A tuple (k, d) in K signifies that the keyword 'k' is present in the document 'd'

2. "E" with columns ["e", "d"] of types [int, int] where "e" stands for entity id and "d" is document id. 

3. "C" with columns ["e", "c"] of types [int, string] where "e" is entity-id and "c" is a category to which the entity belongs to.

Along with the metadata, we also require a sample of tuples for each table. This sampled data is used to obtain cost estimates. For instance, say 10,000 tuples may be selected randomly out of the original base relation K to obtain a representative sample. For each base relation, we assume that a csv file contains this sampled data.

### Step-2: specify query workload

Next the user specifies a set of query workload along with the normalized weights. For instance, following is an example query workload:

1. Q1[k1, k2]\(d, e\) :- K(k1, d); K(k2, d); E(e, d); C(e, "phone")
2. Q2[k1, k2]\(d, e\) :- K(k1, d); K(k2, d); E(e, d); C(e, "email")

The first query is only concerned with the category "email" and the second query cares about phone numbers.

### Step-3: generate candidate indexes and view tuples

In this step, we generate candidate indexes to be stored. For instance, "INV[k]\(d\) :- K(k, d)" is a candidate index that maps keywords to the documents containing them. We generate candidate in the following 4 steps:

1. Enumerate all subsets of goals within a workload query with size <= k (usually k=3). Keep only the subsets that are connected, i.e. no cross joins are present. Merge all isomorphic subsets to obtain a cnadidate index. All instances of a candidate index within a query form their view tuples. For instance, "INV[k1]\(d\)" and "INV[k2]\(d\)" are the view tuples of the above candidate index INV within both the queries covering one of the two goals in them.

2. Merge indexes that have the same join structure and differ only in the selection conditions. The merged index must be general enough so that it can be used in place of both of its indexes. For instance, the two indexes "I1[d]\(e\):-E(e,d); C(e, "phone")" and "I2[d]\(e\):-E(e,d); C(e, "email")" are merged into the index "I[d]\(c, e\):-E(e,d); C(e, c)". 

3. Remove columns from an index to obtain more candidate indexes. For instance, consider the index "I[k]\(d, e\):-K(k,d); E(e,d); C(e,"phone")". Its view tuples "I[k1]\(d,e\)" has three sub-cores, within Q2, \{\{1\}, \{3\}, \{4\}\} corresponding to the goals covered by it. After we drop the column "e" from "I", we obtain the new candidate index "I'[k]\(d\):-K(k,d); E(e,d); C(e,"phone")". The recomputed sub-cores for the updated view tuple "I'[k1]\(d,e\)" is \{\{1\}\}. In this step, we drop columns from an index as long as we obtain a unique set of subcores for its view tuples. 

4. For every candidate index, we consider all ways of dividing column into input columns and output columns. Different configuration may be initially pruned based on cost to reduce the size of the candidate index set.


### Step-4: optimize for a least cost query plan

Select the optimal set of indexes to store and the corresponding query plans. To compute the cost of a plan, we run the plan on the sampled base relations given as an input in the beginning.


## Roadmap

Implement the components:

1. Candidate generator
	- [x] step-1: generating subsets of goals and merging the isomorphic ones
	- [] step-2: merge indexes that have the same join structure
	- [] step-3: drop columns from an index to generate new candidates
	- [] step-4: dividing columns into input and output for a given candidate index
2. Cost model:
	- [] class to represent the intermediate results of query execution
	- [] implement algorithms for performing selects, projects and joins on the intermediate results
	- [] implement cost computation for a query plan
3. Optimization algorithm:
	- [] implement set cover based heuristic
	- [] implement branch and bound based search

