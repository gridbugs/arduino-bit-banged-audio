# Arduino Bit-Banged Audio

## Useful Commands

### Flash an Arduino Nano
```
make && sudo avrdude -P /dev/ttyUSB0 -c arduino -p m328p -U flash:w:hello.elf
```

### Connect USB serial console
```
sudo picocom -b9600 /dev/ttyUSB0
```
