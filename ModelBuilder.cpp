#include <iostream>
#include <unordered_map>
#include <algorithm>

#include <meddly.h>

#include "ModelBuilder.h"
#include "System.h"

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
	else if (strcmp(heuristic, "BD") == 0) {
		p.setSinkDown();
	}
	else if (strcmp(heuristic, "BU") == 0) {
		p.setBubbleUp();
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

	_mdd_forest = _domain->createForest(false, MEDDLY::forest::BOOLEAN, MEDDLY::forest::MULTI_TERMINAL, p);

	limit = 100000;

	create_vars();
}

void ModelBuilder::create_vars()
{
	int var = 0;
	for (const auto& input : _model.inputs()) {
		var++;
		_vars.insert({input, var});
	}

	assert(var <= _num_vars);

	var = _num_vars;
	for (auto& gate : _model.gates()){
		for(auto& input : gate.inputs()) {
			string name = get_name(input);
			if(_vars.find(name) == _vars.end()) {
				var++;
				_vars.insert({ name, var });
			}
		}

		string name = get_name(gate.output());
		if(_vars.find(name) == _vars.end()) {
			var++;
			_vars.insert({ name, var });
		}
	}

	for (auto& latch : _model.latches()){
		string name = get_name(latch.input());
		if (_vars.find(name) == _vars.end()) {
			var++;
			_vars.insert({name, var});
		}

		name = get_name(latch.output());
		if (_vars.find(name) == _vars.end()) {
			var++;
			_vars.insert({ name, var });
		}
	}

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
	vector<const Gate*> gate_ptrs(_bdds.size(), nullptr);
	for (auto& gate : _model.gates()) {
		gate_ptrs[get_var(get_name(gate.output()))] = &gate;
	}

	// Determine the building order
	vector<int> order;
	vector<int> refs(_bdds.size(), 0);
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

//		output_status(cout);

		if(_mdd_forest->getCurrentNumNodes() > limit) {
			optimize();
			limit = _mdd_forest->getCurrentNumNodes() * 2;
		}
	}

//	std::map<int, bool> dup;
	for(auto& output : _model.outputs()) {
		int var = get_var(output);
		_output_bdds.insert({output, _bdds[var]});
//		cout << "(" << output << ", " <<_bdds[var].getNodeCount() << ")";
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

void ModelBuilder::clean_up()
{
	if (_domain != nullptr) {
		_vars.clear();
		MEDDLY::destroyDomain(_domain);
		_domain = nullptr;
		_mdd_forest = nullptr;
	}
}

void ModelBuilder::output_status(ostream& out)
{
	static_cast<MEDDLY::expert_forest*>(_mdd_forest)->removeAllComputeTableEntries();

	out << "Model: " << _model.name() << endl;
	out << "Peak Node: " << _mdd_forest->getPeakNumNodes() << endl;
	out << "Total Node: " << _mdd_forest->getCurrentNumNodes() << endl;
}
