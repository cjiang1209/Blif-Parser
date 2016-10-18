#ifndef INDEXSETMODELBUILDER_H_
#define INDEXSETMODELBUILDER_H_

#include <unordered_map>
#include <meddly.h>

#include "ModelBuilder.h"

class IndexSetModelBuilder : public ModelBuilder
{
protected:
	MEDDLY::forest* _evplus_forest;
//	vector<MEDDLY::dd_edge> _evplus_bdds;
	unordered_map<string, MEDDLY::dd_edge> _output_evplus_bdds;

public:
	IndexSetModelBuilder(const Model& model);

	void construct_index_sets();

	virtual void get_variable_order(int* order);
	virtual void swap_adjacent_variable(int lev);
	virtual void reorder(int* order);

	virtual void output_status(ostream& out);

	virtual void clean_up();
};

#endif
