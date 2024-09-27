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

struct 
{
	float pv[datasetSize];
	float battery[datasetSize];
	float cons[datasetSize];
} myData;

int main() {
	//this currently both optimizes index 1 and extracts the pv, battery and cons values to the files
    int results = system("python3 runopt.py 1 1");
	
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
	if(readFile("consdata.txt",consValue)){
		cout << "error couldn't read consdata.txt\n";
	}
	else{
		cout << "consumption values: \n";
		for(int i = 0; i<datasetSize; i++){
			cout << myData.cons[i] << "\t";
		}
		cout << "\n\n";
	}
    return 0;
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