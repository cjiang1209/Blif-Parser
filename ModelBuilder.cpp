#include <iostream>
#include <unordered_map>
#include <algorithm>

#include <meddly.h>

#include "ModelBuilder.h"
#include "System.h"

default_random_engine ModelBuilder::RANDOM_ENGINE;

ModelBuilder::ModelBuilder(const Model& model)
	: _model(model), _domain(nullptr), _mdd_forest(nullptr)
{
}

void ModelBuilder::build()
{
	initialize("LARC");
	build_model();
}

void ModelBuilder::initialize(const char* heuristic)
{
	clean_up();

	cout << "# Variables: " << num_vars() << " (Actual: " << actual_num_vars() << ")" << endl;
	cout << "# Inputs: " << _model.inputs().size() << endl;
	cout << "# Outputs: " << _model.outputs().size() << endl;
	cout << "# Gates: " << _model.gates().size() << endl;

	int* bounds = new int[num_vars()];
	std::fill_n(bounds, num_vars(), 2);

	_domain = MEDDLY::createDomainBottomUp(bounds, num_vars());

	delete[] bounds;

	MEDDLY::forest::policies p(false);

	if (strcmp(heuristic, "LI") == 0) {
		p.setLowestInversion();
	}
	else if (strcmp(heuristic, "HI") == 0) {
		p.setHighestInversion();
	}
	else if (strcmp(heuristic, "SD") == 0) {
		p.setSinkDown();
	}
	else if (strcmp(heuristic, "BU") == 0) {
		p.setBringUp();
	}
	else if (strcmp(heuristic, "LC") == 0) {
		p.setLowestCost();
	}
	else if (strcmp(heuristic, "LM") == 0) {
		p.setLowestMemory();
	}
	else if (strcmp(heuristic, "RAN") == 0) {
		p.setRandom();
	}
	else if (strcmp(heuristic, "LARC") == 0) {
		p.setLARC();
	}
	else if (strcmp(heuristic, "REBUILD") == 0) {
		;
	}
	else {
		exit(-1);
	}

	_mdd_forest = _domain->createForest(false, MEDDLY::forest::BOOLEAN, MEDDLY::forest::MULTI_TERMINAL, p);

	limit = 100000;

	create_vars();
}

void ModelBuilder::create_vars()
{
	int var = 0;
	for (const auto& input : _model.inputs()) {
		var++;
		_vars.emplace(input, var);
	}

	assert(var <= _num_vars);

	var = _num_vars;
	for (auto& gate : _model.gates()){
		for(auto& input : gate.inputs()) {
			string name = get_name(input);
			if(_vars.find(name) == _vars.end()) {
				var++;
				_vars.emplace(name, var);
			}
		}

		string name = get_name(gate.output());
		if(_vars.find(name) == _vars.end()) {
			var++;
			_vars.emplace(name, var);
		}
	}

	for (auto& latch : _model.latches()){
		string name = get_name(latch.input());
		if (_vars.find(name) == _vars.end()) {
			var++;
			_vars.emplace(name, var);
		}

		name = get_name(latch.output());
		if (_vars.find(name) == _vars.end()) {
			var++;
			_vars.emplace(name, var);
		}
	}

	_num_signals = var;

	MEDDLY::dd_edge bdd(_mdd_forest);
	_mdd_forest->createEdge(false, bdd);
	_bdds.assign(1 + var, bdd);
}

void ModelBuilder::examine_dependency(const Gate* gate, const vector<const Gate*>& gates, vector<int>& order, vector<int>& refs)
{
	assert(gate != nullptr);

	for (const auto& input : gate->inputs()) {
		int input_var = get_var(input);
		if (refs[input_var] > 0) {
			refs[input_var]++;
			continue;
		}
		else if (gates[input_var] == nullptr) {
			order.push_back(input_var);
			refs[input_var]++;
		}
		else {
			examine_dependency(gates[input_var], gates, order, refs);
		}
	}

	int output_var = get_var(get_name(gate->output()));
	order.push_back(output_var);
	refs[output_var]++;
}

void ModelBuilder::build_model()
{
	vector<const Gate*> gate_ptrs(num_signals() + 1, nullptr);
	for (auto& gate : _model.gates()) {
		gate_ptrs[get_var(get_name(gate.output()))] = &gate;
	}

	// Determine the building order
	vector<int> order;
	vector<int> refs(num_signals() + 1, 0);
	for (auto& output : _model.outputs()) {
		if(gate_ptrs[get_var(get_name(output))] != nullptr) {
			examine_dependency(gate_ptrs[get_var(get_name(output))], gate_ptrs, order, refs);
		}
	}

	MEDDLY::dd_edge bdd(_mdd_forest);
	_mdd_forest->createEdge(false, bdd);
	_bdds.assign(_bdds.size(), bdd);

	for(auto& i : order) {
		if(gate_ptrs[i] == nullptr) {
			build_input(i);
		}
		else {
			build_gate(*gate_ptrs[i], refs);
		}

		output_status(cout);

		if(_mdd_forest->getCurrentNumNodes() > limit) {
			optimize();
			limit = _mdd_forest->getCurrentNumNodes() * 2;
		}
	}

	for(auto& output : _model.outputs()) {
		int var = get_var(output);
		_output_bdds.emplace(output, _bdds[var]);
	}
	cout << endl;

	_bdds.clear();

//	vector<MEDDLY::dd_edge> bdds;
//	for(auto& gate : model.gates()) {
//		MEDDLY::dd_edge bdd1(_mdd_forest);
//		build_gate(gate, bdd1);
//		bdds.push_back(bdd1);
//
////		MEDDLY::apply(MEDDLY::INTERSECTION, bdd, bdd1, bdd);
//
//		if(_mdd_forest->getCurrentNumNodes() > limit) {
//			optimize();
//			limit = std::max(limit, _mdd_forest->getCurrentNumNodes()) * 1.5;
//		}
//	}

//	int size = bdds.size();
//	while(size != 1) {
////		cout << size << endl;
//		size /= 2;
//		int middle = (size + 1) / 2;
//		for(int i = 0; i < size; i++) {
//			MEDDLY::apply(MEDDLY::INTERSECTION, bdds[i], bdds[middle + i], bdds[i]);
//
//			if(_mdd_forest->getCurrentNumNodes() > limit) {
//				optimize();
//				limit = std::max(limit, _mdd_forest->getCurrentNumNodes()) * 1.5;
//			}
//		}
//		bdds.resize(size);
//	}
//
//	bdd = bdds.front();
}

void ModelBuilder::build_input(int var)
{
	cout << "Building Input " << var << endl;

	MEDDLY::dd_edge bdd(_mdd_forest);
	const bool POS_TERMS[] = {false, true};
	_mdd_forest->createEdgeForVar(var, false, POS_TERMS, bdd);
	_bdds[var] = bdd;
}

void ModelBuilder::build_gate(const Gate& gate, vector<int>& refs)
{
	cout << "Building Gate " << gate.name() << endl;

	MEDDLY::dd_edge bdd(_mdd_forest);
	_mdd_forest->createEdge(false, bdd);

//	MEDDLY::dd_edge false_bdd(_mdd_forest);
//	_mdd_forest->createEdge(false, false_bdd);

	for (const auto& row : gate.rows()) {
		MEDDLY::dd_edge bdd1(_mdd_forest);
		_mdd_forest->createEdge(true, bdd1);

		for(auto& signal : row) {
			int input_var = _vars.find(get_name(signal))->second;

			MEDDLY::dd_edge bdd2 = _bdds[input_var];
//			assert(bdd2 != false_bdd);
			if(is_complement(signal)) {
				MEDDLY::apply(MEDDLY::COMPLEMENT, bdd2, bdd2);
			}
			MEDDLY::apply(MEDDLY::INTERSECTION, bdd1, bdd2, bdd1);
		}

		MEDDLY::apply(MEDDLY::UNION, bdd, bdd1, bdd);
	}

	for (auto& input : gate.inputs()) {
		int input_var = _vars.find(get_name(input))->second;
		assert(refs[input_var] > 0);
		refs[input_var]--;
		if (refs[input_var] == 0) {
			// Will not be used any more
			_mdd_forest->createEdge(false, _bdds[input_var]);
		}
	}

	int output_var = _vars.find(get_name(gate.output()))->second;
	if (is_complement(gate.output())) {
		MEDDLY::apply(MEDDLY::COMPLEMENT, bdd, bdd);
	}
	_bdds[output_var] = bdd;

	cout << _mdd_forest->getCurrentNumNodes() << endl;
}

void ModelBuilder::optimize()
{
	optimize(_domain->getNumVariables(), 1);
}

void ModelBuilder::optimize(int top, int bottom)
{
	cout << "Optimize: " << endl;
	int num = _mdd_forest->getCurrentNumNodes();
	double start_time = get_cpu_time();
	((MEDDLY::expert_forest*)_mdd_forest)->dynamicReorderVariables(top, bottom);
	double end_time = get_cpu_time();
	cout << num << " -> " << _mdd_forest->getCurrentNumNodes() << endl;
	cout << (end_time - start_time) << " s" << endl;
}

void ModelBuilder::get_variable_order(int* order)
{
	MEDDLY::expert_forest* f = static_cast<MEDDLY::expert_forest*>(_mdd_forest);
	for(int i = 1; i <= _num_vars; i++) {
		order[i] = f->getVarByLevel(i);
	}
}

void ModelBuilder::swap_adjacent_variable(int lev)
{
	static_cast<MEDDLY::expert_forest*>(_mdd_forest)->swapAdjacentVariables(lev);
}

void ModelBuilder::reorder(int* order)
{
	MEDDLY::expert_forest* f = static_cast<MEDDLY::expert_forest*>(_mdd_forest);
	f->resetPeakNumNodes();
//	f->resetPeakMemoryUsed();
	f->reorderVariables(order);
}

void ModelBuilder::dfs_order(int* order)
{
	vector<const Gate*> gate_ptrs(num_signals() + 1, nullptr);
	for(auto& gate : _model.gates()) {
		gate_ptrs[get_var(get_name(gate.output()))] = &gate;
	}

	vector<int> todo;
	for (const auto& output : _model.outputs()) {
		// Primary outputs
		int output_var = get_var(get_name(output));
		if (gate_ptrs[output_var] != nullptr) {
			todo.push_back(output_var);
		}
	}
	std::shuffle(todo.begin(), todo.end(), RANDOM_ENGINE);

	vector<bool> visited(num_signals() + 1, false);

	order[0] = 0;
	int last = 1;
	while (!todo.empty()) {
		int signal = todo.back();
		todo.pop_back();

		if (visited[signal]) {
			continue;
		}
		visited[signal] = true;

		vector<string> inputs = gate_ptrs[signal]->inputs();
		std::shuffle(inputs.begin(), inputs.end(), RANDOM_ENGINE);

		for (const auto& input : inputs) {
			int input_var = get_var(input);
			if (visited[input_var]) {
				continue;
			}
			else if (gate_ptrs[input_var] == nullptr) {
				// Primary input
				order[last++] = input_var;
				visited[input_var] = true;
			}
			else {
				todo.push_back(input_var);
			}
		}
	}
}

void ModelBuilder::fanin_order(int* order)
{
	vector<const Gate*> gate_ptrs(num_signals() + 1, nullptr);
	for (const auto& gate : _model.gates()) {
		gate_ptrs[get_var(get_name(gate.output()))] = &gate;
	}

	vector<int> depths(num_signals() + 1, -1);
	for (int i = 1; i <= actual_num_vars(); i++) {
		depths[i] = 0;
	}

	for (auto& output : _model.outputs()) {
		int output_var = get_var(get_name(output));
		if (gate_ptrs[output_var] != nullptr && depths[output_var] == -1) {
			recursive_tfi_depth(gate_ptrs[output_var], gate_ptrs, depths);
		}
	}

//	for(auto& d : depth) {
//		cout << d <<", ";
//	}
//	cout << endl;

	auto greater_than = [&depths](int x, int y) {
		return depths[x] > depths[y];
	};

	vector<bool> visited(num_signals() + 1, false);
	vector<int> todo;

	vector<int> sorted;
	for(auto& output : _model.outputs()) {
		int output_var = get_var(get_name(output));
		if (gate_ptrs[output_var] != nullptr) {
			sorted.push_back(output_var);
		}
	}
	std::sort(sorted.begin(), sorted.end(), greater_than);

	todo.assign(sorted.begin(), sorted.end());

	order[0] = 0;
	int last = 1;
	while (!todo.empty()) {
		sorted.clear();

		int signal = todo.back();
		todo.pop_back();

		if (visited[signal]) {
			continue;
		}
		visited[signal] = true;

		const Gate* gate = gate_ptrs[signal];
		for (const auto& input : gate->inputs()) {
			int input_var = get_var(input);
			if (visited[input_var]) {
				continue;
			}
			else if(gate_ptrs[input_var] == nullptr) {
				// Primary input
				order[last++] = input_var;
				visited[input_var] = true;
			}
			else {
				sorted.push_back(input_var);
			}
		}
		std::sort(sorted.begin(), sorted.end(), greater_than);
		for (const auto& i : sorted) {
			todo.push_back(i);
		}
	}
}

void ModelBuilder::recursive_tfi_depth(const Gate* gate, const vector<const Gate*>& gates, vector<int>& depths)
{
	assert(gate != nullptr);

	int output_var = get_var(get_name(gate->output()));
	int max = -1;
	for (const auto& input : gate->inputs()) {
		int input_var = get_var(input);
		if (depths[input_var] != -1) {
			max = std::max(depths[input_var] + 1, max);
		}
		else {
			recursive_tfi_depth(gates[input_var], gates, depths);
			max = std::max(depths[input_var] + 1, max);
		}
	}

	depths[output_var] = max;
}

void ModelBuilder::transform_ESRBDD() const
{
	MEDDLY::forest* esrbdd_forest = _domain->createForest(false, MEDDLY::forest::BOOLEAN,
			MEDDLY::forest::ESR, MEDDLY::forest::policies(false));
	vector<MEDDLY::dd_edge> esrbdds;

	int nodeCountBDD = 0;
	int edgeCountBDD = 0;
	int nodeCountESRBDD = 0;
	int edgeCountESRBDD = 0;
	for (const auto& _output : _output_bdds) {
		cout << _output.first << ": ";
		{
			int nodeCount = _output.second.getNodeCount();
			int edgeCount = _output.second.getEdgeCount();
			nodeCountBDD += nodeCount;
			edgeCountBDD += edgeCount;
			cout << nodeCount << ", " << edgeCount;
		}
		cout << " => ";
		{
			MEDDLY::dd_edge esrbdd(esrbdd_forest);
			MEDDLY::apply(MEDDLY::COPY, _output.second, esrbdd);

			int nodeCount = esrbdd.getNodeCount();
			int edgeCount = esrbdd.getEdgeCount();
			nodeCountESRBDD += nodeCount;
			edgeCountESRBDD += edgeCount;
			cout << nodeCount << ", " << edgeCount;

			esrbdds.push_back(esrbdd);
		}
		cout << endl;
	}
	cout << "\nTotal: " << nodeCountBDD << ", " << edgeCountBDD
			<< " => " << nodeCountESRBDD << ", " << edgeCountESRBDD
			<< endl;

	cout << "\nBDD Total Node: " << getNodeCount(_mdd_forest, _output_bdds) << endl;
	cout << "ESRBDD Total Node: " << getNodeCount(esrbdd_forest, esrbdds) << endl;
	cout << "ESRBDD Active Node: " << esrbdd_forest->getCurrentNumNodes() << endl;

	countEdgeLabelsForESR(esrbdd_forest, esrbdds);
}

void ModelBuilder::transform_ZDD_and_CZDD() const
{
	MEDDLY::forest::policies p(false);
	p.setZeroSuppressionReduced();
	MEDDLY::forest* zdd_forest = _domain->createForest(false, MEDDLY::forest::BOOLEAN,
			MEDDLY::forest::MULTI_TERMINAL, p);
	vector<MEDDLY::dd_edge> zdds;

	{
		int nodeCountBDD = 0;
		int edgeCountBDD = 0;
		int nodeCountZDD = 0;
		int edgeCountZDD = 0;
		for (const auto& _output : _output_bdds) {
			cout << _output.first << ": ";
			{
				int nodeCount = _output.second.getNodeCount();
				int edgeCount = _output.second.getEdgeCount();
				nodeCountBDD += nodeCount;
				edgeCountBDD += edgeCount;
				cout << nodeCount << ", " << edgeCount;
			}
			cout << " => ";
			{
				MEDDLY::dd_edge zdd(zdd_forest);
				MEDDLY::apply(MEDDLY::COPY, _output.second, zdd);

				int nodeCount = zdd.getNodeCount();
				int edgeCount = zdd.getEdgeCount();
				nodeCountZDD += nodeCount;
				edgeCountZDD += edgeCount;
				cout << nodeCount << ", " << edgeCount;

				zdds.push_back(zdd);
			}
			cout << endl;
			cout << "ZDD Total Node: " << zdd_forest->getCurrentNumNodes() << endl;
		}
		cout << "\nTotal: " << nodeCountBDD << ", " << edgeCountBDD
				<< " => " << nodeCountZDD << ", " << edgeCountZDD
				<< endl;

		cout << "\nBDD Total Node: " << getNodeCount(_mdd_forest, _output_bdds) << endl;
		cout << "ZDD Total Node: " << getNodeCount(zdd_forest, zdds) << endl;
		cout << "ZDD Active Node: " << zdd_forest->getCurrentNumNodes() << endl;
	}

	MEDDLY::forest* czdd_forest = _domain->createForest(false, MEDDLY::forest::BOOLEAN,
			MEDDLY::forest::CZDD, MEDDLY::forest::policies(false));
	vector<MEDDLY::dd_edge> czdds;

	{
		int nodeCountZDD = 0;
		int edgeCountZDD = 0;
		int nodeCountCZDD = 0;
		int edgeCountCZDD = 0;
		int i = 0;
		for (const auto& _output : _output_bdds) {
			cout << _output.first << ": ";
			{
				int nodeCount = zdds[i].getNodeCount();
				int edgeCount = zdds[i].getEdgeCount();
				nodeCountZDD += nodeCount;
				edgeCountZDD += edgeCount;
				cout << nodeCount << ", " << edgeCount;
			}
			cout << " => ";
			{
				MEDDLY::dd_edge czdd(czdd_forest);
				MEDDLY::apply(MEDDLY::COPY, zdds[i], czdd);

				int nodeCount = czdd.getNodeCount();
				int edgeCount = czdd.getEdgeCount();
				nodeCountCZDD += nodeCount;
				edgeCountCZDD += edgeCount;
				cout << nodeCount << ", " << edgeCount;

				czdds.push_back(czdd);
			}
			cout << endl;
			cout << "CZDD Total Node: " << zdd_forest->getCurrentNumNodes() << endl;

			i++;
		}
		cout << "\nTotal: " << nodeCountZDD << ", " << edgeCountZDD
				<< " => " << nodeCountCZDD << ", " << edgeCountCZDD
				<< endl;

		cout << "\nZDD Total Node: " << getNodeCount(zdd_forest, zdds) << endl;
		cout << "CZDD Total Node: " << getNodeCount(czdd_forest, czdds) << endl;
		cout << "CZDD Active Node: " << czdd_forest->getCurrentNumNodes() << endl;
	}
}

void ModelBuilder::transform_CBDD() const
{

	MEDDLY::forest* cbdd_forest = _domain->createForest(false, MEDDLY::forest::BOOLEAN,
			MEDDLY::forest::CBDD, MEDDLY::forest::policies(false));
	vector<MEDDLY::dd_edge> cbdds;

	int nodeCountBDD = 0;
	int edgeCountBDD = 0;
	int nodeCountCBDD = 0;
	int edgeCountCBDD = 0;
	for (const auto& _output : _output_bdds) {
		cout << _output.first << ": ";
		{
			int nodeCount = _output.second.getNodeCount();
			int edgeCount = _output.second.getEdgeCount();
			nodeCountBDD += nodeCount;
			edgeCountBDD += edgeCount;
			cout << nodeCount << ", " << edgeCount;
		}
		cout << " => ";
		{
			MEDDLY::dd_edge cbdd(cbdd_forest);
			MEDDLY::apply(MEDDLY::COPY, _output.second, cbdd);

			int nodeCount = cbdd.getNodeCount();
			int edgeCount = cbdd.getEdgeCount();
			nodeCountCBDD += nodeCount;
			edgeCountCBDD += edgeCount;
			cout << nodeCount << ", " << edgeCount;

			cbdds.push_back(cbdd);
		}
		cout << endl;
		cout << "CBDD Total Node: " << cbdd_forest->getCurrentNumNodes() << endl;
	}
	cout << "\nTotal: " << nodeCountBDD << ", " << edgeCountBDD
			<< " => " << nodeCountCBDD << ", " << edgeCountCBDD
			<< endl;

	cout << "\nBDD Total Node: " << getNodeCount(_mdd_forest, _output_bdds) << endl;
	cout << "CBDD Total Node: " << getNodeCount(cbdd_forest, cbdds) << endl;
	cout << "CBDD Active Node: " << cbdd_forest->getCurrentNumNodes() << endl;
}

void ModelBuilder::transform_TaggedBDD() const
{
	MEDDLY::forest* taggedbdd_forest = _domain->createForest(false, MEDDLY::forest::BOOLEAN,
			MEDDLY::forest::TAGGED, MEDDLY::forest::policies(false));
	vector<MEDDLY::dd_edge> taggedbdds;

	int nodeCountBDD = 0;
	int edgeCountBDD = 0;
	int nodeCountTaggedBDD = 0;
	int edgeCountTaggedBDD = 0;
	for (const auto& _output : _output_bdds) {
		cout << _output.first << ": ";
		{
			int nodeCount = _output.second.getNodeCount();
			int edgeCount = _output.second.getEdgeCount();
			nodeCountBDD += nodeCount;
			edgeCountBDD += edgeCount;
			cout << nodeCount << ", " << edgeCount;
		}
		cout << " => ";
		{
			MEDDLY::dd_edge taggedbdd(taggedbdd_forest);
			MEDDLY::apply(MEDDLY::COPY, _output.second, taggedbdd);

			int nodeCount = taggedbdd.getNodeCount();
			int edgeCount = taggedbdd.getEdgeCount();
			nodeCountTaggedBDD += nodeCount;
			edgeCountTaggedBDD += edgeCount;
			cout << nodeCount << ", " << edgeCount;

			taggedbdds.push_back(taggedbdd);
		}
		cout << endl;
		cout << "TaggedBDD Total Node: " << taggedbdd_forest->getCurrentNumNodes() << endl;
	}
	cout << "\nTotal: " << nodeCountBDD << ", " << edgeCountBDD
			<< " => " << nodeCountTaggedBDD << ", " << edgeCountTaggedBDD
			<< endl;

	cout << "\nBDD Total Node: " << getNodeCount(_mdd_forest, _output_bdds) << endl;
	cout << "TaggedBDD Total Node: " << getNodeCount(taggedbdd_forest, taggedbdds) << endl;
	cout << "TaggedBDD Active Node: " << taggedbdd_forest->getCurrentNumNodes() << endl;
}

long ModelBuilder::getNodeCount(MEDDLY::forest* forest, const unordered_map<string, MEDDLY::dd_edge>& dds) const
{
	MEDDLY::node_handle* nodes = new MEDDLY::node_handle[dds.size()];
	int i = 0;
	for (const auto& dd : dds) {
		nodes[i] = dd.second.getNode();
		i++;
	}
	long count = dynamic_cast<MEDDLY::expert_forest*>(forest)->getNodeCount(nodes, dds.size());
	delete[] nodes;
	return count;
}

long ModelBuilder::getNodeCount(MEDDLY::forest* forest, const vector<MEDDLY::dd_edge>& dds) const
{
	MEDDLY::node_handle* nodes = new MEDDLY::node_handle[dds.size()];
	for (int i = 0; i < dds.size(); i++) {
		nodes[i] = dds[i].getNode();
	}
	long count = dynamic_cast<MEDDLY::expert_forest*>(forest)->getNodeCount(nodes, dds.size());
	delete[] nodes;
	return count;
}

void ModelBuilder::countEdgeLabelsForESR(MEDDLY::forest* forest, const vector<MEDDLY::dd_edge>& dds) const
{
	MEDDLY::node_handle* nodes = new MEDDLY::node_handle[dds.size()];
	for (int i = 0; i < dds.size(); i++) {
		nodes[i] = dds[i].getNode();
	}
	long* counts = new long[5];
	dynamic_cast<MEDDLY::expert_forest*>(forest)->countEdgeLabels(nodes, dds.size(), counts);
	for (int i = 0; i < dds.size(); i++) {
		long ev = -1;
		dds[i].getEdgeValue(ev);
		counts[ev]++;
	}
	delete[] nodes;

	std::cout << "BLANK: " << counts[0] << std::endl;
	std::cout << "FULL: " << counts[1] << std::endl;
	std::cout << "ZERO: " << counts[2] << std::endl;
	std::cout << "ONE: " << counts[3] << std::endl;
	std::cout << "FULL(FALSE): " << counts[4] << std::endl;

	delete[] counts;
}

void ModelBuilder::clean_up()
{
	if (_domain != nullptr) {
		_vars.clear();
		MEDDLY::destroyDomain(_domain);
	}
	_domain = nullptr;
	_mdd_forest = nullptr;
	_output_bdds.clear();
}

void ModelBuilder::output_status(ostream& out)
{
	static_cast<MEDDLY::expert_forest*>(_mdd_forest)->removeAllComputeTableEntries();

	out << "Model: " << _model.name() << endl;
	out << "Peak Node: " << _mdd_forest->getPeakNumNodes() << endl;
	out << "Total Node: " << _mdd_forest->getCurrentNumNodes() << endl;
}
