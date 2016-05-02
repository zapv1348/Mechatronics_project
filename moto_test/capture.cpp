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

#define PRINTING


int pwm_init(int freq0, int freq1, int pwm0, int pwm1)
{
    FILE * f0=fopen(FREQ0DIR,"w");
    FILE * f1=fopen(FREQ1DIR,"w");
    FILE * d0=fopen(DUTY0DIR,"w");
    FILE * d1=fopen(DUTY1DIR,"w");
    fprintf(f0,"%d",freq0);
    fprintf(f1,"%d",freq1);
    fprintf(d0,"%d",pwm0);
    fprintf(d1,"%d",pwm1);
    fclose(f0);
    fclose(f1);
    fclose(d0);
    fclose(d1);

    pinMode(MOTOENAB,OUTPUT);
    digitalWrite(MOTOENAB,1);

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
        digitalWrite(MOTO0DIR0,LOW);
        digitalWrite(MOTO0DIR1,HIGH);
        digitalWrite(MOTO1DIR0,HIGH);
        digitalWrite(MOTO1DIR1,LOW);
        a=-angle;
        //convert angle to time
    }
    else if(angle >0)
    {
        //change dir to clockwise
        digitalWrite(MOTO0DIR0,HIGH);
        digitalWrite(MOTO0DIR1,LOW);
        digitalWrite(MOTO1DIR0,LOW);
        digitalWrite(MOTO1DIR1,HIGH);
        a=angle;
        //convert angle to time
    }
    float r=(float)a;
    float time5=(float)(r*r*r*2.89*0.0000001-r*r*7.533*0.00001+r*0.01193+0.0504);
    time1.tv_sec=(int)time5;
    time5=time5-time1.tv_sec;
    time1.tv_nsec=(int)(time5*1000000000);
    digitalWrite(MOTOENAB,0);
    nanosleep(&time1,NULL);
    digitalWrite(MOTOENAB,1);
    if(dist<0)
    {
        //change dir to backwards
        digitalWrite(MOTO0DIR0,LOW);
        digitalWrite(MOTO0DIR1,HIGH);
        digitalWrite(MOTO1DIR0,LOW);
        digitalWrite(MOTO1DIR1,HIGH);
        a=-dist;
        //convert distance to time
    }
    else if(dist >0)
    {
        //change dir to forwards
        digitalWrite(MOTO0DIR0,HIGH);
        digitalWrite(MOTO0DIR1,LOW);
        digitalWrite(MOTO1DIR0,HIGH);
        digitalWrite(MOTO1DIR1,LOW);
        a=dist;
        //convert dist to time
    }
    r=(float)a;
    time5=(float)(r*r*r*2.3*0.00001-r*r*0.00158+r*0.0525+0.0378);
    time1.tv_sec=(int)time5;
    time5=time5-time1.tv_sec;
    time1.tv_nsec=(int)(time5*1000000000);
    digitalWrite(MOTOENAB,0);
    nanosleep(&time1,NULL);
    digitalWrite(MOTOENAB,1);

    return 0;
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
    pinMode(IR1_PIN,INPUT);
    pinMode(IR2_PIN,INPUT);
    pinMode(LINE1_PIN,INPUT);
}




int main()
{
    wiringPiSetup();
    pwm_init(980,980,602,602);    
    gpio_init();
    int b;
    while(1)
    {
        pwm_move(10,0); 
        //point us to the center
        sleep(1);
        pwm_move(-10,0);
        sleep(1);
        pwm_move(0,-20);
        sleep(1);
        pwm_move(0,20);
        sleep(1);
    }
    return 0;
}

