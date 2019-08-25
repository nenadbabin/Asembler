/*
 * Memory.cpp
 *
 *  Created on: Jul 23, 2019
 *      Author: etf
 */

#include "Memory.h"
#include "params.h"
#include <iostream>
#include "params.h"

using namespace std;

Memory::Memory() :
		memory(MEMORY_SIZE) {
	for (auto &mem : memory) {
		mem = 0;
	}
}

Memory::~Memory() {
	// TODO Auto-generated destructor stub
}

bool Memory::writeByte(uint16_t address, uint8_t data) {
	memoryMutex.lock();
	memory[address] = data;
	memoryMutex.unlock();

	if (!checkAccessRights(address, AccessRights::W)) {
		return false;
	}

	return true;
}

bool Memory::writeWord(uint16_t address, uint16_t data) {
	if (address == MEMORY_SIZE - 1)
		return false;

	if (address == DATA_OUT) {
		char chr = data;
		if (data == 16) {
			chr = '\n';
		}
		cout << chr;
		cout.flush();
	}

	bool r1 = writeByte(address, (uint8_t) (data & 255u));
	bool r2 = writeByte(address + 1, (uint8_t) ((data >> 8u)));

	return r1 && r2;
}

bool Memory::readWord(uint16_t address, uint16_t &data) {
	if (address == MEMORY_SIZE - 1)
		return false;

	data = memory[address + 1];
	data <<= 8u;
	data |= memory[address];

	if (!checkAccessRights(address, AccessRights::R)) {
		return false;
	}

	if (!checkAccessRights(address + 1, AccessRights::R)) {
		return false;
	}

	return true;
}

bool Memory::readByte(uint16_t address, uint8_t &data) {
	if (address == MEMORY_SIZE - 1)
		return false;

	if (!checkAccessRights(address, AccessRights::R)) {
		return false;
	}

	data = memory[address];

	return true;
}

bool Memory::writeBlock(uint16_t start, uint16_t end,
		const vector<uint8_t> &data) {
	if (start > end)
		return false;
	for (uint16_t i = 0; i < end - start; i++) {
		memory[start + i] = data[i];
	}

	return true;
}

bool Memory::checkAccessRights(uint16_t add, AccessRights::AccessType type) {
	for (auto &accRights : accessRights) {
		if (!accRights->checkAccess(add, type)) {
			return false;
		}
	}
	return true;
}
