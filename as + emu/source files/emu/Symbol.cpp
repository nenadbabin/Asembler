/*
 * Symbol.cpp
 *
 *  Created on: Jun 27, 2019
 *      Author: etf
 */

#include "Symbol.h"
#include <iostream>
#include <iomanip>

using namespace std;

Symbol::Symbol(string name, int number, string section, int value) {
	this->name = name;
	this->number = number;
	this->section = section;
	this->value = value;
	this->defined = 0;
	this->scope = ScopeType::LOCAL;

}

Symbol::Symbol(string name, int number) {
	this->name = name;
	this->number = number;
	this->section = "undefined";
	this->value = 0;
	this->defined = 0;
	this->scope = ScopeType::LOCAL;

}

Symbol::~Symbol() {
	// TODO Auto-generated destructor stub
}

int Symbol::getDefined() const {
	return defined;
}

void Symbol::setDefined(int defined) {
	this->defined = defined;
}

const string& Symbol::getName() const {
	return name;
}

void Symbol::setName(const string& name) {
	this->name = name;
}

int Symbol::getNumber() const {
	return number;
}

void Symbol::setNumber(int number) {
	this->number = number;
}

int Symbol::getValue() const {
	return value;
}

void Symbol::setValue(int value) {
	this->value = value;
}

string Symbol::getSection() const {
	return section;
}

void Symbol::setSection(string section) {
	this->section = section;
}

ScopeType Symbol::getScope() const {
	return scope;
}

void Symbol::setScope(ScopeType scope) {
	this->scope = scope;
}

ostream& operator<<(ostream& out, const Symbol& s) {
	out << "{" << s.name << " - " << s.section << " - " << s.value << " - "
			<< s.scope << "}";
	return out;
}

string Symbol::printSymbol() {

	string lG[2] = { "GLOBAL", "LOCAL" };

	stringstream ss;
	ss << setw(15) << name << setw(15) << section << setw(15) << hex
			<< (unsigned) value << setw(15) << lG[scope] << setw(15) << number
			<< endl;

	return ss.str();
}
