//Calibration library created by George Utsin
#include "Calibration.h"	// Library header to be included

//Default constructor
Calibration::Calibration()
{
	for(int i = 0; i<5; i++){
		cPoints[i][0] = 0;
		cPoints[i][1] = 0;
	}
	calibrTemp = 0;
	isCalibrated = false;
	topBound = 130;
	bottomBound = 90;
	rightBound = 150;
	leftBound = 90;
}

//Method to save the points into the Calibration array during calibration
void Calibration::saveCPoint(int point, int x, int y)
{
	cPoints[point-1][0] = x;
	cPoints[point-1][1] = y;
}

//Method to get the x-coordinate on the servo mapping using the Calibration object.
//Note: Lines between the point are vertical, however the whole view is 'rotated' for simplicity
int Calibration::getServoX(int scrX, int scrY)
{
	//Determine rise of top line
	double topXRise = -1* (cPoints[3][0] -(cPoints[4][0]));
	//Determine run of top line
	double topXRun = cPoints[3][1] - cPoints[4][1];
	//Determine slope of top line
	double m = topXRise/topXRun;
	//Determine the X coordinate of the top line where at the y value of the given point
	double topX = -1*(m *scrY  -1*(cPoints[3][0]) - m*(cPoints[3][1]));
	//Determine distance between top line and given point
	double topDistance = scrX-topX;
	//Repeat steps above for bottom line
	double bottomXRise = -1* (cPoints[1][0] -(cPoints[2][0]));
	double bottomXRun = cPoints[1][1] - cPoints[2][1];
	m = bottomXRise/bottomXRun;
	double bottomX = -1*( m *scrY  -1*(cPoints[1][0]) - m*(cPoints[1][1]));
	double bottomDistance = bottomX-scrX;
	
	//Some sanity checks
	if(topDistance<0)
		topDistance = 1;
	if(topDistance > bottomX-topX)
		topDistance =  bottomX-topX;
	if(bottomDistance<0)
		bottomDistance = 1;
	if(bottomDistance > bottomX-topX)
		bottomDistance =  bottomX-topX;

	//Calculate the distance from the top line as a percentage of the way down
	double percentFromTop = topDistance/(topDistance+bottomDistance) ;
	
	//Calculate the final transposed servo mapping coordinate
	int xServo = (int) (rightBound - (rightBound-leftBound)*percentFromTop);

	return xServo;
}

//Method to get the y-coordinate on the servo mapping using the Calibration object.
int Calibration::getServoY(int scrX, int scrY)
{
	//Determine rise of top line
	double topYRise = -1* (cPoints[2][1] -(cPoints[4][1]));
	//Determine run of top line
	double topYRun = cPoints[2][0] - cPoints[4][0];
	//Determine slope of top line
	double m = topYRise/topYRun;
	//Determine the X coordinate of the top line where at the y value of the given point
	double topY = -1*(m *scrX  -1*(cPoints[2][1]) - m*(cPoints[2][0]));
	//Determine distance between top line and given point
	double topDistance = scrY-topY;
	
	//Repeat steps above for bottom line
	double bottomYRise = -1* (cPoints[1][1] -(cPoints[3][1]));
	double bottomYRun = cPoints[1][0] - cPoints[3][0];
	m = bottomYRise/bottomYRun;
	double bottomY = -1*( m *scrX  -1*(cPoints[1][1]) - m*(cPoints[1][0]));
	double bottomDistance = bottomY-scrY;
	
	
	//Some sanity checks
	if(topDistance<0)
		topDistance = 1;
	if(topDistance > bottomY-topY)
		topDistance =  bottomY-topY;
	if(bottomDistance<0)
		bottomDistance = 1;
	if(bottomDistance > bottomY-topY)
		bottomDistance =  bottomY-topY;

	//Calculate the distance from the top line as a percentage of the way down
	double percentFromTop = topDistance/(topDistance+bottomDistance) ;
	
	//Calculate the final transposed servo mapping coordinate
	int yServo = (int) (topBound - (topBound-bottomBound)*percentFromTop)+1 ;

	return yServo;
}

//Method to correct the temperature given the emmissivity and optics
double Calibration::adjustTemp(double curTemp)
{
	double adjusted =curTemp;
	if(curTemp > calibrTemp +0.3)
		adjusted = (curTemp - calibrTemp) * 22.5 + calibrTemp;
	return adjusted;
}

