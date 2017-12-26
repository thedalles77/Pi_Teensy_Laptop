# Pi_Teensy_Laptop
Convert a Sony Vaio into a Raspberry Pi and Teensy laptop

The PDF file gives a complete description of the project with pictures and parts list. Battery operation is now included.
The folder contains the Eagle files for a circuit board that connects the Teensy ++2.0 to the keyboard FPC connector.
The .ino file is the Teensyduino C code that scans the keyboard, and touchpad, and controls the video card.
The read_battery.c file is run on the Raspberry Pi to read the registers in the battery with a bit-bang SMBus.

A short video showing this laptop project (prior to battery operation is at this 
YouTube address: https://www.youtube.com/watch?v=Gh3VTcLiJrI

