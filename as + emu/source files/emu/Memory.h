/*
 * Memory.h
 *
 *  Created on: Jul 23, 2019
 *      Author: etf
 */

#ifndef MEMORY_H_
#define MEMORY_H_

#include <cstdint>
#include <vector>
#include <mutex>
#include "AccessRights.h"

using namespace std;

class Memory {
public:
	Memory();
	virtual ~Memory();

	bool writeByte(uint16_t address, uint8_t data);
	bool writeWord(uint16_t address, uint16_t data);
	bool writeBlock(uint16_t start, uint16_t end,
			const vector<uint8_t> &data);

	bool readWord(uint16_t address, uint16_t &data);
	bool readByte(uint16_t address, uint8_t &data);

	void setAccessRights(vector<AccessRights *> ar) {
		accessRights = ar;
	}

	bool checkAccessRights(uint16_t add, AccessRights::AccessType type);

private:
	vector<uint8_t> memory;
	mutex memoryMutex;
	vector<AccessRights *> accessRights;
};

#endif /* MEMORY_H_ */
