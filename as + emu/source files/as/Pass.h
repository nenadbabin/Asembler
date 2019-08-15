/*
 * Pass.h
 *
 *  Created on: Aug 5, 2019
 *      Author: etf
 */

#ifndef PASS_H_
#define PASS_H_

#include <string>
#include <queue>
#include <unordered_map>
#include <regex>
#include "Enums.h"
#include "Symbol.h"
#include "Section.h"
#include "Relocation.h"

using namespace std;

class Pass {
public:
	enum State {
		START, MID, END_OF_LINE, END
	};

	Pass();
	virtual ~Pass();
	virtual void begin(string FileName) = 0;

protected:
	virtual State next(State s, queue<string> * q) = 0;

	static unordered_map<TokenType, regex> tokenParsers;
	static unordered_map<string, int> numberOfOperands;
	static unordered_map<string, regex> adressingTypeParsers;
	static unordered_map<string, u_int8_t> opCodes;

	void getTokens(ifstream &inputFile);

	static TokenType parseToken(string token);
	static int getInstructionSize(queue<string> instruction);
	static int getOperandSize(string s, int expectedOperandsSize);
	static string getOperandType(string s);

	static vector<Section> sections;
	static vector<Symbol> symbols;
	static vector<Relocation> relocations;
	static void addSymbol(Symbol * s);
	static void addSection(Section * s);
	static void addRelocation(Relocation * r);

	static Symbol * checkIfSymbolExists(string name);
	static Section * checkIfSectionExists(string name);

	static uint16_t convertStringToInt(string s, uint16_t minVal, uint16_t maxVal);
	static uint16_t stringToInt(string s);
	static uint16_t hexStringToInt(string s);

	static ofstream logFile;

	static int getLocationCounter();
	static void setLocationCounter(int newLC);
	static int incLocationCounter(int toAdd);
	static int locationCounter;

	vector<vector<string> > tokenArray;

	static int MIN8;
	static int MAX8;
	static int MIN16;
	static int MAX16;

	static vector<string> splitSeparator(string str, string sep);
	void split(const string &s, const char* delim, vector<string> & v);
};

#endif /* PASS_H_ */
