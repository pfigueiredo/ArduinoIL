#ifndef Parser_h
#define Parser_h

#include "Arduino.h"
#include <eeprom.h>

//#define DEBUG_TOKENIZER
//#define DEBUG_VARS
//#define DEBUG_PARSER
//#define DEBUG_ROM_OPS
//#define DEBUG_RUNTIME

enum MemValueMarker {
	memv_Integer1Byte = 0,
	memv_Integer2Byte = 1,
	memv_Integer3Byte = 2,
	memv_Integer4Byte = 3,
	memv_Integer5Byte = 4,
	memv_Integer6Byte = 5,
	memv_Integer7Byte = 6,
	memv_Integer8Byte = 7,
	memv_Double	= 8,
	memv_VarAddress = 9,
	memv_End = 10
};

enum ExpressionReturnType {
    Invalid,
    Byte,
    Integer,
    Long,
    Double,
	Variable,
	Void,
	TokenierOverflow,
	StackOverFlow
};

enum Symbols { 
	_WS, _EOF, _ERROR, _NUMBER, _ID, _MULTIPLY, _DIVIDE, _ADD, _SUBTRACT, _LEFT_PARS, _RIGHT_PARS,
	_EQUAL, _BEGIN, _END, _PROG, _EXEC, _IF, _THEN, _ELSE, _WHILE, _FOR, _DO, _TO, _STEP,
	_FX, _COLON, _NOT, _AND, _OR, _XOR, _NAND, _NOR, _XNOR, _EQ, _NEQ, _GT, _GTEQ, _ST, _STEQ, _POW,
	_COMMA, _XSymbolsEnd
};

enum OpCodes {
	PUSH, PUSHV, POP, POPV,
	ADD, SUBTR, MULT, DIV, NEG, NOT, AND, OR, XOR, NAND, NOR, XNOR,
	EQ, NEQ, GT, GTEQ, ST, STEQ, STORE, GOSUB, RET, POW, ENDPROG
};

struct tokenDef {
    char Def[15];
    Symbols Type;
	int Col;
};

enum memValueType { memValueLong, memValueDouble };

struct varDef {
	char Def[15];
	memValueType Type;
	byte Value[4];
	varDef *next;
};

struct fxDef {
	char Def[15];
	memValueType Type;
	uint8_t numParams;
	bool isBuiltin;
	void* Fx;
	fxDef *next;
};

struct memValue {
	memValueType Type;
	uint8_t Size;
	byte *Value;
};

typedef memValue* (*funcptr_0)();
typedef memValue* (*funcptr_1)(memValue*);
typedef memValue* (*funcptr_2)(memValue*, memValue*);
typedef memValue* (*funcptr_3)(memValue*, memValue*, memValue*);
typedef memValue* (*funcptr_4)(memValue*, memValue*, memValue*, memValue*);
typedef memValue* (*funcptr_5)(memValue*, memValue*, memValue*, memValue*, memValue*);

class Parser {

public:
	Parser ();
	void SendHeader();
	void SendPrompt();
	bool parse(const char * code);
	bool testTokenizer(const char * code);
	uint16_t calculatedStackSize;
	uint16_t calculatedHeapSize;
	void run(uint16_t address);
	fxDef* addBuiltinFunction(const char * name, funcptr_0 pointer);
	fxDef* addBuiltinFunction(const char * name, funcptr_1 pointer);
	fxDef* addBuiltinFunction(const char * name, funcptr_2 pointer);
	fxDef* addBuiltinFunction(const char * name, funcptr_3 pointer);
	fxDef* addBuiltinFunction(const char * name, funcptr_4 pointer);
	fxDef* addBuiltinFunction(const char * name, funcptr_5 pointer);

	double GetDouble(memValue *value);
	long GetLong(memValue *value);
	void PutLong(memValue *value, long num);
	void PutDouble(memValue *value, double num);
	bool IsDouble(memValue *value);
	bool IsLong(memValue *value);

private:

	bool isWhiteSpace(char* token);
	bool isIdentifier(char* token);
	bool isNumber(char* token);
	bool stringIsInList(char* s, int start, const char* list);
	bool charIsInList(char c, const char* list);
	bool stringIsSymbol(char* s, const char* symbol);
	bool beginTokenizerTran();
	void commitTokenizerTran();
	void rollbackTokenizerTran();
	void getNextToken(tokenDef * ret);
	void rawGetNextToken(const char * code, tokenDef* ret);

	void calculateMemSizes(uint16_t stackSize, uint16_t heapSize);

	ExpressionReturnType parseProgram(uint16_t * stackSize);
	ExpressionReturnType parseAssignExpression(uint16_t * stackSize);
	ExpressionReturnType parseAddExpression(uint16_t * stackSize);
	ExpressionReturnType parseMultExpression(uint16_t * stackSize);
	ExpressionReturnType parseBaseExpression(uint16_t * stackSize);
	ExpressionReturnType parseNextMultExpression(Symbols operationInsert, ExpressionReturnType prevRetType, uint16_t * stackSize);
	ExpressionReturnType parseValue(uint16_t * stackSize);
	ExpressionReturnType parseParameters(uint16_t * stackSize);


	fxDef* addBuiltinFunction(const char * name, void *pointer, uint8_t numParameters);
	varDef* defineVariable(char * var, memValueType type);
	varDef* getVarAddress(char * var); 
	fxDef* getFunctionAddress(char * var); 
	uint16_t evaluateFunction(fxDef* fxPointer, byte *stack, memValue *retValue, uint16_t stackPointer, uint16_t stackSize);
	uint16_t writeToRom(long value, uint16_t address);
	uint16_t writeToRom(double value, uint16_t address);
	uint16_t writePointerToRom(fxDef *pointer, uint16_t address);
	uint16_t writePointerToRom(varDef *pointer, uint16_t address);
	uint16_t readPointerFromRom(uint16_t address, varDef **pointer);
	uint16_t readPointerFromRom(uint16_t address, fxDef **pointer);
	uint16_t writeByteToROM(byte b, uint16_t address);
	uint16_t readFromROM(uint16_t address, memValue *value);
	byte readByteFromROM(uint16_t address);
	
	uint16_t pushValueIntoStack(memValue *value, byte *stack, uint16_t stackPointer, uint16_t stackSize);
	uint16_t popFromStack(memValue *value, byte *stack, uint16_t stackPointer, uint16_t stackSize);

	const char* buffer;
	uint16_t tokenPosition;
	uint16_t tranStack[5];
	byte tranStackPointer;
	uint16_t romPointer;
	tokenDef lastReadToken;
	varDef * variables;
	fxDef * functions;

};

#endif