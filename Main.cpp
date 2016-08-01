#include <iostream>

#include "BlifParser.h"
#include "ModelBuilder.h"
#include "System.h"

using namespace std;

int print_usage() {
    cerr << "Usage: BlifParser [FILE] [#REORDER] [SEED] [HEURISTIC]" << endl;
    return -1;
}

void shuffle(int* array, int start, int end)
{
	for(int i=start; i<end; i++) {
		int swap=i+rand()%(end-i+1);
		int value=array[i];
		array[i]=array[swap];
		array[swap]=value;
	}
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

int main(int argc, char* argv[]) {
    if (argc != 5){
    	return print_usage();
    }

    BlifParser parser;
    parser.parse(argv[1]);
    int num = atoi(argv[2]);
    int seed = atoi(argv[3]);
    const char* heuristic = argv[4];

    cout << argv[1] << endl;
    cout << heuristic << "\n" << endl;

    const vector<Model>& ms = parser.models();
    vector<Model> models;
    models.insert(models.end(), ms.begin(), ms.end());

    vector<ModelBuilder*> builders;
    int max_num_vars = 0;

    for(auto& model : models){
    	ModelBuilder* builder = new ModelBuilder(model);
    	if(max_num_vars < builder->actual_num_vars()) {
    		max_num_vars = builder->actual_num_vars();
    	}
    	builders.push_back(builder);
    }

    srand(seed);

    MEDDLY::initialize();
    for(auto& builder : builders){
    	double start = get_cpu_time();

    	builder->set_num_vars(max_num_vars);
    	builder->initialize(heuristic);

//    	int best_order[] = {0, 12, 9, 41, 39, 11, 6, 38, 40, 14, 8, 27, 29, 13, 5, 26, 28, 23, 22, 36, 35, 19, 20, 34, 37,
//    	    	    		24, 25, 33, 31, 17, 18, 30, 32, 10, 2, 21, 16, 1, 3, 7, 4, 15};
//    	int order[] = {0, 12, 9, 41, 39, 11, 6, 38, 40, 14, 8, 27, 29, 13, 5, 26, 28, 23, 22, 36, 35, 19, 20, 34, 37,
//    	    		24, 25, 33, 31, 17, 18, 30, 32, 10, 2, 21, 16, 1, 3, 7, 4, 15};
//    	int norder[] = {0, 21, 170, 74, 14, 116, 158, 114, 70, 138, 42, 135, 117, 35, 44, 41, 92, 5, 62, 69, 110, 118, 82, 72, 98, 101, 163, 88, 55, 178, 58, 155, 137, 109, 148, 94, 7, 54, 174, 177, 166, 104, 160, 38, 30, 46, 102, 157, 121, 150, 168, 15, 149, 67, 26, 9, 61, 64, 156, 147, 63, 129, 99, 75, 142, 161, 105, 96, 12, 172, 10, 66, 132, 86, 34, 33, 133, 77, 140, 29, 56, 153, 43, 173, 162, 154, 91, 59, 169, 24, 87, 106, 36, 152, 53, 151, 19, 90, 22, 167, 136, 52, 100, 11, 20, 176, 1, 23, 83, 145, 124, 3, 112, 60, 164, 143, 144, 28, 37, 47, 49, 76, 128, 17, 123, 115, 111, 119, 139, 107, 84, 13, 79, 8, 51, 108, 65, 146, 48, 131, 45, 120, 93, 171, 6, 130, 134, 103, 39, 89, 113, 95, 122, 25, 50, 68, 85, 4, 18, 31, 2, 159, 27, 78, 71, 126, 175, 80, 165, 16, 40, 32, 125, 97, 57, 141, 81, 127, 73};
//    	int best_order[] = {0, 23, 8, 17, 40, 28, 10, 20, 37, 34, 11, 6, 26, 1, 13, 15, 2, 7, 14, 33, 39, 9, 38, 25, 41, 16,
//    			29, 22, 21, 27, 24, 18, 35, 3, 5, 30, 4, 32, 36, 19, 31, 12};
//    	builder->reorder(norder);

    	builder->build_model();
//    	builder->output_status(cout);
    	cout << endl;

    	builder->optimize();
    	builder->output_status(cout);

    	// vda.blif
//    	int order[] = {0, 2, 4, 5, 13, 7, 3, 12, 6, 9, 8, 11, 1, 16, 14, 15, 10, 17};
//    	int order[] = {0, 16, 14, 13, 5, 11, 15, 6, 12, 9, 10, 7, 17, 2, 4, 3, 8, 1};
//    	int order[] = {0, 17, 13, 15, 14, 4, 7, 11, 16, 9, 10, 5, 8, 3, 2, 6, 1, 12};
//    	int order[] = {0, 1, 5, 3, 4, 14, 11, 7, 2, 9, 8, 13, 10, 15, 16, 12, 17, 6};

    	// tt2.blif
//    	int order[] = {0, 24, 23, 21, 18, 15, 13, 17, 14, 16, 1, 4, 2, 3, 6, 7, 22, 8, 9, 10, 12, 19, 20, 5, 11};
//    	int order[] = {0, 1, 2, 4, 7, 8, 12, 8, 11, 9, 24, 20, 23, 22, 19, 18, 3, 17, 16, 15, 13, 6, 5, 20, 14};
//    	int order[] = {0, 1, 2, 16, 3, 22, 21, 4, 7, 9, 5, 8, 6, 20, 24, 19, 18, 17, 15, 14, 23, 11, 13, 12, 10};
//    	int order[] = {0, 24, 23, 9, 22, 3, 4, 21, 18, 16, 20, 17, 19, 5, 1, 6, 7, 8, 10, 11, 2, 14, 12, 13, 15};

    	// C499.blif
//    	int order[] = {0, 12, 9, 41, 39, 11, 6, 38, 40, 14, 8, 27, 29, 13, 5, 26, 28, 23, 22, 36, 35, 19, 20, 34, 37,
//    			24, 25, 33, 31, 17, 18, 30, 32, 10, 2, 21, 16, 1, 3, 7, 4, 15};

//    	double start = get_cpu_time();

//    	builder->reorder(order);
//    	builder->output_status(cout);

    	double end = get_cpu_time();
    	cout << (end - start) << " s" << endl << endl;

    	int* order = new int[builder->num_vars()+1];
    	order[0] = 0;
    	builder->get_variable_order(order);
    	for(int i=0; i<num; i++) {
    		shuffle(order, 1, builder->num_vars());

    		for(int i=1; i<=builder->num_vars(); i++) {
    			cout << order[i] << ", ";
    		}
    		cout << endl;

    		start = get_cpu_time();
    		builder->reorder(order);
    		builder->output_status(cout);
    		end = get_cpu_time();
    		cout << (end - start) << " s" << endl << endl;
    	}

    	delete[] order;

//    	start = get_cpu_time();
//    	builder->reorder(best_order);
//    	builder->output_status(cout);
//    	end = get_cpu_time();
//    	cout << (end - start) << " s" << endl << endl;
    }

//    greedy(*builders[0], *builders[1]);
//    gradual_greedy(*builders[0], *builders[1]);

//    builders[0]->output_status(cout);
//    builders[1]->output_status(cout);19881209


    for(auto& builder : builders) {
    	builder->clean_up();
    	delete builder;
    }
    MEDDLY::cleanup();

    return 0;
}
