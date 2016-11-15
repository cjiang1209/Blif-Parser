#include "Rebuilder.h"

Rebuilder::Rebuilder(const Model& model)
	: ModelBuilder(model)
{
}

void Rebuilder::reorder(int* order)
{
	MEDDLY::forest* target_mdd_forest = _domain->createForest(false, MEDDLY::forest::BOOLEAN, MEDDLY::forest::MULTI_TERMINAL);
	static_cast<MEDDLY::expert_forest*>(target_mdd_forest)->reorderVariables(order);
	MEDDLY::global_rebuilder gr(static_cast<MEDDLY::expert_forest*>(_mdd_forest),
			static_cast<MEDDLY::expert_forest*>(target_mdd_forest));

	for (auto& entry : _output_bdds) {
		cout << "Rebuilding " << entry.first << endl;
		entry.second = gr.rebuild(entry.second);
		gr.clearCache();
	}

	_mdd_forest = target_mdd_forest;
}
