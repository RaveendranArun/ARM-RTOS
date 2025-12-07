
CC=arm-none-eabi-gcc
MACH=cortex-m4
CFLAGS= -mcpu=$(MACH) -mthumb -mfloat-abi=soft -std=gnu11 -Wall -O0 -g3 -O0 -ffreestanding -fno-builtin
LDFLAGS= -mcpu=$(MACH) -mthumb -mfloat-abi=soft --specs=nano.specs -T linker/stm32_ls.ld -Wl,-Map=build/final.map
LDFLAGS_SH= -mcpu=$(MACH) -mthumb -mfloat-abi=soft --specs=rdimon.specs -T linker/stm32_ls.ld -Wl,-Map=build/final_sh.map


# Directories
SRC_DIR = src
STARTUP_DIR = startup
INCLUDE_DIR = include
BUILD_DIR = build

INCLUDES = -I$(INCLUDE_DIR)

SOURCES = $(SRC_DIR)/main.c $(STARTUP_DIR)/stm32_startup.c $(SRC_DIR)/syscalls.c
SOURCES_SEMI = $(SRC_DIR)/main.c $(STARTUP_DIR)/stm32_startup.c

OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(notdir $(SOURCES)))
OBJECTS_SEMI = $(patsubst %.c,$(BUILD_DIR)/%.o,$(notdir $(SOURCES_SEMI)))

all: $(BUILD_DIR) $(BUILD_DIR)/final.elf image_dir copy_image

semi: $(BUILD_DIR) $(BUILD_DIR)/final_sh.elf image_dir copy_image_sh

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

image_dir:
	mkdir -p image

copy_image: $(BUILD_DIR)/final.elf
	cp -f build/final.elf build/final.map image/
	arm-none-eabi-objcopy -O binary build/final.elf image/final.bin
	arm-none-eabi-objcopy -O ihex build/final.elf image/final.hex

copy_image_sh: $(BUILD_DIR)/final_sh.elf
	cp -f build/final_sh.elf build/final_sh.map image/
	arm-none-eabi-objcopy -O binary build/final_sh.elf image/final_sh.bin
	arm-none-eabi-objcopy -O ihex build/final_sh.elf image/final_sh.hex

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: $(STARTUP_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/final.elf: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILD_DIR)/final_sh.elf: $(OBJECTS_SEMI)
	$(CC) $(LDFLAGS_SH) -o $@ $^

clean:
	rm -rf $(BUILD_DIR) image 

load:
	openocd -f ./misc/openocd/board_cfg/stm32f4discovery.cfg

.PHONY: all clean load image_dir copy_image semi copy_image_sh



