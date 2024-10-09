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

using namespace std;



int readFile(string path, int type);
int preprocess(bool printValues);
int greedyMatching();

typedef struct
{
	float	pv;
	float	battery;
	float	cons;
	int		matchedToIndex;
}*Prosumer;

struct 
{
	float pv[datasetSize];
	float battery[datasetSize];
	float cons[datasetSize];
	Prosumer prosumers[datasetSize];
	bool  availableConsumers[datasetSize];
	float currentWeight;
} myData;

int main() {
	preprocess(false);
	greedyMatching();
	
	for(int i=0; i<datasetSize; i++){
		if(myData.prosumers[i] == NULL){
			continue;
		}
		else{
			//no point displaying prosumers who didn't match for now
			if(myData.prosumers[i]->matchedToIndex == -1){
				break;
			}
		}
		cout << "Prosumer " << i << "is matched to consumer " << myData.prosumers[i]->matchedToIndex << endl;
	}
	
    return EXIT_SUCCESS;
}

int greedyMatching(){
	
	// change the i<value to make it run faster when debugging
	for(int i=0; i<datasetSize; i++){
		if(myData.prosumers[i] == NULL){
			continue;
		}
		
		//insert neighborhood code here
		//then mod below to work with neighborhood instead of full set
		
		float	currentBestWeight	= -1;
		int 	currentBestIndex	= -1;
		
		//since no neighborhood code just run it on the first 10 values for now or it takes forever
		//when empty it simply makes no match which is equivalent to having no neighbors
		for(int j=0; j<10; j++){
			if(!myData.availableConsumers[j]){
				continue;
			}
			
			if (system(("python3 runopt.py 2 " + to_string(i) + " " + to_string(j)).c_str())){
					cout << "Failed to run optimizer with arguments " << to_string(i) << " and " << to_string(j) << endl;
					return EXIT_FAILURE;
				}
			else{
				readFile("weight.txt", weightValue);
				cout << "Current weight is: "<< myData.currentWeight << "\n";
				if(currentBestWeight<myData.currentWeight){
					currentBestWeight = myData.currentWeight;
					currentBestIndex  = j;
				}
			}
		}
		cout << "best index: " << currentBestIndex << "\nbest weight: " << currentBestWeight << endl;
		
		//matching the prosumer to its consumer and marking the consumer as unavailable
		myData.prosumers[i]->matchedToIndex = currentBestIndex;
		myData.availableConsumers[currentBestIndex] = false;	
	}
	return EXIT_SUCCESS;
}


//prepares the data for the matching
//calling it with true will print all values used for the matching
int preprocess(bool printValues){
	//this extracts the pv, battery and cons values to the files
	if (system("python3 preprocess.py"))
	{
		cout << "Failed to run extract" << endl;
		return EXIT_FAILURE;
	}
	
	//retrieves the pv and battery values from the files and optionally prints them all out
	//could probably generalize the following more but not sure how much of this stays
	if(readFile("pvdata.txt",pvValue)){
		cout << "error couldn't read pvdata.txt\n";
	}
	else{
		if(printValues){
			cout << "pv values: \n";
			for(int i = 0; i<datasetSize; i++){
				cout << myData.pv[i] << "\t";
			}
			cout << "\n\n";
		}
	}
	if(readFile("batterydata.txt",batteryValue)){
		cout << "error couldn't read batterydata.txt\n";
	}
	else{
		if(printValues){
			cout << "battery values: \n";
			for(int i = 0; i<datasetSize; i++){
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
	
	//initalizes the prosumer and consumer sets
	for(int i=0; i<datasetSize; i++){
		myData.prosumers[i] = NULL;
		myData.availableConsumers[i] = false;
	}
	
	//int counter = 0;
	for(int i=0; i<datasetSize; i++){
		if(myData.pv[i]!=0 || myData.battery[i]!=0){
			//initalizes a new pointer for every prosumer tuple
			Prosumer prosumer = (Prosumer)malloc(sizeof(Prosumer));
			prosumer->pv = myData.pv[i];
			prosumer->battery = myData.battery[i];
			prosumer->matchedToIndex = -1;
			myData.prosumers[i] = prosumer;
			
			/* 	
				The following makes a condensed array instead of a sparse array
				A dense array means less values to check when running the alg
				However NULL checking should be fast so they are probably about equal
				
				*** Make sure to uncomment the //int counter = 0; above and change the NULL checks to break instead of continue in that case ***
			*/
			//myData.prosumers[counter] = prosumer;
			//counter++;
		}
		else{
			myData.availableConsumers[i] = true;
		}
	}
	//prints the prosumer and consumer set
	if(printValues){
		cout << "Index values of prosumers: \n";
			for(int i = 0; i<datasetSize; i++){
				if(myData.prosumers[i] == NULL){
					//break;
					continue;
				}
				cout << i << "\t";
			}
		cout << "\n\n";
		
		cout << "Index values of available consumers: \n";
			for(int i = 0; i<datasetSize; i++){
				if(!myData.availableConsumers[i]){
					//break;
					continue;
				}
				cout << i << "\t";
			}
		cout << "\n\n";
	}
	return EXIT_SUCCESS;
}

//reads in the data from a a file
int readFile(string path, int type){
	ifstream file;
	
	if(!filesystem::exists(path)){
		return EXIT_FAILURE;
	}
	
	file.open(path);
	if(!file.is_open()){
		cout << "error\n";
		return EXIT_FAILURE;
	}
	int i = 0;
	string tmp;
	
	while (file >> tmp){
		if(type == pvValue){
			myData.pv[i] = stof(tmp);
		}
		else if(type == batteryValue){
			myData.battery[i] = stof(tmp);
		}
		else if(type == consValue){
			myData.cons[i] = stof(tmp);
		}
		else if(type == weightValue){
			myData.currentWeight = stof(tmp);
		}
		i++;
	}
	file.close();
	return EXIT_SUCCESS;
}
