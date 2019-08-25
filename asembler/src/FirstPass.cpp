/*
 * FirstPass.cpp
 *
 *  Created on: Aug 5, 2019
 *      Author: etf
 */

#include "FirstPass.h"
#include <iostream>
#include <fstream>

using namespace std;

int FirstPass::number = 0;
string FirstPass::currentSection = "unknown";
queue<char> FirstPass::accessQueue;

FirstPass::FirstPass() {
	logFile.open("assembly.log", ios::out | ios::trunc);

}

FirstPass::~FirstPass() {
	logFile.close();
}

void FirstPass::begin(string fileName) {
	ifstream inputFile(fileName);
	getTokens(inputFile);

	queue<string> queue;

	int i = 1;
	for (auto &tokens : tokenArray) {

		for (auto &token : tokens) {
			queue.push(token);
		}

		Pass::State s = Pass::START;

		while ((s = this->next(s, &queue)) != Pass::END) {
		}

		logFile << "Line " << i << " parsed successfully" << endl;
		i++;
	}

	logFile << "------SYMBOLS (after first pass):------" << endl;
	for (auto i = symbols.begin(); i != symbols.end(); ++i)
		logFile << *i << endl;

	logFile << "------SECTIONS (after first pass):------" << endl;
	for (auto i = sections.begin(); i != sections.end(); ++i)
		logFile << *i << endl;

	for (auto i = sections.begin(); i != sections.end(); ++i)
		i->createContent();

}

Pass::State FirstPass::next(Pass::State s, queue<string> * queue) {
	switch (s) {
	case START: {
		return stateStart(s, queue);
	}
	case MID: {
		return stateMid(s, queue);
	}
	case END_OF_LINE: {

		if (!queue->empty()) {
			throw runtime_error(
					"Unexpected token has been found: " + queue->front());
		}

		return END;
	}
	default: {
		throw runtime_error("Error while first pass");
	}
	}
}

void FirstPass::handleDirective(string dirName, queue<string> * tokens) {

	if (dirName == ".byte") {

		tokens->pop();

		if (currentSection == ".bss" && tokens->size() > 0) {
			throw runtime_error(
					"Error while handling .byte: Content in bss section can't be initialized.");
		}

		int size = 0;
		if (tokens->size() == 0) {
			size = 1;
		} else {
			size = tokens->size();
		}

		Pass::incLocationCounter(size);

		while (tokens->size() > 0) {
			tokens->pop();
		}

		Pass::logFile << "New LC = " << Pass::getLocationCounter() << endl;

		return;
	}

	if (dirName == ".word") {
		tokens->pop();

		if (currentSection == ".bss" && tokens->size() > 0) {
			throw runtime_error(
					"Error while handling .word: Content in bss section can't be initialized.");
		}

		int size = 0;
		if (tokens->size() == 0) {
			size = 1;
		} else {
			size = tokens->size();
		}

		Pass::incLocationCounter(size * 2);

		while (tokens->size() > 0) {
			tokens->pop();
		}

		Pass::logFile << "New LC = " << Pass::getLocationCounter() << endl;

		return;
	}

	if (dirName == ".skip") {
		tokens->pop();

		if (tokens->size() == 0) {
			throw runtime_error("Expected dec number after .skip.");
		}

		if (!regex_match(tokens->front(),
				Pass::tokenParsers[TokenType::OPERAND_DEC])) {
			throw runtime_error("Expected dec number after .skip");
		}

		int16_t conv = Pass::stringToInt(tokens->front());

		if (conv < 0 || conv > 0xffff) {
			throw runtime_error(
					"Number after skip can't be smaller than 0 or bigger than 32767");
		}

		Pass::incLocationCounter(conv);

		tokens->pop();

		Pass::logFile << "New LC = " << Pass::getLocationCounter() << endl;

		return;
	}

	if (dirName == ".align") {
		tokens->pop(); // pop align

		if (tokens->size() == 0) {
			throw runtime_error("Expected number after .align.");
		}

		if (!regex_match(tokens->front(),
				Pass::tokenParsers[TokenType::OPERAND_DEC])) {
			throw runtime_error("Expected dec number after .skip");
		}

		int16_t num = Pass::stringToInt(tokens->front());

		if (num < 0 || num > 0xffff) {
			throw runtime_error(
					"Number after skip can't be smaller than 0 or bigger then 32767");
		}

		int lcOld = Pass::getLocationCounter();
		//int num = Pass::stringToInt(tokens->front()); // number after align

		while (1) {
			if (lcOld % num == 0)
				break;
			else
				lcOld++;
		}

		Pass::setLocationCounter(lcOld);
		Pass::logFile << "New LC = " << Pass::getLocationCounter() << endl;

		tokens->pop();

		return;
	}

	if (dirName == ".equ") {
		tokens->pop(); // pop equ

		if (tokens->size() == 0) {
			throw runtime_error("Expected symbol name after .equ.");
		}

		string symbolName = tokens->front();
		tokens->pop();

		if (tokens->size() == 0) {
			throw runtime_error("Expected value after symbol name (.equ).");
		}

		string symbolValue = tokens->front();
		tokens->pop();

		if (Pass::checkIfSymbolExists(symbolName) != 0) {
			throw runtime_error(
					"Symbol " + symbolName
							+ " has been defined more than once.");
		}

		TokenType type = Pass::parseToken(symbolValue);
		uint16_t value = 0;

		if (type == TokenType::OPERAND_DEC) {
			value = Pass::stringToInt(symbolValue);
		} else if (type == TokenType::OPERAND_HEX) {

			if (symbolValue.size() > 10) {
				throw runtime_error(
						"Value in .equ directive was not correct. Expected 0xnnnnnnnn where n is hex char or number.");
			}

			value = Pass::hexStringToInt(symbolValue);
		} else {
			throw runtime_error(
					"Token after symbol in .equ has to be hex or dec operand.");
		}

		if (value > 0xffff) {
			throw runtime_error(
					"Value for symbol " + symbolName
							+ " has to be 16 bit value.");
		}

		Symbol * s = new Symbol(symbolName, number, "absolute", value);

		Pass::addSymbol(s);

		Pass::logFile << "Symbol " << symbolName << " added" << " (LC="
				<< Pass::getLocationCounter() << " value=" << value << ")"
				<< endl;

		return;
	}
}

Pass::State FirstPass::stateStart(State s, queue<string> * queue) {

	string token = queue->front();
	TokenType tokenType = Pass::parseToken(token);

	if (tokenType == LABEL) {
		string labelName = token.substr(0, token.size() - 1);

		Symbol * existingLabel = Pass::checkIfSymbolExists(labelName);
		Section * section = Pass::checkIfSectionExists(labelName);

		if (section != 0) {
			throw runtime_error(
					"Symbol can't have the same name as a section.");
		}

		if (existingLabel != 0) {
			throw runtime_error(
					"Label/symbol " + labelName
							+ " has been defined more than once.");
		}

		Symbol * s = new Symbol(labelName, number, currentSection,
				Pass::getLocationCounter());
		number++;
		Pass::addSymbol(s);
		Pass::logFile << "Symbol " << labelName << " added" << " (LC="
				<< Pass::getLocationCounter() << " section=" << currentSection
				<< ")" << endl;
		queue->pop();
	}

	return MID;
}

Pass::State FirstPass::stateMid(State s, queue<string> * tokenQueue) {

	if (tokenQueue->empty()) {
		return END;
	}

	string token = tokenQueue->front();
	TokenType tokenType = Pass::parseToken(token);

	switch (tokenType) {

	case SECTION: {

		tokenQueue->pop();

		if (currentSection != "unknown") {

			if (Pass::checkIfSectionExists(currentSection) != 0) {
				throw runtime_error(
						"Expected at most one section called " + currentSection
								+ ".");
			}

			Section * s = new Section(currentSection, number);
			s->setSize(Pass::getLocationCounter());

			while (accessQueue.size() > 0) {
				char ac = accessQueue.front();
				accessQueue.pop();
				if (ac == 'R') {
					s->setR();
				} else if (ac == 'W') {
					s->setW();
				} else if (ac == 'E') {
					s->setE();
				} else if (ac == 'P') {
					s->setP();
				} // ispravnost se gleda tokom parsiranja
			}

			Pass::addSection(s);
			number++;
			Pass::logFile << "Section " << currentSection << " created (LC="
					<< Pass::getLocationCounter() << ")" << endl;
			Pass::setLocationCounter(0);
		}

		if (token == ".section") {

			if (tokenQueue->size() == 0) {
				throw runtime_error(
						"Error while handling .section: Expected name of section after .section.");
			}

			string sectionName = tokenQueue->front();

			if (!regex_match(sectionName, regex("^([a-zA-Z_][a-zA-Z0-9]*)$"))) {
				throw runtime_error("Error while handling .section: Name of the section " + sectionName + " is not correct.");
			}

			if (sectionName == "text" || sectionName == "data"
					|| sectionName == "bss") {
				throw runtime_error(
						"Error while handling .section: " + sectionName
								+ " is not allowed after .section.");
			}

			token = ('.' + sectionName);
			tokenQueue->pop();

			if (tokenQueue->size() > 0) {
				string accessRights = tokenQueue->front();
				tokenQueue->pop();

				if (accessRights[0] == '"'
						&& accessRights[accessRights.size() - 1] == '"') {

					for (unsigned i = 1; i < accessRights.size() - 1; i++) {
						if (accessRights[i] == 'R' || accessRights[i] == 'W'
								|| accessRights[i] == 'E'
								|| accessRights[i] == 'P') {
							accessQueue.push(accessRights[i]);
						} else {
							throw runtime_error(
									"Error while handling access rights: R, W, E, P are only allowed.");
						}
					}

				} else {
					throw runtime_error(
							"Error while handling .section: Expected line .section sectionName [, \"FLAGS\"]");
				}
			} else {
				//prava pristupa je moraju da se navedu
				/*throw runtime_error(
				 "Error while handling .section: Access rights are expected after section name.");*/
			}
		}

		currentSection = token;

		if (Pass::checkIfSymbolExists(
				currentSection.substr(1, currentSection.size())) != 0) {
			throw runtime_error(
					"The section can't have the same name as a symbol.");
		}

		if (token != ".end") {
			Symbol * newSymbol = new Symbol(token.substr(1, token.size()),
					number++, token, 0);
			Pass::addSymbol(newSymbol);
		}

		Pass::logFile << "current section: " << currentSection << endl;

		return END_OF_LINE;
	}

	case GLOB_EXT: {
		handleGlobalExtern(tokenQueue);

		return END_OF_LINE;
	}

	case DIRECTIVE: {
		handleDirective(token, tokenQueue);

		return END_OF_LINE;
	}

	case INSTRUCTION: {
		queue<string> copy = *tokenQueue;
		int size = Pass::getInstructionSize(copy);
		Pass::incLocationCounter(size);

		while (!tokenQueue->empty())
			tokenQueue->pop();

		return END_OF_LINE;
	}

	default: {
		return END_OF_LINE;
	}

	}

	return END_OF_LINE;
}

void FirstPass::handleGlobalExtern(queue<string> * tokens) {
	string directive = tokens->front();
	tokens->pop();

	if (tokens->size() == 0) {
		throw runtime_error("Expected symbol name after .global.");
	}

	if (directive == ".global") {

		while (tokens->size() > 0) {
			string symbolName = tokens->front();

			if (Pass::parseToken(symbolName) == SYMBOL) {
				//do nothing
			} else {
				throw runtime_error("Expected symbol/s in .global directive.");
			}

			tokens->pop();
		}

		return;
	}

	if (directive == ".extern") {

		while (tokens->size() > 0) {
			string symbolName = tokens->front();

			if (Pass::parseToken(symbolName) == SYMBOL) {
				// do nothing
			} else {
				throw runtime_error("Expected symbol/s in .extern directive.");
			}

			tokens->pop();
		}

		return;
	}

}

