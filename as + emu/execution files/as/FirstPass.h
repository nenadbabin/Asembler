/*
 * FirstPass.h
 *
 *  Created on: Aug 5, 2019
 *      Author: etf
 */

#ifndef FIRSTPASS_H_
#define FIRSTPASS_H_

#include "Pass.h"
#include "Enums.h"

class FirstPass: public Pass {
public:
	FirstPass();
	virtual ~FirstPass();
	virtual void begin(string FileName) override;

protected:
	virtual Pass::State next(Pass::State s, queue<string> * q) override;

private:
	static string currentSection;
	static int number;

	static void handleDirective(string dirName, queue<string> * tokens);
	static void handleGlobalExtern(queue<string> * tokens);

	static State stateStart(State s, queue<string> * queue);
	static State stateMid(State s, queue<string> * queue);

	static queue<char> accessQueue;
};

#endif /* FIRSTPASS_H_ */
