//SerialUtil Library Created by SHUBHAM VERMA, Edited By George Utsin
#include "SerialUtil.h"	// Library header to be included
Serial* SerialUtil::SP=NULL;//Pointer to SerialClass

//Default constructor
SerialUtil::SerialUtil()
{
	cout<<":::::::::::Welcome to ThermIS:::::::::::\n\nConnecting to the serial port\n";

	incomingData[0]='\0';
	incomingDataLength = 256;
	readResult = 0;
	receive_command="";

	strcpy(outgoingData,"");
	outgoingDataLength = 256;

	clearedRX=false;

	strcpy(port,"\\\\.\\COM4");
	SP = new Serial(this->port);

	if (SP->IsConnected())
		cout<<"Connected to serial port COM4\n";
	else 
		throw "Failed to open serial port!";
}

//Constructor for given serial port
SerialUtil::SerialUtil(string port)
{
	cout<<":::::::::::Welcome to ThermIS:::::::::::\n\nConnecting to the serial port\n";

	incomingData[0]='\0';
	incomingDataLength = 256;
	readResult = 0;
	receive_command="";

	strcpy(outgoingData,"");
	outgoingDataLength = 256;

	clearedRX=false;

	strcpy(this->port,port.c_str());
	SP = new Serial(this->port);

	if (SP->IsConnected())
		cout<<"Connected to serial port "<<port<<"\n";
	else 
		throw "Failed to open serial port!";
}

//Method to read a string from the arduino
string SerialUtil::read()
{
	string read="";

	readResult = SP->ReadData(incomingData,incomingDataLength);
	receive_command=(incomingData);
	strcpy(incomingData,"");//clear buffer
	clearedRX=PurgeComm(  SP->getHandle(), PURGE_RXCLEAR);//clear RX
	Sleep(10);

	read=string(receive_command);
	return read;
}

//Method that takes the command in the form of X,Y. and returns a temperature double
double SerialUtil::getTemperature(string& str)
{
	double resTemp = 0;
	if(SP->IsConnected())
	{
		//Copy string to char array, prepare rx and write data to arduino
		strcpy(outgoingData,str.c_str());
		clearedRX=PurgeComm(  SP->getHandle(), PURGE_RXCLEAR);//clear RX
		Sleep(10);
		SP->WriteData(outgoingData,outgoingDataLength);
		
		//Read temperature data coming back
		if(SP->IsConnected())
		{
			//Declaration of variables for checks and balances
			int i=0;
			bool gotData = false;
			readResult=-2;

			//While there is still data coming, append the data to the char array
			while(readResult != -1 && !gotData)
			{
				readResult = SP->ReadData(incomingData,6);
				if(readResult==-1){
					Sleep(10);
					readResult=0;
				}else{
					gotData = true;
				}
				
			}

			readResult=0;
			//Convert char array to string
			string tempString = string(incomingData);
			//Convert string to double
			resTemp = stod(tempString);

			clearedRX=PurgeComm(  SP->getHandle(), PURGE_RXCLEAR);//clear RX
		}

		//Clear the variables
		receive_command="";
		strcpy(incomingData,"\0");
		Sleep(10);

		return resTemp;
	}
}

void SerialUtil::commandWrite(string& str)
{
	if(SP->IsConnected())
	{
		//Copy the string to the char array
		strcpy(outgoingData,str.c_str());
		
		//Clear RX
		clearedRX=PurgeComm(  SP->getHandle(), PURGE_RXCLEAR);
		Sleep(10);

		//Write to the arduino
		SP->WriteData(outgoingData,outgoingDataLength);
		
		//Clear the variables
		receive_command="";
		strcpy(incomingData,"\0");
		Sleep(10);
		
	}
}



