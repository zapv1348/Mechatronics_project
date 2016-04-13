#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <stddef.h>
#include <time.h>
#include <math.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#include <iostream>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

#define HRES 1280
#define VRES 720

#define UARTPORT "/dev/ttyS2"

int iLowH[3]={0,0,0};//low hue values for ground, red hoop, then blue hoop
int iHighH[3]={255,255,255};

int iLowS[3]={0,0,0};//same for saturation
int iHighS[3]={255,255,255};

int iLowV[3]={0,0,0};//same for val
int iHighV[3]={255,255,255};

char dist_angle[2];


void look_for(int a, Mat imgOriginal)
{
    vector<Vec3f> circles;
    vector<Vec4i> hierarchy;
    vector<vector<Point> > contours;

    Mat imflood, imgThreshold;


    cvtColor(imgOriginal,imgOriginal,COLOR_BGR2HSV);

    inRange(imgOriginal,Scalar(iLowH[a],iLowS[a],iLowV[a]),Scalar(iHighH[a],iHighS[a],iHighV[a]),imgThreshold);
    erode(imgThreshold,imgThreshold,getStructuringElement(MORPH_ELLIPSE,Size(5,5)));
    dilate(imgThreshold,imgThreshold,getStructuringElement(MORPH_ELLIPSE,Size(5,5)));

    dilate(imgThreshold,imgThreshold,getStructuringElement(MORPH_ELLIPSE,Size(5,5)));
    erode(imgThreshold,imgThreshold,getStructuringElement(MORPH_ELLIPSE,Size(5,5)));

    imflood=imgThreshold.clone();
    floodFill(imflood,cv::Point(0,0),Scalar(255));

    bitwise_not(imflood,imflood);
    imgThreshold=(imflood|imgThreshold);

    findContours(imgThreshold,contours,hierarchy,CV_RETR_TREE,CV_CHAIN_APPROX_SIMPLE,Point(0,0));

    vector <RotatedRect> minEllipse(contours.size());
    vector <RotatedRect> minRect(contours.size());
    for(int i=0; i<contours.size();i++)
    {
        minRect[i]=minAreaRect(Mat(contours[i]));
        if(contours[i].size() > 5)
        {
            minEllipse[i]=fitEllipse(Mat(contours[i]));
        }
    }
    RotatedRect ret_vals=minEllipse[0];
    for(int l=1; l<contours.size();l++)
    {
        if(minEllipse[l].size.width*minEllipse[l].size.height>ret_vals.size.width*ret_vals.size.height)
        {
            ret_vals=minEllipse[l];
        }
    }
    //now calculate distance and center



}

int set_interface_attribs (int fd, int speed, int parity)
{
    struct termios tty;
    memset(&tty,0,sizeof(tty));
    if(tcgetattr(fd,&tty)!=0)
    {
        printf("error %d from tcgetattr",errno);
        return -1;
    }

    cfsetospeed(&tty,speed);
    cfsetispeed(&tty,speed);

    tty.c_cflag=(tty.c_cflag&~CSIZE)|CS8; //8 bit chars

    tty.c_iflag &= ~IGNBRK; //disable break processing
    tty.c_lflag=0;      //no signaling chars, no echo, no cannonical processing

    tty.c_oflag=0;      //no remapping, no delays
    tty.c_cc[VMIN]=1;   //read does block
    tty.c_cc[VTIME]=5;  //0.5 seconds read timeout

    tty.c_iflag &= ~(IXON |IXOFF|IXANY); //shut off xon/xoff ctrl
    tty.c_cflag |= (CLOCAL |CREAD); //ignore modem controls, allow read

    tty.c_cflag &=~(PARENB|PARODD); //shutoff parity
    tty.c_cflag |= parity;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &- ~CRTSCTS;

    if(tcsetattr(fd,TCSANOW,&tty)!=0)
    {
        printf("error %d from tcsetattr",errno);
        return -1;
    }
    return 0;
}


void set_blocking(int fd, int should_block)
{
    struct termios tty;
    memset( &tty, 0, sizeof(tty));
    if(tcgetattr(fd,&tty)!=0)
    {
        printf("err %d from tggetattr",errno);
        return;
    }
    tty.c_cc[VMIN]=should_block ? 1:0; //should it block or not
    tty.c_cc[VTIME]=5; //0.5 seconds read timeout

    if(tcsetattr(fd,TCSANOW, &tty)!=0)
    {
        printf("error %d setting term attributes",errno);
    }
}


int main()
{
    VideoCapture capture(0);
    Mat imgOriginal;
    int fd=open(UARTPORT, O_RDWR|O_NOCTTY|O_SYNC);
    if(fd<0)
    {
        printf("error %d opening %s: %s",errno, UARTPORT, strerror(errno));
        return -1;
    }
    set_interface_attribs(fd, B115200, 0);
    set_blocking(fd,0);

    char buff[10];
    uint8_t empty[2]={0,0};
    while(1)
    {
        capture.read(imgOriginal);
        int n=read(fd,buff, 1);
        if(buff[0]=='R')
        {
           look_for(1,imgOriginal);//look for red hoop
           write(fd,dist_angle,2);
        }
        else if(buff[0]=='B')
        {
           look_for(2,imgOriginal);//look for blue hoop
           write(fd,dist_angle,2);
        }
        else if(buff[0]=='G')
        {
           look_for(0,imgOriginal);//look for ground circle
           write(fd,dist_angle,2);
        }
        else
        {
           write(fd,empty,2);
        }
        
    }
    return 0;
}
