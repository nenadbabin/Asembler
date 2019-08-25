/*
 * Enums.h
 *
 *  Created on: Jun 26, 2019
 *      Author: etf
 */

#ifndef ENUMS_H_
#define ENUMS_H_

enum TokenType {GLOB_EXT = 0, LABEL, SECTION, SYMBOL, OPERAND_REGISTRY, OPERAND_HEX, OPERAND_DEC, INSTRUCTION, DIRECTIVE, ILLEGAL};
enum ScopeType {GLOBAL = 0, LOCAL};
enum RelocationType {R_8 = 0, R_PC8, R_16, R_PC16, R_32, R_PC32};



#endif /* ENUMS_H_ */
