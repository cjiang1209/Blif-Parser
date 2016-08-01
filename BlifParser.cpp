#include <iostream>
#include <boost/algorithm/string.hpp>

#include "BlifParser.h"

using namespace std;

void BlifParser::parse(const char* file)
{
	in.open(file);
	if (!in.is_open()) {
		cout << "Cannot find the file: " << file << endl;
		exit(1);
	}

	read_next_line_and_skip_comment();
	while(!in.eof()){
		_models.push_back(Model());
		Model& model = _models.back();
		parse_model(model);
	}
}

void BlifParser::parse_model(Model& model)
{
	assert(boost::algorithm::starts_with(line, ".model"));

	vector<string> tokens;
	boost::algorithm::split(tokens, line, boost::is_space(), boost::algorithm::token_compress_on);
	model.name() = tokens[1];

	vector<string> inputs;
	vector<string> outputs;
	vector<Gate> gates;

	read_next_line_and_skip_comment();
	while(!in.eof() && !boost::algorithm::starts_with(line, ".end")){
		if(boost::algorithm::starts_with(line, ".inputs")){
			boost::algorithm::split(tokens, line, boost::is_space(), boost::algorithm::token_compress_on);
			for(vector<string>::iterator itr=tokens.begin()+1; itr!=tokens.end(); itr++){
				inputs.push_back(*itr);
			}
			read_next_line_and_skip_comment();
		}
		else if(boost::algorithm::starts_with(line, ".outputs")){
			boost::algorithm::split(tokens, line, boost::is_space(), boost::algorithm::token_compress_on);
			for(vector<string>::iterator itr=tokens.begin()+1; itr!=tokens.end(); itr++){
				outputs.push_back(*itr);
			}
			read_next_line_and_skip_comment();
		}
		else if(boost::algorithm::starts_with(line, ".names")){
			Gate& gate = model.add_gate();
			parse_gate(gate);
		}
		else if(boost::algorithm::starts_with(line, ".latch")){
			Latch& latch = model.add_latch();
			parse_latch(latch);
		}
		else{
			read_next_line_and_skip_comment();
		}
	}

	model.inputs() = inputs;
	model.outputs() = outputs;

	read_next_line_and_skip_comment();
}

void BlifParser::parse_gate(Gate& gate)
{
	assert(boost::algorithm::starts_with(line, ".names"));

	vector<string> tokens;
	boost::algorithm::split(tokens, line, boost::is_space(), boost::algorithm::token_compress_on);

	vector<string> inputs(tokens.begin()+1, tokens.end()-1);
	string output(tokens.back());
	gate.name() = output;
	gate.inputs() = inputs;
	gate.output() = output;

	read_next_line_and_skip_comment();
	if(inputs.empty()){
		// Constant
		if(line[0]=='1'){
			gate.constant() = true;
		}
		else{
			gate.constant() = false;
		}
	}
	else{
		vector<string> row;
		bool set = false;
		while(line[0]=='0' || line[0]=='1' || line[0]=='-'){
			row.clear();

			boost::algorithm::split(tokens, line, boost::is_space(), boost::algorithm::token_compress_on);
			assert(tokens[0].size()==inputs.size());

			if(!set) {
				if(tokens[1] == "-") {
					continue;
				}
				else if(tokens[1] == "0") {
					gate.output() = "-" + gate.output();
				}
				set = true;
			}

			string plane = tokens[0];
			for(size_t i = 0; i < plane.size(); i++) {
				if(plane[i] == '1'){
					row.push_back(inputs[i]);
				}
				else if(plane[i] == '0'){
					row.push_back("-" + inputs[i]);
				}
			}
			gate.add_row(row);

			read_next_line_and_skip_comment();
		}
	}
}

void BlifParser::parse_latch(Latch& latch)
{
	assert(boost::algorithm::starts_with(line, ".latch"));

	vector<string> tokens;
	boost::algorithm::split(tokens, line, boost::is_space(), boost::algorithm::token_compress_on);

	latch.input() = tokens[1];
	latch.output() = tokens[2];
	latch.init_value() = (tokens[3] == "1");
}

void BlifParser::read_next_line_and_skip_comment()
{
	line = "";
	string temp;
	while(!in.eof()){
		getline(in, temp);
		boost::algorithm::trim(temp);
		if(!is_blank_or_comment(temp)){
			if(boost::algorithm::ends_with(temp, "\\")){
				// More lines
				line += temp.substr(0, temp.size()-1);
				line += " ";
			}
			else{
				line += temp;
				break;
			}
		}
	}
}

bool BlifParser::is_blank_or_comment(string str)
{
	return str=="" || boost::algorithm::starts_with(str, "#");
}
