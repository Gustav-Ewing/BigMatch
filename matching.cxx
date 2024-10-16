#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <Python.h>
#define datasetSize 2221
#define pvValue 0
#define batteryValue 1
#define consValue 2
#define weightValue 3
#define Range 40000

#define runopt "runopt"
#define runOptimize "runOptimize"
#define neighborHood "neighborhood"
#define findNeighborhood "findNeighborhood"
#define loadOpt "loadOpt"

using namespace std;

#include <vector>
#include <tuple>
#include <utility>
#include <chrono>
#include <ctime>


struct Edge {
    int source;
    int destination;
    double weight;
    // Constructor for easy initialization
    Edge(int src, int dest, double w) : source(src), destination(dest), weight(w) {}
};

typedef struct{
    std::vector<Edge> graph;  // Simple version of a graph
} uGraph;

PyObject *makelist(int array[], size_t size);
int readFile(string path, int type);
int preprocess(bool printValues);
int greedyMatching();
int pythonOptimizer(const char *name, const char *function, int indexCount, int indexes[]);
int pythonNeighborhood(const char *name, const char *function, int index, int range);
string loadingBar(float percent);
vector<std::tuple<int, int, double>> doublegreedyMatching(vector<int> prosumersList, vector<int> consumersList);
pair<int, float> next_edge_greedy_path(int prosumer, vector<Edge> *graph, int prosumerSize, bool available[]);
bool canCreateNewEdge(const vector<Edge> *tempGraph, int householdId, int targetHouseholdId);


class Prosumer
{
public:
	float pv;
	float battery;
	// float	cons;
	int matchedToIndex;
	float saved;
};

struct
{
	float pv[datasetSize];
	float battery[datasetSize];
	float cons[datasetSize];
	Prosumer *prosumers[datasetSize];
	bool availableConsumers[datasetSize];
	float currentWeight;
	int neighbors[datasetSize];
	int neighborCount;
	int l;
	int procheck;
} myData;

PyObject *pName, *pModule, *pFunc;
PyObject *pArgs, *pValue;

int main(int argc, char *argv[])
{
	time_t t1 = time(NULL);
	Py_Initialize();
	if (argc < 3)
	{
		myData.l = datasetSize;
		myData.procheck = stoi(argv[1]);
		cout << "l value not specified so no cap implemented" << endl;
	}
	else if (argc < 2)
	{
		myData.l = datasetSize;
		myData.procheck = datasetSize;
		cout << "i value not specified defaulting" << endl;
		cout << "l value not specified so no cap implemented" << endl;
	}
	else
	{
		myData.procheck = stoi(argv[1]);
		myData.l = stoi(argv[2])+1;
	}

	// These 2 lines just allow the interpreter to access python files in the current directory
	// this means that the shell running the process has to be in python/ even if the executable is in another e.g. python/build/
	PyRun_SimpleString("import sys");
	PyRun_SimpleString("sys.path.append(\".\")");

	myData.neighborCount = 0; // simple init value to avoid undefined behavior
	pythonNeighborhood(neighborHood, findNeighborhood, 0, 40000);

	/* Initializes the dataset opt2221() */
	pName = PyUnicode_DecodeFSDefault(runopt);
	/* Error checking of pName left out */

	pModule = PyImport_Import(pName);
	Py_DECREF(pName);
	if (pModule != NULL)
	{
		pFunc = PyObject_GetAttrString(pModule, loadOpt);
		/* pFunc is a new reference */

		if (pFunc && PyCallable_Check(pFunc))
		{
			PyObject_CallObject(pFunc, NULL);
			cout << "Initialized opt2221()" << endl;
		}
		Py_XDECREF(pFunc);
		Py_DECREF(pModule);
	} /* End of initialization */

	preprocess(false);
	//greedyMatching();
	
	
	
		//double greedy debug
	vector<int> prosumersList;
	vector<int> consumersList;
	vector<std::tuple<int, int, double>> result;

	for (int i = 0; i < myData.procheck; i++)
	{
		if (myData.prosumers[i] == NULL)
		{
		consumersList.push_back(i);
		continue;
		}
		
		prosumersList.push_back(i);
	}
	result = doublegreedyMatching(prosumersList, consumersList);
	if (!result.empty()) {
    std::cout << "First value: " << std::get<0>(result[0]) << std::endl;
    std::cout << "Second value: " << std::get<1>(result[0]) << std::endl;
    std::cout << "Third value: " << std::get<2>(result[0]) << std::endl;
} else {
    std::cout << "The result vector is empty!" << std::endl;
}
	

	for (int i = 0; i < datasetSize; i++)
	{
		if (myData.prosumers[i] == NULL)
		{
			continue;
		}
		else
		{
			// no point displaying prosumers who didn't match for now
			if (myData.prosumers[i]->matchedToIndex == -1)
			{
				break;
			}
		}
		cout << endl
			 << "Prosumer " << "\033[94m" << i << "\033[m" << " is matched to consumer " << "\033[94m" << myData.prosumers[i]->matchedToIndex << "\033[m" << " which saves " << "\033[92m" << myData.prosumers[i]->saved << "\033[m" << endl;
	}

	if (Py_FinalizeEx() < 0)
	{
		return 120;
	}
	time_t t2 = time(NULL);
	double diff = difftime(t2, t1);
	printf("\n\033[91m%.f\033[m seconds since start of execution.\n", diff);

	double sum = 0;
	for (int i = 0; i < datasetSize; i++)
	{
		if (myData.prosumers[i] == NULL)
		{
			continue;
		}
		if (myData.prosumers[i]->matchedToIndex == -1)
		{
			continue;
		}
		sum = sum + myData.prosumers[i]->saved;
	}

	printf("\n\033[92m%.6f\033[m saved in total across all pairs.\n", sum);

	return EXIT_SUCCESS;
}

int greedyMatching()
{

	// change the i<value to make it run faster when debugging
	for (int i = 0; i < myData.procheck; i++)
	{
		if (myData.prosumers[i] == NULL)
		{
			continue;
		}

		cout << endl
			 << "******* Prosumer " << "\033[94m" << i << "\033[m" << "/" << "\033[94m" << myData.procheck << "\033[m" << " *******" << endl;

		// finds the neighborhood via a python function see below for indepth
		pythonNeighborhood(neighborHood, findNeighborhood, i, Range);

		float currentBestWeight = -1;
		int currentBestIndex = -1;

		/*
		debug code to check the neighborhoods of the prosumers
		cout << "Prosumer " << i << " has the following neighbors: " << endl;
		for(int j=0; j<myData.neighborCount; j++){
			cout << myData.neighbors[j] << endl;
		}
		cout << "\n\n" << endl;
		continue;
		*/

		// makes sure l doesn't account for unavailable neighbors
		int count = 0;
		// when empty it simply makes no match which is equivalent to having no neighbors
		for (int j = 0; j < myData.neighborCount && j < (myData.l + count); j++)
		{

			int maxRounds = min((myData.l + count), myData.neighborCount);
			cout
				<< "Checking neighbours " << "\033[94m" << j + 1 << "\033[m" << "/" << "\033[94m" << maxRounds << "\t" << "\033[m" << loadingBar(float(j + 1) / float((maxRounds))) << "\t\r" << flush;
			// checking if edge j in the neighborhood is available

			/*
			Found the segfault that arises somewhere in here with gdb
			Thread 1 "Matching" received signal SIGSEGV, Segmentation fault.
			0x00007ffff7e157ea in tupledealloc.lto_priv () from /home/sven/anaconda3/envs/py37/lib/libpython3.7m.so.1.0
			not sure exactly what causes it but something goes wrong when deallocating a tuple

			here is the backtrace so the pythonOptimizer(runopt, runOptimize, 2, list); call below is the cause
			(gdb) backtrace
			#0  0x00007ffff7e157ea in tupledealloc.lto_priv () from /home/sven/anaconda3/envs/py37/lib/libpython3.7m.so.1.0
			#1  0x0000555555557837 in pythonOptimizer(char*, char*, int, int*) ()
			#2  0x0000555555557d32 in greedyMatching() ()
			#3  0x00005555555572a8 in main ()

			might not neceseraly be a bug could be due to lack of memory so will change some settings and try rerunning it

			*/

			if (!myData.availableConsumers[myData.neighbors[j]])
			{
				// this can count prosumers which could lead to weird behavior so should be refined further
				count++;
				continue;
			}

			int list[2] = {i, j};
			pythonOptimizer(runopt, runOptimize, 2, list);
			// cout << "Current weight is: "<< myData.currentWeight << endl;
			if (currentBestWeight < myData.currentWeight)
			{
				currentBestWeight = myData.currentWeight;
				currentBestIndex = myData.neighbors[j];
			}
		}
		cout << endl
			 << "best index: " << "\033[92m" << currentBestIndex << "\033[m" << "\nbest weight: " << "\033[92m" << currentBestWeight << "\033[m" << endl;

		// matching the prosumer to its consumer and marking the consumer as unavailable
		myData.prosumers[i]->matchedToIndex = currentBestIndex;
		myData.prosumers[i]->saved = currentBestWeight;
		myData.availableConsumers[currentBestIndex] = false;
	}
	return EXIT_SUCCESS;
}

// prepares the data for the matching
// calling it with true will print all values used for the matching
int preprocess(bool printValues)
{
	// this extracts the pv, battery and cons values to the files
	if (system("python3 preprocess.py"))
	{
		cout << "Failed to run extract" << endl;
		return EXIT_FAILURE;
	}

	// retrieves the pv and battery values from the files and optionally prints them all out
	// could probably generalize the following more but not sure how much of this stays
	if (readFile("pvdata.txt", pvValue))
	{
		cout << "error couldn't read pvdata.txt\n";
	}
	else
	{
		if (printValues)
		{
			cout << "pv values: \n";
			for (int i = 0; i < datasetSize; i++)
			{
				cout << myData.pv[i] << "\t";
			}
			cout << "\n\n";
		}
	}
	if (readFile("batterydata.txt", batteryValue))
	{
		cout << "error couldn't read batterydata.txt\n";
	}
	else
	{
		if (printValues)
		{
			cout << "battery values: \n";
			for (int i = 0; i < datasetSize; i++)
			{
				cout << myData.battery[i] << "\t";
			}
			cout << "\n\n";
		}
	}

	// The file this reads is huge so ignored for now
	/*
	if(readFile("consdata.txt",consValue)){
		cout << "error couldn't read consdata.txt\n";
	}
	else{
		if(printValues){
			cout << "consumption values: \n";
			for(int i = 0; i<100; i++){
				cout << myData.cons[i] << "\t";
			}
			cout << "\n\n";
		}
	}
	*/

	// initalizes the prosumer and consumer sets
	for (int i = 0; i < datasetSize; i++)
	{
		myData.prosumers[i] = NULL;
		myData.availableConsumers[i] = false;
	}

	// int counter = 0;
	for (int i = 0; i < datasetSize; i++)
	{
		if (myData.pv[i] != 0 || myData.battery[i] != 0)
		{
			// initalizes a new pointer for every prosumer tuple
			Prosumer *prosumer = new Prosumer{myData.pv[i], myData.battery[i], -1, -1};
			/*(Prosumer)malloc(sizeof(Prosumer));
			prosumer->pv = myData.pv[i];
			prosumer->battery = myData.battery[i];
			prosumer->matchedToIndex = -1;*/
			myData.prosumers[i] = prosumer;

			/*
				The following makes a condensed array instead of a sparse array
				A dense array means less values to check when running the alg
				However NULL checking should be fast so they are probably about equal

				*** Make sure to uncomment the //int counter = 0; above and change the NULL checks to break instead of continue in that case ***
			*/
			// myData.prosumers[counter] = prosumer;
			// counter++;
		}
		else
		{
			myData.availableConsumers[i] = true;
		}
	}
	// prints the prosumer and consumer set
	if (printValues)
	{
		cout << "Index values of prosumers: \n";
		for (int i = 0; i < datasetSize; i++)
		{
			if (myData.prosumers[i] == NULL)
			{
				// break;
				continue;
			}
			cout << i << "\t";
		}
		cout << "\n\n";

		cout << "Index values of available consumers: \n";
		for (int i = 0; i < datasetSize; i++)
		{
			if (!myData.availableConsumers[i])
			{
				// break;
				continue;
			}
			cout << i << "\t";
		}
		cout << "\n\n";
	}
	return EXIT_SUCCESS;
}

// reads in the data from a a file
int readFile(string path, int type)
{
	ifstream file;

	if (!filesystem::exists(path))
	{
		return EXIT_FAILURE;
	}

	file.open(path);
	if (!file.is_open())
	{
		cout << "error\n";
		return EXIT_FAILURE;
	}
	int i = 0;
	string tmp;

	while (file >> tmp)
	{
		if (type == pvValue)
		{
			myData.pv[i] = stof(tmp);
		}
		else if (type == batteryValue)
		{
			myData.battery[i] = stof(tmp);
		}
		else if (type == consValue)
		{
			myData.cons[i] = stof(tmp);
		}
		else if (type == weightValue)
		{
			myData.currentWeight = stof(tmp);
		}
		i++;
	}
	file.close();
	return EXIT_SUCCESS;
}

PyObject *makelist(int array[], int size)
{
	PyObject *l = PyList_New(size);
	for (int i = 0; i < size; ++i)
	{
		PyList_SET_ITEM(l, i, PyLong_FromLong(array[i]));
	}
	return l;
}

int *makearr(PyObject *list)
{
	for (int i = 0; i < myData.neighborCount; ++i)
	{
		myData.neighbors[i] = PyLong_AsLong(PyList_GetItem(list, i));
	}
	return EXIT_SUCCESS;
}

// calls the optimizer python function and returns the result
int pythonOptimizer(const char *name, const char *function, int indexCount, int indexes[])
{

	pName = PyUnicode_DecodeFSDefault(name);
	/* Error checking of pName left out */

	pModule = PyImport_Import(pName);
	Py_DECREF(pName);

	if (pModule != NULL)
	{
		pFunc = PyObject_GetAttrString(pModule, function);
		/* pFunc is a new reference */

		if (pFunc && PyCallable_Check(pFunc))
		{
			pArgs = PyTuple_New(2);
			PyTuple_SetItem(pArgs, 0, PyLong_FromLong(indexCount));
			PyObject *pList;
			pList = makelist(indexes, indexCount);
			PyTuple_SetItem(pArgs, 1, pList);

			pValue = PyObject_CallObject(pFunc, pArgs);
			Py_DECREF(pList);
			Py_DECREF(pArgs);

			if (pValue != NULL)
			{
				// printf("Result of call: %f\n", PyFloat_AsDouble(pValue));
				myData.currentWeight = PyFloat_AsDouble(pValue);
				Py_DECREF(pValue);
			}
			else
			{
				Py_DECREF(pFunc);
				Py_DECREF(pModule);
				PyErr_Print();
				fprintf(stderr, "Call failed\n");
				return EXIT_FAILURE;
			}
		}
		else
		{
			if (PyErr_Occurred())
				PyErr_Print();
			fprintf(stderr, "Cannot find function \"%s\"\n", function);
		}
		Py_XDECREF(pFunc);
		Py_DECREF(pModule);
		return EXIT_SUCCESS;
	}
	else
	{
		PyErr_Print();
		fprintf(stderr, "Failed to load \"%s\"\n", name);
		return EXIT_FAILURE;
	}
}

// calls a python function that returns the neighborhood for a consumer/prosumer
// the neighborhood is an unsorted array of indexes which is stored inside myData.neighbors
// myData.neighbors only lazy deletes so only the first myData.neighborCount elements are actually valid at any time
int pythonNeighborhood(const char *name, const char *function, int index, int range)
{

	pName = PyUnicode_DecodeFSDefault(name);
	/* Error checking of pName left out */

	pModule = PyImport_Import(pName);
	Py_DECREF(pName);

	if (pModule != NULL)
	{
		pFunc = PyObject_GetAttrString(pModule, function);
		/* pFunc is a new reference */

		if (pFunc && PyCallable_Check(pFunc))
		{
			pArgs = PyTuple_New(2);
			PyTuple_SetItem(pArgs, 0, PyLong_FromLong(range));
			PyTuple_SetItem(pArgs, 1, PyLong_FromLong(index));

			pValue = PyObject_CallObject(pFunc, pArgs);
			Py_DECREF(pArgs);

			if (pValue != NULL)
			{
				// saving the data to global variables
				myData.neighborCount = PyList_Size(pValue);	
				makearr(pValue);

				// GC
				Py_DECREF(pValue);
			}
			else
			{
				Py_DECREF(pFunc);
				Py_DECREF(pModule);
				PyErr_Print();
				fprintf(stderr, "Call failed\n");
				return EXIT_FAILURE;
			}
		}
		else
		{
			if (PyErr_Occurred())
				PyErr_Print();
			fprintf(stderr, "Cannot find function \"%s\"\n", function);
		}
		Py_XDECREF(pFunc);
		Py_DECREF(pModule);
		return EXIT_SUCCESS;
	}
	else
	{
		PyErr_Print();
		fprintf(stderr, "Failed to load \"%s\"\n", name);
		return EXIT_FAILURE;
	}
}

string loadingBar(float percent)
{
	string bar = "\033[92m";
	int barWidth = 70;

	bar += "[";
	int pos = barWidth * percent;
	for (int i = 0; i < barWidth; ++i)
	{
		if (i < pos)
			bar += "=";
		else if (i == pos)
			bar += ">";
		else
			bar += " ";
	}
	bar = bar + "] " + "\033[m" + to_string(int(percent * 100.0)) + "%";
	return bar;
}


std::vector<std::tuple<int, int, double>>  doublegreedyMatching(vector<int> prosumersList, vector<int> consumersList){



	bool available[datasetSize];

	for (int i = 0; i < datasetSize; i++)
	{
		
		available[i] = true;
		
	}

	int prosumerSize = int(prosumersList.size()); //equal to the amount of prosumers in vector prosumersList

	std::vector<std::tuple<int, int, double>> allmatchings;

	std::size_t size1 = prosumersList.size();
    std::cout << "Size (using std::size_t): " << size1 << std::endl;

	
	for(int i = 0;i < prosumerSize; i++)
	{
		int k = 0;
		int household;
		//cout << "do we enter?" << i << endl;

		cout << "starting with prosumer " << prosumersList[i] << "which is" << available[prosumersList[i]] << endl;
		if (available[prosumersList[i]])
		{
			//cout << "fail here?3" << endl;
			  vector<Edge> tempGraph;
			  int o = 1;
			  household = prosumersList[i];
			while(o == 1 || k > 100){
				pair<int, float> result = next_edge_greedy_path(household, &tempGraph, prosumersList.size(), available);
				if(result.first != -1){
					Edge addEdge = {household, result.first, result.second};
					tempGraph.push_back(addEdge);
					//cout << "are we stuck with prosumer " << i << " Path attempt:"<< k << "\t\r" << flush;
					household = result.first;
					k++;
				}
				else{
					o=0;
				}
			}
			if(tempGraph.size() == size_t(0)){

				continue;
			}
			else{
				for (const Edge edge : tempGraph) {
					
					if (myData.prosumers[edge.source] == NULL) //if the source vertex is a consumer
					{
					cout << "skiped since index " << edge.source << " is a consumer" << endl;
					continue; //we skip, since we only want prosumer to consumer,  other, if we dont remove this we could get groups of size 3.
					}
		
					

					cout << "added a edge with origin " << edge.source << " to the graph" << endl;
					allmatchings.push_back(make_tuple(edge.source, edge.destination, edge.weight));
					available[edge.source] = false;
					available[edge.destination] = false;
				}
				

			}
		}
	}
	/* code arr.push_back(std::make_tuple(1, 3.14, "Hello")); // First tuple */

std::cout << "Size of result inside doublegreedyMatching: " << allmatchings.size() << std::endl;
	return allmatchings;
}


std::pair<int, float> next_edge_greedy_path(int household, vector<Edge> *tempgraph, int prosumersize, bool available[]) {

	cout << "we are checking household number " << household << endl; 
	pythonNeighborhood(neighborHood, findNeighborhood, household, Range);

	std::vector<int> N; //vi kan möjligtvis begränsa längden av N genom att göra den l+1 lång array.
	
	/*
	 * if(myData.prosumers[household] != NULL){
	 * int count = 0;
	 * auto ceiling = myData.l+count;
	 * if(myData.l > myData.neighborCount){
	 *	ceiling = myData.neighborCount+count;
	 *	for (int i = 0; i < ceiling; i++)
	 *	{
	 *		if(canCreateNewEdge(tempgraph, household, myData.neighbors[i]) && available[i] && myData.prosumers[myData.neighbors[i]] == NULL){
	 *			N.push_back(myData.neighbors[i]);
}

if(canCreateNewEdge(tempgraph, household, myData.neighbors[i]) && available[i] && myData.prosumers[myData.neighbors[i]] != NULL){
	count++;
}
}
}
}
	 *
	 * */

	if(myData.prosumers[household] != NULL){
		int count = 0;
		if(myData.l < myData.neighborCount){// {
			for (int i = 0; i < myData.l+count; i++)
			{
				if(canCreateNewEdge(tempgraph, household, myData.neighbors[i]) && available[i] && myData.prosumers[myData.neighbors[i]] == NULL){
					N.push_back(myData.neighbors[i]);
				}

				if(canCreateNewEdge(tempgraph, household, myData.neighbors[i]) && available[i] && myData.prosumers[myData.neighbors[i]] != NULL){
					count++;
				}
			}
		}else if(myData.l > myData.neighborCount){
			for (int i = 0; i < myData.neighborCount+count; i++)
			{
				if(canCreateNewEdge(tempgraph, household, myData.neighbors[i]) && available[i] && myData.prosumers[myData.neighbors[i]] == NULL){
					N.push_back(myData.neighbors[i]);
				}
				if(canCreateNewEdge(tempgraph, household, myData.neighbors[i]) && available[i] && myData.prosumers[myData.neighbors[i]] != NULL){
					count++;
				}
			}
		}
	}
	else{
		int count = 0;
		if(myData.l < myData.neighborCount){
			for (int i = 0; i < myData.l+count; i++)
			{
				if(canCreateNewEdge(tempgraph, household, myData.neighbors[i]) && available[i] && myData.prosumers[myData.neighbors[i]] != NULL){
					N.push_back(myData.neighbors[i]);
				}

				if(canCreateNewEdge(tempgraph, household, myData.neighbors[i]) && available[i] && myData.prosumers[myData.neighbors[i]] == NULL){
					count++;
				}
			}
		}else if(myData.l > myData.neighborCount){
			for (int i = 0; i < myData.neighborCount+count; i++)
			{
				if(canCreateNewEdge(tempgraph, household, myData.neighbors[i]) && available[i] && myData.prosumers[myData.neighbors[i]] != NULL){
					N.push_back(myData.neighbors[i]);
				}
				if(canCreateNewEdge(tempgraph, household, myData.neighbors[i]) && available[i] && myData.prosumers[myData.neighbors[i]] == NULL){
					count++;
				}
			}
		}
	}


	int j;
	float weight = -1;
	if(N.size() != size_t(0)){
		if(N.size() > size_t(1)){
			//l+1 search
			for (size_t i = 0; i < N.size(); i++){
			int list[2] = {household,  N[i]};
			pythonOptimizer(runopt, runOptimize, 2, list);
			// cout << "Current weight is: "<< myData.currentWeight << endl;
			//cout << "are we stuck auxillary? " << i << endl;
			if (weight < myData.currentWeight)
			{
				weight = myData.currentWeight;
				j = N[i];
			}
			}
		}
		else{
			j = N[0];
			int list[2] = {household,  j};
			pythonOptimizer(runopt, runOptimize, 2, list);
			weight = myData.currentWeight;
		}	
	}
	else{
		j = -1;
	}


    return std::make_pair(j, weight); 
}

// 0->7
// 5->3
// 10->3

/*
 *
 *
 *
 */


// Function to check if a household can create a new edge with a target household
bool canCreateNewEdge(const vector<Edge> *tempGraph, int householdId, int targetHouseholdId) {
    // Check if the current household is a source
    for (const Edge& edge : *tempGraph) {
        if (edge.source == targetHouseholdId) {
            return false; // Current household is a source
        }
		 if (edge.destination == targetHouseholdId) {
            return false; // Target household is a source
        }
    }
    // If both checks pass, return true
    return true;
}
