# Common targets for building a product with toybox
# These directories are assumed to exist:
#   * src 		- All source code as *.cpp, and *.S if bulding for target
#   * include 	- All include files.
#   * data		- Data to be copied if building for install
# These directories are creates
#	* build 	- Temporary build files.
#	* install	- product ready to install / run

CFLAGS+=-I ./include

SOURCES=$(shell find src -name "*.cpp")
OBJECTS=$(patsubst src/%.cpp,build/%.o,$(SOURCES))
DEPS=$(OBJECTS:.o=.d)

ifneq ($(HOST),sdl2)
	ASM_SOURCES=$(shell find src -name "*.S")
	SOURCES+=$(ASM_SOURCES)
	OBJECTS+=$(patsubst src/%.S,build/%.o,$(ASM_SOURCES))
endif

# Graphics conversion: PNG -> ILBM (requires png2ilbm)
PNG_SOURCES=$(wildcard assets/*.png)
LBM_DATA=$(patsubst assets/%.png,data/%.lbm,$(PNG_SOURCES))

# Audio conversion: WAV -> 8SVX (requires sox)
WAV_SOURCES=$(wildcard assets/*.wav)
SVX_DATA=$(patsubst assets/%.wav,data/%.svx,$(WAV_SOURCES))

# Music conversion: TPM -> SNDH (requires tpmtool)
TPM_SOURCES=$(sort $(wildcard assets/*.tpm))
SND_DATA=$(if $(TPM_SOURCES),data/music.snd)

-include $(DEPS)

all: product

toybox:
	$(MAKE) -C $(TOYBOX) libtoybox.a $(if $(HOST),HOST=$(HOST))

product: toybox $(OBJECTS)
ifeq ($(HOST),sdl2)
	$(CC) $(OBJECTS) $(LDFLAGS) -o build/$(PRODUCT)
else
	$(CC) $(LIBCMINIOBJ)/crt0.o $(OBJECTS) $(LDFLAGS) -o build/$(PRODUCT).tos
endif

build/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@

build/%.o: src/%.S
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@

data/%.lbm: assets/%.png
	@mkdir -p $(dir $@)
	png2ilbm $$(if [ -f $<.args ]; then cat $<.args; fi) $< $@

data/%.svx: assets/%.wav
	@mkdir -p $(dir $@)
	sox $< -t 8svx -r 12517 -c 1 -e signed-integer -b 8 $@ channels 1 rate -v -L 12517 dither -s

data/music.snd: $(TPM_SOURCES)
	@mkdir -p $(dir $@)
	tpmtool -e -n -o $@ $^

install: product $(LBM_DATA) $(SVX_DATA) $(SND_DATA)
	mkdir -p install
	if [ -d data ]; then mkdir -p install/data && cp -r data/. install/data/; fi
ifeq ($(HOST),sdl2)
	cp build/$(PRODUCT) install/$(PRODUCT)
else
	mkdir -p install/auto
	cp build/$(PRODUCT).tos install/$(PRODUCT).TOS
	$(MAKE) -C $(TOYBOX)/tools/auto_trampoline PRODUCT=$(PRODUCT)
	cp $(TOYBOX)/tools/auto_trampoline/build/$(PRODUCT).prg install/auto/$(PRODUCT).PRG
	if [ -d other ]; then cp -r other/* install/; fi
endif

clean:
	$(MAKE) -C $(TOYBOX) clean
	$(MAKE) -C $(TOYBOX)/tools/auto_trampoline clean
	rm -rf build
	rm -rf install

.DEFAULT_GOAL:=all
.PHONY: all toybox install audio cracked clean

