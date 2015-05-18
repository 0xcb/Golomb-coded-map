/*
 *  The MIT License (MIT)
 *
 *  Copyright (c) 2015 Colt Blackmore
 *  
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.

 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
 */


#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "gc_map.h"
#include "xxHash/xxhash.c"

#if(_MSC_VER)
    #include <intrin.h>
#elif(__GNUC__)
    #define __forceinline __attribute__((always_inline))
#else
    #error Unrecognized compiler: Only GCC and Visual Studio are supported.
#endif


#pragma GCC diagnostic ignored "-Wattributes"


/*
 *  Returns the position of the most significant set bit. Handles the
 *  case of a 0 value, which is undefined by the compiler intrinsics.
 *  (If 512-bit+ values were needed, the uint8_t would be too small
 *  a return type; but it's sufficient for 256-bit values, and currently
 *  64 bits is the limit.)
 */
static __forceinline uint8_t __bsr(uint64_t vector)
{
    unsigned long index;

    if(!vector) return 0;

    #if(_MSC_VER)
        _BitScanReverse64(&index, vector);
    #elif(__GNUC__)
        index = 63 - __builtin_clzll(vector);
    #endif

    return (uint8_t)(index + 1);
}


/*
 *  Sorting function for qsort.
 */
static int compare_ints(const void* a, const void* b)
{
    if(*((uint64_t*)a) < *((uint64_t*)b)) return -1;
    if(*((uint64_t*)a) > *((uint64_t*)b)) return 1;
    return 0;
}


/*
 *  #defines are silly.
 */
static __forceinline uint8_t min(uint8_t a, uint8_t b)
{
    return a < b ? a : b;
}


/*
 *  Generates a 64-bit hash for the input element.
 */
static __forceinline uint64_t gc_map_get_hash(uint8_t* element, uint8_t element_length)
{
    return XXH64(element, element_length, 1337);
}


/*
 *  Writes num_bits from vector to a gc_map->vector starting at offset.
 *  These bits will be the Golomb-coded quotient or remainder of an
 *  element hash_dif.
 */
static __forceinline uint8_t gc_map_write_vector(gc_map* gcm, uint32_t offset, uint8_t num_bits, uint64_t vector)
{
    vector <<= (64 - num_bits);
    uint8_t remaining = num_bits;
    while(remaining)
    {
        uint32_t qword = offset / 64;
        uint32_t bit   = offset % 64;

        gcm->vector[qword] |= vector >> bit;
        uint8_t written = min(remaining, 64 - bit);
        vector <<= written;
        remaining -= written;
        offset += written;
    }
    return num_bits;
}


/*
 *  Reads a Golomb-coded value from gc_map->vector starting at offset.
 *  The value is placed in the corresponding argument, and the number of
 *  bits read is returned.
 */
static __forceinline uint8_t gc_map_read_vector(gc_map* gcm, uint32_t offset, uint64_t* value)
{
    // Get the quotient.
    uint8_t quotient = 0;
    uint8_t msb = 0;
    while(!msb)
    {
        uint32_t qword = offset / 64;
        uint32_t bit = offset % 64;

        msb = __bsr(~(gcm->vector[qword] << bit));
        uint8_t bits_read = 64 - msb;
        msb -= bit;

        quotient += bits_read;
        offset += bits_read;
    }
    // Trailing 0.
    ++offset;

    // Get the remainder.
    uint64_t remainder = 0;
    int8_t   remaining = (int8_t)gcm->remainder_size;
    for(;;)
    {
        uint32_t qword = offset / 64;
        uint32_t bit = offset % 64;

        remainder |= (gcm->vector[qword] << bit) >> (64 - remaining);
        uint8_t bits_read = 64 - bit;
        remaining -= bits_read;
        if(remaining < 1) break;
        offset += bits_read;
    }

    *value = quotient * gcm->error_rate + remainder;

    return quotient + 1 + gcm->remainder_size;
}


/*
 *  Searches a gc_map for an element and places the index of that
 *  element in the corresponding argument, if found. Returns true
 *  if the element is found, otherwise false. If the element is not
 *  found, the value of index is undefined.
 */
uint8_t gc_map_query(gc_map* gcm, uint8_t* element, uint8_t element_length, uint32_t* index)
{
    uint64_t hash = gc_map_get_hash(element, element_length) % (gcm->element_count * gcm->error_rate);
    uint32_t bin = (uint32_t)(hash / gcm->element_divisor);

    if(gcm->element_table[bin] < 0) return 0;
    uint32_t offset = gcm->element_table[bin];

    uint64_t hash_dif = bin * gcm->element_divisor;
    uint64_t value;

    *index = gcm->element_table_count[bin];
    while(1)
    {
        offset += gc_map_read_vector(gcm, offset, &value);
        hash_dif += value;
        if(hash == hash_dif) return 1;
        if(hash < hash_dif || *index >= gcm->element_count) break;
        ++*index;
    }

    return 0;
}


/*
 *  Deallocates a gc_map.
 */
void gc_map_free(gc_map* gcm)
{
    if(gcm)
    {
        if(gcm->vector)              free(gcm->vector);
        if(gcm->element_table)       free(gcm->element_table);
        if(gcm->element_table_count) free(gcm->element_table_count);
        free(gcm);
    }
}


/*
 *  Creates a gc_map for element_list, given error_rate expressed
 *  in the form of '1 in n' (1/p).
 */
gc_map* gc_map_new(uint8_t* element_list, uint8_t element_length, uint32_t element_count, uint64_t error_rate, uint32_t table_size)
{
    if(!element_list || !element_length || !element_count || error_rate < 2 || table_size == 0 || table_size > element_count - 1) goto bad_arguments;

    // Prevent overflow of error_rate. Compromises could be made here but...
    if(__bsr(error_rate) > 63) goto bad_arguments;

    /*
     *  Get the lowest power of 2 that fits error_rate and increase
     *  error_rate to maximize the value of that power of 2.
     *
     *  You might think error_rate can be extended to the max value of
     *  the required number of bits used as the divisor in the modulo
     *  operation at the Golomb coding step, i.e.:
     *
     *      ~0ULL >> (64 - __bsr(element_count * error_rate));
     *
     *  However we're first optimizing error_rate for the number of bits
     *  required for the Golomb-coded remainder, then the max error_rate
     *  for that number of bits.
     */
    error_rate = 1ULL << __bsr(error_rate - 1);

    /*
     *  hash_length can be used to make the system more dynamic in the future. For
     *  now it simply prevents us from exceeding 64 bits per hash.
     */
    {
        // Prevent overflow on element_count * error_rate.
        if(element_count > /*(2^64)-1*/ ~0ULL / error_rate) goto bad_arguments;

        // Hash size must be byte-aligned.
        uint8_t hash_length = __bsr(element_count * error_rate);

        hash_length = hash_length / 8 + (hash_length % 8 ? 1 : 0);
        if(hash_length > 8) goto bad_arguments;
    }

    uint64_t* hash_list = (uint64_t*)malloc(element_count * sizeof(uint64_t));
    if(!hash_list) goto out_of_memory;

    struct gc_map* gcm = (gc_map*)calloc(1, sizeof(gc_map));
    if(!gcm) goto out_of_memory;

    // error_rate will always be > 1, so bsr can't fail here.
    gcm->remainder_size = __bsr(error_rate - 1);

    for(uint32_t i = 0; i < element_count; ++i)
    {
        hash_list[i] = gc_map_get_hash(element_list + i * element_length, element_length) % (element_count * error_rate);
    }

    qsort(hash_list, element_count, sizeof(uint64_t), compare_ints);

    gcm->element_divisor = error_rate * element_count / table_size;
    gcm->element_divisor += gcm->element_divisor % table_size ? 1 : 0;

    // Count bytes needed for bit vector. 512MB max size (unsigned 32-bit int).
    {
        uint32_t vec_bits = 0;
        uint32_t curr_bin = ~0; // Any non-zero value is fine.
        for(uint32_t i = 0; i < element_count; ++i)
        {
            // To support element_table as a lookup table, the first value in
            // each bin is current - divisor rather than current - previous.
            uint32_t next_bin = (uint32_t)(hash_list[i] / gcm->element_divisor);
            uint64_t hash_dif = hash_list[i] - (next_bin != curr_bin ? next_bin * gcm->element_divisor : hash_list[i - 1]);
            if(!hash_dif && next_bin == curr_bin) continue;
            vec_bits += (uint32_t)(hash_dif / error_rate + 1 + gcm->remainder_size);
            curr_bin = next_bin;
        }

        // Round up to nearest qword.
        gcm->vector_size = 64 * (vec_bits / 64 + (vec_bits % 64 ? 1 : 0));
    }

    if(!(gcm->vector = (uint64_t*)calloc(1, gcm->vector_size / sizeof(uint64_t)))) goto out_of_memory;
    if(!(gcm->element_table = (int32_t*)malloc(table_size * sizeof(*gcm->element_table)))) goto out_of_memory;
    if(!(gcm->element_table_count = (uint32_t*)calloc(table_size, sizeof(*gcm->element_table_count)))) goto out_of_memory;

    // Empty bins have a negative index.
    memset(gcm->element_table, -1L, table_size * sizeof(*gcm->element_table));
    gcm->element_table_size = table_size;

    {
        uint32_t offset = 0;
        uint32_t curr_bin = ~0;
        for(uint32_t i = 0; i < element_count; ++i)
        {
            // Populate lookup and count tables.
            uint32_t next_bin = (uint32_t)(hash_list[i] / gcm->element_divisor);

            if(next_bin != curr_bin)       gcm->element_table[next_bin] = offset;
            if(next_bin != table_size - 1) gcm->element_table_count[next_bin + 1]++;

            // Golomb code hash and write to vector.
            uint64_t hash_dif = hash_list[i] - (next_bin != curr_bin ? next_bin * gcm->element_divisor : hash_list[i - 1]);
            if(!hash_dif && next_bin == curr_bin) continue;

            uint8_t  quotient  = (uint8_t)(hash_dif / error_rate);
            uint64_t remainder = hash_dif % error_rate;

            /*
             *  If there are many collisions, in theory, the quotient can exceed 64 bits because
             *  the divisor will remain large, while the actual number of elements will be much
             *  smaller, skewing the geometric distribution. Thus hte loop. The maximum value for
             *  looping is 63 to allow for the terminal 0 bit in unary coding on the final write.
             */
            while(quotient > 63)
            {
                offset += gc_map_write_vector(gcm, offset, 63, ~0);
                quotient -= 63;
            }
            offset += gc_map_write_vector(gcm, offset, quotient + 1, ~1);
            offset += gc_map_write_vector(gcm, offset, gcm->remainder_size, remainder);

            curr_bin = next_bin;
        }

        // Make count table cumulative.
        for(uint32_t i = 1; i < table_size; ++i)
        {
            gcm->element_table_count[i] += gcm->element_table_count[i - 1];
        }
    }

    gcm->error_rate    = error_rate;
    gcm->element_count = element_count;

    free(hash_list);

    return gcm;

bad_arguments:
    errno = GCM_ERR_BAD_ARGUMENTS;
    return NULL;

out_of_memory:
    errno = GCM_ERR_OUT_OF_MEMORY;
    gc_map_free(gcm);
    if(hash_list) free(hash_list);
    return NULL;
}
