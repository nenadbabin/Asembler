/*
 * Symbol.h
 *
 *  Created on: Jun 27, 2019
 *      Author: etf
 */

#ifndef FILES_SYMBOL_SYMBOL_H_
#define FILES_SYMBOL_SYMBOL_H_

#include <string>
#include "Enums.h"
using namespace std;

class Symbol {
public:
	Symbol(string name, int number, string section, int value);
	Symbol(string name, int number);
	virtual ~Symbol();

	int getDefined() const;
	void setDefined(int defined);
	const string& getName() const;
	void setName(const string& name);
	int getNumber() const;
	void setNumber(int number);
	int getValue() const;
	void setValue(int value);
	string getSection() const;
	void setSection(string section);
	ScopeType getScope() const;
	void setScope(ScopeType scope);

	friend ostream& operator<<(ostream& os, const Symbol& s);
	string printSymbol();

private:
	string name;
	int defined; //0 - undefined; 1 - defined
	string section;
	int value;
	int number;
	ScopeType scope;
};

#endif /* FILES_SYMBOL_SYMBOL_H_ */
