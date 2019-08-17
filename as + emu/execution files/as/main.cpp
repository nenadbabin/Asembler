/*
 * main.cpp
 *
 *  Created on: Jun 25, 2019
 *      Author: etf
 */

#include <iostream>
#include <string>
#include <fstream>
#include <memory>
#include "FirstPass.h"
#include "Pass.h"
#include "SecondPass.h"

using namespace std;

int main(int argc, char ** argv) {

	if (argc < 3) {
		cout << "Error: Not enough arguments!" << endl;
		return 1;
	}

	ifstream inputFile(argv[1]);
	ofstream outputFile(argv[2]);

	string stringFileName = argv[1];

	if (!inputFile.is_open()) {
		cerr << "Error opening input file" << endl;
		return 1;
	}

	if (!outputFile.is_open()) {
		cerr << "Error opening output file" << endl;
		return 1;
	}

	unique_ptr<FirstPass> firstPass(new FirstPass());
	firstPass->begin(stringFileName);
	unique_ptr<SecondPass> secondPass(new SecondPass());
	secondPass->begin(stringFileName);
	secondPass->writeObjectFile(outputFile);


	cout << "Success" << endl;
	return 0;
}

