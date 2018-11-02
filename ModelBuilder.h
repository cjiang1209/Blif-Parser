#ifndef MODELBUILDER_H_
#define MODELBUILDER_H_

#include <random>
#include <vector>
#include <unordered_map>
#include <meddly.h>
#include <meddly_expert.h>

#include "Model.h"

using namespace std;

class ModelBuilder
{
protected:
	static default_random_engine RANDOM_ENGINE;

	long limit;

	const Model& _model;
	unordered_map<string, int> _vars;
	// Number of input signals
	int _num_vars;
	// Number of input, internal and output signals
	int _num_signals;
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

	int num_signals() const;

	virtual void get_variable_order(int* order);
	virtual void swap_adjacent_variable(int lev);
	virtual void reorder(int* order);

	// Static ordering heuristics
	void dfs_order(int* order);

	void fanin_order(int* order);
	// Transient fanin
	void recursive_tfi_depth(const Gate* gate, const vector<const Gate*>& gates, vector<int>& depths);

	int num_nodes() const;

	void transform_ESRBDD() const;
	void transform_ZDD_and_CZDD() const;
	void transform_CBDD() const;
	void transform_TaggedBDD() const;
	long getNodeCount(MEDDLY::forest* forest, const unordered_map<string, MEDDLY::dd_edge>& dds) const;
	long getNodeCount(MEDDLY::forest* forest, const vector<MEDDLY::dd_edge>& dds) const;

	void countEdgeLabelsForESR(MEDDLY::forest* forest, const vector<MEDDLY::dd_edge>& dds) const;

	virtual void output_status(ostream& out);
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

inline int ModelBuilder::num_signals() const
{
	return _num_signals;
}

inline int ModelBuilder::num_nodes() const
{
	return _mdd_forest->getCurrentNumNodes();
}

inline void ModelBuilder::reset_stat()
{
	_mdd_forest->resetPeakNumNodes();
	//_mdd_forest->resetPeakMemoryUsed();
}

inline bool ModelBuilder::is_complement(string signal)
{
	return signal[0] == '-';
}

#endif
