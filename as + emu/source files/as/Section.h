/*
 * Section.h
 *
 *  Created on: Jun 27, 2019
 *      Author: etf
 */

#ifndef FILES_SECTION_SECTION_H_
#define FILES_SECTION_SECTION_H_

#include <string>
using namespace std;

class Section {
public:
	Section(string name, int number);
	virtual ~Section();

	const string& getName() const;
	void setName(const string& name);
	int getNumber() const;
	void setNumber(int number);
	int getSize() const;
	void setSize(int size);

	void writeTo(void *src, int pos, size_t length);

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

private:
	string name;
	int size;
	int number;
	//flags ???
	u_int8_t * content;
	u_int8_t accessBits;
};

#endif /* FILES_SECTION_SECTION_H_ */
