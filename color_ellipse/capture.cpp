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

RNG rng(12345);

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
    
    vector<Vec4i> hierarchy;
    vector<vector<Point> > contours;

    Mat gray, IMGBGR;
    namedWindow("OG", CV_WINDOW_AUTOSIZE);
    namedWindow("Threshold", CV_WINDOW_AUTOSIZE);

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
        imshow("OG",imgOriginal);

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
        Mat im_flood=imgThreshold.clone();
        floodFill(im_flood,cv::Point(0,0),Scalar(255));
        
        bitwise_not(im_flood,im_flood);

        imgThreshold=(im_flood|imgThreshold);

        imshow("Threshold",imgThreshold);
        

        findContours(imgThreshold,contours,hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0,0));

        vector<RotatedRect> minEllipse(contours.size());
        vector<RotatedRect> minRect(contours.size());
        for (int i=0; i<contours.size(); i++)
        {
            minRect[i]=minAreaRect(Mat(contours[i]));
            if(contours[i].size() >5)
            {
                minEllipse[i]=fitEllipse(Mat(contours[i]));
            }
        }
        Mat drawing=Mat::zeros(imgThreshold.size(), CV_8UC3);
        for(int i=0; i<contours.size();i++)
        {
            Scalar color= Scalar(rng.uniform(0,255),rng.uniform(0,255),rng.uniform(0,255));

            drawContours(drawing,contours,i,color,1,8,vector<Vec4i>(),0,Point());
            ellipse(drawing,minEllipse[i],color,2,8);

            Point2f rect_points[4]; minRect[i].points(rect_points);
            for(int k=0;k<4;k++)
            {
                line(drawing,rect_points[k],rect_points[(k+1)%4],color,1,8);
            }
        }
        for(int l=0;l<contours.size();l++)
        {
            if(minEllipse[l].size.width>20)
            {
                cout <<"width is :"<< minEllipse[l].size.width<<"height is"<<minEllipse[l].size.height<<endl;
            }
        }

        imshow("Threshold Image", drawing);

        if(waitKey(30)==27)
        {
            cout <<"esc key is pressed" <<endl;
            break; 
        }

    }
    return 0;
}

