#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

void poh_verify_many_simd_avx2(uint8_t* hashes, const uint64_t* num_hashes);
void poh_verify_many_simd_avx512skx(uint8_t* hashes, const uint64_t* num_hashes);

static void print_hash(const char* label, uint8_t hash[32]) {
    printf("%s: ", label);
    for (int i = 0; i < 32; i++) {
        printf("%02x", hash[i]);
    }
    printf("\n");
}

static double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

int main() {
    printf("Testing AVX2 path (8 hashes):\n");
    uint8_t hashes_avx2[8 * 32];
    uint64_t num_hashes_avx2[8];
    
    // Initialize test data
    for (int i = 0; i < 8; i++) {
        memset(&hashes_avx2[i * 32], i, 32);  // Simple pattern
        num_hashes_avx2[i] = 1000;  // 1000 iterations each
    }
    
    print_hash("First hash before", &hashes_avx2[0]);
    
    double start = get_time();
    poh_verify_many_simd_avx2(hashes_avx2, num_hashes_avx2);
    double elapsed = get_time() - start;
    
    print_hash("First hash after", &hashes_avx2[0]);
    printf("Time: %.3f ms for 8,000 SHA-256 operations\n", elapsed * 1000);
    printf("Throughput: %.0f hashes/sec\n\n", 8000 / elapsed);
    
    // Test AVX512 path (16 hashes)
    printf("Testing AVX512 path (16 hashes):\n");
    uint8_t hashes_avx512[16 * 32];
    uint64_t num_hashes_avx512[16];
    
    // Initialize test data
    for (int i = 0; i < 16; i++) {
        memset(&hashes_avx512[i * 32], i, 32);
        num_hashes_avx512[i] = 1000;
    }
    
    print_hash("First hash before", &hashes_avx512[0]);
    
    start = get_time();
    poh_verify_many_simd_avx512skx(hashes_avx512, num_hashes_avx512);
    elapsed = get_time() - start;
    
    print_hash("First hash after", &hashes_avx512[0]);
    printf("Time: %.3f ms for 16,000 SHA-256 operations\n", elapsed * 1000);
    printf("Throughput: %.0f hashes/sec\n\n", 16000 / elapsed);
    
    return 0;
}
