/*
 * Emulator.cpp
 *
 *  Created on: Jul 23, 2019
 *      Author: etf
 */

#include "Emulator.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <thread>
#include <termios.h>
#include <unistd.h>

using namespace std;

volatile bool Emulator::processorON = true;

struct ChangedAddress {
	uint16_t address;
	bool word;

	ChangedAddress(uint16_t a, bool w) {
		address = a;
		word = w;
	}
};

void Emulator::pushToQueue(uint16_t a, bool w) {
	struct ChangedAddress s(a, w);
	changedValuesInMemory.push(s);
}

Emulator::Emulator() {
	this->memory = new Memory();
	writeToRegister(SP_REGISTER, true, false, SP_START);
	writeToFile = false;

	for (int i = 0; i < 8; i++) {
		interruptSignals[i] = 0;
	}
}

Emulator::~Emulator() {
	// TODO Auto-generated destructor stub
}

Memory * Emulator::getMemory() {
	return memory;
}

void Emulator::start() {

	ofstream file("exec.log");
	thread timer(Emulator::timerInterrupt, this);
	thread keyboardThread(Emulator::keyboardReader, this);

	struct termios new_tio, old_tio;
	tcgetattr(STDIN_FILENO, &old_tio);
	new_tio = old_tio;
	new_tio.c_lflag &= (~ICANON & ~ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);

	cout << "EMU START!" << endl;
	while (processorON) {
		lock_guard<recursive_mutex> lck(mtx);
		Instruction * i = fetchInstruction();
		//cout << "CODE: " << dec << (unsigned)i->getOpCode() << endl;
		if (i != nullptr) {
			executeInstruction(i);
		}

		if (writeToFile && (i != nullptr)) {
			printEmulatorState(file, i);
		}

		interruptHandler();
	}

	timer.detach();
	keyboardThread.detach();
	file.close();
	tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);

	cout << "EMU END!" << endl;
}

void Emulator::setPC(uint16_t pc) {
	writeToRegister(PC_REGISTER, true, false, pc);
}

void Emulator::incPC(int times) {
	uint16_t currentPC = readFromRegister(PC_REGISTER, true, false);
	currentPC += times;
	setPC(currentPC);
}

bool Emulator::executeInstruction(Instruction * i) {
	u_int8_t opCode = i->getOpCode();

	switch (opCode) {
	case 1: {
		return haltExecutor(i);
	}
	case 2: {
		return xchgExecutor(i);
	}
	case 3: {
		return intExecutor(i);
	}
	case 4: {
		return movExecutor(i);
	}
	case 5: {
		return addExecutor(i);
	}
	case 6: {
		return subExecutor(i);
	}
	case 7: {
		return mulExecutor(i);
	}
	case 8: {
		return divExecutor(i);
	}
	case 9: {
		return cmpExecutor(i);
	}
	case 10: {
		return notExecutor(i);
	}
	case 11: {
		return andExecutor(i);
	}
	case 12: {
		return orExecutor(i);
	}
	case 13: {
		return xorExecutor(i);
	}
	case 14: {
		return testExecutor(i);
	}
	case 15: {
		return shlExecutor(i);
	}
	case 16: {
		return shrExecutor(i);
	}
	case 17: {
		return pushExecutor(i);
	}
	case 18: {
		return popExecutor(i);
	}
	case 19: {
		return jmpExecutor(i);
	}
	case 20: {
		return jeqExecutor(i);
	}
	case 21: {
		return jneExecutor(i);
	}
	case 22: {
		return jgtExecutor(i);
	}
	case 23: {
		return callExecutor(i);
	}
	case 24: {
		return retExecutor(i);
	}
	case 25: {
		return iretExecutor(i);
	}
	default: {
		callInterrupt(1, 2);
		return false;
	}
	}

	return false;
}

Instruction * Emulator::fetchInstruction() {
	uint16_t currentPC = readFromRegister(PC_REGISTER, true, false);
	u_int8_t instruction;
	//memory->readByte(currentPC, instruction);
	if (!readByteFromMemory(currentPC, instruction, AccessRights::E)) {
		callInterrupt(1, 1);
		return nullptr;
	}
	incPC();
	currentPC++;
	u_int8_t opCode(0);
	u_int8_t operandSize(0);
	opCode = instruction >> 3;
	operandSize = (instruction & 0x04) >> 2;
	u_int8_t numOfOperands = numberOfOperands[opCode];

	Instruction * i = new Instruction(instruction);

	u_int8_t tmp = 1;
	while (tmp <= numOfOperands) {
		u_int8_t opDescr;
		//memory->readByte(currentPC, opDescr);
		if (!readByteFromMemory(currentPC, opDescr, AccessRights::E)) {
			callInterrupt(1, 1);
			return nullptr;
		}
		incPC();
		currentPC++;

		if (tmp == 1) {
			i->op1Descr(opDescr);
		} else if (tmp == 2) {
			i->op2Descr(opDescr);
		}

		u_int8_t am = opDescr >> 5;
		AddressingType addressing = addressingTypes[am];

		switch (addressing) {
		case AddressingType::IMMED: {
			if (operandSize == 0) {
				u_int8_t imDiAd(0);
				//memory->readByte(currentPC, imDiAd);
				if (!readByteFromMemory(currentPC, imDiAd, AccessRights::E)) {
					callInterrupt(1, 1);
					return nullptr;
				}
				incPC();
				currentPC++;
				if (tmp == 1) {
					i->imDiAdOp1(imDiAd);
				} else if (tmp == 2) {
					i->imDiAdOp2(imDiAd);
				}
			} else {
				u_int16_t imDiAd(0);
				//memory->readWord(currentPC, imDiAd);
				if (!readWordFromMemory(currentPC, imDiAd, AccessRights::E)) {
					callInterrupt(1, 1);
					return nullptr;
				}
				incPC(2);
				currentPC += 2;
				if (tmp == 1) {
					i->imDiAdOp1(imDiAd);
				} else if (tmp == 2) {
					i->imDiAdOp2(imDiAd);
				}
			}
			break;
		}
		case AddressingType::REG_DIR:
		case AddressingType::REG_IND: {
			break;
		}
		case AddressingType::REG_IND8: {
			u_int8_t imDiAd(0);
			//memory->readByte(currentPC, imDiAd);
			if (!readByteFromMemory(currentPC, imDiAd, AccessRights::E)) {
				callInterrupt(1, 1);
				return nullptr;
			}
			incPC();
			currentPC++;
			if (tmp == 1) {
				i->imDiAdOp1(imDiAd);
			} else if (tmp == 2) {
				i->imDiAdOp2(imDiAd);
			}
			break;
		}
		case AddressingType::REG_IND16:
		case AddressingType::MEMDIR: {
			u_int16_t imDiAd(0);
			//memory->readWord(currentPC, imDiAd);
			if (!readWordFromMemory(currentPC, imDiAd, AccessRights::E)) {
				callInterrupt(1, 1);
				return nullptr;
			}
			incPC(2);
			currentPC += 2;
			if (tmp == 1) {
				i->imDiAdOp1(imDiAd);
			} else if (tmp == 2) {
				i->imDiAdOp2(imDiAd);
			}
			break;
		}
		}

		tmp++;
	}

	return i;

}

unordered_map<u_int8_t, u_int8_t> Emulator::numberOfOperands = { { 1, 0 }, { 2,
		2 }, { 3, 1 }, { 4, 2 }, { 5, 2 }, { 6, 2 }, { 7, 2 }, { 8, 2 },
		{ 9, 2 }, { 10, 1 }, { 11, 2 }, { 12, 2 }, { 13, 2 }, { 14, 2 },
		{ 15, 2 }, { 16, 2 }, { 17, 1 }, { 18, 1 }, { 19, 1 }, { 20, 1 }, { 21,
				1 }, { 22, 1 }, { 23, 1 }, { 24, 0 }, { 25, 0 } };

unordered_map<u_int8_t, AddressingType> Emulator::addressingTypes = { { 0,
		AddressingType::IMMED }, { 1, AddressingType::REG_DIR }, { 2,
		AddressingType::REG_IND }, { 3, AddressingType::REG_IND8 }, { 4,
		AddressingType::REG_IND16 }, { 5, AddressingType::MEMDIR } };

uint16_t Emulator::getFirstOperand(Instruction * i) {
	AddressingType type = i->getType1();

	//cout << "TYPE: " << type << endl;
	switch (type) {
	case AddressingType::IMMED: {
		return i->getImDiAd1();
	}
	case AddressingType::REG_DIR: {
		uint16_t number = i->getReg1();
		bool word = false;
		bool high = false;

		if (i->getOperandSize() == false) { // 1 byte operand
			high = i->isHl1();
		} else { // 2 bytes operand
			word = true;
		}

		return readFromRegister(number, word, high);
		break;
	}
	case AddressingType::REG_IND: {
		uint16_t number = i->getReg1();
		bool word = false;
		bool high = false;

		if (i->getOperandSize() == false) { // 1 byte operand
			high = i->isHl1();
		} else { // 2 bytes operand
			word = true;
		}

		uint16_t regValue = readFromRegister(number, word, high);

		if (i->getOperandSize() == false) {
			uint8_t fromMemory(0);
			memory->readByte(regValue, fromMemory);
			return fromMemory;
		} else {
			uint16_t fromMemory(0);
			memory->readWord(regValue, fromMemory);
			return fromMemory;
		}

		break;
	}
	case AddressingType::REG_IND8:
	case AddressingType::REG_IND16: {

		uint16_t number = i->getReg1();
		bool word = false;
		bool high = false;

		if (i->getOperandSize() == false) { // 1 byte operand
			high = i->isHl1();
		} else { // 2 bytes operand
			word = true;
		}

		uint16_t regValue = readFromRegister(number, word, high);

		if (i->getOperandSize() == false) {
			uint8_t fromMemory(0);
			memory->readByte((uint8_t) regValue + (int8_t) i->getImDiAd1(),
					fromMemory);
			return fromMemory;
		} else {
			uint16_t fromMemory(0);
			memory->readWord((uint16_t) regValue + (int16_t) i->getImDiAd1(),
					fromMemory);
			return fromMemory;
		}
		break;
	}
	case AddressingType::MEMDIR: {

		if (i->getOperandSize() == false) {
			uint8_t fromMemory(0);
			memory->readByte(i->getImDiAd1(), fromMemory);
			return fromMemory;
		} else {
			uint16_t fromMemory(0);
			memory->readWord(i->getImDiAd1(), fromMemory);
			return fromMemory;
		}
		break;
	}
	}

	return 0;
}

uint16_t Emulator::getSecondOperand(Instruction * i) {
	AddressingType type = i->getType2();

	switch (type) {
	case AddressingType::IMMED: {
		return i->getImDiAd2();
	}
	case AddressingType::REG_DIR: {
		uint16_t number = i->getReg2();
		bool word = false;
		bool high = false;

		if (i->getOperandSize() == false) { // 1 byte operand
			high = i->isHl2();
		} else { // 2 bytes operand
			word = true;
		}

		//cout << hex << (unsigned) readFromRegister(number, word, high) << endl;
		return readFromRegister(number, word, high);
		break;
	}
	case AddressingType::REG_IND: {
		uint16_t number = i->getReg2();
		bool word = false;
		bool high = false;

		if (i->getOperandSize() == false) { // 1 byte operand
			high = i->isHl2();
		} else { // 2 bytes operand
			word = true;
		}

		uint16_t regValue = readFromRegister(number, word, high);

		if (i->getOperandSize() == false) {
			uint8_t fromMemory(0);
			memory->readByte(regValue, fromMemory);
			return fromMemory;
		} else {
			uint16_t fromMemory(0);
			memory->readWord(regValue, fromMemory);
			return fromMemory;
		}

		break;
	}
	case AddressingType::REG_IND8:
	case AddressingType::REG_IND16: {

		uint16_t number = i->getReg2();
		bool word = false;
		bool high = false;

		if (i->getOperandSize() == false) { // 1 byte operand
			high = i->isHl2();
		} else { // 2 bytes operand
			word = true;
		}

		uint16_t regValue = readFromRegister(number, word, high);

		if (i->getOperandSize() == false) {
			uint8_t fromMemory(0);
			memory->readByte((uint8_t) regValue + (int8_t) i->getImDiAd2(),
					fromMemory);
			return fromMemory;
		} else {
			uint16_t fromMemory(0);
			memory->readWord((uint16_t) regValue + (int16_t) i->getImDiAd2(),
					fromMemory);
			return fromMemory;
		}
		break;
	}
	case AddressingType::MEMDIR: {

		if (i->getOperandSize() == false) {
			uint8_t fromMemory(0);
			memory->readByte(i->getImDiAd2(), fromMemory);
			return fromMemory;
		} else {
			uint16_t fromMemory(0);
			memory->readWord(i->getImDiAd2(), fromMemory);
			return fromMemory;
		}
		break;
	}
	}

	return 0;
}

uint16_t Emulator::readFromRegister(uint16_t number, bool word, bool high) {
	uint16_t value16(0);
	uint8_t value8(0);

	if (word) {
		memory->readWord(REGISTERS_START + number * 2, value16);
		return value16;
	} else {
		if (high) {
			memory->readByte(REGISTERS_START + number * 2 + 1, value8);
			return (uint16_t) value8;
		} else {
			memory->readByte(REGISTERS_START + number * 2, value8);
			return (uint16_t) value8;
		}
	}
}

void Emulator::writeToRegister(uint16_t number, bool word, bool high,
		uint16_t data) {

	if (word) {
		memory->writeWord(REGISTERS_START + number * 2, data);
	} else {
		if (high) {
			memory->writeByte(REGISTERS_START + number * 2 + 1, data);
		} else {
			memory->writeByte(REGISTERS_START + number * 2, data);
		}
	}
}

bool Emulator::storeResult(Instruction * i, bool second, int32_t result) {
	AddressingType type;

	if (second) {
		type = i->getType2();
	} else {
		type = i->getType1();
	}

	switch (type) {
	case AddressingType::REG_DIR: {
		uint16_t number;

		if (second) {
			number = i->getReg2();
		} else {
			number = i->getReg1();
		}

		bool word = false;
		bool high = false;

		if (i->getOperandSize() == false) { // 1 byte operand
			if (second) {
				high = i->isHl2();
			} else {
				high = i->isHl1();
			}
		} else { // 2 bytes operand
			word = true;
		}
		writeToRegister(number, word, high, result);
		return true;
		break;
	}
	case AddressingType::REG_IND: {
		uint16_t number;

		if (second) {
			number = i->getReg2();
		} else {
			number = i->getReg1();
		}

		bool word = false;
		bool high = false;

		if (i->getOperandSize() == false) { // 1 byte operand
			if (second) {
				high = i->isHl2();
			} else {
				high = i->isHl1();
			}
		} else { // 2 bytes operand
			word = true;
		}

		uint16_t address = readFromRegister(number, word, high);
		pushToQueue(address, word);

		if (i->getOperandSize() == false) {
			return memory->writeByte(address, (uint8_t) result);
		} else {
			return memory->writeWord(address, result);
		}
		break;
	}
	case AddressingType::REG_IND8:
	case AddressingType::REG_IND16: {
		uint16_t number;

		if (second) {
			number = i->getReg2();
		} else {
			number = i->getReg1();
		}

		bool word = false;
		bool high = false;

		if (i->getOperandSize() == false) { // 1 byte operand
			if (second) {
				high = i->isHl2();
			} else {
				high = i->isHl1();
			}
		} else { // 2 bytes operand
			word = true;
		}

		uint16_t regValue = readFromRegister(number, word, high);

		//cout << "REG VALUE: " << hex << (unsigned)regValue << endl;

		if (i->getOperandSize() == false) {
			if (second) {
				if (memory->writeByte(
						(uint8_t) regValue + (int8_t) i->getImDiAd2(),
						result)) {
					pushToQueue((uint8_t) regValue + (int8_t) i->getImDiAd2(),
							word);
					return true;
				} else {
					return false;
				}
			} else {
				if (memory->writeByte(
						(uint8_t) regValue + (int8_t) i->getImDiAd1(),
						result)) {
					pushToQueue((uint8_t) regValue + (int8_t) i->getImDiAd1(),
							word);
					return true;
				} else {
					return false;
				}
				//cout << "ImDiAd1: " << hex << (unsigned)i->getImDiAd1() << endl;
			}
		} else {
			if (second) {
				if (memory->writeWord(
						(uint16_t) regValue + (int16_t) i->getImDiAd2(),
						result)) {
					pushToQueue(regValue + (int16_t) i->getImDiAd2(), word);
					return true;
				} else {
					return false;
				}
			} else {
				if (memory->writeWord(
						(uint16_t) regValue + (int16_t) i->getImDiAd1(),
						result)) {
					pushToQueue((uint16_t) regValue + (int16_t) i->getImDiAd1(),
							word);
					return true;
				} else {
					return false;
				}
			}
		}
		break;
	}
	case AddressingType::MEMDIR: {
		if (i->getOperandSize() == false) {
			if (second) {
				if (memory->writeByte(i->getImDiAd2(), result)) {
					pushToQueue(i->getImDiAd2(), false);
					return true;
				} else {
					return false;
				}
			} else {
				if (memory->writeByte(i->getImDiAd1(), result)) {
					pushToQueue(i->getImDiAd1(), false);
					return true;
				} else {
					return false;
				}
			}
		} else {
			if (second) {
				if (memory->writeWord(i->getImDiAd2(), result)) {
					pushToQueue(i->getImDiAd2(), true);
					return true;
				} else {
					return false;
				}
			} else {
				if (memory->writeWord(i->getImDiAd1(), result)) {
					pushToQueue(i->getImDiAd1(), true);
					return true;
				} else {
					return false;
				}
			}
		}
		break;
	}
	case AddressingType::IMMED: {
		return false;
	}
	}

	return true;
}

void Emulator::updateZOCN(bool isZ, bool isO, bool isC, bool isN,
		int32_t result) {

	int16_t realResult = (uint16_t) result;

	if (isZ) {
		z(realResult == 0);
	}
	if (isO) {
		o(result > realResult);
	}
	if (isC) {
		c(result > realResult);
	}
	if (isN) {
		n(realResult < 0);
	}
}

bool Emulator::haltExecutor(Instruction * instr) {
	processorON = false;
	return true;
}

bool Emulator::xchgExecutor(Instruction * instr) {

	if (instr->getType1() == AddressingType::IMMED
			|| instr->getType2() == AddressingType::IMMED) {
		callInterrupt(1, 2);
		return false;
	}

	int16_t first = getFirstOperand(instr);
	int16_t second = getSecondOperand(instr);
	int16_t tmp = first;
	first = second;
	second = tmp;
	if (!storeResult(instr, false, first)) {
		callInterrupt(1, 1);
		return false;
	}
	if (!storeResult(instr, true, second)) {
		callInterrupt(1, 1);
		return false;
	}
	return false;
}

bool Emulator::intExecutor(Instruction * instr) {

	if (instr->getType1() == AddressingType::IMMED) {
		callInterrupt(1, 2);
		return false;
	}

	uint16_t dst = getFirstOperand(instr);

	if (dst > 0x0007) {
		callInterrupt(1, 2);
		return false;
	}

	notifyInterrupt(dst);
	return true;
}

bool Emulator::movExecutor(Instruction * instr) {

	if (instr->getType1() == AddressingType::IMMED) {
		callInterrupt(1, 2);
		return false;
	}

	int16_t toMove = getSecondOperand(instr);
	if (!storeResult(instr, false, toMove)) {
		callInterrupt(1, 1);
		return false;
	}

	updateZOCN(true, false, false, true, toMove);

	return true;
}

bool Emulator::addExecutor(Instruction * instr) {

	if (instr->getType1() == AddressingType::IMMED) {
		callInterrupt(1, 2);
		return false;
	}

	int16_t dst = getFirstOperand(instr);
	int16_t src = getSecondOperand(instr);
	int32_t result = dst + src;
	if (!storeResult(instr, false, result)) {
		callInterrupt(1, 1);
		return false;
	}

	updateZOCN(true, true, true, true, result);

	return true;
}

bool Emulator::subExecutor(Instruction * instr) {

	if (instr->getType1() == AddressingType::IMMED) {
		callInterrupt(1, 2);
		return false;
	}

	int16_t dst = getFirstOperand(instr);
	int16_t src = getSecondOperand(instr);
	int32_t result = dst - src;
	if (!storeResult(instr, false, result)) {
		callInterrupt(1, 1);
		return false;
	}

	updateZOCN(true, true, true, true, result);

	return true;
}

bool Emulator::mulExecutor(Instruction * instr) {

	if (instr->getType1() == AddressingType::IMMED) {
		callInterrupt(1, 2);
		return false;
	}

	int16_t dst = getFirstOperand(instr);
	int16_t src = getSecondOperand(instr);
	int32_t result = dst * src;
	if (!storeResult(instr, false, result)) {
		callInterrupt(1, 1);
		return false;
	}

	updateZOCN(true, false, false, true, result);

	return true;
}

bool Emulator::divExecutor(Instruction * instr) {

	if (instr->getType1() == AddressingType::IMMED) {
		callInterrupt(1, 2);
		return false;
	}

	int16_t dst = getFirstOperand(instr);
	int16_t src = getSecondOperand(instr);

	if (src == 0) {
		callInterrupt(1, 2);
		return false;
	}
	int32_t result = dst / src;
	if (!storeResult(instr, false, result)) {
		callInterrupt(1, 1);
		return false;
	}

	updateZOCN(true, false, false, true, result);

	return true;
}

bool Emulator::cmpExecutor(Instruction * instr) {

	if (instr->getType1() == AddressingType::IMMED) {
		callInterrupt(1, 2);
		return false;
	}

	int16_t dst = getFirstOperand(instr);
	int16_t src = getSecondOperand(instr);
	int32_t result = dst - src;

	updateZOCN(true, true, true, true, result);

	return true;
}

bool Emulator::notExecutor(Instruction * instr) {

	if (instr->getType1() == AddressingType::IMMED) {
		callInterrupt(1, 2);
		return false;
	}

	int16_t dst = getFirstOperand(instr);
	int32_t result = ~dst;
	if (!storeResult(instr, false, result)) {
		callInterrupt(1, 1);
		return false;
	}

	updateZOCN(true, false, false, true, result);

	return true;
}

bool Emulator::andExecutor(Instruction * instr) {

	if (instr->getType1() == AddressingType::IMMED) {
		callInterrupt(1, 2);
		return false;
	}

	int16_t dst = getFirstOperand(instr);
	int16_t src = getSecondOperand(instr);
	int32_t result = dst & src;
	if (!storeResult(instr, false, result)) {
		callInterrupt(1, 1);
		return false;
	}

	updateZOCN(true, false, false, true, result);

	return true;
}

bool Emulator::orExecutor(Instruction * instr) {

	if (instr->getType1() == AddressingType::IMMED) {
		callInterrupt(1, 2);
		return false;
	}

	int16_t dst = getFirstOperand(instr);
	int16_t src = getSecondOperand(instr);
	int32_t result = dst | src;
	if (!storeResult(instr, false, result)) {
		callInterrupt(1, 1);
		return false;
	}

	updateZOCN(true, false, false, true, result);

	return true;
}

bool Emulator::xorExecutor(Instruction * instr) {

	if (instr->getType1() == AddressingType::IMMED) {
		callInterrupt(1, 2);
		return false;
	}

	int16_t dst = getFirstOperand(instr);
	int16_t src = getSecondOperand(instr);
	int32_t result = dst ^ src;
	if (!storeResult(instr, false, result)) {
		callInterrupt(1, 1);
		return false;
	}

	updateZOCN(true, false, false, true, result);

	return true;
}

bool Emulator::testExecutor(Instruction * instr) {

	if (instr->getType1() == AddressingType::IMMED) {
		callInterrupt(1, 2);
		return false;
	}

	int16_t dst = getFirstOperand(instr);
	int16_t src = getSecondOperand(instr);
	int32_t result = dst & src;

	updateZOCN(true, false, false, true, result);

	return true;
}

bool Emulator::shlExecutor(Instruction * instr) {

	if (instr->getType1() == AddressingType::IMMED) {
		callInterrupt(1, 2);
		return false;
	}

	int16_t dst = getFirstOperand(instr);
	int16_t src = getSecondOperand(instr);
	int32_t result = dst << src;
	if (!storeResult(instr, false, result)) {
		callInterrupt(1, 1);
		return false;
	}

	updateZOCN(true, false, true, true, result);

	return true;
}

bool Emulator::shrExecutor(Instruction * instr) {

	if (instr->getType1() == AddressingType::IMMED) {
		callInterrupt(1, 2);
		return false;
	}

	int16_t dst = getFirstOperand(instr);
	int16_t src = getSecondOperand(instr);
	int32_t result = dst >> src;
	if (!storeResult(instr, false, result)) {
		callInterrupt(1, 1);
		return false;
	}

	updateZOCN(true, false, true, true, result);

	return true;
}

bool Emulator::pushExecutor(Instruction * instr) {
	uint16_t currentSP = readFromRegister(SP_REGISTER, true, false);
	currentSP -= 2;
	writeToRegister(SP_REGISTER, true, false, currentSP);
	uint16_t value = getFirstOperand(instr);
	memory->writeWord(currentSP, value);

	return true;
}

bool Emulator::popExecutor(Instruction * instr) {

	if (instr->getType1() == AddressingType::IMMED) {
		callInterrupt(1, 2);
		return false;
	}

	uint16_t currentSP = readFromRegister(SP_REGISTER, true, false);
	uint16_t value;
	memory->readWord(currentSP, value);
	if (!storeResult(instr, false, value)) {
		callInterrupt(1, 1);
		return false;
	}
	currentSP += 2;
	writeToRegister(SP_REGISTER, true, false, currentSP);

	return true;
}

bool Emulator::jmpExecutor(Instruction * instr) {
	uint16_t dst = getFirstOperand(instr);
	//cout << dec << (unsigned)instr->getImDiAd1() << endl;
	writeToRegister(PC_REGISTER, true, false, dst);

	return true;
}

bool Emulator::jeqExecutor(Instruction * instr) {
	if (z()) {
		jmpExecutor(instr);
	}

	return true;
}

bool Emulator::jneExecutor(Instruction * instr) {
	if (!z()) {
		jmpExecutor(instr);
	}

	return true;
}

bool Emulator::jgtExecutor(Instruction * instr) {
	if (!n() && !z()) {
		jmpExecutor(instr);
	}
	return true;
}

bool Emulator::callExecutor(Instruction * instr) {

	if (instr->getType1() == AddressingType::IMMED) {
		callInterrupt(1, 2);
		return false;
	}

	uint16_t currentSP = readFromRegister(SP_REGISTER, true, false);
	currentSP -= 2;
	writeToRegister(SP_REGISTER, true, false, currentSP);
	u_int16_t value = readFromRegister(PC_REGISTER, true, false);
	memory->writeWord(currentSP, value);
	//cout << dec<< (unsigned)instr->gge() << endl;
	uint16_t newPC = getFirstOperand(instr);
	//cout << "CALL: " << dec << (unsigned)newPC << endl;
	writeToRegister(PC_REGISTER, true, false, newPC);

	return true;
}

bool Emulator::retExecutor(Instruction * instr) {
	uint16_t currentSP = readFromRegister(SP_REGISTER, true, false);
	uint16_t value;
	memory->readWord(currentSP, value);
	writeToRegister(PC_REGISTER, true, false, value);
	currentSP += 2;
	writeToRegister(SP_REGISTER, true, false, currentSP);

	//cout << "RET: " << dec << (unsigned)value << endl;

	return true;
}

bool Emulator::iretExecutor(Instruction * instr) {
	uint16_t currentSP = readFromRegister(SP_REGISTER, true, false);
	uint16_t value;
	memory->readWord(currentSP, value);
	writeToRegister(PSW_REGISTER, true, false, value);
	currentSP += 2;
	writeToRegister(SP_REGISTER, true, false, currentSP);

	retExecutor(instr);
	return true;
}

bool Emulator::n() {
	return readFromRegister(PSW_REGISTER, true, false) & 8;
}

bool Emulator::z() {
	return readFromRegister(PSW_REGISTER, true, false) & 1;
}
bool Emulator::c() {
	return readFromRegister(PSW_REGISTER, true, false) & 4;
}

bool Emulator::o() {
	return readFromRegister(PSW_REGISTER, true, false) & 2;
}

void Emulator::n(bool value) {

	u_int16_t currentState = readFromRegister(PSW_REGISTER, true, false);

	if (value) {
		writeToRegister(PSW_REGISTER, true, false, currentState | 8);
	} else {
		writeToRegister(PSW_REGISTER, true, false, currentState & ~8);
	}
}

void Emulator::z(bool value) {
	u_int16_t currentState = readFromRegister(PSW_REGISTER, true, false);

	if (value) {
		writeToRegister(PSW_REGISTER, true, false, currentState | 1);
	} else {
		writeToRegister(PSW_REGISTER, true, false, currentState & ~1);
	}
}

void Emulator::c(bool value) {
	u_int16_t currentState = readFromRegister(PSW_REGISTER, true, false);

	if (value) {
		writeToRegister(PSW_REGISTER, true, false, currentState | 4);
	} else {
		writeToRegister(PSW_REGISTER, true, false, currentState & ~4);
	}
}

void Emulator::o(bool value) {
	u_int16_t currentState = readFromRegister(PSW_REGISTER, true, false);

	if (value) {
		writeToRegister(PSW_REGISTER, true, false, currentState | 2);
	} else {
		writeToRegister(PSW_REGISTER, true, false, currentState & ~2);
	}
}

void Emulator::printEmulatorState(ofstream &file, Instruction * i) {
	file << "Instruction: " << instructionNames[i->getOpCode()] << endl;
	file << "%Registers%" << endl;
	for (int i = 0; i <= 7; i++) {
		file << "r" << i << " " << setfill('0') << setw(4) << hex
				<< (unsigned) readFromRegister(i, true, false) << endl;
	}

	file << "psw" << " " << setfill('0') << setw(4) << hex
			<< (unsigned) readFromRegister(15, true, false) << endl;

	file << endl;
	file << "%Stack%" << endl;
	u_int16_t start = 0xff00;
	u_int16_t currentSP = readFromRegister(SP_REGISTER, true, false);
	bool somethingOnStack = false;

	if (currentSP > start) {
		file << "Stack is not in regular condition." << endl;
	} else {
		for (uint16_t i = start; i > currentSP;) {
			i -= 2;
			uint8_t value;
			memory->readByte(i + 1, value);
			file << setfill('0') << setw(2) << hex << (unsigned) value << endl;
			memory->readByte(i, value);
			file << setfill('0') << setw(2) << hex << (unsigned) value << endl;
			somethingOnStack = true;
		}

		if (!somethingOnStack) {
			file << "There's nothing on stack." << endl;
		}
	}
	file << endl;
	file << "%Changed values in memory%" << endl;

	if (changedValuesInMemory.size() == 0) {
		file << "There were no changes in memory." << endl;
	}

	while (changedValuesInMemory.size() > 0) {
		struct ChangedAddress s = changedValuesInMemory.front();
		changedValuesInMemory.pop();
		uint16_t address = s.address;
		bool word = s.word;

		if (word) {
			uint16_t value;
			memory->readWord(address, value);
			file << "MEM16[" << setfill('0') << setw(4) << hex
					<< (unsigned) address << "] = " << setfill('0') << setw(4)
					<< hex << (unsigned) value << endl;
		} else {
			uint8_t value;
			memory->readByte(address, value);
			file << "MEM8[" << setfill('0') << setw(4) << hex
					<< (unsigned) address << "] = " << setfill('0') << setw(2)
					<< hex << (unsigned) value << endl;
		}
	}

	file << endl;
}

unordered_map<u_int8_t, string> Emulator::instructionNames = { { 1, "halt" }, {
		2, "xchg" }, { 3, "int" }, { 4, "mov" }, { 5, "add" }, { 6, "sub" }, {
		7, "mul" }, { 8, "div" }, { 9, "cmp" }, { 10, "not" }, { 11, "and" }, {
		12, "or" }, { 13, "xor" }, { 14, "test" }, { 15, "shl" }, { 16, "shr" },
		{ 17, "push" }, { 18, "pop" }, { 19, "jmp" }, { 20, "jeq" },
		{ 21, "jne" }, { 22, "jgt" }, { 23, "call" }, { 24, "ret" }, { 25,
				"iret" } };

void Emulator::timerInterrupt(Emulator * e) {

	while (processorON) {

		uint16_t PSW = e->readFromRegister(PSW_REGISTER, true, false);

		//if ((PSW & ((uint16_t) 1 << 13)) == 0) { // mask = 00100...
			u_int16_t value;
			e->getMemory()->readWord(TIMER_CFG, value);
			int milis = 0;
			switch (value) {
			case 0: {
				milis = 500;
				break;
			}
			case 1: {
				milis = 1000;
				break;
			}
			case 2: {
				milis = 1500;
				break;
			}
			case 3: {
				milis = 2000;
				break;
			}
			case 4: {
				milis = 5000;
				break;
			}
			case 5: {
				milis = 10000;
				break;
			}
			case 6: {
				milis = 30000;
				break;
			}
			case 7: {
				milis = 60000;
				break;
			}
			}

			this_thread::sleep_for(chrono::milliseconds(milis));
			e->notifyInterrupt(2);
		//}
	}
}

void Emulator::pop(uint16_t &value) {
	uint16_t currentSP = readFromRegister(SP_REGISTER, true, false);
	memory->readWord(currentSP, value);
	currentSP += 2;
	writeToRegister(SP_REGISTER, true, false, currentSP);
}

void Emulator::push(uint16_t value) {
	uint16_t currentSP = readFromRegister(SP_REGISTER, true, false);
	currentSP -= 2;
	writeToRegister(SP_REGISTER, true, false, currentSP);
	memory->writeWord(currentSP, value);
}

bool Emulator::interrupt(uint8_t id) {
	if (id > 15)
		return false;

	uint16_t PC = readFromRegister(PC_REGISTER, true, false);
	uint16_t PSW = readFromRegister(PSW_REGISTER, true, false);

	if ((PSW & ((uint16_t) 1 << 15)) != 0) {
		return true;
	}

	push(PC);
	push(PSW);
	PSW &= (1 << 15);
	writeToRegister(PSW_REGISTER, true, false, PSW);
	uint16_t addressToJmp;
	memory->readWord(id * 2, addressToJmp);

	if (addressToJmp == 0)
		return false;

	//cout << "To jmp: " << dec << (unsigned)addressToJmp << endl;
	writeToRegister(PC_REGISTER, true, false, addressToJmp);

	return true;
}

bool Emulator::notifyInterrupt(int i) {
	lock_guard<recursive_mutex> lck(mtx);
	if (i > 15)
		return false;
	bool old = interruptSignals[i];
	interruptSignals[i] = true;

	if (old) {
		return false;
	} else {
		return true;
	}
}

void Emulator::interruptHandler() {
	lock_guard<recursive_mutex> lck(mtx);
	uint16_t PSW = readFromRegister(PSW_REGISTER, true, false);

	if ((PSW & ((uint16_t) 1 << 15)) == 0) {
		for (int i = 0; i < 8; i++) {
			if (interruptSignals[i]) {
				if ((i == 2 && (PSW & ((uint16_t) 1 << 14)) == 0)
						|| (i == 3 && (PSW & ((uint16_t) 1 << 13)) == 0)
						|| (i != 2 && i != 3)) {
					if (interrupt(i)) {
						interruptSignals[i] = false;
						/*if (i == 1) {
						 cout << "TIMER!" << endl;
						 }*/
						break;
					}
				}
			}
		}
	}
}

void Emulator::keyboardReader(Emulator * e) {
	while (processorON) {
		//uint16_t PSW = e->readFromRegister(PSW_REGISTER, true, false);

		//if ((PSW & ((uint16_t) 1 << 14)) == 0) { // mask = 01000...
			int val = getchar();
			if (val == EOF)
				break;

			e->mtx.lock();
			while (!e->notifyInterrupt(3)) {
				e->mtx.unlock();
				this_thread::sleep_for(chrono::milliseconds(100));
				e->mtx.lock();
			}

			e->getMemory()->writeWord(DATA_IN, val);
			e->notifyInterrupt(3);
			e->mtx.unlock();
			this_thread::sleep_for(chrono::milliseconds(100));
			//cout << "READER!" << endl;
		//}
	}
}

void Emulator::setWriteToFile() {
	writeToFile = true;
}

bool Emulator::readByteFromMemory(uint16_t address, uint8_t &value,
		AccessRights::AccessType accessType) {

	if (accessType == AccessRights::E) {
		if (!memory->checkAccessRights(address, AccessRights::E)) {
			return false;
		}
	}

	if (!memory->readByte(address, value)) {
		return false;
	}

	return true;
}

bool Emulator::readWordFromMemory(uint16_t address, uint16_t &value,
		AccessRights::AccessType accessType) {

	if (accessType == AccessRights::E) {
		if (!memory->checkAccessRights(address, AccessRights::E)) {
			return false;
		}
	}

	if (!memory->readWord(address, value)) {
		return false;
	}

	return true;
}

void Emulator::callInterrupt(int number, int msgNumber) {
	push(msgNumber);
	notifyInterrupt(number);
}
