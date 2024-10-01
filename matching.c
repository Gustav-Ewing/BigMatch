#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#define datasetSize 2221
#define pvValue 0
#define batteryValue 1
#define consValue 2

using namespace std;



int readFile(string path, int type);
int preprocess();

typedef struct
{
	float	pv;
	float	battery;
	float	cons;
	int		index;
}*Prosumer;

struct 
{
	float pv[datasetSize];
	float battery[datasetSize];
	float cons[datasetSize];
	Prosumer prosumers[datasetSize];
	bool  availableConsumers[datasetSize];
} myData;

int main() {
	//this currently both optimizes index 1 and extracts the pv, battery and cons values to the files
	if (system("python3 runopt.py 1 1"))
	{
		cout << "Failed to run optimizer" << endl;
		return EXIT_FAILURE;
	}
	//could probably generalize the following more but not sure how much of this stays
	cout << "\n";
	if(readFile("pvdata.txt",pvValue)){
		cout << "error couldn't read pvdata.txt\n";
	}
	else{
		cout << "pv values: \n";
		for(int i = 0; i<datasetSize; i++){
			cout << myData.pv[i] << "\t";
		}
		cout << "\n\n";
	}
	if(readFile("batterydata.txt",batteryValue)){
		cout << "error couldn't read batterydata.txt\n";
	}
	else{
		cout << "battery values: \n";
		for(int i = 0; i<datasetSize; i++){
			cout << myData.battery[i] << "\t";
		}
		cout << "\n\n";
	}
	
	// The file this reads is huge so ignored for now
	/*
	if(readFile("consdata.txt",consValue)){
		cout << "error couldn't read consdata.txt\n";
	}
	else{
		cout << "consumption values: \n";
		for(int i = 0; i<100; i++){
			cout << myData.cons[i] << "\t";
		}
		cout << "\n\n";
	}
	*/
	
	preprocess();
	
	cout << "Index values of prosumers: \n";
		for(int i = 0; i<datasetSize; i++){
			if(myData.prosumers[i] == NULL){
				//break;
				continue;
			}
			cout << myData.prosumers[i]->index << "\t";
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
	
    return 0;
}


int preprocess(){
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
			prosumer->index = i;
			myData.prosumers[i] = prosumer;
			
			/* 	
				The following makes a condensed array instead of a sparse array
				A dense array means less values to check when running the alg
				However NULL checking should be fast so they are prolly equal
				
				*** Make sure to uncomment the //int counter = 0; above and change the NULL checks to break instead of continue in that case ***
			*/
			//myData.prosumers[counter] = prosumer;
			//counter++;
		}
		else{
			myData.availableConsumers[i] = true;
		}
	}
	
	return EXIT_SUCCESS;
}

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
		i++;
	}
	file.close();
	return EXIT_SUCCESS;
}
