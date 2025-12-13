# Auto folder trampoline builder
# Usage: make PRODUCT=name
# Builds a small PRG that launches ..\name.TOS via Pexec()

# Allow clean without PRODUCT
ifeq ($(filter clean,$(MAKECMDGOALS)),)
ifndef PRODUCT
$(error PRODUCT must be specified)
endif
endif

CC=/opt/cross-mint/bin/m68k-atari-mintelf-c++
CFLAGS=-m68000 -nostdlib -s

all: build/$(PRODUCT).prg

build/filename.S: | build
	@echo '	.data' > $@
	@echo '	.globl filename' >> $@
	@echo 'filename:' >> $@
	@echo '	.asciz "$(PRODUCT).TOS"' >> $@

build/trampoline.o: src/trampoline.S | build
	$(CC) $(CFLAGS) -c $< -o $@

build/filename.o: build/filename.S
	$(CC) $(CFLAGS) -c $< -o $@

build/$(PRODUCT).prg: build/trampoline.o build/filename.o
	$(CC) $(CFLAGS) $^ -o $@

build:
	mkdir -p build

clean:
	rm -rf build

.PHONY: all clean
