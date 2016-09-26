#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
// #include <usb.h>

#include <libusb.h>

// /sys/bus/usb/devices/1-4/ is our device, as currently plugged in
//
// It supports
//	1 configuration
//	1 interface
// and has a product string of...
//	"XuLA - XESS Micro Logic Array"
// 
#define	VENDOR_ID	0x04d8
#define	PRODUCT_ID	0x0ff8c
#define	XESS_ENDPOINT_OUT	0x01
#define	XESS_ENDPOINT_IN	0x81
//
#define	JTAG_CMD	0x4f
#define	GET_TDO_MASK	0x01
#define	PUT_TMS_MASK	0x02
#define	TMS_VAL_MASK	0x04
#define	PUT_TDI_MASK	0x08
#define	TDI_VAL_MASK	0x10
//
#define	USER1_INSTR	0x02	// a SIX bit two


bool	gbl_transfer_received = false;

// libusbtransfer_cb_fn	callback;
extern "C" {
void	my_callback(libusb_transfer *tfr) {
	gbl_transfer_received = true;

	printf("Callback received!\n");
}
}

// All messages must be 32 bytes or less
//


// Walk us through the JTAG Chain:
//	5-1's to go to test/reset
//	0	to go to Run-Test/Idle
//	1	to go to select-dr-scan
//	1	to go to select-ir-scan
//	0	to go to capture-ir
//	0	to go to shift-ir
//	(6-bit code 0x02 through TDI to IR, while sending 0-bits to TMS)
//	1	to leave shift IR and go to exit1-ir
//	1	to go to update-ir
//	1	to go to select-dr-scan
//	0	to go to capture-dr
//	0	to go to shift-dr
#define	RESET_JTAG_LEN	12
const char	RESET_TO_USER_DR[RESET_JTAG_LEN] = {
	JTAG_CMD,
	21, // clocks
	0,0,0,	// Also clocks, higher order bits
	PUT_TMS_MASK | PUT_TDI_MASK, // flags
	(char)(0x0df),	// TMS: Five ones, then one zero, and two ones -- low bits first
	0x00,	// TDI: irrelevant here
	(char)(0x80),	// TMS: two zeros, then six zeros
	0x08,	// TDI: user command #1, bit reversed
	0x03,	// TMS: three ones, then two zeros
	0x00	// TDI byte -- irrelevant here
	//
	// 0xc0, // TDI byte -- user command #2
	// 0x40,	// TDI: user command #1
	// 0x0c, // TDI byte -- user command #2, bit reversed
};

//
//	TMS: 
/*
#define	TX_DR_LEN	12
const	char	TX_DR_BITS[TX_DR_LEN] = {
	JTAG_CMD,
	48, // clocks
	0,0,0,	// Also clocks, higher order bits
	PUT_TDI_MASK, // flags
	(char)0x0ff, 0, 0, 0, 0, 0	// Six data bytes
	// module_id = 255
	// 32'h(payload.length)
	// payload
	// module_id + payload.len + num_result_bits, length=32 + payload ???
};
*/

//
//	TMS: 
//	
#define	REQ_RX_LEN	6
const	char	REQ_RX_BITS[REQ_RX_LEN] = {
	JTAG_CMD,
	(char)((32-6)*8), // bits-requested
	0,0,0,	// Also clocks, higher order bits
	GET_TDO_MASK|TDI_VAL_MASK, // flags:TDI is kept low here, so no TDI flag
	// No data given, since there's no info to send or receive
	// Leave the result in shift-DR mode
};

#define	RETURN_TO_RESET_LEN	7
const char	RETURN_TO_RESET[RETURN_TO_RESET_LEN] = {
	JTAG_CMD,
	5, // clocks
	0,0,0,	// Also clocks, higher order bits
	PUT_TMS_MASK, // flags
	(char)(0x0ff), // Five ones
};

int	dec(int v) {
	int br = 0;

	/*
	br = (br<<1)|(v&1); v>>=1;
	br = (br<<1)|(v&1); v>>=1;
	br = (br<<1)|(v&1); v>>=1;
	br = (br<<1)|(v&1); v>>=1;

	br = (br<<1)|(v&1); v>>=1;
	br = (br<<1)|(v&1); v>>=1;
	br = (br<<1)|(v&1); v>>=1;
	br = (br<<1)|(v&1); v>>=1;
	*/
	br = v&0x07f;

	if (br == ' ')
		return br;
	else if (isgraph(br))
		return br;
	else
		return '.';
}

//
// If max packet length is 32, why do you waste 4 bytes on num_clocks?
// Why is the bit counter always from 8 to zero?
//

int main(int argc, char **argv) {
	libusb_context		*usb_context;
	libusb_device_handle	*xula_usb_device;
	int	config;

	if (0 != libusb_init(&usb_context)) {
		fprintf(stderr, "Error initializing the USB library\n");
		perror("O/S Err:");
		exit(-1);
	}

	xula_usb_device = libusb_open_device_with_vid_pid(usb_context,
		VENDOR_ID, PRODUCT_ID);
	if (!xula_usb_device) {
		fprintf(stderr, "Could not open XuLA device\n");
		perror("O/S Err:");

		libusb_exit(usb_context);
		exit(-1);
	}

	if (0 != libusb_get_configuration(xula_usb_device, &config)) {
		fprintf(stderr, "Could not get configuration\n");
		perror("O/S Err:");

		libusb_close(xula_usb_device);
		libusb_exit(usb_context);
		exit(-1);
	}

	printf("Current configuration is %d\n", config);
	int	interface = 0;

	if (0 != libusb_claim_interface(xula_usb_device, interface)) {
		fprintf(stderr, "Could not claim interface\n");
		perror("O/S Err:");

		libusb_close(xula_usb_device);
		libusb_exit(usb_context);
		exit(-1);
	}

	unsigned char	*abuf = new unsigned char[32];
	int	actual_length = 32;
	memcpy(abuf, RESET_TO_USER_DR, RESET_JTAG_LEN);
	int r = libusb_bulk_transfer(xula_usb_device, XESS_ENDPOINT_OUT,
		abuf, RESET_JTAG_LEN, &actual_length, 20);
	if ((r==0)&&(actual_length == RESET_JTAG_LEN)) {
		printf("Successfully sent RESET_TO_USER_DR!\n");
	} else {
		printf("Some error took place requesting RESET_TO_USER_DR\n");
		perror("O/S Err");
	}

	const char hello_world[] = "Hello, World!\n";
	abuf[0] = JTAG_CMD;
	abuf[1] = strlen(hello_world) * 8 + 8;
	abuf[2] = abuf[3] = abuf[4] = 0;
	abuf[5] = PUT_TDI_MASK | GET_TDO_MASK;
	strcpy((char *)&abuf[6], hello_world);
	// abuf[6] = 0xff;
	// abuf[7] = 0xff;
	// abuf[8] = 0x00;
	// abuf[9] = 0xff;
	// abuf[10] = 0x01;
	// abuf[11] = 0x02;
	// abuf[12] = 0x04;
	// abuf[13] = 0x08;
	r = libusb_bulk_transfer(xula_usb_device, XESS_ENDPOINT_OUT,
		abuf, strlen(hello_world)+6+1, &actual_length, 20);
	if ((r==0)&&(actual_length == strlen(hello_world)+6+1)) {
		printf("Successfully sent request for TDO bits!\n");
	} else {
		printf("Some error took place in requesting TDO bits\n");
		printf("r = %d, actual_length = %d (!= %d)\n", r,
			actual_length, (int)strlen(hello_world)+6);
		perror("O/S Err");
	}

	r = libusb_bulk_transfer(xula_usb_device, XESS_ENDPOINT_IN,
		abuf, strlen(hello_world)+1, &actual_length, 20);
	if ((r==0)&&(actual_length > 0)) {
		printf("Successfully read %d bytes from port!\n", actual_length);
		for(int i=0; i<(actual_length); i+=4)
			printf("%2d: %02x %02x %02x %02x -- %c%c%c%c\n", i,
				abuf[i+0], abuf[i+1], abuf[i+2], abuf[i+3],
				dec(abuf[i+0]),dec(abuf[i+1]), dec(abuf[i+2]), dec(abuf[i+3]));
	} else {
		printf("Some error took place in receiving\n");
		perror("O/S Err");
	}


	// Release our interface
	if (0 != libusb_release_interface(xula_usb_device, interface)) {
		fprintf(stderr, "Could not release interface\n");
		perror("O/S Err:");

		libusb_close(xula_usb_device);
		libusb_exit(usb_context);
		exit(-1);
	}

	// And then close our device with
	libusb_close(xula_usb_device);

	// And just before exiting, we free our USB context
	libusb_exit(usb_context);
}

