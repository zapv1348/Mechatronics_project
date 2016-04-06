/* Zachary Vogel
 * Mechatronics
 * doing some fancy capture
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <pthread.h>
#include <sched.h>
#include <math.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

#define HRES 1280
#define VRES 720

int main ( int argc, char** argv)
{
    VideoCapture cap(0);

    if( !cap.isOpened())
    {
        cout << "cannot open webcam" <<endl;
        return -1;
    }

    namedWindow("Control", CV_WINDOW_AUTOSIZE);

    int iLowH = 0;                                                  
    int iHighH = 255;

    int iLowS = 0; 
    int iHighS = 255;

    int iLowV = 0;
    int iHighV = 255;

    //Create trackbars in "Control" window
    cvCreateTrackbar("LowH", "Control", &iLowH, 179); //Hue (0 - 179)
    cvCreateTrackbar("HighH", "Control", &iHighH, 179);

    cvCreateTrackbar("LowS", "Control", &iLowS, 255); //Saturation (0 - 255)
    cvCreateTrackbar("HighS", "Control", &iHighS, 255);

    cvCreateTrackbar("LowV", "Control", &iLowV, 255); //Value (0 - 255)
    cvCreateTrackbar("HighV", "Control", &iHighV, 255);
    
    vector<Vec3f> circles;
    //Mat gray;
    
    Mat gray, IMGBGR;

    while(1)
    {
        Mat imgOriginal;

        bool bSuccess = cap.read(imgOriginal);
        if (!bSuccess)
        {
            cout << "Cannot read a grame from stream" <<endl;
            break;
        }
        Mat imgHSV;

        cvtColor(imgOriginal, imgHSV, COLOR_BGR2HSV);

        Mat imgThreshold;
        //imgThreshold=imgHSV;
        inRange(imgHSV, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH, iHighS, iHighV), imgThreshold);
        erode(imgThreshold, imgThreshold, getStructuringElement(MORPH_ELLIPSE, Size(5,5)));
        dilate(imgThreshold, imgThreshold, getStructuringElement(MORPH_ELLIPSE, Size(5,5)));

        dilate(imgThreshold, imgThreshold, getStructuringElement(MORPH_ELLIPSE, Size(5,5)));
        erode(imgThreshold, imgThreshold, getStructuringElement(MORPH_ELLIPSE, Size(5,5)));

        //cvtColor(imgThreshold,IMGBGR, CV_HSV2BGR);
        //cvtColor(imgThreshold,gray,CV_BGR2GRAY);
        //GaussianBlur(gray,gray,Size(9,9),2,2);
        //GaussianBlur(imgThreshold,imgThreshold,Size(9,9),2,2);
        
        //need to replace this with a find countours function that finds contours and approximates them with elipses.
        //HoughCircles(gray, circles, CV_HOUGH_GRADIENT, 1, gray.rows/8,100,50,0,0);
        HoughCircles(imgThreshold, circles, CV_HOUGH_GRADIENT, 1, imgThreshold.rows/8,100,50,0,0);

        for(size_t j=0; j<circles.size();j++)
        {
            Point center(cvRound(circles[j][0]),cvRound(circles[j][1]));
            int radius=cvRound(circles[j][2]);

            circle(imgThreshold,center,3,Scalar(0,255,0), -1,8,0);
            circle(imgThreshold,center,radius,Scalar(0,0,255),3,8,0);
        }
        
        imshow("Threshold Image", imgThreshold);

        if(waitKey(30)==27)
        {
            cout <<"esc key is pressed" <<endl;
            break; 
        }

    }
    return 0;
}

