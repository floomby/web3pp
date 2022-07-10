#pragma once

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

/** libkeccak-tiny
 *
 * A single-file implementation of SHA-3 and SHAKE.
 *
 * Implementor: David Leon Gil
 * License: CC0, attribution kindly requested. Blame taken too,
 * but not liability.
 */

/******** The Keccak-f[1600] permutation ********/

/*** Constants. ***/
static uint8_t const rho[24] =
    {1, 3, 6, 10, 15, 21,
     28, 36, 45, 55, 2, 14,
     27, 41, 56, 8, 25, 43,
     62, 18, 39, 61, 20, 44};
static uint8_t const pi[24] =
    {10, 7, 11, 17, 18, 3,
     5, 16, 8, 21, 24, 4,
     15, 23, 19, 13, 12, 2,
     20, 14, 22, 9, 6, 1};
static uint64_t const RC[24] =
    {1ULL, 0x8082ULL, 0x800000000000808aULL, 0x8000000080008000ULL,
     0x808bULL, 0x80000001ULL, 0x8000000080008081ULL, 0x8000000000008009ULL,
     0x8aULL, 0x88ULL, 0x80008009ULL, 0x8000000aULL,
     0x8000808bULL, 0x800000000000008bULL, 0x8000000000008089ULL, 0x8000000000008003ULL,
     0x8000000000008002ULL, 0x8000000000000080ULL, 0x800aULL, 0x800000008000000aULL,
     0x8000000080008081ULL, 0x8000000000008080ULL, 0x80000001ULL, 0x8000000080008008ULL};

/*** Helper macros to unroll the permutation. ***/
#define rol(x, s) (((x) << s) | ((x) >> (64 - s)))
#define REPEAT6(e) e e e e e e
#define REPEAT24(e) REPEAT6(e e e e)
#define REPEAT5(e) e e e e e
#define FOR5(type, v, s, e) \
    v = 0;                  \
    REPEAT5(e; v = static_cast<type>(v + s);)

/*** Keccak-f[1600] ***/
static inline void keccakf(void* state) {
    auto* a = static_cast<uint64_t*>(state);
    uint64_t b[5] = {0};

    for (int i = 0; i < 24; i++) {
        uint8_t x, y;
        // Theta
        FOR5(uint8_t, x, 1,
             b[x] = 0;
             FOR5(uint8_t, y, 5,
                  b[x] ^= a[x + y];))
        FOR5(uint8_t, x, 1,
             FOR5(uint8_t, y, 5,
                  a[y + x] ^= b[(x + 4) % 5] ^ rol(b[(x + 1) % 5], 1);))
        // Rho and pi
        uint64_t t = a[1];
        x = 0;
        REPEAT24(b[0] = a[pi[x]];
                 a[pi[x]] = rol(t, rho[x]);
                 t = b[0];
                 x++;)
        // Chi
        FOR5(uint8_t,
             y,
             5,
             FOR5(uint8_t, x, 1,
                  b[x] = a[y + x];)
                 FOR5(uint8_t, x, 1,
                      a[y + x] = b[x] ^ ((~b[(x + 1) % 5]) & b[(x + 2) % 5]);))
        // Iota
        a[0] ^= RC[i];
    }
}

/******** The FIPS202-defined functions. ********/

/*** Some helper macros. ***/

#define _(S) \
    do {     \
        S    \
    } while (0)
#define FOR(i, ST, L, S) \
    _(for (size_t i = 0; i < L; i += ST) { S; })
#define mkapply_ds(NAME, S)                     \
    static inline void NAME(uint8_t* dst,       \
                            uint8_t const* src, \
                            size_t len) {       \
        FOR(i, 1, len, S);                      \
    }
#define mkapply_sd(NAME, S)                     \
    static inline void NAME(uint8_t const* src, \
                            uint8_t* dst,       \
                            size_t len) {       \
        FOR(i, 1, len, S);                      \
    }

mkapply_ds(xorin, dst[i] ^= src[i])      // xorin
    mkapply_sd(setout, dst[i] = src[i])  // setout
#define P keccakf
#define Plen 200

// Fold P*F over the full blocks of an input.
#define foldP(I, L, F)  \
    while (L >= rate) { \
        F(a, I, rate);  \
        P(a);           \
        I += rate;      \
        L -= rate;      \
    }

    /** The sponge-based hash construction. **/
    inline void hash(
        uint8_t* out,
        size_t outlen,
        uint8_t const* in,
        size_t inlen,
        size_t rate,
        uint8_t delim) {
    uint8_t a[Plen] = {0};
    // Absorb input.
    foldP(in, inlen, xorin);
    // Xor in the DS and pad frame.
    a[inlen] ^= delim;
    a[rate - 1] ^= 0x80;
    // Xor in the last block.
    xorin(a, in, inlen);
    // Apply P
    P(a);
    // Squeeze output.
    foldP(out, outlen, setout);
    setout(a, out, outlen);
    memset(a, 0, 200);
}

template <class T> struct is_std_vector : std::false_type {};
template <class T> struct is_std_vector<std::vector<T>> : std::true_type {};

template <typename T, typename U = std::array<uint8_t, 32>>
inline U keccak256(const T &_input) {
    U output;
    
    if constexpr (is_std_vector<U>::value) {
        output.resize(32);
    }

    // Parameters used:
    // The 0x01 is the specific padding for keccak (sha3 uses 0x06) and
    // the way the round size (or window or whatever it was) is calculated.
    // 200 - (256 / 4) is the "rate"
    if constexpr (std::is_same_v<decltype(_input.data()), const char *>) {
        hash(output.data(), output.size(), reinterpret_cast<const uint8_t *>(_input.data()), _input.size(), 200 - (256 / 4), 0x01);
    } else {
        hash(output.data(), output.size(), _input.data(), _input.size(), 200 - (256 / 4), 0x01);
    }
    return output;
}

template <typename T> inline std::vector<unsigned char> keccak256_v(const T &_input) {
    return keccak256<T, std::vector<unsigned char>>(_input);
}