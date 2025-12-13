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

install: product
	mkdir -p install
	if [ -d data ]; then mkdir -p install/data && cp -r data/. install/data/; fi
ifeq ($(HOST),sdl2)
	cp build/$(PRODUCT) install/$(PRODUCT)
else
	mkdir -p install/auto
	cp build/$(PRODUCT).tos install/$(PRODUCT).tos
	cp build/$(PRODUCT).tos install/auto/$(PRODUCT).prg
	if [ -d other ]; then cp -r other/* install/; fi
endif

clean:
	$(MAKE) -C $(TOYBOX) clean
	rm -rf build
	rm -rf install

.DEFAULT_GOAL:=all
.PHONY: all toybox install cracked clean

