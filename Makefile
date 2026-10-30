# visor - a tiny type-2 hypervisor on the Linux KVM API
CC      ?= cc
NASM    ?= nasm
BUILD   := build

CFLAGS  := -std=gnu11 -O2 -g -Wall -Wextra -Werror -Isrc

SRCS    := $(wildcard src/*.c)
VISOR   := $(BUILD)/visor

GUEST_SRCS := $(wildcard guests/*.asm)
GUEST_BINS := $(patsubst guests/%.asm,$(BUILD)/guests/%.img,$(GUEST_SRCS))

all: $(VISOR) $(GUEST_BINS)

$(VISOR): $(SRCS)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $(SRCS) -o $@

$(BUILD)/guests/%.img: guests/%.asm
	@mkdir -p $(BUILD)/guests
	$(NASM) -f bin $< -o $@

test: $(VISOR) $(GUEST_BINS)
	./tests/unit/run_tests.sh
	./tests/run-guest-tests.sh

clean:
	rm -rf $(BUILD)

.PHONY: all test clean
