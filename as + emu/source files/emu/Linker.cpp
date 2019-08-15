/*
 * Linker.cpp
 *
 *  Created on: Jul 22, 2019
 *      Author: etf
 */

#include "Linker.h"
#include <unordered_map>
#include <string.h>
#include <iomanip>

using namespace std;

Linker::Linker(vector<ObjectFile *> files) {
	this->files = files;

}

Linker::~Linker() {
	// TODO Auto-generated destructor stub
}

unordered_map<string, vector<u_int8_t>> Linker::link(
		unordered_map<string, u_int16_t> &startAddresses,
		vector<AccessRights *> &accRights) {

	// dohvati sve globalne simbole
	unordered_map<string, Symbol *> globalSymbols;
	checkSymbols(globalSymbols);

	unordered_map<string, vector<Section *>> sectionPairs;

	// section_name0 : section0, section1, ...
	// section_name1 : section2, section3, ...
	// skupljene su sve istoimene sekcije iz svih fajlova
	for (auto &file : files) {
		for (auto &section : file->getSections()) {
			sectionPairs[section->getName()].push_back(section);
		}
	}

	// change value of symbols that are relative to start of the section
	// change starting positions of sections
	u_int16_t globalCounter = 0;
	bool cont = false;
	globalCounter = getAddressForLoading(startAddresses, sectionPairs);

	for (auto &sectionPair : sectionPairs) {
		string sectionName = sectionPair.first;
		cont = false;

		u_int16_t lc;
		if (startAddresses.count(sectionName) == 1) {
			lc = startAddresses[sectionName];
		} else {
			lc = globalCounter;
			startAddresses[sectionName] = lc;
			cont = true;
		}

		for (auto &section : sectionPair.second) {
			section->setStartPosition(lc);

			// za sve simbole definisane u sekciji se dodaje pomeraj od pocetka sekcije
			// ovo je pomeraj nakon punjenja
			for (auto &symbolInSection : section->getSymbolsInSection()) {
				//if (symbolInSection->getScope() == ScopeType::LOCAL) {
				int value = symbolInSection->getValue();
				symbolInSection->setValue(value + lc);
				//}
				//cout << *symbolInSection << endl;
			}
			// locationCounter se uvecava za velicinu sekcije koja je obradjenja
			lc += section->getSize();
		}

		if (cont == true) {
			globalCounter = lc;
		}
	}
	//cout << endl;

	int correct = checkOverlapping(sectionPairs);
	if (correct == -1) {
		throw runtime_error("Error while linking: section overlapping.");
	} else if (correct == -2) {
		throw runtime_error("Error while linking: section out of bounds.");
	}

	if (!createAccessRights(accRights, sectionPairs)) {
		throw runtime_error(
				"Error while linking: Not all sections that have same name have the same access rights.");
	}

	// change values of relocations that are realative to section
	for (auto &file : files) {
		for (auto &relocation : file->getRelocations()) {
			//if (relocation->getSymbol()->getScope() == ScopeType::LOCAL) {
			Section * sectionObject = relocation->getSection();

			int currentOffset = relocation->getOffset();
			int sectionOffset = sectionObject->getStartPosition();
			relocation->setOffset(currentOffset + sectionOffset);

			// change what is written in the file
			/*u_int16_t writtenInSection = sectionObject->readTwoBytesOfContent(
			 currentOffset);

			 writtenInSection += sectionOffset;
			 sectionObject->writeTo(&writtenInSection, currentOffset, 2);*/

			//cout << *relocation << currentOffset << endl;
			//}
		}
	}

	unordered_map<string, vector<u_int8_t>> sectionsContent;

	int overallSize = 0;
	for (auto &section : sectionPairs) {
		string sectionName = section.first;
		vector<Section *> sectionObjects = section.second;

		vector<u_int8_t> vector;
		int size = 0;
		for (auto &singleObj : sectionObjects) {

			for (int i = 0; i < singleObj->getSize(); i++) {
				vector.push_back(singleObj->getContent()[i]);
			}

			size += singleObj->getSize();
		}

		sectionsContent[sectionName] = vector;

		//cout << sectionName;
		/*const int tokensByLine = 8;
		 for (int i = 0; i < vector.size(); i++) {
		 if (i % tokensByLine == 0)
		 cout << endl;
		 cout  << setfill('0') << setw(2) << hex
		 << (u_int32_t) vector[i] << " ";
		 }
		 cout << endl;*/

		overallSize += size;

	}

	// check if overall size fits memory

	//edit content of sections
	for (auto &file : files) {
		for (auto &relocation : file->getRelocations()) {
			string sectionName = relocation->getSection()->getName();
			int relOffset = relocation->getOffset();

			relOffset -= startAddresses[sectionName];
			Symbol * symbolObject = relocation->getSymbol();
			//Section * sectionObject = file->getSection(sectionName); // sekcija u kojoj se radi relokacija
			Section * sectionObject = file->getSection(
					symbolObject->getSection()); //sekcija simbola

			RelocationType type = relocation->getReloctionType();

			// change what is written in the file
			u_int8_t lower8bits = sectionsContent[sectionName][relOffset];
			u_int8_t higher8bits = sectionsContent[sectionName][relOffset + 1];

			u_int16_t word = (higher8bits << 8) | lower8bits;

			if (symbolObject->getScope() == ScopeType::GLOBAL
					|| symbolObject->getSection() == "absolute") {
				// ugradjuje se vrednost simbola
				word += globalSymbols[symbolObject->getName()]->getValue();
			} else {
				word += (sectionObject->getStartPosition());
			}

			if (type == RelocationType::R_PC16) {
				word -= relocation->getOffset();
			} else {
				// ... to do ...
			}

			// mora da se promeni nacin adresiranja ako je globalan i apsolutan
			if (globalSymbols[symbolObject->getName()]) {
				if (globalSymbols[symbolObject->getName()]->getScope()
						== ScopeType::GLOBAL
						&& globalSymbols[symbolObject->getName()]->getSection()
								== "absolute"
						&& ((sectionsContent[sectionName][relOffset - 1] & 0xa0)
								== 0xa0)) {
					sectionsContent[sectionName][relOffset - 1] &=
							(uint8_t) 0x1f;
				}

				if (symbolObject->getScope() == ScopeType::GLOBAL
						&& symbolObject->getSection() == "absolute"
						&& (sectionsContent[sectionName][relOffset - 1] & 0x60)) {
					throw runtime_error(
							"Error while linking: Using 2B symbol with MEMIND8");
				}
			}

			sectionsContent[sectionName][relOffset] = (uint8_t) (word & 255u);
			sectionsContent[sectionName][relOffset + 1] =
					(uint8_t) (word >> 8u);

		}
	}

	ofstream fileOut("link.log");

	for (auto &sectionC : sectionsContent) {
		vector<u_int8_t> vector = sectionC.second;

		fileOut << sectionC.first << " " << hex
				<< startAddresses[sectionC.first];
		const int tokensByLine = 8;
		for (unsigned i = 0; i < vector.size(); i++) {
			if (i % tokensByLine == 0)
				fileOut << endl;
			fileOut << setfill('0') << setw(2) << hex << (u_int32_t) vector[i]
					<< " ";
		}
		fileOut << endl << endl;
		;

	}

	fileOut << endl;

	for (auto &file : files) {
		for (auto &section : file->getSections()) {
			fileOut << file->getFileName() << " " << section->getName() << " "
					<< hex << (unsigned) section->getStartPosition() << endl;
		}
	}

	fileOut << endl;

	fileOut << setfill(' ');
	fileOut << "%SYMBOLS%" << endl;
	fileOut << setw(15) << "NAME" << setw(15) << "SECTION" << setw(15)
			<< "VALUE" << setw(15) << "SCOPE" << setw(15) << "NUMBER" << endl;
	for (auto &file : files) {
		fileOut << file->getFileName() << endl;
		for (auto &symbol : file->getSymbols()) {
			if (symbol->getSection() != "undefined") {
				fileOut << symbol->printSymbol();
			}
		}
	}
	fileOut << "%END%" << endl << endl;

	fileOut << "%RELOCATIONS%" << endl;
	fileOut << setw(15) << "OFFSET" << setw(15) << "SECTION" << setw(15)
			<< "SYMBOL" << setw(15) << "RELATIVE_TO" << setw(15) << "TYPE"
			<< endl;
	for (auto &file : files) {
		fileOut << file->getFileName() << endl;
		for (auto &relocation : file->getRelocations()) {
			fileOut << relocation->printRelocation();
		}
	}
	fileOut << "%END%" << endl << endl;

	return sectionsContent;

}

uint16_t Linker::getAddressForLoading(
		unordered_map<string, u_int16_t> userInputSections,
		unordered_map<string, vector<Section *>> sectionObjects) {

	uint16_t max = 0;
	for (auto &uIS : userInputSections) {
		string sectionName = uIS.first;
		uint16_t startAddress = uIS.second;

		uint16_t temp = startAddress;
		vector<Section *> objects = sectionObjects[sectionName];
		for (auto &object : objects) {
			temp += object->getSize();
		}

		if (temp > max) {
			max = temp;
		}

	}

	return max;
}

void Linker::checkSymbols(unordered_map<string, Symbol *> &globalSymbols) {

	for (auto &file : files) {
		for (auto &symbol : file->getSymbols()) {
			if (symbol->getScope() == ScopeType::GLOBAL
					&& symbol->getSection() != "undefined") {
				if (globalSymbols.count(symbol->getName())) {
					throw runtime_error(
							"Duplicate definition of " + symbol->getName()
									+ ".");
				}
				globalSymbols[symbol->getName()] = symbol;
			}
		}
	}

	for (auto &file : files) {
		for (auto &symbol : file->getSymbols()) {
			if (symbol->getScope() == ScopeType::GLOBAL
					&& symbol->getSection() == "undefined"
					&& globalSymbols.count(symbol->getName()) == 0) {
				throw runtime_error(
						"Unresolved symbol " + symbol->getName() + ".");
			}
		}
	}

	if (globalSymbols.count("_start") == 0) {
		throw runtime_error("Missing symbol _start.");
	}

}

int Linker::checkOverlapping( // 0 - ok; -1 - overlapping; -2 out of bounds
		unordered_map<string, vector<Section *>> sectionPairs) {

	for (auto &sectionPair : sectionPairs) {
		string sectionName = sectionPair.first;
		int32_t startPosition = sectionPair.second[0]->getStartPosition();
		int32_t endPosition = startPosition;

		for (auto &section : sectionPair.second) {
			endPosition += section->getSize();
		}

		endPosition--;

		if (endPosition > 0x0000ffff)
			return -2;

		for (auto &pairCheck : sectionPairs) {
			if (pairCheck.first == sectionName)
				continue;

			int32_t startPosition2 = pairCheck.second[0]->getStartPosition();
			int32_t endPosition2 = startPosition2;
			for (auto &section2 : pairCheck.second) {
				endPosition2 += section2->getSize();
			}

			endPosition2--;

			if (!((startPosition > endPosition2 && endPosition > endPosition2)
					|| (startPosition < endPosition2
							&& endPosition < endPosition2))) {
				/*cout << sectionName << " " << pairCheck.first << endl;
				 cout << hex << (unsigned) startPosition << " "
				 << (unsigned) endPosition << endl;
				 cout << hex << (unsigned) startPosition2 << " "
				 << (unsigned) endPosition2 << endl;*/
				return -1;
			}

		}

	}

	return 0;
}

bool Linker::createAccessRights(vector<AccessRights *> &accRights,
		unordered_map<string, vector<Section *>> sectionPairs) {

	for (auto &sectionPair : sectionPairs) {
		string sectionName = sectionPair.first;
		int32_t startPosition = sectionPair.second[0]->getStartPosition();
		int32_t endPosition = startPosition;
		uint8_t accessRights = sectionPair.second[0]->getAccessBits();

		for (auto &section : sectionPair.second) {
			endPosition += section->getSize();

			if (section->getAccessBits() != accessRights) {
				return false;
			}
		}

		endPosition--;

		AccessRights * ar = new AccessRights(startPosition, endPosition,
				accessRights);
		accRights.push_back(ar);
	}

	return true;
}
