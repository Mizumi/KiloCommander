TARGETS = calico-firmware

all: $(TARGETS)
.PHONY: $(TARGETS)

firmware: build/calico-firmware.elf build/calico-firmware.hex build/calico-firmware.lss

CC = avr-gcc -std=c99
AVROC = avr-objcopy
AVROD = avr-objdump
AVRUP = avrdude

CFLAGS = -mmcu=atmega328p -Wall -gdwarf-2 -O3 -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -Ikilolib -Isrc/main/calico -Isrc/main/calico/firmware -ISpaceTimeVStig/src/main/virtual-stigmergy
CFLAGS += -DF_CPU=8000000

FLASH = -R .eeprom -R .fuse -R .lock -R .signature
EEPROM = -j .eeprom --set-section-flags=.eeprom="alloc,load" --change-section-lma .eeprom=0  

KILOLIB = kilolib/build/kilolib.a

driver:
	gcc -shared -o libkilobotcalicodriver.so -fPIC src/main/kiloCommander.c src/main/calico/driver/kilobotCalicoDriver.c -Isrc/main -Isrc/main/calico -Isrc/main/calico/driver

driver-mac: driver
	mv libkilobotcalicodriver.so libkilobotcalicodriver.dylib

server: driver
	gcc src/main/kiloCommanderExampleCalicoServer.c -Isrc/main -Isrc/main/calico -Isrc/main/calico/driver -L. -lkilobotcalicodriver -o server
	chmod +x server

$(KILOLIB):
	make -C kilolib build/kilolib.a

%.lss: %.elf
	$(AVROD) -d -S $< > $@

%.hex: %.elf
	$(AVROC) -O ihex $(FLASH) $< $@

%.eep: %.elf
	$(AVROC) -O ihex $(EEPROM) $< $@

%.bin: %.elf
	$(AVROC) -O binary $(FLASH) $< $@ 

build:
	mkdir -p $@

build/calico-firmware.elf: $(KILOLIB) | build
	$(CC) $(CFLAGS) -o $@ SpaceTimeVStig/src/main/virtual-stigmergy/vs.c src/main/calico/firmware/kilobotCalicoFirmwareDefault.c src/main/calico/firmware/kilobotCalicoFirmwareHelper.c $(KILOLIB)

clean:
	rm -rf $(KILOLIB)
	rm -rf build
	rm -rf server
	rm -rf libkilobotcalicodriver.so
