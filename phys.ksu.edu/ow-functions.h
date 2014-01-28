#include <stdio.h>
#define READ_BUFFER_LEN 256

#define USB_TIMEOUT 1000
#define RESULT_TIMEOUT 1
#define SERIAL_SIZE 8

#define ALT_INTERFACE 3

#define CONTROL_CMD 0x00
#define COMM_CMD 0x01
#define MODE_CMD 0x02

#define ADAPTER_RESET_CMD 0x0000

#define RESET_CMD 0x0443
#define BIT_0_CMD 0x0421
#define BIT_1_CMD 0x0429
#define BLOCK_CMD 0x0475
#define BLOCK_RESET_CMD 0x0575
#define SEARCH_ACCESS_CMD 0x45FD

#define PRESENCE_PULSE 0xA5

#define END_OF_SEARCH 0x80
#define REDIRECTED_PAGE 0x40
#define CRC_ERROR 0x20
#define COMP_ERROR 0x10
#define PROG_PULSE_ERROR 0x08
#define OW_INTERRUPT 0x04
#define OW_SHORT 0x02
#define NO_PRESENCE_PULSE 0x01

#define SEARCH_ROM_CMD 0xF0
#define ALARM_SEARCH_CMD 0xEC

#define bulkRead(b)    usb_bulk_read  (usbHandle, 0x83, b, 128, USB_TIMEOUT)
#define bulkWrite(b,l) usb_bulk_write (usbHandle, 0x02, b, l,   USB_TIMEOUT)
#define stateRead(b)   usb_bulk_read  (usbHandle, 0x81, b, 32,  USB_TIMEOUT)

int acquireAdapter (void);
int releaseAdapter (void);
int getResult (unsigned char *state, unsigned char mask);
int resetAdapter (void);
int readBuffer (unsigned char buff[READ_BUFFER_LEN], int len);
int writeBuffer (unsigned char *buff, int len);
int blockRW (unsigned char *src, unsigned char dest[READ_BUFFER_LEN], int len, int reset);
int bitRW (unsigned char bit);
int resetPulse (void);
long findDevices (const unsigned char searchCmd, unsigned char SN[][8], long maxDevices);
void printBits (FILE *stream, const unsigned char *buff, int len);
int checkCRC8 (unsigned char *ptr, int len);
void readAdapterSettings (const char *filename);

extern struct usb_dev_handle *usbHandle;
