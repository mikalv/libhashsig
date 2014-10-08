/*
Implementation by the Keccak, Keyak and Ketje Teams, namely, Guido Bertoni,
Joan Daemen, Michaël Peeters, Gilles Van Assche and Ronny Van Keer, hereby
denoted as "the implementer".

For more information, feedback or questions, please refer to our websites:
http://keccak.noekeon.org/
http://keyak.noekeon.org/
http://ketje.noekeon.org/

To the extent possible under law, the implementer has waived all copyright
and related or neighboring rights to the source code in this file.
http://creativecommons.org/publicdomain/zero/1.0/
*/

#include <string.h>
#include <stdlib.h>
#include "brg_endian.h"
#include "KeccakF-1600-opt64-settings.h"
#include "KeccakF-1600-interface.h"

typedef unsigned char UINT8;
typedef unsigned long long int UINT64;

#if defined(__GNUC__)
#define ALIGN __attribute__ ((aligned(32)))
#elif defined(_MSC_VER)
#define ALIGN __declspec(align(32))
#else
#define ALIGN
#endif

#if defined(UseLaneComplementing)
#define UseBebigokimisa
#endif

#if defined(_MSC_VER)
#define ROL64(a, offset) _rotl64(a, offset)
#elif defined(UseSHLD)
    #define ROL64(x,N) ({ \
    register UINT64 __out; \
    register UINT64 __in = x; \
    __asm__ ("shld %2,%0,%0" : "=r"(__out) : "0"(__in), "i"(N)); \
    __out; \
    })
#else
#define ROL64(a, offset) ((((UINT64)a) << offset) ^ (((UINT64)a) >> (64-offset)))
#endif

#include "KeccakF-1600-64.macros"
#include "KeccakF-1600-unrolling.macros"

void hashsig_KeccakF1600_StateXORPermuteExtract(void *state, const unsigned char *inData, unsigned int inLaneCount, unsigned char *outData, unsigned int outLaneCount);

/* ---------------------------------------------------------------- */

void hashsig_KeccakF1600_Initialize( void )
{
}

/* ---------------------------------------------------------------- */

void hashsig_KeccakF1600_StateInitialize(void *state)
{
    memset(state, 0, 200);
#ifdef UseLaneComplementing
    ((UINT64*)state)[ 1] = ~(UINT64)0;
    ((UINT64*)state)[ 2] = ~(UINT64)0;
    ((UINT64*)state)[ 8] = ~(UINT64)0;
    ((UINT64*)state)[12] = ~(UINT64)0;
    ((UINT64*)state)[17] = ~(UINT64)0;
    ((UINT64*)state)[20] = ~(UINT64)0;
#endif
}

/* ---------------------------------------------------------------- */

void hashsig_KeccakF1600_StateXORBytesInLane(void *state, unsigned int lanePosition, const unsigned char *data, unsigned int offset, unsigned int length)
{
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
    if (length == 0)
        return;
    UINT64 lane;
    if (length == 1)
        lane = data[0];
    else {
        lane = 0;
        memcpy(&lane, data, length);
    }
    lane <<= offset*8;
#else
    UINT64 lane = 0;
    unsigned int i;
    for(i=0; i<length; i++)
        lane |= ((UINT64)data[i]) << ((i+offset)*8);
#endif
    ((UINT64*)state)[lanePosition] ^= lane;
}

/* ---------------------------------------------------------------- */

void hashsig_KeccakF1600_StateXORLanes(void *state, const unsigned char *data, unsigned int laneCount)
{
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
    unsigned int i = 0;
#ifdef NO_MISALIGNED_ACCESSES
    // If either pointer is misaligned, fall back to byte-wise xor.
    if (((((uintptr_t)state) & 7) != 0) || ((((uintptr_t)data) & 7) != 0)) {
      for (i = 0; i < laneCount * 8; i++) {
        ((unsigned char*)state)[i] ^= data[i];
      }
    }
    else
#endif
    {
      // Otherwise...
      for( ; (i+8)<=laneCount; i+=8) {
          ((UINT64*)state)[i+0] ^= ((UINT64*)data)[i+0];
          ((UINT64*)state)[i+1] ^= ((UINT64*)data)[i+1];
          ((UINT64*)state)[i+2] ^= ((UINT64*)data)[i+2];
          ((UINT64*)state)[i+3] ^= ((UINT64*)data)[i+3];
          ((UINT64*)state)[i+4] ^= ((UINT64*)data)[i+4];
          ((UINT64*)state)[i+5] ^= ((UINT64*)data)[i+5];
          ((UINT64*)state)[i+6] ^= ((UINT64*)data)[i+6];
          ((UINT64*)state)[i+7] ^= ((UINT64*)data)[i+7];
      }
      for( ; (i+4)<=laneCount; i+=4) {
          ((UINT64*)state)[i+0] ^= ((UINT64*)data)[i+0];
          ((UINT64*)state)[i+1] ^= ((UINT64*)data)[i+1];
          ((UINT64*)state)[i+2] ^= ((UINT64*)data)[i+2];
          ((UINT64*)state)[i+3] ^= ((UINT64*)data)[i+3];
      }
      for( ; (i+2)<=laneCount; i+=2) {
          ((UINT64*)state)[i+0] ^= ((UINT64*)data)[i+0];
          ((UINT64*)state)[i+1] ^= ((UINT64*)data)[i+1];
      }
      if (i<laneCount) {
          ((UINT64*)state)[i+0] ^= ((UINT64*)data)[i+0];
      }
    }
#else
    unsigned int i;
    UINT8 *curData = data;
    for(i=0; i<laneCount; i++, curData+=8) {
        UINT64 lane = (UINT64)curData[0]
            | ((UINT64)curData[1] << 8)
            | ((UINT64)curData[2] << 16)
            | ((UINT64)curData[3] << 24)
            | ((UINT64)curData[4] <<32)
            | ((UINT64)curData[5] << 40)
            | ((UINT64)curData[6] << 48)
            | ((UINT64)curData[7] << 56);
        ((UINT64*)state)[i] ^= lane;
    }
#endif
}

/* ---------------------------------------------------------------- */

void hashsig_KeccakF1600_StateOverwriteBytesInLane(void *state, unsigned int lanePosition, const unsigned char *data, unsigned int offset, unsigned int length)
{
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
#ifdef UseLaneComplementing
    if ((lanePosition == 1) || (lanePosition == 2) || (lanePosition == 8) || (lanePosition == 12) || (lanePosition == 17) || (lanePosition == 20)) {
        unsigned int i;
        for(i=0; i<length; i++)
            ((unsigned char*)state)[lanePosition*8+offset+i] = ~data[i];
    }
    else
#endif
    {
        memcpy((unsigned char*)state+lanePosition*8+offset, data, length);
    }
#else
#error "Not yet implemented"
#endif
}

/* ---------------------------------------------------------------- */

void hashsig_KeccakF1600_StateOverwriteLanes(void *state, const unsigned char *data, unsigned int laneCount)
{
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
#ifdef UseLaneComplementing
    unsigned int lanePosition;

    for(lanePosition=0; lanePosition<laneCount; lanePosition++)
        if ((lanePosition == 1) || (lanePosition == 2) || (lanePosition == 8) || (lanePosition == 12) || (lanePosition == 17) || (lanePosition == 20))
            ((UINT64*)state)[lanePosition] = ~((const UINT64*)data)[lanePosition];
        else
            ((UINT64*)state)[lanePosition] = ((const UINT64*)data)[lanePosition];
#else
    memcpy(state, data, laneCount*8);
#endif
#else
#error "Not yet implemented"
#endif
}

/* ---------------------------------------------------------------- */

void hashsig_KeccakF1600_StateOverwriteWithZeroes(void *state, unsigned int byteCount)
{
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
#ifdef UseLaneComplementing
    unsigned int lanePosition;

    for(lanePosition=0; lanePosition<byteCount/8; lanePosition++)
        if ((lanePosition == 1) || (lanePosition == 2) || (lanePosition == 8) || (lanePosition == 12) || (lanePosition == 17) || (lanePosition == 20))
            ((UINT64*)state)[lanePosition] = ~0;
        else
            ((UINT64*)state)[lanePosition] = 0;
    if (byteCount%8 != 0) {
        lanePosition = byteCount/8;
        if ((lanePosition == 1) || (lanePosition == 2) || (lanePosition == 8) || (lanePosition == 12) || (lanePosition == 17) || (lanePosition == 20))
            memset((char *)state+lanePosition*8, 0xFF, byteCount%8);
        else
            memset((char *)state+lanePosition*8, 0, byteCount%8);
    }
#else
    memset(state, 0, byteCount);
#endif
#else
#error "Not yet implemented"
#endif
}

/* ---------------------------------------------------------------- */

void hashsig_KeccakF1600_StateComplementBit(void *state, unsigned int position)
{
    UINT64 lane = (UINT64)1 << (position%64);
    ((UINT64*)state)[position/64] ^= lane;
}

/* ---------------------------------------------------------------- */

void hashsig_KeccakF1600_StatePermute(void *state)
{
    hashsig_KeccakF1600_StateXORPermuteExtract(state, 0, 0, 0, 0);
}

/* ---------------------------------------------------------------- */

void hashsig_KeccakF1600_StateExtractBytesInLane(const void *state, unsigned int lanePosition, unsigned char *data, unsigned int offset, unsigned int length)
{
    UINT64 lane = ((UINT64*)state)[lanePosition];
#ifdef UseLaneComplementing
    if ((lanePosition == 1) || (lanePosition == 2) || (lanePosition == 8) || (lanePosition == 12) || (lanePosition == 17) || (lanePosition == 20))
        lane = ~lane;
#endif
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
    {
        UINT64 lane1[1];
        lane1[0] = lane;
        memcpy(data, (UINT8*)lane1+offset, length);
    }
#else
    unsigned int i;
    lane >>= offset*8;
    for(i=0; i<length; i++) {
        data[i] = lane & 0xFF;
        lane >>= 8;
    }
#endif
}

/* ---------------------------------------------------------------- */

#if (PLATFORM_BYTE_ORDER != IS_LITTLE_ENDIAN)
void hashsig_fromWordToBytes(UINT8 *bytes, const UINT64 word)
{
    unsigned int i;

    for(i=0; i<(64/8); i++)
        bytes[i] = (word >> (8*i)) & 0xFF;
}
#endif

void hashsig_KeccakF1600_StateExtractLanes(const void *state, unsigned char *data, unsigned int laneCount)
{
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
    memcpy(data, state, laneCount*8);
#else
    unsigned int i;

    for(i=0; i<laneCount; i++)
        hashsig_fromWordToBytes(data+(i*8), ((const UINT64*)state)[i]);
#endif
#ifdef UseLaneComplementing
    if (laneCount > 1) {
        ((UINT64*)data)[ 1] = ~((UINT64*)data)[ 1];
        if (laneCount > 2) {
            ((UINT64*)data)[ 2] = ~((UINT64*)data)[ 2];
            if (laneCount > 8) {
                ((UINT64*)data)[ 8] = ~((UINT64*)data)[ 8];
                if (laneCount > 12) {
                    ((UINT64*)data)[12] = ~((UINT64*)data)[12];
                    if (laneCount > 17) {
                        ((UINT64*)data)[17] = ~((UINT64*)data)[17];
                        if (laneCount > 20) {
                            ((UINT64*)data)[20] = ~((UINT64*)data)[20];
                        }
                    }
                }
            }
        }
    }
#endif
}

/* ---------------------------------------------------------------- */

void hashsig_KeccakF1600_StateExtractAndXORBytesInLane(const void *state, unsigned int lanePosition, unsigned char *data, unsigned int offset, unsigned int length)
{
    UINT64 lane = ((UINT64*)state)[lanePosition];
#ifdef UseLaneComplementing
    if ((lanePosition == 1) || (lanePosition == 2) || (lanePosition == 8) || (lanePosition == 12) || (lanePosition == 17) || (lanePosition == 20))
        lane = ~lane;
#endif
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
    {
        unsigned int i;
        UINT64 lane1[1];
        lane1[0] = lane;
        for(i=0; i<length; i++)
            data[i] ^= ((UINT8*)lane1)[offset+i];
    }
#else
    unsigned int i;
    lane >>= offset*8;
    for(i=0; i<length; i++) {
        data[i] ^= lane & 0xFF;
        lane >>= 8;
    }
#endif
}

/* ---------------------------------------------------------------- */

void hashsig_KeccakF1600_StateExtractAndXORLanes(const void *state, unsigned char *data, unsigned int laneCount)
{
    unsigned int i;
#if (PLATFORM_BYTE_ORDER != IS_LITTLE_ENDIAN)
    unsigned char temp[8];
    unsigned int j;
#endif

    for(i=0; i<laneCount; i++) {
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
        ((UINT64*)data)[i] ^= ((const UINT64*)state)[i];
#else
        hashsig_fromWordToBytes(temp, ((const UINT64*)state)[i]);
        for(j=0; j<8; j++)
            data[i*8+j] ^= temp[j];
#endif
    }
#ifdef UseLaneComplementing
    if (laneCount > 1) {
        ((UINT64*)data)[ 1] = ~((UINT64*)data)[ 1];
        if (laneCount > 2) {
            ((UINT64*)data)[ 2] = ~((UINT64*)data)[ 2];
            if (laneCount > 8) {
                ((UINT64*)data)[ 8] = ~((UINT64*)data)[ 8];
                if (laneCount > 12) {
                    ((UINT64*)data)[12] = ~((UINT64*)data)[12];
                    if (laneCount > 17) {
                        ((UINT64*)data)[17] = ~((UINT64*)data)[17];
                        if (laneCount > 20) {
                            ((UINT64*)data)[20] = ~((UINT64*)data)[20];
                        }
                    }
                }
            }
        }
    }
#endif
}

/* ---------------------------------------------------------------- */

#ifdef ProvideFastAbsorb1344
void hashsig_KeccakF1600_StateXORPermuteExtract_absorb1344(void *state, const unsigned char *inData, unsigned int inLaneCount)
{
    declareABCDE
    #if (Unrolling != 24)
    unsigned int i;
    #endif
    UINT64 *stateAsLanes = (UINT64*)state;
    UINT64 *inDataAsLanes = (UINT64*)inData;
    
    copyFromStateAndXOR(A, stateAsLanes, inDataAsLanes, 21)
    rounds
    copyToState(stateAsLanes, A)
}
#endif

#ifdef ProvideFastSqueeze1344
void hashsig_KeccakF1600_StateXORPermuteExtract_squeeze1344(void *state, unsigned char *outData, unsigned int outLaneCount)
{
    declareABCDE
    #if (Unrolling != 24)
    unsigned int i;
    #endif
    UINT64 *stateAsLanes = (UINT64*)state;
    UINT64 *outDataAsLanes = (UINT64*)outData;
    
    copyFromStateAndXOR(A, stateAsLanes, outDataAsLanes, 0)
    rounds
    copyToStateAndOutput(A, stateAsLanes, outDataAsLanes, 21)
}
#endif

void hashsig_KeccakF1600_StateXORPermuteExtract(void *state, const unsigned char *inData, unsigned int inLaneCount, unsigned char *outData, unsigned int outLaneCount)
{
#ifdef ProvideFastAbsorb1344
    if ((inLaneCount == 21) && (outLaneCount == 0))
        KeccakF1600_StateXORPermuteExtract_absorb1344(state, inData, inLaneCount);
    else
#endif
#ifdef ProvideFastSqueeze1344
    if ((inLaneCount == 0) && (outLaneCount == 21))
        KeccakF1600_StateXORPermuteExtract_squeeze1344(state, outData, outLaneCount);
    else
#endif
    {
        declareABCDE
        #if (Unrolling != 24)
        unsigned int i;
        #endif
        UINT64 *stateAsLanes = (UINT64*)state;
        UINT64 *inDataAsLanes = (UINT64*)inData;
        UINT64 *outDataAsLanes = (UINT64*)outData;
        
        copyFromStateAndXOR(A, stateAsLanes, inDataAsLanes, inLaneCount)
        rounds
        copyToStateAndOutput(A, stateAsLanes, outDataAsLanes, outLaneCount)
    }
}

/* ---------------------------------------------------------------- */

size_t hashsig_KeccakF1600_FBWL_Absorb(void *state, unsigned int laneCount, const unsigned char *data, const size_t dataByteLen, unsigned char trailingBits)
{
    size_t originalDataByteLen = dataByteLen;
    declareABCDE
    #if (Unrolling != 24)
    unsigned int i;
    #endif
    UINT64 *stateAsLanes = (UINT64*)state;
    UINT64 *inDataAsLanes = (UINT64*)data;
    size_t len = dataByteLen;

    copyFromState(A, stateAsLanes)
    while(len >= laneCount*8) {
        XORinputAndTrailingBits(A, inDataAsLanes, laneCount, ((UINT64)trailingBits))
        rounds
        inDataAsLanes += laneCount;
        len -= laneCount*8;
    }
    copyToState(stateAsLanes, A)
    return originalDataByteLen - len;
}

/* ---------------------------------------------------------------- */

size_t hashsig_KeccakF1600_FBWL_Squeeze(void *state, unsigned int laneCount, unsigned char *data, size_t dataByteLen)
{
    size_t originalDataByteLen = dataByteLen;
    declareABCDE
    #if (Unrolling != 24)
    unsigned int i;
    #endif
    UINT64 *stateAsLanes = (UINT64*)state;
    UINT64 *outDataAsLanes = (UINT64*)data;

    copyFromState(A, stateAsLanes)
    while(dataByteLen >= laneCount*8) {
        rounds
        output(A, outDataAsLanes, laneCount)
        outDataAsLanes += laneCount;
        dataByteLen -= laneCount*8;
    }
    copyToState(stateAsLanes, A)
    return originalDataByteLen - dataByteLen;
}

/* ---------------------------------------------------------------- */

size_t hashsig_KeccakF1600_FBWL_Wrap(void *state, unsigned int laneCount, const unsigned char *dataIn, unsigned char *dataOut, size_t dataByteLen, unsigned char trailingBits)
{
    size_t originalDataByteLen = dataByteLen;
    declareABCDE
    #if (Unrolling != 24)
    unsigned int i;
    #endif
    UINT64 *stateAsLanes = (UINT64*)state;
    UINT64 *inDataAsLanes = (UINT64*)dataIn;
    UINT64 *outDataAsLanes = (UINT64*)dataOut;

    copyFromState(A, stateAsLanes)
    while(dataByteLen >= laneCount*8) {
        wrap(A, inDataAsLanes, outDataAsLanes, laneCount, ((UINT64)trailingBits))
        rounds
        inDataAsLanes += laneCount;
        outDataAsLanes += laneCount;
        dataByteLen -= laneCount*8;
    }
    copyToState(stateAsLanes, A)
    return originalDataByteLen - dataByteLen;
}

/* ---------------------------------------------------------------- */

size_t hashsig_KeccakF1600_FBWL_Unwrap(void *state, unsigned int laneCount, const unsigned char *dataIn, unsigned char *dataOut, size_t dataByteLen, unsigned char trailingBits)
{
    size_t originalDataByteLen = dataByteLen;
    declareABCDE
    #if (Unrolling != 24)
    unsigned int i;
    #endif
    UINT64 *stateAsLanes = (UINT64*)state;
    UINT64 *inDataAsLanes = (UINT64*)dataIn;
    UINT64 *outDataAsLanes = (UINT64*)dataOut;

    copyFromState(A, stateAsLanes)
    while(dataByteLen >= laneCount*8) {
        unwrap(A, inDataAsLanes, outDataAsLanes, laneCount, ((UINT64)trailingBits))
        rounds
        inDataAsLanes += laneCount;
        outDataAsLanes += laneCount;
        dataByteLen -= laneCount*8;
    }
    copyToState(stateAsLanes, A)
    return originalDataByteLen - dataByteLen;
}
