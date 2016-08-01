#ifndef LATCH_H_
#define LATCH_H_

#include <string>

using namespace std;

class Latch
{
private:
	string _input;
	string _output;
	bool _init_value;

public:
	string& input() { return _input; }
	string input() const { return _input; }
	string& output() { return _output; }
	string output() const { return _output; }
	bool& init_value() { return _init_value; }
	bool init_value() const { return _init_value; }
};

#endif
