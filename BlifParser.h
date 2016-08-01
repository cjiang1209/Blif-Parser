#ifndef BLIFPARSER_H_
#define BLIFPARSER_H_

#include <fstream>
#include <vector>

#include "Model.h"

using namespace std;

class BlifParser
{
private:
	ifstream in;
	string line;

	vector<Model> _models;

	void parse_model(Model& model);
	void parse_gate(Gate& gate);
	void parse_latch(Latch& latch);

	void read_next_line_and_skip_comment();

	bool is_blank_or_comment(string str);

public:
	void parse(const char* file);
	const vector<Model>& models() const;
};

inline const vector<Model>& BlifParser::models() const
{
	return _models;
}

#endif
