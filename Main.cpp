#include <iostream>
#include <random>
#include <algorithm>

#include "BlifParser.h"
#include "ModelBuilder.h"
#include "IndexSetModelBuilder.h"
#include "Rebuilder.h"
#include "System.h"

using namespace std;

int print_usage() {
    cerr << "Usage: BlifParser [INSTANCE] [#REORDER] [HEURISTIC]" << endl;
    return -1;
}

void greedy(ModelBuilder& x, ModelBuilder& y)
{
	assert(x.num_vars() == y.num_vars());

	cout << "Unify the variable orders..." << endl;

	x.reset_stat();
	y.reset_stat();

	int num_var = x.num_vars();

	int* x_order = new int[num_var + 1];
	x.get_variable_order(x_order);

	int* y_order = new int[num_var + 1];
	y.get_variable_order(y_order);

	for(int i = num_var; i >= 1; i--) {
		if(x_order[i] != y_order[i]) {
			int y_distance = 1;
			for(int j = i-1; j >= 1; j--) {
				if(x_order[i] == y_order[j]) {
					break;
				}
				y_distance++;
			}

			int x_distance = 1;
			for(int j = i-1; j >= 1; j--) {
				if(x_order[j] == y_order[i]) {
					break;
				}
				x_distance++;
			}

			if(x_distance < y_distance) {
				for(int lev = i-x_distance; lev < i; lev++) {
					x.swap_adjacent_variable(lev);
					x_order[lev] = x_order[lev+1];
				}
				x_order[i] = y_order[i];
			}
			else {
				for(int lev = i-y_distance; lev < i; lev++) {
					y.swap_adjacent_variable(lev);
					y_order[lev] = y_order[lev+1];
				}
				y_order[i] = x_order[i];
			}
		}
	}

	x.get_variable_order(x_order);
	y.get_variable_order(y_order);
	for(int i = 1; i <= num_var; i++) {
		assert(x_order[i] == y_order[i]);
	}

	delete[] x_order;
	delete[] y_order;
}

void gradual_greedy(ModelBuilder& x, ModelBuilder& y)
{
	assert(x.num_vars() == y.num_vars());

	cout << "Unify the variable orders..." << endl;

	x.reset_stat();
	y.reset_stat();

	int num_var = x.num_vars();

	int* x_order = new int[num_var + 1];
	x.get_variable_order(x_order);

	int* y_order = new int[num_var + 1];
	y.get_variable_order(y_order);

	int xlimit = x.num_nodes() * 1.2;
	int ylimit = y.num_nodes() * 1.2;

	for(int i = num_var; i >= 1; i--) {
		if(x_order[i] != y_order[i]) {
			int y_distance = 1;
			for(int j = i-1; j >= 1; j--) {
				if(x_order[i] == y_order[j]) {
					break;
				}
				y_distance++;
			}

			int x_distance = 1;
			for(int j = i-1; j >= 1; j--) {
				if(x_order[j] == y_order[i]) {
					break;
				}
				x_distance++;
			}

			if(x_distance < y_distance) {
				for(int lev = i-x_distance; lev < i; lev++) {
					x.swap_adjacent_variable(lev);
					x_order[lev] = x_order[lev+1];
				}
				x_order[i] = y_order[i];

				if(x.num_nodes() > xlimit) {
					x.optimize(i-1, 1);
					xlimit = x.num_nodes() * 1.2;

					x.get_variable_order(x_order);
				}
			}
			else {
				for(int lev = i-y_distance; lev < i; lev++) {
					y.swap_adjacent_variable(lev);
					y_order[lev] = y_order[lev+1];
				}
				y_order[i] = x_order[i];

				if(y.num_nodes() > ylimit) {
					y.optimize(i-1, 1);
					ylimit = y.num_nodes() * 1.2;

					y.get_variable_order(y_order);
				}
			}
		}
	}

	x.get_variable_order(x_order);
	y.get_variable_order(y_order);
	for(int i = 1; i <= num_var; i++) {
		assert(x_order[i] == y_order[i]);
	}

	delete[] x_order;
	delete[] y_order;
}

vector<vector<int>> readOrderFile(const char* filename)
{
	vector<vector<int>> orders;

	ifstream in(filename);
	const int MAX_LINE_LENGTH = 100000;
	char lineBuffer[MAX_LINE_LENGTH];
	char literalBuffer[10];
	while (in.getline(lineBuffer, MAX_LINE_LENGTH)) {
		vector<int> order;

		char* linePtr = lineBuffer;
		char* literalPtr = literalBuffer;
		while (*linePtr != '\0'){
			literalPtr = literalBuffer;
			while (*linePtr && isspace(*linePtr)) {
				linePtr++;
			}
			while (*linePtr && !isspace(*linePtr)) {
				*(literalPtr++) = *(linePtr++); // copy variable
			}
			*literalPtr = '\0'; // terminate variable

			if (strlen(literalBuffer) > 0) {
				order.push_back(atoi(literalBuffer));
			}
		}

		orders.push_back(order);
	}

	return orders;
}

int main(int argc, char* argv[]) {
    if (argc != 4){
    	return print_usage();
    }

    BlifParser parser;
    parser.parse(argv[1]);
    int num = atoi(argv[2]);
    const char* heuristic = argv[3];

    cout << "Instance: " << argv[1] << endl;
    cout << "Reordering: " << num << endl;
    cout << "Method: " << heuristic << "\n" << endl;

    const vector<Model>& ms = parser.models();
    vector<Model> models;
    models.insert(models.end(), ms.begin(), ms.end());

    vector<ModelBuilder*> builders;
    int max_num_vars = 0;

    for (const auto& model : models) {
    	ModelBuilder* builder = (strcmp(heuristic, "REBUILD") == 0
    			? new Rebuilder(model) : new ModelBuilder(model));
    	if(max_num_vars < builder->actual_num_vars()) {
    		max_num_vars = builder->actual_num_vars();
    	}
    	builders.push_back(builder);
    }

    MEDDLY::initialize();

#if false

    default_random_engine rg;
    for(auto& builder : builders){
    	double start = get_cpu_time();

    	builder->set_num_vars(max_num_vars);
    	builder->initialize(heuristic);

    	builder->build_model();
//    	builder->output_status(cout);
    	cout << endl;

    	builder->optimize();

//    	builder->construct_index_sets();

    	builder->output_status(cout);

    	double end = get_cpu_time();
    	cout << "Time: " << (end - start) << " s" << endl;

    	int* order = new int[builder->num_vars() + 1];
    	order[0] = 0;
    	builder->get_variable_order(order);

    	int* original_order = new int[builder->num_vars() + 1];
    	original_order[0] = 0;
    	builder->get_variable_order(original_order);

    	cout << "Order: ";
    	for (int i = 1; i <= builder->num_vars(); i++) {
    		cout << original_order[i] << ", ";
    	}
    	cout << "\n" << endl;

    	for (int i = 0; i < num; i++) {
    		cout << "Reordering..." << endl;
    		std::shuffle(&order[1], &order[builder->num_vars() + 1], rg);
//    		builder->dfs_order(order);

    		cout << "Order: ";
    		for (int i = 1; i <= builder->num_vars(); i++) {
    			cout << order[i] << ", ";
    		}
    		cout << endl;

    		builder->output_status(cout);
    		start = get_cpu_time();
    		builder->reorder(order);
    		end = get_cpu_time();
    		builder->output_status(cout);
    		cout << "Time: " << (end - start) << " s" << endl << endl;
    	}

    	delete[] order;

//    	cout << "Reordering to the original order..." << endl;
//    	builder->reorder(original_order);
//    	builder->output_status(cout);
    }

//    greedy(*builders[0], *builders[1]);
//    gradual_greedy(*builders[0], *builders[1]);

//    builders[0]->output_status(cout);
//    builders[1]->output_status(cout);

#else
    cout << "Start transforming..." << endl;
    for (auto& builder : builders) {
    	builder->set_num_vars(max_num_vars);
    	builder->initialize(heuristic);

    	builder->build_model();
//    	builder->output_status(cout);
    	cout << endl;

    	builder->optimize();

    	builder->output_status(cout);

    	{
			double start = get_cpu_time();

			builder->transform_ESRBDD();

			double end = get_cpu_time();
			cout << "Time: " << (end - start) << " s" << endl;
    	}

    	{
			double start = get_cpu_time();

			builder->transform_ZDD_and_CZDD();

			double end = get_cpu_time();
			cout << "Time: " << (end - start) << " s" << endl;
    	}

    	{
			double start = get_cpu_time();

			builder->transform_CBDD();

			double end = get_cpu_time();
			cout << "Time: " << (end - start) << " s" << endl;
    	}
    }
#endif

    for (auto& builder : builders) {
    	builder->clean_up();
    	delete builder;
    }
    MEDDLY::cleanup();

    return 0;
}
