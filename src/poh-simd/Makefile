CC = gcc
DEPS := sha256.h

CFLAGS = -Wall -Werror -DNDEBUG
LDFLAGS = -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack

COMPILER_VERSION := $(shell $(CC) --version)
ifneq ('', $(findstring clang,$(COMPILER_VERSION)))
	#CFLAGS += -Wno-error=ignored-optimization-argument -DCOMPILER="$(findstring clang,$(COMPILER_VERSION))"
endif

EXPOSE_SHA256_NI_TRANSFORM = 1
ifeq ($(EXPOSE_SHA256_NI_TRANSFORM),1)
	CFLAGS += -DEXPOSE_SHA256_NI_TRANSFORM
endif

AMD_ZEN4_FLAGS := $(CFLAGS) -I. -O3 -march=znver4 -mtune=znver4 -msha -mavx2 -fPIC -funroll-loops -fprefetch-loop-arrays -falign-functions=32 -falign-loops=32
AMD_ZEN4_FLAGS += -mprefer-vector-width=256 -fomit-frame-pointer -flto -ffat-lto-objects

AMD_ZEN4_OBJ = amd-zen4
AMD_ZEN4_Objects := $(AMD_ZEN4_OBJ)/poh-verify-amd-zen4.o

OUT ?= libs

.PHONY: all run
all: $(OUT)/libpoh-simd.so
run: all

$(AMD_ZEN4_OBJ)/poh-verify-amd-zen4.o: poh-verify-amd-zen4.c $(DEPS)
	@mkdir -p $(AMD_ZEN4_OBJ)
	$(CC) $(AMD_ZEN4_FLAGS) -c $< -o $@

$(OUT)/libpoh-simd.so: $(AMD_ZEN4_Objects)
	@mkdir -p $(OUT)
	$(CC) $(AMD_ZEN4_FLAGS) $(LDFLAGS) -shared -o $@ $^

check-asm: poh-verify-amd-zen4.c
	$(CC) $(AMD_ZEN4_FLAGS) -S -o - $< | grep -E "sha256" | head -20

test: $(OUT)/libpoh-simd.so
	$(CC) -O2 -o test_poh test_poh.c -L$(OUT) -Wl,-rpath,$(OUT) -lpoh-simd
	./test_poh

test-openssl:
	$(CC) $(LDFLAGS) -o test_openssl ./test_openssl.c -O3 -Wall -msha -msse4.1 -mssse3 -L$(OUT) -Wl,-rpath,$(OUT) -lpoh-simd -lcrypto -lssl
	./test_openssl

clean:
	@rm -rf $(AMD_ZEN4_OBJ) $(OUT) ./test_poh ./test_openssl
