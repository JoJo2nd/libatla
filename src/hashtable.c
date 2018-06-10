/********************************************************************
    Written by James Moran
    Please see the file LICENSE.txt in the repository root directory.
*********************************************************************/
/*
* Largely based on
* http://www.sebastiansylvan.com/post/robin-hood-hashing-should-be-your-default-hash-table-implementation/
* also http://www.sebastiansylvan.com/post/more-on-robin-hood-hashing-2/
*/
#include "atla/hashtable.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static const int INITIAL_SIZE = 256;
static const int LOAD_FACTOR_PERCENT = 90;

struct ht_hash_elem {
  void const* key;
  void*       value;
};

static void insert_helper(ht_hash_table_t* hash_tbl,
                          uint32_t         hash,
                          void const*      key,
                          void*            val);

static uint32_t hash_key(ht_hash_table_t* hash_tbl, void const* key) {
  uint32_t h = hash_tbl->hash_key(key);

  // MSB is used to indicate a deleted elem, so
  // clear it
  h &= 0x7fffffff;

  // Ensure that we never return 0 as a hash,
  // since we use 0 to indicate that the elem has never
  // been used at all.
  h |= h == 0;
  return h;
}

static int is_deleted(uint32_t hash) {
  // MSB set indicates that this hash is a "tombstone"
  return (hash >> 31) != 0;
}

static int desired_pos(const ht_hash_table_t* hash_tbl, uint32_t hash) {
  return hash & hash_tbl->mask;
}

static int probe_distance(const ht_hash_table_t* hash_tbl,
                          uint32_t               hash,
                          uint32_t               slot_index) {
  return (slot_index + hash_tbl->capacity - desired_pos(hash_tbl, hash)) &
         hash_tbl->mask;
}

static uint32_t elem_hash(const ht_hash_table_t* hash_tbl, int ix) {
  return hash_tbl->hashes[ix];
}

// alloc buffer according to currently set capacity
static void alloc(ht_hash_table_t* hash_tbl) {
  hash_tbl->hashes =
    (*hash_tbl->mralloc)(NULL, sizeof(uint32_t) * hash_tbl->capacity, hash_tbl->user);
  hash_tbl->buffer =
    (*hash_tbl->mralloc)(NULL, sizeof(ht_elem_t) * hash_tbl->capacity, hash_tbl->user);

  // flag all elems as hfree
  for (int i = 0; i < hash_tbl->capacity; ++i) {
    hash_tbl->hashes[i] = 0;
  }

  hash_tbl->resize_threshold = (hash_tbl->capacity * LOAD_FACTOR_PERCENT) / 100;
  hash_tbl->mask = hash_tbl->capacity - 1;
}

static void rehash(ht_hash_table_t* hash_tbl) {
  ht_elem_t* old_elems = hash_tbl->buffer;
  int        old_capacity = hash_tbl->capacity;
  uint32_t*  old_hashes = hash_tbl->hashes;
  hash_tbl->capacity *= 2;
  alloc(hash_tbl);

  // now copy over old elems
  for (int i = 0; i < old_capacity; ++i) {
    ht_elem_t* e = old_elems + i;
    uint32_t   hash = old_hashes[i];
    if (hash != 0 && !is_deleted(hash)) {
      insert_helper(hash_tbl, hash, &e->key, e->value);
    }
  }

  (*hash_tbl->mfree)(old_elems, hash_tbl->user);
  (*hash_tbl->mfree)(old_hashes, hash_tbl->user);
}

static void construct(ht_hash_table_t* hash_tbl,
                      int              ix,
                      uint32_t         hash,
                      void const*      key,
                      void*            val) {
  hash_tbl->buffer[ix].value = val;
  hash_tbl->buffer[ix].key = key;
  hash_tbl->hashes[ix] = hash;
}

static void insert_helper(ht_hash_table_t* hash_tbl,
                          uint32_t         hash,
                          void const*      key,
                          void*            val) {
  int pos = desired_pos(hash_tbl, hash);
  int dist = 0;
  for (;;) {
    if (elem_hash(hash_tbl, pos) == 0) {
      construct(hash_tbl, pos, hash, key, val);
      return;
    }

    // If the existing elem has probed less than us, then swap places with
    // existing
    // elem, and keep going to find another slot for that elem.
    int existing_elem_probe_dist =
      probe_distance(hash_tbl, elem_hash(hash_tbl, pos), pos);
    if (existing_elem_probe_dist < dist) {
      if (is_deleted(elem_hash(hash_tbl, pos))) {
        construct(hash_tbl, pos, hash, key, val);
        return;
      }

      uint32_t th = hash;
      hash = hash_tbl->hashes[pos];
      hash_tbl->hashes[pos] = th;

      void const* tmp_key = key;
      key = hash_tbl->buffer[pos].key;
      hash_tbl->buffer[pos].key = tmp_key;

      void* tv = val;
      val = hash_tbl->buffer[pos].value;
      hash_tbl->buffer[pos].value = tv;

      dist = existing_elem_probe_dist;
    }

    pos = (pos + 1) & hash_tbl->mask;
    ++dist;
  }
}

static int lookup_index(ht_hash_table_t* hash_tbl, void const* key) {
  const uint32_t hash = hash_key(hash_tbl, key);
  int            pos = desired_pos(hash_tbl, hash);
  int            dist = 0;
  for (;;) {
    uint32_t element_hash = elem_hash(hash_tbl, pos);
    if (element_hash == 0)
      return -1;
    else if (dist > probe_distance(hash_tbl, element_hash, pos))
      return -1;
    else if (element_hash == hash &&
             hash_tbl->compare_key(hash_tbl->buffer[pos].key, key) == 0)
      return pos;

    pos = (pos + 1) & hash_tbl->mask;
    ++dist;
  }
}

// public interface
void ht_table_init(ht_hash_table_t* hash_tbl,
                   uint32_t (*in_hash_key)(void const*),
                   int (*in_compare_key)(void const*, void const*),
                   void (*in_value_free)(void const* k, void* v),
                   void* (*in_mralloc)(void* ptr, size_t size, void* user),
                   void (*in_mfree)(void* ptr, void* user),
                   void* user) {
  //: buffer(nullptr), num_elems(0), capacity(INITIAL_SIZE)
  memset(hash_tbl, 0, sizeof(ht_hash_table_t));
  hash_tbl->hash_key = in_hash_key;
  hash_tbl->compare_key = in_compare_key;
  hash_tbl->value_free = in_value_free;
  hash_tbl->capacity = INITIAL_SIZE;
  hash_tbl->mralloc = in_mralloc;
  hash_tbl->mfree = in_mfree;
  hash_tbl->user = user;
  alloc(hash_tbl);
}

void ht_table_insert(ht_hash_table_t* hash_tbl, void const* key, void* val) {
  if (++hash_tbl->num_elems >= hash_tbl->resize_threshold) {
    rehash(hash_tbl);
  }
  insert_helper(hash_tbl, hash_key(hash_tbl, key), key, val);
}

void ht_table_destroy(ht_hash_table_t* hash_tbl) {
  for (int i = 0; i < hash_tbl->capacity; ++i) {
    if (elem_hash(hash_tbl, i) != 0) {
      if (hash_tbl->value_free) {
        hash_tbl->value_free(hash_tbl->buffer[i].key,
                             hash_tbl->buffer[i].value);
      }
    }
  }
  free(hash_tbl->buffer);
  free(hash_tbl->hashes);
}

void* ht_table_find(ht_hash_table_t* hash_tbl, void const* key) {
  const int ix = lookup_index(hash_tbl, key);
  return ix != -1 ? hash_tbl->buffer[ix].value : NULL;
}

int ht_table_erase(ht_hash_table_t* hash_tbl, void const* key) {
  const uint32_t hash = hash_key(hash_tbl, key);
  const int      ix = lookup_index(hash_tbl, key);

  if (ix == -1) return 0;

  if (hash_tbl->value_free) {
    hash_tbl->value_free(hash_tbl->buffer[ix].key, hash_tbl->buffer[ix].value);
  }
  hash_tbl->hashes[ix] |= 0x80000000; // mark as deleted
  --hash_tbl->num_elems;
  return 1;
}

int ht_table_size(ht_hash_table_t* hash_tbl) {
  return hash_tbl->num_elems;
}

float ht_average_probe_count(ht_hash_table_t* hash_tbl) {
  float probe_total = 0;
  for (int i = 0; i < hash_tbl->capacity; ++i) {
    uint32_t hash = elem_hash(hash_tbl, i);
    if (hash != 0 && !is_deleted(hash)) {
      probe_total += probe_distance(hash_tbl, hash, i);
    }
  }
  return probe_total / ht_table_size(hash_tbl) + 1.0f;
}