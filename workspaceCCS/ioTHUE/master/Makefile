NAME = IotHueMaster
DEVICE= msp430g2553
# PROJECT = $(NAME)-$(DEVICE)
PROJECT = $(NAME)
CC = msp430-gcc -std=c99 -Os -mmcu=$(DEVICE)
MSPDEBUG = mspdebug rf2500
SRC = *.c ../msprf24/*.c ../msprf24/*.h

all: $(PROJECT).elf

$(PROJECT).elf: $(SRC)
	$(CC) -o $(PROJECT).elf $(SRC)

write: $(PROJECT).elf
	$(MSPDEBUG) "prog $(PROJECT).elf"

run: $(PROJECT).elf
	$(MSPDEBUG) "prog $(PROJECT).elf" "reset" "run"

reset:
	$(MSPDEBUG) "reset"

erase:
	$(MSPDEBUG) "erase"

clean:
	rm -f *.elf
