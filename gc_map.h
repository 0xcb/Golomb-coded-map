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
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
 */


#pragma once


#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


typedef struct gc_map
{
    uint64_t* vector;              // Bit vector for storing sorted, Golomb-coded entries
    uint64_t  vector_size;         // Size of Golomb-coded vector in bits
    uint8_t   remainder_size;      // Size of Golomb-coded remainder in bits
    uint64_t  error_rate;          // Rate of false positives in the form of '1 in error_rate'

    int32_t   element_table_size;  // Number of indices in element_table
    int32_t*  element_table;       // Indices into vector for faster lookups
    uint32_t* element_table_count; // Cumulative count of elements up to the given index

    uint32_t  element_count;       // Number of elements in map
    uint64_t  element_divisor;     // Index divisor for element_table and element_table_count
} gc_map;


/*
 *  errno error codes for gc_map_new(). These could be made more granular.
 */
enum gc_map_error
{
    GCM_ERR_BAD_ARGUMENTS = 1,
    GCM_ERR_OUT_OF_MEMORY,
};


gc_map* gc_map_new(uint8_t* element_list, uint8_t element_length, uint32_t element_count, uint64_t error_rate, uint32_t table_size);

uint8_t gc_map_query(gc_map* gcm, uint8_t* element, uint8_t element_length, uint32_t* index);

void gc_map_free(gc_map* gcm);