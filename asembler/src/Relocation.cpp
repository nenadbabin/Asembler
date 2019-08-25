/*
 * Relocation.cpp
 *
 *  Created on: Jul 1, 2019
 *      Author: etf
 */

#include "Relocation.h"

#include <iostream>
#include <iomanip>
using namespace std;

Relocation::Relocation(Symbol * s, RelocationType relT, int offset,
		Section * sec) {
	this->symbol = s;
	this->type = relT;
	this->offset = offset;
	this->section = sec;

}

Relocation::~Relocation() {
	// TODO Auto-generated destructor stub
}

ostream& operator<<(ostream& out, const Relocation& s) {
	out << "{" << s.symbol->getName() << " - " << s.symbol->getSection()
			<< " - " << s.offset << " - " << s.type << "}";
	return out;
}

string Relocation::printRelocation() {

	string types[6] = { "R_8", "R_PC8", "R_16", "R_PC16", "R_32", "R_PC32" };

	string relativeTo = "";

	if (symbol->getScope() == GLOBAL) {
		relativeTo = symbol->getName();
	} else {
		relativeTo = symbol->getSection();
	}

	stringstream out;

	out  << setw(15) << offset << setw(15) << section->getName()
			<< setw(15) << symbol->getName() << setw(15) << relativeTo
			<< setw(15) << types[type] << endl;

	return out.str();
}

