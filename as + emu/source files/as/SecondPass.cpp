/*
 * SecondPass.cpp
 *
 *  Created on: Aug 6, 2019
 *      Author: etf
 */

#include "SecondPass.h"
#include <string>
#include <queue>
#include <fstream>
#include <stdlib.h>
#include <iomanip>

using namespace std;

int SecondPass::number = 0;
string SecondPass::currentSection = "unknown";

SecondPass::SecondPass() {
	locationCounter = 0;

}

SecondPass::~SecondPass() {
	// TODO Auto-generated destructor stub
}

void SecondPass::begin(string fileName) {
	ifstream inputFile(fileName);
	getTokens(inputFile);

	queue<string> queue;

	int i = 1;
	for (auto &tokens : tokenArray) {

		for (auto &token : tokens) {
			//cout << token << endl;
			queue.push(token);
		}

		Pass::State s = Pass::START;

		while ((s = this->next(s, &queue)) != Pass::END) {
			//logFile << "Current LC = " << locationCounter << endl;
		}

		logFile << "Line " << i << " parsed" << endl;
		i++;
	}

	logFile << "------RELOCATIONS (after second pass):------" << endl;
	for (auto i = relocations.begin(); i != relocations.end(); ++i)
		logFile << *i << endl;

}

Pass::State SecondPass::next(Pass::State s, queue<string> * queue) {
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

Pass::State SecondPass::stateStart(State s, queue<string> * queue) {

	string token = queue->front();
	TokenType tokenType = Pass::parseToken(token);

	if (tokenType == LABEL) {
		queue->pop();

	}

	return MID;
}

Pass::State SecondPass::stateMid(State s, queue<string> * tokenQueue) {

	if (tokenQueue->empty()) {
		return END;
	}

	string token = tokenQueue->front();
	TokenType tokenType = Pass::parseToken(token);

	switch (tokenType) {

	case SECTION: {

		if (token == ".section") {
			tokenQueue->pop(); // pops .section
			token = ('.' + tokenQueue->front());
			tokenQueue->pop(); // pops sectionName
		}

		currentSection = token;
		Pass::setLocationCounter(0);
		if (!tokenQueue->empty()) {
			tokenQueue->pop(); // pops "FLAGS" or .text, .bss etc.
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
		handleInstruction(tokenQueue);

		return END_OF_LINE;
	}

	default: {
		return END_OF_LINE;
	}

	}

	return END_OF_LINE;

}

void SecondPass::handleDirective(string dirName, queue<string> * tokens) {

	if (dirName == ".skip") {
		tokens->pop();

		if (tokens->size() == 0) {
			throw runtime_error("Expected number after .skip.");
		}

		int num = Pass::stringToInt(tokens->front());
		//int lcOld = Assembly::getLocationCounter();

		Section * sectionObj = Pass::checkIfSectionExists(currentSection);

		if (sectionObj != 0) {
			//vec je inicijalizovano nulama

		} else {
			throw runtime_error("Expected .skip to be in some section.");
		}

		Pass::incLocationCounter(num);

		tokens->pop();

		Pass::logFile << "New LC = " << Pass::getLocationCounter() << endl;

		return;
	}

	if (dirName == ".align") {
		tokens->pop(); // pop align

		if (tokens->size() == 0) {
			throw runtime_error("Expected number after .align.");
		}

		while (tokens->size() > 0) {

			int lcOld = Pass::getLocationCounter();
			int lcNew = lcOld;
			int num = Pass::stringToInt(tokens->front()); // number after align

			while (1) {
				if (lcNew % num == 0)
					break;
				else
					lcNew++;
			}

			Pass::logFile << "New LC = " << Pass::getLocationCounter() << endl;

			Section * sectionObj = Pass::checkIfSectionExists(currentSection);

			if (sectionObj != 0) {
				// write zeros, but it's already zeros

			} else {
				throw runtime_error("Expected .align to be in some section.");
			}

			Pass::setLocationCounter(lcNew);

			tokens->pop();
		}

		return;
	}

	if (dirName == ".word") {
		tokens->pop();

		if (tokens->size() == 0) {
			Pass::incLocationCounter(2);
		}

		while (tokens->size() > 0) {

			if (regex_match(tokens->front(), Pass::tokenParsers[SYMBOL])) {
				Section * s = Pass::checkIfSectionExists(currentSection);
				Symbol * symbolObject = Pass::checkIfSymbolExists(
						tokens->front());

				if (symbolObject->getSection() == "absolute") { //immed
					// no need for reloaction
					u_int16_t imDiAd(symbolObject->getValue());
					s->writeTo(&imDiAd, Pass::getLocationCounter(), 2);
					Pass::incLocationCounter(2);

				} else { // mem dir; section exists
					Relocation * relocationObject = new Relocation(symbolObject,
							RelocationType::R_16, Pass::getLocationCounter(),
							s);
					Pass::addRelocation(relocationObject);

					u_int16_t imDiAd;
					if (symbolObject->getScope() == ScopeType::GLOBAL) {
						imDiAd = 0;
					} else {
						imDiAd = symbolObject->getValue();
					}
					s->writeTo(&imDiAd, Pass::getLocationCounter(), 2);
					Pass::incLocationCounter(2);
				}

			} else if (regex_match(tokens->front(),
					Pass::tokenParsers[OPERAND_DEC])
					|| regex_match(tokens->front(),
							Pass::tokenParsers[OPERAND_HEX])) {
				int num = Pass::convertStringToInt(tokens->front(), 0,
						0x0000ffff);

				void * binVal = new u_int16_t(num);
				int numBytes = 2;
				int oldLC = Pass::getLocationCounter();

				Section * sectionObj = Pass::checkIfSectionExists(
						currentSection);

				if (sectionObj != 0) {
					sectionObj->writeTo(binVal, oldLC, numBytes);

				} else {
					throw runtime_error(
							"Expected .word to be in .data section.");
				}

				Pass::incLocationCounter(2);
			} else {
				throw runtime_error(
						"Expected numer/numbers of symbols after .word");
			}

			tokens->pop();

			Pass::logFile << "New LC = " << Pass::getLocationCounter() << endl;
		}

		return;
	}

	if (dirName == ".byte") {
		tokens->pop();

		if (tokens->size() == 0) {
			Pass::incLocationCounter(1);
		}

		while (tokens->size() > 0) {
			int num = Pass::convertStringToInt(tokens->front(), 0, 0x00ff);

			void * binVal = new u_int8_t(num);
			int numBytes = 1;
			int oldLC = Pass::getLocationCounter();

			Section * sectionObj = Pass::checkIfSectionExists(currentSection);

			if (sectionObj != 0) {
				sectionObj->writeTo(binVal, oldLC, numBytes);

			} else {
				throw runtime_error("Expected .byte to be in .data section.");
			}

			Pass::incLocationCounter(1);

			tokens->pop();

			Pass::logFile << "New LC = " << Pass::getLocationCounter() << endl;
		}

		return;
	}

	if (dirName == ".equ") {
		while (tokens->size() > 0)
			tokens->pop();

		return;
	}

}

void SecondPass::handleGlobalExtern(queue<string> * tokens) {

	string directive = tokens->front();
	tokens->pop();

	if (directive == ".global") {

		while (tokens->size() > 0) {

			string symbolName = tokens->front();

			Symbol * existingSymbol = Pass::checkIfSymbolExists(symbolName);

			if (existingSymbol != 0) {

				existingSymbol->setScope(ScopeType::GLOBAL);

			} else {
				throw runtime_error("Symbol declared global was not defined.");
			}

			tokens->pop();
		}

		return;

	}

	if (directive == ".extern") {

		while (tokens->size() > 0) {

			string symbolName = tokens->front();

			Symbol * existingSymbol = Pass::checkIfSymbolExists(symbolName);

			if (existingSymbol != 0) {

				throw runtime_error(
						"Symbol " + symbolName
								+ " was declared extern, but it was defined");

			} else {
				Symbol * newS = new Symbol(symbolName, number);
				number++;
				newS->setScope(ScopeType::GLOBAL);
				Pass::addSymbol(newS);

				Pass::logFile << "Symbol " << symbolName
						<< " is created as extern (but with no section and value)."
						<< endl;
			}

			tokens->pop();
		}

		return;
	}

}

void SecondPass::handleInstruction(queue<string> * tokens) {
	/*queue<string> copy = *tokens;
	 int size = Assembly::getInstructionSize(copy);
	 Assembly::incLocationCounter(size);

	 while (!tokens->empty())
	 tokens->pop();*/
	string currentInstruction = tokens->front();

	Section * s = Pass::checkIfSectionExists(currentSection);

	//it can't be moved
	if (s == 0 || s->getE() == 0) {
		throw runtime_error(
				"Error while handling " + currentInstruction
						+ ": Instruction has to be in section that has execution access (E).");
	}

	string instruction = tokens->front();
	string instrCopy = instruction;

	if (regex_search(instruction,
			regex(
					"^(halt|xchg|int|mov|add|sub|mul|cmp|not|and|or|xor|test|shl|shr|push|pop|jmp|jeq|jne|jgt|call|ret|iret)(b|w)$"))) {

		instrCopy = instruction.substr(0, instruction.size() - 1);
	}

	int expectedSizeOfOperands = 2;

	if (regex_search(instruction,
			regex(
					"^(halt|xchg|int|mov|add|sub|mul|cmp|not|and|or|xor|test|shl|shr|push|pop|jmp|jeq|jne|jgt|call|ret|iret)(b)$"))) {

		expectedSizeOfOperands = 1;
	}

	queue<string> copy = *tokens;
	int instructionSize = Pass::getInstructionSize(copy);
	int numberOfOperands = Pass::numberOfOperands[instrCopy];
	tokens->pop();

	u_int8_t opcode = Pass::opCodes[instrCopy];

	int startAddressForWriting = Pass::getLocationCounter();

	u_int8_t instrByte(0);
	instrByte = instrByte | (opcode << 3);

	if (expectedSizeOfOperands == 2) {
		instrByte = instrByte | ((u_int8_t) 1 << 2); // two bytes opperands
	} // else do nothing

	s->writeTo(&instrByte, Pass::getLocationCounter(), 1);
	Pass::incLocationCounter(1);

	if (numberOfOperands == 2) {

		string firstOperand = tokens->front();
		//cout << firstOperand << endl;
		tokens->pop();
		string secondOperand = tokens->front();
		tokens->pop();

		// ne znam sta ce ovo ovde, mozda zatreba
		string firstOperandType = Pass::getOperandType(firstOperand);
		string secondOperandType = Pass::getOperandType(secondOperand);

		if (firstOperandType == "symbPCRel") {
			handlePCRel(firstOperand, expectedSizeOfOperands,
					startAddressForWriting, instructionSize);
		} else {
			handleOperandOfInstruction(firstOperand, "dst",
					expectedSizeOfOperands);
		}

		if (secondOperandType == "symbPCRel") {
			handlePCRel(secondOperand, expectedSizeOfOperands,
					startAddressForWriting, instructionSize);
		} else {
			handleOperandOfInstruction(secondOperand, "src",
					expectedSizeOfOperands);
		}

	} else if (numberOfOperands == 1) {
		string firstOperand = tokens->front();
		string firstOperandType = Pass::getOperandType(firstOperand);
		tokens->pop();

		if (instrCopy == "push" || instrCopy == "jmp" || instrCopy == "jne"
				|| instrCopy == "jeq" || instrCopy == "jgt") {
			if (firstOperandType == "symbPCRel") {
				handlePCRel(firstOperand, expectedSizeOfOperands,
						startAddressForWriting, instructionSize);
			} else {
				handleOperandOfInstruction(firstOperand, "src",
						expectedSizeOfOperands);
			}
		} else {
			if (firstOperandType == "symbPCRel") {
				handlePCRel(firstOperand, expectedSizeOfOperands,
						startAddressForWriting, instructionSize);
			} else {
				handleOperandOfInstruction(firstOperand, "dst",
						expectedSizeOfOperands);
			}
		}
	} else if (numberOfOperands == 0) {
		// no operands to write
	} else {
		throw runtime_error(
				"Unexpected error occured. Number of operands was not 0, 1 or 2.");
	}

	/*while (!tokens->empty())
	 tokens->pop();*/

	return;
}

void SecondPass::handleOperandOfInstruction(string firstOperand, string srcDst,
		int expectedSizeOfOperands) {

	string firstOperandType = Pass::getOperandType(firstOperand);
	Section * s = Pass::checkIfSectionExists(currentSection);

	if (srcDst == "dst"
			&& (firstOperandType == "immedHex" || firstOperandType == "immedDec"
					|| firstOperandType == "symbImmAdd")) {
		throw runtime_error(
				"Dst operand in instruction can't be immediate or absolute value.");
	}

	if (regex_search(firstOperand, regex("^(pc|sp|psw)"))) {
		firstOperand = regex_replace(firstOperand, regex("^(pc)"), "r7");
		firstOperand = regex_replace(firstOperand, regex("^(sp)"), "r6");
		firstOperand = regex_replace(firstOperand, regex("^(psw)"), "r15");
	}

	u_int8_t firstOpByte(0);

	if (firstOperandType == "regDir") {
		u_int8_t regNumber(getRegistryNumber(firstOperand));

		u_int8_t hl = getLowHigh(firstOperand);

		firstOpByte = firstOpByte | ((u_int8_t) 1 << 5); // adressing type
		firstOpByte = firstOpByte | (regNumber << 1); // register
		firstOpByte = firstOpByte | hl; //high or low, if nothing, than low

		if (regex_search(firstOperand,
				regex("^((r([0-9]{1}))|(sp|pc|psw))([h,l])$"))) { // it has l or h
			if (expectedSizeOfOperands == 2) {
				throw runtime_error(
						"Expected size of operand " + firstOperand
								+ " was 2B. Either don't use sufix h/l or change mnemonic to {instrName}b.");
			}
		}

		//register has to have sufix h/l if operandSize is 2
		if (regex_search(firstOperand,
				regex("^((r([0-9]{1}))|(sp|pc|psw))$"))) { // it has l or h
			if (expectedSizeOfOperands == 1) {
				throw runtime_error(
						"Expected size of operand " + firstOperand
								+ " was 1B. Either use sufix h/l or remove sufix b from mnemonic.");
			}
		}

		s->writeTo(&firstOpByte, Pass::getLocationCounter(), 1);
		Pass::incLocationCounter(1);

		//cout << hex << (unsigned) firstOpByte << endl;
	} else if (firstOperandType == "regIndOffDec"
			|| firstOperandType == "regIndOffHex") {
		u_int8_t regNumber(getRegistryNumber(firstOperand));
		//cout << dec << regNumber << endl;
		u_int8_t hl = getLowHigh(firstOperand);

		firstOpByte = firstOpByte | (regNumber << 1); // register
		firstOpByte = firstOpByte | hl; //high or low, if nothing, than low

		int n = -1;
		int val = getValFromBrackets(firstOperand, n);

		bool skip = false;

		if (val == 0) {
			firstOpByte = firstOpByte | ((u_int8_t) 2 << 5); //regInd no offset, no imm/..
			s->writeTo(&firstOpByte, Pass::getLocationCounter(), 1);
			Pass::incLocationCounter(1);

			skip = true;
		}

		if (skip == false) {
			if (n == 1) {
				firstOpByte = firstOpByte | ((u_int8_t) 3 << 5); // addressing type = 8 bit offset
				//cout << hex << (unsigned) firstOpByte << endl;
				s->writeTo(&firstOpByte, Pass::getLocationCounter(), 1);
				Pass::incLocationCounter(1);

				u_int8_t imDiAd(val);
				s->writeTo(&imDiAd, Pass::getLocationCounter(), 1);
				Pass::incLocationCounter(1);

				//write val to file
			} else { // n == 2
				firstOpByte = firstOpByte | ((u_int8_t) 4 << 5); // addressing type = 16 bit offset
				//cout << hex << (unsigned) firstOpByte << endl;
				s->writeTo(&firstOpByte, Pass::getLocationCounter(), 1);
				Pass::incLocationCounter(1);

				u_int16_t imDiAd(val);
				s->writeTo(&imDiAd, Pass::getLocationCounter(), 2);
				Pass::incLocationCounter(2);
				//write val to file

			}
		}

	} else if (firstOperandType == "regIndOffSymb") {
		u_int8_t regNumber(getRegistryNumber(firstOperand));
		u_int8_t hl = getLowHigh(firstOperand);

		firstOpByte = firstOpByte | ((u_int8_t) 4 << 5); // register
		firstOpByte = firstOpByte | (regNumber << 1); // register
		firstOpByte = firstOpByte | hl; //high or low, if nothing, than low

		s->writeTo(&firstOpByte, Pass::getLocationCounter(), 1);
		Pass::incLocationCounter(1);

		string symbolName = getSymbolFromBrackets(firstOperand);

		Symbol * symbolObject = Pass::checkIfSymbolExists(symbolName);

		if (symbolObject == 0) {
			throw runtime_error("Expected " + symbolName + " to be declared.");
		}

		if (symbolObject->getSection() == "absolute") { //immed
			// no need for reloaction
			u_int16_t imDiAd(symbolObject->getValue());
			s->writeTo(&imDiAd, Pass::getLocationCounter(), 2);
			Pass::incLocationCounter(2);

		} else { // section exists or undefined
			Relocation * relocationObject = new Relocation(symbolObject,
					RelocationType::R_16, Pass::getLocationCounter(), s);
			Pass::addRelocation(relocationObject);

			u_int16_t imDiAd;
			if (symbolObject->getScope() == ScopeType::GLOBAL) {
				imDiAd = 0;
			} else {
				imDiAd = symbolObject->getValue();
			}
			s->writeTo(&imDiAd, Pass::getLocationCounter(), 2);
			Pass::incLocationCounter(2);
		}

	} else if (firstOperandType == "memDirDec"
			|| firstOperandType == "memDirHex") {

		firstOpByte = firstOpByte | ((u_int8_t) 5 << 5); // addressing type
		s->writeTo(&firstOpByte, Pass::getLocationCounter(), 1);
		Pass::incLocationCounter(1);

		int val = getValAfterStar(firstOperand);

		u_int16_t binVal(val);

		s->writeTo(&binVal, Pass::getLocationCounter(), 2);
		Pass::incLocationCounter(2);

	} else if (firstOperandType == "immedDec"
			|| firstOperandType == "immedHex") {

		firstOpByte = firstOpByte | ((u_int8_t) 0 << 5); // addressing type
		s->writeTo(&firstOpByte, Pass::getLocationCounter(), 1);
		Pass::incLocationCounter(1);

		int immedValue = -1;
		if (expectedSizeOfOperands == 1) {
			immedValue = Pass::convertStringToInt(firstOperand, 0, 0x00ff);
			uint16_t imDiAd(immedValue);
			s->writeTo(&imDiAd, Pass::getLocationCounter(), 1);
			Pass::incLocationCounter(1);
		} else {
			immedValue = Pass::convertStringToInt(firstOperand, 0, 0xffff);
			u_int16_t imDiAd(immedValue);
			s->writeTo(&imDiAd, Pass::getLocationCounter(), 2);
			Pass::incLocationCounter(2);
		}

	} else if (firstOperandType == "symbAbs") {

		if (expectedSizeOfOperands == 1) {
			throw runtime_error(
					"Values of symbols are 2B and can't be part of instruction that has 1B operands.");
		}

		Symbol * symbolObject = Pass::checkIfSymbolExists(firstOperand);

		if (symbolObject == 0) {
			throw runtime_error(
					"Expected " + firstOperand + " to be declared.");
		}

		if (srcDst == "dst" && symbolObject->getSection() == "absolute") {
			throw runtime_error(
					"Absolute symbol " + firstOperand
							+ " can't be destination operand.");
		}

		if (symbolObject->getSection() == "absolute") { //immed
			firstOpByte = firstOpByte | ((u_int8_t) 0 << 5); // addressing type
			s->writeTo(&firstOpByte, Pass::getLocationCounter(), 1);
			Pass::incLocationCounter(1);

			// no need for reloaction

			u_int16_t imDiAd(symbolObject->getValue());
			s->writeTo(&imDiAd, Pass::getLocationCounter(), 2);
			Pass::incLocationCounter(2);

			//instrByte = instrByte | ((u_int8_t) 1 << 2);
			//s->writeTo(&instrByte, startAddressForWriting, 1);

		} else if (symbolObject->getSection() != "undefined") { // mem dir; section exists
			firstOpByte = firstOpByte | ((u_int8_t) 5 << 5); // addressing type
			s->writeTo(&firstOpByte, Pass::getLocationCounter(), 1);
			Pass::incLocationCounter(1);

			Relocation * relocationObject = new Relocation(symbolObject,
					RelocationType::R_16, Pass::getLocationCounter(), s);
			Pass::addRelocation(relocationObject);

			u_int16_t imDiAd;
			if (symbolObject->getScope() == ScopeType::GLOBAL) {
				imDiAd = 0;
			} else {
				imDiAd = symbolObject->getValue();
			}
			s->writeTo(&imDiAd, Pass::getLocationCounter(), 2);
			Pass::incLocationCounter(2);

		} else if (symbolObject->getSection() == "undefined") { // undefined symbol; same as immed; NEEEE! mem dir, pa linker da prepravi
			firstOpByte = firstOpByte | ((u_int8_t) 5 << 5); // addressing type
			s->writeTo(&firstOpByte, Pass::getLocationCounter(), 1);
			Pass::incLocationCounter(1);

			Relocation * relocationObject = new Relocation(symbolObject,
					RelocationType::R_16, Pass::getLocationCounter(), s);
			Pass::addRelocation(relocationObject);

			u_int16_t imDiAd(symbolObject->getValue()); //symbolObject->getValue() = 0
			s->writeTo(&imDiAd, Pass::getLocationCounter(), 2);
			Pass::incLocationCounter(2);

			//instrByte = instrByte | ((u_int8_t) 1 << 2);
			//s->writeTo(&instrByte, startAddressForWriting, 1);
		}

	} else if (firstOperandType == "symbImmAdd") {

		if (expectedSizeOfOperands == 1) {
			throw runtime_error(
					"Address of symbol is 2B and can't be part of instruction that has 1B operands.");
		}

		firstOperand = firstOperand.substr(1, firstOperand.size());
		Symbol * symbolObject = Pass::checkIfSymbolExists(firstOperand);

		if (symbolObject == 0) {
			throw runtime_error(
					"Expected " + firstOperand + " to be declared.");
		}

		if (symbolObject->getSection() == "absolute") {
			throw runtime_error(
					"Can't get memory address of symbol " + firstOperand
							+ " defined with equ.");
		} else { // not equ or undefined
			firstOpByte = firstOpByte | ((u_int8_t) 0 << 5); // addressing type
			s->writeTo(&firstOpByte, Pass::getLocationCounter(), 1);
			Pass::incLocationCounter(1);

			Relocation * relocationObject = new Relocation(symbolObject,
					RelocationType::R_16, Pass::getLocationCounter(), s);
			Pass::addRelocation(relocationObject);

			u_int16_t imDiAd;
			if (symbolObject->getScope() == ScopeType::GLOBAL) {
				imDiAd = 0;
			} else {
				imDiAd = symbolObject->getValue();
			}
			s->writeTo(&imDiAd, Pass::getLocationCounter(), 2);
			Pass::incLocationCounter(2);

			//instrByte = instrByte | ((u_int8_t) 1 << 2);
			//s->writeTo(&instrByte, startAddressForWriting, 1);
		}

	} else {
		throw runtime_error(
				"Fatal error during second pass: Unknown type of operand.");
	}
}

void SecondPass::handlePCRel(string firstOperand, int expectedSizeOfOperands,
		int instructionStart, int instructionSize) {

	Section * s = Pass::checkIfSectionExists(currentSection);

	firstOperand = firstOperand.substr(1, firstOperand.size());

	u_int8_t firstOpByte(0);
//cout << hex << (unsigned) instrByte << endl;

	firstOpByte = firstOpByte | ((u_int8_t) 4 << 5);	// red ind 16 bit offset
	firstOpByte = firstOpByte | ((u_int8_t) 7 << 1);		// register r7 = pc
	s->writeTo(&firstOpByte, Pass::getLocationCounter(), 1);
	Pass::incLocationCounter(1);

	Symbol * symbolObject = Pass::checkIfSymbolExists(firstOperand);

	if (symbolObject == 0) {
		throw runtime_error("Expected " + firstOperand + " to be declared.");
	}

	if (symbolObject->getSection() == "absolute") {
		throw runtime_error(
				"PC rel can't be used for " + firstOperand
						+ " that is absolute.");
	}

	if (symbolObject->getSection() != "undefined") {

		// ako je u okviru iste sekcije
		if (s->getName() == symbolObject->getSection()) {
			// u okviru iste sekcije: vrednostSimbola - PC
			// nema zapisa o rekolaciji
			u_int16_t imDiAd = symbolObject->getValue()
					- (instructionStart + instructionSize);
			s->writeTo(&imDiAd, Pass::getLocationCounter(), 2);
			Pass::incLocationCounter(2);

		} else /*if (s->getName() != "undefined")*/{ // definisan, ali nije u istoj sekciji
			Relocation * relocationObject = new Relocation(symbolObject,
					RelocationType::R_PC16, Pass::getLocationCounter(), s);
			Pass::addRelocation(relocationObject);

			// ugradjuje se pomeraj do sledece istrukcije + vrednost od pocetka sekcije
			u_int16_t imDiAd = Pass::getLocationCounter()
					- (instructionStart + instructionSize);
			if (symbolObject->getScope() == ScopeType::GLOBAL) {
				imDiAd += 0;
			} else {
				imDiAd += symbolObject->getValue();
			}

			s->writeTo(&imDiAd, Pass::getLocationCounter(), 2);
			Pass::incLocationCounter(2);
		}

	} else {
		Relocation * relocationObject = new Relocation(symbolObject,
				RelocationType::R_PC16, Pass::getLocationCounter(), s);
		Pass::addRelocation(relocationObject);

		// ugradjuje se pomeraj do sledece istrukcije
		u_int16_t imDiAd(Pass::getLocationCounter() // == 0
		- (instructionStart + instructionSize));
		s->writeTo(&imDiAd, Pass::getLocationCounter(), 2);
		Pass::incLocationCounter(2);
	}
}

u_int8_t SecondPass::getRegistryNumber(string s) {
	u_int8_t result(0);

	if (s.length() >= 5) { //r1[10], r2[0], r15[0]

		s = s.substr(0, s.find("["));
	}

	if (s.length() == 4) { //r12h, r15l

		result += (s[1] - '0') * 10;
		result += (s[2] - '0');

		if (result > 7 && result < 15) {
			throw runtime_error("Expected registers r0..r7 or r15.");
		}

		return result;

	}

	if (s.length() == 3) {

		if (isdigit(s[s.size() - 1]) == 0) { // r1h, r1l, ...
			result += (s[1] - '0');

			if (result > 7 && result < 15) {
				throw runtime_error("Expected registers r0..r7 or r15.");
			}

			return result;
		} else { //r10, r12, ...
			result += (s[1] - '0') * 10;
			result += (s[2] - '0');

			if (result > 7 && result < 15) {
				throw runtime_error("Expected registers r0..r7 or r15.");
			}

			return result;
		}

	}

	if (s.length() == 2) {
		result += s[1] - '0';

		if (result > 7 && result < 15) {
			throw runtime_error("Expected registers r0..r7 or r15.");
		}
	}

	return result;
}

u_int8_t SecondPass::getLowHigh(string s) {
	int size = s.size();

	s = s.substr(0, s.find("["));

	if (s[size - 1] == 'h') {
		return 0x01;
	} else {
		return 0x00;
	}
}

int SecondPass::getValFromBrackets(string s, int &n) {
	vector<string> v = Pass::splitSeparator(s, "[");
	vector<string> v1 = Pass::splitSeparator(v[1], "]");
	string final = v1[0];

	if (regex_search(final, Pass::tokenParsers[OPERAND_DEC])) {
		int num = Pass::stringToInt(final);

		if (num > 0x00ff) {
			if (num > 0x0000ffff) {
				throw runtime_error("Value too big.");
			} else {
				n = 2;
				return num;
			}
		} else {
			n = 1;
			return num;
		}
	} else {
		int num = Pass::hexStringToInt(final);

		if (num > 0x00ff) {
			if (num > 0x0000ffff) {
				throw runtime_error("Value too big.");
			} else {
				n = 2;
				return num;
			}
		} else {
			n = 1;
			return num;
		}
	}
}

string SecondPass::getSymbolFromBrackets(string s) {
	vector<string> v = Pass::splitSeparator(s, "[");
	vector<string> v1 = Pass::splitSeparator(v[1], "]");
	string final = v1[0];

	return final;
}

int SecondPass::getValAfterStar(string s) {
	vector<string> v = Pass::splitSeparator(s, "*");
	string final = v[0];

	if (regex_search(final, Pass::tokenParsers[OPERAND_DEC])) {
		int num = Pass::stringToInt(final);

		if (num > 0x00ff) {
			if (num > 0x0000ffff) {
				throw runtime_error(
						"Value after star was too big. Address doesn't exit.");
			} else {
				return num;
			}
		} else {
			return num;
		}
	} else {
		int num = Pass::hexStringToInt(final);

		if (num > 0x00ff) {
			if (num > 0x0000ffff) {
				throw runtime_error("Value too big.");
			} else {
				return num;
			}
		} else {
			return num;
		}
	}
}

void SecondPass::writeObjectFile(ofstream& outputFile) {

	outputFile << "%SECTIONS%" << endl;
	outputFile << setw(15) << "NAME" << setw(15) << "SIZE" << setw(15)
			<< "NUMBER" << setw(10) << "R" << setw(3) << "W" << setw(3) << "E"
			<< setw(3) << "P" << endl;
	for (auto &section : sections) {
		outputFile << section.printSection();
	}
	outputFile << "%END%" << endl << endl;

	outputFile << "%SYMBOLS%" << endl;
	outputFile << setw(15) << "NAME" << setw(15) << "SECTION" << setw(15)
			<< "VALUE" << setw(15) << "SCOPE" << setw(15) << "NUMBER" << endl;
	for (auto &symbol : symbols) {
		outputFile << symbol.printSymbol();
	}
	outputFile << "%END%" << endl << endl;

	outputFile << "%RELOCATIONS%" << endl;
	outputFile << setw(15) << "OFFSET" << setw(15) << "SECTION" << setw(15)
			<< "SYMBOL" << setw(15) << "RELATIVE_TO" << setw(15) << "TYPE"
			<< endl;
	for (auto &rel : relocations) {
		outputFile << rel.printRelocation();
	}
	outputFile << "%END%" << endl << endl;

	outputFile << "%SECTIONS_CONTENT%" << endl;
	for (auto &section : sections) {
		if (section.getP() == 1) {
			outputFile << section.printContent();
		}
	}
	outputFile << "%END%" << endl;

}
