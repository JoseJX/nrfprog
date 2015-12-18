# BusPirate NRF24LE1 programmer
This is a simple programmer for the nRF24LE1 using the Bus Pirate as the SPI
device.

## building
```
make clean
make
```

### debugging
for debug build, use `DEBUG=1 make`

## usage
`nrfprog BUSPIRATE_PORT [filename]`

Running the program directly (./nrfprog) will dump the InfoPage. Put this some 
where safe, as this contains information unique to your nRF24LE1. Running the
program, but passing a .hex file as a parameter will make the programmer write
the .hex file to the device.

## references
 * bus pirate binary modes: http://dangerousprototypes.com/docs/SPI_%28binary%29
 * nrf24le1 datasheet: https://www.nordicsemi.com/kor/nordic/content_download/2443/29442/file/nRF24LE1_Product_Specification_rev1_6.pdf

## hardware

| bus pirate  | nrf 24pin | nrf 32 pin | nrf 48 pin |
|-------------|-----------|------------|------------|
| GND (brown) | GND       | GND        | GND        |
| 3V3 (red)   | PROG      | PROG       | PROG       |
| AUX (blue)  | RESET     | RESET      | RESET      |
| CS          | P0.5      | P1.1       | P2.0       |
| MISO        | P0.4      | P1.0       | P1.6       |
| MOSI        | P0.3      | P0.7       | P1.5       |
| CLK         | P0.2      | P0.5       | P1.2       |
