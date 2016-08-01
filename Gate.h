#ifndef GATE_H_
#define GATE_H_

#include <string>
#include <vector>

using namespace std;

class Gate
{
private:
	string _name;
	vector<string> _inputs;
	string _output;
	vector<vector<string> > _rows;
	bool _constant;

public:
	string& name() { return _name; }
	string name() const { return _name; }
	vector<string>& inputs() { return _inputs; }
	const vector<string>& inputs() const { return _inputs; }
	string& output() { return _output; }
	string output() const { return _output; }
	bool& constant() { return _constant; }
	bool constant() const { return _constant; }
	bool is_constant() const { return _inputs.empty(); }

	void add_row(const vector<string>& row)
	{
		_rows.push_back(row);
	}
	const vector<vector<string> >& rows() const { return _rows; };

	friend ostream& operator<<(ostream& out, const Gate& gate)
	{
		out << "Gate: " << gate.name() << endl;

		out << gate.output() << " =";
		if(gate.is_constant()){
			out << " " << (gate.constant() ? "TRUE" : "FALSE") << endl;
		}
		else{
			const vector<vector<string> > rows = gate.rows();
			if(rows.size()==1){
				for(vector<string>::const_iterator itr=rows[0].begin(); itr!=rows[0].end(); itr++){
					out << " " << *itr;
				}
			}
			else{
				for(size_t i=0; i<rows.size(); i++){
					if(i>0){
						out << " +";
					}
					if(rows[i].size()==1){
						out << " " << rows[i][0];
					}
					else{
						out << " (";
						for(vector<string>::const_iterator itr=rows[i].begin(); itr!=rows[i].end(); itr++){
							out << " " << *itr;
						}
						out << " )";
					}
				}
			}
			out << endl;
		}

		return out;
	}
};

#endif
