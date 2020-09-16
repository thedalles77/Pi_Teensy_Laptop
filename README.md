# Pi_Teensy_Laptop
Convert a Sony Vaio into a Raspberry Pi and Teensy laptop

The PDF file gives a complete description of the project with pictures and parts list.
The folder contains the Eagle files for a circuit board that connects the Teensy ++2.0 to the keyboard FPC connector.
The .ino file is the Teensyduino C code that scans the keyboard, and touchpad, and controls the video card.
The read_battery.c file is run on the Raspberry Pi to read the registers in the battery with a bit-bang SMBus using 2 of the GPIO pins.
The monitor_battery.c file runs on the Raspberry Pi at startup and monitors battery state of charge every minute over the SMBus.

A short video of this laptop project is at this address: https://vimeo.com/458640649
Battery operation was added after this video was made.
