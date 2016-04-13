#include <stdio.h>
#include <unistd.h>


#define DUTY0DIR "/sys/devices/platform/pwm-ctrl/duty0"
#define FREQ0DIR "/sys/devices/platform/pwm-ctrl/freq0"
#define ENAB0DIR "/sys/devices/platform/pwm-ctrl/enable0"

#define DUTY1DIR "/sys/devices/platform/pwm-ctrl/duty1"
#define FREQ1DIR "/sys/devices/platform/pwm-ctrl/freq1"
#define ENAB1DIR "/sys/devices/platform/pwm-ctrl/enable1"

int pwm_init(int freq0,int freq1)
{
    FILE * f0=fopen(FREQ0DIR,"w");
    FILE * f1=fopen(FREQ1DIR,"w");
    fwrite(&freq0,sizeof(int),1,f0);
    fwrite(&freq1,sizeof(int),1,f1);
    fclose(f0);
    fclose(f1);
}

int pwm_move(int time,int pwm0,int pwm1)
{
    FILE * d0=fopen(DUTY0DIR,"w");
    FILE * d1=fopen(DUTY1DIR,"w");
    FILE * e0=fopen(ENAB0DIR,"w");
    FILE * e1=fopen(ENAB1DIR,"w");
    int a=1;
    fwrite(&pwm0,sizeof(int),1,d0);
    fwrite(&pwm1,sizeof(int),1,d1);
    fwrite(&a,sizeof(int),1,e0);
    fwrite(&a,sizeof(int),1,e1);
    fclose(d0);
    fclose(d1);
    fclose(e0);
    fclose(e1);
    usleep(time);
    e0=fopen(ENAB0DIR,"w");
    e1=fopen(ENAB1DIR,"w");
    fwrite(&a,sizeof(int),1,e0);
    fwrite(&a,sizeof(int),1,e1);
    fclose(e0);
    fclose(e1);
    return 0;
}


int main()
{
    int i=pwm_init(2000,2000);
    while(1)
    {
        int b=pwm_move(1000000,512,512);
    }
    return 0;
}
