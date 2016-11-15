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
    cerr << "Usage: BlifParser [FILE] [#REORDER] [SEED] [HEURISTIC]" << endl;
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

//int main(int argc, char* argv[]) {
//    if (argc != 4){
//    	return print_usage();
//    }
//
//    BlifParser parser;
//    parser.parse(argv[1]);
//    int num = atoi(argv[2]);
//    const char* heuristic = argv[3];
//
//    cout << argv[1] << endl;
//    cout << heuristic << "\n" << endl;
//
//    const vector<Model>& ms = parser.models();
//    vector<Model> models;
//    models.insert(models.end(), ms.begin(), ms.end());
//
//    vector<ModelBuilder*> builders;
//    int max_num_vars = 0;
//
//    for (const auto& model : models) {
//    	ModelBuilder* builder = new ModelBuilder(model);
//    	if(max_num_vars < builder->actual_num_vars()) {
//    		max_num_vars = builder->actual_num_vars();
//    	}
//    	builders.push_back(builder);
//    }
//
//    default_random_engine rg;
//
//    MEDDLY::initialize();
//    for(auto& builder : builders){
//    	double start = get_cpu_time();
//
//    	builder->set_num_vars(max_num_vars);
//    	builder->initialize(heuristic);
//
//    	builder->build_model();
////    	builder->output_status(cout);
//    	cout << endl;
//
//    	builder->optimize();
//    	builder->output_status(cout);
//
////    	double start = get_cpu_time();
//
//    	double end = get_cpu_time();
//    	cout << (end - start) << " s" << endl << endl;
//
//    	int* order = new int[builder->num_vars()+1];
//    	order[0] = 0;
//    	builder->get_variable_order(order);
//
//    	for (int i = 0; i < num; i++) {
//    		std::shuffle(&order[1], &order[builder->num_vars() + 1], rg);
////    		builder->dfs_order(order);
//
//    		for (int i = 1; i <= builder->num_vars(); i++) {
//    			cout << order[i] << ", ";
//    		}
//    		cout << endl;
//
//    		start = get_cpu_time();
//    		builder->reorder(order);
//    		builder->output_status(cout);
//    		end = get_cpu_time();
//    		cout << (end - start) << " s" << endl << endl;
//    	}
//
//    	delete[] order;
//
////    	start = get_cpu_time();
////    	builder->reorder(best_order);
////    	builder->output_status(cout);
////    	end = get_cpu_time();
////    	cout << (end - start) << " s" << endl << endl;
//    }
//
////    greedy(*builders[0], *builders[1]);
////    gradual_greedy(*builders[0], *builders[1]);
//
////    builders[0]->output_status(cout);
////    builders[1]->output_status(cout);
//
//
//    for (auto& builder : builders) {
//    	builder->clean_up();
//    	delete builder;
//    }
//    MEDDLY::cleanup();
//
//    return 0;
//}

int main(int argc, char* argv[]) {
    if (argc != 4){
    	return print_usage();
    }

    BlifParser parser;
    parser.parse(argv[1]);
    int num = atoi(argv[2]);
    const char* heuristic = argv[3];

    cout << argv[1] << endl;
    cout << heuristic << "\n" << endl;

    const vector<Model>& ms = parser.models();
    vector<Model> models;
    models.insert(models.end(), ms.begin(), ms.end());

    vector<ModelBuilder*> builders;
    int max_num_vars = 0;

    for (const auto& model : models) {
    	ModelBuilder* builder = new Rebuilder(model);
//    	ModelBuilder* builder = new ModelBuilder(model);
    	if(max_num_vars < builder->actual_num_vars()) {
    		max_num_vars = builder->actual_num_vars();
    	}
    	builders.push_back(builder);
    }

    default_random_engine rg;

    MEDDLY::initialize();
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

//    	double start = get_cpu_time();

    	double end = get_cpu_time();
    	cout << (end - start) << " s" << endl << endl;

    	int* order = new int[builder->num_vars() + 1];
    	order[0] = 0;
    	builder->get_variable_order(order);

    	int* original_order = new int[builder->num_vars() + 1];
    	original_order[0] = 0;
    	builder->get_variable_order(original_order);

    	for (int i = 0; i < num; i++) {
    		cout << "Reordering..." << endl;
    		std::shuffle(&order[1], &order[builder->num_vars() + 1], rg);
//    		builder->dfs_order(order);

//    		for (int i = 1; i <= builder->num_vars(); i++) {
//    			cout << order[i] << ", ";
//    		}
//    		cout << endl;

    		start = get_cpu_time();
    		builder->reorder(order);
    		builder->output_status(cout);

//    		builder->reorder(original_order);
//    		builder->output_status(cout);

    		end = get_cpu_time();
    		cout << (end - start) << " s" << endl << endl;
    	}

    	delete[] order;

    	builder->reorder(original_order);
    	builder->output_status(cout);

//    	start = get_cpu_time();
//    	builder->reorder(best_order);
//    	builder->output_status(cout);
//    	end = get_cpu_time();
//    	cout << (end - start) << " s" << endl << endl;
    }

//    greedy(*builders[0], *builders[1]);
//    gradual_greedy(*builders[0], *builders[1]);

//    builders[0]->output_status(cout);
//    builders[1]->output_status(cout);


    for (auto& builder : builders) {
    	builder->clean_up();
    	delete builder;
    }
    MEDDLY::cleanup();

    return 0;
}
