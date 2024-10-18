#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <Python.h>


// defines for global number macros
#define datasetSize 2221
#define pvValue 0
#define batteryValue 1
#define consValue 2
#define weightValue 3
#define RANGE 40000

// defines for the naming of python modules
#define runopt "runopt"
#define neighborHood "neighborhood"
#define preComputeNeighborhoods "preComputeNeighborhoods"
#define preComputeRunOpt "preComputeRunOpt"


// defines for the naming of python functions
#define loadOpt "loadOpt"
#define findNeighborhood "findNeighborhood"
#define runOptimize "runOptimize"
#define findNeighborhoods "findNeighborhoods"

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
    vector<Edge> graph;  // Simple version of a graph
} uGraph;


int greedyMatching(vector<int> prosumersList, vector<int> consumersList);

vector<tuple<int, int, double>> doublegreedyMatching(vector<int> prosumersList, vector<int> consumersList);
pair<int, float> next_edge_greedy_path(int prosumer, vector<Edge> *graph, int prosumerSize, bool available[], bool consumers[]);
bool canCreateNewEdge(const vector<Edge> *tempGraph, int householdId, int targetHouseholdId);

int readFile(string path, int type);
int preprocess(bool printValues);

int precomputePython(char *fileName);
int precomputePythonNeighborhood(const char *name, const char *function, char *fileName);

int pythonOptimizer(int indexCount, int indexes[]);
int pythonOptimizerOnline(const char *name, const char *function, int indexCount, int indexes[]);
int pythonOptimizerPreComputed(const char *name, const char *function, int indexCount, int indexes[]);

int pythonNeighborhood(int index);
int pythonNeighborhoodOnline(const char *name, const char *function, int index);
int pythonNeighborhoodPrecomputed(const char *name, const char *function, int index);


string loadingBar(float percent);

/*
class Prosumer
{
public:
	int matchedToIndex;
	float saved;
};
*/

struct
{
	float pv[datasetSize];
	float battery[datasetSize];
	float cons[datasetSize];
	vector<tuple<int,int,float>> prosumers;
	bool availableConsumers[datasetSize];
	float currentWeight;
	vector<int> neighbors;
	int neighborCount;
	int l;
	int procheck;
	bool precompute;
	vector<int> prosumerList;
	vector<int> consumerList;
	int prosumerCount;
	int consumerCount;
	int length;	// only ever use this length when it concerns the whole dataset !!!Trust Me!!!
} myData;

int main(int argc, char *argv[])
{

	time_t t1 = time(NULL);
	Py_Initialize();

	// These 2 lines just allow the interpreter to access python files in the current directory
	// this means that the shell running the process has to be in python/ even if the executable is in another e.g. python/build/
	PyRun_SimpleString("import sys");
	PyRun_SimpleString("sys.path.append(\".\")");


	if (argc == 4){
		myData.precompute = true;

		//probably should sanitize this but whatever!?!?!?
		int length = strlen(argv[3])+1;
		char filePath[length];
		strcpy(filePath,argv[3]);

		cout << "Precompute mode activated! Using file: " << argv[3] << endl;
		precomputePython(filePath);

		myData.procheck = stoi(argv[1]);
		myData.l = stoi(argv[2])+1;
	}
	else if (argc == 3)
	{
		myData.precompute = false;
		myData.procheck = stoi(argv[1]);
		myData.l = stoi(argv[2])+1;

		myData.l = datasetSize;
		myData.procheck = stoi(argv[1]);
		cout << "Standard mode activated!" << endl;
	}
	else if (argc == 2)
	{
		myData.precompute = false;
		myData.l = datasetSize;
		myData.procheck = stoi(argv[1]);
		cout << "Standard mode activated!" << endl;
		cout << "l value not specified so no cap implemented" << endl;
	}
	else
	{
		myData.precompute = false;
		myData.l = datasetSize;
		myData.procheck = datasetSize;
		cout << "Standard mode activated!" << endl;
		cout << "i value not specified defaulting" << endl;
		cout << "l value not specified so no cap implemented" << endl;
	}







	myData.neighborCount = 0; // simple init value to avoid undefined behavior
	// NOTE Not sure if this is used anywhere. Doesn't seem to be needed.
	//pythonNeighborhood(0);

	//preprocess(false);


	

	vector<tuple<int, int, double>> result;
	if(myData.procheck){
		result = doublegreedyMatching(myData.prosumerList, myData.consumerList);
	}else{
		greedyMatching(myData.prosumerList, myData.consumerList);
	}


	
	if (Py_FinalizeEx() < 0)
	{
		return 120;
	}
	time_t t2 = time(NULL);
	double diff = difftime(t2, t1);
	printf("\n\033[91m%.f\033[m seconds since start of execution.\n", diff);

	// A file for storing the results of the run
	string path = "result.txt";
	ofstream file;
	file.open(path);
	if (!file.is_open())
	{
		cout << "error couldnt open output file\n";
		return EXIT_FAILURE;
	}

	double sum = 0;
	cout << myData.prosumers.empty() << endl;
	while(!myData.prosumers.empty()){
		tuple<int,int,float> match = myData.prosumers.back();
		myData.prosumers.pop_back();
		sum += get<2>(match);

		//writing the match to the file
		file << "Prosumer: " << get<0>(match) << endl;
		file << "Consumer: " << get<1>(match) << endl;
		file << "Weight: " << get<2>(match) << endl;

		cout << endl << "Prosumer " << "\033[94m" << get<0>(match) << "\033[m" << " is matched to consumer " << "\033[94m" << get<1>(match) << "\033[m" << " which saves " << "\033[92m" << get<2>(match) << "\033[m" << endl;
	}
	printf("\n\033[92m%.6f\033[m saved in total across all pairs.\n", sum);

	file.close();

	return EXIT_SUCCESS;
}

int greedyMatching(vector<int> prosumersList, vector<int> consumersList)
{
	size_t prosumerSize = prosumersList.size(); //equal to the amount of prosumers in vector prosumersList
	size_t consumerSize = consumersList.size(); //equal to the amount of consumers in vector consumersList
	bool available[myData.length];				//available consumers

	for (size_t i = 0; i < prosumerSize; i++){
		available[prosumersList[i]] = false;
	}

	for (size_t i = 0; i < consumerSize; i++){
		available[consumersList[i]] = true;
	}
	for (size_t i = 0; i < prosumerSize; i++)
	{
		cout << endl << "******* Prosumer " << "\033[94m" << i+1 << "\033[m" << "/" << "\033[94m" << prosumerSize << "\033[m" << " *******" << endl;

		// finds the neighborhood via a python function see below for indepth
		cout << prosumersList[i] << endl;
		if(pythonNeighborhood(prosumersList[i])){
			cout << "error when getting neighborHood from python" << endl;
		}

		// setting this as -10 for so it is easier to see when the output is clearly false
		// should change for 0 later
		float currentBestWeight = -10;
		int currentBestIndex = -1;

		/*
		//debug code to check the neighborhoods of the prosumers
		cout << "Prosumer " << i+1 << " with the index: " << prosumersList[i] << " has the following neighbors: " << endl;
		for(int j=0; j<myData.neighborCount; j++){
			cout << myData.neighbors[j] << endl;
		}
		cout << "\n\n" << endl;
		if(i>2){
			return 0;
		}
		continue;
		*/




		// makes sure l doesn't account for unavailable neighbors
		int count = 0;
		// when empty it simply makes no match which is equivalent to having no neighbors
		for (int j = 0; j < myData.neighborCount && j < myData.l + count; j++)
		{

			int maxRounds = min((myData.l + count), myData.neighborCount);
			cout << "Checking neighbours " << "\033[94m" << j + 1 << "\033[m" << "/" << "\033[94m" << maxRounds << "\t" << "\033[m" << loadingBar(float(j + 1) / float((maxRounds))) << "\t\r" << flush;


			// checking if edge j in the neighborhood is available
			if (!available[myData.neighbors[j]]){
				// this counts prosumers too which means we always examine the entire neighborhood or l+1 available consumers depending on which of these values is smaller
				count++;
				continue;
			}

			int list[2] = {prosumersList[i], myData.neighbors[j]};
			pythonOptimizer(2, list);
			if (currentBestWeight < myData.currentWeight)
			{
				currentBestWeight = myData.currentWeight;
				currentBestIndex = myData.neighbors[j];
			}
		}
		cout << endl << "best index: " << "\033[92m" << currentBestIndex << "\033[m" << "\nbest weight: " << "\033[92m" << currentBestWeight << "\033[m" << endl;

		// matching the prosumer to its consumer and marking the consumer as unavailable
		tuple<int,int,float> match = make_tuple(prosumersList[i], currentBestIndex, currentBestWeight);
		myData.prosumers.push_back(match);
		available[currentBestIndex] = false;
	}
	return EXIT_SUCCESS;
}

vector<tuple<int, int, double>>  doublegreedyMatching(vector<int> prosumersList, vector<int> consumersList){

	size_t prosumerSize = prosumersList.size(); //equal to the amount of prosumers in vector prosumersList
	size_t consumerSize = consumersList.size(); //equal to the amount of consumers in vector consumersList
	bool consumers[myData.length];				//list of consumers at their actual index for fast lookups
	bool available[myData.length];				//list of available prosumers/consumers


	for (size_t i = 0; i < prosumerSize; i++){
		consumers[prosumersList[i]] = false;
	}
	for (size_t i = 0; i < consumerSize; i++){
		consumers[consumersList[i]] = true;
	}



	for (size_t i = 0; i < consumerSize+prosumerSize; i++){
		available[i] = true;
	}


	vector<tuple<int, int, double>> allmatchings;

	size_t size1 = prosumersList.size();
	cout << "Size: " << prosumerSize << endl;


	for(size_t i = 0; i < prosumerSize; i++)
	{
		int k = 0;
		int household;
		//cout << "do we enter?" << i << endl;

		cout << "Starting with prosumer " << prosumersList[i] << ". Is available: " << available[prosumersList[i]] << endl;
		if (available[prosumersList[i]])
		{
			//cout << "fail here?3" << endl;
			vector<Edge> tempGraph;
			int o = 1;
			household = prosumersList[i];
			while(o == 1 || k > 100){
				pair<int, float> result = next_edge_greedy_path(household, &tempGraph, prosumersList.size(), available, consumers);
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

					if (consumers[edge.source]) //if the source vertex is a consumer
					{
						cout << "skiped since index " << edge.source << " is a consumer" << endl;
						continue; //we skip, since we only want prosumer to consumer,  other, if we dont remove this we could get groups of size 3.
					}


					tuple<int,int,float> match = make_tuple(edge.source, edge.destination, edge.weight);
					myData.prosumers.push_back(match);
					allmatchings.push_back(match);
					cout << "added a edge with origin " << edge.source << " to the graph" << endl;
					available[edge.source] = false;
					available[edge.destination] = false;
				}
			}
		}
	}

	cout << "Size of result inside doublegreedyMatching: " << allmatchings.size() << endl;
	return allmatchings;
}


pair<int, float> next_edge_greedy_path(int household, vector<Edge> *tempgraph, int prosumersize, bool available[], bool consumers[]) {

	cout << "we are checking household number " << household << endl;
	if(pythonNeighborhood(household)){
		cout << "error when getting neighborHood from python" << endl;
	}

	vector<int> N; //vi kan möjligtvis begränsa längden av N genom att göra den l+1 lång array.

	if(!consumers[household]){
		int count = 0;
		for (int i = 0; i < myData.neighborCount && i < myData.l + count; i++)
		{
			if(!available[myData.neighbors[i]]){
				count++;
			}else{
				if(canCreateNewEdge(tempgraph, household, myData.neighbors[i])){
					if(consumers[myData.neighbors[i]]){
						N.push_back(myData.neighbors[i]);
					}else{
						count++;
					}
				}else{
					count++;
				}

			}
		}
	}
	else{
		int count = 0;
		for (int i = 0; i < myData.neighborCount && i < myData.l + count; i++)
		{
			if(!available[myData.neighbors[i]]){
				count++;
			}else{
				if(canCreateNewEdge(tempgraph, household, myData.neighbors[i])){
					if(!consumers[myData.neighbors[i]]){
						N.push_back(myData.neighbors[i]);
					}else{
						count++;
					}
				}else{
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
				int list[2];
				// the function runOptimize(indexList) is picky about the order of the arguments
				if(!consumers[household]){
					list[0] = household;
					list[1] = N[i];
				}
				else{
					list[0] = N[i];
					list[1] = household;
				}
				pythonOptimizer(2, list);
				if (weight < myData.currentWeight)
				{
					weight = myData.currentWeight;
					j = N[i];
				}
			}
		}
		else{
			j = N[0];
			int list[2];
			if(!consumers[household]){
				list[0] = household;
				list[1] = j;
			}
			else{
				list[0] = j;
				list[1] = household;
			}
			pythonOptimizer(2, list);
			weight = myData.currentWeight;
		}
	}
	else{
		j = -1;
	}


	return make_pair(j, weight);
}


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

int makearrNeighbors(PyObject *list)
{
	myData.neighbors.clear();
	for (int i = 0; i < myData.neighborCount; ++i)
	{
		myData.neighbors.push_back(PyLong_AsLong(PyList_GetItem(list, i)));
	}
	cout << myData.neighbors.front() << endl;
	return EXIT_SUCCESS;
}

int makearrLists(PyObject *prosumerList, PyObject *consumerList)
{
	myData.prosumerCount = PyList_Size(prosumerList);
	myData.consumerCount = PyList_Size(consumerList);
	for (int i = 0; i < myData.prosumerCount; ++i)
	{
		myData.prosumerList.push_back(PyLong_AsLong(PyList_GetItem(prosumerList, i)));
	}
	for (int i = 0; i < myData.consumerCount; ++i)
	{
		myData.consumerList.push_back(PyLong_AsLong(PyList_GetItem(consumerList, i)));
	}
	return EXIT_SUCCESS;
}

int precomputePython(char *fileName)
{
	PyObject *pName, *pModule, *pFunc;
	PyObject *pArgs, *pValue;

	if(!myData.precompute){
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
			else{
				return EXIT_FAILURE;
			}
			Py_XDECREF(pFunc);
			Py_DECREF(pModule);
			return EXIT_SUCCESS;
		/* End of initialization */
		}
		return EXIT_FAILURE;

	}else{

		// the following code starts two global 2 objects on the python by calling 2 python functions
		// the exection is almost identical for these and they take a lot of space but it works for now

		/* Initializes the cost */
		pName = PyUnicode_DecodeFSDefault(preComputeRunOpt);
		/* Error checking of pName left out */

		pModule = PyImport_Import(pName);
		Py_DECREF(pName);
		if (pModule != NULL)
		{
			pFunc = PyObject_GetAttrString(pModule, "loadCost");
			/* pFunc is a new reference */

			if (pFunc && PyCallable_Check(pFunc))
			{
				pArgs = PyTuple_New(1);
				PyTuple_SetItem(pArgs, 0, PyUnicode_DecodeFSDefault(fileName));

				PyObject_CallObject(pFunc, pArgs);
				cout << "Initialized cost" << endl;
			}
			else{
				return EXIT_FAILURE;
			}
			Py_XDECREF(pFunc);
			Py_DECREF(pModule);
			/* End of initialization */
		}else{
			return EXIT_FAILURE;
		}
		/* Initializes the neighborHood dictionary */
		pName = PyUnicode_DecodeFSDefault(preComputeNeighborhoods);
		/* Error checking of pName left out */

		pModule = PyImport_Import(pName);
		Py_DECREF(pName);
		if (pModule != NULL)
		{
			pFunc = PyObject_GetAttrString(pModule, "start");
			/* pFunc is a new reference */

			if (pFunc && PyCallable_Check(pFunc))
			{
				PyObject_CallObject(pFunc, NULL);
				cout << "Initialized neighborHood" << endl;
			}
			else{
				return EXIT_FAILURE;
			}
			Py_XDECREF(pFunc);
			Py_DECREF(pModule);
			/* End of initialization */
		}else{
			return EXIT_FAILURE;
		}

		/* Grabs the total amount of prosumers and consumers */
		pName = PyUnicode_DecodeFSDefault(preComputeRunOpt);
		/* Error checking of pName left out */

		pModule = PyImport_Import(pName);
		Py_DECREF(pName);
		if (pModule != NULL)
		{
			pFunc = PyObject_GetAttrString(pModule, "getLength");
			/* pFunc is a new reference */

			if (pFunc && PyCallable_Check(pFunc))
			{
				pArgs = PyTuple_New(1);
				PyTuple_SetItem(pArgs, 0, PyUnicode_DecodeFSDefault(fileName));

				pValue = PyObject_CallObject(pFunc, pArgs);
				myData.length = PyLong_AsLong(pValue);
				Py_XDECREF(pValue);
				cout << myData.length << endl;
				cout << "Initialized length" << endl;
			}
			else{
				return EXIT_FAILURE;
			}
			Py_XDECREF(pFunc);
			Py_DECREF(pModule);
			/* End of initialization */
		}else{
			return EXIT_FAILURE;
		}

		return precomputePythonNeighborhood(preComputeNeighborhoods, findNeighborhoods, fileName);
	}
}

int precomputePythonNeighborhood(const char *name, const char *function, char *fileName)
{
	PyObject *pName, *pModule, *pFunc;
	PyObject *pArgs, *pValue;
	PyObject *pTup1, *pTup2;


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
			pArgs = PyTuple_New(1);
			PyTuple_SetItem(pArgs, 0, PyUnicode_DecodeFSDefault(fileName));

			pValue = PyObject_CallObject(pFunc, pArgs);
			Py_DECREF(pArgs);

			if (pValue != NULL)
			{
				// saving the data to global variables
				pTup1 = PyTuple_GetItem(pValue, 0);
				pTup2 = PyTuple_GetItem(pValue, 1);

				makearrLists(pTup1, pTup2);

				// GC for the lists from the tuple pValue
				Py_DECREF(pTup1);
				Py_DECREF(pTup2);
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

// prepares the data for the matching
// calling it with true will print all values used for the matching
int preprocess(bool printValues)
{
	/*
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
			Prosumer *prosumer = new Prosumer{-1, -1};
			myData.prosumers[i] = prosumer;
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
				continue;
			}
			cout << i << "\t";
		}
		cout << "\n\n";
	}
	*/
	return EXIT_SUCCESS;
}

// calls a helper function which calls the optimizer python function and returns the result
int pythonOptimizer(int indexCount, int indexes[])
{
	if(!myData.precompute){
		return pythonOptimizerOnline(runopt, runOptimize, indexCount, indexes);
	}else{
		return pythonOptimizerPreComputed(preComputeRunOpt, runOptimize, indexCount, indexes);
	}
}

//does an online calculation
int pythonOptimizerOnline(const char *name, const char *function, int indexCount, int indexes[])
{
	PyObject *pName, *pModule, *pFunc;
	PyObject *pArgs, *pValue;

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

//uses a precomputed value
int pythonOptimizerPreComputed(const char *name, const char *function, int indexCount, int indexes[])
{
	PyObject *pName, *pModule, *pFunc;
	PyObject *pArgs, *pValue;

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
			pArgs = PyTuple_New(1);
			PyObject *pList;
			pList = makelist(indexes, indexCount);
			PyTuple_SetItem(pArgs, 0, pList);

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

// calls a helper function that calls a python function that returns the neighborhood for a consumer/prosumer
// which helper function is called depends on if the application is running in the precompute mode or not
//
// What both helpers do:
// calls a python function that returns the neighborhood for a consumer/prosumer
// the neighborhood is an unsorted, NOTE this is the default but precompute can change this, array of indecies which is stored inside myData.neighbors
// myData.neighbors only lazy deletes so only the first myData.neighborCount elements are actually valid at any time
int pythonNeighborhood(int index)
{
	if(!myData.precompute){
		return pythonNeighborhoodOnline(neighborHood, findNeighborhood, index);
	}else{
		return pythonNeighborhoodPrecomputed(preComputeNeighborhoods, findNeighborhood, index);
	}
}

// 	!!!NOTE that this version also returns prosumer->prosumer or consumer->consumer neighborhoods NOTE!!!
// technically is precomputed but uses a different method and dataset than the function below
int pythonNeighborhoodOnline(const char *name, const char *function, int index)
{
	PyObject *pName, *pModule, *pFunc;
	PyObject *pArgs, *pValue;

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
			PyTuple_SetItem(pArgs, 0, PyLong_FromLong(RANGE));
			PyTuple_SetItem(pArgs, 1, PyLong_FromLong(index));

			pValue = PyObject_CallObject(pFunc, pArgs);
			Py_DECREF(pArgs);

			if (pValue != NULL)
			{
				// saving the data to global variables
				myData.neighborCount = PyList_Size(pValue);
				makearrNeighbors(pValue);

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


// 	!!!NOTE that this version only returns prosumer->consumer or consumer->prosumer neighborhoods NOTE!!!
// i.e. it can never return a neighborhood with consumers in it for a consumer
int pythonNeighborhoodPrecomputed(const char *name, const char *function, int index)
{
	PyObject *pName, *pModule, *pFunc;
	PyObject *pArgs, *pValue;

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
			pArgs = PyTuple_New(1);
			cout << index << endl;
			PyTuple_SetItem(pArgs, 0, PyLong_FromLong(index));

			pValue = PyObject_CallObject(pFunc, pArgs);
			Py_DECREF(pArgs);

			if (pValue != NULL)
			{
				// saving the data to global variables
				myData.neighborCount = PyList_Size(pValue);
				makearrNeighbors(pValue);

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
