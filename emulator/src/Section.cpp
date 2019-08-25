/*
 * Section.cpp
 *
 *  Created on: Jun 27, 2019
 *      Author: etf
 */

#include "Section.h"
#include <iostream>
#include <string.h>
#include <iomanip>

using namespace std;

Section::Section(string name, int number) {
	this->name = name;
	this->number = number;
	this->size = 0;
	this->content = 0;
	this->accessBits = 0;
	this->startPostion = 0;

	// 0 0 0 0 P E W R
	if (name == ".text") {
		this->accessBits = 0x0d; // 0 0 0 0 1 1 0 1
	} else if (name == ".data") {
		this->accessBits = 0x0b; // 0 0 0 0 1 0 1 1
	} else if (name == ".bss") {
		this->accessBits = 0x03; // 0 0 0 0 0 0 1 1
	}

}

Section::Section(string name, int size, int number) {
	this->name = name;
	this->number = number;
	this->size = size;
	this->content = 0;
	this->accessBits = 0;
	this->startPostion = 0;

	// 0 0 0 0 P E W R
	if (name == ".text") {
		this->accessBits = 0x0d; // 0 0 0 0 1 1 0 1
	} else if (name == ".data") {
		this->accessBits = 0x0b; // 0 0 0 0 1 0 1 1
	} else if (name == ".bss") {
		this->accessBits = 0x03; // 0 0 0 0 0 0 1 1
	}

}

Section::~Section() {
	// TODO Auto-generated destructor stub
}

const string& Section::getName() const {
	return name;
}

void Section::setName(const string& name) {
	this->name = name;
}

int Section::getNumber() const {
	return number;
}

void Section::setNumber(int number) {
	this->number = number;
}

int Section::getSize() const {
	return size;
}

void Section::setSize(int size) {
	this->size = size;
}

ostream& operator<<(ostream& out, const Section& s) {
	out << "{" << s.name << " - " << s.size << "}";
	return out;
}

void Section::createContent() {

	content = new u_int8_t[size];

	for (int i = 0; i < size; i++) {
		content[i] = 0X00;
	}

	/*for (int i = 0; i < size; i++) {
	 cout << hex << (unsigned) content[i] << " ";
	 }*/

}

void Section::writeTo(void *src, int pos, size_t length) {
	memcpy(content + pos, src, length);

	/*for (int i = 0; i < size; i++) {
	 cout << hex << (unsigned) content[i] << " ";
	 }
	 cout << endl;*/
}

void Section::writeZerosTo(int pos, size_t length) {
	memset(content + pos, 0, length);

	/*for (int i = 0; i < size; i++) {
	 cout << hex << (unsigned) content[i] << " ";
	 }*/
}

string Section::printContent() {
	stringstream out;
	const int tokensByLine = 8;
//    out << "Section:" << endl ;
	out << name;
	for (int i = 0; i < size; i++) {
		if (i % tokensByLine == 0)
			out << endl;
		out << setfill('0') << setw(2) << hex << (u_int32_t) content[i] << " ";

	}

	out << endl;
	out << ".end" << endl << endl;

	return out.str();
}

string Section::printSection() {

	string lG[2] = { "LOCAL", "GLOBAL" };

	stringstream ss;
	ss << setw(15) << name << setw(15) << size << setw(15) << number << setw(10)
			<< (unsigned) getR() << setw(3) << (unsigned) getW() << setw(3)
			<< (unsigned) getE() << setw(3) << (unsigned) getP() << endl;

	return ss.str();

}

void Section::setP() {
	accessBits |= 0x08;
}

void Section::setE() {
	accessBits |= 0x04;
}

void Section::setW() {
	accessBits |= 0x02;
}

void Section::setR() {
	accessBits |= 0x01;
}

u_int8_t Section::getP() {
	u_int8_t tmp(accessBits);
	tmp &= 0x08;

	return accessBits >> 3;
}

u_int8_t Section::getE() {
	u_int8_t tmp(accessBits);
	tmp &= 0x04;

	return tmp >> 2;
}

u_int8_t Section::getW() {
	u_int8_t tmp(accessBits);
	tmp &= 0x02;

	return tmp >> 1;
}

u_int8_t Section::getR() {
	u_int8_t tmp(accessBits);
	tmp &= 0x01;

	return tmp;
}

u_int16_t Section::getStartPosition() {
	return startPostion;
}

void Section::setStartPosition(u_int16_t pos) {
	this->startPostion = pos;
}

void Section::addSymbol(Symbol * symbol) {
	this->symbolsInSection.push_back(symbol);
}

vector<Symbol *> Section::getSymbolsInSection() {
	return symbolsInSection;
}

u_int16_t Section::readTwoBytesOfContent(u_int8_t address) {
	if (address < 0 || address + 1 > size)
		throw runtime_error(
				"Error while reading byte of content of section " + name);

	u_int8_t lower8bits = content[address];
	u_int8_t higher8bits = content[address + 1];

	return (higher8bits << 8) | lower8bits;
}

u_int8_t Section::readByteOfContent(u_int8_t address) {

	if (address < 0 || address > size)
		throw runtime_error(
				"Error while reading byte of content of section " + name);

	return content[address];
}

u_int8_t * Section::getContent() {
	return content;
}
