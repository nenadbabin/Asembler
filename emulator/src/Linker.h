/*
 * Linker.h
 *
 *  Created on: Jul 22, 2019
 *      Author: etf
 */

#ifndef LINKER_H_
#define LINKER_H_

#include "ObjectFile.h"
#include <vector>
#include <unordered_map>
#include <stdint.h>
#include "AccessRights.h"

class Linker {
public:
	Linker(vector<ObjectFile *> files);
	virtual ~Linker();

	unordered_map<string, vector<u_int8_t>> link(
			unordered_map<string, u_int16_t> &, vector<AccessRights *> &);

private:
	vector<ObjectFile *> files;
	void checkSymbols(unordered_map<string, Symbol *> &gs);
	uint16_t getAddressForLoading(
			unordered_map<string, u_int16_t> userInputSections,
			unordered_map<string, vector<Section *>> sectionObjects);
	int checkOverlapping(unordered_map<string, vector<Section *>> sectionPairs);
	bool createAccessRights(vector<AccessRights *> &, unordered_map<string, vector<Section *>> sectionPairs);

};

#endif /* LINKER_H_ */
