#include <immintrin.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "sha256.h"

// SHA-NI implementation for AMD Zen processors with SHA extensions
static inline void sha256_ni_transform(uint8_t data[SHA256_BLOCK_SIZE]) {
    __m128i state0, state1;
    __m128i msg, tmp;
    __m128i msg0, msg1, msg2, msg3;
    
    // SHA-256 initial hash values
    const __m128i ABEF_INIT = _mm_set_epi32(RC_INIT[0], RC_INIT[1], RC_INIT[4], RC_INIT[5]);
    const __m128i CDGH_INIT = _mm_set_epi32(RC_INIT[2], RC_INIT[3], RC_INIT[6], RC_INIT[7]);
    
    // Byte swap mask for endianness conversion
    const __m128i SHUF_MASK = _mm_set_epi8(12,13,14,15, 8,9,10,11, 4,5,6,7, 0,1,2,3);
    
    // Load initial state
    tmp = ABEF_INIT;
    state1 = CDGH_INIT;
    
    // Prepare working variables
    state0 = _mm_shuffle_epi32(tmp, 0xB1);          // CDAB
    state1 = _mm_shuffle_epi32(state1, 0x1B);       // EFGH -> GHEF
    tmp = _mm_alignr_epi8(tmp, state1, 8);          // ABEF
    state1 = _mm_blend_epi16(state1, tmp, 0xF0);    // CDGH
    state0 = _mm_shuffle_epi32(tmp, 0x1B);          // ABEF -> FEBA
    
    // Save initial state for final addition
    const __m128i abef_save = state0;
    const __m128i cdgh_save = state1;
    
    // Load and prepare message (32 bytes of input)
    msg0 = _mm_loadu_si128((const __m128i*)(data + 0));
    msg1 = _mm_loadu_si128((const __m128i*)(data + 16));
    msg2 = _mm_set_epi32(0, 0, 0, 0x80000000);  // Padding
    msg3 = _mm_set_epi32(0x00000100, 0, 0, 0);  // Length (256 bits)
    
    // Convert to big-endian
    msg0 = _mm_shuffle_epi8(msg0, SHUF_MASK);
    msg1 = _mm_shuffle_epi8(msg1, SHUF_MASK);
    
    // Rounds 0-3
    msg = _mm_add_epi32(msg0, _mm_load_si128((const __m128i*)&RC[0]));
    state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
    msg = _mm_shuffle_epi32(msg, 0x0E);
    state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
    
    // Rounds 4-7
    msg = _mm_add_epi32(msg1, _mm_load_si128((const __m128i*)&RC[4]));
    state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
    msg = _mm_shuffle_epi32(msg, 0x0E);
    state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
    msg0 = _mm_sha256msg1_epu32(msg0, msg1);
    
    // Rounds 8-11
    msg = _mm_add_epi32(msg2, _mm_load_si128((const __m128i*)&RC[8]));
    state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
    msg = _mm_shuffle_epi32(msg, 0x0E);
    state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
    msg1 = _mm_sha256msg1_epu32(msg1, msg2);
    
    // Rounds 12-15
    msg = _mm_add_epi32(msg3, _mm_load_si128((const __m128i*)&RC[12]));
    state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
    tmp = _mm_alignr_epi8(msg3, msg2, 4);
    msg0 = _mm_add_epi32(msg0, tmp);
    msg0 = _mm_sha256msg2_epu32(msg0, msg3);
    msg = _mm_shuffle_epi32(msg, 0x0E);
    state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
    msg2 = _mm_sha256msg1_epu32(msg2, msg3);
    
    // Rounds 16-63 (main loop)
    for (int i = 16; i < 64; i += 16) {
        // Rounds i+0 to i+3
        msg = _mm_add_epi32(msg0, _mm_load_si128((const __m128i*)&RC[i]));
        state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
        tmp = _mm_alignr_epi8(msg0, msg3, 4);
        msg1 = _mm_add_epi32(msg1, tmp);
        msg1 = _mm_sha256msg2_epu32(msg1, msg0);
        msg = _mm_shuffle_epi32(msg, 0x0E);
        state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
        msg3 = _mm_sha256msg1_epu32(msg3, msg0);
        
        // Rounds i+4 to i+7
        msg = _mm_add_epi32(msg1, _mm_load_si128((const __m128i*)&RC[i+4]));
        state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
        tmp = _mm_alignr_epi8(msg1, msg0, 4);
        msg2 = _mm_add_epi32(msg2, tmp);
        msg2 = _mm_sha256msg2_epu32(msg2, msg1);
        msg = _mm_shuffle_epi32(msg, 0x0E);
        state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
        msg0 = _mm_sha256msg1_epu32(msg0, msg1);
        
        // Rounds i+8 to i+11
        msg = _mm_add_epi32(msg2, _mm_load_si128((const __m128i*)&RC[i+8]));
        state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
        tmp = _mm_alignr_epi8(msg2, msg1, 4);
        msg3 = _mm_add_epi32(msg3, tmp);
        msg3 = _mm_sha256msg2_epu32(msg3, msg2);
        msg = _mm_shuffle_epi32(msg, 0x0E);
        state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
        msg1 = _mm_sha256msg1_epu32(msg1, msg2);
        
        // Rounds i+12 to i+15
        msg = _mm_add_epi32(msg3, _mm_load_si128((const __m128i*)&RC[i+12]));
        state1 = _mm_sha256rnds2_epu32(state1, state0, msg);
        tmp = _mm_alignr_epi8(msg3, msg2, 4);
        msg0 = _mm_add_epi32(msg0, tmp);
        msg0 = _mm_sha256msg2_epu32(msg0, msg3);
        msg = _mm_shuffle_epi32(msg, 0x0E);
        state0 = _mm_sha256rnds2_epu32(state0, state1, msg);
        msg2 = _mm_sha256msg1_epu32(msg2, msg3);
    }
    
    // Add initial hash values
    state0 = _mm_add_epi32(state0, abef_save);
    state1 = _mm_add_epi32(state1, cdgh_save);
    
    // Prepare final hash for storage
    tmp = _mm_shuffle_epi32(state0, 0x1B);        // FEBA -> ABEF
    state1 = _mm_shuffle_epi32(state1, 0xB1);     // CDGH -> GHCD
    state0 = _mm_blend_epi16(tmp, state1, 0xF0);  // ABCD
    state1 = _mm_alignr_epi8(state1, tmp, 8);     // EFGH
    
    // Convert back to little-endian and store
    state0 = _mm_shuffle_epi8(state0, SHUF_MASK);
    state1 = _mm_shuffle_epi8(state1, SHUF_MASK);
    
    _mm_storeu_si128((__m128i*)data, state0);
    _mm_storeu_si128((__m128i*)(data + 16), state1);
}

// Main entry point - Solana calls this with AVX512 (batch_size=16)
__attribute__((visibility("default")))
void poh_verify_many_simd_avx512skx(uint8_t* hashes, const uint64_t* num_hashes) {
    // Process 16 hashes
    for (int i = 0; i < 16; i++) {
        uint8_t* hash = hashes + (i * SHA256_BLOCK_SIZE);
        uint64_t iterations = num_hashes[i];
        
        // Process all iterations for this hash
        for (uint64_t j = 0; j < iterations; j++) {
            sha256_ni_transform(hash);
        }
    }
}

// Solana calls this with AVX2 (batch_size=8)
__attribute__((visibility("default")))
void poh_verify_many_simd_avx2(uint8_t* hashes, const uint64_t* num_hashes) {
    // Process 8 hashes
    for (int i = 0; i < 8; i++) {
        uint8_t* hash = hashes + (i * SHA256_BLOCK_SIZE);
        uint64_t iterations = num_hashes[i];
        
        // Process all iterations for this hash
        for (uint64_t j = 0; j < iterations; j++) {
            sha256_ni_transform(hash);
        }
    }
}

// Compatibility exports for other variants (all route to SHA-NI)
__attribute__((visibility("default")))
void poh_verify_many_simd_sse2(uint8_t* hashes, const uint64_t* num_hashes) {
    // Assume batch of 4 for SSE2
    for (int i = 0; i < 4; i++) {
        uint8_t* hash = hashes + (i * SHA256_BLOCK_SIZE);
        uint64_t iterations = num_hashes[i];
        for (uint64_t j = 0; j < iterations; j++) {
            sha256_ni_transform(hash);
        }
    }
}

__attribute__((visibility("default")))
void poh_verify_many_simd_sse4(uint8_t* hashes, const uint64_t* num_hashes) {
    // Assume batch of 4 for SSE4
    for (int i = 0; i < 4; i++) {
        uint8_t* hash = hashes + (i * SHA256_BLOCK_SIZE);
        uint64_t iterations = num_hashes[i];
        for (uint64_t j = 0; j < iterations; j++) {
            sha256_ni_transform(hash);
        }
    }
}

__attribute__((visibility("default")))
void poh_verify_many_simd_avx1(uint8_t* hashes, const uint64_t* num_hashes) {
    // Assume batch of 8 for AVX1
    for (int i = 0; i < 8; i++) {
        uint8_t* hash = hashes + (i * SHA256_BLOCK_SIZE);
        uint64_t iterations = num_hashes[i];
        for (uint64_t j = 0; j < iterations; j++) {
            sha256_ni_transform(hash);
        }
    }
}

// Used for getting poh-verify-test working
__attribute__((visibility("default")))
void poh_verify_many(uint8_t* hashes, const uint64_t* num_hashes, size_t num_elems, uint8_t _unused) {
    for (size_t i = 0; i < num_elems; i++)  {
        uint8_t* hash = hashes + (i * SHA256_BLOCK_SIZE);
        uint64_t iterations = num_hashes[i];
        for (uint64_t j = 0; j < iterations; j++) {
            sha256_ni_transform(hash);
        }
    }
}

__attribute__((visibility("default")))
void poh_verify_many_set_verbose(bool verbose) {
    // No-op
    (void) verbose;
}
