#ifndef __NRF24LE1_H_
#define __NRF24LE1_H_


#define NRF24_BLOCK_SZ		(512)
#define NRF24_FLASH_SZ		(32*NRF24_BLOCK_SZ)
#define NRF24_INFO_SZ		(256)

#define NRF24_SPI_WRSR		(0x01)
#define NRF24_SPI_PROGRAM	(0x02)
#define NRF24_SPI_READ		(0x03)
#define NRF24_SPI_WRDIS		(0x04)
#define NRF24_SPI_RDSR		(0x05)
#define NRF24_SPI_WREN		(0x06)
#define NRF24_SPI_ERASE_PAGE	(0x52)
#define NRF24_SPI_ERASE_ALL	(0x62)

#endif
