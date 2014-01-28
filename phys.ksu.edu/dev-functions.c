#include <stdio.h>
#include <time.h>
#include "ow-functions.h"
#include "dev-functions.h"

#define MAX_TIME_STRING 80

// Family codes
#define FIXED_RES_THERMOMETER 0x10
#define A_TO_D_CONVERTER 0x20
#define ECONO_THERMOMETER 0x22
#define PROG_RES_THERMOMETER 0x28

// ROM commands
#define SEARCH_ROM_CMD 0xF0
#define READ_ROM_CMD 0x33
#define MATCH_ROM_CMD 0x55
#define SKIP_ROM_CMD 0xCC
#define ALARM_SEARCH_CMD 0xEC
#define RESUME_CMD 0xA5
#define OVERDRIVE_SKIP_ROM_CMD 0x3C
#define OVERDRIVE_MATCH_ROM_CMD 0x69

// Thermometer commands
#define CONVERT_T_CMD 0x44
#define WRITE_SCRATCHPAD_CMD 0x4E
#define READ_SCRATCHPAD_CMD 0xBE
#define COPY_SCRATCHPAD_CMD 0x48
#define RECALL_E2_CMD 0xB8
#define READ_POWER_SUPPLY_CMD 0xB4

// A-to-D converter commands
#define READ_MEMORY_CMD 0xAA
#define WRITE_MEMORY_CMD 0x55
#define CONVERT_A_TO_D_CMD 0x3C

static void setupFixedResTherm (owDevice *dev);
static void setupProgResTherm (owDevice *dev);
static void doConvertT (owDevice *dev);
static int readTherm (owDevice *dev, float *temp);
static void setupAtoD (owDevice *dev);
static void doConvertAtoD (owDevice *dev);
static int readAtoD (owDevice *dev, float *voltage);
static int deviceRW (unsigned char serialNum[8], unsigned char const *src, unsigned char const *isWriting, unsigned char *dest, int len);
static int checkCRC16 (unsigned char init1, unsigned char init2, unsigned char *ptr, int len);

void setupDevice (owDevice *dev) {
    switch (dev->SN[0]) {
        case FIXED_RES_THERMOMETER:
            setupFixedResTherm (dev);
            return;
        case ECONO_THERMOMETER:
        case PROG_RES_THERMOMETER:
            setupProgResTherm (dev);
            return;
        case A_TO_D_CONVERTER:
            setupAtoD (dev);
            return;
    }
}

void doConversion (owDevice *dev) {
    switch (dev->SN[0]) {
        case FIXED_RES_THERMOMETER:
        case ECONO_THERMOMETER:
        case PROG_RES_THERMOMETER:
            doConvertT (dev);
            return;
        case A_TO_D_CONVERTER:
            doConvertAtoD (dev);
            return;
    }
}

int readDevice (owDevice *dev, float *data) {
    switch (dev->SN[0]) {
        case FIXED_RES_THERMOMETER:
        case ECONO_THERMOMETER:
        case PROG_RES_THERMOMETER:
            return readTherm (dev, data);
        case A_TO_D_CONVERTER:
            return readAtoD (dev, data);
        default:
            return -1;
    }
}

int makeDeviceList (owDevice *devList) {
    unsigned char serialNum[MAX_DEVICES][8];
    long numDevices, n;

    numDevices = findDevices (SEARCH_ROM_CMD, serialNum, MAX_DEVICES);
    if (numDevices <= 0) {
        return -1;
    }

    for (n = 0; n < numDevices; n++) {
        int i;
        for (i = 0; i < 8; i++) {
            devList[n].SN[i] = serialNum[n][i];
        }
        devList[n].configError = 1;
        devList[n].convertError = 1;
        devList[n].alarmError = 1;
        switch (serialNum[n][0]) {
            case FIXED_RES_THERMOMETER:
            case ECONO_THERMOMETER:
            case PROG_RES_THERMOMETER:
                devList[n].channels = 1;
            break;
            case A_TO_D_CONVERTER:
                devList[n].channels = 4;
            break;
            default:
                devList[n].channels = 0;
            break;
        }
        devList[n].successes = 0;
        devList[n].tries = 0;
    }

    return numDevices;
}

void alarmSearch (owDevice *devList, long numDevices) {
    unsigned char alarmSN[MAX_DEVICES][8];
    long numAlarming, n;

    numAlarming = findDevices (ALARM_SEARCH_CMD, alarmSN, MAX_DEVICES);
    if (numAlarming < 0) {
        fprintf (stderr, "\n");
        for (n = 0; n < numDevices; n++) {
            devList[n].alarmError = 1;
        }
    } else {
        int i;
        for (i = 0; i < numAlarming; i++) {

            fprintf (stderr, "ERROR: Device is alarming\n");
            errPrintDet (alarmSN[i]);

            for (n = 0; n < numDevices; n++) {
                int j;
                for (j = 0; j < 8; j++) {
                    if (devList[n].SN[j] != alarmSN[i][j]) {
                        break;
                    }
                }
                if (j == 8) {
                    devList[n].alarmError = 1;
                    break;
                }
            }

            if (n >= numDevices) {
                fprintf (stderr, "ERROR: Alarming device not found on original list\n");
            }

            fprintf (stderr, "\n");

        }
    }
}

void errPrintDet (unsigned char *serialNum) {
    char sTime[MAX_TIME_STRING];
    time_t t;
    int i;

    time (&t);
    strftime (sTime, MAX_TIME_STRING, "%c", localtime(&t));

    fprintf (stderr, "  (");
    if (serialNum != NULL) {
        fprintf (stderr, "SN: ");
        for (i = 7; i >= 0; i--) {
            fprintf (stderr, "%02X", serialNum[i]);
        }
        fprintf (stderr, ", ");
    }
    fprintf (stderr, "Time: %s)\n", sTime);

    return;
}

void printSuccessRate (owDevice *devList, long numDevices) {
    long n;
    int i;

    printf ("\nDevice                Success rate\n");
    for (n = 0; n < numDevices; n++) {
        if (devList[n].tries != 0) {
            printf ("  ");
            for (i = 7; i >= 0; i--) {
                printf ("%02X", devList[n].SN[i]);
            }
            printf ("      %ld / %ld\n", devList[n].successes, devList[n].tries);
        }
    }
    printf ("\n");

    return;
}

void setupFixedResTherm (owDevice *dev) {
    const char *errAction = "Error occurred during fixed-resolution thermometer setup";
    unsigned char const writeScratchPad[2][3]
        = { {WRITE_SCRATCHPAD_CMD,   // WRITE SCRATCHPAD command
             0x7F,                   // Max Temp = +127 C
             0x80},                  // Min Temp = -128 C
            {1, 1, 1} };

    unsigned char buff[sizeof(writeScratchPad[0]) + 9];

    // Send command
    if ( rw(dev->SN, writeScratchPad, buff) < 0 ) {
        fprintf (stderr, "%s\n", errAction);
        errPrintDet (dev->SN);
        fprintf (stderr, "\n");
        dev->alarmError = 1;
        return;
    }

    dev->configError = 0;

    // Check thermometer
    if (readTherm (dev, NULL) < 0) {
        fprintf (stderr, "%s\n", errAction);
        fprintf (stderr, "\n");
    }
    return;
}

void setupProgResTherm (owDevice *dev) {
    const char *errAction = "Error occurred during programmable-resolution thermometer setup";
    unsigned char const writeScratchPad[2][4]
        = { {WRITE_SCRATCHPAD_CMD,   // WRITE SCRATCHPAD command
             0x7F,                   // Max Temp = +127 C
             0x80,                   // Min Temp = -128 C
             0x7F},                  // 12-bit resolution
            {1, 1, 1, 1} };

    unsigned char buff[sizeof(writeScratchPad[0]) + 9];

    // Send command
    if ( rw(dev->SN, writeScratchPad, buff) < 0 ) {
        fprintf (stderr, "%s\n", errAction);
        errPrintDet (dev->SN);
        fprintf (stderr, "\n");
        dev->configError = 1;
        dev->alarmError = 1;
        return;
    }

    // Check thermometer
    if (readTherm (dev, NULL) < 0) {
        fprintf (stderr, "%s\n", errAction);
        fprintf (stderr, "\n");
    }
    return;
}

void doConvertT (owDevice *dev) {
    const char *errAction = "Error occurred while starting temperature conversion";
    unsigned char const convertT[2][2]
        = { {CONVERT_T_CMD,          // CONVERT T command
             0xFF},                  // Read slots for busy signal
            {1, 0} };

    unsigned char buff[sizeof(convertT[0]) + 9];

    // Send command
    if ( rw(dev->SN, convertT, buff) < 0 ) {
        goto failure;
    }

    // Record time
    time ( &(dev->convertTime) );

    if (buff[10] == 0xFF) {
        fprintf (stderr, "ERROR: No busy signal from thermometer after CONVERT T command\n");
        goto failure;
    } else if (buff[10] != 0) {
        fprintf (stderr, "ERROR: Inconsistent busy signal from thermometer after CONVERT T command:  %02X\n", buff[10]);
        fprintf (stderr, "%s\n", errAction);
        errPrintDet (dev->SN);
        fprintf (stderr, "\n");
    }

    dev->convertError = 0;
    return;

    failure: {
        fprintf (stderr, "%s\n", errAction);
        errPrintDet (dev->SN);
        fprintf (stderr, "\n");
        dev->convertError = 1;
        return;
    }
}

int readTherm (owDevice *dev, float *temp) {
    const char *errAction = "Error occurred during thermometer readout";
    unsigned char const readScratchpad[2][10]
        = { {READ_SCRATCHPAD_CMD,    // READ SCRATCHPAD command
             0xFF, 0xFF, 0xFF, 0xFF, // Read slots for scratchpad data
             0xFF, 0xFF, 0xFF, 0xFF,
             0xFF},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0} };

    unsigned char buff[sizeof(readScratchpad[0]) + 9], *scratchpad;
    int goodData = 1;
    int anyData, i;
    signed long tempInt;
    unsigned int resolution = 9;

    // Send READ SCRATCHPAD command
    if ( rw(dev->SN, readScratchpad, buff) < 0 ) {
        fprintf (stderr, "%s\n", errAction);
        errPrintDet (dev->SN);
        return -1;
    }
    scratchpad = buff + 10;

    // Check whether any data was read
    anyData = 0;
    for (i = 0; i < 9; i++) {
        if (scratchpad[i] != 0xFF) {
            anyData = 1;
            break;
        }
    }
    if (!anyData) {
        fprintf (stderr, "ERROR: All ones read from thermometer\n");
        fprintf (stderr, "%s\n", errAction);
        errPrintDet (dev->SN);
        return -1;
    }

    // Check cyclic redundancy code
    goodData &= (checkCRC8 (scratchpad, 8) >= 0);

    // Check consistency of temperature sign
    if (dev->SN[0] == FIXED_RES_THERMOMETER) {
        if (scratchpad[1] != 0x00 && scratchpad[1] != 0xFF) {
            fprintf (stderr, "ERROR: Sign bits inconsistent\n");
            goodData = 0;
        }
    } else {
        if ((scratchpad[1] & 0xF8) != 0x00 && (scratchpad[1] & 0xF8) != 0xF8) {
            fprintf (stderr, "ERROR: Sign bits inconsistent\n");
            goodData = 0;
        }
    }

    // Check unused bits of configuration byte (if programmable-resolution)
    if (dev->SN[0] != FIXED_RES_THERMOMETER && (scratchpad[4] & 0x9F) != 0x1F) {
        fprintf (stderr, "ERROR: Format of configuration register wrong\n");
        goodData = 0;
    }

    if (dev->SN[0] == FIXED_RES_THERMOMETER) {
        // Check whether extended precision bits are in range
        if (scratchpad[6] == 0 || scratchpad[6] > 16) {
            fprintf (stderr, "ERROR: COUNT REMAIN out of range\n");
            goodData = 0;
        } else {
            // Check consistency of extended-precision temperature with last bit of temperature
            if ( (scratchpad[6] < 9) != (scratchpad[0] & 1) ) {
                fprintf (stderr, "ERROR: COUNT REMAIN and Temperature inconsistent\n");
                goodData = 0;
            }
        }

        // Check COUNT PER deg C
        if (scratchpad[7] != 16) {
            fprintf (stderr, "ERROR: COUNT PER deg C is not 16\n");
            goodData = 0;
        }
    }

    if (!goodData) {
        fprintf (stderr, "Data from thermometer:\n ");
        for (i = 0; i < 9; i++) {
            fprintf (stderr, " %02X", scratchpad[i]);
        }
        fprintf (stderr, "\n");
        fprintf (stderr, "%s\n", errAction);
        errPrintDet (dev->SN);
        return -1;
    }

    // Check alarm settings
    dev->alarmError = 0;
    if (scratchpad[2] != 0x7F) {
        fprintf (stderr, "ERROR: Alarm's maximum temperature setting should be +127 C, but is %+d C\n", (signed int) scratchpad[2]);
        fprintf (stderr, "%s\n", errAction);
        errPrintDet (dev->SN);
        fprintf (stderr, "\n");
        dev->alarmError = 1;
    }
    if (scratchpad[3] != 0x80) {
        fprintf (stderr, "ERROR: Alarm's minimum temperature setting should be -128 C, but is %+d C\n", (signed int) scratchpad[3]);
        fprintf (stderr, "%s\n", errAction);
        errPrintDet (dev->SN);
        fprintf (stderr, "\n");
        dev->alarmError = 1;
    }

    // Check resolution (if programmable-resolution)
    if (dev->SN[0] != FIXED_RES_THERMOMETER) {
        dev->configError = 0;
        resolution = (scratchpad[4] >> 5) + 9;
        if (resolution != 12) {
            fprintf (stderr, "ERROR: Thermometer set to %u-bit resolution instead of 12-bit\n", resolution);
            fprintf (stderr, "%s\n", errAction);
            errPrintDet (dev->SN);
            fprintf (stderr, "\n");
            dev->configError = 1;
        }
    }

    if (temp != NULL) {
        // Convert temperature to human-readable format
        tempInt = scratchpad[1] & 0x7F;
        tempInt <<= 8;
        tempInt |= scratchpad[0];
        if (dev->SN[0] != FIXED_RES_THERMOMETER) {
            tempInt &= 0x7FFF ^ ((1 << (12 - resolution)) - 1);
        }
        if (scratchpad[1] & 0x80) {
            tempInt -= 0x8000;
        }

        if (dev->SN[0] == FIXED_RES_THERMOMETER) {
            *temp = tempInt / 2 - 0.25 + ((float) (16 - scratchpad[6])) / 16;
        } else {
            *temp = (float) tempInt / 16;
        }

        if (*temp == 85) {
            fprintf (stderr, "ERROR: Temperature register contains power-on reset value (85 degrees)\n");
            fprintf (stderr, "%s\n", errAction);
            errPrintDet (dev->SN);
            return -1;
        }

        if (*temp < -55 || *temp > 125) {
            fprintf (stderr, "ERROR: Thermometer's range is -55 C to +125 C, but read %+f\n", *temp);
            fprintf (stderr, "%s\n", errAction);
            errPrintDet (dev->SN);
            fprintf (stderr, "\n");
        }

        dev->successes += 1;
    }

    return 0;
}

void setupAtoD (owDevice *dev) {
    const char *errAction = "Error occurred during A-to-D converter setup";
    unsigned char const writeMemory[2][67]
        = { {WRITE_MEMORY_CMD,       // WRITE MEMORY command
             0x08, 0x00,             // Address of data to write
             0x00, 0xFF, 0xFF, 0xFF, // Data to write followed
             0x01, 0xFF, 0xFF, 0xFF, //    by read slots for
             0x00, 0xFF, 0xFF, 0xFF, //    CRC16 (2 bytes)
             0x01, 0xFF, 0xFF, 0xFF, //    and readback of byte
             0x00, 0xFF, 0xFF, 0xFF, //    written (1 byte)
             0x01, 0xFF, 0xFF, 0xFF,
             0x00, 0xFF, 0xFF, 0xFF,
             0x01, 0xFF, 0xFF, 0xFF,
             0x00, 0xFF, 0xFF, 0xFF, // Page 2
             0xFF, 0xFF, 0xFF, 0xFF,
             0x00, 0xFF, 0xFF, 0xFF,
             0xFF, 0xFF, 0xFF, 0xFF,
             0x00, 0xFF, 0xFF, 0xFF,
             0xFF, 0xFF, 0xFF, 0xFF,
             0x00, 0xFF, 0xFF, 0xFF,
             0xFF, 0xFF, 0xFF, 0xFF},
            {1, 1, 1,
             1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
             1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
             1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
             1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,} };

    unsigned char buff[sizeof(writeMemory[0]) + 9], *ptr;
    int i, anyData, passed;

    // Write settings to converter's memory
    if ( rw(dev->SN, writeMemory, buff) < 0 ) {
        fprintf (stderr, "%s\n", errAction);
        errPrintDet (dev->SN);
        fprintf (stderr, "\n");
        dev->configError = 1;
        dev->alarmError = 1;
        return;
    }
    ptr = buff + 9;

    // Check whether any data was read
    anyData = 0;
    for (i = 3; i < sizeof(writeMemory[0]) && !anyData; i++) {
        if ( (i & 3) != 3 && ptr[i] != 0xFF ) {
            anyData = 1;
        }
    }
    if (!anyData) {
        fprintf (stderr, "ERROR: All ones read from A-to-D converter while writing configuration\n");
        fprintf (stderr, "%s\n", errAction);
        errPrintDet (dev->SN);
        fprintf (stderr, "\n");
        dev->configError = 1;
        dev->alarmError = 1;
        return;
    }

    dev->configError = 0;
    dev->alarmError = 0;

    // Check that data was written okay
    for (i = 0; i < 16; i++, ptr += 4) {
        if (i == 0) {
            passed = (checkCRC16 (0, 0, ptr, 4) >= 0);
            ptr += 3;
        } else {
            passed = (checkCRC16 (8 + i, 0, ptr, 1) >= 0);
        }

        if (!passed) {
            fprintf (stderr, "  (byte %u of A-to-D converter configuration)\n", 8 + i);
            fprintf (stderr, "%s\n", errAction);
            errPrintDet (dev->SN);
            fprintf (stderr, "\n");
            if (i < 8) {
                dev->configError = 1;
            } else {
                dev->alarmError = 1;
            }
        }

        if (ptr[0] != ptr[3]) {
            fprintf (stderr, "ERROR: Wrote %02X but read back %02X\n", ptr[0], ptr[3]);
            fprintf (stderr, "  (byte %u of A-to-D converter configuration)\n", 8 + i);
            fprintf (stderr, "%s\n", errAction);
            errPrintDet (dev->SN);
            fprintf (stderr, "\n");
            if (i < 8) {
                dev->configError = 1;
            } else {
                dev->alarmError = 1;
            }
        }
    }

    return;
}

void doConvertAtoD (owDevice *dev) {
    const char *errAction = "Error occurred while starting A-to-D conversion";
    unsigned char const convertAtoD[2][5]
        = { {CONVERT_A_TO_D_CMD,     // CONVERT command
             0x0F,                   // All channels
             0x00,                   // No preset
             0xFF, 0xFF},            // CRC16
            {1, 1, 1, 0, 0} };

    unsigned char buff[sizeof(convertAtoD[0]) + 9], *ptr;

    dev->convertError = 0;

    // Send CONVERT command
    if ( rw(dev->SN, convertAtoD, buff) < 0 ) {
        goto failure;
    }

    // Record time
    time ( &(dev->convertTime) );

    // Check whether any data was read
    ptr = buff + 9;
    if (ptr[3] == 0xFF && ptr[4] == 0xFF) {
        fprintf (stderr, "ERROR: All ones read from A-to-D converter while sending conversion command\n");
        goto failure;
    }

    // Check CRC
    if ( checkCRC16(0, 0, ptr, 3) < 0 ) {
        goto failure;
    }

    return;

    failure: {
        fprintf (stderr, "%s\n", errAction);
        errPrintDet (dev->SN);
        fprintf (stderr, "\n");
        dev->convertError = 1;
        return;
    }
}

int readAtoD (owDevice *dev, float *voltage) {
    const char *errAction = "Error occurred during A-to-D converter readout";
    unsigned char const readMemory[2][33]
        = { {READ_MEMORY_CMD,        // READ MEMORY command
             0x00, 0x00,             // Memory address
             0xFF, 0xFF, 0xFF, 0xFF, // Memory page 0
             0xFF, 0xFF, 0xFF, 0xFF, //
             0xFF, 0xFF,             // CRC16
             0xFF, 0xFF, 0xFF, 0xFF, // Memory page 1
             0xFF, 0xFF, 0xFF, 0xFF, //
             0xFF, 0xFF,             // CRC16
             0xFF, 0xFF, 0xFF, 0xFF, // Memory page 2
             0xFF, 0xFF, 0xFF, 0xFF, //
             0xFF, 0xFF,},           // CRC16
            {1, 1, 1,
             0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
             0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
             0, 0, 0, 0, 0, 0, 0, 0, 0, 0} };

    unsigned char buff[sizeof(readMemory[0]) + 9], *ptr;
    unsigned short voltInt[4];
    int i, j, anyData, resolution;
    int pageGood[3], ret;

    // Read converter's memory
    if ( rw(dev->SN, readMemory, buff) < 0 ) {
        fprintf (stderr, "%s\n", errAction);
        errPrintDet (dev->SN);
        return -1;
    }
    ptr = buff + 9;

    // Check whether any data was read
    anyData = 0;
    for (i = 3; i < sizeof(readMemory[0]) && !anyData; i++) {
        if (ptr[i] != 0xFF) {
            anyData = 1;
        }
    }
    if (!anyData) {
        fprintf (stderr, "ERROR: All ones read from A-to-D converter while reading voltages\n");
        fprintf (stderr, "%s\n", errAction);
        errPrintDet (dev->SN);
        return -1;
    }

    // Check CRC16 values
    pageGood[0] = (checkCRC16 (0, 0, ptr,     11) >= 0);
    if (!pageGood[0]) {
        fprintf (stderr, " (page 0)\n");
    }
    pageGood[1] = (checkCRC16 (0, 0, ptr + 13, 8) >= 0);
    if (!pageGood[1]) {
        fprintf (stderr, " (page 1)\n");
    }
    pageGood[2] = (checkCRC16 (0, 0, ptr + 23, 8) >= 0);
    if (!pageGood[2]) {
        fprintf (stderr, " (page 2)\n");
    }

    ptr += 3;

    for (i = 0; i < 4; i++) {
        // Check bits on page 1 which should be zero
        if ( (ptr[2*i + 10] & 0x30) || (ptr[2*i + 11] & 0x42) ) {
            fprintf (stderr, "ERROR: Ones read in position where zeros were expected\n");
            pageGood[1] = 0;
        }

        // Check for power failure flag
        if (ptr[2*i + 11] & 0x80) {
            fprintf (stderr, "ERROR: Data lost due to power failure (channel %c)\n", 'A' + i);
            pageGood[0] = 0;
        }
    }

    // Check for changes in settings
    if (pageGood[1]) {
        dev->configError = 0;
        for (i = 0; i < 4; i++) {
            if ( ((ptr[2*i + 10] & 0xCF) != 0) || ((ptr[2*i + 11] & 0x4D) != 1) ) {
                fprintf (stderr, "ERROR: Settings changed for channel %c\n", 'A' + i);
                dev->configError = 1;
            }
        }
    }

    if (pageGood[1] && pageGood[2]) {
        dev->alarmError = 0;
        for (i = 0; i < 4; i++) {

            // Check alarm flags
            if (ptr[2*i + 11] & 0x10) {
                fprintf (stderr, "ERROR: Low voltage alarm flag set (channel %c)\n", 'A' + i);
                dev->alarmError = 1;
            }
            if (ptr[2*i + 11] & 0x20) {
                fprintf (stderr, "ERROR: High voltage alarm flag set (channel %c)\n", 'A' + i);
                dev->alarmError = 1;
            }

            // Check for changes in alarm settings
            if ( (ptr[2*i + 20] != 0) || (ptr[2*i + 21] != 0xFF) ) {
                fprintf (stderr, "ERROR: Alarm settings changed for channel %c\n", 'A' + i);
                dev->alarmError = 1;
            }

        }
    }

    if (pageGood[0] && pageGood[1] && voltage != NULL) {

        // Calculate voltages
        for (i = 0; i < 4; i++) {
            voltInt[i] = ptr[2*i] | ((unsigned short) ptr[2*i + 1] << 8);
            resolution = ptr[2*i + 10] & 0xF;
            if (resolution == 0) {
                resolution = 16;
            }
            voltInt[i] &= (0xFFFF << (16 - resolution));
            if (ptr[2*i + 11] & 0x01) {
                voltage[i] = (float) voltInt[i] / 12800;
            } else {
                voltage[i] = (float) voltInt[i] / 25600;
            }
        }

        dev->successes += 1;
        ret = 0;
    } else {
        ret = -1;
    }

    if (!pageGood[0] || !pageGood[1] || !pageGood[2]) {
        fprintf (stderr, "Data from A-to-D converter:\n");
        for (i = 0; i < 3; i++) {
            fprintf (stderr, " ");
            for (j = 0; j < 10; j++) {
                fprintf (stderr, " %02X", ptr[10 * i + j]);
            }
            fprintf (stderr, "\n");
        }
        fprintf (stderr, "%s\n", errAction);
        errPrintDet (dev->SN);
        if (ret >= 0) {
            fprintf (stderr, "\n");
        }
    }

    return ret;
}

int deviceRW (unsigned char serialNum[8], unsigned char const *src, unsigned char const *isWriting, unsigned char *dest, int len) {
    unsigned char buff[READ_BUFFER_LEN];
    int i, j;
    int ret = 0;

    // MATCH ROM command
    dest[0] = MATCH_ROM_CMD;

    // Serial number
    for (i = 0; i < 8; i++) {
        dest[1 + i] = serialNum[i];
    }

    // Data for device
    for (i = 0; i < len; i++) {
        dest[9 + i] = src[i];
    }

    // Read & Write
    if ( blockRW(dest, buff, 9 + len, 1) < 0 ) {
        return -1;
    }

    // Check writes
    for (i = 0; i < 9 + len; i++) {
        if (i < 9 || isWriting[i - 9]) {
            if (buff[i] != dest[i]) {
                fprintf (stderr, "ERROR: Data on one-wire net did not match data being written:\n  Written:");
                while (i >= 0 && (i < 9 || isWriting[i - 9])) {
                    i--;
                }
                for (j = i + 1; j < 9 + len && (j < 9 || isWriting[j - 9]); j++) {
                    fprintf (stderr, " %02X", dest[j]);
                }
                fprintf (stderr, "\n  Read:   ");
                for (j = i + 1; j < 9 + len && (j < 9 || isWriting[j - 9]); j++) {
                    fprintf (stderr, " %02X", buff[j]);
                }
                fprintf (stderr, "\n");
                i = j;
                ret = -1;
            }
        }
        dest[i] = buff[i];
    }

    return ret;
}

int checkCRC16 (unsigned char init1, unsigned char init2, unsigned char *ptr, int len) {
    unsigned long calcCRC16, bit, j;
    unsigned int readCRC16;
    int i;

    calcCRC16 = init1 | ((unsigned int) init2 << 8);

    for (i = 0; i < len; i++) {
        for (j = 0x01; j < 0x100; j <<= 1) {
            bit = ( (ptr[i] & j) != 0 ) ^ (calcCRC16 & 0x01);
            calcCRC16 >>= 1;
            if (bit) {
                calcCRC16 ^= 0xA001;
            }
        }
    }

    readCRC16 = 0xFFFF ^ (ptr[len] | ((unsigned int) ptr[len + 1] << 8));

    if (readCRC16 == calcCRC16) {
        return 0;
    } else {
        fprintf (stderr, "ERROR: CRC received = %04X but CRC calculated = %04X.  Data was:\n ", readCRC16, (unsigned int) calcCRC16);
        for (i = 0; i < len; i++) {
            fprintf (stderr, " %02X", ptr[i]);
        }
        fprintf (stderr, "\n");
        return -1;
    }
}
