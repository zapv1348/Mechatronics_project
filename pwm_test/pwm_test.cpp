#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <time.h>


#define DUTY0DIR "/sys/devices/platform/pwm-ctrl/duty0"
#define FREQ0DIR "/sys/devices/platform/pwm-ctrl/freq0"
#define ENAB0DIR "/sys/devices/platform/pwm-ctrl/enable0"

#define DUTY1DIR "/sys/devices/platform/pwm-ctrl/duty1"
#define FREQ1DIR "/sys/devices/platform/pwm-ctrl/freq1"
#define ENAB1DIR "/sys/devices/platform/pwm-ctrl/enable1"

using namespace std;

int pwm_init(int freq0,int freq1)
{
    FILE * f0=fopen(FREQ0DIR,"w");
    FILE * f1=fopen(FREQ1DIR,"w");


    fprintf(f0,"%d",freq0);
    fprintf(f1,"%d",freq1);
    fclose(f0);
    fclose(f1);
}

int pwm_move(struct timespec time,int pwm0,int pwm1)
{
    FILE * d0=fopen(DUTY0DIR,"w");
    FILE * d1=fopen(DUTY1DIR,"w");
    FILE * e0=fopen(ENAB0DIR,"w");
    FILE * e1=fopen(ENAB1DIR,"w");
    fprintf(d0,"%d",pwm0);
    fprintf(d1,"%d",pwm1);
    fprintf(e0,"%d",1);
    fprintf(e1,"%d",1);
    fclose(d0);
    fclose(d1);
    fclose(e0);
    fclose(e1);
    nanosleep(&time,NULL);
    e0=fopen(ENAB0DIR,"w");
    e1=fopen(ENAB1DIR,"w");
    fprintf(e0,"%d",0);
    fprintf(e1,"%d",0);
    fclose(e0);
    fclose(e1);
    return 0;
}


int main()
{
    struct timespec a;
    a.tv_sec=5;
    a.tv_nsec=0;
    int i=pwm_init(2000,2000);
    int b=pwm_move(a,512,512);
    return 0;
}
