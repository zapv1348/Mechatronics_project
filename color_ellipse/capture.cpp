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

#define HRES 640
#define VRES 480

RNG rng(12345);

string type2str (int type)
{
    string r;
    uchar depth=type & CV_MAT_DEPTH_MASK;
    uchar chans=1+(type>>CV_CN_SHIFT);
    switch(depth)
    {
        case CV_8U: r="8U"; break;
        case CV_8S: r="8S"; break;
        case CV_16U: r="16U"; break;
        case CV_16S: r="16S"; break;
        case CV_32S: r="32S"; break;
        case CV_32F: r="32F"; break;
        case CV_64F: r="64F"; break;
    }
    r+="C";
    r+=(chans+'0');
    return r;
}

int super_cam()
{
    VideoCapture cap(0);
    cap.set(CV_CAP_PROP_FRAME_WIDTH,HRES);
    cap.set(CV_CAP_PROP_FRAME_HEIGHT,VRES);
    if( !cap.isOpened())
    {
        cout << "cannot open webcam" <<endl;
        return -1;
    }

    /*namedWindow("Control", CV_WINDOW_AUTOSIZE);

    int iLowH = 0;                                                  
    int iHighH = 255;

    int iLowS = 0; 
    int iHighS = 255;

    int iLowV = 0;
    int iHighV = 255;

    //Create trackbars in "Control" window
    cvCreateTrackbar("LowH", "Control", &iLowH, 255); //Hue (0 - 179)
    cvCreateTrackbar("HighH", "Control", &iHighH, 255);

    cvCreateTrackbar("LowS", "Control", &iLowS, 255); //Saturation (0 - 255)
    cvCreateTrackbar("HighS", "Control", &iHighS, 255);

    cvCreateTrackbar("LowV", "Control", &iLowV, 255); //Value (0 - 255)
    cvCreateTrackbar("HighV", "Control", &iHighV, 255);*/

    vector<Vec3f> circles;
    //Mat gray;

    vector<Vec4i> hierarchy;
    vector<vector<Point> > contours;

    Mat gray, IMGBGR;
    //namedWindow("OG", CV_WINDOW_AUTOSIZE);
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
        //string ty=type2str(imgOriginal.type());
        //printf("Matrix: %s %dx%d \n",ty.c_str(),imgOriginal.cols,imgOriginal.rows);
        Mat imgHSV;
        imshow("OG",imgOriginal);

        cvtColor(imgOriginal, imgHSV, COLOR_BGR2HSV);

        Mat imgThreshold;
        Mat imgThreshold1;
        
        //imgThreshold=imgHSV;
        //inRange(imgHSV, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH,iHighS , iHighV), imgThreshold);
        inRange(imgHSV, Scalar(0, 98, 23), Scalar(5, 255, 255), imgThreshold);
        inRange(imgHSV, Scalar(155, 98, 23), Scalar(255, 255, 255), imgThreshold1);
        cv::bitwise_or(imgThreshold,imgThreshold1,imgThreshold);
    
        erode(imgThreshold, imgThreshold, getStructuringElement(MORPH_ELLIPSE, Size(5,5)));
        dilate(imgThreshold, imgThreshold, getStructuringElement(MORPH_ELLIPSE, Size(5,5)));

        dilate(imgThreshold, imgThreshold, getStructuringElement(MORPH_ELLIPSE, Size(5,5)));
        erode(imgThreshold, imgThreshold, getStructuringElement(MORPH_ELLIPSE, Size(5,5)));
        //cvtColor(imgThreshold,IMGBGR, CV_HSV2BGR);
        //cvtColor(imgThreshold,gray,CV_BGR2GRAY);
        GaussianBlur(gray,gray,Size(9,9),2,2);
        GaussianBlur(imgThreshold,imgThreshold,Size(9,9),2,2);
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
            if(minEllipse[i].size.width>20)
            {

                Scalar color= Scalar(rng.uniform(0,255),rng.uniform(0,255),rng.uniform(0,255));

                drawContours(drawing,contours,i,color,1,8,vector<Vec4i>(),0,Point());
                ellipse(drawing,minEllipse[i],color,2,8);

                Point2f rect_points[4]; minRect[i].points(rect_points);
                cout <<"width is :"<< minEllipse[i].size.width<<"height is"<<minEllipse[i].size.height<<endl;
                cout << "center position in x is :"<< minEllipse[i].center.x<<endl;
                float r=(float)minEllipse[i].size.height;
                cout << "distance should be:" <<(int)(-6.405*0.0000001*r*r*r+0.001389*r*r-0.9314*r+226.1)<<endl;
                if(minEllipse[i].center.x<280)
                {
                    cout <<"turn left"<<endl;
                }
                else if(minEllipse[i].center.x>360)
                {
                    cout << "turn right"<<endl;
                }
                for(int k=0;k<4;k++)
                {
                    line(drawing,rect_points[k],rect_points[(k+1)%4],color,1,8);
                }
            }
        }
        imshow("Contours", drawing);

        if(waitKey(30)==27)
        {
            cout <<"esc key is pressed" <<endl;
            break; 
        }

    }
    return 0;
}


int main()
{
    super_cam();
    return 0;
}

