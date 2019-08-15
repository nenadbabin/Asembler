/*
 * Instruction.cpp
 *
 *  Created on: Jul 27, 2019
 *      Author: etf
 */

#include "Instruction.h"
#include "Emulator.h"

Instruction::Instruction(u_int8_t firstByte) { // @suppress("Class members should be properly initialized")
	opCode = firstByte >> 3;
	operandSize = (firstByte & 0x04) >> 2;
}

Instruction::~Instruction() {
	// TODO Auto-generated destructor stub
}

void Instruction::op1Descr(uint8_t opDescr) {
	u_int8_t am = opDescr >> 5;
	type1 = Emulator::addressingTypes[am];
	reg1 = (opDescr & 0x1e) >> 1;
	hl1 = (opDescr & 0x01);

}

void Instruction::imDiAdOp1(uint16_t word) {
	imDiAd1 = word;
}

void Instruction::op2Descr(uint8_t opDescr) {
	u_int8_t am = opDescr >> 5;
	type2 = Emulator::addressingTypes[am];
	reg2 = (opDescr & 0x1e) >> 1;
	hl2 = (opDescr & 0x01);
}

void Instruction::imDiAdOp2(uint16_t word) {
	imDiAd2 = word;
}
