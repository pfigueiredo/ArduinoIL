#include "Parser.h"
#include "Arduino.h"
#include <avr\eeprom.h>
#include "math.h"

PROGMEM const char range_IDStart[] = "_abcdefghijklpmnoprstuvwxyzABCDEFGHIJKLPMNOPRSTUVWXYZ";
PROGMEM const char range_ID[] = "_abcdefghijklpmnoprstuvwxyzABCDEFGHIJKLPMNOPRSTUVWXYZ0123456789";
PROGMEM const char range_Number[] = ".0123456789";
PROGMEM const char range_WS[] = " \t\n\r";

PROGMEM const char symb_EMPTY[] = "";
PROGMEM const char symb_MULTIPLY[] = "*"; 
PROGMEM const char symb_DIVIDE[] = "/";
PROGMEM const char symb_ADD[] = "+";
PROGMEM const char symb_SUBTRACT[] = "-"; 
PROGMEM const char symb_LEFT_PARS[] = "(";
PROGMEM const char symb_RIGHT_PARS[] = ")";
PROGMEM const char symb_EQUAL[] = "=";
PROGMEM const char symb_BEGIN[] = "begin";
PROGMEM const char symb_END[] = "end";
PROGMEM const char symb_PROG[] = "prog";
PROGMEM const char symb_EXEC[] = "exec";
PROGMEM const char symb_IF[] = "if";
PROGMEM const char symb_THEN[] = "then";
PROGMEM const char symb_ELSE[] = "else";
PROGMEM const char symb_WHILE[] = "while";
PROGMEM const char symb_FOR[] = "for";
PROGMEM const char symb_DO[] = "do";
PROGMEM const char symb_TO[] = "to";
PROGMEM const char symb_STEP[] = "step";
PROGMEM const char symb_FX[] = "fx";
PROGMEM const char symb_COLON[] = ";";
PROGMEM const char symb_NOT[] = "not";
PROGMEM const char symb_AND[] = "and";
PROGMEM const char symb_OR[] = "or";
PROGMEM const char symb_XOR[] = "xor";
PROGMEM const char symb_NAND[] = "nand";
PROGMEM const char symb_NOR[] = "nor";
PROGMEM const char symb_XNOR[] = "xnor";
PROGMEM const char symb_EQ[] = "==";
PROGMEM const char symb_NEQ[] = "!=";
PROGMEM const char symb_GT[] = ">";
PROGMEM const char symb_GTEQ[] = ">=";
PROGMEM const char symb_ST[] = "<";
PROGMEM const char symb_STEQ[] = "<=";
PROGMEM const char symb_POW[] = "^";
PROGMEM const char symb_COMMA[] = ",";

	//_WS, _EOF, _ERROR, _NUMBER, _ID, 
	//_MULTIPLY, _DIVIDE, _ADD, _SUBTRACT, _LEFT_PARS, _RIGHT_PARS,
	//_EQUAL, _BEGIN, _END, _PROG, _EXEC, _IF, _THEN, _ELSE, _WHILE, 
	//_FOR, _DO, _TO, _STEP, _FX, _COLON, _NOT, _AND, _OR, 
    //_XOR, _NAND, _NOR, _XNOR, _EQ, _NEQ, _GT, _GTEQ, _ST, 
	//_STEQ, _POW

const char* symbolsDef[] = {

	symb_EMPTY, //_WS
	symb_EMPTY, //_EOF
	symb_EMPTY, //ERROR
	symb_EMPTY, //NUMBER
	symb_EMPTY, //ID
	symb_MULTIPLY, symb_DIVIDE, symb_ADD, symb_SUBTRACT, symb_LEFT_PARS, symb_RIGHT_PARS,
	symb_EQUAL, symb_BEGIN, symb_END, symb_PROG, symb_EXEC, symb_IF, symb_THEN, symb_ELSE, symb_WHILE,
	symb_FOR, symb_DO, symb_TO, symb_STEP, symb_FX, symb_COLON, symb_NOT, symb_AND, symb_OR,
	symb_XOR, symb_NAND, symb_NOR, symb_XNOR, symb_EQ, symb_NEQ, symb_GT, symb_GTEQ, symb_ST,
	symb_STEQ, symb_POW, symb_COMMA
};

int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

void printHex(int num, int precision) {
     char tmp[16];
     char format[128];

     sprintf(format, "%%.%dX", precision);

     sprintf(tmp, format, num);
     Serial.print(tmp);
}

Parser::Parser() {
	romPointer = 0;
	byte b1 = eeprom_read_byte((uint8_t*)0);
	byte b2 = eeprom_read_byte((uint8_t*)1);
	romPointer = (b1 << 8) | b2;
	romPointer = 2;
	variables = NULL;
}

void Parser::SendPrompt() {

	Serial.print("(");
	Serial.print(freeRam());
	Serial.print(")> ");
}

void Parser::SendHeader() {
	//Serial.begin(9600);
	Serial.print(F("ASTL v0.1"));
	Serial.print(F(" RAM: "));
	Serial.print(freeRam());
	Serial.print(F(" ROM_P: "));
	Serial.println(romPointer);
	Serial.println(F("Ready."));
}

bool Parser::parse(const char * code) {

	strcpy(lastReadToken.Def, "");
	lastReadToken.Col = 0;
	lastReadToken.Type = _EOF;

	buffer = code;
	tokenPosition = 0;
	tranStackPointer = 0;
	calculatedStackSize = 0;
	calculatedHeapSize = 0;
	romPointer = 2;
	uint16_t stackSize = 0;
	uint16_t startRomAddress = romPointer;

	romPointer = writeByteToROM(0, romPointer); //make room to store the stack and heap sizes
	romPointer = writeByteToROM(0, romPointer);
	romPointer = writeByteToROM(0, romPointer);

	#ifdef DEBUG_PARSER
		Serial.print("Code: ");
		Serial.println(code);
	#endif

	ExpressionReturnType ret = parseProgram(&stackSize);

	if (calculatedHeapSize > 128)
		Serial.print(F("The calculated stack for the program is bigger then the 64 bytes allowed"));

	Serial.print(F("compile: "));
	Serial.print((ret != Invalid && lastReadToken.Type == _EOF) ? F("Ok;") : F("error"));

	if (ret == Invalid || lastReadToken.Type != _EOF) {
		Serial.print(" column: ");
		Serial.print(lastReadToken.Col);
		Serial.print(" near: '");
		Serial.print(lastReadToken.Def);
		Serial.print("' :");
		Serial.print(lastReadToken.Type);
	} else {
		romPointer = writeByteToROM(ENDPROG, romPointer);

		calculatedStackSize *= 5;

		Serial.print(F(" stack: "));
		Serial.print(calculatedStackSize);
		Serial.print(F(" bytes;"));

		Serial.print(F(" prog: "));
		Serial.print(romPointer - 3);
		Serial.println(F(" bytes;"));

		writeByteToROM(*((byte*)(void*)&(calculatedStackSize)), startRomAddress);

		#ifdef DEBUG_PARSER
		/*Serial.print(F("PROG: "));
		for (uint16_t i = startRomAddress; i < romPointer; i++) {
			uint16_t b = eeprom_read_byte((uint8_t*)i);
			printHex(b, 2);
			Serial.print(":");
			Serial.print(i);
			Serial.print(",");
		}

		Serial.print("Size: ");
		Serial.print(romPointer);
		Serial.println(" bytes");
		Serial.println();*/
		#endif
	}

	

	return ret != Invalid && lastReadToken.Type == _EOF;
}

bool Parser::testTokenizer(const char * code) {
	buffer = code;
	tokenPosition = 0;
	tranStackPointer = 0;

	tokenDef t = { "", _EOF };

	Serial.begin(9600);
	Serial.println(freeRam());
	Serial.println(code);

	do {

		getNextToken(&t);

		Serial.print((char*)t.Def);
		Serial.print(" (");
		Serial.println((int)t.Type);
		Serial.println(")");

	} while (t.Type != _ERROR &&  t.Type != _EOF);

	Serial.end();
}

bool Parser::isWhiteSpace(char* token) {
	return stringIsInList(token, 0, range_WS);
}

bool Parser::isIdentifier(char* token) {
	bool valid = true;
    if (strlen(token) == 0) valid = false;
    else
        valid &= charIsInList(token[0], range_IDStart);

    if (strlen(token) > 1)
        valid &= stringIsInList(token, 1, range_ID);

    return valid;
}

bool Parser::isNumber(char* token) {
	return stringIsInList(token, 0, range_Number);
}

bool Parser::stringIsInList(char* s, int start, const char* list) {
	for (uint16_t i = start; i < strlen(s); i++) {
		if (!charIsInList(s[i], list)) return false;
	}

    return true;
}

bool Parser::charIsInList(char c, const char* list) {

	for (uint8_t i = 0; i < strlen_P(list); i++) {
		char lc = (char)pgm_read_byte_near(list + i);
		if (c == lc) return true;
	}
	return false;
}

bool Parser::stringIsSymbol(char* s, const char* symbol) {
	if (strlen(s) == strlen_P(symbol)) {
		for (uint8_t i = 0; i < strlen(s); i++) {
            if (s[i] !=  (char)pgm_read_byte_near(symbol + i)) {
                return false;
            }
		}
		return true;
	} else
		return false;
}

bool Parser::beginTokenizerTran() {
	if (tranStackPointer < 5) {
		tranStack[tranStackPointer] = tokenPosition;
		tranStackPointer++;
		return true;
	}
	return false;
}

void Parser::commitTokenizerTran() {
	//just get the pointer back;
	//Serial.println(">>>> Commit");
	if (tranStackPointer > 0) tranStackPointer--;
}

void Parser::rollbackTokenizerTran() {
	//Serial.println("<<<< Rollback");
	//get the pointer back
	if (tranStackPointer > 0) tranStackPointer--;
	//and read the last position
	tokenPosition = tranStack[tranStackPointer];
}

void Parser::getNextToken(tokenDef * token) {
    do {
		rawGetNextToken(buffer, token);
	} while (token->Type == _WS);

	strcpy(lastReadToken.Def, token->Def);
	lastReadToken.Col = token->Col;
	lastReadToken.Type = token->Type;

	#ifdef DEBUG_TOKENIZER
	/*Serial.print(" ");
	Serial.print(token->Def);
	Serial.print(" (");
	Serial.print(token->Type);
	Serial.print(") col:");
	Serial.print(token->Col);
	Serial.print(" Pos:");
	Serial.println(tokenPosition);*/
	#endif
}

void Parser::rawGetNextToken(const char * code, tokenDef* ret) {

    byte numOfValidSymbols = 0;

	if (tokenPosition >= strlen(code)) {
		strcpy(ret->Def, "EOF");
		ret->Type = _EOF;
		return;
	}

	for (uint16_t i = tokenPosition; i < strlen(code); i++) {
		uint16_t curPosition = i - tokenPosition;
		char token[15] = { 0 };

		strncpy(token, code + tokenPosition, curPosition + 1); 

		int currNumOfValidSymbols = 0;

        for (uint8_t s = 0; s < _XSymbolsEnd; s++) {
            bool symbolIsValid = stringIsSymbol(token, symbolsDef[s]);
			
            if (symbolIsValid) {
                currNumOfValidSymbols++;
				ret->Col = i;
                ret->Type = (Symbols) s;
				strcpy(ret->Def, token);
            }
        }

        //ckeck Special Symbols
        if (isIdentifier(token)) {
            currNumOfValidSymbols++;
            strcpy(ret->Def, token);
			ret->Col = i;
            ret->Type = _ID;
        }

        if (isNumber(token)) {
            currNumOfValidSymbols++;
            strcpy(ret->Def, token);
			ret->Col = i;
            ret->Type = _NUMBER;
        }

        if (isWhiteSpace(token)) {
            currNumOfValidSymbols++;
            strcpy(ret->Def, token);
			ret->Col = i;
            ret->Type = _WS;
        }

        if (currNumOfValidSymbols == 0) {
            if (numOfValidSymbols == 1) {
                tokenPosition = i;
				return;
            } else {
				//Serial.println(currNumOfValidSymbols);
				//Serial.println(numOfValidSymbols);
				ret->Type = _ERROR;
				ret->Col = i;
				return;
            }
		} else { numOfValidSymbols = currNumOfValidSymbols; }
	}

	if (numOfValidSymbols == 1) {
        tokenPosition = strlen(code);
		ret->Col = tokenPosition;
		return;
    } else {
		strcpy(ret->Def, "");
		ret->Type = _ERROR;
		ret->Col = strlen(code);
		return;
    }

}

void Parser::calculateMemSizes(uint16_t stackSize, uint16_t heapSize) {
	if (stackSize > calculatedStackSize) calculatedStackSize = stackSize;
	if (heapSize > calculatedHeapSize) calculatedHeapSize = heapSize;
}

ExpressionReturnType Parser::parseProgram(uint16_t * stackSize) {
	return parseAssignExpression(stackSize);
}

ExpressionReturnType Parser::parseAssignExpression(uint16_t * stackSize) {
	if (!beginTokenizerTran()) return TokenierOverflow;

	tokenDef varToken = { "", _EOF };
	getNextToken(&varToken);
	if (varToken.Type == _ID) {
		tokenDef eqToken = { "", _EOF };
		getNextToken(&eqToken);
		if (eqToken.Type == _EQUAL) {
			commitTokenizerTran();
			ExpressionReturnType retType = parseAddExpression(stackSize);
			if (retType != Invalid) {
				varDef * varDef = defineVariable(varToken.Def, (retType == Double) ? memValueDouble : memValueLong);

				#ifdef DEBUG_PARSER
					Serial.print(freeRam());
					Serial.print(" ");
					Serial.print("STORE");
					Serial.print(" P:");
					Serial.println((int)varDef);
				#endif

				romPointer = writeByteToROM(STORE, romPointer);
				romPointer = writePointerToRom(varDef, romPointer);
				return retType;
			} else
				return retType;
		}
	}
	rollbackTokenizerTran();
	return parseAddExpression(stackSize);

}

/*
<AddExpression> ::= <AddExpression> '+' <MultExpression>
                |  <AddExpression> '-' <MultExpression>
                |  <MultExpression>*/
ExpressionReturnType Parser::parseAddExpression(uint16_t * stackSize) {

	//Serial.println("\t\tAddExpression");

    ExpressionReturnType retType = parseMultExpression(stackSize);

    if (retType != Invalid) {

		calculateMemSizes(*stackSize, 0);
        if (!beginTokenizerTran()) return TokenierOverflow;

		tokenDef token = { "", _EOF };
        getNextToken(&token);

        if (token.Type == _ADD || token.Type == _SUBTRACT) {

			commitTokenizerTran();

            ExpressionReturnType retType2 = parseAddExpression(stackSize);
            if (retType2 != Invalid) {

				#ifdef DEBUG_PARSER
					Serial.print(freeRam());
					Serial.print(" ");
					Serial.print(token.Type == _ADD ? "ADD" : "SUBTRACT");
					Serial.print(" P:");
					Serial.println(romPointer);
				#endif

				calculateMemSizes(*stackSize, 0);
				*stackSize -= 1;
				romPointer = writeByteToROM(token.Type == _ADD ? ADD : SUBTR, romPointer);

                return (retType > retType2) ? retType : retType2;
            }
        } else {
            rollbackTokenizerTran();
            return retType;
        }
    }
    return Invalid;
}

/*
<MultExpression> ::= <MultExpression> '*' <BaseExpression>
                |  <MultExpression> '/' <BaseExpression>
                |  <BaseExpression>*/

ExpressionReturnType Parser::parseMultExpression(uint16_t * stackSize) {

	//Serial.println("\t\tMultExpression");

    ExpressionReturnType retType = parseBaseExpression(stackSize);
    if (retType != Invalid) {
		calculateMemSizes(*stackSize, 0);
        if (!beginTokenizerTran()) return TokenierOverflow;

        tokenDef token = { "", _EOF };
        getNextToken(&token);
        if (token.Type == _MULTIPLY || token.Type == _DIVIDE) {

			commitTokenizerTran();
            ExpressionReturnType retType2 = parseNextMultExpression(token.Type, retType, stackSize);

            if (retType2 != Invalid) {
				calculateMemSizes(*stackSize, 0);
                return (retType > retType2) ? retType : retType2;
            } else {
                return Invalid;
            }
        }
        rollbackTokenizerTran();
        return retType;
    }
    return Invalid;
}

ExpressionReturnType Parser::parseNextMultExpression(
	Symbols operationInsert, ExpressionReturnType prevRetType, 
	uint16_t * stackSize
) {

	//Serial.println("\t\tNextMultExpression");

    ExpressionReturnType retType = parseBaseExpression(stackSize);

    if (retType != Invalid) {

		#ifdef DEBUG_PARSER
			Serial.print(freeRam());
			Serial.print(" ");
			Serial.print(operationInsert == _MULTIPLY ? "MULTIPLY" : "DIVIDE");
			Serial.print(" P:");
			Serial.println(romPointer);
		#endif

		calculateMemSizes(*stackSize, 0);
		*stackSize -= 1;
		romPointer = writeByteToROM(operationInsert == _MULTIPLY ? MULT : DIV, romPointer);

        retType = (prevRetType > retType) ? prevRetType : retType;

        if (!beginTokenizerTran()) return TokenierOverflow;

        tokenDef token = { "", _EOF };
        getNextToken(&token);

        if (token.Type == _MULTIPLY || token.Type == _DIVIDE) {

			commitTokenizerTran();
            ExpressionReturnType retType2 = parseNextMultExpression(token.Type, retType, stackSize);

            if (retType2 != Invalid) {
				calculateMemSizes(*stackSize, 0);
                return (retType > retType2) ? retType : retType2;
            } else {
                return Invalid;
            }
        }
        rollbackTokenizerTran();
        return retType;
    }
    return Invalid;
}

/*
<BaseExpression> ::= 'not' <Value>
                |  '-' <Value>
                | <Value> '^' <Value>
                | <Value>*/

ExpressionReturnType Parser::parseBaseExpression(uint16_t * stackSize) {

	//Serial.println("\t\tBaseExpression");

    ExpressionReturnType retType;
    if (!beginTokenizerTran()) return TokenierOverflow;

    tokenDef token = { "", _EOF };
    getNextToken(&token);

    if (token.Type == _NOT) {
        retType = parseValue(stackSize);
        if (retType != Invalid) {
			calculateMemSizes(*stackSize, 0);
			romPointer = writeByteToROM(NOT, romPointer);

			#ifdef DEBUG_PARSER
				Serial.print(freeRam());
				Serial.print(" ");
				Serial.println("NOT");
			#endif

            commitTokenizerTran();
            return retType;
        }
    } else if (token.Type == _SUBTRACT) {
        retType = parseValue(stackSize);
        if (retType != Invalid) {
			calculateMemSizes(*stackSize, 0);
			romPointer = writeByteToROM(NEG, romPointer);

			#ifdef DEBUG_PARSER
				Serial.print(freeRam());
				Serial.print(" ");
				Serial.println("NEGATE");
			#endif

            commitTokenizerTran();
            return retType;
        }
    }

    rollbackTokenizerTran();

    retType = parseValue(stackSize);

    if (retType != Invalid) {
		calculateMemSizes(*stackSize, 0);
        if (!beginTokenizerTran()) return TokenierOverflow;

		getNextToken(&token);

        if (token.Type == _POW) {
            retType = parseValue(stackSize);
            if (retType != Invalid) {
				calculateMemSizes(*stackSize, 0);
				*stackSize -= 1;
				romPointer = writeByteToROM(POW, romPointer);

				#ifdef DEBUG_PARSER
					Serial.print(freeRam());
					Serial.print(" ");
					Serial.println("POW");
				#endif

                commitTokenizerTran();
                return retType;
            }
        }
        rollbackTokenizerTran();
        return retType;
    }

    return Invalid;

}

/*
<Value>       ::= Identifier '(' <Params> ')'
                |  Identifier
                |  Number
                |  '(' <CompExpression> ')'
    */
ExpressionReturnType Parser::parseValue(uint16_t * stackSize) {

	//Serial.println("\t\tValueExpression");

    ExpressionReturnType retType;

    tokenDef token = { "", _EOF };
    getNextToken(&token);

    if (token.Type != _NUMBER && token.Type != _LEFT_PARS) {
        //var laToken = this.GetNextToken() //Todo implement look ahead
        if (token.Type == _ID) {

			//is it a function?
			if (!beginTokenizerTran()) return TokenierOverflow;

			tokenDef nextToken = {"", _EOF};
			getNextToken(&nextToken);

			if (nextToken.Type == _LEFT_PARS) { 
				//yes its a function
				commitTokenizerTran();

				uint16_t currStackSize = *stackSize;
				retType = parseParameters(stackSize);
				uint8_t numParams = *stackSize - currStackSize;

				if (retType != Invalid) {
					calculateMemSizes(*stackSize, 0);
					getNextToken(&nextToken);
					if (nextToken.Type == _RIGHT_PARS) {

						fxDef *func = getFunctionAddress(token.Def);

						if (numParams != func->numParams) {
							lastReadToken.Col = token.Col;
							lastReadToken.Type = token.Type;
							strcpy(lastReadToken.Def, token.Def);
							return Invalid;
						}

						if (func != NULL) {
							if (numParams == 0) *stackSize += 1;
							romPointer = writeByteToROM(GOSUB, romPointer);
							romPointer = writePointerToRom(func, romPointer);
							return Double; //Todo make this ok.
						} else {
							lastReadToken.Col = token.Col;
							lastReadToken.Type = token.Type;
							strcpy(lastReadToken.Def, token.Def);
							return Invalid;
						}
					}
				}
				return Invalid;

			} else { 
				// no its just a variable
				rollbackTokenizerTran();


				varDef *address = getVarAddress(token.Def);

				if (address == NULL) {
					address = defineVariable(token.Def, memValueDouble);
					//implicit declaration of variable... :(((
				}

				#ifdef DEBUG_PARSER
					Serial.print(freeRam());
					Serial.print(" ");
					Serial.print(F("PUSHV "));
					Serial.print(token.Def);
					Serial.print(" P:");
					Serial.println(romPointer);
				#endif

				*stackSize += 1;
				romPointer = writeByteToROM(PUSHV, romPointer);
				romPointer = writePointerToRom(address, romPointer);

				return Variable;
			}
        }

    } else {
        if (token.Type == _NUMBER) {

			if (strcspn(token.Def, ".") >= strlen(token.Def)) {
				long num = strtol(token.Def, NULL, 10);

				#ifdef DEBUG_PARSER
					Serial.print(freeRam());
					Serial.print(" ");
					Serial.print("PUSH(L) ");
					Serial.print(token.Def);
					Serial.print(" P:");
					Serial.println(romPointer);
				#endif

				*stackSize += 1;
				romPointer = writeByteToROM(PUSH, romPointer);
				romPointer = writeToRom(num, romPointer);

				return Long;
			} else {
				double num = strtod(token.Def, NULL);

				#ifdef DEBUG_PARSER
					Serial.print(freeRam());
					Serial.print(" ");
					Serial.print("PUSH(D) ");
					Serial.print(token.Def);
					Serial.print(" P:");
					Serial.println(romPointer);
				#endif

				*stackSize += 1;
				romPointer = writeByteToROM(PUSH, romPointer);
				romPointer = writeToRom(num, romPointer);

				return Double;
			}
            return Invalid;

        } else if (token.Type == _LEFT_PARS) {
            retType = parseAddExpression(stackSize);
            if (retType != Invalid) {
				calculateMemSizes(*stackSize, 0);
                getNextToken(&token);
                if (token.Type == _RIGHT_PARS) {
                    return retType;
                }
            }
        }
    }
    return Invalid;
}

/*      <Params> ::= <Params> ',' <CompExpression>
            | <CompExpression>
    */
ExpressionReturnType Parser::parseParameters(uint16_t * stackSize) {
	if (!beginTokenizerTran()) return TokenierOverflow;
	tokenDef token = { "", _EOF }; 
	getNextToken(&token);
	if (token.Type == _RIGHT_PARS) { //Empty parameter list;
		rollbackTokenizerTran();
		return Void;
	}

	rollbackTokenizerTran(); //start and parse expressions

	do {
		ExpressionReturnType retType = parseAddExpression(stackSize);
		if (!beginTokenizerTran()) return TokenierOverflow;
		tokenDef token = { "", _EOF }; 
		getNextToken(&token);
		if (token.Type == _RIGHT_PARS) { //Empty parameter list;
			rollbackTokenizerTran(); //the parentisis will be read again;
			break;
		} else if (token.Type == _COMMA) {
			commitTokenizerTran();
			continue;
		} else {
			rollbackTokenizerTran();
			return Invalid;
		}
	} while (true);

	return Double;
}

fxDef* Parser::addBuiltinFunction(const char * name, funcptr_0 pointer) { 
	return addBuiltinFunction(name, (void*) pointer, 0); 
}
fxDef* Parser::addBuiltinFunction(const char * name, funcptr_1 pointer) { 
	return addBuiltinFunction(name, (void*) pointer, 1); 
}
fxDef* Parser::addBuiltinFunction(const char * name, funcptr_2 pointer) { 
	return addBuiltinFunction(name, (void*) pointer, 2); 
}
fxDef* Parser::addBuiltinFunction(const char * name, funcptr_3 pointer) { 
	return addBuiltinFunction(name, (void*) pointer, 3); 
}
fxDef* Parser::addBuiltinFunction(const char * name, funcptr_4 pointer) { 
	return addBuiltinFunction(name, (void*) pointer, 4); 
}
fxDef* Parser::addBuiltinFunction(const char * name, funcptr_5 pointer) { 
	return addBuiltinFunction(name, (void*) pointer, 5); 
}

fxDef* Parser::addBuiltinFunction(const char * name, void *pointer, uint8_t numParameters) {
	fxDef *newFx = (fxDef*)malloc(sizeof(fxDef));
	newFx->numParams = numParameters;
	newFx->Fx = pointer;
	newFx->isBuiltin = true;
	strcpy(newFx->Def, name);

	fxDef *funcs = functions;
	fxDef *last = NULL;

	while (funcs != NULL) {
		last = funcs;
		funcs = funcs->next;
	}

	if (last == NULL)
		functions = newFx;
	else
		last->next = newFx;

	return newFx;

}

fxDef* Parser::getFunctionAddress(char * var) {
	fxDef *funcs = functions;
	while (funcs != NULL) {
		if (strcmp(funcs->Def, var)  == 0) {
			return funcs;
		}
		funcs = funcs->next;
	}
	return NULL;
}

varDef* Parser::defineVariable(char * var, memValueType type) {
	varDef *vars = variables;
	varDef *last = NULL;
	while (vars != NULL) {
		if (strcmp(vars->Def, var)  == 0) {
			return vars;
		}
		last = vars;
		vars = vars->next;
	}

	varDef *newVar = (varDef*)malloc(sizeof(varDef));

	if (newVar) {
		strcpy(newVar->Def, var);
		newVar->next = NULL;
		newVar->Type = type;

		#ifdef DEBUG_VARS
			Serial.print("NEWVAR: @");
			Serial.print((int)newVar);
			Serial.print(" Def: ");
			Serial.print(newVar->Def);
			Serial.print(" Root: @");
			Serial.println((int)variables);
		#endif

		if (last == NULL)
			variables = newVar;
		else
			last->next = newVar;

		return newVar;
	} else
		return NULL;

}

varDef* Parser::getVarAddress(char * var) {
	varDef *vars = variables;
	while (vars != NULL) {
		if (strcmp(vars->Def, var)  == 0) {
			return vars;
		}
		vars = vars->next;
	}
	return NULL;
}

uint16_t Parser::writeToRom(long value, uint16_t address) {

	MemValueMarker marker;
	byte buffer[4] = { 0 };

	if (value <= 0xFF) marker = memv_Integer1Byte;
	else if (value <= 0xFFFF) marker = memv_Integer2Byte;
	else if (value <= 0xFFFFFF) marker = memv_Integer3Byte;
	else if (value <= 0xFFFFFFFF) marker = memv_Integer4Byte;
	else marker = memv_Integer5Byte;	

	const byte* p = (const byte*)(const void*) &value; 

	for (int i = 0; i < sizeof(buffer); i++) {
		buffer[i] = *p++;
	}

	writeByteToROM(marker, address++);
	#ifdef DEBUG_ROM_OPS
		printHex(marker, 2); 
	#endif // DEBUG_PARSER
	for (uint8_t i = 0; i <= ((uint8_t)marker - (uint8_t)memv_Integer1Byte); i++) {
		writeByteToROM(buffer[i], address++);
		#ifdef DEBUG_ROM_OPS
			printHex(buffer[i], 2); 
		#endif // DEBUG_PARSER
	}

	#ifdef DEBUG_ROM_OPS
		Serial.println(" "); 
	#endif // DEBUG_PARSER

	return address;

}

uint16_t Parser::writeToRom(double value, uint16_t address) {
	
    writeByteToROM(memv_Double, address++);

	#ifdef DEBUG_ROM_OPS
		printHex(memv_Double, 2); 
	#endif // DEBUG_PARSER

	for (uint8_t i = 0; i < sizeof(double); i++) {
		writeByteToROM(((byte*)(void*)&value)[i], address++);
		#ifdef DEBUG_ROM_OPS
			printHex(((byte*)(void*)&value)[i], 2); 
		#endif // DEBUG_PARSER
	}

	#ifdef DEBUG_ROM_OPS
		Serial.println(" "); 
	#endif // DEBUG_PARSER

	return address;
}

uint16_t Parser::writePointerToRom(fxDef *pointer, uint16_t address) {
	writeByteToROM(memv_VarAddress, address++);
	uint16_t value = (uint16_t)pointer;

	#ifdef DEBUG_ROM_OPS
		printHex(memv_Double, 2); 
	#endif // DEBUG_PARSER

	for (uint8_t i = 0; i < sizeof(value); i++) {
		writeByteToROM(((byte*)(void*)&value)[i], address++);
		#ifdef DEBUG_ROM_OPS
			printHex(((byte*)(void*)&value)[i], 2); 
		#endif // DEBUG_PARSER
	}

	#ifdef DEBUG_ROM_OPS
		Serial.println(" "); 
	#endif // DEBUG_PARSER

	return address;
}

uint16_t Parser::writePointerToRom(varDef *pointer, uint16_t address) {
	writeByteToROM(memv_VarAddress, address++);
	uint16_t value = (uint16_t)pointer;

	#ifdef DEBUG_ROM_OPS
		printHex(memv_Double, 2); 
	#endif // DEBUG_PARSER

	for (uint8_t i = 0; i < sizeof(value); i++) {
		writeByteToROM(((byte*)(void*)&value)[i], address++);
		#ifdef DEBUG_ROM_OPS
			printHex(((byte*)(void*)&value)[i], 2); 
		#endif // DEBUG_PARSER
	}

	#ifdef DEBUG_ROM_OPS
		Serial.println(" "); 
	#endif // DEBUG_PARSER

	return address;
}

uint16_t Parser::readPointerFromRom(uint16_t address, varDef **pointer) {

	uint16_t pointerAddress = 0;

	MemValueMarker marker = (MemValueMarker)readByteFromROM(address++);
	if (marker == memv_VarAddress) {
		((byte*)(void*)&pointerAddress)[0] = readByteFromROM(address++);
		((byte*)(void*)&pointerAddress)[1] = readByteFromROM(address++);
		*pointer = (varDef*)pointerAddress;
	} else
		*pointer = NULL;

	return address;
}

uint16_t Parser::readPointerFromRom(uint16_t address, fxDef **pointer) {

	uint16_t pointerAddress = 0;

	MemValueMarker marker = (MemValueMarker)readByteFromROM(address++);
	if (marker == memv_VarAddress) {
		((byte*)(void*)&pointerAddress)[0] = readByteFromROM(address++);
		((byte*)(void*)&pointerAddress)[1] = readByteFromROM(address++);
		*pointer = (fxDef*)pointerAddress;
	} else
		*pointer = NULL;

	return address;
}

uint16_t Parser::readFromROM(uint16_t address, memValue *value) {


	MemValueMarker marker = (MemValueMarker)readByteFromROM(address++);

	#ifdef DEBUG_ROM_OPS
		Serial.print(" Marker: "); Serial.print(marker); Serial.print(" ");
	#endif

	#ifdef DEBUG_ROM_OPS
		Serial.print(address); Serial.print(";");
	#endif

	for (uint16_t i = 0; i < value->Size; i++)
		value->Value[i] = 0;

	value->Type = memValueLong;

	switch (marker) {
		case memv_VarAddress:
			value->Value[0] = readByteFromROM(address++);
			value->Value[1] = readByteFromROM(address++);
			break;
		case memv_Integer1Byte:
			value->Value[0] = readByteFromROM(address++);
			break;
		case memv_Integer2Byte:
			value->Value[0] = readByteFromROM(address++);
			value->Value[1] = readByteFromROM(address++);
			break;
		case memv_Integer3Byte:
			value->Value[0] = readByteFromROM(address++);
			value->Value[1] = readByteFromROM(address++);
			value->Value[2] = readByteFromROM(address++);
			break;
		case memv_Integer4Byte:
			value->Value[0] = readByteFromROM(address++);
			value->Value[1] = readByteFromROM(address++);
			value->Value[2] = readByteFromROM(address++);
			value->Value[3] = readByteFromROM(address++);
			break;
			//value->Type = (marker == Double) ? memValueDouble : memValueLong;
		case memv_Double:
			value->Value[0] = readByteFromROM(address++);
			value->Value[1] = readByteFromROM(address++);
			value->Value[2] = readByteFromROM(address++);
			value->Value[3] = readByteFromROM(address++);
			value->Type = memValueDouble;
			break;
		default:
			break;
	}

	#ifdef DEBUG_ROM_OPS
		if(value->Type == memValueDouble)
			Serial.print(*(double*)(void*)value->Value);
		else
			Serial.print(*(double*)(void*)value->Value);

		Serial.print(":");
		Serial.print(address);
	#endif	

	return address;
}

byte Parser::readByteFromROM(uint16_t address) {
	return (byte)eeprom_read_byte((uint8_t*)address);
}

uint16_t Parser::writeByteToROM(byte b, uint16_t address) {
	eeprom_write_byte((uint8_t*)address++, b);
	return address;
}

uint16_t Parser::pushValueIntoStack(memValue *value, byte *stack, uint16_t stackPointer, uint16_t stackSize) {

	/*Serial.print("SP:");
	Serial.print(stackPointer);*/

	if (stackPointer < stackSize) {

		MemValueMarker marker;

		if (value->Type == memValueDouble) {
			//Serial.print(" D ");
			marker = memv_Double;
		} else {
			//Serial.print(" L ");
			if (*(long*)(void*)(value->Value) <= 0xFF) marker = memv_Integer1Byte;
			else if (*(long*)(void*)(value->Value) <= 0xFFFF) marker = memv_Integer2Byte;
			else if (*(long*)(void*)(value->Value) <= 0xFFFFFF) marker = memv_Integer3Byte;
			else if (*(long*)(void*)(value->Value) <= 0xFFFFFFFF) marker = memv_Integer4Byte;
			else marker = memv_Integer5Byte;	
		}

		//printHex(marker, 2);
		if (marker == memv_Double) {
			for (int8_t i = value->Size - 1; i >= 0; i--) {
				stack[stackPointer++] = value->Value[i];
				//printHex(value->Value[i], 2);
			}
		} else {
			for (int8_t i = ((uint8_t)marker - (uint8_t)memv_Integer1Byte); i >= 0; i--) {
				stack[stackPointer++] = value->Value[i];
				//printHex(value->Value[i], 2);
			}
		}

		stack[stackPointer++] = marker;

	}

	return stackPointer;
}

uint16_t Parser::popFromStack(memValue *value, byte *stack, uint16_t stackPointer, uint16_t stackSize) {

	if (stackPointer <= stackSize) {

		MemValueMarker marker = (MemValueMarker)stack[--stackPointer];

		for (uint16_t i = 0; i < value->Size; i++)
			value->Value[i] = 0;

		value->Type = memValueLong;

		switch (marker) {
			case memv_VarAddress:
				value->Value[0] = stack[--stackPointer];
				value->Value[1] = stack[--stackPointer];
				break;
			case memv_Integer1Byte:
				value->Value[0] = stack[--stackPointer]; 
				break;
			case memv_Integer2Byte:
				value->Value[0] = stack[--stackPointer];
				value->Value[1] = stack[--stackPointer];
				break;
			case memv_Integer3Byte:
				value->Value[0] = stack[--stackPointer];
				value->Value[1] = stack[--stackPointer];
				value->Value[2] = stack[--stackPointer];
				break;
			case memv_Integer4Byte:
				value->Value[0] = stack[--stackPointer];
				value->Value[1] = stack[--stackPointer];
				value->Value[2] = stack[--stackPointer];
				value->Value[3] = stack[--stackPointer];
				break;
				//value->Type = (marker == Double) ? memValueDouble : memValueLong;
			case memv_Double:
				value->Value[0] = stack[--stackPointer];
				value->Value[1] = stack[--stackPointer];
				value->Value[2] = stack[--stackPointer];
				value->Value[3] = stack[--stackPointer];
				value->Type = memValueDouble;
				break;
			default:
				break;
		}

	}

	return stackPointer;
}



double Parser::GetDouble(memValue *value) {
	if (value->Type == memValueDouble) {
		return *(double*)(void*)value->Value;
	} else 
		return (double)GetLong(value);
}

long Parser::GetLong(memValue *value) {
	if (value->Type == memValueDouble)
		return (long)GetDouble(value);
	else
		return *(long*)(void*)value->Value;
}

void Parser::PutLong(memValue *value, long num) {
	value->Type = memValueLong;
	value->Value[0] = ((byte*)(void*)&num)[0];
	value->Value[1] = ((byte*)(void*)&num)[1];
	value->Value[2] = ((byte*)(void*)&num)[2];
	value->Value[3] = ((byte*)(void*)&num)[3];
}

void Parser::PutDouble(memValue *value, double num) {
	value->Type = memValueDouble;
	value->Value[0] = ((byte*)(void*)&num)[0];
	value->Value[1] = ((byte*)(void*)&num)[1];
	value->Value[2] = ((byte*)(void*)&num)[2];
	value->Value[3] = ((byte*)(void*)&num)[3];
}

bool Parser::IsDouble(memValue *value) {
	return value->Type == memValueDouble;
}

bool Parser::IsLong(memValue *value) {
	return value->Type == memValueLong;
}

void Parser::run(uint16_t address) {

	uint16_t stackSize;
	((byte*)(void*)&stackSize)[0] = readByteFromROM(address++);
	((byte*)(void*)&stackSize)[1] = readByteFromROM(address++);

	address++; 

	int16_t stackPointer = 0;
	byte *stack = (byte*) calloc(stackSize, sizeof(byte));
	OpCodes opCode;

	byte memValueArr[4] = { 0 };
	byte memValueArrB[4] = { 0 };
	memValue value = { memValueLong, sizeof(memValueArr), (byte*)memValueArr };
	memValue valueB = { memValueLong, sizeof(memValueArrB), (byte*)memValueArrB };
	varDef *pointer = NULL;
	fxDef *fxPointer = NULL;

	do {

		if (stackPointer < 0) {
			Serial.println(F("Error Stack reached 0"));
			return; //TODO: report stack error
		}

		#ifdef DEBUG_RUNTIME
			Serial.print(freeRam());
			Serial.print(" ");
			Serial.print(" Addr: ");
			Serial.print(address);
		#endif

		opCode = (OpCodes) readByteFromROM(address++);

		#ifdef DEBUG_RUNTIME
			Serial.print(" OPCode: ");
			printHex((int)opCode, 2);
			Serial.print(" ");
		#endif

		switch (opCode)
		{
		case PUSH:

			address = readFromROM(address, &value);

			#ifdef DEBUG_RUNTIME
				Serial.print("PUSH ");
				if (value.Type == memValueDouble)
					Serial.print(*(double*)(void*)value.Value);
				else
					Serial.print(*(long*)(void*)value.Value);
			#endif

			stackPointer = pushValueIntoStack(&value, stack, stackPointer, stackSize);

			break;
		case PUSHV:

			address = readPointerFromRom(address, &pointer);

			value.Type = pointer->Type;
			value.Value[0] = pointer->Value[0];
			value.Value[1] = pointer->Value[1];
			value.Value[2] = pointer->Value[2];
			value.Value[3] = pointer->Value[3];

			#ifdef DEBUG_RUNTIME
				Serial.print("PUSHV ");
				if (value.Type == memValueDouble)
					Serial.print(*(double*)(void*)value.Value);
				else
					Serial.print(*(long*)(void*)value.Value);
			#endif

			stackPointer = pushValueIntoStack(&value, stack, stackPointer, stackSize);

			break;
		case STORE:

			address = readPointerFromRom(address, &pointer);
			stackPointer = popFromStack(&value, stack, stackPointer, stackSize);

			pointer->Value[0] = value.Value[0];
			pointer->Value[1] = value.Value[1];
			pointer->Value[2] = value.Value[2];
			pointer->Value[3] = value.Value[3];
			pointer->Type = value.Type;

			#ifdef DEBUG_RUNTIME
				Serial.print("STORE @");
				Serial.print((int)pointer);
				Serial.print(" V:");
				Serial.print(*(long*)(void*)pointer->Value);
			#endif

			stackPointer = pushValueIntoStack(&value, stack, stackPointer, stackSize);

			break;
		case GOSUB:

			address = readPointerFromRom(address, &fxPointer);

			#ifdef DEBUG_RUNTIME
				Serial.print("GOSUB @");
				Serial.print((int)fxPointer);
			#endif

			stackPointer = evaluateFunction(fxPointer, stack, &value, stackPointer, stackSize);

			#ifdef DEBUG_RUNTIME
				Serial.print(" ");
				if (value.Type == memValueDouble)
					Serial.print(*(double*)(void*)value.Value);
				else
					Serial.print(*(long*)(void*)value.Value);
			#endif

			stackPointer = pushValueIntoStack(&value, stack, stackPointer, stackSize);

		case POP:
			break;
			stackPointer = popFromStack(&value, stack, stackPointer, stackSize);
		case POPV:
			break;
		case ADD:

			stackPointer = popFromStack(&value, stack, stackPointer, stackSize);
			stackPointer = popFromStack(&valueB, stack, stackPointer, stackSize);

			#ifdef DEBUG_RUNTIME
				if (valueB.Type == memValueDouble)
					Serial.print(*(double*)(void*)valueB.Value);
				else
					Serial.print(*(long*)(void*)valueB.Value);
				Serial.print(" + ");
				if (value.Type == memValueDouble)
					Serial.print(*(double*)(void*)value.Value);
				else
					Serial.print(*(long*)(void*)value.Value);
				Serial.print(" = ");
			#endif

			if (value.Type == memValueDouble || valueB.Type == memValueDouble) {
				double result = GetDouble(&valueB) + GetDouble(&value);
				value.Type = memValueDouble;
				PutDouble(&value, result);



			} else {
				long result = GetLong(&valueB) + GetLong(&value);
				value.Type = memValueLong;
				PutLong(&value, result);
			}

			#ifdef DEBUG_RUNTIME
			if (value.Type == memValueDouble)
					Serial.print(*(double*)(void*)value.Value);
				else
					Serial.print(*(long*)(void*)value.Value);
			#endif

			stackPointer = pushValueIntoStack(&value, stack, stackPointer, stackSize);

			break;
		case SUBTR:

			stackPointer = popFromStack(&value, stack, stackPointer, stackSize);
			stackPointer = popFromStack(&valueB, stack, stackPointer, stackSize);

			#ifdef DEBUG_RUNTIME
				if (valueB.Type == memValueDouble)
					Serial.print(*(double*)(void*)valueB.Value);
				else
					Serial.print(*(long*)(void*)valueB.Value);
				Serial.print(" - ");
				if (value.Type == memValueDouble)
					Serial.print(*(double*)(void*)value.Value);
				else
					Serial.print(*(long*)(void*)value.Value);
				Serial.print(" = ");
			#endif

			if (value.Type == memValueDouble || valueB.Type == memValueDouble) {
				double result = GetDouble(&valueB) - GetDouble(&value);
				value.Type = memValueDouble;
				PutDouble(&value, result);
			} else {
				long result = GetLong(&valueB) - GetLong(&value);
				value.Type = memValueLong;
				PutLong(&value, result);
			}

			#ifdef DEBUG_RUNTIME
			if (value.Type == memValueDouble)
					Serial.print(*(double*)(void*)value.Value);
				else
					Serial.print(*(long*)(void*)value.Value);
			#endif

			stackPointer = pushValueIntoStack(&value, stack, stackPointer, stackSize);

			break;
		case MULT:

			stackPointer = popFromStack(&value, stack, stackPointer, stackSize);
			stackPointer = popFromStack(&valueB, stack, stackPointer, stackSize);

			#ifdef DEBUG_RUNTIME
				if (valueB.Type == memValueDouble)
					Serial.print(*(double*)(void*)valueB.Value);
				else
					Serial.print(*(long*)(void*)valueB.Value);
				Serial.print(" x ");
				if (value.Type == memValueDouble)
					Serial.print(*(double*)(void*)value.Value);
				else
					Serial.print(*(long*)(void*)value.Value);
				Serial.print(" = ");
			#endif

			if (value.Type == memValueDouble || valueB.Type == memValueDouble) {
				double result = GetDouble(&valueB) * GetDouble(&value);
				value.Type = memValueDouble;
				PutDouble(&value, result);
			} else {
				long result = GetLong(&valueB) * GetLong(&value);
				value.Type = memValueLong;
				PutLong(&value, result);
			}

			#ifdef DEBUG_RUNTIME
			if (value.Type == memValueDouble)
					Serial.print(*(double*)(void*)value.Value);
				else
					Serial.print(*(long*)(void*)value.Value);
			#endif

			stackPointer = pushValueIntoStack(&value, stack, stackPointer, stackSize);

			break;
		case DIV:

			stackPointer = popFromStack(&value, stack, stackPointer, stackSize);
			stackPointer = popFromStack(&valueB, stack, stackPointer, stackSize);

			#ifdef DEBUG_RUNTIME
				if (valueB.Type == memValueDouble)
					Serial.print(*(double*)(void*)valueB.Value);
				else
					Serial.print(*(long*)(void*)valueB.Value);
				Serial.print(" / ");
				if (value.Type == memValueDouble)
					Serial.print(*(double*)(void*)value.Value);
				else
					Serial.print(*(long*)(void*)value.Value);
				Serial.print(" = ");
			#endif

			if (value.Type == memValueDouble || valueB.Type == memValueDouble) {
				double result = GetDouble(&valueB) / GetDouble(&value);
				value.Type = memValueDouble;
				PutDouble(&value, result);
			} else {
				long result = GetLong(&valueB) / GetLong(&value);
				value.Type = memValueLong;
				PutLong(&value, result);
			}

			#ifdef DEBUG_RUNTIME
			if (value.Type == memValueDouble)
					Serial.print(*(double*)(void*)value.Value);
				else
					Serial.print(*(long*)(void*)value.Value);
			#endif

			stackPointer = pushValueIntoStack(&value, stack, stackPointer, stackSize);
			break;
		
		default:
			break;
		}

		#ifdef DEBUG_RUNTIME
		Serial.println();
		#endif

		if (address > 256) break;

	} while (opCode != ENDPROG);

	Serial.print(F("Result: "));

	popFromStack(&value, stack, stackPointer, stackSize);
	if (value.Type == memValueDouble)
		Serial.print(*(double*)(void*)value.Value);
	else
		Serial.print(*(long*)(void*)value.Value);

	free(stack);

}

uint16_t Parser::evaluateFunction(fxDef* fxPointer, byte *stack, memValue *retValue, 
								   uint16_t stackPointer, uint16_t stackSize
) {

	byte mem[5*4] = {0};
	memValue mValues[] = {
		{ memValueLong, 4, (byte*)&(mem[0]) },
		{ memValueLong, 4, (byte*)&(mem[1*4]) },
		{ memValueLong, 4, (byte*)&(mem[2*4]) },
		{ memValueLong, 4, (byte*)&(mem[3*4]) },
		{ memValueLong, 4, (byte*)&(mem[4*4]) }
	};

	memValue* ret = NULL;

	for (int8_t i = fxPointer->numParams - 1; i >= 0 ; i--) {
		stackPointer = popFromStack(&(mValues[i]), stack, stackPointer, stackSize);
	}

	if (fxPointer->isBuiltin) {
		switch (fxPointer->numParams) {
			case 0: ret = (((funcptr_0)(fxPointer->Fx))()); break;
			case 1: ret = (((funcptr_1)(fxPointer->Fx))(&(mValues[0]))); break;
			case 2: ret = (((funcptr_2)(fxPointer->Fx))(&(mValues[0]), &(mValues[1]))); break;
			case 3: ret = (((funcptr_3)(fxPointer->Fx))(&(mValues[0]), &(mValues[1]), &(mValues[2]))); break;
			case 4: ret = (((funcptr_4)(fxPointer->Fx))(&(mValues[0]), &(mValues[1]), &(mValues[2]), &(mValues[3]))); break;
			case 5: ret = (((funcptr_5)(fxPointer->Fx))(&(mValues[0]), &(mValues[1]), &(mValues[2]), &(mValues[3]), &(mValues[4]))); break;
			default: break;
		};
	}

	if (ret != NULL) {
		retValue->Type = ret->Type;
		retValue->Size = ret->Size;
		retValue->Value[0] = ret->Value[0];
		retValue->Value[1] = ret->Value[1];
		retValue->Value[2] = ret->Value[2];
		retValue->Value[3] = ret->Value[3];
	}
	
	return stackPointer;

}