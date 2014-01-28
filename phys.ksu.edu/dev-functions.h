#define rw(serialNum,src,dest) deviceRW(serialNum, src[0], src[1], buff, (int) sizeof(src[0]))

#define MAX_DEVICES 256
#define MAX_CHANNELS 4
#define CONVERT_TIME 1

typedef struct {
    unsigned char SN[8];
    unsigned int configError:  1;
    unsigned int convertError: 1;
    unsigned int alarmError:   1;
    time_t convertTime;
    int channels;
    long successes, tries;
} owDevice;

void setupDevice (owDevice *dev);
void doConversion (owDevice *dev);
int readDevice (owDevice *dev, float *data);
int makeDeviceList (owDevice *devList);
void alarmSearch (owDevice *devList, long numDevices);
void errPrintDet (unsigned char *serialNum);
void printSuccessRate (owDevice *devList, long numDevices);
