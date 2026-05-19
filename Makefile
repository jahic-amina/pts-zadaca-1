PROJECT_NAME := main

export OUTPUT_FILENAME
MAKEFILE_NAME := $(MAKEFILE_LIST)
MAKEFILE_DIR := $(dir $(MAKEFILE_NAME) ) 

SDK_ROOT = ../../../
TEMPLATE_PATH = ../../../components/toolchain/gcc
NRFJPROG = ../../../nRF5x-Command-Line-Tools_9_3_1_Linux-x86_64/nrfjprog/


GNU_INSTALL_ROOT = ../../../../gcc-arm-none-eabi-4_8-2014q2
GNU_VERSION = 4.8.4
GNU_PREFIX = arm-none-eabi


MK := mkdir
RM := rm -rf

#echo suspend
ifeq ("$(VERBOSE)","1")
NO_ECHO := 
else
NO_ECHO := @
endif

# Toolchain commands
CC              := '$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-gcc'
AS              := '$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-as'
AR              := '$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-ar' -r
LD              := '$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-ld'
NM              := '$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-nm'
OBJDUMP         := '$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-objdump'
OBJCOPY         := '$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-objcopy'
SIZE            := '$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-size'

#function for removing duplicates in a list
remduplicates = $(strip $(if $1,$(firstword $1) $(call remduplicates,$(filter-out $(firstword $1),$1))))

#source common to all targets
C_SOURCE_FILES += main.c
C_SOURCE_FILES += uart.c
C_SOURCE_FILES += misc.c
C_SOURCE_FILES += delay.c
C_SOURCE_FILES += radio.c
C_SOURCE_FILES += rtc.c
C_SOURCE_FILES += packet.c
C_SOURCE_FILES += $(SDK_ROOT)components/toolchain/system_nrf52.c



#assembly files common to all targets
ASM_SOURCE_FILES  = gcc_startup_nrf52.s

#includes common to all targets
INC_PATHS  = -I./
INC_PATHS += -I$(abspath $(SDK_ROOT)components/device)
INC_PATHS += -I$(abspath $(SDK_ROOT)components/toolchain)
INC_PATHS += -I$(abspath $(SDK_ROOT)components/toolchain/cmsis/include)
INC_PATHS += -I$(abspath $(SDK_ROOT)components/toolchain/gcc)







# Libraries common to all targets
INC_PATHS += \

OBJECT_DIRECTORY = _build
LISTING_DIRECTORY = $(OBJECT_DIRECTORY)
OUTPUT_BINARY_DIRECTORY = $(OBJECT_DIRECTORY)

# Sorting removes duplicates
BUILD_DIRECTORIES := $(sort $(OBJECT_DIRECTORY) $(OUTPUT_BINARY_DIRECTORY) $(LISTING_DIRECTORY) )

#flags common to all targets
CFLAGS += -DNRF52840_XXAA
CFLAGS += -DBOARD_PCA10056
CFLAGS += -D__HEAP_SIZE=0
CFLAGS += -D__STACK_SIZE=2048
CFLAGS += -mcpu=cortex-m4
CFLAGS += -mthumb -mabi=aapcs --std=gnu99
CFLAGS += -Wall -O3 -g3
CFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
CFLAGS += -ffunction-sections -fdata-sections -fno-strict-aliasing
CFLAGS += -fno-builtin --short-enums

LDFLAGS += -Xlinker -Map=$(LISTING_DIRECTORY)/$(OUTPUT_FILENAME).map
LDFLAGS += -mthumb -mabi=aapcs -L $(TEMPLATE_PATH) -Tnrf52840_xxaa.ld
LDFLAGS += -mcpu=cortex-m4
LDFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
LDFLAGS += -Wl,--gc-sections
LDFLAGS += --specs=nano.specs -lc -lnosys

# Assembler flags
ASMFLAGS += -x assembler-with-cpp
ASMFLAGS += -g3
ASMFLAGS += -mcpu=cortex-m4
ASMFLAGS += -mthumb -mabi=aapcs
ASMFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
ASMFLAGS += -DNRF52840_XXAA
ASMFLAGS += -D__HEAP_SIZE=0
ASMFLAGS += -D__STACK_SIZE=2048

#default target - first one defined
default: nrf52832_xxaa_s132

#building all targets
all: 
	$(NO_ECHO)$(MAKE) -f $(MAKEFILE_NAME) -C $(MAKEFILE_DIR) -e cleanobj
	$(NO_ECHO)$(MAKE) -f $(MAKEFILE_NAME) -C $(MAKEFILE_DIR) -e nrf52832_xxaa_s132


C_SOURCE_FILE_NAMES = $(notdir $(C_SOURCE_FILES))
C_PATHS = $(call remduplicates, $(dir $(C_SOURCE_FILES) ) )
C_OBJECTS = $(addprefix $(OBJECT_DIRECTORY)/, $(C_SOURCE_FILE_NAMES:.c=.o) )

ASM_SOURCE_FILE_NAMES = $(notdir $(ASM_SOURCE_FILES))
ASM_PATHS = $(call remduplicates, $(dir $(ASM_SOURCE_FILES) ))
ASM_OBJECTS = $(addprefix $(OBJECT_DIRECTORY)/, $(ASM_SOURCE_FILE_NAMES:.s=.o) )

vpath %.c $(C_PATHS)
vpath %.s $(ASM_PATHS)

OBJECTS = $(C_OBJECTS) $(ASM_OBJECTS)

nrf52832_xxaa_s132: OUTPUT_FILENAME := main
nrf52832_xxaa_s132: LINKER_SCRIPT=nrf52832.ld
nrf52832_xxaa_s132: $(BUILD_DIRECTORIES) $(OBJECTS)
	@echo Linking target: $(OUTPUT_FILENAME).out
	$(NO_ECHO)$(CC) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out
	$(NO_ECHO)$(MAKE) -f $(MAKEFILE_NAME) -C $(MAKEFILE_DIR) -e finalize

## Create build directories
$(BUILD_DIRECTORIES):
	echo $(MAKEFILE_NAME)
	$(MK) $@

# Create objects from C SRC files
$(OBJECT_DIRECTORY)/%.o: %.c
	@echo Compiling file: $(notdir $<)
	$(NO_ECHO)$(CC) $(CFLAGS) $(INC_PATHS) -c -o $@ $<

# Assemble files
$(OBJECT_DIRECTORY)/%.o: %.s
	@echo Compiling file: $(notdir $<)
	$(NO_ECHO)$(CC) $(ASMFLAGS) $(INC_PATHS) -c -o $@ $<


# Link
$(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out: $(BUILD_DIRECTORIES) $(OBJECTS)
	@echo Linking target: $(OUTPUT_FILENAME).out
	$(NO_ECHO)$(CC) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out


## Create binary .bin file from the .out file
$(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).bin: $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out
	@echo Preparing: $(OUTPUT_FILENAME).bin
	$(NO_ECHO)$(OBJCOPY) -O binary $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).bin

## Create binary .hex file from the .out file
$(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).hex: $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out
	@echo Preparing: $(OUTPUT_FILENAME).hex
	$(NO_ECHO)$(OBJCOPY) -O ihex $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).hex

finalize: genbin genhex echosize

genbin:
	@echo Preparing: $(OUTPUT_FILENAME).bin
	$(NO_ECHO)$(OBJCOPY) -O binary $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).bin
	
## Create binary .hex file from the .out file
genhex: 
	@echo Preparing: $(OUTPUT_FILENAME).hex
	$(NO_ECHO)$(OBJCOPY) -O ihex $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).hex

echosize:
	$(NO_ECHO)$(SIZE) $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).out
	$(NO_ECHO)ls -l $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).bin

clean:
	$(RM) $(BUILD_DIRECTORIES)

cleanobj:
	$(RM) $(BUILD_DIRECTORIES)/*.o

flash: $(OUTPUT_DIRECTORY)/nrf52832_xxaa.hex
	@echo Flashing: $<
	$(NRFJPROG)nrfjprog --program $< -f nrf52 --sectorerase
	$(NRFJPROG)nrfjprog --reset -f nrf52

# Flash softdevice
flash_softdevice:
	@echo Flashing: s132_nrf52_5.0.0-1.alpha_softdevice.hex
	$(NRFJPROG)nrfjprog --program $(SDK_ROOT)/components/softdevice/s132/hex/s132_nrf52_5.0.0-1.alpha_softdevice.hex -f nrf52 --sectorerase 
	$(NRFJPROG)nrfjprog --reset -f nrf52


#upload-bin: $(BUILD_DIRECTORIES)/$(OUTPUT_FILENAME).bin
	#$(OPENOCD) -c "program $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_FILENAME).bin  0x00020000  verify; reset; exit"
	

#upload: build/main.hex
	#$(OPENOCD) -c "program build/main.hex verify; reset; exit"	


#upload-softdevice: softdevice/s132_nrf52_5.0.0-1.alpha_softdevice.hex
	#$(OPENOCD) -c init -c "reset init" -c halt -c "nrf52 mass_erase; sleep 500 ;program softdevice/s132_nrf52_5.0.0-1.alpha_softdevice.hex 0x0 ; verify_image oftdevice/s132_nrf52_5.0.0-1.alpha_softdevice.hex 0; shutdown"
 



upload-softdevice:
	@echo Flashing: s132_nrf52_5.0.0_softdevice.hex
	$(NRFJPROG)nrfjprog --program softdevice/s132_nrf52_5.0.0_softdevice.hex -f nrf52 --sectorerase
	$(NRFJPROG)nrfjprog --reset

erase:
	@echo Mass erase: 
	$(NRFJPROG)nrfjprog -f nrf52 --eraseall
	$(NRFJPROG)nrfjprog --reset

upload: $(OUTPUT_BINARY_DIRECTORY)/main.hex
	@echo Flashing: $(OUTPUT_BINARY_DIRECTORY)/main.hex
	$(NRFJPROG)nrfjprog --program $(OUTPUT_BINARY_DIRECTORY)/main.hex  --verify --sectorerase -f nrf52 
	$(NRFJPROG)nrfjprog --reset		


merge:
	srec_cat softdevice/s132_nrf52_5.0.0_softdevice.hex -intel build/main.hex -intel -o firmware.hex -intel --line-length=44
	
upload-merged: 
	@echo Flashing: firmware.hex
	$(NRFJPROG)nrfjprog --program firmware.hex  --verify --sectorerase -f nrf52 
	$(NRFJPROG)nrfjprog --reset		

upload-led-test: 
	@echo Flashing: ledtestfirmware.hex
	$(NRFJPROG)nrfjprog --program ledtestfirmware.hex  --verify --sectorerase -f nrf52 
	$(NRFJPROG)nrfjprog --reset		
