/*
 * Instruction.h
 *
 *  Created on: Jul 27, 2019
 *      Author: etf
 */

#ifndef INSTRUCTION_H_
#define INSTRUCTION_H_
#include <stdint.h>
#include <iostream>
#include "Enums.h"

using namespace std;

class Instruction {
public:
	Instruction(u_int8_t firstByte);
	virtual ~Instruction();

	void op1Descr(uint8_t);
	void imDiAdOp1(uint16_t);
	void op2Descr(uint8_t);
	void imDiAdOp2(uint16_t);

	bool isHl1() const {
		return hl1;
	}

	bool isHl2() const {
		return hl2;
	}

	u_int16_t getImDiAd1() const {
		return imDiAd1;
	}

	u_int16_t getImDiAd2() const {
		return imDiAd2;
	}

	u_int8_t getOpCode() const {
		return opCode;
	}

	bool getOperandSize() const {
		return operandSize;
	}

	u_int16_t getReg1() const {
		return reg1;
	}

	u_int16_t getReg2() const {
		return reg2;
	}

	AddressingType getType1() const {
		return type1;
	}

	AddressingType getType2() const {
		return type2;
	}

private:
	u_int8_t opCode;
	AddressingType type1;
	AddressingType type2;
	u_int16_t reg1;
	u_int16_t reg2;
	bool hl1;
	bool hl2;
	bool operandSize;

	u_int16_t imDiAd1;
	u_int16_t imDiAd2;

};

#endif /* INSTRUCTION_H_ */
