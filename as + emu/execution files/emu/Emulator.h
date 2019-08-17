/*
 * Emulator.h
 *
 *  Created on: Jul 23, 2019
 *      Author: etf
 */

#ifndef EMULATOR_H_
#define EMULATOR_H_
#include <stdint.h>
#include <unordered_map>
#include "Instruction.h"
#include "Enums.h"
#include <functional>
#include "params.h"
#include <queue>
#include <mutex>

#include "Memory.h"

using namespace std;

class Emulator {
public:
	Emulator();
	virtual ~Emulator();

	void start();
	Memory * getMemory();
	void setPC(uint16_t startAddress);
	void incPC(int times = 1);

	static unordered_map<u_int8_t, AddressingType> addressingTypes;
	void setWriteToFile();

private:
	Memory * memory;
	Instruction * fetchInstruction();
	bool executeInstruction(Instruction *);
	static volatile bool processorON;

	static unordered_map<u_int8_t, u_int8_t> numberOfOperands;
	static unordered_map<u_int8_t, string> instructionNames;

	uint16_t getFirstOperand(Instruction * i);
	uint16_t getSecondOperand(Instruction * i);
	uint16_t readFromRegister(uint16_t number, bool word, bool high);
	void writeToRegister(uint16_t number, bool word, bool high, uint16_t data);

	bool haltExecutor(Instruction * i);
	bool xchgExecutor(Instruction * i);
	bool intExecutor(Instruction * i);
	bool movExecutor(Instruction * i);
	bool addExecutor(Instruction * i);
	bool subExecutor(Instruction * i);
	bool mulExecutor(Instruction * i);
	bool divExecutor(Instruction * i);
	bool cmpExecutor(Instruction * i);
	bool notExecutor(Instruction * i);
	bool andExecutor(Instruction * i);
	bool orExecutor(Instruction * i);
	bool xorExecutor(Instruction * i);
	bool testExecutor(Instruction * i);
	bool shlExecutor(Instruction * i);
	bool shrExecutor(Instruction * i);
	bool pushExecutor(Instruction * i);
	bool popExecutor(Instruction * i);
	bool jmpExecutor(Instruction * i);
	bool jeqExecutor(Instruction * i);
	bool jneExecutor(Instruction * i);
	bool jgtExecutor(Instruction * i);
	bool callExecutor(Instruction * i);
	bool retExecutor(Instruction * i);
	bool iretExecutor(Instruction * i);

	bool n();
	bool z();
	bool c();
	bool o();

	void n(bool);
	void z(bool);
	void c(bool);
	void o(bool);

	bool storeResult(Instruction * i, bool firstSecond, int32_t result);
	void updateZOCN(bool, bool, bool, bool, int32_t result);

	void printEmulatorState(ofstream &file, Instruction * i);

	queue<struct ChangedAddress> changedValuesInMemory;
	void pushToQueue(uint16_t a, bool w);

	static void timerInterrupt(Emulator * e);
	static void keyboardReader(Emulator * e);

	void pop(uint16_t &value);
	void push(uint16_t value);

	bool interrupt(uint8_t id);
	bool interruptSignals[8];
	bool notifyInterrupt(int i);
	void interruptHandler();

	recursive_mutex mutex;

	bool writeToFile;

	bool readByteFromMemory(uint16_t address, uint8_t &value, AccessRights::AccessType accessType = AccessRights::R);
	bool readWordFromMemory(uint16_t address, uint16_t &value, AccessRights::AccessType accessType = AccessRights::R);
	void callInterrupt(int number, int msgNumber);

};

#endif /* EMULATOR_H_ */
