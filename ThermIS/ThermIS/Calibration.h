#include <iostream>
#include <string>
using namespace std;

class Calibration{
	
public:
	Calibration();

	double calibrTemp;
	bool isCalibrated;
	int cPoints[5][2];
	int topBound, bottomBound, rightBound, leftBound;

	void saveCPoint(int point, int x, int y);
	int getServoX(int scrX, int scrY);
	int getServoY(int scrX, int scrY);
	double adjustTemp(double curTemp);

};