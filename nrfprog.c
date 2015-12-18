#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <stdlib.h>
#include <stdint.h>
#include "bp.h"
#include "nrf24le1.h"

// Debug printing
// #define TRACE_MSG	{printf("%s() [%s:%d] here\n", __FUNCTION__, __FILE__, __LINE__);}
#define TRACE_MSG	{}

#define s2b(s, b) {\
	b[1] = s & 0xFF;\
	b[0] = (s >> 8) & 0xFF;\
}

/* Open a serial port */
int ser_open(char *port) {
	int fd;
	struct termios cfg;

	TRACE_MSG;

	// Open the port
	fd = open(port, O_RDWR | O_NOCTTY | O_NDELAY);
	if(fd == -1) {
		printf("Could not open the serial port: %s\n", port);
		exit(-1);
	}
	
	// Set the flags, make sure we're using the nonblocking interface
	fcntl(fd, F_SETFL, FNDELAY);

	// Get the port's current configuration
	tcgetattr(fd, &cfg);
	// Set the baud rate to 115200
	cfsetispeed(&cfg, B115200);
	cfsetospeed(&cfg, B115200);
	// Enable the receiver
	cfg.c_cflag |= CREAD;
	// Set the data bits to 8
	cfg.c_cflag &= ~CSIZE;
	cfg.c_cflag |= CS8;
	// Set the parity to none
	cfg.c_cflag &= ~PARENB;
	// Set the stop bits to 1
	cfg.c_cflag &= ~CSTOPB;
	// Disable HW Flow Control
	// cfg.c_cflag &= ~CNEW_RTSCTS;
	// Set I/O to RAW
	cfg.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	cfg.c_oflag &= ~OPOST;
	// Disable Software Flow Control
	cfg.c_iflag &= ~(IXON | IXOFF | IXANY);
	// Send the config to the port
	tcsetattr(fd, TCSANOW, &cfg);
	return fd;
}

/* Close the serial port */
void ser_close(int fd) {
	TRACE_MSG;
	close(fd);
}

/* Write a stream of bytes to the serial port */
void ser_write(int fd, uint8_t *stream, uint16_t sz) {
	int ret, ct;
	uint8_t c;
	TRACE_MSG;
	tcdrain(fd);
	// Write out one byte at a time
#ifdef DEBUG_PRINT
		printf("ser_write: ");
#endif	
	for(ct=0; ct<sz; ct++) {
#ifdef DEBUG_PRINT
		printf("0x%02X ", stream[ct]);
#endif
		c = stream[ct];
		ret = write(fd, &c, 1);
		if(ret != 1) {
			printf("Unable to write to the port!\n");
			close(fd);
			exit(-1);
		}
	}
#ifdef DEBUG_PRINT
	printf("\n");
#endif
	tcdrain(fd);
	return;
}

/* Write a single byte command to the serial port */
void ser_cmd(int fd, int cmd) {
	uint8_t ccmd = cmd;	
	TRACE_MSG;
#ifdef DEBUG_PRINT
	printf("bp_cmd: ");
#endif
	ser_write(fd, &ccmd, 1);
	// Delay for the hardware to respond
	usleep(10000);
	return;
}

/* Set the bus pirate to binary mode */
void ser_bp_bin(int fd) {
	int count, ret;
	char buf[6];
	TRACE_MSG;
	memset(buf, 0, 6);

	// We try up to 25 times to make it work
	for(count=0; count < 25; count++) {
		ser_cmd(fd, BP_BIN_RESET);
		ret = read(fd, buf, 5);
#ifdef DEBUG_PRINT
		int tmp;
		printf("sz: %i\n", ret);
		for(tmp=0; tmp<ret; tmp++)
			printf("0x%X %c\n", buf[tmp], buf[tmp]);
#endif
		if(ret == 5 && strncmp(buf, BP_BINARY_STRING, 5) == 0) {
			// In binary mode!
			return;
		}
	}
	printf("Unable to put Bus Pirate in Binary Mode!\n");
	exit(-1);
}

/* Set the bus pirate back to normal mode */
void ser_bp_exit_bin(int fd) {
	int ret, count;
	uint8_t buf;
	TRACE_MSG;
	// Make sure we're in the BBIO and not anywhere else
	ser_bp_bin(fd);

	// Reset to normal mode
	for(count=0; count<25; count++) {
		// Reset the BP
		tcdrain(fd);
		tcflush(fd, TCIOFLUSH);
		ser_cmd(fd, BP_RESET);
		// Check that we got the reset response
		ret = read(fd, &buf, 1);
#ifdef DEBUG_PRINT
		printf("ret = %i (0x%X) %c\n", ret, buf, buf);
#endif

		if(buf == BP_RESP_SUCCESS)
			break;
		
	}
	return;
}

/* Check a response */
void check_resp(int fd, char *err) {
	int ret, ct = 0;
	uint8_t buf;
	TRACE_MSG;

	// Check up to 25 times for a response
	while(ct < 50) {
		ret = read(fd, &buf, 1);
		if(ret != -1)
			break;
		// If we didn't get any data, wait for 0.25s and try again
		usleep(250000);
		ct++;
	}
#ifdef DEBUG_PRINT
	printf("check_resp: 0x%X (%c)\n", buf, buf);
#endif
	if(buf != BP_RESP_SUCCESS) {
		// Try to set the bus pirate back to normal mode
		printf("check_resp: %s failed\nSetting BP back to normal.\n", err);
		ser_bp_exit_bin(fd);
		// Close the connection
		ser_close(fd);
		exit(-1);
	} 
#ifdef DEBUG_PRINT
	else {
		printf("check_resp: %s succeeded\n", err);
	}
#endif
	return;
}

/* Configure the BP SPI Mode */
void ser_bp_spi_cfg(int fd) {
	int ret;
	char buf[5];
	TRACE_MSG;
	memset(buf, 0, 5);

	// Go to SPI Mode	
	ser_cmd(fd, BP_SPI);

	// Check that we're in SPI Mode
	ret = read(fd, &buf, 4);
	if(ret == 4 && strncmp(buf, BP_SPI_STRING, 4) == 0) {
		// Set the SPI Speed to 30KHz
		ser_cmd(fd, BP_SPI_SPEED(BP_SPI_SPEED_30K));
		check_resp(fd, "SPI Set Speed");

		// Set the SPI Configuration (pin output 3.3V, Clock Idle Phase Low, Clock Edge Active to Idle = 1), Sample Time Middle)
		ser_cmd(fd, BP_SPI_CFG((BP_SPI_CFG_3v3 | BP_SPI_CFG_CKE)));
		check_resp(fd, "SPI Configuration");

		// Turn on the power and AUX Pins
		ser_cmd(fd, BP_SPI_PERF((BP_SPI_PERF_POWER | BP_SPI_PERF_AUX)));
		check_resp(fd, "SPI Turn on Power and AUX");
	
		// Turn off the AUX pin to reset the chip
		ser_cmd(fd, BP_SPI_PERF(BP_SPI_PERF_POWER));
		check_resp(fd, "SPI Turn off the AUX pin");
	
		// Turn on the AUX pin to release the chip from reset
		ser_cmd(fd, BP_SPI_PERF((BP_SPI_PERF_POWER | BP_SPI_PERF_AUX | BP_SPI_PERF_CS)));
		check_resp(fd, "SPI Turn on AUX pin");
	}
	
	return;
}

/* Load a hex file */
void hf_read(char *fn, uint8_t **data, uint32_t *sz) {
	FILE *f;
	int line = 0, i, done = 0;
	uint8_t byte_count, rec_type, checksum, c_checksum;
	uint16_t address = 0, address_high = 0;
	char buf[532];
	uint8_t dat[256];
	TRACE_MSG;

	// Open the file
	f = fopen(fn, "r");

	// Allocate 16kbytes, the max size
	*sz = NRF24_FLASH_SZ;
	
	// Allocate the data and make sure it's cleared
	*data = (uint8_t *)malloc(sizeof(uint8_t) * (*sz));
	memset(*data, 0xFF, sizeof(uint8_t) * (*sz));

	// Read in the data from the file and store in the array
	while(!done) {
		// Read in the line
		if(fgets(buf, 532, f) == 0)
			break;

		// Make sure this record starts with :, otherwise, skip it
		if(buf[0] != ':')
			continue;

		// Get the byte count, address and record type
		sscanf(buf, ":%02hhX%04hX%02hhX", &byte_count, &address, &rec_type);
		
		// Get the checksum
		sscanf(buf + 9 + (byte_count * 2), "%02hhX", &checksum);

		// Get the data on this line
		for(i=0; i<byte_count; i++)
			sscanf(buf + 9 + i*2, "%02hhX", &(dat[i]));

		// Compute the checksum
		c_checksum = 0;
		for(i=0; i<byte_count; i++)
			c_checksum += dat[i];
		c_checksum += byte_count + rec_type + (address & 0xFF) + ((address >> 8) & 0xFF);
		c_checksum = (0x100 - c_checksum) & 0xFF;
		if(c_checksum != checksum)
			printf("Warning checksum error on line: %i (%02X != %02X)\n", line, c_checksum, checksum); 

		// Check based on the record type
		switch(rec_type) {
		// Data record
		case 0:
			// Copy the data into the buffer, at the right place
			memcpy(&(*data)[(address_high << 16) + address], dat, byte_count);
			break;
		// EOF Record
		case 1:
			// At EOF, we're done
			if(byte_count == 0)
				done = 1;
			break;
		// Extended Segment Address Record
		case 2:
			printf("FIXME: ESA\n");
			break;
		// Start Segment Address Record
		case 3:
			printf("FIXME: SSA\n");
			break;
		// Extended Linear Address Record
		case 4:
			// This is the new high order bits
			address_high = (dat[0] << 8) & dat[1];
			break;
		// Start Linear Address Record
		case 5:
			printf("FIXME: SLA\n");
			break;
		}
		line++;
	}
	// Complete
	fclose(f);
	return;
}

/* Execute an SPI command */
void spi_cmd(int fd, uint8_t *cmd, int cmd_len, char *err) {
	uint16_t write_bytes, read_bytes;
	uint8_t b[2];
	TRACE_MSG;
	// Initiate the WR_RD operation
	ser_cmd(fd, BP_SPI_WR_RD);	

	// Send the read/write length and command bytes
	write_bytes = cmd_len; read_bytes = 0;
	s2b(write_bytes, b);
	ser_write(fd, (uint8_t *) &b, 2);
	s2b(read_bytes, b);
	ser_write(fd, (uint8_t *) &b, 2);
	ser_write(fd, cmd, write_bytes);
	usleep(1000);
	check_resp(fd, "SPI Command");
	return;	
}

/* Execute the SPI Read command */
void spi_read(int fd, uint8_t *buf, uint16_t len, int start_addr, char *err) {
	uint16_t write_bytes = 3;
	uint8_t bytes[3];
	int pos, err_ct = 0;
	TRACE_MSG;
#ifdef DEBUG_PRINT	
	printf("spi_read: %s\n", err);
#endif
	// Initiate the WR_RD operation
	ser_cmd(fd, BP_SPI_WR_RD);
	// Send the read/write length and command bytes
	s2b(write_bytes, bytes);
	ser_write(fd, (uint8_t *) bytes, 2);
	s2b(len, bytes);
	ser_write(fd, (uint8_t *) bytes, 2);
	// Setup the write buffer
	bytes[0] = NRF24_SPI_READ;
	ser_write(fd, (uint8_t *) bytes, 1);
	s2b(start_addr, bytes);
	ser_write(fd, bytes, 2);
	// Delay for the response
	usleep(200000); // wait for bp spi read to buffer

	check_resp(fd, "SPI Read Start");

#ifdef DEBUG_PRINT
	printf("READ %i bytes\n", len);
#endif
	for(pos = 0; pos < len; pos++) {
		err_ct = 0;
		while(read(fd, &(buf[pos]), 1) != 1) {
			usleep(1000);
			err_ct++;	
			if(err_ct > 10) {
				printf("Unable to read!\n");
				break;
			}
		}
#ifdef DEBUG_PRINT
		printf("%02X ", buf[pos]);
		if (pos % 16 == 15) {
			printf("\n");
		}
#endif
	}
#ifdef DEBUG_PRINT
	printf("\n");
#endif

	return;	
}

/* SPI Wait for a write to get an expected response */
void spi_wait(int fd, uint8_t *cmd, uint16_t len,  char *resp_exp, uint16_t resp_len, char *err) {
	int ct, rd_ct, err_ct = 0;
	uint16_t write_bytes, read_bytes;
	uint8_t bytes[3];
	char *buf;
	TRACE_MSG;
	buf = (char *)malloc(resp_len * sizeof(char));
	for(ct = 0; ct < 25; ct++) {
		// Initiate the WR_RD operation
		ser_cmd(fd, BP_SPI_WR_RD);
		// Send the read/write length and command bytes
		write_bytes = len; read_bytes = resp_len;
		s2b(write_bytes, bytes);
		ser_write(fd, (uint8_t *) bytes, 2);
		s2b(read_bytes, bytes);
		ser_write(fd, (uint8_t *) bytes, 2);
		// Send the command
		ser_write(fd, cmd, len);

		// Get the response
		check_resp(fd, "SPI CMD Write\n");

		// Get the result
		for(rd_ct=0; rd_ct < resp_len; rd_ct++) {
			err_ct = 0;
			while(read(fd, &(buf[rd_ct]), 1) != 1) {
				usleep(1000);
				err_ct++;	
				if(err_ct > 25) {
					printf("Unable to read!\n");
					break;
				}
			}
		}

		// Check it
		if(strncmp(buf, resp_exp, resp_len) == 0)
			break;
	}
	free(buf);
	return;
}

/* Execute an SPI Write command */
void spi_write(int fd, uint8_t *buf, uint16_t len, int start_addr, char *err) {
	char result;
	uint16_t write_bytes, read_bytes;
	uint8_t bytes[3];
	TRACE_MSG;
	// Set WREN to enable writing
	bytes[0] = NRF24_SPI_WREN;
	spi_cmd(fd, bytes, 1, "Enable Writing");
	
	// Initiate the WR_RD operation
	ser_cmd(fd, BP_SPI_WR_RD);
	
	// Send the read/write length and command bytes
	write_bytes = 3 + len; read_bytes = 0;
	s2b(write_bytes, bytes);
	ser_write(fd, (uint8_t *) bytes, 2);
	s2b(read_bytes, bytes);
	ser_write(fd, (uint8_t *) bytes, 2);

	// Setup the write buffer
	bytes[0] = NRF24_SPI_PROGRAM;
	ser_write(fd, bytes, 1);
	s2b(start_addr, bytes);
	ser_write(fd, bytes, 2);
	ser_write(fd, buf, len);
	usleep(200000); // wait for bp buffer
	check_resp(fd, "spi_write");

	// Check that we've completed the write command
	bytes[0] = NRF24_SPI_RDSR;
	result = 0;
	spi_wait(fd, bytes, 1,  &result, 1, "Waiting for the write to complete");
	return;	
}

/* Read from the device and output a data file
 * ip == 1 if we want to read out the info page
 */
void read_hex(int fd, int ip, char *fn) {
	uint16_t read_bytes, pos;
	uint8_t *bytes, cmd[2];
	int fout;
	TRACE_MSG;
	
	// Allocate memory
	bytes = (uint8_t *) malloc(sizeof(uint8_t) * NRF24_FLASH_SZ);

	// Set up the read operation FSR
	if(ip) {
		cmd[0] = NRF24_SPI_WRSR;
		cmd[1] = 0x08;
	} else {
		cmd[0] = NRF24_SPI_WRSR;
		cmd[1] = 0x00;
	}
	spi_cmd(fd, cmd, 2, "FSR Register Write");

	// Do the Read Operation
	printf("starting read operation\n");
	if(ip) 
		read_bytes = NRF24_INFO_SZ;
	else 
		read_bytes = NRF24_FLASH_SZ;

	for(pos=0; pos < read_bytes; pos = pos + NRF24_BLOCK_SZ) 
		spi_read(fd, &(bytes[pos]), NRF24_BLOCK_SZ, pos, "Flash Read");

	// Open the output file
	fout = open(fn, O_RDWR | O_CREAT, 0666);
	if(fout == -1) {
		printf("Unable to open the file for writing\n");
		exit(-1);
	}

	// Write out the data
	for(pos = 0; pos < read_bytes; pos++)
		write(fout, &(bytes[pos]), 1);

	// Free the memory
	free(bytes);
	
	// Close the output file
	close(fout);
	return;
}

/* Write a hex file to the device */
int write_hex(int fd, char *fn) {
	uint8_t *data;
	uint8_t *verify_data;
	uint32_t sz;
	int pos;
	uint8_t cmd_bytes[8];
	char result;
	TRACE_MSG;

	// Open the hex file
	hf_read(fn, &data, &sz);
	
	// Make sure INFEN is low so we don't erase the IP
	cmd_bytes[0] = NRF24_SPI_WRSR;
	cmd_bytes[1] = 0x00;
	spi_cmd(fd, cmd_bytes, 2, "Set INFEN");

	// Write the data by 1024 byte blocks
	for(pos = 0; pos < sz; pos = pos + NRF24_BLOCK_SZ) {
		// Set WREN to enable writing
		cmd_bytes[0] = NRF24_SPI_WREN;
		spi_cmd(fd, cmd_bytes, 1, "Enable Writing");

		// Next, we erase the block we're overwriting
		cmd_bytes[0] = NRF24_SPI_ERASE_PAGE;
		cmd_bytes[1] = pos / NRF24_BLOCK_SZ;
		spi_cmd(fd, cmd_bytes, 2, "Erase Page");
	
		// Check that we've completed the erase command
		cmd_bytes[0] = NRF24_SPI_RDSR;
		result = 0;
		spi_wait(fd, cmd_bytes, 1,  &result, 1, "Waiting for the erase to complete");

		// Write the data
		spi_write(fd, &(data[pos]), NRF24_BLOCK_SZ, pos, "Writing to Flash");
	}

	// Read out the data
	printf("Verifying flash...\n");	
	verify_data = (uint8_t *)calloc(sizeof(uint8_t), NRF24_FLASH_SZ);
	for(pos=0; pos < sz; pos = pos + NRF24_BLOCK_SZ) {
		spi_read(fd, &(verify_data[pos]), NRF24_BLOCK_SZ, pos, "Flash Read");
	}

	// Verfiy that the flash was performed correctly
	result = 0;
	for(pos = 0; pos < sz; pos++) {
		if(data[pos] != verify_data[pos]) {
			printf("Error at address: 0x%08X: 0x%02X != 0x%02X\n", pos, verify_data[pos], data[pos]);
			result = 1;
		}
	}

	// Release the used memory
	free(data);
	free(verify_data);
	
	return result;
}

/* The main program, doesn't do much */
int main(int argc, char **argv) {
	int fd;
	TRACE_MSG;
	if (argc < 2) {
		printf("usage: nrfprog PORT [FILE]\n");
		exit(1);
	}

	char *port = argv[1];
	char *fname = argv[2];

	printf("Opening the Bus Pirate UART\n");
	fd = ser_open(port);
	
	printf("Setting the Bus Pirate to Binary Mode\n");
	ser_bp_bin(fd);

	printf("Configuring SPI mode\n");
	ser_bp_spi_cfg(fd);

	printf("Backing up the info page\n");
	read_hex(fd, 1, "info_page.dat");

	// Read/Write the Hex File, if requested
	if(argc > 2) {
		// If we can't open the file, we're reading out the memory contents into it
		if(access(fname, R_OK)) {	
			printf("Reading from the device to %s\n", fname);
			read_hex(fd, 0, fname);
		// Otherwise, write it out
		} else {
			printf("Writing %s to the device\n", fname);
			if(write_hex(fd, fname) == 0)
				printf("Write Verified\n");
		}
	}

	printf("Putting the Bus Pirate back in normal operating mode\n");
	ser_bp_exit_bin(fd);

	printf("Closing the Bus Pirate\n");
	ser_close(fd);
	return 0;
}
