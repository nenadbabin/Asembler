/*
 * AccessRights.h
 *
 *  Created on: Aug 11, 2019
 *      Author: etf
 */

#ifndef ACCESSRIGHTS_H_
#define ACCESSRIGHTS_H_

#include <stdint.h>

class AccessRights {
public:
	enum AccessType {
		R, W, E
	};
	AccessRights(uint16_t s, uint16_t e, uint8_t r) {
		startAddress = s;
		endAddress = e;
		rights = r;
	}

	virtual ~AccessRights() {
		// TODO Auto-generated destructor stub
	}

	bool checkAccess(uint16_t add, AccessType type) {
		if (add < startAddress || add > endAddress) {
			return true;
		} else {
			switch (type) {
			case AccessType::R: {
				return rights & 0x01;
			}
			case AccessType::W: {
				return rights & 0x02;
			}
			case AccessType::E: {
				return rights & 0x04;
			}
			}
		}

		return false;
	}

private:
	uint16_t startAddress;
	uint16_t endAddress;
	uint8_t rights;
}
;

#endif /* ACCESSRIGHTS_H_ */
