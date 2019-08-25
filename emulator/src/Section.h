/*
 * Section.h
 *
 *  Created on: Jun 27, 2019
 *      Author: etf
 */

#ifndef FILES_SECTION_SECTION_H_
#define FILES_SECTION_SECTION_H_

#include <string>
#include <vector>
#include "Symbol.h"
using namespace std;

class Section {
public:
	Section(string name, int number);
	Section(string name, int size, int number);
	virtual ~Section();

	const string& getName() const;
	void setName(const string& name);
	int getNumber() const;
	void setNumber(int number);
	int getSize() const;
	void setSize(int size);
	u_int8_t * getContent();

	void writeTo(void *src, int pos, size_t length);
	void writeZerosTo(int pos, size_t length);

	void createContent();
	string printContent();
	string printSection();

	friend ostream& operator<<(ostream& os, const Section& s);

	void setP();
	void setE();
	void setW();
	void setR();

	u_int8_t getP();
	u_int8_t getE();
	u_int8_t getW();
	u_int8_t getR();

	u_int16_t getStartPosition();
	void setStartPosition(u_int16_t pos);
	void addSymbol(Symbol *);
	vector<Symbol *> getSymbolsInSection();

	u_int16_t readTwoBytesOfContent(u_int8_t address);
	u_int8_t readByteOfContent(u_int8_t address);

	u_int8_t getAccessBits() {
		return accessBits;
	}

private:
	string name;
	int size;
	int number;
	u_int8_t * content;
	u_int8_t accessBits;
	u_int16_t startPostion;
	vector<Symbol *> symbolsInSection;
};

#endif /* FILES_SECTION_SECTION_H_ */
