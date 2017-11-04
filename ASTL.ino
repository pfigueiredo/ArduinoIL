#include "Parser.h"
#include "math.h"

Parser p = Parser();
char buffer[128];
int bufferPointer = 0;
bool inputError = false;

memValue* pow2 (memValue *valueA, memValue *valueB) {
	double a = p.GetDouble(valueA);
	double b = p.GetDouble(valueB);
	double c = pow(a, b);

	p.PutDouble(valueA, c);
	return valueA;
}

void setup() {
	Serial.begin(9600);
	p.SendHeader();
	p.SendPrompt();
	p.addBuiltinFunction("pow", pow2);

	////p.parse("127865 * 2.0 / (1033 + 4)");
	//p.parse("123.23");
	//p.parse("12* 2.0 / (3665 + 4) + 12* 2.0 / (3665 + 4 + 12* 2.0 / (3665 + 4))");
	//p.run(2);
}

void loop() {
	/* add main program code here */
	while (Serial.available()) {

		char inChar = (char)Serial.read();

		if (bufferPointer > sizeof(buffer) -1 && !inputError) {
			Serial.println();
			Serial.print(F("error: input lenght > "));
			Serial.println(sizeof(buffer));
			inputError = true; //continue read the error string until \n;
		} else if (inChar == 127) {
			if (bufferPointer > 0) {
				bufferPointer--;
			}
		} else if (!inputError)
			buffer[bufferPointer++] = inChar;

		Serial.print(inChar);

		if (inChar == '\n' || inChar == '\r') {
			if (inChar == '\r') Serial.println();
			buffer[bufferPointer] = '\0';

			if (!inputError) {
				if (p.parse(buffer)) {	
					p.run(2);
				}
			}
			Serial.println();
			p.SendPrompt();
			bufferPointer = 0;
			inputError = false;
		}

	}
}
