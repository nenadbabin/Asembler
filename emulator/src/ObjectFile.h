/*
 * ObjectFile.h
 *
 *  Created on: Jul 18, 2019
 *      Author: etf
 */

#ifndef OBJECTFILE_H_
#define OBJECTFILE_H_

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include "Symbol.h"
#include "Relocation.h"
#include "Section.h"
#include <queue>

using namespace std;

class ObjectFile {
public:
	explicit ObjectFile(string fileName);
	virtual ~ObjectFile();

	void handleObjectFile();

	string getFileName() {
		return this->fileName;
	}

	vector<Symbol *> getSymbols();
	vector<Section *> getSections();
	vector<Relocation *> getRelocations();
	Symbol * getSymbol(string name);
	Section * getSection(string name);

	enum State {
		RELOCATIONS, SYMBOLS, SECTIONS, NONE, SECTIONS_CONTENT
	};
private:
	string fileName;
	vector<Symbol *> symbols;
	vector<Relocation *> relocations;
	vector<Section *> sections;
	vector<vector<string>> tokenArray;

	ifstream inputFile;

	void getTokens();
	void split(const string &s, const char* delim, vector<string> & v);
	int hexStringToInt(string s);
	State currentState;

	bool fillSymbols(queue<string> &queue);
	bool fillSections(queue<string> &queue);
	bool fillRelocations(queue<string> &queue);
	bool fillSectionContent(queue<string> &queue);
};

#endif /* OBJECTFILE_H_ */
