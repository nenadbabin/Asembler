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
#include <vector>
#include "ObjectFile.h"
#include <unordered_map>
#include "Linker.h"
#include "Emulator.h"
#include <regex>

using namespace std;

vector<string> split(string str, string sep) {
	char* cstr = const_cast<char*>(str.c_str());
	char* current;
	vector<std::string> arr;
	current = strtok(cstr, sep.c_str());
	while (current != NULL) {
		arr.push_back(current);
		current = strtok(NULL, sep.c_str());
	}
	return arr;
}

uint16_t hexStringToInt(string s) {
	uint16_t x;
	std::stringstream ss;

	string newS = s.substr(2, s.size());
	string temp = "";

	if (newS.size() > 4) {
		throw runtime_error(
				"String " + s
						+ " has to consist of at most 4 characters after 0x.");
	}

	int toFill = 8 - newS.size();

	if (toFill != 0) {
		if (newS[0] == '8' || newS[0] == '9' || newS[0] == 'a' || newS[0] == 'b'
				|| newS[0] == 'c' || newS[0] == 'd' || newS[0] == 'e'
				|| newS[0] == 'f') {
			for (int i = 0; i < toFill; i++) {
				temp += 'f';
			}
		} else {
			for (int i = 0; i < toFill; i++) {
				temp += '0';
			}
		}
	}

	temp += newS;

	ss << std::hex << temp;
	ss >> x;
	// output it as a signed type
	//std::cout << static_cast<int>(x) << std::endl;

	return x;
}

int main(int argc, char ** argv) {

	if (argc < 3) {
		cout << "Error: Not enough arguments!" << endl;
		return 1;
	}

	regex placeArg(
			"^\\-place\\=\\.([a-zA-Z_][a-zA-Z0-9]*)\\@(0x[0-9abcdef]{4})$");
	regex fileName("^([a-zA-Z0-9_]*\\.txt)$");
	regex writeToFile("^(true|false)$");

	if (!regex_match(argv[1], writeToFile)) {
		throw runtime_error(
				"Error while reading cmd line argumenrs: First argument has to be true or false.");
	}

	unordered_map<string, u_int16_t> startAdresses;
	vector<ObjectFile *> files;

	for (int i = 2; i < argc; i++) {
		string argument = argv[i];
		if (regex_match(argument, placeArg)) {
			vector<string> tokens = split(argument, "@");
			uint16_t value = hexStringToInt(tokens[1]);
			string sectionName = split(tokens[0], "=")[1];
			startAdresses[sectionName] = value;
		} else if (regex_match(argument, fileName)) {
			ifstream inputFile(argument);
			if (!inputFile.is_open()) {
				cerr << "Error opening input file " << argument << endl;
				return 1;
			}
			ObjectFile * objectFile = new ObjectFile(argument);
			files.push_back(objectFile);
		} else {
			throw runtime_error("Cmd line parameters are not correct: " + argument);
		}
	}

	for (auto &file : files) {
		file->handleObjectFile();
	}

	vector<AccessRights *> accessRights;
	Linker * linker = new Linker(files);
	unordered_map<string, vector<u_int8_t>> sectionsContent = linker->link(
			startAdresses, accessRights);

	Emulator * emul = new Emulator();
	Memory * memory = emul->getMemory();
	memory->setAccessRights(accessRights);

	for (auto &content : sectionsContent) {
		string sectionName = content.first;
		u_int16_t size = content.second.size();
		u_int16_t start = startAdresses[sectionName];
		memory->writeBlock(start, start + size, content.second);
	}

	for (auto &file : files) {
		for (auto &symbol : file->getSymbols()) {
			if (symbol->getName() == "_start") {
				emul->setPC(symbol->getValue());
			}
		}
	}

	if (regex_match(argv[1], regex("^true$"))) {
		emul->setWriteToFile();
	}

	emul->start();

	return 0;
}

