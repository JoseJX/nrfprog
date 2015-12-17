This is a simple programmer for the nRF24LE1 using the Bus Pirate as the SPI
device.

## building
Type "make" to build the source.

## usage
`nrfprog PORT [filename]`
Running the program directly (./nrfprog) will dump the InfoPage. Put this some 
where safe, as this contains information unique to your nRF24LE1. Running the
program, but passing a .hex file as a parameter will make the programmer write
the .hex file to the device.

## debugging
for debug build, use `DEBUG=1 make`

## references
 * bus pirate binary modes: http://dangerousprototypes.com/docs/SPI_%28binary%29
 * nrf24le1 datasheet: https://www.nordicsemi.com/kor/nordic/content_download/2443/29442/file/nRF24LE1_Product_Specification_rev1_6.pdf

## mods
forked from repo JoseJX/nrfprog
mods by zerog2k:
 * fixes to buspirate not reading info page and main flash blocks
 * more debugging
 * pass in serial port as param
