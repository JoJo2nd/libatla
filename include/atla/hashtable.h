/********************************************************************
    Written by James Moran
    Please see the file LICENSE.txt in the repository root directory.
*********************************************************************/
#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct ht_hash_elem ht_elem_t;

struct ht_hash_table {
  uint32_t (*hash_key)(void const* key);
  int (*compare_key)(void const* a, void const* b);
  void (*value_free)(void const* k, void* v);
  void* (*mralloc)(void* ptr, size_t size, void* user);
  void (*mfree)(void* ptr, void* user);

  ht_elem_t* __restrict buffer;
  uint32_t* __restrict hashes;
  void*    user;
  int      num_elems;
  int      capacity;
  int      resize_threshold;
  uint32_t mask;
};

typedef struct ht_hash_table ht_hash_table_t;

void ht_table_init(ht_hash_table_t* table,
                   uint32_t (*in_hash_key)(void const*),
                   int (*in_compare_key)(void const*, void const*),
                   void (*in_value_free)(void const* k, void* v),
                   void* (*in_mralloc)(void* ptr, size_t size, void* user),
                   void (*in_mfree)(void* ptr, void* user),
                   void* user);
void ht_table_insert(ht_hash_table_t* hash_tbl, void const* key, void* val);
void ht_table_destroy(ht_hash_table_t* hash_tbl);
void* ht_table_find(ht_hash_table_t* hash_tbl, void const* key);
int ht_table_erase(ht_hash_table_t* hash_tbl, void const* key);
int ht_table_size(ht_hash_table_t* hash_tbl);
float ht_average_probe_count(ht_hash_table_t* hash_tbl);
