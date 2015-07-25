//SerialUtil Library Created by SHUBHAM VERMA
#include "Serial.h"	// Library header to be included
#include <iostream>
#include <string>
using namespace std;

// This Demo reads from the specified serial port and reports the collected data
class SerialUtil{
	char port[15];				//Port Address for windows it is like "\\\\.\\COM4"
	char incomingData[256];		// don't forget to pre-allocate memory
	int incomingDataLength;
	int readResult;
	string receive_command;

	char outgoingData[256];		// don't forget to pre-allocate memory
	int outgoingDataLength;
	
	bool clearedRX;//check if RX is successfully cleared
public:
	static Serial* SP;//Pointer to SerialClass
	/*default Constructor*/
	SerialUtil();
	
	/*Parameterised Constructor Accepts parameter port:The port to be used for Serial Communication*/
	SerialUtil(string port);

	/*Function Prototypes*/
	double getTemperature(string& str);
	string read();
	void commandWrite(string& str);
};