/*
 * ObjectFile.cpp
 *
 *  Created on: Jul 18, 2019
 *      Author: etf
 */

#include "ObjectFile.h"
#include <cstdlib>
#include <iomanip>
#include <string.h>
#include <queue>

using namespace std;

ObjectFile::ObjectFile(string fileName) :
		inputFile(fileName) {
	this->fileName = fileName;
	this->currentState = NONE;
}

ObjectFile::~ObjectFile() {
	// TODO Auto-generated destructor stub
}

void ObjectFile::handleObjectFile() {

	if (!inputFile.is_open()) {
		cerr << "Error opening input file " << fileName << endl;
		return;
	}

	//get all the tokens
	getTokens();

	queue<string> queue;

	for (auto &tokens : tokenArray) {

		for (auto &token : tokens) {
			//cout << token << endl;
			queue.push(token);
		}

	}

	while (queue.size() > 0) {

		string token = queue.front();
		queue.pop();

		if (token == "%SECTIONS%") {
			currentState = State::SECTIONS;

			for (int i = 0; i < 7; i++) {
				queue.pop();
			}

		} else if (token == "%SYMBOLS%") {
			currentState = State::SYMBOLS;

			for (int i = 0; i < 5; i++) {
				queue.pop();
			}

		} else if (token == "%RELOCATIONS%") {
			currentState = State::RELOCATIONS;

			for (int i = 0; i < 5; i++) {
				queue.pop();
			}
		} else if (token == "%END%") {
			currentState = State::NONE;
		} else if (token == "%SECTIONS_CONTENT%") {
			currentState = State::SECTIONS_CONTENT;
		}

		switch (currentState) {
		case State::SECTIONS: {
			while (fillSections(queue))
				;
			break;
		}

		case State::SYMBOLS: {
			while (fillSymbols(queue))
				;
			break;
		}
		case State::RELOCATIONS: {
			while (fillRelocations(queue))
				;
			break;
		}
		case State::SECTIONS_CONTENT: {
			while (fillSectionContent(queue))
				;
			break;
		}
		case State::NONE: {
			break;
		}
		}

	}

	/*for (int i = 0; i < sections.size(); i++) {
	 cout << sections[i]->printContent() << endl;
	 }*/

}

bool ObjectFile::fillSections(queue<string> &queue) {
	string name = queue.front();

	if (name == "%END%") {
		return false;
	}

	queue.pop();

	string size = queue.front();
	queue.pop();

	string number = queue.front();
	queue.pop();

	string r = queue.front();
	queue.pop();

	string w = queue.front();
	queue.pop();

	string e = queue.front();
	queue.pop();

	string p = queue.front();
	queue.pop();

	Section * s = new Section(name, stoi(size), stoi(number));
	s->createContent();

	if (r == "1") {
		s->setR();
	}

	if (w == "1") {
		s->setW();
	}

	if (e == "1") {
		s->setE();
	}

	if (p == "1") {
		s->setP();
	}

	sections.push_back(s);
	//cout << *s << endl;

	if (queue.front() == "%END%") {
		return false;
	} else {
		return true;
	}
}

bool ObjectFile::fillSymbols(queue<string> &queue) {
	string name = queue.front();

	if (name == "%END%") {
		return false;
	}

	queue.pop();

	string section = queue.front();
	queue.pop();

	string value = queue.front();
	queue.pop();

	string scope = queue.front();
	queue.pop();

	string number = queue.front();
	queue.pop();

	Symbol * s = new Symbol(name, stoi(number), section, stoi(value));

	if (scope == "GLOBAL") {
		s->setScope(ScopeType::GLOBAL);
	}

	if (section != "undefined" && section != "absolute") {
		Section * sectionObject = getSection(section);
		sectionObject->addSymbol(s);
	}

	symbols.push_back(s);
	//cout << *s << endl;

	if (queue.front() == "%END%") {
		return false;
	} else {
		return true;
	}
}

bool ObjectFile::fillRelocations(queue<string> &queue) {
	string offset = queue.front();

	if (offset == "%END%") {
		return false;
	}

	queue.pop();

	string section = queue.front();
	queue.pop();

	string symbol = queue.front();
	queue.pop();

	string relativeTo = queue.front();
	queue.pop();

	string type = queue.front();
	queue.pop();

	Symbol * symbolObject = getSymbol(symbol);
	Section * sectionObject = getSection(section);
	RelocationType relocationType = RelocationType::R_16;

	if (type == "R_16") {
		relocationType = RelocationType::R_16;
	} else {
		relocationType = RelocationType::R_PC16;
	}

	Relocation * r = new Relocation(symbolObject, relocationType, stoi(offset),
			sectionObject);

	relocations.push_back(r);
	//cout << *r << endl;

	if (queue.front() == "%END%") {
		return false;
	} else {
		return true;
	}
}

bool ObjectFile::fillSectionContent(queue<string> &queue) {
	string sectionName = queue.front();

	if (sectionName == "%END%") {
		return false;
	}

	queue.pop();

	//cout << sectionName << endl;
	Section * sectionObject = getSection(sectionName);

	int tmp = sectionObject->getSize();
	int lc = 0;

	while (tmp > 0) {

		string hexValueString = queue.front();
		queue.pop();

		int intValue = hexStringToInt(hexValueString);
		u_int8_t valueToWrite(intValue);
		sectionObject->writeTo(&valueToWrite, lc, 1);

		tmp--;
		lc++;
	}

	string dotEnd = queue.front();
	queue.pop();

	//cout << dotEnd << endl;

	if (dotEnd != ".end") {
		throw runtime_error(
				"Error while parsing object file: Expected .end, but found "
						+ dotEnd + ".");
	}

	if (queue.front() == "%END%") {
		return false;
	} else {
		return true;
	}
}

int ObjectFile::hexStringToInt(string newS) {
	unsigned int x;
	std::stringstream ss;

	string temp = "";

	if (newS.size() > 4) {
		throw runtime_error(
				"String " + newS
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

Symbol * ObjectFile::getSymbol(string name) {

	for (auto &symbol : symbols) {
		if (symbol->getName() == name) {
			return symbol;
		}
	}

	return nullptr;
}

Section * ObjectFile::getSection(string name) {

	for (auto &section : sections) {
		if (section->getName() == name) {
			return section;
		}
	}

	return nullptr;
}

void ObjectFile::getTokens() {
	string line;

	tokenArray.clear();

	while (getline(inputFile, line)) {

		//strip comments
		size_t found = line.find(';');
		if (found != string::npos) {
			line = line.substr(0, found);
		}

		vector<string> tokens;
		split(line, " ,\t\n", tokens);

		if (tokens.size() == 0)
			continue;

		/*if (tokens[0] == ".end") {
		 tokenArray.push_back(tokens);
		 break;
		 }*/

		tokenArray.push_back(tokens);

	}
}

void ObjectFile::split(const string &s, const char* delim, vector<string> & v) {
// to avoid modifying original string
// first duplicate the original string and return a char pointer then free the memory
	char * dup = strdup(s.c_str());
	char * token = strtok(dup, delim);
	while (token != NULL) {
		v.push_back(string(token));
		// the call is treated as a subsequent calls to strtok:
		// the function continues from where it left in previous invocation
		token = strtok(NULL, delim);
	}
	free(dup);
}

vector<Symbol *> ObjectFile::getSymbols() {
	return symbols;
}

vector<Section *> ObjectFile::getSections() {
	return sections;
}

vector<Relocation *> ObjectFile::getRelocations() {
	return relocations;
}
