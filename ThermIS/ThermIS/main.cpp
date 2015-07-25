#include <string>
#include <iostream>
#include <stdio.h>
#include <math.h>
#include <opencv\cv.h>
#include <opencv\highgui.h>
#include <opencv2\objdetect\objdetect.hpp>
#include <opencv2\highgui\highgui.hpp>
#include <opencv2\imgproc\imgproc.hpp>
#include <vector>
#include <time.h>
#include <windows.h>

#include "SerialUtil.h"
#include "Calibration.h"

using namespace cv;
using namespace std;

//Global variable declaration
int curCalibrate = 0;
SerialUtil* su=new SerialUtil();
Calibration* cal = new Calibration();
string str;
bool dismissed = true, feverDetected = false, forceAlert = false;
std::ostringstream alertText;
Rect dismissButton(Point(142,742), Point(9,705));
Rect calibrateButton(Point(650,645), Point(506,563));
Rect toggleLaserButton(Point(650,740), Point(506,655));
Rect captureBounds(Point(650,554), Point(10,74));

//Function Prototypes
void calibrateServos(int x, int y);
void sendCalibrationCommand(int c);
void thermisGUICallBack(int event, int x, int y, int flags, void* userdata);
void calibrationCallBack(int event, int x, int y, int flags, void* userdata);
const std::string currentDateTime();

int main()
{
	//OpenCV variable declaration
	CascadeClassifier face_cascade;
	Mat display, cap_img,gray_img,  raw_img;
	vector<Rect> faces;

	//Create the console handle and save the original console colors
	HANDLE hConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO *ConsoleInfo = new CONSOLE_SCREEN_BUFFER_INFO();
	GetConsoleScreenBufferInfo(hConsoleHandle, ConsoleInfo);
	WORD OriginalColors = ConsoleInfo->wAttributes;

	//Load face cascade
	if(!face_cascade.load("haarcascade_frontalface_alt.xml")) {
		//Alternative cascade to load:
		//if(!face_cascade.load("lbpcascade_frontalface.xml")) {
		printf("Error loading cascade file for face");
		return 1;
	}

	//Load webcam feed to capture
	VideoCapture capture(0); //-1, 0, 1 device id
	if(!capture.isOpened()){
		printf("Cannot to initialize camera");
		return 1;
	}

	//Infinite loop
	while(1)
	{
		//Loop variable declarations
		bool faceDetected = false;
		int x =0, y=0;
		int largestFaceIndex = 0;
		int rectT =0, rectR=0, rectB=0, rectL=0;

		//Copy the capture into cap_img
		capture >> cap_img;
		capture >> raw_img;
		waitKey(10);

		//Make the capture black and white
		cvtColor(cap_img, gray_img, CV_BGR2GRAY);
		cv::equalizeHist(gray_img,gray_img);

		//Detect faces in the capture, maximum 3 neighbours, 1.1 accuracy, and from the gray_img capture feed. Max size of a face to be detected
		//is 300 pixels wide, while the smallest is 50 pixels wide
		face_cascade.detectMultiScale(gray_img, faces, 1.1, 3, CV_HAAR_SCALE_IMAGE | CV_HAAR_DO_CANNY_PRUNING, cvSize(50,50), cvSize(300,300));


		//If the system is calibrated, output the bounds of the servo
		if(cal->isCalibrated){
			line(cap_img, Point(cal->cPoints[1][0], cal->cPoints[1][1]), Point(cal->cPoints[2][0], cal->cPoints[2][1]), cvScalar(255,0,0), 1, CV_AA, 0);
			line(cap_img, Point(cal->cPoints[2][0], cal->cPoints[2][1]), Point(cal->cPoints[4][0], cal->cPoints[4][1]), cvScalar(255,0,0), 1, CV_AA, 0);
			line(cap_img, Point(cal->cPoints[4][0], cal->cPoints[4][1]), Point(cal->cPoints[3][0], cal->cPoints[3][1]), cvScalar(255,0,0), 1, CV_AA, 0);
			line(cap_img, Point(cal->cPoints[3][0], cal->cPoints[3][1]), Point(cal->cPoints[1][0], cal->cPoints[1][1]), cvScalar(255,0,0), 1, CV_AA, 0);
		}

		//For each face found
		for(int i=0; i < faces.size();i++)
		{
			//If the current face is larger than the largest face so far, update the largest face with the current face
			if(faces[i].width > faces[largestFaceIndex].width || faceDetected ==false){
				rectL = faces[i].x;
				rectT = faces[i].y;
				rectR = faces[i].x+faces[i].width;
				rectB = faces[i].y+faces[i].height;

				x = (faces[i].x+faces[i].width/2);
				y= (faces[i].y+faces[i].height/3);

				largestFaceIndex = i;
			}

			//Since at least one face was found, set the detected variable as true
			faceDetected = true;
		}

		//If a face was detected, output a green rectangle around the face and a red dot to show the point of interest
		if(faceDetected){
			Point pt1(rectR, rectB);
			Point pt2(rectL, rectT);
			Point pta(x, y);
			Point ptb(x+1, y+1);

			rectangle(cap_img, pt1, pt2, cvScalar(0,255,0), 2, 8, 0);
			rectangle(cap_img, pta, ptb, cvScalar(0,0,255), 2, 8, 0);
		}



		//Retrieve the temperature using the coordinates inside the detected face
		if(faceDetected && dismissed){
			try{
				//Sanity check if the serial port is still connected and the system has been calibrated
				if(SerialUtil::SP->IsConnected() && cal->isCalibrated)
				{
					//Make a new ostring
					std::ostringstream o;

					//Append coordinates of the servo mapping through the calibration object
					o << cal->getServoX(x, y) <<","<< cal->getServoY(x, y) <<".";

					//Retrieve the temperature and store
					double curTemp = su->getTemperature(o.str());

					Point pt1(rectR , rectB);
					Point pt2(rectL, rectT);
					cv::Rect faceRect(pt1, pt2);
					cv::Mat croppedFaceImage;

					croppedFaceImage = raw_img(faceRect).clone();
					imwrite("Resources/detected_face.jpg", croppedFaceImage);

					//Adjust the temperature and if it is above 38 degrees C, then output the alert
					if(cal->adjustTemp(curTemp)>38 ||(forceAlert &&faceDetected)){
						//set text to red
						SetConsoleTextAttribute(hConsoleHandle, FOREGROUND_RED | FOREGROUND_INTENSITY);

						cout << currentDateTime() << " ALERT: Individual with high body temperature detected\a" <<endl;
						cout << "Please manually check the individual" << endl << endl;

						//set text to white again
						SetConsoleTextAttribute(hConsoleHandle, OriginalColors);

						//set the system to alert detected and undismissed (yet)
						feverDetected = true;
						dismissed = false;

						//Save alert string at time of detection
						alertText << "ALERT: " <<currentDateTime();
					}

				}
			}catch(string s)
			{
				//Output an error if the temperature method fails
				cout<<"ERROR OCCURED::"<<s<<" \n";
			}
		}

		//Create a window
		namedWindow("ThermIS", 1);
		resizeWindow("ThermIS", 660,750);
		cv::Mat thermisGUI = cv::Mat::zeros(750,660,CV_8UC3);

		//Copy the background to the GUI
		cv::Mat destination(thermisGUI,
			cv::Range(0, 750),
			cv::Range(0, 660));
		Mat bg_img = imread("Resources/gui_bg.png", 1);
		bg_img.copyTo(destination);

		//Copy webcam (cap_img) to the GUI
		destination = cv::Mat(thermisGUI,
			cv::Range(74, 554),
			cv::Range(10, 650));
		cap_img.copyTo(destination);

		

		if(!cal->isCalibrated){ //Display welcome message if system is not calibrated
			int textHeight = 20;
			putText(thermisGUI, "Welcome to ThermIS", Point(155, 620), CV_FONT_HERSHEY_SIMPLEX, 0.5,  cvScalar(100,100,100), 1, CV_AA );
			putText(thermisGUI, "Please calibrate the system.", Point(155, 620+ textHeight), CV_FONT_HERSHEY_SIMPLEX, 0.5,  cvScalar(100,100,100), 1, CV_AA );
		} else { //Display alert if there is one and system is calibrated
			if((feverDetected  &&  !dismissed) || (forceAlert &&faceDetected) ){
				//Copy the detected face to the GUI
				Mat detectedFace = imread("Resources/detected_face.jpg", 1);
				if(detectedFace.data)//Check to see if the image exists
				{
					destination = cv::Mat(thermisGUI,
						cv::Range(568, 691),
						cv::Range(14, 137));
					resize(detectedFace, detectedFace, cvSize(123, 123));
					detectedFace.copyTo(destination);
				}

				int textHeight = 20;

				//Output error text
				putText(thermisGUI, alertText.str(), Point(155, 620), CV_FONT_HERSHEY_SIMPLEX, 0.5,  cvScalar(0,0,255), 1, CV_AA );
				textHeight = 15;
				putText(thermisGUI, "Please manually examine the individual.", Point(155, 620+ textHeight), CV_FONT_HERSHEY_SIMPLEX, 0.5,  cvScalar(0,0,255), 1, CV_AA );
			}
		}


		//set the callback function for any mouse event
		setMouseCallback("ThermIS", thermisGUICallBack, NULL);
		//Show the final resulting feed
		imshow("ThermIS", thermisGUI);


		//waitKey(3);
		char c = waitKey(30);
		if(c == 27){
			break;
			return 0;
		} else if (c ==99){
			sendCalibrationCommand(1);
			calibrateServos(x,y);
		} else if (c ==97){
			forceAlert = !forceAlert;
			if(forceAlert)
				cout << "Alerts are forced" <<endl;
			else
				cout << "Alerts are not forced" <<endl;
		}
	}
	return 0;
}


//Function to pick up mouse clicks on main GUI
void thermisGUICallBack(int event, int x, int y, int flags, void* userdata)
{
	Point click(x,y);
	//Left mouse button starts and continues the calibration process
	if  ( event == EVENT_LBUTTONDOWN )
	{
		if(dismissButton.contains(click)){
			cout << "Dismiss clicked" <<endl;
			feverDetected = false;
			dismissed = true;
			alertText.clear();
			alertText.str("");
		} else if(curCalibrate ==0 && calibrateButton.contains(click)){
			cout << "Calibration started"<< endl;
			cout << "Please click on point 1" << endl;
			sendCalibrationCommand(1);
			calibrateServos(x,y);
		} else if(toggleLaserButton.contains(click)){
			cout << "Toggle laser clicked" <<endl;
			String x = "l";
			su->commandWrite(x);
		}


	}
	//Right mouse button toggles the laser manually, for demonstrational purposes
	//EDIT THIS
	else if  ( event == EVENT_RBUTTONDOWN )
	{
		/*  String x = "l";
		su->commandWrite(x);
		cout << "Laser toggled" << endl;*/
		//Make a new ostring
		std::ostringstream o;

		//Append coordinates of the servo mapping through the calibration object
		o << cal->getServoX(x, y) <<","<< cal->getServoY(x, y) <<".";

		//Retrieve the temperature and store
		double curTemp = su->getTemperature(o.str());

		//Adjust the temperature and if it is above 38 degrees C, then output the alert
		if(cal->adjustTemp(curTemp)>38){
			//set text to red
			//SetConsoleTextAttribute(hConsoleHandle, FOREGROUND_RED | FOREGROUND_INTENSITY);

			cout << currentDateTime() << " ALERT: Individual with high body temperature detected\a" <<endl;
			cout << "Please manually check the individual" << endl << endl;

			//set text to white again
			//SetConsoleTextAttribute(hConsoleHandle, OriginalColors);
		}
	}
}

//Function to pick up mouse clicks on main GUI
void calibrationCallBack(int event, int x, int y, int flags, void* userdata)
{
	Point click(x,y);
	//Catch left mouse click during calibration
	if  ( event == EVENT_LBUTTONDOWN && captureBounds.contains(click))
	{
		if(curCalibrate <5)
			cout << "Please click on point " << curCalibrate+1 <<endl;
		cal->saveCPoint(curCalibrate, x-10, y-74);
		sendCalibrationCommand(2);
		calibrateServos(x,y);

	}
}



//Method to send calibration commands and receive calibration data
void sendCalibrationCommand(int c){
	try{
		if(SerialUtil::SP->IsConnected())
		{

			//If the calibration just started
			if(c==1){
				cal->calibrTemp = 0; //Reset the calibration temperature
				String x = "c";
				su->commandWrite(x);
				cal->calibrTemp = 0;
			} else{ //Or if the the system is mid-calibration
				String x = "n";
				double temp =  su->getTemperature(x);
				cal->calibrTemp +=0.2*temp; //Read temperature as a part of calibration
			}

		}
	}catch(string s)
	{
		cout<<"ERROR OCCURED::"<<s<<" \n";
	}	
}


void calibrateServos(int x, int y){
	curCalibrate +=1;
	//If the calibration is complete
	if(curCalibrate>5){
		cal->isCalibrated = true;
		curCalibrate =0;
		String x = "l";
		su->commandWrite(x);
		cout << "Calibration finished!" << endl << endl;
		//Redirect program flow back to main
		main();
	}

	//Create capture feed, similar to main however doesn't have unnecessary facial detection
	VideoCapture capture(0); //-1, 0, 1 device id
	if(!capture.isOpened()){
		printf("Cannot to initialize camera");
	}
	Mat cap_img,gray_img;
	vector<Rect> faces, eyes;

	while(1){
		capture >> cap_img;


		//Create a window
		namedWindow("ThermIS", 1);
		resizeWindow("ThermIS", 660,750);
		cv::Mat thermisGUI = cv::Mat::zeros(750,660,CV_8UC3);

		//Copy the background to the GUI
		cv::Mat destination(thermisGUI,
			cv::Range(0, 750),
			cv::Range(0, 660));
		Mat bg_img = imread("Resources/gui_bg_calibration.png", 1);
		bg_img.copyTo(destination);

		//Copy webcam (cap_img) to the GUI
		destination = cv::Mat(thermisGUI,
			cv::Range(74, 554),
			cv::Range(10, 650));
		cap_img.copyTo(destination);

		std::ostringstream message;

		//Append coordinates of the servo mapping through the calibration object
		message << "Please click on point " << curCalibrate << " of 5";

		//Display instructions
		int textHeight = 20;
		putText(thermisGUI, message.str(), Point(155, 620), CV_FONT_HERSHEY_SIMPLEX, 0.5,  cvScalar(100,100,100), 1, CV_AA );
		 
		//set the callback function for any mouse event
		setMouseCallback("ThermIS", calibrationCallBack, NULL);
		//Show the final resulting feed
		imshow("ThermIS", thermisGUI);


		//waitKey(3);
		char c = waitKey(30);
		if(c == 27){
			break;
		}
	}
}

//Method to get the timestamp
const std::string currentDateTime() {
	time_t     now = time(0);
	struct tm  tstruct;
	char       buf[80];
	tstruct = *localtime(&now);
	strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

	return buf;
}
