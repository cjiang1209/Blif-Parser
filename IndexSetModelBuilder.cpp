#include <meddly.h>
#include <meddly_expert.h>

#include "IndexSetModelBuilder.h"

IndexSetModelBuilder::IndexSetModelBuilder(const Model& model)
	: ModelBuilder(model), _evplus_forest(nullptr)
{
}

void IndexSetModelBuilder::construct_index_sets()
{
	cout << "Constructing Index Sets of BDDs..." << endl;

	MEDDLY::forest::policies p(false);
	//p.setQuasiReduced();
	_evplus_forest = _domain->createForest(false, MEDDLY::forest::INTEGER, MEDDLY::forest::INDEX_SET, p);
	int* order = new int[num_vars() + 1];
	order[0] = 0;
	static_cast<MEDDLY::expert_forest*>(_mdd_forest)->getVariableOrder(order);

	for (int i = 0; i < num_vars() + 1; i++) {
		cout << order[i] << ", ";
	}
	cout << endl;

	static_cast<MEDDLY::expert_forest*>(_evplus_forest)->reorderVariables(order);
	delete[] order;

	MEDDLY::FILE_output out(stdout);
	for (const auto& bdd : _output_bdds) {
		cout << bdd.first << " (MDD): " << bdd.second.getNodeCount() << " Card: " << bdd.second.getCardinality() << endl;
		MEDDLY::dd_edge evplus_bdd(_evplus_forest);
//		bdd.second.show(out, 2);
		MEDDLY::apply(MEDDLY::CONVERT_TO_INDEX_SET, bdd.second, evplus_bdd);
		cout << bdd.first << " (EV+): " << evplus_bdd.getNodeCount() << " Card: " << evplus_bdd.getCardinality() << endl;
//		evplus_bdd.show(out, 2);
		_output_evplus_bdds.emplace(bdd.first, evplus_bdd);
	}
	_output_bdds.clear();
}

void IndexSetModelBuilder::get_variable_order(int* order)
{
	MEDDLY::expert_forest* f = static_cast<MEDDLY::expert_forest*>(_evplus_forest);
//	for(int i = 1; i <= _num_vars; i++) {
//		order[i] = f->getVarByLevel(i);
//	}

	f->getVariableOrder(order);
}

void IndexSetModelBuilder::swap_adjacent_variable(int lev)
{
	static_cast<MEDDLY::expert_forest*>(_evplus_forest)->swapAdjacentVariables(lev);
}

void IndexSetModelBuilder::reorder(int* order)
{
	MEDDLY::expert_forest* f = static_cast<MEDDLY::expert_forest*>(_evplus_forest);
	f->resetPeakNumNodes();
	f->resetPeakMemoryUsed();
	f->reorderVariables(order);
}

void IndexSetModelBuilder::output_status(ostream& out)
{
	out << "Model: " << _model.name() << endl;
	if (_mdd_forest != nullptr) {
		static_cast<MEDDLY::expert_forest*>(_mdd_forest)->removeAllComputeTableEntries();
		out << "MDD Peak Node: " << _mdd_forest->getPeakNumNodes() << endl;
		out << "MDD Total Node: " << _mdd_forest->getCurrentNumNodes() << endl;
	}
	if (_evplus_forest != nullptr) {
		static_cast<MEDDLY::expert_forest*>(_evplus_forest)->removeAllComputeTableEntries();
		out << "EV+ Peak Node: " << _evplus_forest->getPeakNumNodes() << endl;
		out << "EV+ Total Node: " << _evplus_forest->getCurrentNumNodes() << endl;
	}
}

void IndexSetModelBuilder::clean_up()
{
	ModelBuilder::clean_up();
	_evplus_forest = nullptr;
	_output_evplus_bdds.clear();
}
