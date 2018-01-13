#ifndef __BP_H_
#define __BP_H_

/* Bus Pirate Binary Commands */

// Binary protocol version #
#define BP_BINARY_STRING	"BBIO1"

// All commands respond with 0x01 as a success message
#define BP_RESP_SUCCESS		0x01

// Commands
#define BP_BIN_RESET		0x00
#define BP_SPI			0x01
	#define BP_SPI_STRING	"SPI1"
	// SPI Commands
	#define BP_SPI_CS(x)	(0x02 | (x & 1))
	#define BP_SPI_PERF(x)	(0x40 | (x & 0xF))
		#define BP_SPI_PERF_POWER	0x08
		#define BP_SPI_PERF_PULLUPS	0x04
		#define BP_SPI_PERF_AUX		0x02
		#define BP_SPI_PERF_CS		0x01
	#define BP_SPI_SPEED(x)	(0x60 | (x & 0x7))
		#define BP_SPI_SPEED_30K 	0x00
		#define BP_SPI_SPEED_125K	0x01
		#define BP_SPI_SPEED_250K	0x02
		#define BP_SPI_SPEED_1M		0x03
		#define BP_SPI_SPEED_2M		0x04
		#define BP_SPI_SPEED_2M6K	0x05
		#define BP_SPI_SPEED_4M		0x06
		#define BP_SPI_SPEED_8M		0x07
	#define BP_SPI_CFG(x)	(0x80 | (x & 0xF))
		#define BP_SPI_CFG_3v3		0x08
		#define BP_SPI_CFG_CKP		0x04
		#define BP_SPI_CFG_CKE		0x02
		#define BP_SPI_CFG_SMP		0x01
	#define BP_SPI_WR_RD 		0x04
	#define BP_SPI_WR_RD_NO_CS 		0x05

#define BP_I2C		0x02
#define BP_UART		0x03
#define BP_1_WIRE	0x04
#define BP_RAW_WIRE	0x05
#define BP_JTAG		0x06
// Reserved
#define BP_RESET	0x0F
// Bus Pirate Self Tests

// Bus Pirate Pin Configuration
#define BP_PINIO(x)	(0x40 | x)
#define BP_PINIO_AUX	0x10
#define BP_PINIO_MOSI	0x08
#define BP_PINIO_CLK	0x04
#define BP_PINIO_MISO	0x02
#define BP_PINIO_CS	0x01

// Bus Pirate Set Command, takes an OR'd list of BP_SET_X values
#define BP_SET(x)	(0x80 | x)
#define BP_SET_POWER	0x40
#define BP_SET_PULLUP	0x20
#define BP_SET_AUX	0x10
#define BP_SET_MOSI	0x08
#define BP_SET_CLK	0x04
#define BP_SET_MISO	0x02
#define BP_SET_CS	0x01


#endif /* __BP_H_ */
