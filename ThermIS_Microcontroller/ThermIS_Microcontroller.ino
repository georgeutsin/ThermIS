#include <i2cmaster.h>
#include <Servo.h>

Servo horizontalServo, verticalServo;
int horizontalPosition;
int verticalPosition ;
String inString = "";
int ver = 90;
int hor = 120;
int currentPosition = 1;
int topBound = 130, bottomBound = 90, leftBound = 90, rightBound = 150;

void setup(){
	Serial.begin(9600);
	Serial.println("Setup...");
	
	i2c_init(); //Initialise the i2c bus
	PORTC = (1 << PORTC4) | (1 << PORTC5);//enable pullups
        horizontalServo.attach(7);
        verticalServo.attach(9);
       
        horizontalServo.write((leftBound+rightBound)/2);
        verticalServo.write(bottomBound);
        pinMode(11, OUTPUT);
}

void loop(){
    while (Serial.available() > 0) {
        int inChar = Serial.read();
        if(inChar == 'l'){ //Toggle the laser
            digitalWrite(11, !digitalRead(11));
        } else if(inChar == 'c'){ //Start the calibration
          digitalWrite(11, HIGH);
          horizontalServo.write((rightBound+leftBound)/2);
          verticalServo.write((topBound+bottomBound)/2);
          currentPosition =2;
          measureTemp();
        } else if(inChar == 'n'){ //Continue the calibration and point servos to the right spot
          measureTemp();
          switch (currentPosition) {
            case 2:
              horizontalServo.write(leftBound);
              verticalServo.write(bottomBound);
              currentPosition +=1;
              break;
            case 3:
              horizontalServo.write(leftBound);
              verticalServo.write(topBound);
              currentPosition +=1;
              break;
             case 4:
              horizontalServo.write(rightBound);
              verticalServo.write(bottomBound);
              currentPosition +=1;
              break;
             case 5:
              horizontalServo.write(rightBound);
              verticalServo.write(topBound);
              currentPosition =1;
              delay(500);
              break;
            default:
             delay(500);
              currentPosition = 1; 
              break;
          }
        } else if (inChar == ',') { //Break off the inString and save to x-coordinate
            hor = inString.toInt();
            inString="";
        } else if (inChar == '.') { //Break off the inString and save to y-coordinate. Point servos and take thermal reading
          ver = inString.toInt();
          inString="";
          if(hor >= leftBound && hor <=rightBound && ver >=bottomBound && ver<= topBound){
            horizontalServo.write(hor);
            verticalServo.write(ver);
          }
          delay(285);
          measureTemp();
        } else if (isDigit(inChar)) { //If applicable, append the incoming digit to the inString
          // convert the incoming byte to a char
          // and add it to the string:
          inString += (char)inChar;
        } else {
          //delay(100);
        }
        
    }
    inString="";
    
    delay(80); // wait a moment before the next serial instantiation
}

//Method to measure the temperature and output the data through Serial
void measureTemp(){
  int dev = 0x5A<<1;
  int data_low = 0;
  int data_high = 0;
  int pec = 0;
  
  i2c_start_wait(dev+I2C_WRITE);
  i2c_write(0x07);
  
  // read
  i2c_rep_start(dev+I2C_READ);
  data_low = i2c_readAck(); //Read 1 byte and then send ack
  data_high = i2c_readAck(); //Read 1 byte and then send ack
  pec = i2c_readNak();
  i2c_stop();
  
  //This converts high and low bytes together and processes temperature, MSB is a error bit and is ignored for temps
  double tempFactor = 0.02; // 0.02 degrees per LSB (measurement resolution of the MLX90614)
  double tempData = 0x0000; // zero out the data
  int frac; // data past the decimal point
  
  // This masks off the error bit of the high byte, then moves it left 8 bits and adds the low byte.
  tempData = (double)(((data_high & 0x007F) << 8) + data_low);
  tempData = (tempData * tempFactor)-0.01;
  
  float celcius = tempData - 273.15;
  Serial.println(celcius);
}


