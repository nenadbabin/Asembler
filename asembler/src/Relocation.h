/*
 * Relocation.h
 *
 *  Created on: Jul 1, 2019
 *      Author: etf
 */

#ifndef FILES_RELOCATION_RELOCATION_H_
#define FILES_RELOCATION_RELOCATION_H_

#include "Symbol.h"
#include "Enums.h"
#include "Section.h"

class Relocation {
public:
	Relocation(Symbol * s, RelocationType relT, int offset, Section * sec);
	virtual ~Relocation();

	string printRelocation();

	friend ostream& operator<<(ostream& os, const Relocation& s);

private:
	Symbol * symbol;
	RelocationType type;
	int offset;
	Section * section;
};

#endif /* FILES_RELOCATION_RELOCATION_H_ */
