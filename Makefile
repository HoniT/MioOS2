BUILD = $(CURDIR)/build
SRC   = $(CURDIR)/src
UACPI_DIR = $(CURDIR)/libs/uACPI

INCLUDE = -I $(SRC)/include -I $(UACPI_DIR)/include

COMMON_FLAGS = $(INCLUDE) -g -Wall -O2 -ffreestanding -m64 -mcmodel=kernel \
               -mno-red-zone -mgeneral-regs-only -ffunction-sections -fdata-sections \
			   -DUACPI_USE_BUILTIN_STRING

CC  = x86_64-elf-gcc
CXX = x86_64-elf-g++
LD  = x86_64-elf-ld
OBJCOPY = x86_64-elf-objcopy
NASM = nasm

CXX_FLAGS = $(COMMON_FLAGS) -fno-use-cxa-atexit -fno-exceptions -fno-rtti -fno-pic \
            -fno-asynchronous-unwind-tables -fno-threadsafe-statics
C_FLAGS   = $(COMMON_FLAGS)
DEP_FLAGS = -MMD -MP

CPP_SOURCES = $(shell find $(SRC) -name "*.cpp")
ASM_SOURCES = $(shell find $(SRC) -name "*.asm")
C_SOURCES   = $(shell find $(UACPI_DIR)/source -name "*.c")

OS_ELF = $(CURDIR)/iso/boot/mio_os.elf

CPP_OBJECTS = $(patsubst $(SRC)/%.cpp,$(BUILD)/%.o,$(CPP_SOURCES))
ASM_OBJECTS = $(patsubst $(SRC)/%.asm,$(BUILD)/%.o,$(ASM_SOURCES))
# Output uACPI objects into their own build subdirectory
C_OBJECTS   = $(patsubst $(UACPI_DIR)/source/%.c,$(BUILD)/uacpi/%.o,$(C_SOURCES))

DEPS = $(CPP_OBJECTS:.o=.d) $(C_OBJECTS:.o=.d)

all: $(OS_ELF)

# Added $(C_OBJECTS) and --gc-sections to strip out unused uACPI features
$(OS_ELF): $(ASM_OBJECTS) $(CPP_OBJECTS) $(C_OBJECTS)
	@mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -T $(SRC)/kernel/linker.ld --gc-sections -o $@ \
		$(ASM_OBJECTS) $(CPP_OBJECTS) $(C_OBJECTS)

$(BUILD)/%.o: $(SRC)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXX_FLAGS) $(DEP_FLAGS) -std=c++23 -c $< -o $@

$(BUILD)/%.o: $(SRC)/%.asm
	@mkdir -p $(dir $@)
	$(NASM) -f elf64 -g $< -o $@

# Compilation rule specifically for uACPI
$(BUILD)/uacpi/%.o: $(UACPI_DIR)/source/%.c
	@mkdir -p $(dir $@)
	$(CC) $(C_FLAGS) $(DEP_FLAGS) -std=c11 -c $< -o $@

clean:
	rm -rf $(BUILD)
	mkdir -p $(BUILD)
	rm -f $(OS_ELF) iso/mio_os.iso

-include $(DEPS)

.PHONY: all clean