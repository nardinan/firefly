#include <stdio.h>
#include <usb.h>
#include <time.h>
#include "ow-functions.h"

struct usb_dev_handle *usbHandle;
unsigned char *adapterSettings = NULL;

// Acquires DS2490 adapter, setting the alternate interface to interface.
int acquireAdapter (void) {
	struct usb_bus *usbBus;
	struct usb_device *usbDev;
	unsigned int vendor, product;

	// Find all USB devices
	if (usb_find_busses() < 0) {
		fprintf (stderr, "ERROR: %s\n", usb_strerror());
		goto failure;
	}
	if (usb_find_devices() < 0) {
		fprintf (stderr, "ERROR: %s\n", usb_strerror());
		goto failure;
	}

	usbBus = usb_get_busses();
	if (usbBus == NULL) {
		fprintf (stderr, "ERROR: No USB bus found\n");
		goto failure;
	}

	// Loop over all USB busses and devices
	for (; usbBus != NULL; usbBus = usbBus->next) {
		for (usbDev = usbBus->devices; usbDev != NULL; usbDev = usbDev->next) {

			// Check vendor and product
			vendor = usbDev->descriptor.idVendor;
			product = usbDev->descriptor.idProduct;
			if (vendor == 0x04FA && product == 0x2490) {

				// Attempt to open, configure adapter
				usbHandle = usb_open (usbDev);
				if (usbHandle != NULL) {

					if (
							usb_set_configuration(usbHandle, 1) >= 0 &&
							usb_claim_interface(usbHandle, 0) >= 0
					   ) {

						if (
								usb_set_altinterface(usbHandle, ALT_INTERFACE) >= 0 &&
								usb_clear_halt(usbHandle, 0x81) >= 0 &&
								usb_clear_halt(usbHandle, 0x02) >= 0 &&
								usb_clear_halt(usbHandle, 0x83) >= 0
						   ) {
							return 0;
						} else {
							fprintf (stderr, "ERROR: %s\n", usb_strerror());
						}

						if (usb_release_interface(usbHandle, 0) < 0) {
							fprintf (stderr, "ERROR: %s\n", usb_strerror());
						}

					} else {
						fprintf (stderr, "ERROR: %s\n", usb_strerror());
					}

					if (usb_close(usbHandle) < 0) {
						fprintf (stderr, "ERROR: %s\n", usb_strerror());
					}

				} else {
					fprintf (stderr, "ERROR: %s\n", usb_strerror());
				}

				goto failure;

			}

		}
	}
	goto failure;

failure: {
		 return -1;
	 }
}

// Releases DS2490 adapter.
int releaseAdapter (void) {
	int ret = 0;

	if (usb_release_interface(usbHandle, 0) < 0) {
		fprintf (stderr, "ERROR: %s\n", usb_strerror());
		ret = -1;
	}
	if (usb_close(usbHandle) < 0) {
		fprintf (stderr, "ERROR: %s\n", usb_strerror());
		ret = -1;
	}

	if (ret < 0) {
		goto failure;
	} else {
		return 0;
	}

failure: {
		 fprintf (stderr, "Failed to release adapter\n");
		 return -1;
	 }
}

// Waits for the DS2490 adapter to be finished with the current command, then
// returns the result register from that command.
// Returns -1, indicating an error,
// if any of the bits indicated in mask are set, or some other error occurs.
// If state is non-NULL, stores the state registers there.
int getResult (unsigned char *state, unsigned char mask) {
	char errorDescription[8][64]
		= {"No presence pulse detected",
			"Short on one-wire bus",
			"Alarming presence pulse detected",
			"Bus voltage not raised to 12V for programming pulse",
			"Byte read did not match byte written",
			"CRC read did not match CRC written",
			"Page was redirected",
			"Did not find specified number of devices"};

	unsigned char buff[32], prevSettings[8];
	int len, i, j;
	clock_t timeStarted, timeNow;
	int result = -1;
	int ret = 0;

	if (adapterSettings != NULL) {
		for (i = 0; i < 7; i++) {
			prevSettings[i] = adapterSettings[i];
		}
	}

	timeStarted = clock();

	for (;;) {

		// Read state, result registers
		len = stateRead (buff);
		if (len < 0) {
			fprintf (stderr, "ERROR: %s\n", usb_strerror());
			goto failure;
		}
		if (len < 16) {
			fprintf (stderr, "ERROR: At least 16 bytes expected from EP1, but only %d bytes read", len);
			if (len != 0) {
				fprintf (stderr, ":\n ");
				for (i = 0; i < len; i++) {
					fprintf (stderr, " %02X", buff[i]);
				}
			}
			fprintf (stderr, "\n");
			goto failure;
		}

		// Check adapter settings
		if (adapterSettings != NULL) {
			for (i = 0; i < 7; i++) {
				if (buff[i] != prevSettings[i]) {
					fprintf (stderr, "ERROR: Settings of DS2490 adapter have changed:\n");
					fprintf (stderr, "  Prev:");
					for (j = 0; j < 7; j++) {
						fprintf (stderr, " %02X", prevSettings[j]);
					}
					fprintf (stderr, "\n  Curr:");
					for (j = 0; j < 7; j++) {
						fprintf (stderr, " %02X", buff[j]);
					}
					fprintf (stderr, "\n");
					for (j = 0; j < 7; j++) {
						prevSettings[j] = buff[j];
					}
					ret = -1;
					break;
				}
			}
		}

		// Interpret result registers
		for (i = 16; i < len; i++) {

			// Presence pulses
			if (buff[i] == PRESENCE_PULSE) {
				fprintf (stderr, "ERROR: Unrequested presence pulse from new or power-cycled device detected\n");
				ret = -1;
				continue;
			}

			// Multiple result registers
			if (result != -1) {
				fprintf (stderr, "ERROR: Result registers from multiple commands received:\n");
				for (j = i - 1; buff[j] == PRESENCE_PULSE; j--);
				fprintf (stderr, "  Prev: %02X\n", buff[j]);
				fprintf (stderr, "  Curr: %02X\n", buff[i]);
				ret = -1;
			}

			result = buff[i];

			// Print errors
			if (buff[i] & mask) {
				fprintf (stderr, "ERROR: Result code %02X:\n", buff[i]);
				for (j = 0; j < 8; j++) {
					if (buff[i] & mask & (1 << j)) {
						fprintf (stderr, "  %s\n", errorDescription[j]);
						ret = -1;
					}
				}
			}
		}

		// If adapter is idle
		if (buff[8] & 0x20) {
			// Check for leftover data in command buffer
			if (buff[11] != 0) {
				fprintf (stderr, "ERROR: %d bytes left over in command buffer\n", buff[11]);
				ret = -1;
			}

			// Check for leftover data in outgoing buffer
			if (buff[12] != 0) {
				fprintf (stderr, "ERROR: %d bytes left over in outgoing data buffer\n", buff[12]);
				ret = -1;
			}

			// Check whether result returned
			if (result < 0) {
				fprintf (stderr, "ERROR: Adapter idle but no result code returned\n");
				ret = -1;
			}

			// Save state registers if requested
			if (state != NULL) {
				for (i = 0; i < 16; i++) {
					state[i] = buff[i];
				}
			}

			// Reset adapter if error occurred
			if (ret < 0) {
				goto failure;
			}

			return result;
		}

		// Timeout
		timeNow = clock();
		if (timeNow > timeStarted + CLOCKS_PER_SEC * RESULT_TIMEOUT) {
			fprintf (stderr, "ERROR: Timed out waiting for result register from adapter\n");
			goto failure;
		}
		if (timeNow < timeStarted) {
			timeStarted = timeNow;
		}

	}

failure: {
		 fprintf (stderr, "Error while getting result register from adapter; reseting adapter\n");
		 resetAdapter();
		 return -1;
	 }
}

// Resets the DS2490 adapter, clearing its input, output, and command buffers.
// Also, if adapter settings are specified with a non-NULL adapterSettings,
// sets up the adapter with these settings.
int resetAdapter (void) {
	unsigned int param[8];
	int i;

	if (usb_control_msg(usbHandle, 0x40, CONTROL_CMD, ADAPTER_RESET_CMD, 0, NULL, 0, USB_TIMEOUT) < 0) {
		fprintf (stderr, "ERROR: %s\n", usb_strerror());
		goto failure;
	}

	if (adapterSettings != NULL) {
		param[0] = ((adapterSettings[0] >> 1) & 0x01) | ((adapterSettings[0] << 1) & 0x02);
		param[1] = (adapterSettings[0] >> 2) & 0x01;
		param[2] = adapterSettings[1];
		param[3] = adapterSettings[2];
		param[4] = adapterSettings[4];
		param[5] = adapterSettings[3];
		param[6] = adapterSettings[5];
		param[7] = adapterSettings[6];

		for (i = 0; i < 8; i++) {
			if (usb_control_msg(usbHandle, 0x40, MODE_CMD, i, param[i], NULL, 0, USB_TIMEOUT) < 0) {
				fprintf (stderr, "ERROR: %s\n", usb_strerror());
				goto failure;
			}
		}
	}

	return 0;

failure: {
		 fprintf (stderr, "Failed to reset adapter\n");
		 return -1;
	 }
}

// Reads len bytes from the DS2490 adapter after a communication command.
// In case of error, up to READ_BUFFER_LEN bytes may be stored at buff.
int readBuffer (unsigned char buff[READ_BUFFER_LEN], int len) {
	int pos, dpos, i;

	for (pos = 0; pos < len; pos += dpos) {
		dpos = bulkRead (buff + pos);
		if (dpos < 0) {
			fprintf(stderr, "ERROR: %s\n", usb_strerror());
			goto failure;
		}
		if (dpos == 0) {
			fprintf(stderr, "ERROR: USB request read zero bytes from adapter\n");
			goto failure;
		}
	}

	if (pos > len) {
		fprintf (stderr, "ERROR: Adapter indicated %d bytes to read, but %d bytes were read", len, pos);
		if (pos != 0) {
			fprintf (stderr, ":\n ");
			for (i = 0; i < pos; i++) {
				fprintf (stderr, " %02X", buff[i]);
			}
		}
		fprintf (stderr, "\n");
		goto failure;
	}

	return 0;

failure: {
		 fprintf (stderr, "Error while reading data from adapter; reseting adapter\n");
		 resetAdapter();
		 return -1;
	 }
}

// Writes len bytes to the DS2490 adapter in preparation for a communication command.
int writeBuffer (unsigned char *buff, int len) {
	int pos, dpos;

	for (pos = 0; pos < len; pos += dpos) {
		dpos = bulkWrite (buff + pos, len - pos);
		if (dpos < 0) {
			fprintf (stderr, "ERROR: %s\n", usb_strerror());
			goto failure;
		}
		if (dpos == 0) {
			fprintf (stderr, "ERROR: USB request wrote zero bytes to adapter\n");
			goto failure;
		}
	}

	if (pos > len) {
		fprintf (stderr, "ERROR: Should have written %d bytes but wrote %d bytes\n", len, pos);
		goto failure;
	}

	return 0;

failure: {
		 fprintf (stderr, "Error encountered while writing data from adapter; reseting adapter\n");
		 resetAdapter();
		 return -1;
	 }
}

// Performs an initial one-wire reset if reset=1, then
// writes len bytes of data from src to the one-wire network,
// and stores len bytes from the network in dest.
// In case of error, up to READ_BUFFER_LEN bytes may be stored at dest.
int blockRW (unsigned char *src, unsigned char dest[READ_BUFFER_LEN], int len, int reset) {
	unsigned char state[16];
	int i;

	if (writeBuffer(src, len) < 0) {
		goto failure;
	}

	if (usb_control_msg(usbHandle, 0x40, COMM_CMD, reset ? BLOCK_RESET_CMD: BLOCK_CMD, len, NULL, 0, USB_TIMEOUT) < 0) {
		fprintf (stderr, "ERROR: %s\nError sending control message to adapter, reseting adapter", usb_strerror());
		resetAdapter();
		goto failure;
	}

	if (getResult(state, 0xFF ^ PROG_PULSE_ERROR) < 0) {
		goto failure;
	}

	if (readBuffer(dest, state[13]) < 0) {
		goto failure;
	}

	if (state[13] != len) {
		fprintf (stderr, "ERROR: %d bytes written, but %u bytes received", len, state[13]);
		if (state[13] != 0) {
			fprintf (stderr, ":\n ");
			for (i = 0; i < state[13]; i++) {
				fprintf (stderr, " %02X", dest[i]);
			}
		}
		fprintf (stderr, "\nReseting adapter\n");
		resetAdapter();
		goto failure;
	}

	return 0;

failure: {
		 fprintf (stderr, "One-wire bulk read/write failed\n");
		 return -1;
	 }
}

// Writes bit to one-wire network; returns bit read.
int bitRW (unsigned char bit) {
	unsigned char buff[READ_BUFFER_LEN];
	unsigned char state[16];
	unsigned short cmd;
	int i;

	if (bit) {
		cmd = BIT_1_CMD;
	} else {
		cmd = BIT_0_CMD;
	}

	if (usb_control_msg(usbHandle, 0x40, COMM_CMD, cmd, 0, NULL, 0, USB_TIMEOUT) < 0) {
		fprintf (stderr, "ERROR: %s\nError sending control message to adapter, reseting adapter", usb_strerror());
		resetAdapter();
		goto failure;
	}

	if (getResult(state, 0xFF ^ PROG_PULSE_ERROR) < 0) {
		goto failure;
	}

	if (readBuffer(buff, 1) < 0) {
		goto failure;
	}

	if (state[13] != 1) {
		fprintf (stderr, "ERROR: One byte expected, but %u bytes received", state[13]);
		if (state[13] != 0) {
			fprintf (stderr, ":\n ");
			for (i = 0; i < state[13]; i++) {
				fprintf (stderr, " %02X", buff[i]);
			}
		}
		fprintf (stderr, "\nReseting adapter\n");
		goto failure;
	}

	return buff[0];

failure: {
		 fprintf (stderr, "One-wire bit read/write failed\n");
		 return -1;
	 }
}

// Performs one-wire reset.
int resetPulse (void) {
	if (usb_control_msg(usbHandle, 0x40, COMM_CMD, RESET_CMD, 0, NULL, 0, USB_TIMEOUT) < 0) {
		fprintf (stderr, "ERROR: %s\nError sending control message to adapter, reseting adapter", usb_strerror());
		resetAdapter();
		goto failure;
	}

	if (getResult(NULL, 0xFF ^ PROG_PULSE_ERROR) < 0) {
		goto failure;
	}

	return 0;

failure: {
		 fprintf (stderr, "One-wire reset failed\n");
		 return -1;
	 }
}

// Finds up to maxDevices using the one-wire ROM command searchCmd;
// stores serial numbers in SN; returns number of devices found.
long findDevices (const unsigned char searchCmd, unsigned char SN[][8], long maxDevices) {
	unsigned char tranBuff[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	unsigned char state[32];
	unsigned char recvBuff[READ_BUFFER_LEN];
	unsigned char branch[8];
	int i, len, result;
	long dev;
	unsigned int mask;
	int depth = 0;
	long numDevices = 0;

	while (numDevices < maxDevices) {

		// Write initial SN to DS2490
		if (writeBuffer(tranBuff, 8) < 0) {
			goto failure;
		}

		// Send command to DS2490
		if (usb_control_msg(usbHandle, 0x40, COMM_CMD, SEARCH_ACCESS_CMD, 0x100 | searchCmd, NULL, 0, USB_TIMEOUT) < 0) {
			fprintf (stderr, "ERROR: %s\nError sending control message to adapter, reseting adapter", usb_strerror());
			resetAdapter();
			goto failure;
		}

		// Get result register
		mask = 0xFF ^ END_OF_SEARCH ^ PROG_PULSE_ERROR;
		if (numDevices == 0 && searchCmd == ALARM_SEARCH_CMD) {
			mask ^= NO_PRESENCE_PULSE;
		}
		result = getResult(state, mask);
		if (result < 0) {
			goto failure;
		}
		if (result & NO_PRESENCE_PULSE) {
			return 0;
		}

		// Read SN from DS2490
		if (readBuffer(recvBuff, state[13]) < 0) {
			goto failure;
		}
		if (result & END_OF_SEARCH) {
			len = 8;
		} else {
			len = 16;
		}
		if (state[13] != len) {
			fprintf (stderr, "ERROR: %d bytes expected, but %u bytes received", len, state[13]);
			if (state[13] != 0) {
				fprintf (stderr, ":\n ");
				for (i = 0; i < state[13]; i++) {
					fprintf (stderr, " %02X", recvBuff[i]);
				}
			}
			fprintf (stderr, "\nReseting adapter\n");
			resetAdapter();
			goto failure;
		}

		// Check SN against expected next SN
		for (i = 0; i < depth; i++) {
			if ( (recvBuff[i>>3] ^ tranBuff[i>>3]) & (1<<(i&7)) ) {
				fprintf (stderr, "ERROR: Expected next SN different from SN read:\n");
				fprintf (stderr, "  Expected: ");
				printBits (stderr, tranBuff, depth);
				fprintf (stderr, "\n  Read:     ");
				printBits (stderr, recvBuff, 64);
				fprintf (stderr, "\n");
				goto failure;
			}
		}

		// Check cyclic redundancy code
		if ( checkCRC8(recvBuff, 7) < 0 ) {
			goto failure;
		}

		// Record SN
		for (i = 0; i < 8; i++) {
			SN[numDevices][i] = recvBuff[i];
		}
		numDevices++;

		if (result & 0x80) {
			// Check whether there are more branches to explore
			for (i = 0; i < depth - 1; i++) {
				if ( branch[i>>3] & (~recvBuff[i>>3]) & (1<<(i&7)) ) {
					fprintf (stderr, "ERROR: Search ended, but there are more branches to explore:  \n");
					printBits (stderr, branch, depth - 1);
					fprintf (stderr, "\n");
					goto failure;
				}
			}

			return numDevices;
		}

		// Check for inconsistencies in branches found
		for (i = 0; i < depth; i++) {
			if ( ((recvBuff + 8)[i>>3] ^ branch[i>>3]) & (1<<(i&7)) ) {
				fprintf (stderr, "ERROR: Branches detected in different runs different\n");
				fprintf (stderr, "  Prev: ");
				printBits (stderr, branch, depth);
				fprintf (stderr, "\n  Curr: ");
				printBits (stderr, recvBuff + 8, 64);
				fprintf (stderr, "\n");
				goto failure;
			}
		}

		// Calculate next initial SN
		for (i = 63; i >= 0; i--) {
			if ( (recvBuff + 8)[i>>3] & (~recvBuff[i>>3]) & (1<<(i&7)) ) {
				depth = i + 1;
				break;
			}
		}
		if (i < 0) {
			fprintf (stderr, "ERROR: Not end of search, but no more branches to explore\n");
			goto failure;
		}
		for (i = 0; i < 8; i++) {
			tranBuff[i] = 0;
			branch[i] = 0;
		}
		for (i = 0; i < depth - 1; i++) {
			tranBuff[i>>3] |= recvBuff[i>>3] & (1<<(i&7));
			branch  [i>>3] |= (recvBuff + 8)[i>>3] & (1<<(i&7));
		}
		tranBuff[i>>3] |= (1<<(i&7));
		branch  [i>>3] |= (1<<(i&7));

	}

	fprintf (stderr, "ERROR: Maximum number of devices %ld exceeded\n", maxDevices);
	goto failure;

failure: {
		 fprintf (stderr, "Search for devices failed.  Devices found before failure:\n");
		 for (dev = 0; dev < numDevices; dev++) {
			 fprintf (stderr, "  ");
			 for (i = 7; i >= 0; i--) {
				 fprintf (stderr, "%02X", SN[dev][i]);
			 }
			 fprintf (stderr, "\n");
		 }
		 if (numDevices == 0) {
			 fprintf (stderr, "  None\n");
		 }
		 return -1;
	 }
}

// Prints len bits from buff in binary to stream.
void printBits (FILE *stream, const unsigned char *buff, int len) {
	int i;
	unsigned int mask;
	int bits = 0;

	for (i = 0; i < len; i++) {
		for (mask = 0x01; mask < 0x100; mask <<= 1) {
			fputc ((buff[i] & mask) ? '1' : '0', stream);
			if ((++bits) >= len) {
				goto endOfData;
			}
		}
	} endOfData:

	return;
}

// Calculates 8-bit CRC for len bytes of data at ptr,
// and compares with the byte following the data (ptr[len]).
int checkCRC8 (unsigned char *ptr, int len) {
	unsigned int crc8, bit, j;
	int i;

	crc8 = 0x00;
	for (i = 0; i < len; i++) {
		for (j = 0x01; j < 0x100; j <<= 1) {
			bit = ( (ptr[i] & j) != 0 ) ^ (crc8 & 0x01);
			crc8 >>= 1;
			if (bit) {
				crc8 ^= 0x8C;
			}
		}
	}

	if ((unsigned int) ptr[len] == crc8) {
		return 0;
	} else {
		fprintf (stderr, "ERROR: CRC received = %02X but CRC calculated = %02X.  Data was:\n ", ptr[len], crc8);
		for (i = 0; i < len; i++) {
			fprintf (stderr, " %02X", ptr[i]);
		}
		fprintf (stderr, "\n");
		return -1;
	}
}

// Reads adapter settings from file named filename.
// Attempts to Creates file with power-on default values if it does not exist.
void readAdapterSettings (const char *filename) {
	static unsigned char settings[7] = {0x00, 0x00, 0x20, 0x40, 0x05, 0x04, 0x04};
	unsigned int byte;
	FILE *stream;
	int i, j;

	adapterSettings = settings;

	i = 0;
	stream = fopen (filename, "r");
	if (stream != NULL) {
		for (; i < 7; i++) {
			if ( fscanf(stream, "%02x", &byte) == 1) {
				settings[i] = byte;
			} else {
				fprintf (stderr, "ERROR: Only %d of 7 settings read from '%s'", i, filename);
				if (i != 0) {
					fprintf (stderr, ":\n ");
					for (j = 0; j < i; j++) {
						fprintf (stderr, " %02X", settings[j]);
					}
				}
				fprintf (stderr, "\n");
				break;
			}
		}
		fclose (stream);
	} else {
		fprintf (stderr, "ERROR: Settings file '%s' not found; creating it\n", filename);
	}

	if (i < 7) {
		stream = fopen (filename, "a");
		if (stream == NULL) {
			fprintf (stderr, "ERROR: Could not open '%s' for writing\n\n", filename);
			return;
		}
		fprintf (stderr, "Writing default settings to '%s':\n ", filename);
		for (; i < 7; i++) {
			fprintf (stream, " %02X", settings[i]);
			fprintf (stderr, " %02X", settings[i]);
		}
		fprintf (stream, "\n");
		fprintf (stderr, "\n\n");
		fclose (stream);
	}

	return;
}
