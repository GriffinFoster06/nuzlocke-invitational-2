#include "global.h"
#include "random.h"
#if MODERN
#include <alloca.h>
#endif

// IWRAM common
COMMON_DATA rng_value_t gRngValue = {0};
COMMON_DATA rng_value_t gRng2Value = {0};


EWRAM_DATA static volatile bool8 sRngLoopUnlocked;

// Streams allow generators seeded the same to have separate outputs.
#define STREAM1 1
#define STREAM2 29

// A variant of SFC32 that lets you change the stream.
// stream can be any odd number.
static inline u32 _SFC32_Next_Stream(struct Sfc32State *state, const u8 stream)
{
    const u32 result = state->a + state->b + state->ctr;
    state->ctr += stream;
    state->a = state->b ^ (state->b >> 9);
    state->b = state->c * 9;
    state->c = result + ((state->c << 21) | (state->c >> 11));
    return result;
}

static void SFC32_Seed(struct Sfc32State *state, u32 seed, u8 stream)
{
    u32 i;
    state->a = state->b = 0;
    state->c = seed;
    state->ctr = stream;
    for (i = 0; i < 16; i++)
    {
        _SFC32_Next_Stream(state, stream);
    }
}

/*This ASM implementation uses some shortcuts and is generally faster on the GBA.
* It's not necessarily faster if inlined, or on other platforms.
* In addition, it's extremely non-portable. */
u32 NAKED Random32(void)
{
    asm(".thumb\n\
    push {r4, r5, r6}\n\
    mov r6, #11\n\
    ldr r5, =gRngValue\n\
    ldmia r5!, {r1, r2, r3, r4}\n\
    @ result = a + b + (d+=STREAM1)\n\
    add r1, r1, r2\n\
    add r0, r1, r4\n\
    add r4, r4, #" STR(STREAM1) "\n\
    @ a = b ^ (b >> 9)\n\
    lsr r1, r2, #9\n\
    eor r1, r1, r2\n\
    @ b = c + (c << 3) [c * 9]\n\
    lsl r2, r3, #3\n\
    add r2, r2, r3\n\
    @ c = rol(c, 21) + result\n\
    ror r3, r3, r6\n\
    add r3, r3, r0\n\
    sub r5, r5, #16\n\
    stmia r5!, {r1, r2, r3, r4}\n\
    pop {r4, r5, r6}\n\
    bx lr\n\
    .ltorg"
    );
}

u32 Random2_32(void)
{
    return _SFC32_Next_Stream(&gRng2Value, STREAM2);
}

void SeedRng(u32 seed)
{
    struct Sfc32State state;
    SFC32_Seed(&state, seed, STREAM1);

    sRngLoopUnlocked = FALSE;
    gRngValue = state;
    sRngLoopUnlocked = TRUE;
}

void SeedRng2(u32 seed)
{
    SFC32_Seed(&gRng2Value, seed, STREAM2);
}

rng_value_t LocalRandomSeed(u32 seed)
{
    rng_value_t result;
    SFC32_Seed(&result, seed, STREAM1);
    return result;
}

void AdvanceRandom(void)
{
    if (sRngLoopUnlocked == TRUE)
        Random32();
}

#define LOOP_RANDOM_START \
    struct Sfc32State *const state = &gRngValue; \
    sRngLoopUnlocked = FALSE;

#define LOOP_RANDOM_END sRngLoopUnlocked = TRUE;

#define LOOP_RANDOM ((u16)(_SFC32_Next(state) >> 16))

#define SHUFFLE_IMPL \
    u32 tmp; \
    LOOP_RANDOM_START; \
    --n; \
    while (n > 1) \
    { \
        int j = (LOOP_RANDOM * (n+1)) >> 16; \
        SWAP(data[n], data[j], tmp); \
        --n; \
    } \
    LOOP_RANDOM_END

void Shuffle8(void *data_, size_t n)
{
    u8 *data = data_;
    SHUFFLE_IMPL;
}

void Shuffle16(void *data_, size_t n)
{
    u16 *data = data_;
    SHUFFLE_IMPL;
}

void Shuffle32(void *data_, size_t n)
{
    u32 *data = data_;
    SHUFFLE_IMPL;
}

void ShuffleN(void *data, size_t n, size_t size)
{
    void *tmp = alloca(size);
    LOOP_RANDOM_START;
    --n;

    while (n > 1)
    {
        int j = (LOOP_RANDOM * (n+1)) >> 16;
        memcpy(tmp, (u8 *)data + n*size, size); // tmp = data[n];
        memcpy((u8 *)data + n*size, (u8 *)data + j*size, size); // data[n] = data[j];
        memcpy((u8 *)data + j*size, tmp, size); // data[j] = tmp;
        --n;
    }

    LOOP_RANDOM_END;
}

__attribute__((weak, alias("RandomUniformDefault")))
u32 RandomUniform(enum RandomTag tag, u32 lo, u32 hi);

__attribute__((weak, alias("RandomUniformExceptDefault")))
u32 RandomUniformExcept(enum RandomTag, u32 lo, u32 hi, bool32 (*reject)(u32));

__attribute__((weak, alias("RandomWeightedArrayDefault")))
u32 RandomWeightedArray(enum RandomTag tag, u32 sum, u32 n, const u16 *weights);

__attribute__((weak, alias("RandomElementArrayDefault")))
const void *RandomElementArray(enum RandomTag tag, const void *array, size_t size, size_t count);

u32 RandomUniformDefault(enum RandomTag tag, u32 lo, u32 hi)
{
    assertf(lo <= hi);
    return lo + (((hi - lo + 1) * Random()) >> 16);
}

u32 RandomUniformExceptDefault(enum RandomTag tag, u32 lo, u32 hi, bool32 (*reject)(u32))
{
    assertf(lo <= hi);
    LOOP_RANDOM_START;
    while (TRUE)
    {
        // TODO: assertf to abort after too many iterations.
        u32 n = lo + (((hi - lo + 1) * LOOP_RANDOM) >> 16);
        if (!reject(n))
            return n;
    }
    LOOP_RANDOM_END;
}

u32 RandomWeightedArrayDefault(enum RandomTag tag, u32 sum, u32 n, const u16 *weights)
{
    assertf(n > 0);
    assertf(sum <= MAX_u16);
    u32 i, targetSum;
    targetSum = (sum * Random()) >> 16;
    for (i = 0; i < n - 1; i++)
    {
        if (targetSum < weights[i])
            return i;
        targetSum -= weights[i];
    }
    return n - 1;
}

const void *RandomElementArrayDefault(enum RandomTag tag, const void *array, size_t size, size_t count)
{
    assertf(count > 0);
    return (const u8 *)array + size * RandomUniformDefault(tag, 0, count - 1);
}

// Returns a random index according to a list of weights
u8 RandomWeightedIndex(u8 *weights, u8 length)
{
    u32 i;
    u16 randomValue;
    u16 weightSum = 0;
    for (i = 0; i < length; i++)
        weightSum += weights[i];
    randomValue = weightSum > 0 ? Random() % weightSum : 0;
    weightSum = 0;
    for (i = 0; i < length; i++)
    {
        weightSum += weights[i];
        if (randomValue < weightSum)
            return i;
    }
    return 0;
}

// Returns the index instead; don't call with no set bits
u32 RandomBitIndex(enum RandomTag tag, u32 bits)
{
  u8 setIndexes[32];
  u32 n = 0;
  for (u32 i = 0; i < 32; i++)
  {
    if (bits & (1 << i))
      setIndexes[n++] = i;
  }

  if (n == 0)
    return 0; // This is a little awkward, there are no set bits!
  else
    return setIndexes[RandomUniform(tag, 0, n-1)];
}

// Standard reflected CRC-32 (poly 0xEDB88320) lookup table, generated from the
// original bit-serial loop below. Perf: RunRng_Seed() (include/run_rng.h) -
// the seed derivation behind every Randomizer_*/AbilityGen_*/LearnsetGen_*
// category - calls this once per stream, and the bit-serial form burned 8
// loop iterations per input byte. Table-driven form does one lookup per byte
// instead; output is bit-identical (host-verified exhaustively over every
// 1-byte input and 2M random 24-byte buffers, the RunRng_Seed pieces[] shape).
static const u32 sCrc32Table[256] =
{
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
    0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
    0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
    0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
    0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
    0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
    0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
    0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
    0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
    0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
    0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
    0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
    0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
    0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
    0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
    0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
    0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
    0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
    0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
    0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
    0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
    0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
    0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
    0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
    0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
    0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
    0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
    0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
    0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
    0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
    0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
    0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
    0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
    0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
    0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
    0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
    0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D,
};

u32 Crc32B (const u8 *data, u32 size)
{
   u32 i, crc;

   crc = 0xFFFFFFFF;
   for (i = 0; i < size; ++i)
        crc = sCrc32Table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
   return ~crc;
}
