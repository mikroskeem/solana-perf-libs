/*
 * Test harness for sha256_ni_transform against OpenSSL's SHA256
 *
 * Compile on AMD Zen (with SHA extension) using:
 *   gcc -O3 -Wall -msha -msse4.1 -mssse3 test_sha256_ni.c -lcrypto -o test_sha256_ni
 *
 * or, if you used per-function attributes in your sha256_ni_transform, just:
 *   gcc -O3 -Wall test_sha256_ni.c -lcrypto -o test_sha256_ni
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <openssl/sha.h>

#include "sha256.h"

extern void sha256_ni_transform(uint8_t data[SHA256_BLOCK_SIZE]);

/* Helper: print hex */
static void print_hex(const uint8_t *data, size_t len) {
    for (size_t i = 0; i < len; i++)
        printf("%02x", data[i]);
}

/* Compute reference SHA256 (single block input, 32 bytes) */
static void ref_sha256(const uint8_t in[32], uint8_t out[32]) {
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, in, 32);
    SHA256_Final(out, &ctx);
}

/* Compute repeated PoH style hash using reference OpenSSL */
static void ref_sha256_repeated(const uint8_t in[32], uint8_t out[32], int iterations) {
    uint8_t tmp[32];
    memcpy(tmp, in, 32);
    for (int i = 0; i < iterations; i++) {
        ref_sha256(tmp, tmp);
    }
    memcpy(out, tmp, 32);
}

/* Test single case */
static int test_case(const char *label, const uint8_t *input, int iterations) {
    uint8_t ref[32];
    uint8_t test[32];

    memcpy(test, input, 32);
    for (int i = 0; i < iterations; i++) {
        sha256_ni_transform(test);
    }
    ref_sha256_repeated(input, ref, iterations);

    int ok = memcmp(ref, test, 32) == 0;
    printf("[%s] iterations=%d: %s\n", label, iterations, ok ? "OK" : "FAIL");
    if (!ok) {
        printf(" expected: "); print_hex(ref, 32); printf("\n");
        printf(" got:      "); print_hex(test, 32); printf("\n");
    }
    return ok ? 0 : 1;
}

int main(void) {
    srand((unsigned)time(NULL));
    int fails = 0;

    /* Zero input, 1 iteration */
    uint8_t zeros[32] = {0};
    fails += test_case("zeros", zeros, 1);

    /* Zero input, 10 iterations */
    fails += test_case("zeros", zeros, 10);

    /* Zero input, 1000 iterations */
    fails += test_case("zeros", zeros, 1000);

    /* Random inputs */
    for (int t = 0; t < 10; t++) {
        uint8_t rnd[32];
        for (int i = 0; i < 32; i++)
            rnd[i] = rand() & 0xFF;
        char label[32];
        snprintf(label, sizeof(label), "random%d", t);
        fails += test_case(label, rnd, 1);
    }

    printf("----\nResult: %s\n", fails ? "FAIL" : "PASS");
    return fails ? 1 : 0;
}

