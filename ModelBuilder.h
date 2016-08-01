#ifndef MODELBUILDER_H_
#define MODELBUILDER_H_

#include <vector>
#include <unordered_map>
#include <meddly.h>
#include <meddly_expert.h>

#include "Model.h"

using namespace std;

class ModelBuilder
{
private:
	long limit;

	const Model& _model;
	unordered_map<string, int> _vars;
	int _num_vars;
	MEDDLY::domain* _domain;
	MEDDLY::forest* _mdd_forest;
	vector<MEDDLY::dd_edge> _bdds;
	unordered_map<string, MEDDLY::dd_edge> _output_bdds;

	string get_name(string signal);
	int get_var(string name);

	void build_input(int var);
	void build_gate(const Gate& gate, vector<int>& refs);

	void examine_dependency(const Gate* gate, const vector<const Gate*>& gates, vector<int>& order, vector<int>& exists);

	bool is_complement(string signal);

public:
	ModelBuilder(const Model& model);
	void create_vars();
	void initialize(const char* heuristic);
	void build();
	void build_model();
	void optimize();
	void optimize(int top, int bottom);
	void clean_up();

	void set_num_vars(int num_vars);
	int num_vars() const;
	int actual_num_vars() const;
	void reset_stat();

	void get_variable_order(int* order);
	void swap_adjacent_variable(int lev);
	void reorder(int* order);

	int num_nodes() const;

	void output_status(ostream& out);
};

inline string ModelBuilder::get_name(string signal)
{
	if(signal[0] == '-'){
		return signal.substr(1, signal.length()-1);
	}
	else{
		return signal;
	}
}

inline int ModelBuilder::get_var(string name)
{
	assert(_vars.find(name) != _vars.end());
	return _vars.find(name)->second;
}

inline void ModelBuilder::set_num_vars(int num_vars)
{
	assert(num_vars >= actual_num_vars());
	_num_vars = num_vars;
}

inline int ModelBuilder::num_vars() const
{
	return _num_vars;
}

inline int ModelBuilder::actual_num_vars() const
{
	return _model.inputs().size();
}

inline void ModelBuilder::get_variable_order(int* order)
{
	MEDDLY::expert_forest* f = static_cast<MEDDLY::expert_forest*>(_mdd_forest);
	for(int i = 1; i <= _num_vars; i++) {
		order[i] = f->getVarByLevel(i);
	}
}

inline void ModelBuilder::swap_adjacent_variable(int lev)
{
	static_cast<MEDDLY::expert_forest*>(_mdd_forest)->swapAdjacentVariables(lev);
}

inline void ModelBuilder::reorder(int* order)
{
	static_cast<MEDDLY::expert_forest*>(_mdd_forest)->reorderVariables(order);
}

inline int ModelBuilder::num_nodes() const
{
	return _mdd_forest->getCurrentNumNodes();
}

inline void ModelBuilder::reset_stat()
{
	_mdd_forest->resetPeakNumNodes();
	_mdd_forest->resetPeakMemoryUsed();
}

inline bool ModelBuilder::is_complement(string signal)
{
	return signal[0] == '-';
}

#endif
