/*
 * Pass.cpp
 *
 *  Created on: Aug 5, 2019
 *      Author: etf
 */

#include "Pass.h"
#include <iostream>
#include <stdlib.h>
#include <iomanip>
#include <fstream>

using namespace std;

Pass::Pass() {
	// TODO Auto-generated constructor stub

}

Pass::~Pass() {
	// TODO Auto-generated destructor stub
}

unordered_map<TokenType, regex> Pass::tokenParsers =
		{ { GLOB_EXT, regex("^(\\.global|\\.extern)$") }, { LABEL, regex(
				"^([a-zA-Z_][a-zA-Z0-9]*):$") }, { SECTION, regex(
				"^\\.(text|data|bss|end|section)$") }, { DIRECTIVE, regex(
				"^\\.(byte|word|align|skip|equ)$") }, { SYMBOL, regex(
				"^([a-zA-Z_][a-zA-Z0-9]*)$") }, { OPERAND_REGISTRY, regex(
				"^r([0-9]{1})([h,l]?)$") }, { OPERAND_REGISTRY_SUB, regex(
				"^(pc|sp|psw)") }, { OPERAND_DEC, regex("^([-]?[0-9]+)$") }, {
				OPERAND_HEX, regex("^(0x[0-9abcdef]+)$") },
				{ INSTRUCTION,
						regex(
								"^(halt|xchg|int|mov|add|sub|mul|div|cmp|not|and|or|xor|test|shl|shr|push|pop|jmp|jeq|jne|jgt|call|ret|iret)([b,w]?)$") } };

unordered_map<string, int> Pass::numberOfOperands = { { "halt", 0 },
		{ "xchg", 2 }, { "int", 1 }, { "mov", 2 }, { "add", 2 }, { "sub", 2 }, {
				"mul", 2 }, { "div", 2 }, { "cmp", 2 }, { "not", 1 },
		{ "and", 2 }, { "or", 2 }, { "xor", 2 }, { "test", 2 }, { "shl", 2 }, {
				"shr", 2 }, { "push", 1 }, { "pop", 1 }, { "jmp", 1 }, { "jeq",
				1 }, { "jne", 1 }, { "jgt", 1 }, { "call", 1 }, { "ret", 0 }, {
				"iret", 0 } };

unordered_map<string, regex> Pass::adressingTypeParsers =
		{ { "regDir", regex("^((r([0-9]{1}))|(sp|pc|psw))([h,l]?)$") },
				{ "regIndOffSymb",
						regex(
								"^((r([0-9]{1}))|(sp|pc|psw))([h,l]?)\\[([a-zA-Z_][a-zA-Z0-9]*)\\]$") },
				{ "regIndOffDec",
						regex(
								"^((r([0-9]{1}))|(sp|pc|psw))([h,l]?)\\[([-]?[0-9]+)\\]$") },
				{ "regIndOffHex",
						regex(
								"^((r([0-9]{1}))|(sp|pc|psw))([h,l]?)\\[(0x[0-9abcdef]+)\\]$") },
				{ "regIndNoOff", regex(
						"^\\[((r([0-9]{1}))|(sp|pc))([h,l]?)$\\]") }, {
						"immedDec", regex("^([-]?[0-9]+)$") }, { "immedHex",
						regex("^(0x[0-9abcdef]+)$") }, { "symbAbs", regex(
						"^([a-zA-Z_][a-zA-Z0-9]*)$") }, { "memDirDec", regex(
						"^\\*([0-9]+)$") }, { "memDirHex", regex(
						"^\\*(0x[0-9abcdef]+)$") }, { "symbImmAdd", regex(
						"^\\&([a-zA-Z_][a-zA-Z0-9]*)$") }, { "symbPCRel", regex(
						"^\\$([a-zA-Z_][a-zA-Z0-9]*)$") } };

unordered_map<string, u_int8_t> Pass::opCodes = { { "halt", 1 }, { "xchg", 2 },
		{ "int", 3 }, { "mov", 4 }, { "add", 5 }, { "sub", 6 }, { "mul", 7 }, {
				"div", 8 }, { "cmp", 9 }, { "not", 10 }, { "and", 11 }, { "or",
				12 }, { "xor", 13 }, { "test", 14 }, { "shl", 15 },
		{ "shr", 16 }, { "push", 17 }, { "pop", 18 }, { "jmp", 19 },
		{ "jeq", 20 }, { "jne", 21 }, { "jgt", 22 }, { "call", 23 },
		{ "ret", 24 }, { "iret", 25 }

};

int Pass::MIN8 = -128;
int Pass::MAX8 = 127;
int Pass::MIN16 = -32768;
int Pass::MAX16 = 32767;

ofstream Pass::logFile;
int Pass::locationCounter = 0;
vector<Symbol> Pass::symbols;
vector<Section> Pass::sections;
vector<Relocation> Pass::relocations;

TokenType Pass::parseToken(string token) {
	TokenType ret = ILLEGAL;

	if (regex_search(token, tokenParsers[SYMBOL]))
		ret = SYMBOL;

	for (auto &parseRule : tokenParsers) {
		if (parseRule.first == SYMBOL)
			continue;

		if (regex_search(token, parseRule.second)) {
			if (ret == ILLEGAL || ret == SYMBOL) {
				ret = (TokenType) parseRule.first;
			} else {
				throw runtime_error("Ambigous token! " + token);
			}
		}
	}

	logFile << "token " << token << " parsed as " << (TokenType) ret << endl;

	if (ret == ILLEGAL)
		throw runtime_error("Illegal token: " + token);

	return ret;
}

int Pass::getInstructionSize(queue<string> instrution) {

	int size = 1; // opcode

	string instructionName = instrution.front();
	instrution.pop();

	int expectedSizeOfOperands = 2;

	if (regex_search(instructionName,
			regex(
					"^(halt|xchg|int|mov|add|sub|mul|cmp|not|and|or|xor|test|shl|shr|push|pop|jmp|jeq|jne|jgt|call|ret|iret)(b)$"))) {

		expectedSizeOfOperands = 1;
	}

	string instrCopy = instructionName;

	if (regex_search(instrCopy,
			regex(
					"^(halt|xchg|int|mov|add|sub|mul|cmp|not|and|or|xor|test|shl|shr|push|pop|jmp|jeq|jne|jgt|call|ret|iret)(b|w)$"))) {

		instrCopy = instrCopy.substr(0, instrCopy.size() - 1);
		//cout << instructionName << endl;
		//cout << instrCopy << endl;
	}

	int numOfOperands = numberOfOperands[instrCopy];

	if (instrution.size() != numOfOperands) {
		throw runtime_error(
				("Expected " + numOfOperands)
						+ (" for instruction " + instructionName));
	}

	size += numOfOperands;

	int temp = numOfOperands;

	while (temp > 0) {
		string op = instrution.front();
		int opSize = getOperandSize(op, expectedSizeOfOperands);
		size += opSize;

		instrution.pop();
		temp--;
	}

	return size;
}

int Pass::getOperandSize(string s1, int expectedSizeOfOperands) {

	string type = "illegal";
	string s = "";
	s += s1;

	if (regex_search(s, adressingTypeParsers["symbAbs"]))
		type = "symbAbs";

	for (auto &parseRule : adressingTypeParsers) {
		if (parseRule.first == "symbAbs")
			continue;

		if (regex_search(s, parseRule.second)) {
			if (type == "illegal" || type == "symbAbs") {
				type = parseRule.first;
			} else {
				throw runtime_error("Ambigous token! " + s);
			}
		}
	}

	logFile << "GetOperandSize: Operand " << s << " parsed as " << type << endl;

	if (type == "illegal")
		throw runtime_error(
				"Error while getting size of operand: Illegal operand: " + s);

	if (type == "regDir" || type == "regIndNoOff") {
		return 0;
	}

	if (type == "memDirDec" || type == "memDirHex") {
		return 2;
	}

	if (type == "symbAbs" || type == "symbImmAdd" || type == "symbPCRel") { //mozda da se izmeni
		return 2;
	}

	if (type == "regIndOffDec" || type == "regIndOffHex") {
		vector<string> v = splitSeparator(s, "[");
		vector<string> v1 = splitSeparator(v[1], "]");
		string final = v1[0];

		if (regex_search(final, tokenParsers[OPERAND_DEC])) {
			int num = stringToInt(final);

			if (num > 0x00ff) {
				if (num > 0x0000ffff) {
					throw runtime_error("Value too big.");
				} else {
					return 2;
				}
			} else {

				if (num == 0) { //reg ind no offset
					return 0;
				}
				return 1;
			}
		} else {
			int num = hexStringToInt(final);

			if (num > 0x00ff) {
				if (num > 0x0000ffff) {
					throw runtime_error("Value too big.");
				} else {
					return 2;
				}
			} else {
				return 1;
			}
		}
	}

	// ovde moze da se ispita dalje da li je sve ok (npr. immed je 2B, a operadni su 1B)
	if (type == "immedDec" || type == "immedHex") {
		if (expectedSizeOfOperands == 2) {
			return 2;
		} else {
			return 1;
		}
	}

	if (type == "regIndOffSymb") {
		return 2;
	}

	return -1;
}

string Pass::getOperandType(string s) {

	string type = "illegal";

	if (regex_search(s, adressingTypeParsers["symbAbs"]))
		type = "symbAbs";

	for (auto &parseRule : adressingTypeParsers) {
		if (parseRule.first == "symbAbs")
			continue;

		if (regex_search(s, parseRule.second)) {
			if (type == "illegal" || type == "symbAbs") {
				type = parseRule.first;
			} else {
				throw runtime_error("Ambigous token! " + s);
			}
		}
	}

	if (type == "illegal")
		throw runtime_error(
				"Error while getting type of operand: Illegal operand: " + s);

	logFile << "GetOperandType: Operand " << s << " parsed as " << type << endl;

	return type;
}

Symbol * Pass::checkIfSymbolExists(string name) {

	for (auto &symbol : symbols) {
		if (symbol.getName() == name) {
			return &symbol;
		}
	}

	return 0;
}

Section * Pass::checkIfSectionExists(string name) {

	for (auto &section : sections) {
		if (section.getName() == name) {
			return &section;
		}
	}

	return 0;
}

void Pass::getTokens(ifstream &inputFile) {
	string line;

	tokenArray.clear();

	while (getline(inputFile, line)) {

		//strip comments
		/*size_t found = line.find(';');
		if (found != string::npos) {
			line = line.substr(0, found);
		}*/

		vector<string> tokens;
		split(line, " ,\t\n", tokens);

		if (tokens.size() == 0)
			continue;

		if (tokens[0] == ".end") {
			tokenArray.push_back(tokens);
			break;
		}

		tokenArray.push_back(tokens);

	}
}

uint16_t Pass::convertStringToInt(string s, uint16_t minVal, uint16_t maxVal) {
	TokenType type = parseToken(s);
	uint16_t value = 0;

	if (type == TokenType::OPERAND_DEC) {
		value = stringToInt(s);
	} else if (type == TokenType::OPERAND_HEX) {

		if (s.size() > 10) {
			throw runtime_error(
					"Error in converting hex value from string to int. Expected 0xnnnnnnnn where n is hex char or number.");
		}
		value = hexStringToInt(s);
	} else {
		throw runtime_error(
				"Conversion from string to int error: Expected hex or dec value, but other type of token found.");
	}

	//cout << value;

	if (minVal == 0x0000 && maxVal == 0x00ff) {
		if ((uint8_t)value < minVal || (uint8_t)value > maxVal) {
			throw runtime_error(
					"Error in converting string to int: Expected value was not between min and max value.");
		}

		return (uint8_t)value;
	}

	//cout << hex << (unsigned)minVal << " " << (unsigned)maxVal << " " << (unsigned)value << " " << s << endl;
	if (value < minVal || value > maxVal) {
		throw runtime_error(
				"Error in converting string to int: Expected value was not between min and max value.");
	}

	return value;
}

int Pass::getLocationCounter() {
	return locationCounter;
}

void Pass::setLocationCounter(int newLC) {
	locationCounter = newLC;
}

int Pass::incLocationCounter(int toAdd) {
	locationCounter += toAdd;

	return locationCounter;
}

void Pass::addSymbol(Symbol * s) {
	symbols.push_back(*s);
}

void Pass::addSection(Section * s) {
	sections.push_back(*s);
}

void Pass::addRelocation(Relocation * r) {
	relocations.push_back(*r);
}

void Pass::split(const string &s, const char* delim, vector<string> & v) {
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

vector<string> Pass::splitSeparator(string str, string sep) {
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

uint16_t Pass::stringToInt(string s) {
	// object from the class stringstream
	stringstream geek(s);

	// The object has the value 12345 and stream
	// it to the integer x
	uint16_t x = 0;
	geek >> x;

	// Now the variable x holds the value 12345
	return x;
}

uint16_t Pass::hexStringToInt(string s) {
	uint16_t x;
	stringstream ss;

	string newS = s.substr(2, s.size());

	ss << hex << newS;
	ss >> x;
	// output it as a signed type
	//std::cout << static_cast<int>(x) << std::endl;

	return x;
}
