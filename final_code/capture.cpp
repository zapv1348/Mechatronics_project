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
#include <pthread.h>
#include <wiringPi.h>


#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>

#define DUTY0DIR "/sys/devices/platform/pwm-ctrl/duty0"
#define FREQ0DIR "/sys/devices/platform/pwm-ctrl/freq0"
#define ENAB0DIR "/sys/devices/platform/pwm-ctrl/enable0"

#define DUTY1DIR "/sys/devices/platform/pwm-ctrl/duty1"
#define FREQ1DIR "/sys/devices/platform/pwm-ctrl/freq1"
#define ENAB1DIR "/sys/devices/platform/pwm-ctrl/enable1"

using namespace cv;
using namespace std;

#define HRES 640
#define VRES 480


/* chart of Wiring PIN number and actual pin number
   WIRING| ACTUAL
   0   |       11
   1   |       12
   2   |       13
   3   |       15
   4   |       16
   5   |       18
   6   |       22
   7   |       7
   10  |       24
   11  |       26
   12  |       19 pwm
   13  |       21
   14  |       23
   21  |       29
   22  |       31
   23  |       33 pwm
   24  |       35
   26  |       32
   27  |       36
   ADC0|       40
   ADC1|       37
 */
#define MOTOENAB 5
#define MOTO0DIR0 6
#define MOTO0DIR1 14
#define MOTO1DIR0 13
#define MOTO1DIR1 24
#define FANPIN 10
#define CANPIN 11
#define IR1_PIN 22
#define IR2_PIN 21
#define LINE1_PIN 0

//#define PRINTING

pthread_mutex_t IR1_mutex;
pthread_mutex_t IR2_mutex;

static volatile int ir1=0;
static volatile int ir2=0;


void IR1_ISR(void)
{
    //pthread_mutex_lock(&IR1_mutex);
    ir1++;
    //pthread_mutex_unlock(&IR1_mutex);
}

void IR2_ISR(void)
{
    //pthread_mutex_lock(&IR2_mutex);
    ir2++;
    //pthread_mutex_unlock(&IR2_mutex);
}

#ifdef PRINTING
    RNG rng(12345);
#endif


int pwm_init(int freq0, int freq1, int pwm0, int pwm1)
{
    FILE * f0=fopen(FREQ0DIR,"w");
    FILE * f1=fopen(FREQ1DIR,"w");
    FILE * d0=fopen(DUTY0DIR,"w");
    FILE * d1=fopen(DUTY1DIR,"w");
    fprintf(f0,"%d",freq0);
    fprintf(f0,"%d",freq1);
    fprintf(d0,"%d",pwm0);
    fprintf(d1,"%d",pwm1);
    fclose(f0);
    fclose(f1);
    fclose(d0);
    fclose(d1);

    pinMode(MOTOENAB,OUTPUT);
    digitalWrite(MOTOENAB,0);

    FILE * e0=fopen(ENAB0DIR,"w");
    FILE * e1=fopen(ENAB1DIR,"w");
    fprintf(e0,"%d",1);
    fprintf(e1,"%d",1);
    fclose(e0);
    fclose(e1);
    return 0;
}
//takes an angle and a distance 
int pwm_move(int angle, int dist)
{
    struct timespec time1;
    int a;
    if(angle<0)
    {
        //change dir to counterclockwise
        //turn left
        digitalWrite(MOTO0DIR0,0);
        digitalWrite(MOTO0DIR1,1);
        digitalWrite(MOTO1DIR0,1);
        digitalWrite(MOTO1DIR1,0);
        a=-angle;
        //convert angle to time
    }
    else if(angle >0)
    {
        //change dir to clockwise
        //right
        digitalWrite(MOTO0DIR0,1);
        digitalWrite(MOTO0DIR1,0);
        digitalWrite(MOTO1DIR0,0);
        digitalWrite(MOTO1DIR1,1);
        a=angle;
        //convert angle to time
    }
    float r=(float)a;
    float time5=(float)(r*r*r*2.89*0.0000001-r*r*7.533*0.00001+r*0.01193+0.0504);
    time1.tv_sec=(int)time5;
    time5=time5-time1.tv_sec;
    time1.tv_nsec=(int)(time5*1000000000);
    digitalWrite(MOTOENAB,1);
    nanosleep(&time1,NULL);
    digitalWrite(MOTOENAB,0);
    if(dist<0)
    {
        //change dir to backwards
        digitalWrite(MOTO0DIR0,0);
        digitalWrite(MOTO0DIR1,1);
        digitalWrite(MOTO1DIR0,0);
        digitalWrite(MOTO1DIR1,1);
        a=-dist;
        //convert distance to time
    }
    else if(dist >0)
    {
        //change dir to forwards
        digitalWrite(MOTO0DIR0,1);
        digitalWrite(MOTO0DIR1,0);
        digitalWrite(MOTO1DIR0,1);
        digitalWrite(MOTO1DIR1,0);
        a=dist;
        //convert dist to time
    }
    r=(float)a;
    time5=(float)(r*r*r*2.3*0.00001-r*r*0.00158+r*0.0525+0.0378);
    time1.tv_sec=(int)time5;
    time5=time5-time1.tv_sec;
    time1.tv_nsec=(int)(time5*1000000000);
    digitalWrite(MOTOENAB,1);
    nanosleep(&time1,NULL);
    digitalWrite(MOTOENAB,0);

    return 0;
}



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

int super_cam(int cord, int frames)
{
    VideoCapture cap(0);
    cap.set(CV_CAP_PROP_FRAME_WIDTH,HRES);
    cap.set(CV_CAP_PROP_FRAME_HEIGHT,VRES);
    if( !cap.isOpened())
    {
#ifdef PRINTING
        cout << "cannot open webcam" <<endl;
#endif
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
#ifdef PRINTING
    namedWindow("OG", CV_WINDOW_AUTOSIZE);
    namedWindow("Threshold", CV_WINDOW_AUTOSIZE);
#endif
    int n=0;
    int cent_avg=0;
    int height_avg=0;
    int l=0;
    while(n<frames)
    {
        Mat imgOriginal;

        bool bSuccess = cap.read(imgOriginal);
        if (!bSuccess)
        {
#ifdef PRINTING
            cout << "Cannot read a grame from stream" <<endl;
            break;
#endif
        }
        //string ty=type2str(imgOriginal.type());
        //printf("Matrix: %s %dx%d \n",ty.c_str(),imgOriginal.cols,imgOriginal.rows);
        Mat imgHSV;
#ifdef PRINTING
        imshow("OG",imgOriginal);
#endif
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

#ifdef PRINTING
        imshow("Threshold",imgThreshold);
#endif

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
        
#ifdef PRINTING
        Mat drawing=Mat::zeros(imgThreshold.size(), CV_8UC3);
#endif
        for(int i=0; i<contours.size();i++)
        {
            if(minEllipse[i].size.width>40)
            {
#ifdef PRINTING
                Scalar color= Scalar(rng.uniform(0,255),rng.uniform(0,255),rng.uniform(0,255));

                drawContours(drawing,contours,i,color,1,8,vector<Vec4i>(),0,Point());
                ellipse(drawing,minEllipse[i],color,2,8);

                Point2f rect_points[4]; minRect[i].points(rect_points);
#endif
                cent_avg=(cent_avg*l+minEllipse[i].center.x)/(l+1);
                height_avg=(height_avg*l+minEllipse[i].size.height)/(l+1);
                l++;
#ifdef PRINTING
                cout <<"width is :"<< minEllipse[i].size.width<<"height is"<<minEllipse[i].size.height<<endl;
                  cout << "center width is :"<< minEllipse[i].center.x<<endl;
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
#endif
            }
        }
#ifdef PRINTING
        imshow("Contours", drawing);

          if(waitKey(30)==27)
          {
          cout <<"esc key is pressed" <<endl;
          break; 
          }
#endif
        n++;
    }
    if(cord==0)
    {
        return cent_avg;
    }
    else
    {
        return height_avg;
    }
}

int find_hoop()
{
    struct timespec t1,t2;
    int a=0;
    int angle=0;
    int distance=0;
    while(a>340||a<300)
    {
        a=super_cam(0,5);
#ifdef PRINTING
        cout << "cent avg was" << a <<endl;
#endif
        if(a==0)
        {
            pwm_move(0,5);
            //turn right a set amount make sure to add it to t2
            angle+=5;
        }
        else if(a>360)
        {
            //turn right 5 deg
            angle-=1;
            //convert a angle
            pwm_move(0,-1);
        }
        else
        {
            //turn left 5 deg
            //convert a to t1 and add to t2
            pwm_move(0,1);
            angle+=1;
        }
    }
    a=0;
    float r;
    while(distance<40||distance>72)
    //while(1)
    {
        a=super_cam(1,5);
        r=(float)a;
        distance=(int)(-6.405*0.0000001*r*r*r+0.001389*r*r-0.9314*r+226.1);
#ifdef PRINTING
        cout <<"distance was" << distance << endl;
#endif
        if(distance>54)
        {
            //forward move
            distance=distance-56;
            //convert distance to t1
            pwm_move(1,0);
        }
        else if(distance<36)
        {
            //move back
            distance=38-distance;
            //convert distance to t1
            pwm_move(-1,0);
        }
    }
    return angle;
}

void shoot()
{
    digitalWrite(CANPIN,HIGH);
    sleep(10);
    digitalWrite(CANPIN,LOW);
    return;
}
void gpio_init()
{
    pinMode(MOTO0DIR0,OUTPUT);
    pinMode(MOTO0DIR1,OUTPUT);
    pinMode(MOTO1DIR0,OUTPUT);
    pinMode(MOTO1DIR1,OUTPUT);
    pinMode(MOTOENAB,OUTPUT);
    pinMode(FANPIN,OUTPUT);
    pinMode(CANPIN,OUTPUT);
    //pinMode(IR1_PIN,INPUT);
    //pinMode(IR2_PIN,INPUT);
    pinMode(LINE1_PIN,INPUT);
    //wiringPiISR(IR1_PIN,INT_EDGE_BOTH,&IR1_ISR);
    //wiringPiISR(IR2_PIN,INT_EDGE_BOTH,&IR2_ISR);
}

int detect_freq1()
{
    struct timespec a={0,0};
    struct timespec b={0,0};
    //pthread_mutex_lock(&IR1_mutex);
    ir1=0;
    //pthread_mutex_unlock(&IR1_mutex);
    clock_gettime(CLOCK_REALTIME,&a);
    //pthread_mutex_lock(&IR1_mutex);
    while(ir1<10&&b.tv_sec<1)
    {
        //pthread_mutex_unlock(&IR1_mutex);
        clock_gettime(CLOCK_REALTIME,&b);
        b.tv_sec=b.tv_sec-a.tv_sec;
        //pthread_mutex_lock(&IR1_mutex);
    }
    //pthread_mutex_unlock(&IR1_mutex);
    clock_gettime(CLOCK_REALTIME,&b);
    return (int)(5.0/((float)(a.tv_sec-b.tv_sec)+(float)(a.tv_nsec-b.tv_nsec)/(1000000000.0)));
}

int detect_freq2()
{
    struct timespec a={0,0};
    struct timespec b={0,0};
    //pthread_mutex_lock(&IR2_mutex);
    ir2=0;
    //pthread_mutex_unlock(&IR2_mutex);
    clock_gettime(CLOCK_REALTIME,&a);
    //pthread_mutex_lock(&IR2_mutex);
    while(ir2<10&&b.tv_sec<1)
    {
        //pthread_mutex_unlock(&IR2_mutex);
        clock_gettime(CLOCK_REALTIME,&b);
        b.tv_sec=b.tv_sec-a.tv_sec;
        //pthread_mutex_lock(&IR2_mutex);

    }
    //pthread_mutex_unlock(&IR2_mutex);
    clock_gettime(CLOCK_REALTIME,&b);
    return (int)(5.0/((float)(a.tv_sec-b.tv_sec)+(float)(a.tv_nsec-b.tv_nsec)/(1000000000.0)));
}


int main()
{
    wiringPiSetup();
    pwm_init(980,980,602,602);    
    gpio_init();
    int b;
    int freq1=0;//left side
    int freq2=0;//right side
    while(1)
    {
        /*freq1=detect_freq1();
        freq2=detect_freq2();
        //point us to the center
        while(1)
        {
            if((400<freq1)&(400<freq2)&(freq1<600)&(freq2<600))
            {
                //printf("freq 1 is: %d, freq2 is: %d\n",freq1,freq2);
                break;
            }
            else if(400<freq1<600)
            {
                pwm_move(0,-1);//turn left   
            }
            else if(400<freq2<600)
            {
                pwm_move(0,1);
                //turn right
            }
            else
            {
                pwm_move(0,5);
                printf("neither frequency found\n");
                //turn
            }
            freq1=detect_freq1();
            freq2=detect_freq2();
            //printf("looking for frequencies\n");
        }
       //printf("are we here\n"); 
         */
        while(analogRead(LINE1_PIN)<850)
        {
            //move forward
            pwm_move(1,0);
        }
        while(analogRead(LINE1_PIN)>850)
        {
            pwm_move(1,0);
        }
        digitalWrite(FANPIN,HIGH);
        sleep(1);
        pwm_move(2,20);
        sleep(1);
        pwm_move(-2,-20);
        sleep(1);
        digitalWrite(FANPIN,LOW);
        b=find_hoop();//returns the amount of time it turned.
        shoot();
        pwm_move(0,-(b%360));
    }
    return 0;
}

