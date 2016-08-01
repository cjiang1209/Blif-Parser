#ifndef MODEL_H_
#define MODEL_H_

#include <string>

#include "Gate.h"
#include "Latch.h"

using namespace std;

class Model {
private:
	string _name;
	vector<string> _inputs;
	vector<string> _outputs;
	vector<Gate> _gates;
	vector<Latch> _latches;

public:
	string& name() { return _name; };
	string name() const { return _name; };
	vector<string>& inputs() { return _inputs; };
	const vector<string>& inputs() const { return _inputs; };
	vector<string>& outputs() { return _outputs; };
	const vector<string>& outputs() const { return _outputs; };

	Gate& add_gate()
	{
		_gates.push_back(Gate());
		return _gates.back();
	}
	const vector<Gate>& gates() const { return _gates; };

	Latch& add_latch()
	{
		_latches.push_back(Latch());
		return _latches.back();
	}
	const vector<Latch>& latches() const { return _latches; };

	friend ostream& operator<<(ostream& out, const Model& model)
	{
		out << "Model: " << model.name() <<endl;

		out << "Input:";
		const vector<string>& inputs = model.inputs();
		for(vector<string>::const_iterator itr=inputs.begin(); itr!=inputs.end(); itr++){
			out << " " << *itr;
		}
		out<<endl;

		out << "Output:";
		const vector<string>& outputs = model.outputs();
		for(vector<string>::const_iterator itr=outputs.begin(); itr!=outputs.end(); itr++){
			out << " " << *itr;
		}
		out<<endl;

		const vector<Gate>& gates = model.gates();
		for(vector<Gate>::const_iterator itr=gates.begin(); itr!= gates.end(); itr++){
			 out << *itr;
		}

		out << endl;
		return out;
	}
};

#endif
