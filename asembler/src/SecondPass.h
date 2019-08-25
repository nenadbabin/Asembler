/*
 * SecondPass.h
 *
 *  Created on: Aug 6, 2019
 *      Author: etf
 */

#ifndef SECONDPASS_H_
#define SECONDPASS_H_

#include "Pass.h"
#include "Enums.h"

class SecondPass: public Pass {
public:
	SecondPass();
	virtual ~SecondPass();
	virtual void begin(string FileName) override;
	void writeObjectFile(ofstream& outputFile);

protected:
	virtual Pass::State next(Pass::State s, queue<string> * q) override;
private:
	static string currentSection;
	static int number;

	static void handleInstruction(queue<string> * tokens);
	static void handleDirective(string dirName, queue<string> * tokens);
	static void handleGlobalExtern(queue<string> * tokens);

	static void handleOperandOfInstruction(string firstOperand, string srcDst,
			int expectedSizeOfOperands);

	static void handlePCRel(string operand, int expectedSizeOfOperands,
			int instructionStart, int instructionSize);

	static State stateStart(State s, queue<string> * queue);
	static State stateMid(State s, queue<string> * queue);
	static State state(State s, queue<string> * queue);

	static string getSymbolFromBrackets(string s);
	static int getValFromBrackets(string s, int &n);
	static int getValAfterStar(string s);

	static u_int8_t getLowHigh(string s);
	static u_int8_t getRegistryNumber(string s);
};

#endif /* SECONDPASS_H_ */
